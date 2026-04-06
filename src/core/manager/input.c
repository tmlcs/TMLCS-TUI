#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/layout.h"
#include "core/theme.h"
#include "core/help.h"
#include "core/types_private.h"
#include "core/widget.h"
#include "widget/text_input.h"
#include "widget/button.h"
#include "widget/checkbox.h"
#include "widget/list.h"
#include "widget/context_menu.h"
#include <string.h>
#include <stdio.h>

/* Forward declarations for widget type registration */
void tui_text_input_ensure_registered(void);
int tui_text_input_get_type_id(void);
void tui_textarea_ensure_registered(void);
int tui_textarea_get_type_id(void);
void tui_label_ensure_registered(void);
int tui_label_get_type_id(void);
void tui_progress_ensure_registered(void);
int tui_progress_get_type_id(void);

/* ------------------------------------------------------------------ */
/*  Helpers - static, file-scope only                                  */
/* ------------------------------------------------------------------ */

/**
 * @brief Handle click on the tab bar (y=0).
 * @return true if a tab was clicked and activated.
 */
static bool process_tab_click(TuiManager* mgr, const struct ncinput* ni) {
    if (mgr->_active_workspace_index == -1) return false;
    int clicked = tui_manager_get_tab_at(mgr, ni->y, ni->x);
    if (clicked == -1) return false;
    TuiWorkspace* ws = mgr->_workspaces[mgr->_active_workspace_index];
    tui_workspace_set_active_tab(ws, clicked);
    return true;
}

/**
 * @brief Handle click within a tab's viewport (window focus, close, resize, drag).
 * @return true if any window interaction was initiated.
 */
static bool process_window_click(TuiManager* mgr, TuiTab* tab, uint32_t key, const struct ncinput* ni) {
    int wly, wlx;
    TuiWindow* win = tui_tab_get_window_at(tab, ni->y, ni->x, &wly, &wlx);
    if (!win) return false;

    /* Update active window index */
    for (int i = 0; i < tab->_window_count; i++) {
        if (tab->_windows[i] == win) {
            tab->_active_window_index = i;
            break;
        }
    }

    ncplane_move_top(win->_plane);
    win->_needs_redraw = true;

    /* --- Try widget mouse handling first --- */
    if (key == NCKEY_BUTTON1 && ni->evtype == NCTYPE_PRESS) {
        int wtype_id = 0;
        void* widget = tui_window_get_widget_at(win, wly, wlx, &wtype_id);
        if (widget) {
            bool consumed = tui_widget_handle_mouse(wtype_id, widget, key, ni);
            if (consumed) return true;
        }
    }

    unsigned dimy, dimx;
    ncplane_dim_yx(win->_plane, &dimy, &dimx);

    /* Close button zone: top row, rightmost CLOSE_BUTTON_AREA_MIN_X columns */
    if (wly == 0 && wlx >= (int)dimx - CLOSE_BUTTON_AREA_MIN_X && wlx <= (int)dimx - 1) {
        if (mgr->_dragged_window == win) mgr->_dragged_window = NULL;
        if (mgr->_resizing_window == win) mgr->_resizing_window = NULL;
        tui_tab_remove_window(tab, win);
        tui_window_destroy(win);
        return true;
    }

    /* Resize grip zone: bottom row, rightmost GRIP_AREA_START_X columns */
    if (wly == (int)dimy - 1 && wlx >= (int)dimx - GRIP_AREA_START_X && wlx <= (int)dimx - 1) {
        mgr->_resizing_window = win;
        mgr->_resize_start_dimy = dimy;
        mgr->_resize_start_dimx = dimx;
        mgr->_resize_start_mouse_y = ni->y;
        mgr->_resize_start_mouse_x = ni->x;
        return true;
    }

    /* Title bar drag (y=0, excluding close button zone) */
    if (wly == 0) {
        mgr->_dragged_window = win;
        mgr->_drag_start_wly = wly;
        mgr->_drag_start_wlx = wlx;
        return true;
    }

    return true;  /* Window focused, no special action */
}

/**
 * @brief Handle mouse motion during window resize.
 */
static void process_resize_motion(TuiManager* mgr, const struct ncinput* ni) {
    TuiWindow* rw = mgr->_resizing_window;
    if (!rw) return;

    int delta_y = ni->y - mgr->_resize_start_mouse_y;
    int delta_x = ni->x - mgr->_resize_start_mouse_x;

    unsigned new_dimy = mgr->_resize_start_dimy + delta_y;
    unsigned new_dimx = mgr->_resize_start_dimx + delta_x;

    if (new_dimy < WINDOW_MIN_HEIGHT) new_dimy = WINDOW_MIN_HEIGHT;
    if (new_dimx < WINDOW_MIN_WIDTH) new_dimx = WINDOW_MIN_WIDTH;

    struct ncplane* parent = ncplane_parent(rw->_plane);
    unsigned p_dimy, p_dimx;
    ncplane_dim_yx(parent, &p_dimy, &p_dimx);
    int wp_y, wp_x;
    ncplane_yx(rw->_plane, &wp_y, &wp_x);

    if (wp_y + new_dimy > p_dimy) new_dimy = p_dimy - wp_y;
    if (wp_x + new_dimx > p_dimx) new_dimx = p_dimx - wp_x;

    ncplane_resize_simple(rw->_plane, new_dimy, new_dimx);

    /* Reposition text input if present */
    if (rw->_text_input) {
        TuiTextInput* ti = (TuiTextInput*)rw->_text_input;
        int inner_width = (int)new_dimx - INPUT_WIDTH_REDUCTION;
        if (inner_width < MIN_TEXT_INPUT_WIDTH) inner_width = MIN_TEXT_INPUT_WIDTH;
        ncplane_resize_simple(ti->plane, 1, (unsigned)inner_width);
    }

    if (rw->_render_cb) rw->_render_cb(rw);
}

/**
 * @brief Handle mouse motion during window drag.
 */
static void process_drag_motion(TuiManager* mgr, const struct ncinput* ni) {
    TuiWindow* win = mgr->_dragged_window;
    if (!win) return;

    struct ncplane* parent = ncplane_parent(win->_plane);
    int p_abs_y, p_abs_x;
    unsigned p_dimy_u, p_dimx_u, w_dimy_u, w_dimx_u;

    ncplane_abs_yx(parent, &p_abs_y, &p_abs_x);
    ncplane_dim_yx(parent, &p_dimy_u, &p_dimx_u);
    ncplane_dim_yx(win->_plane, &w_dimy_u, &w_dimx_u);

    int p_dimy = (int)p_dimy_u, p_dimx = (int)p_dimx_u;
    int w_dimy = (int)w_dimy_u, w_dimx = (int)w_dimx_u;

    int new_rel_y = ni->y - p_abs_y - mgr->_drag_start_wly;
    int new_rel_x = ni->x - p_abs_x - mgr->_drag_start_wlx;

    /* Clamping */
    if (new_rel_y < 0) new_rel_y = 0;
    if (new_rel_y + w_dimy > p_dimy) new_rel_y = p_dimy - w_dimy;
    if (new_rel_y < 0) new_rel_y = 0;
    if (new_rel_x < 0) new_rel_x = 0;
    if (new_rel_x + w_dimx > p_dimx) new_rel_x = p_dimx - w_dimx;
    if (new_rel_x < 0) new_rel_x = 0;

    ncplane_move_yx(win->_plane, new_rel_y, new_rel_x);
}

/* ------------------------------------------------------------------ */
/*  Public API                                                        */
/* ------------------------------------------------------------------ */

int tui_manager_get_tab_at(TuiManager* mgr, int y, int x) {
    if (y != 0) return -1;
    if (mgr->_active_workspace_index == -1) return -1;

    TuiWorkspace* ws = mgr->_workspaces[mgr->_active_workspace_index];
    int offset_x = TAB_BAR_START_X;
    for (int i = 0; i < ws->_tab_count; i++) {
        int len = strlen(ws->_tabs[i]->_name) + TAB_LABEL_PADDING;
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
        if (mgr->_active_workspace_index == -1) return true;
        TuiWorkspace* ws = mgr->_workspaces[mgr->_active_workspace_index];
        if (ws->_tab_count == 0) return true;

        if (key == NCKEY_SCROLL_UP) {
            /* Next tab */
            int next = (ws->_active_tab_index + 1) % ws->_tab_count;
            tui_workspace_set_active_tab(ws, next);
        } else {
            /* Previous tab */
            int prev = (ws->_active_tab_index - 1 + ws->_tab_count) % ws->_tab_count;
            tui_workspace_set_active_tab(ws, prev);
        }
        tui_manager_mark_toolbar_dirty(mgr);
        tui_manager_mark_tabs_dirty(mgr);
        return true;
    }

    /* 1. Button press */
    if (key == NCKEY_BUTTON1 && ni->evtype == NCTYPE_PRESS) {
        /* Tab bar click (y=0) */
        if (ni->y == 0 && process_tab_click(mgr, ni)) return true;

        /* Window interaction */
        if (mgr->_active_workspace_index != -1) {
            TuiWorkspace* ws = mgr->_workspaces[mgr->_active_workspace_index];
            if (ws->_active_tab_index != -1) {
                TuiTab* tab = ws->_tabs[ws->_active_tab_index];
                if (process_window_click(mgr, tab, key, ni)) return true;
            }
        }
    }

    /* 2. Button release */
    if (key == NCKEY_BUTTON1 && ni->evtype == NCTYPE_RELEASE) {
        mgr->_dragged_window = NULL;
        mgr->_resizing_window = NULL;
        return true;
    }

    /* 3. Resize motion */
    if (mgr->_resizing_window != NULL && (key == NCKEY_BUTTON1 || key == NCKEY_MOTION)
        && ni->evtype != NCTYPE_RELEASE) {
        process_resize_motion(mgr, ni);
        return true;
    }

    /* 4. Drag motion */
    if (mgr->_dragged_window != NULL && (key == NCKEY_BUTTON1 || key == NCKEY_MOTION)
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
    /* For keyboard, evtype is NCTYPE_UNKNOWN. Only discard RELEASE (exclusive to mouse). */
    if (ni->evtype == NCTYPE_RELEASE) return false;

    /* Terminal resize event: mark all layouts dirty for recomputation */
    if (key == NCKEY_RESIZE) {
        tui_manager_mark_full_redraw(mgr);
        if (mgr->_active_workspace_index >= 0) {
            TuiWorkspace* ws = mgr->_workspaces[mgr->_active_workspace_index];
            if (ws && ws->_active_tab_index >= 0) {
                TuiTab* tab = ws->_tabs[ws->_active_tab_index];
                for (int i = 0; i < tab->_window_count; i++) {
                    if (tab->_windows[i]->_attached_layout) {
                        tui_layout_mark_dirty(tab->_windows[i]->_attached_layout);
                    }
                }
            }
        }
        return true;
    }

    /* Help screen toggle */
    if (key == 0x1003F || key == '?') {  /* 0x1003F = NCKEY_F1 */
        if (tui_help_is_visible()) {
            tui_help_hide();
        } else {
            tui_help_show(mgr->_stdplane);
        }
        return true;
    }

    /* Dismiss help with any key */
    if (tui_help_is_visible()) {
        tui_help_hide();
        return true;
    }

    /* Early workspace resolution for handlers that need it */
    TuiWorkspace* ws = (mgr->_active_workspace_index != -1) ? mgr->_workspaces[mgr->_active_workspace_index] : NULL;

    /* New Tab: Ctrl+T */
    if (ni->ctrl && (key == 't' || key == 'T')) {
        if (ws && ws->_tab_count < 10) {
            char name[32];
            snprintf(name, sizeof(name), "Tab %d", ws->_tab_count + 1);
            TuiTab* new_tab = tui_tab_create(ws, name);
            if (new_tab) tui_workspace_add_tab(ws, new_tab);
            tui_manager_mark_toolbar_dirty(mgr);
            tui_manager_mark_tabs_dirty(mgr);
        }
        return true;
    }

    /* New Window: Ctrl+N */
    if (ni->ctrl && (key == 'n' || key == 'N')) {
        if (ws && ws->_active_tab_index != -1) {
            TuiTab* tab = ws->_tabs[ws->_active_tab_index];
            if (tab->_window_count < 8) {
                TuiWindow* new_win = tui_window_create(tab, 40, 10);
                if (new_win) {
                    new_win->_user_data = "New Window";
                    tui_tab_add_window(tab, new_win);
                    ncplane_move_yx(new_win->_plane, 2, 2);
                    new_win->_needs_redraw = true;
                }
            }
        }
        return true;
    }

    /* Escape: dismiss help, cancel operations */
    if (key == 0x1B) {  /* ESC */
        if (tui_help_is_visible()) {
            tui_help_hide();
            return true;
        }
        if (mgr->_dragged_window) { mgr->_dragged_window = NULL; return true; }
        if (mgr->_resizing_window) { mgr->_resizing_window = NULL; return true; }
        return false;
    }

    /* Quit App */
    if (key == 'q' || key == 'Q') {
        mgr->_running = false;
        return true;
    }

    /* Workspaces: Alt + 1..9 */
    if (ni->alt && key >= '1' && key <= '9') {
        int index = key - '1';
        tui_manager_set_active_workspace(mgr, index);
        return true;
    }

    if (mgr->_active_workspace_index == -1) return false;
    if (!ws) return false;

    /* Tabs: Left / Right Arrows */
    if (key == NCKEY_RIGHT) {
        if (ws->_tab_count > 0) {
            int next = (ws->_active_tab_index + 1) % ws->_tab_count;
            tui_workspace_set_active_tab(ws, next);
            tui_manager_mark_toolbar_dirty(mgr);
            tui_manager_mark_tabs_dirty(mgr);
        }
        return true;
    }

    if (key == NCKEY_LEFT) {
         if (ws->_tab_count > 0) {
            int prev = (ws->_active_tab_index - 1 + ws->_tab_count) % ws->_tab_count;
            tui_workspace_set_active_tab(ws, prev);
            tui_manager_mark_toolbar_dirty(mgr);
            tui_manager_mark_tabs_dirty(mgr);
        }
        return true;
    }

    /* Keyboard window resize: Ctrl+Shift+Arrows */
    if (ni->ctrl && ni->shift) {
        if (ws->_active_tab_index != -1) {
            TuiTab* tab = ws->_tabs[ws->_active_tab_index];
            if (tab->_active_window_index != -1) {
                TuiWindow* win = tab->_windows[tab->_active_window_index];
                int wp_y, wp_x;
                unsigned w_dimy, w_dimx;
                ncplane_yx(win->_plane, &wp_y, &wp_x);
                ncplane_dim_yx(win->_plane, &w_dimy, &w_dimx);

                if (key == NCKEY_UP && w_dimy + 1 <= 50) {
                    ncplane_resize_simple(win->_plane, w_dimy + 1, w_dimx);
                    if (win->_render_cb) win->_render_cb(win);
                } else if (key == NCKEY_DOWN && w_dimy > WINDOW_MIN_HEIGHT) {
                    ncplane_resize_simple(win->_plane, w_dimy - 1, w_dimx);
                    if (win->_render_cb) win->_render_cb(win);
                } else if (key == NCKEY_RIGHT && w_dimx + 1 <= 100) {
                    ncplane_resize_simple(win->_plane, w_dimy, w_dimx + 1);
                    if (win->_render_cb) win->_render_cb(win);
                } else if (key == NCKEY_LEFT && w_dimx > WINDOW_MIN_WIDTH) {
                    ncplane_resize_simple(win->_plane, w_dimy, w_dimx - 1);
                    if (win->_render_cb) win->_render_cb(win);
                }
                return true;
            }
        }
    }

    /* Window Focus: Tab (forward), Shift+Tab (reverse) */
    if (key == NCKEY_TAB) {
        if (ws->_active_tab_index != -1) {
            TuiTab* tab = ws->_tabs[ws->_active_tab_index];
            if (tab->_window_count > 0) {
                TuiWindow* focused_win = tab->_windows[tab->_active_window_index];
                if (!focused_win) return true;

                /* If window has widgets, traverse widget focus */
                if (focused_win->_widget_count > 0) {
                    int current = focused_win->_focused_widget_index;
                    int next = -1;
                    int count = focused_win->_widget_count;

                    for (int i = 0; i < count; i++) {
                        int idx;
                        if (ni->shift) {
                            idx = (current - 1 - i + count) % count;
                        } else {
                            idx = (current + 1 + i) % count;
                        }
                        if (idx < 0) idx += count;
                        int type_id = focused_win->_widget_type_ids[idx];
                        void* w = focused_win->_widgets[idx];
                        if (w && tui_widget_is_focusable(type_id, w)) {
                            next = idx;
                            break;
                        }
                    }

                    if (next >= 0) {
                        /* Blur current */
                        if (current >= 0) {
                            int cur_type_id = focused_win->_widget_type_ids[current];
                            const TuiWidgetIface* iface = tui_widget_get_iface(cur_type_id);
                            if (iface && iface->on_blur) {
                                iface->on_blur(focused_win->_widgets[current]);
                            }
                        }

                        /* Focus next */
                        focused_win->_focused_widget_index = next;
                        int type_id = focused_win->_widget_type_ids[next];
                        const TuiWidgetIface* iface = tui_widget_get_iface(type_id);
                        if (iface && iface->on_focus) {
                            iface->on_focus(focused_win->_widgets[next]);
                        }
                        tui_window_mark_dirty(focused_win);
                        return true;
                    }
                }

                /* Fall back to window-level cycling */
                if (ni->shift) {
                    int prev = (tab->_active_window_index - 1 + tab->_window_count) % tab->_window_count;
                    tab->_active_window_index = prev;
                } else {
                    int next = (tab->_active_window_index + 1) % tab->_window_count;
                    tab->_active_window_index = next;
                }
                ncplane_move_top(tab->_windows[tab->_active_window_index]->_plane);
                /* Mark all windows for redraw (focus ring changes) */
                for (int i = 0; i < tab->_window_count; i++) {
                    tab->_windows[i]->_needs_redraw = true;
                }
            }
        }
        return true;
    }

    /* Window Close: 'x' */
    if (key == 'x') {
        if (ws->_active_tab_index != -1) {
            TuiTab* tab = ws->_tabs[ws->_active_tab_index];
            if (tab->_active_window_index != -1) {
                TuiWindow* win = tab->_windows[tab->_active_window_index];
                /* Nullify dangling pointers before freeing the window */
                if (mgr->_dragged_window  == win) mgr->_dragged_window  = NULL;
                if (mgr->_resizing_window == win) mgr->_resizing_window = NULL;
                tui_tab_remove_window(tab, win);
                tui_window_destroy(win);
            }
        }
        return true;
    }

    /* Tab Close: 'w' */
    if (key == 'w') {
        tui_workspace_remove_active_tab(ws);
        return true;
    }

    /* Workspace Close: 'd' */
    if (key == 'd') {
        tui_manager_remove_active_workspace(mgr);
        return true;
    }

    return false;
}
