#include <notcurses/notcurses.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"

// Helper: dibuja marco y titulo de ventana reutilizable
static void render_window_frame(TuiWindow* win) {
    uint64_t bg_channels = 0;
    ncchannels_set_bg_rgb(&bg_channels, 0x282a36);
    ncchannels_set_fg_rgb(&bg_channels, 0xbd93f9);
    ncchannels_set_bg_alpha(&bg_channels, NCALPHA_OPAQUE);
    ncchannels_set_fg_alpha(&bg_channels, NCALPHA_OPAQUE);
    ncplane_set_base(win->plane, " ", 0, bg_channels);
    ncplane_erase(win->plane);
    uint64_t border_channels = 0;
    ncchannels_set_fg_rgb(&border_channels, 0xbd93f9);
    ncchannels_set_bg_rgb(&border_channels, 0x282a36);
    ncplane_perimeter_rounded(win->plane, 0, border_channels, 0);

    unsigned dimy, dimx;
    ncplane_dim_yx(win->plane, &dimy, &dimx);

    // Titulo
    ncplane_set_bg_rgb(win->plane, 0xbd93f9);
    ncplane_set_fg_rgb(win->plane, 0x282a36);
    ncplane_printf_yx(win->plane, 0, 2, " %s ", (char*)win->user_data);
    // Boton [X]
    ncplane_set_bg_rgb(win->plane, 0xff5555);
    ncplane_set_fg_rgb(win->plane, 0xf8f8f2);
    ncplane_putstr_yx(win->plane, 0, dimx - 4, "[X]");
    // Grip resize
    ncplane_set_bg_rgb(win->plane, 0x111116);
    ncplane_set_fg_rgb(win->plane, 0xbd93f9);
    ncplane_putstr_yx(win->plane, dimy - 1, dimx - 2, "⌟");
}

void my_window_render(TuiWindow* win) {
    if (!win || !win->plane) return;
    render_window_frame(win);
    unsigned dimy, dimx;
    ncplane_dim_yx(win->plane, &dimy, &dimx);
    ncplane_set_bg_rgb(win->plane, 0x282a36);
    ncplane_set_fg_rgb(win->plane, 0xf8f8f2);
    if (win->user_data) {
        ncplane_printf_yx(win->plane, 2, 2, "> %s", (char*)win->user_data);
    }
}

void log_server_render(TuiWindow* win) {
    if (!win || !win->plane) return;
    render_window_frame(win);
    unsigned dimy, dimx;
    ncplane_dim_yx(win->plane, &dimy, &dimx);

    TuiLogBuffer* buf = tui_logger_get_buffer();
    if (!buf || buf->count == 0) return;

    // Pintar las últimas `rows_available` entradas del ring buffer
    int rows_available = (int)dimy - 2;
    if (rows_available <= 0) return;

    int entries = (buf->count < rows_available) ? buf->count : rows_available;
    // Calcular desde la entrada mas antigua que queremos mostrar
    int start = (buf->head - entries + MAX_LOG_LINES) % MAX_LOG_LINES;
    int offset = rows_available - entries; // Alinear al fondo

    for (int i = 0; i < entries; i++) {
        int idx = (start + i) % MAX_LOG_LINES;
        LogLevel lv = buf->levels[idx];

        // Color por nivel
        switch(lv) {
            case LOG_ERROR: ncplane_set_fg_rgb(win->plane, 0xff5555); break;
            case LOG_WARN:  ncplane_set_fg_rgb(win->plane, 0xffb86c); break;
            case LOG_DEBUG: ncplane_set_fg_rgb(win->plane, 0x6272a4); break;
            default:        ncplane_set_fg_rgb(win->plane, 0x50fa7b); break; // INFO: verde
        }
        ncplane_set_bg_rgb(win->plane, 0x282a36);

        // Truncar al ancho de la ventana
        char line[256];
        int max_cols = (int)dimx - 4;
        if (max_cols <= 0) max_cols = 1;
        snprintf(line, sizeof(line), "%.*s", max_cols, buf->lines[idx]);
        ncplane_printf_yx(win->plane, 1 + offset + i, 2, "%s", line);
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

    // Inicializar motor de logging
    tui_logger_init("/tmp/tmlcs_tui_debug.log");
    tui_log(LOG_INFO, "Iniciando TMLCS-TUI");
    
    // Habilitar todos los eventos de ratón (clicks + arrastres)
    notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

    TuiManager* mgr = tui_manager_create(nc);

    // Workspace 1
    TuiWorkspace* ws1 = tui_workspace_create(mgr, "Desarrollo");
    tui_manager_add_workspace(mgr, ws1);
    
    TuiTab* tab1 = tui_tab_create(ws1, "Vista Principal");
    tui_workspace_add_tab(ws1, tab1);
    
    // Múltiples ventanas dentro de la misma pestaña conviven juntas
    TuiWindow* win1 = tui_window_create(tab1, 30, 16);
    win1->user_data = "Log Server";
    win1->render_cb = log_server_render;
    tui_tab_add_window(tab1, win1);
    ncplane_move_yx(win1->plane, 2, 2);

    TuiWindow* win2 = tui_window_create(tab1, 40, 12);
    win2->user_data = "Git Status";
    win2->render_cb = my_window_render;
    tui_tab_add_window(tab1, win2);
    ncplane_move_yx(win2->plane, 4, 38);

    tui_log(LOG_INFO, "Workspace [Desarrollo] listo con 2 ventanas");
    tui_log(LOG_WARN, "Notcurses requiere UTF-8 para Unicode avanzado");
    tui_log(LOG_DEBUG, "win1 plane=%p | ID=%d", (void*)win1->plane, win1->id);

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
        
        // Router TUI Híbrido: Ratón vs Teclado
        if (key >= NCKEY_BUTTON1 && key <= NCKEY_BUTTON11) {
            tui_manager_process_mouse(mgr, key, &ni);
        } else if (key == NCKEY_MOTION) {
            tui_manager_process_mouse(mgr, key, &ni);
        } else {
            tui_manager_process_keyboard(mgr, key, &ni);
        }
    }

    tui_manager_destroy(mgr);
    tui_logger_destroy();
    notcurses_stop(nc);
    return 0;
}
