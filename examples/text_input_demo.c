/**
 * @file text_input_demo.c
 * @brief Demonstrates the text input widget with submit callback.
 * Quit: 'q'
 */
#include <notcurses/notcurses.h>
#include <locale.h>
#include <stdio.h>
#include "tmlcs_tui.h"

static TuiTextInput* g_input = NULL;

static void on_submit(const char* text, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "Submitted: %s", text);
}

static void render_console(TuiWindow* win) {
    if (!win || !win->_plane) return;
    unsigned dimy, dimx;
    ncplane_dim_yx(win->_plane, &dimy, &dimx);
    uint64_t bg = 0;
    ncchannels_set_fg_rgb(&bg, THEME_FG_DEFAULT);
    ncchannels_set_bg_rgb(&bg, THEME_BG_WINDOW);
    ncchannels_set_bg_alpha(&bg, NCALPHA_OPAQUE);
    ncplane_set_base(win->_plane, " ", 0, bg);
    ncplane_erase(win->_plane);
    ncplane_set_fg_rgb(win->_plane, THEME_BG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 1, 1, "[ Type below and press Enter ]");
    if (g_input) {
        ncplane_move_top(g_input->plane);
        tui_text_input_render(g_input);
    }
}

int main(void) {
    if (!setlocale(LC_ALL, "")) return 1;
    struct notcurses_options nopts = { .flags = 0 };
    struct notcurses* nc = notcurses_init(&nopts, NULL);
    if (!nc) return 1;
    notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);
    tui_logger_init("/tmp/tmlcs_text_input_demo.log");

    TuiManager* mgr = tui_manager_create(nc);
    TuiWorkspace* ws = tui_workspace_create(mgr, "Console");
    tui_manager_add_workspace(mgr, ws);
    TuiTab* tab = tui_tab_create(ws, "Input");
    tui_workspace_add_tab(ws, tab);
    TuiWindow* win = tui_window_create(tab, 60, 6);
    win->_user_data = "Console";
    win->_render_cb = render_console;
    tui_tab_add_window(tab, win);
    ncplane_move_yx(win->_plane, 2, 2);

    g_input = tui_text_input_create(win->_plane, 3, 1, 58, on_submit, NULL);
    g_input->focused = true;
    win->_text_input = g_input;

    tui_manager_set_active_workspace(mgr, 0);

    struct ncinput ni;
    while (mgr->_running) {
        tui_manager_render(mgr);
        uint32_t key = notcurses_get_nblock(nc, &ni);
        if (key == (uint32_t)-1) {
            struct timespec ts = {0, 16000000};
            nanosleep(&ts, NULL);
            continue;
        }
        bool consumed = tui_text_input_handle_key(g_input, key, &ni);
        if (!consumed) tui_manager_process_keyboard(mgr, key, &ni);
    }

    tui_manager_destroy(mgr);
    tui_logger_destroy();
    notcurses_stop(nc);
    return 0;
}
