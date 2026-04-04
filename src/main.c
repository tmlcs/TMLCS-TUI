#include <notcurses/notcurses.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"

// Callback de renderizado para prueba
void my_window_render(TuiWindow* win) {
    if (!win || !win->plane) return;
    
    uint64_t bg_channels = 0;
    ncchannels_set_bg_rgb(&bg_channels, 0x282a36); // Tema Dracula oscuro
    ncchannels_set_fg_rgb(&bg_channels, 0xbd93f9); // Lila
    ncchannels_set_bg_alpha(&bg_channels, NCALPHA_OPAQUE); // Fijando fondo OPACO
    ncchannels_set_fg_alpha(&bg_channels, NCALPHA_OPAQUE);
    ncplane_set_base(win->plane, " ", 0, bg_channels);
    ncplane_erase(win->plane);
    
    // Set colors for the frame explicitly so putstr picks it up
    ncplane_set_bg_rgb(win->plane, 0x282a36); 
    ncplane_set_fg_rgb(win->plane, 0xbd93f9); 
    
    unsigned dimy, dimx;
    ncplane_dim_yx(win->plane, &dimy, &dimx);
    
    // Draw modern unicode rounded box
    ncplane_putstr_yx(win->plane, 0, 0, "╭");
    ncplane_putstr_yx(win->plane, 0, dimx-1, "╮");
    ncplane_putstr_yx(win->plane, dimy-1, 0, "╰");
    ncplane_putstr_yx(win->plane, dimy-1, dimx-1, "╯");

    for(unsigned i = 1; i < dimx-1; i++) {
        ncplane_putstr_yx(win->plane, 0, i, "─");
        ncplane_putstr_yx(win->plane, dimy-1, i, "─");
    }
    for(unsigned i = 1; i < dimy-1; i++) {
        ncplane_putstr_yx(win->plane, i, 0, "│");
        ncplane_putstr_yx(win->plane, i, dimx-1, "│");
    }
    
    // Window Title
    ncplane_set_bg_rgb(win->plane, 0xbd93f9);
    ncplane_set_fg_rgb(win->plane, 0x282a36);
    ncplane_printf_yx(win->plane, 0, 2, " Ventana ID: %d ", win->id);
    
    // Text ref
    ncplane_set_bg_rgb(win->plane, 0x282a36);
    ncplane_set_fg_rgb(win->plane, 0xf8f8f2);
    if (win->user_data) {
         ncplane_printf_yx(win->plane, 2, 2, "> %s", (char*)win->user_data);
    }
}

int main(void) {
    if (!setlocale(LC_ALL, "")) {
        fprintf(stderr, "Error en setlocale\n");
    }

    // Remover NCOPTION_INHIBIT_SETLOCALE para heredar UTF-8
    struct notcurses_options n_opts = { .flags = 0 };
    
    struct notcurses* nc = notcurses_init(&n_opts, NULL);
    if (!nc) return 1;
    
    // Habilitar todos los eventos de ratón (clicks + arrastres)
    notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

    TuiManager* mgr = tui_manager_create(nc);

    // Workspace 1
    TuiWorkspace* ws1 = tui_workspace_create(mgr, "Desarrollo");
    tui_manager_add_workspace(mgr, ws1);
    
    TuiTab* tab1 = tui_tab_create(ws1, "Vista Principal");
    tui_workspace_add_tab(ws1, tab1);
    
    // Múltiples ventanas dentro de la misma pestaña conviven juntas
    TuiWindow* win1 = tui_window_create(tab1, 30, 10);
    win1->user_data = "Log Server";
    win1->render_cb = my_window_render;
    tui_tab_add_window(tab1, win1);
    ncplane_move_yx(win1->plane, 2, 5); // y, x (rows, cols)

    TuiWindow* win2 = tui_window_create(tab1, 40, 15);
    win2->user_data = "Git Status";
    win2->render_cb = my_window_render;
    tui_tab_add_window(tab1, win2);
    ncplane_move_yx(win2->plane, 5, 40);

    // Workspace 2
    TuiWorkspace* ws2 = tui_workspace_create(mgr, "Navegador");
    tui_manager_add_workspace(mgr, ws2);

    TuiTab* tab3 = tui_tab_create(ws2, "Web");
    tui_workspace_add_tab(ws2, tab3);

    TuiWindow* win3 = tui_window_create(tab3, 50, 20);
    win3->user_data = "www.google.com";
    win3->render_cb = my_window_render;
    tui_tab_add_window(tab3, win3);
    ncplane_move_yx(win3->plane, 4, 10);

    // Activamos Workspace inicial
    tui_manager_set_active_workspace(mgr, 0);

    // Event Loop primitivo y delegación a Gestores
    struct ncinput ni;
    while (mgr->running) {
        tui_manager_render(mgr);
        
        uint32_t key = notcurses_get_blocking(nc, &ni);
        
        // 1. Delegar eventos de Ratón al Gestor Oculto
        if (tui_manager_process_mouse(mgr, key, &ni)) {
            continue; // Fue consumido por el ratón, ignoramos teclado
        }
        
        // 2. Teclado Convencional
        if (key == 'q' || key == 'Q') {
            mgr->running = false;
        } else if (key == '1') {
            tui_manager_set_active_workspace(mgr, 0);
        } else if (key == '2') {
            tui_manager_set_active_workspace(mgr, 1);
        } else if (key == NCKEY_TAB) { // Avanzar de tab
            if (mgr->active_workspace_index != -1) {
                TuiWorkspace* active_ws = mgr->workspaces[mgr->active_workspace_index];
                if (active_ws->tab_count > 0) {
                    int next_tab = (active_ws->active_tab_index + 1) % active_ws->tab_count;
                    tui_workspace_set_active_tab(active_ws, next_tab);
                }
            }
        } else if (key == 'x') { // Close Window
            if (mgr->active_workspace_index != -1) {
                TuiWorkspace* ws = mgr->workspaces[mgr->active_workspace_index];
                if (ws->active_tab_index != -1) {
                    tui_tab_remove_active_window(ws->tabs[ws->active_tab_index]);
                }
            }
        } else if (key == 'w') { // Close Tab
            if (mgr->active_workspace_index != -1) {
                tui_workspace_remove_active_tab(mgr->workspaces[mgr->active_workspace_index]);
            }
        } else if (key == 'd') { // Close Workspace
            tui_manager_remove_active_workspace(mgr);
        }
    }

    tui_manager_destroy(mgr);
    notcurses_stop(nc);
    return 0;
}
