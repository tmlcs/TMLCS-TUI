#include <notcurses/notcurses.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/theme.h"
#include "widget/text_input.h"

// Instancia global del input activo (solo uno a la vez)
static TuiTextInput* g_active_input = NULL;

void on_console_submit(const char* text, void* userdata) {
    (void)userdata;
    if (!text || text[0] == '\0') {
        tui_log(LOG_DEBUG, "on_console_submit: texto vacío");
        return;
    }
    tui_log(LOG_INFO, ">> %s", text);
}

// Destructor de la ventana Consola: libera el TextInput antes de que el motor
// destruya el ncplane padre (evita planos huérfanos y punteros colgantes)
void console_on_destroy(TuiWindow* win) {
    if (!win) return;
    if (win->text_input) {
        // Si es el input activo, limpiar referencia global
        if (g_active_input == (TuiTextInput*)win->text_input) {
            g_active_input = NULL;
        }
        tui_text_input_destroy((TuiTextInput*)win->text_input);
        win->text_input = NULL;
    }
}
// Helper: dibuja marco y titulo de ventana reutilizable
static void render_window_frame(TuiWindow* win) {
    uint64_t bg_channels = 0;
    ncchannels_set_bg_rgb(&bg_channels, THEME_BG_WINDOW);
    ncchannels_set_fg_rgb(&bg_channels, THEME_BG_TITLE_INA);
    ncchannels_set_bg_alpha(&bg_channels, NCALPHA_OPAQUE);
    ncchannels_set_fg_alpha(&bg_channels, NCALPHA_OPAQUE);
    ncplane_set_base(win->plane, " ", 0, bg_channels);
    ncplane_erase(win->plane);

    // Color de borde según foco: amarillo activo / lila inactivo
    unsigned int border_col = win->focused ? THEME_BG_TITLE_ACT : THEME_FG_BORDER_INA;
    uint64_t border_channels = 0;
    ncchannels_set_fg_rgb(&border_channels, border_col);
    ncchannels_set_bg_rgb(&border_channels, THEME_BG_WINDOW);
    ncplane_perimeter_rounded(win->plane, 0, border_channels, 0);

    unsigned dimy, dimx;
    ncplane_dim_yx(win->plane, &dimy, &dimx);

    // Titulo
    unsigned int title_bg = win->focused ? THEME_BG_TITLE_ACT : THEME_BG_TITLE_INA;
    unsigned int title_fg = THEME_FG_TAB_ACTIVE;
    ncplane_set_bg_rgb(win->plane, title_bg);
    ncplane_set_fg_rgb(win->plane, title_fg);
    ncplane_printf_yx(win->plane, 0, 2, " %s ", (char*)win->user_data);
    // Boton [X]
    ncplane_set_bg_rgb(win->plane, THEME_BG_CLOSE_BTN);
    ncplane_set_fg_rgb(win->plane, THEME_FG_DEFAULT);
    ncplane_putstr_yx(win->plane, 0, dimx - 4, "[X]");
    // Grip resize
    ncplane_set_bg_rgb(win->plane, THEME_BG_DARKEST);
    ncplane_set_fg_rgb(win->plane, border_col);
    ncplane_putstr_yx(win->plane, dimy - 1, dimx - 2, "⌟");
}

void my_window_render(TuiWindow* win) {
    if (!win || !win->plane) return;
    render_window_frame(win);
    unsigned dimy, dimx;
    ncplane_dim_yx(win->plane, &dimy, &dimx);
    ncplane_set_bg_rgb(win->plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->plane, THEME_FG_DEFAULT);
    if (win->user_data) {
        ncplane_printf_yx(win->plane, 2, 2, "> %s", (char*)win->user_data);
    }
}

void console_render(TuiWindow* win) {
    if (!win || !win->plane) return;
    render_window_frame(win);
    // Etiqueta decorativa
    ncplane_set_bg_rgb(win->plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->plane, THEME_BG_TAB_ACTIVE); // Verde claro para destacar
    ncplane_putstr_yx(win->plane, 2, 2, "[ Input ] ▼");
    // Garantizar que el plano del TextInput esté siempre encima (z-order)
    if (win->text_input) {
        TuiTextInput* ti = (TuiTextInput*)win->text_input;
        ncplane_move_top(ti->plane);  // Forzar al frente cada frame
        tui_text_input_render(ti);
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
            case LOG_ERROR: ncplane_set_fg_rgb(win->plane, THEME_FG_LOG_ERROR); break;
            case LOG_WARN:  ncplane_set_fg_rgb(win->plane, THEME_FG_LOG_WARN); break;
            case LOG_DEBUG: ncplane_set_fg_rgb(win->plane, THEME_FG_LOG_DEBUG); break;
            default:        ncplane_set_fg_rgb(win->plane, THEME_FG_LOG_INFO); break; // INFO: verde
        }
        ncplane_set_bg_rgb(win->plane, THEME_BG_WINDOW);

        // Truncar al ancho de la ventana
        char line[256];
        int max_cols = (int)dimx - 4;
        if (max_cols <= 0) max_cols = 1;
        snprintf(line, sizeof(line), "%.*s", max_cols, buf->lines[idx]);
        ncplane_printf_yx(win->plane, 1 + offset + i, 2, "%s", line);
    }
}

int demo_main(struct notcurses* nc) {
    // Inicializar motor de logging
    tui_logger_init("/tmp/tmlcs_tui_debug.log");
    tui_log(LOG_INFO, "Iniciando TMLCS-TUI (demo)");
    
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

    // Ventana Consola con TextInput
    TuiWindow* win_console = tui_window_create(tab1, 50, 5);
    win_console->user_data = "Consola";
    win_console->render_cb = console_render;
    tui_tab_add_window(tab1, win_console);
    ncplane_move_yx(win_console->plane, 14, 2);

    // Crear el TuiTextInput dentro del plano de la ventana
    g_active_input = tui_text_input_create(win_console->plane,
                                            3, 1, 47, // y=3 x=1 width=47
                                            on_console_submit, NULL);
    g_active_input->focused = true;
    win_console->text_input = g_active_input;
    win_console->on_destroy = console_on_destroy; // Registrar limpiador

    tui_log(LOG_INFO, "Consola lista. Escribe y pulsa Enter para loguear.");

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

    // Event Loop no-bloqueante (~60fps) para render continuo
    struct ncinput ni;
    while (mgr->running) {
        tui_manager_render(mgr);
        
        uint32_t key = notcurses_get_nblock(nc, &ni);
        
        if (key == (uint32_t)-1) {
            // Sin input: ceder CPU 16ms (~60fps)
            struct timespec ts = {0, 16000000};
            nanosleep(&ts, NULL);
            continue;
        }
        
        // Router TUI Híbrido: Widget activo → Ratón → Teclado global
        if (key >= NCKEY_BUTTON1 && key <= NCKEY_BUTTON11) {
            tui_manager_process_mouse(mgr, key, &ni);
        } else if (key == NCKEY_MOTION) {
            tui_manager_process_mouse(mgr, key, &ni);
        } else {
            bool consumed = g_active_input
                            ? tui_text_input_handle_key(g_active_input, key, &ni)
                            : false;
            if (!consumed) {
                tui_manager_process_keyboard(mgr, key, &ni);
            }
        }
    }

    tui_manager_destroy(mgr);
    tui_logger_destroy();
    return 0;
}
