/**
 * @file demo_system.c
 * @brief System workspace: CPU monitor with slider threshold, spinner, progress bars.
 */

#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/logger.h"
#include "widget/slider.h"
#include "widget/progress.h"
#include "widget/spinner.h"
#include "widget/label.h"
#include <stdio.h>

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiProgressBar* s_cpu_bar;
static TuiProgressBar* s_mem_bar;
static TuiProgressBar* s_disk_bar;
static TuiSpinner* s_activity_spinner;
static TuiSlider* s_cpu_threshold;
static TuiLabel* s_cpu_label;
static TuiLabel* s_threshold_label;

/* ============================================================
 * Renderers — called every frame to update widget state
 * ============================================================ */

static void system_cpu_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (s_cpu_bar) {
        tui_progress_set_value(s_cpu_bar, g_demo.cpu_usage / 100.0f);
        tui_progress_render(s_cpu_bar);
    }
    if (s_cpu_label) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 4, 2, "CPU: %.1f%%  |  Cores: 8  |  Threads: 16", g_demo.cpu_usage);
    }
}

static void system_memory_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (s_mem_bar) {
        tui_progress_set_value(s_mem_bar, 0.575f);
        tui_progress_render(s_mem_bar);
    }
    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "Total: 32 GB");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 4, 2, "Used:  18.4 GB");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN);
    ncplane_printf_yx(win->_plane, 5, 2, "Free:  13.6 GB");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_DEBUG);
    ncplane_printf_yx(win->_plane, 6, 2, "Swap:  2.1 GB");
}

static void system_disk_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (s_disk_bar) {
        tui_progress_set_value(s_disk_bar, 0.65f);
        tui_progress_render(s_disk_bar);
    }
    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "/dev/sda1  450GB/1TB");
    ncplane_printf_yx(win->_plane, 4, 2, "/dev/sda2  200GB/500GB");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 5, 2, "/dev/nvme0 120GB/1TB");
}

static void system_dashboard_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (s_cpu_threshold) {
        float thresh = tui_slider_get_value(s_cpu_threshold);
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 2, 2, "Alert Threshold: %.0f%%", thresh);
        if (s_threshold_label) {
            if (g_demo.cpu_usage > thresh) {
                ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_ERROR);
                ncplane_putstr_yx(win->_plane, 3, 2, "*** ALERT: CPU above threshold! ***");
            } else {
                ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
                ncplane_putstr_yx(win->_plane, 3, 2, "Status: Normal");
            }
        }
        if (s_activity_spinner) {
            tui_spinner_render(s_activity_spinner);
            ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
            ncplane_putstr_yx(win->_plane, 5, 2, "System monitoring active");
        }
    }
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_system_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "System");
    if (!ws) return NULL;

    TuiTab* tab_monitor = tui_tab_create(ws, "Monitor");
    TuiTab* tab_dashboard = tui_tab_create(ws, "Dashboard");
    tui_workspace_add_tab(ws, tab_monitor);
    tui_workspace_add_tab(ws, tab_dashboard);

    /* Monitor tab: 3 windows */
    TuiWindow* win_cpu = tui_window_create(tab_monitor, 30, 8);
    win_cpu->_user_data = "CPU Monitor";
    win_cpu->_render_cb = system_cpu_render;
    tui_tab_add_window(tab_monitor, win_cpu);
    ncplane_move_yx(win_cpu->_plane, 2, 2);
    s_cpu_bar = tui_progress_create(win_cpu->_plane, 6, 2, 24);
    if (s_cpu_bar) tui_window_add_widget(win_cpu, s_cpu_bar, s_cpu_bar->plane, tui_progress_get_type_id());
    s_cpu_label = tui_label_create(win_cpu->_plane, 4, 2, "");
    if (s_cpu_label) tui_window_add_widget(win_cpu, s_cpu_label, s_cpu_label->plane, tui_label_get_type_id());

    TuiWindow* win_mem = tui_window_create(tab_monitor, 25, 10);
    win_mem->_user_data = "Memory";
    win_mem->_render_cb = system_memory_render;
    tui_tab_add_window(tab_monitor, win_mem);
    ncplane_move_yx(win_mem->_plane, 2, 33);
    s_mem_bar = tui_progress_create(win_mem->_plane, 7, 2, 20);
    if (s_mem_bar) tui_window_add_widget(win_mem, s_mem_bar, s_mem_bar->plane, tui_progress_get_type_id());

    TuiWindow* win_disk = tui_window_create(tab_monitor, 25, 8);
    win_disk->_user_data = "Disk Usage";
    win_disk->_render_cb = system_disk_render;
    tui_tab_add_window(tab_monitor, win_disk);
    ncplane_move_yx(win_disk->_plane, 10, 33);
    s_disk_bar = tui_progress_create(win_disk->_plane, 6, 2, 20);
    if (s_disk_bar) tui_window_add_widget(win_disk, s_disk_bar, s_disk_bar->plane, tui_progress_get_type_id());

    /* Dashboard tab */
    TuiWindow* win_dash = tui_window_create(tab_dashboard, 40, 10);
    win_dash->_user_data = "System Dashboard";
    win_dash->_render_cb = system_dashboard_render;
    tui_tab_add_window(tab_dashboard, win_dash);
    ncplane_move_yx(win_dash->_plane, 2, 2);

    s_cpu_threshold = tui_slider_create(win_dash->_plane, 5, 2, 28, 0.0f, 100.0f, 80.0f, NULL, NULL);
    if (s_cpu_threshold) tui_window_add_widget(win_dash, s_cpu_threshold, s_cpu_threshold->plane, tui_slider_get_type_id());
    s_threshold_label = tui_label_create(win_dash->_plane, 3, 2, "");
    if (s_threshold_label) tui_window_add_widget(win_dash, s_threshold_label, s_threshold_label->plane, tui_label_get_type_id());
    s_activity_spinner = tui_spinner_create(win_dash->_plane, 7, 2, SPINNER_DOTS);
    if (s_activity_spinner) {
        tui_spinner_start(s_activity_spinner);
        tui_window_add_widget(win_dash, s_activity_spinner, s_activity_spinner->plane, tui_spinner_get_type_id());
    }

    return ws;
}
