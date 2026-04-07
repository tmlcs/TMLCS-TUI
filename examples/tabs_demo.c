/**
 * @file tabs_demo.c
 * @brief Demonstrates workspace and tab management.
 * Quit: 'q'
 */
#include <notcurses/notcurses.h>
#include <locale.h>
#include <time.h>
#include "tmlcs_tui.h"

static void render_tab_info(TuiWindow* win) {
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
    ncplane_printf_yx(win->_plane, 1, 1, "Window: %s", (char*)win->_user_data);
    ncplane_printf_yx(win->_plane, 2, 1, "Press Left/Right to switch tabs");
    ncplane_printf_yx(win->_plane, 3, 1, "Press Alt+1/2 to switch workspaces");
}

int main(void) {
    if (!setlocale(LC_ALL, "")) return 1;
    struct notcurses_options nopts = { .flags = 0 };
    struct notcurses* nc = notcurses_init(&nopts, NULL);
    if (!nc) return 1;
    notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

    TuiManager* mgr = tui_manager_create(nc);

    /* Workspace 1 with 2 tabs */
    TuiWorkspace* ws1 = tui_workspace_create(mgr, "Dev");
    tui_manager_add_workspace(mgr, ws1);
    TuiTab* tab1a = tui_tab_create(ws1, "Editor");
    tui_workspace_add_tab(ws1, tab1a);
    TuiTab* tab1b = tui_tab_create(ws1, "Terminal");
    tui_workspace_add_tab(ws1, tab1b);
    TuiWindow* w1 = tui_window_create(tab1a, 40, 8);
    w1->_user_data = "Editor Window";
    w1->_render_cb = render_tab_info;
    tui_tab_add_window(tab1a, w1);
    ncplane_move_yx(w1->_plane, 2, 2);

    /* Workspace 2 with 1 tab */
    TuiWorkspace* ws2 = tui_workspace_create(mgr, "Browser");
    tui_manager_add_workspace(mgr, ws2);
    TuiTab* tab2 = tui_tab_create(ws2, "Web");
    tui_workspace_add_tab(ws2, tab2);
    TuiWindow* w2 = tui_window_create(tab2, 40, 8);
    w2->_user_data = "Browser Window";
    w2->_render_cb = render_tab_info;
    tui_tab_add_window(tab2, w2);
    ncplane_move_yx(w2->_plane, 2, 2);

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
