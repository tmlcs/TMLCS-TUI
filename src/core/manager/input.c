#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include <string.h>

int tui_manager_get_tab_at(TuiManager* mgr, int y, int x) {
    if (y != 0) return -1;
    if (mgr->active_workspace_index == -1) return -1;
    
    TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
    int offset_x = 30; // Correspondiente al render
    for (int i = 0; i < ws->tab_count; i++) {
        int len = strlen(ws->tabs[i]->name) + 6;
        if (x >= offset_x && x < offset_x + len) {
            return i;
        }
        offset_x += len;
    }
    return -1;
}

bool tui_manager_process_mouse(TuiManager* mgr, uint32_t key, const struct ncinput* ni) {
    // 1. Detección Mouse Click y Drag
    if (key == NCKEY_BUTTON1) {
        if (ni->evtype == NCTYPE_PRESS) {
            int clicked_tab = tui_manager_get_tab_at(mgr, ni->y, ni->x);
            if (clicked_tab != -1) {
                TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
                tui_workspace_set_active_tab(ws, clicked_tab);
                return true;
            } else if (mgr->active_workspace_index != -1) {
                TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
                if (ws->active_tab_index != -1) {
                    TuiTab* tab = ws->tabs[ws->active_tab_index];
                    int wly, wlx;
                    TuiWindow* win = tui_tab_get_window_at(tab, ni->y, ni->x, &wly, &wlx);
                    if (win) {
                        // Sincronizar indice visual para aplicar contraste alto en el Taskbar
                        for (int i = 0; i < tab->window_count; i++) {
                            if (tab->windows[i] == win) {
                                tab->active_window_index = i;
                                break;
                            }
                        }
                        
                        ncplane_move_top(win->plane);
                        unsigned dimy, dimx;
                        ncplane_dim_yx(win->plane, &dimy, &dimx);
                        
                        if (wly == 0 && wlx == (int)dimx - 2) {
                            // Click exacto en [X] - anular punteros colgantes antes de liberar
                            if (mgr->dragged_window == win)  mgr->dragged_window  = NULL;
                            if (mgr->resizing_window == win) mgr->resizing_window = NULL;
                            tui_tab_remove_window(tab, win);
                            tui_window_destroy(win);
                            return true;
                        } else if (wly == (int)dimy - 1 && wlx >= (int)dimx - 3 && wlx <= (int)dimx - 1) {
                            // Click ancla inferior ⌟ - Empezar Redimensionado
                            mgr->resizing_window = win;
                            mgr->resize_start_dimy = dimy;
                            mgr->resize_start_dimx = dimx;
                            mgr->resize_start_mouse_y = ni->y;
                            mgr->resize_start_mouse_x = ni->x;
                        } else if (wly == 0) { // Título de la ventana
                            mgr->dragged_window = win;
                            mgr->drag_start_wly = wly;
                            mgr->drag_start_wlx = wlx;
                        }
                    }
                }
            }
        } else if (ni->evtype == NCTYPE_RELEASE) {
            mgr->dragged_window = NULL;
            mgr->resizing_window = NULL;
        }
    }
    
    // 2. Procesar Redimensionado de Ventana (Resize)
    if (mgr->resizing_window != NULL && (key == NCKEY_BUTTON1 || key == NCKEY_MOTION)) {
        if (ni->evtype != NCTYPE_RELEASE) {
            int delta_y = ni->y - mgr->resize_start_mouse_y;
            int delta_x = ni->x - mgr->resize_start_mouse_x;
            
            unsigned new_dimy = mgr->resize_start_dimy + delta_y;
            unsigned new_dimx = mgr->resize_start_dimx + delta_x;
            
            if (new_dimy < 4) new_dimy = 4;
            if (new_dimx < 15) new_dimx = 15;
            
            struct ncplane* parent = ncplane_parent(mgr->resizing_window->plane);
            unsigned p_dimy, p_dimx;
            ncplane_dim_yx(parent, &p_dimy, &p_dimx);
            int wp_y, wp_x;
            ncplane_yx(mgr->resizing_window->plane, &wp_y, &wp_x);
            
            if (wp_y + new_dimy > p_dimy) new_dimy = p_dimy - wp_y;
            if (wp_x + new_dimx > p_dimx) new_dimx = p_dimx - wp_x;
            
            ncplane_resize_simple(mgr->resizing_window->plane, new_dimy, new_dimx);
            return true;
        }
    }
    
    // 3. Procesar Arrastre y Clamping
    if (mgr->dragged_window != NULL && (key == NCKEY_BUTTON1 || key == NCKEY_MOTION)) {
        if (ni->evtype != NCTYPE_RELEASE) {
            struct ncplane* parent = ncplane_parent(mgr->dragged_window->plane);
            int p_abs_y, p_abs_x;
            unsigned p_dimy, p_dimx, w_dimy, w_dimx;
            
            ncplane_abs_yx(parent, &p_abs_y, &p_abs_x);
            ncplane_dim_yx(parent, &p_dimy, &p_dimx);
            ncplane_dim_yx(mgr->dragged_window->plane, &w_dimy, &w_dimx);
            
            int new_rel_y = ni->y - p_abs_y - mgr->drag_start_wly;
            int new_rel_x = ni->x - p_abs_x - mgr->drag_start_wlx;
            
            // Aplicar Clamping Espacial
            if (new_rel_y < 0) new_rel_y = 0;
            if ((unsigned)new_rel_y + w_dimy > p_dimy) new_rel_y = p_dimy - w_dimy;
            if (new_rel_x < 0) new_rel_x = 0;
            if ((unsigned)new_rel_x + w_dimx > p_dimx) new_rel_x = p_dimx - w_dimx;
            
            ncplane_move_yx(mgr->dragged_window->plane, new_rel_y, new_rel_x);
        }
    }
    
    // Filter generic mouse clicks out to prevent them acting as keyboard
    if (key >= NCKEY_BUTTON1 && key <= NCKEY_BUTTON11) return true;
    if (key == NCKEY_MOTION) return true;
    
    return false;
}

bool tui_manager_process_keyboard(TuiManager* mgr, uint32_t key, const struct ncinput* ni) {
    // Para teclado, evtype es NCTYPE_UNKNOWN. Solo descartar RELEASE (exclusivo de raton)
    if (ni->evtype == NCTYPE_RELEASE) return false;

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
    TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
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

    // Windows Foco: Tab
    if (key == NCKEY_TAB) {
        if (ws->active_tab_index != -1) {
            TuiTab* tab = ws->tabs[ws->active_tab_index];
            if (tab->window_count > 0) {
                int next = (tab->active_window_index + 1) % tab->window_count;
                tab->active_window_index = next;
                ncplane_move_top(tab->windows[next]->plane);
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
