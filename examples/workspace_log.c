#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/theme.h"
#include "core/types_private.h"
#include "widget/text_input.h"
#include "widget/checkbox.h"
#include "widget/progress.h"
#include "widget/label.h"

/* ============================================================
 * Persistent widget pointers
 * ============================================================ */

static TuiCheckbox* g_log_cb_debug = NULL;
static TuiTextInput* g_log_search = NULL;
static TuiProgressBar* g_log_buffer_prog = NULL;
static TuiLabel* g_log_stats_label = NULL;

/* ============================================================
 * Log workspace renderers
 * ============================================================ */

static void log_viewer_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    unsigned dimy, dimx;
    ncplane_dim_yx(p, &dimy, &dimx);

    TuiLogBuffer* buf = tui_logger_get_buffer();
    if (!buf || buf->count == 0) {
        ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
        ncplane_printf_yx(p, 2, 2, "No logs yet.");
        return;
    }

    int rows_available = (int)dimy - 2;
    if (rows_available <= 0) return;

    int entries = (buf->count < rows_available) ? buf->count : rows_available;
    int start = (buf->head - entries + MAX_LOG_LINES) % MAX_LOG_LINES;
    int offset = rows_available - entries;

    for (int i = 0; i < entries; i++) {
        int idx = (start + i) % MAX_LOG_LINES;
        LogLevel lv = buf->levels[idx];

        switch (lv) {
        case LOG_ERROR: ncplane_set_fg_rgb(p, THEME_FG_LOG_ERROR); break;
        case LOG_WARN:  ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN); break;
        case LOG_DEBUG: ncplane_set_fg_rgb(p, THEME_FG_LOG_DEBUG); break;
        default:        ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO); break;
        }
        ncplane_set_bg_rgb(p, THEME_BG_WINDOW);

        char line[256];
        int max_cols = (int)dimx - 4;
        if (max_cols <= 0) max_cols = 1;
        snprintf(line, sizeof(line), "%.*s", max_cols, buf->lines[idx]);
        ncplane_printf_yx(p, 1 + offset + i, 2, "%s", line);
    }

    tui_window_mark_dirty(win);
}

static void log_filter_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Log Filters:");

    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 3, 2, "[+] Show INFO");
    ncplane_printf_yx(p, 4, 2, "[+] Show WARN");

    if (g_log_cb_debug) {
        tui_checkbox_render(g_log_cb_debug);
    }

    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 7, 2, "Search:");
    if (g_log_search) {
        tui_text_input_render(g_log_search);
    }

    tui_window_mark_dirty(win);
}

static void log_stats_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Log Statistics:");

    TuiLogBuffer* buf = tui_logger_get_buffer();
    int total = buf ? buf->count : 0;

    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 3, 2, "Total entries: %d", total);
    ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN);
    ncplane_printf_yx(p, 4, 2, "Console commands: %d", g_demo.console_log_count);

    if (g_log_buffer_prog) {
        float usage = (total > 0) ? (float)total / MAX_LOG_LINES : 0.0f;
        tui_progress_set_value(g_log_buffer_prog, usage);
        tui_progress_render(g_log_buffer_prog);
        ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
        ncplane_printf_yx(p, 6, 2, "Buffer usage: %.0f%%", usage * 100);
    }

    if (g_log_stats_label) {
        tui_label_render(g_log_stats_label);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Factory function
 * ============================================================ */

TuiWorkspace* create_log_workspace(TuiManager* mgr) {
    TuiWorkspace* ws_log = tui_workspace_create(mgr, "Log");
    tui_manager_add_workspace(mgr, ws_log);

    TuiTab* tab_log = tui_tab_create(ws_log, "Debug Log");
    tui_workspace_add_tab(ws_log, tab_log);

    TuiWindow* win_log_viewer = tui_window_create(tab_log, 50, 14);
    win_log_viewer->_user_data = "Log Viewer";
    win_log_viewer->_render_cb = log_viewer_render;
    tui_tab_add_window(tab_log, win_log_viewer);
    ncplane_move_yx(win_log_viewer->_plane, 2, 2);

    TuiWindow* win_log_filter = tui_window_create(tab_log, 25, 12);
    win_log_filter->_user_data = "Filters";
    win_log_filter->_render_cb = log_filter_render;
    tui_tab_add_window(tab_log, win_log_filter);
    ncplane_move_yx(win_log_filter->_plane, 2, 53);

    {
        struct ncplane* p = tui_window_get_plane(win_log_filter);
        g_log_cb_debug = tui_checkbox_create(p, 5, 2, " Show DEBUG", false, NULL, NULL);
        if (g_log_cb_debug) {
            tui_checkbox_ensure_registered();
            tui_window_add_widget(win_log_filter, g_log_cb_debug, g_log_cb_debug->plane, tui_checkbox_get_type_id());
        }

        g_log_search = tui_text_input_create(p, 8, 2, 20, NULL, NULL);
        if (g_log_search) {
            tui_text_input_ensure_registered();
            tui_window_add_widget(win_log_filter, g_log_search, g_log_search->plane, tui_text_input_get_type_id());
        }
    }

    TuiWindow* win_log_stats = tui_window_create(tab_log, 50, 8);
    win_log_stats->_user_data = "Statistics";
    win_log_stats->_render_cb = log_stats_render;
    tui_tab_add_window(tab_log, win_log_stats);
    ncplane_move_yx(win_log_stats->_plane, 16, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_log_stats);
        g_log_buffer_prog = tui_progress_create(p, 5, 2, 40);
        if (g_log_buffer_prog) {
            tui_progress_ensure_registered();
            tui_window_add_widget(win_log_stats, g_log_buffer_prog, g_log_buffer_prog->plane, tui_progress_get_type_id());
        }

        g_log_stats_label = tui_label_create(p, 7, 2, "Log buffer ring active");
        if (g_log_stats_label) {
            tui_label_ensure_registered();
            tui_window_add_widget(win_log_stats, g_log_stats_label, g_log_stats_label->plane, tui_label_get_type_id());
        }
    }

    return ws_log;
}
