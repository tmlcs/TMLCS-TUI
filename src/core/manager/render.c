#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/types_private.h"
#include <string.h>
#include <time.h>

/* Last mouse position (set by input.c for hover highlight) */
static int s_hover_y = -1, s_hover_x = -1;

void tui_manager_update_hover(int y, int x) {
    s_hover_y = y;
    s_hover_x = x;
}

/* Dirty tracking helpers */
void tui_manager_mark_toolbar_dirty(TuiManager* mgr) {
    if (mgr) mgr->_toolbar_needs_redraw = true;
}

void tui_manager_mark_taskbar_dirty(TuiManager* mgr) {
    if (mgr) mgr->_taskbar_needs_redraw = true;
}

void tui_manager_mark_tabs_dirty(TuiManager* mgr) {
    if (mgr) mgr->_tabs_needs_redraw = true;
}

void tui_manager_mark_full_redraw(TuiManager* mgr) {
    if (!mgr) return;
    mgr->_toolbar_needs_redraw = true;
    mgr->_taskbar_needs_redraw = true;
    mgr->_tabs_needs_redraw = true;
}

void tui_manager_render(TuiManager* manager) {
    if (!manager) return;

    unsigned dimy, dimx;
    ncplane_dim_yx(manager->_stdplane, &dimy, &dimx);

    /* Check for state changes that require full/partial redraw */
    bool workspace_changed = (manager->_active_workspace_index != manager->_last_active_workspace);
    bool tab_changed = false;
    bool window_focus_changed = false;

    if (manager->_active_workspace_index != -1) {
        TuiWorkspace* ws = manager->_workspaces[manager->_active_workspace_index];
        tab_changed = (ws->_active_tab_index != manager->_last_active_tab);
        if (ws->_active_tab_index != -1) {
            TuiTab* tab = ws->_tabs[ws->_active_tab_index];
            window_focus_changed = (tab->_active_window_index != manager->_last_active_window);
        }
    }

    /* Mark zones dirty if state changed */
    if (workspace_changed) {
        tui_manager_mark_full_redraw(manager);
    }
    if (tab_changed) {
        tui_manager_mark_toolbar_dirty(manager);
        tui_manager_mark_tabs_dirty(manager);
        tui_manager_mark_taskbar_dirty(manager);
    }
    if (window_focus_changed) {
        tui_manager_mark_toolbar_dirty(manager); /* tab bar might need update */
        /* Mark all windows in current tab for redraw (focus ring changes) */
        if (manager->_active_workspace_index != -1) {
            TuiWorkspace* ws = manager->_workspaces[manager->_active_workspace_index];
            if (ws->_active_tab_index != -1) {
                TuiTab* tab = ws->_tabs[ws->_active_tab_index];
                for (int i = 0; i < tab->_window_count; i++) {
                    tab->_windows[i]->_needs_redraw = true;
                }
            }
        }
    }

    /* Base background of the global UI (only on full redraw or first render) */
    if (manager->_toolbar_needs_redraw || manager->_tabs_needs_redraw) {
        uint64_t bg_channels = 0;
        ncchannels_set_fg_rgb(&bg_channels, THEME_FG_TIMESTAMP);
        ncchannels_set_bg_rgb(&bg_channels, THEME_BG_DARKEST);
        ncchannels_set_bg_alpha(&bg_channels, NCALPHA_OPAQUE);
        ncplane_set_base(manager->_stdplane, " ", 0, bg_channels);
        ncplane_erase(manager->_stdplane);
    }

    if (manager->_active_workspace_index != -1) {
        TuiWorkspace* ws = manager->_workspaces[manager->_active_workspace_index];

        /* 1. Stylized Toolbar Bar (only if marked dirty) */
        if (manager->_toolbar_needs_redraw) {
            ncplane_set_bg_rgb(manager->_stdplane, THEME_BG_DARK);
            for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->_stdplane, 0, i, " ");

            /* Subtle horizontal separator */
            ncplane_set_fg_rgb(manager->_stdplane, THEME_FG_SEPARATOR);
            for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->_stdplane, 1, i, "━");

            /* Workspace info */
            ncplane_set_fg_rgb(manager->_stdplane, THEME_FG_WS_LABEL);
            ncplane_printf_yx(manager->_stdplane, 0, 2, "◆ Workspace: %s", ws->_name);

            int offset_x = TAB_BAR_START_X;
            for(int i=0; i<ws->_tab_count; i++){
                /* Selected tab with high contrast */
                if (i == ws->_active_tab_index) {
                     ncplane_set_bg_rgb(manager->_stdplane, THEME_BG_TAB_ACTIVE);
                     ncplane_set_fg_rgb(manager->_stdplane, THEME_FG_TAB_ACTIVE);
                     ncplane_printf_yx(manager->_stdplane, 0, offset_x, " %s  ", ws->_tabs[i]->_name);
                } else {
                     ncplane_set_bg_rgb(manager->_stdplane, THEME_BG_DARK);
                     ncplane_set_fg_rgb(manager->_stdplane, THEME_FG_TAB_INA);
                     ncplane_printf_yx(manager->_stdplane, 0, offset_x + 2, " %s ", ws->_tabs[i]->_name);
                }
                offset_x += strlen(ws->_tabs[i]->_name) + 6;
            }

            /* Reset stdplane with base background */
            ncplane_set_bg_rgb(manager->_stdplane, THEME_BG_DARKEST);
        }

        /* 2. Process renders inside the Tab */
        if (ws->_active_tab_index != -1) {
            TuiTab* tab = ws->_tabs[ws->_active_tab_index];

            /* RENDER: Decorative external border for the "Tab Viewport" (only if dirty) */
            if (manager->_tabs_needs_redraw) {
                unsigned t_dimy, t_dimx;
                ncplane_dim_yx(tab->_tab_plane, &t_dimy, &t_dimx);

                ncplane_set_bg_rgb(tab->_tab_plane, THEME_BG_DARKEST);
                ncplane_set_fg_rgb(tab->_tab_plane, THEME_FG_SEPARATOR);
                ncplane_erase(tab->_tab_plane);

                /* Draw rounded box-drawing with native contingency API */
                uint64_t border_channels = 0;
                ncchannels_set_fg_rgb(&border_channels, THEME_FG_SEPARATOR);
                ncchannels_set_bg_rgb(&border_channels, THEME_BG_DARKEST);
                ncplane_perimeter_rounded(tab->_tab_plane, 0, border_channels, 0);

                /* Decorative subtitle for the container */
                ncplane_set_fg_rgb(tab->_tab_plane, THEME_FG_SUBTITLE);
                ncplane_printf_yx(tab->_tab_plane, 0, 2, " Container: %s ", tab->_name);
            }

            /* RENDER: Child windows — only if needs_redraw or focus changed */
            for (int i = 0; i < tab->_window_count; i++) {
                TuiWindow* win = tab->_windows[i];

                /* Sync focus state (active changes border color) */
                bool was_focused = win->_focused;
                win->_focused = (i == tab->_active_window_index);

                /* Determine if needs redraw before clearing the flag */
                bool needs_render = win->_needs_redraw || win->_focused != was_focused;

                /* Call render_cb only if the window is marked dirty or focus changed */
                if (needs_render) {
                    if (win->_render_cb) {
                        win->_render_cb(win);
                    }
                    win->_needs_redraw = false;
                }

                /* Focus ring: double-line border on focused window (only if focus changed) */
                if (win->_focused && (win->_focused != was_focused)) {
                    unsigned w_dimy, w_dimx;
                    ncplane_dim_yx(win->_plane, &w_dimy, &w_dimx);
                    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
                    for (unsigned ci = 1; ci < w_dimx - 1; ci++) {
                        ncplane_putchar_yx(win->_plane, 0, (int)ci, '=');
                        ncplane_putchar_yx(win->_plane, (int)w_dimy - 1, (int)ci, '=');
                    }
                    for (unsigned ri = 1; ri < w_dimy - 1; ri++) {
                        ncplane_putchar_yx(win->_plane, (int)ri, 0, '|');
                        ncplane_putchar_yx(win->_plane, (int)ri, (int)w_dimx - 1, '|');
                    }
                }
            }
        }
    }

    /* --- RENDER BOTTOM TASKBAR (only if marked dirty or clock changed) --- */
    char time_str[10];
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%H:%M", timeinfo);

    bool clock_changed = (strcmp(manager->_last_clock, time_str) != 0);

    if (manager->_taskbar_needs_redraw || clock_changed) {
        ncplane_set_bg_rgb(manager->_stdplane, THEME_BG_TASKBAR);
        for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->_stdplane, dimy - 1, i, " ");

        ncplane_set_bg_rgb(manager->_stdplane, THEME_BG_TITLE_INA);
        ncplane_set_fg_rgb(manager->_stdplane, THEME_FG_TAB_ACTIVE);
        ncplane_printf_yx(manager->_stdplane, dimy - 1, 0, " START ");

        ncplane_set_bg_rgb(manager->_stdplane, THEME_BG_WINDOW);
        ncplane_set_fg_rgb(manager->_stdplane, THEME_FG_DEFAULT);
        ncplane_printf_yx(manager->_stdplane, dimy - 1, dimx - 10, " %s ", time_str);

        /* Save clock state */
        strncpy(manager->_last_clock, time_str, sizeof(manager->_last_clock) - 1);
        manager->_last_clock[sizeof(manager->_last_clock) - 1] = '\0';
    }

    /* Update cached state */
    manager->_last_active_workspace = manager->_active_workspace_index;
    if (manager->_active_workspace_index != -1) {
        TuiWorkspace* ws = manager->_workspaces[manager->_active_workspace_index];
        manager->_last_active_tab = ws->_active_tab_index;
        if (ws->_active_tab_index != -1) {
            TuiTab* tab = ws->_tabs[ws->_active_tab_index];
            manager->_last_active_window = tab->_active_window_index;
        }
    }

    /* Clear dirty flags after rendering */
    manager->_toolbar_needs_redraw = false;
    manager->_tabs_needs_redraw = false;
    manager->_taskbar_needs_redraw = false;

    notcurses_render(manager->_nc);
}
