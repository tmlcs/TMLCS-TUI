/**
 * @file hello_world.c
 * @brief Minimal TMLCS-TUI application: one workspace, one tab, one window.
 *
 * Build: make examples/hello_world
 * Run:   ./build/examples/hello_world
 * Quit:  press 'q'
 */
#include <notcurses/notcurses.h>
#include <locale.h>
#include <time.h>
#include "tmlcs_tui.h"

static void render_hello(TuiWindow* win) {
    if (!win || !win->_plane) return;
    unsigned dimy, dimx;
    ncplane_dim_yx(win->_plane, &dimy, &dimx);
    uint64_t bg = 0;
    ncchannels_set_fg_rgb(&bg, THEME_FG_DEFAULT);
    ncchannels_set_bg_rgb(&bg, THEME_BG_WINDOW);
    ncchannels_set_bg_alpha(&bg, NCALPHA_OPAQUE);
    ncplane_set_base(win->_plane, " ", 0, bg);
    ncplane_erase(win->_plane);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, dimy / 2, 2, "Hello, TMLCS-TUI! Press 'q' to quit.");
}

int main(void) {
    if (!setlocale(LC_ALL, "")) { fprintf(stderr, "setlocale failed\n"); return 1; }

    struct notcurses_options nopts = { .flags = 0 };
    struct notcurses* nc = notcurses_init(&nopts, NULL);
    if (!nc) { fprintf(stderr, "notcurses_init failed\n"); return 1; }

    notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

    TuiManager* mgr = tui_manager_create(nc);
    TuiWorkspace* ws = tui_workspace_create(mgr, "Hello");
    tui_manager_add_workspace(mgr, ws);

    TuiTab* tab = tui_tab_create(ws, "Main");
    tui_workspace_add_tab(ws, tab);

    TuiWindow* win = tui_window_create(tab, 50, 5);
    win->_user_data = "Hello";
    win->_render_cb = render_hello;
    tui_tab_add_window(tab, win);
    ncplane_move_yx(win->_plane, 2, 2);

    tui_manager_set_active_workspace(mgr, 0);

    struct ncinput ni;
    while (mgr->_running) {
        tui_manager_render(mgr);
        uint32_t key = notcurses_get_nblock(nc, &ni);
        if (key != (uint32_t)-1) {
            tui_manager_process_keyboard(mgr, key, &ni);
        } else {
            struct timespec ts = {0, 16000000};
            nanosleep(&ts, NULL);
        }
    }

    tui_manager_destroy(mgr);
    notcurses_stop(nc);
    return 0;
}
