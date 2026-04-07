#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/theme.h"
#include "core/types_private.h"
#include "widget/label.h"
#include "widget/slider.h"
#include "widget/list.h"
#include "widget/progress.h"

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_slider_change(float value, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Slider] CPU threshold: %.0f%%", value);
}

/* ============================================================
 * Persistent widget pointers
 * ============================================================ */

static TuiProgressBar* g_sys_cpu_prog = NULL;
static TuiSlider* g_sys_cpu_slider = NULL;
static TuiList* g_sys_disk_list = NULL;

/* ============================================================
 * System workspace renderers
 * ============================================================ */

static void system_cpu_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 2, 2, "CPU: %.1f%%", g_demo.cpu_usage);
    ncplane_printf_yx(p, 3, 2, "Cores: 8  Threads: 16");

    if (g_sys_cpu_prog) {
        tui_progress_set_value(g_sys_cpu_prog, g_demo.cpu_usage / 100.0f);
        tui_progress_render(g_sys_cpu_prog);
    }

    tui_window_mark_dirty(win);
}

static void system_memory_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 2, 2, "Total: 32 GB");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO);
    ncplane_printf_yx(p, 3, 2, "Used:  18.4 GB");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN);
    ncplane_printf_yx(p, 4, 2, "Free:  13.6 GB");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_DEBUG);
    ncplane_printf_yx(p, 5, 2, "Swap:  2.1 GB");

    tui_window_mark_dirty(win);
}

static void system_disk_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Disk Usage:");

    if (g_sys_disk_list) {
        tui_list_render(g_sys_disk_list);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Factory function
 * ============================================================ */

TuiWorkspace* create_system_workspace(TuiManager* mgr) {
    TuiWorkspace* ws_system = tui_workspace_create(mgr, "System");
    tui_manager_add_workspace(mgr, ws_system);

    TuiTab* tab_system = tui_tab_create(ws_system, "Overview");
    tui_workspace_add_tab(ws_system, tab_system);

    /* CPU window */
    TuiWindow* win_sys_cpu = tui_window_create(tab_system, 35, 10);
    win_sys_cpu->_user_data = "CPU Usage";
    win_sys_cpu->_render_cb = system_cpu_render;
    tui_tab_add_window(tab_system, win_sys_cpu);
    ncplane_move_yx(win_sys_cpu->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_sys_cpu);
        g_sys_cpu_prog = tui_progress_create(p, 3, 2, 30);
        if (g_sys_cpu_prog) {
            tui_progress_set_value(g_sys_cpu_prog, 0.45f);
            tui_progress_ensure_registered();
            tui_window_add_widget(win_sys_cpu, g_sys_cpu_prog, g_sys_cpu_prog->plane, tui_progress_get_type_id());
        }

        g_sys_cpu_slider = tui_slider_create(p, 5, 2, 30, 0.0f, 100.0f, 1.0f,
                                              on_slider_change, NULL);
        if (g_sys_cpu_slider) {
            tui_slider_set_value(g_sys_cpu_slider, 75.0f);
            tui_slider_ensure_registered();
            tui_window_add_widget(win_sys_cpu, g_sys_cpu_slider, g_sys_cpu_slider->plane, tui_slider_get_type_id());
        }

        TuiLabel* cpu_label = tui_label_create(p, 6, 2, "CPU Threshold: 75%");
        if (cpu_label) {
            tui_label_ensure_registered();
            tui_window_add_widget(win_sys_cpu, cpu_label, cpu_label->plane, tui_label_get_type_id());
        }
    }

    /* Memory window */
    TuiWindow* win_sys_mem = tui_window_create(tab_system, 25, 8);
    win_sys_mem->_user_data = "Memory";
    win_sys_mem->_render_cb = system_memory_render;
    tui_tab_add_window(tab_system, win_sys_mem);
    ncplane_move_yx(win_sys_mem->_plane, 2, 38);

    /* Disk window */
    TuiWindow* win_sys_disk = tui_window_create(tab_system, 25, 10);
    win_sys_disk->_user_data = "Disk Usage";
    win_sys_disk->_render_cb = system_disk_render;
    tui_tab_add_window(tab_system, win_sys_disk);
    ncplane_move_yx(win_sys_disk->_plane, 12, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_sys_disk);
        g_sys_disk_list = tui_list_create(p, 3, 2, 20, 6);
        if (g_sys_disk_list) {
            tui_list_add_item(g_sys_disk_list, "/dev/sda1  450GB/1TB");
            tui_list_add_item(g_sys_disk_list, "/dev/sda2  200GB/500GB");
            tui_list_add_item(g_sys_disk_list, "/dev/nvme0 120GB/1TB");
            tui_list_add_item(g_sys_disk_list, "/dev/sdb1  800GB/2TB");
            tui_list_set_selected(g_sys_disk_list, 0);
            tui_list_ensure_registered();
            tui_window_add_widget(win_sys_disk, g_sys_disk_list, g_sys_disk_list->plane, tui_list_get_type_id());
        }
    }

    return ws_system;
}
