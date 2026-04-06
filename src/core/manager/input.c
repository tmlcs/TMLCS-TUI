#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/help.h"
#include "widget/text_input.h"
#include <string.h>
#include <stdio.h>

/* ------------------------------------------------------------------ */
/*  Helpers – static, file-scope only                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief Handle click on the tab bar (y=0).
 * @return true if a tab was clicked and activated.
 */
static bool process_tab_click(TuiManager* mgr, const struct ncinput* ni) {
    if (mgr->active_workspace_index == -1) return false;
    int clicked = tui_manager_get_tab_at(mgr, ni->y, ni->x);
    if (clicked == -1) return false;
    TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
    tui_workspace_set_active_tab(ws, clicked);
    return true;
}

/**
 * @brief Handle click within a tab's viewport (window focus, close, resize, drag).
 * @return true if any window interaction was initiated.
 */
static bool process_window_click(TuiManager* mgr, TuiTab* tab, const struct ncinput* ni) {
    int wly, wlx;
    TuiWindow* win = tui_tab_get_window_at(tab, ni->y, ni->x, &wly, &wlx);
    if (!win) return false;

    /* Update active window index */
    for (int i = 0; i < tab->window_count; i++) {
        if (tab->windows[i] == win) {
            tab->active_window_index = i;
            break;
        }
    }

    ncplane_move_top(win->plane);
    win->needs_redraw = true;

    unsigned dimy, dimx;
    ncplane_dim_yx(win->plane, &dimy, &dimx);

    /* Close button zone: top row, rightmost CLOSE_BUTTON_AREA_MIN_X columns */
    if (wly == 0 && wlx >= (int)dimx - CLOSE_BUTTON_AREA_MIN_X && wlx <= (int)dimx - 1) {
        if (mgr->dragged_window == win) mgr->dragged_window = NULL;
        if (mgr->resizing_window == win) mgr->resizing_window = NULL;
        tui_tab_remove_window(tab, win);
        tui_window_destroy(win);
        return true;
    }

    /* Resize grip zone: bottom row, rightmost GRIP_AREA_START_X columns */
    if (wly == (int)dimy - 1 && wlx >= (int)dimx - GRIP_AREA_START_X && wlx <= (int)dimx - 1) {
        mgr->resizing_window = win;
        mgr->resize_start_dimy = dimy;
        mgr->resize_start_dimx = dimx;
        mgr->resize_start_mouse_y = ni->y;
        mgr->resize_start_mouse_x = ni->x;
        return true;
    }

    /* Title bar drag (y=0, excluding close button zone) */
    if (wly == 0) {
        mgr->dragged_window = win;
        mgr->drag_start_wly = wly;
        mgr->drag_start_wlx = wlx;
        return true;
    }

    return true;  /* Window focused, no special action */
}

/**
 * @brief Handle mouse motion during window resize.
 */
static void process_resize_motion(TuiManager* mgr, const struct ncinput* ni) {
    TuiWindow* rw = mgr->resizing_window;
    if (!rw) return;

    int delta_y = ni->y - mgr->resize_start_mouse_y;
    int delta_x = ni->x - mgr->resize_start_mouse_x;

    unsigned new_dimy = mgr->resize_start_dimy + delta_y;
    unsigned new_dimx = mgr->resize_start_dimx + delta_x;

    if (new_dimy < WINDOW_MIN_HEIGHT) new_dimy = WINDOW_MIN_HEIGHT;
    if (new_dimx < WINDOW_MIN_WIDTH) new_dimx = WINDOW_MIN_WIDTH;

    struct ncplane* parent = ncplane_parent(rw->plane);
    unsigned p_dimy, p_dimx;
    ncplane_dim_yx(parent, &p_dimy, &p_dimx);
    int wp_y, wp_x;
    ncplane_yx(rw->plane, &wp_y, &wp_x);

    if (wp_y + new_dimy > p_dimy) new_dimy = p_dimy - wp_y;
    if (wp_x + new_dimx > p_dimx) new_dimx = p_dimx - wp_x;

    ncplane_resize_simple(rw->plane, new_dimy, new_dimx);

    /* Reposition text input if present */
    if (rw->text_input) {
        TuiTextInput* ti = (TuiTextInput*)rw->text_input;
        int inner_width = (int)new_dimx - INPUT_WIDTH_REDUCTION;
        if (inner_width < MIN_TEXT_INPUT_WIDTH) inner_width = MIN_TEXT_INPUT_WIDTH;
        ncplane_resize_simple(ti->plane, 1, (unsigned)inner_width);
    }

    if (rw->render_cb) rw->render_cb(rw);
}

/**
 * @brief Handle mouse motion during window drag.
 */
static void process_drag_motion(TuiManager* mgr, const struct ncinput* ni) {
    TuiWindow* win = mgr->dragged_window;
    if (!win) return;

    struct ncplane* parent = ncplane_parent(win->plane);
    int p_abs_y, p_abs_x;
    unsigned p_dimy_u, p_dimx_u, w_dimy_u, w_dimx_u;

    ncplane_abs_yx(parent, &p_abs_y, &p_abs_x);
    ncplane_dim_yx(parent, &p_dimy_u, &p_dimx_u);
    ncplane_dim_yx(win->plane, &w_dimy_u, &w_dimx_u);

    int p_dimy = (int)p_dimy_u, p_dimx = (int)p_dimx_u;
    int w_dimy = (int)w_dimy_u, w_dimx = (int)w_dimx_u;

    int new_rel_y = ni->y - p_abs_y - mgr->drag_start_wly;
    int new_rel_x = ni->x - p_abs_x - mgr->drag_start_wlx;

    /* Clamping */
    if (new_rel_y < 0) new_rel_y = 0;
    if (new_rel_y + w_dimy > p_dimy) new_rel_y = p_dimy - w_dimy;
    if (new_rel_y < 0) new_rel_y = 0;
    if (new_rel_x < 0) new_rel_x = 0;
    if (new_rel_x + w_dimx > p_dimx) new_rel_x = p_dimx - w_dimx;
    if (new_rel_x < 0) new_rel_x = 0;

    ncplane_move_yx(win->plane, new_rel_y, new_rel_x);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

int tui_manager_get_tab_at(TuiManager* mgr, int y, int x) {
    if (y != 0) return -1;
    if (mgr->active_workspace_index == -1) return -1;

    TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
    int offset_x = TAB_BAR_START_X;
    for (int i = 0; i < ws->tab_count; i++) {
        int len = strlen(ws->tabs[i]->name) + TAB_LABEL_PADDING;
        if (x >= offset_x && x < offset_x + len) {
            return i;
        }
        offset_x += len;
    }
    return -1;
}

bool tui_manager_process_mouse(TuiManager* mgr, uint32_t key, const struct ncinput* ni) {
    /* Scroll wheel support */
    if (key == NCKEY_SCROLL_UP || key == NCKEY_SCROLL_DOWN) {
        if (mgr->active_workspace_index == -1) return true;
        TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
        if (ws->tab_count == 0) return true;

        if (key == NCKEY_SCROLL_UP) {
            /* Next tab */
            int next = (ws->active_tab_index + 1) % ws->tab_count;
            tui_workspace_set_active_tab(ws, next);
        } else {
            /* Previous tab */
            int prev = (ws->active_tab_index - 1 + ws->tab_count) % ws->tab_count;
            tui_workspace_set_active_tab(ws, prev);
        }
        return true;
    }

    /* 1. Button press */
    if (key == NCKEY_BUTTON1 && ni->evtype == NCTYPE_PRESS) {
        /* Tab bar click (y=0) */
        if (ni->y == 0 && process_tab_click(mgr, ni)) return true;

        /* Window interaction */
        if (mgr->active_workspace_index != -1) {
            TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
            if (ws->active_tab_index != -1) {
                TuiTab* tab = ws->tabs[ws->active_tab_index];
                if (process_window_click(mgr, tab, ni)) return true;
            }
        }
    }

    /* 2. Button release */
    if (key == NCKEY_BUTTON1 && ni->evtype == NCTYPE_RELEASE) {
        mgr->dragged_window = NULL;
        mgr->resizing_window = NULL;
        return true;
    }

    /* 3. Resize motion */
    if (mgr->resizing_window != NULL && (key == NCKEY_BUTTON1 || key == NCKEY_MOTION)
        && ni->evtype != NCTYPE_RELEASE) {
        process_resize_motion(mgr, ni);
        return true;
    }

    /* 4. Drag motion */
    if (mgr->dragged_window != NULL && (key == NCKEY_BUTTON1 || key == NCKEY_MOTION)
        && ni->evtype != NCTYPE_RELEASE) {
        process_drag_motion(mgr, ni);
        return true;
    }

    /* Filter generic mouse events */
    if (key >= NCKEY_BUTTON1 && key <= MAX_MOUSE_BUTTON) return true;
    if (key == NCKEY_MOTION) return true;

    /* Update hover position */
    tui_manager_update_hover(ni->y, ni->x);

    return false;
}

bool tui_manager_process_keyboard(TuiManager* mgr, uint32_t key, const struct ncinput* ni) {
    // Para teclado, evtype es NCTYPE_UNKNOWN. Solo descartar RELEASE (exclusivo de raton)
    if (ni->evtype == NCTYPE_RELEASE) return false;

    // Help screen toggle
    if (key == 0x1003F || key == '?') {  /* 0x1003F = NCKEY_F1 */
        if (tui_help_is_visible()) {
            tui_help_hide();
        } else {
            tui_help_show(mgr->stdplane);
        }
        return true;
    }

    // Dismiss help with any key
    if (tui_help_is_visible()) {
        tui_help_hide();
        return true;
    }

    // Early workspace resolution for handlers that need it
    TuiWorkspace* ws = (mgr->active_workspace_index != -1) ? mgr->workspaces[mgr->active_workspace_index] : NULL;

    // New Tab: Ctrl+T
    if (ni->ctrl && (key == 't' || key == 'T')) {
        if (ws && ws->tab_count < 10) {
            char name[32];
            snprintf(name, sizeof(name), "Tab %d", ws->tab_count + 1);
            TuiTab* new_tab = tui_tab_create(ws, name);
            if (new_tab) tui_workspace_add_tab(ws, new_tab);
        }
        return true;
    }

    // New Window: Ctrl+N
    if (ni->ctrl && (key == 'n' || key == 'N')) {
        if (ws && ws->active_tab_index != -1) {
            TuiTab* tab = ws->tabs[ws->active_tab_index];
            if (tab->window_count < 8) {
                TuiWindow* new_win = tui_window_create(tab, 40, 10);
                if (new_win) {
                    new_win->user_data = "New Window";
                    tui_tab_add_window(tab, new_win);
                    ncplane_move_yx(new_win->plane, 2, 2);
                }
            }
        }
        return true;
    }

    // Escape: dismiss help, cancel operations
    if (key == 0x1B) {  /* ESC */
        if (tui_help_is_visible()) {
            tui_help_hide();
            return true;
        }
        if (mgr->dragged_window) { mgr->dragged_window = NULL; return true; }
        if (mgr->resizing_window) { mgr->resizing_window = NULL; return true; }
        return false;
    }

    // Quitar App
    if (key == 'q' || key == 'Q') {
        mgr->running = false;
        return true;
    }

    // Workspaces: Alt + 1..9
    if (ni->alt && key >= '1' && key <= '9') {
        int index = key - '1';
        tui_manager_set_active_workspace(mgr, index);
        return true;
    }

    if (mgr->active_workspace_index == -1) return false;
    if (!ws) return false;

    // Tabs: Left / Right Arrows
    if (key == NCKEY_RIGHT) {
        if (ws->tab_count > 0) {
            int next = (ws->active_tab_index + 1) % ws->tab_count;
            tui_workspace_set_active_tab(ws, next);
        }
        return true;
    }
    
    if (key == NCKEY_LEFT) {
         if (ws->tab_count > 0) {
            int prev = (ws->active_tab_index - 1 + ws->tab_count) % ws->tab_count;
            tui_workspace_set_active_tab(ws, prev);
        }
        return true;
    }

    // Keyboard window resize: Ctrl+Shift+Arrows
    if (ni->ctrl && ni->shift) {
        if (ws->active_tab_index != -1) {
            TuiTab* tab = ws->tabs[ws->active_tab_index];
            if (tab->active_window_index != -1) {
                TuiWindow* win = tab->windows[tab->active_window_index];
                int wp_y, wp_x;
                unsigned w_dimy, w_dimx;
                ncplane_yx(win->plane, &wp_y, &wp_x);
                ncplane_dim_yx(win->plane, &w_dimy, &w_dimx);

                if (key == NCKEY_UP && w_dimy + 1 <= 50) {
                    ncplane_resize_simple(win->plane, w_dimy + 1, w_dimx);
                    if (win->render_cb) win->render_cb(win);
                } else if (key == NCKEY_DOWN && w_dimy > WINDOW_MIN_HEIGHT) {
                    ncplane_resize_simple(win->plane, w_dimy - 1, w_dimx);
                    if (win->render_cb) win->render_cb(win);
                } else if (key == NCKEY_RIGHT && w_dimx + 1 <= 100) {
                    ncplane_resize_simple(win->plane, w_dimy, w_dimx + 1);
                    if (win->render_cb) win->render_cb(win);
                } else if (key == NCKEY_LEFT && w_dimx > WINDOW_MIN_WIDTH) {
                    ncplane_resize_simple(win->plane, w_dimy, w_dimx - 1);
                    if (win->render_cb) win->render_cb(win);
                }
                return true;
            }
        }
    }

    // Windows Foco: Tab (forward), Shift+Tab (reverse)
    if (key == NCKEY_TAB) {
        if (ws->active_tab_index != -1) {
            TuiTab* tab = ws->tabs[ws->active_tab_index];
            if (tab->window_count > 0) {
                if (ni->shift) {
                    /* Reverse cycle */
                    int prev = (tab->active_window_index - 1 + tab->window_count) % tab->window_count;
                    tab->active_window_index = prev;
                } else {
                    /* Forward cycle */
                    int next = (tab->active_window_index + 1) % tab->window_count;
                    tab->active_window_index = next;
                }
                ncplane_move_top(tab->windows[tab->active_window_index]->plane);
            }
        }
        return true;
    }
    
    // Windows Cierre Local: 'x'
    if (key == 'x') {
        if (ws->active_tab_index != -1) {
            TuiTab* tab = ws->tabs[ws->active_tab_index];
            if (tab->active_window_index != -1) {
                TuiWindow* win = tab->windows[tab->active_window_index];
                // Anular punteros colgantes antes de liberar la ventana
                if (mgr->dragged_window  == win) mgr->dragged_window  = NULL;
                if (mgr->resizing_window == win) mgr->resizing_window = NULL;
                tui_tab_remove_window(tab, win);
                tui_window_destroy(win);
            }
        }
        return true;
    }
    
    // Cierre de Pestaña Local: 'w'
    if (key == 'w') {
        tui_workspace_remove_active_tab(ws);
        return true;
    }
    
    // Cierre de Escritorio: 'd'
    if (key == 'd') {
        tui_manager_remove_active_workspace(mgr);
        return true;
    }

    return false;
}
