#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

int s_next_id = 1;

TuiManager* tui_manager_create(struct notcurses* nc) {
    TuiManager* mgr = (TuiManager*)calloc(1, sizeof(TuiManager));
    mgr->nc = nc;
    mgr->stdplane = notcurses_stdplane(nc);
    mgr->workspace_capacity = 4;
    mgr->workspaces = (TuiWorkspace**)calloc(mgr->workspace_capacity, sizeof(TuiWorkspace*));
    mgr->active_workspace_index = -1;
    mgr->running = true;
    return mgr;
}

void tui_manager_destroy(TuiManager* manager) {
    if (!manager) return;
    for (int i = 0; i < manager->workspace_count; i++) {
        tui_workspace_destroy(manager->workspaces[i]);
    }
    free(manager->workspaces);
    free(manager);
}

void tui_manager_add_workspace(TuiManager* manager, TuiWorkspace* ws) {
    if (manager->workspace_count >= manager->workspace_capacity) {
        manager->workspace_capacity *= 2;
        manager->workspaces = (TuiWorkspace**)realloc(manager->workspaces, manager->workspace_capacity * sizeof(TuiWorkspace*));
    }
    manager->workspaces[manager->workspace_count++] = ws;
    if (manager->active_workspace_index == -1) {
        tui_manager_set_active_workspace(manager, manager->workspace_count - 1);
    }
}

void tui_manager_set_active_workspace(TuiManager* manager, int index) {
    if (index < 0 || index >= manager->workspace_count) return;
    
    // Ocultar enviándolo fuera de pantalla (Off-screen parking)
    if (manager->active_workspace_index != -1) {
        TuiWorkspace* prev = manager->workspaces[manager->active_workspace_index];
        ncplane_move_yx(prev->ws_plane, -10000, 0);
    }
    
    manager->active_workspace_index = index;
    TuiWorkspace* next = manager->workspaces[index];
    ncplane_move_yx(next->ws_plane, 0, 0);
    ncplane_move_top(next->ws_plane);
}

void tui_manager_remove_active_workspace(TuiManager* mgr) {
    if (!mgr || mgr->workspace_count == 0 || mgr->active_workspace_index < 0) return;
    int idx = mgr->active_workspace_index;
    tui_workspace_destroy(mgr->workspaces[idx]);
    for (int i = idx; i < mgr->workspace_count - 1; i++) {
        mgr->workspaces[i] = mgr->workspaces[i + 1];
    }
    mgr->workspace_count--;
    mgr->active_workspace_index = -1;
    if (mgr->workspace_count > 0) {
        tui_manager_set_active_workspace(mgr, mgr->workspace_count - 1);
    }
}

void tui_manager_render(TuiManager* manager) {
    // Fondo base del UI global real
    uint64_t bg_channels = 0;
    ncchannels_set_fg_rgb(&bg_channels, 0xCCCCCC);
    ncchannels_set_bg_rgb(&bg_channels, 0x111116);
    ncchannels_set_bg_alpha(&bg_channels, NCALPHA_OPAQUE); // FUERZA opacidad para evitar terminal host
    ncplane_set_base(manager->stdplane, " ", 0, bg_channels);
    ncplane_erase(manager->stdplane);
    unsigned dimy, dimx;
    ncplane_dim_yx(manager->stdplane, &dimy, &dimx);

    if (manager->active_workspace_index != -1) {
        TuiWorkspace* ws = manager->workspaces[manager->active_workspace_index];
        
        // 1. Barra de Herramientas Estilizada
        ncplane_set_bg_rgb(manager->stdplane, 0x1A1A24); 
        for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->stdplane, 0, i, " ");
        for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->stdplane, 1, i, " ");
        
        // Separador sútil horizontal
        ncplane_set_fg_rgb(manager->stdplane, 0x333344);
        for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->stdplane, 2, i, "━");

        // Info de Workspace
        ncplane_set_fg_rgb(manager->stdplane, 0xffb86c);
        ncplane_printf_yx(manager->stdplane, 1, 2, "◆ Workspace: %s", ws->name);

        int offset_x = 30;
        for(int i=0; i<ws->tab_count; i++){
            // Tab seleccionado con contraste alto
            if (i == ws->active_tab_index) {
                 ncplane_set_bg_rgb(manager->stdplane, 0x50fa7b); // Verde brillante
                 ncplane_set_fg_rgb(manager->stdplane, 0x282a36);
                 ncplane_putstr_yx(manager->stdplane, 1, offset_x, " ");
                 ncplane_printf_yx(manager->stdplane, 1, offset_x + 1, " %s ", ws->tabs[i]->name);
                 ncplane_putstr_yx(manager->stdplane, 1, offset_x + 3 + strlen(ws->tabs[i]->name), " ");
            } else {
                 ncplane_set_bg_rgb(manager->stdplane, 0x1A1A24);
                 ncplane_set_fg_rgb(manager->stdplane, 0x6272a4);
                 ncplane_printf_yx(manager->stdplane, 1, offset_x + 2, " %s ", ws->tabs[i]->name);
            }
            offset_x += strlen(ws->tabs[i]->name) + 6;
        }

        // Dejar listo el stdplane con fondo base de nuevo
        ncplane_set_bg_rgb(manager->stdplane, 0x111116);
        
        // 2. Procesar los renders dentro del Tab 
        if (ws->active_tab_index != -1) {
            TuiTab* tab = ws->tabs[ws->active_tab_index];
            for (int i = 0; i < tab->window_count; i++) {
                if (tab->windows[i]->render_cb) {
                    tab->windows[i]->render_cb(tab->windows[i]);
                }
            }
        }
    }
    
    // --- RENDER TASKBAR INFERIOR ---
    ncplane_set_bg_rgb(manager->stdplane, 0x21222c); // Fondo taskbar independiente
    for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->stdplane, dimy - 1, i, " ");
    
    ncplane_set_bg_rgb(manager->stdplane, 0xbd93f9); // Botón Lila centralizado
    ncplane_set_fg_rgb(manager->stdplane, 0x282a36);
    ncplane_printf_yx(manager->stdplane, dimy - 1, 0, " ❖ START ");
    
    // Indicadores de Procesos / Ventanas Activas
    int taskbar_x = 12;
    if (manager->active_workspace_index != -1) {
        TuiWorkspace* ws = manager->workspaces[manager->active_workspace_index];
        if (ws->active_tab_index != -1) {
            TuiTab* tab = ws->tabs[ws->active_tab_index];
            for (int i = 0; i < tab->window_count; i++) {
                TuiWindow* win = tab->windows[i];
                char* title = win->user_data ? (char*)win->user_data : "Window";
                
                if (i == tab->active_window_index) {
                     ncplane_set_bg_rgb(manager->stdplane, 0x44475a); // Activo
                     ncplane_set_fg_rgb(manager->stdplane, 0xf8f8f2);
                } else {
                     ncplane_set_bg_rgb(manager->stdplane, 0x21222c); // Inactivo
                     ncplane_set_fg_rgb(manager->stdplane, 0x6272a4);
                }
                
                ncplane_printf_yx(manager->stdplane, dimy - 1, taskbar_x, "  %s  ", title);
                taskbar_x += strlen(title) + 4;
                
                ncplane_set_bg_rgb(manager->stdplane, 0x21222c);
                ncplane_set_fg_rgb(manager->stdplane, 0x44475a);
                ncplane_putstr_yx(manager->stdplane, dimy - 1, taskbar_x, "│");
                taskbar_x += 1;
            }
        }
    }
    
    time_t rawtime;
    struct tm * timeinfo;
    char time_str[10];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%H:%M", timeinfo);
    
    ncplane_set_bg_rgb(manager->stdplane, 0x282a36);
    ncplane_set_fg_rgb(manager->stdplane, 0xf8f8f2);
    ncplane_printf_yx(manager->stdplane, dimy - 1, dimx - 10, " %s ", time_str);
    
    notcurses_render(manager->nc);
}

int tui_manager_get_taskbar_window_at(TuiManager* mgr, int y, int x) {
    if (mgr->active_workspace_index == -1) return -1;
    unsigned dimy, dimx;
    ncplane_dim_yx(mgr->stdplane, &dimy, &dimx);
    if (y != (int)dimy - 1) return -1;

    TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
    if (ws->active_tab_index == -1) return -1;
    TuiTab* tab = ws->tabs[ws->active_tab_index];

    int offset_x = 12; // Inicia tras " ❖ START "
    for (int i = 0; i < tab->window_count; i++) {
        TuiWindow* win = tab->windows[i];
        char* title = win->user_data ? (char*)win->user_data : "Window";
        int len = strlen(title) + 5; // "  " + title + "  │"
        if (x >= offset_x && x < offset_x + len) {
            return i;
        }
        offset_x += len;
    }
    return -1;
}

int tui_manager_get_tab_at(TuiManager* mgr, int y, int x) {
    if (y != 1) return -1;
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
            
            // Priority 0: Has the user clicked a process in the Bottom Taskbar?
            int clicked_taskbar_win = tui_manager_get_taskbar_window_at(mgr, ni->y, ni->x);
            if (clicked_taskbar_win != -1) {
                TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
                TuiTab* tab = ws->tabs[ws->active_tab_index];
                tab->active_window_index = clicked_taskbar_win;
                ncplane_move_top(tab->windows[clicked_taskbar_win]->plane);
                return true;
            }
            
            int clicked_tab = tui_manager_get_tab_at(mgr, ni->y, ni->x);
            if (clicked_tab != -1) {
                TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
                tui_workspace_set_active_tab(ws, clicked_tab);
                return true;
            } else if (mgr->active_workspace_index != -1) {
                TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
                if (ws->active_tab_index != -1) {
                    int wly, wlx;
                    TuiTab* tab = ws->tabs[ws->active_tab_index];
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
                        if (wly == 0) { // Título de la ventana
                            mgr->dragged_window = win;
                            mgr->drag_start_wly = wly;
                            mgr->drag_start_wlx = wlx;
                        }
                    }
                }
            }
        } else if (ni->evtype == NCTYPE_RELEASE) {
            mgr->dragged_window = NULL;
        }
    }
    
    // 2. Procesar Arrastre y Clamping
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
