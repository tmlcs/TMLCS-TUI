/**
 * @file demo_log.c
 * @brief Log workspace: log viewer list, level filter checkboxes,
 *         search input, and buffer usage progress.
 */

#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/logger.h"
#include "widget/list.h"
#include "widget/checkbox.h"
#include "widget/text_input.h"
#include "widget/progress.h"
#include <stdio.h>

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiCheckbox* s_info_cb;
static TuiCheckbox* s_warn_cb;
static TuiCheckbox* s_error_cb;
static TuiCheckbox* s_debug_cb;
static TuiTextInput* s_search_input;
static TuiProgressBar* s_buffer_bar;

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_search_submit(const char* text, void* ud) {
    (void)ud;
    if (text && text[0]) tui_log(LOG_INFO, "Searching logs for: %s", text);
}

/* ============================================================
 * Renderers
 * ============================================================ */

static void log_viewer_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    unsigned dimy, dimx;
    ncplane_dim_yx(win->_plane, &dimy, &dimx);

    TuiLogBuffer* buf = tui_logger_get_buffer();
    if (!buf || buf->count == 0) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 2, 2, "No logs yet.");
        return;
    }

    int rows = (int)dimy - 2;
    if (rows <= 0) return;
    int entries = (buf->count < rows) ? buf->count : rows;
    int start = (buf->head - entries + MAX_LOG_LINES) % MAX_LOG_LINES;
    int offset = rows - entries;

    for (int i = 0; i < entries; i++) {
        int idx = (start + i) % MAX_LOG_LINES;
        LogLevel lv = buf->levels[idx];
        switch(lv) {
            case LOG_ERROR: ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_ERROR); break;
            case LOG_WARN:  ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN); break;
            case LOG_DEBUG: ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_DEBUG); break;
            default:        ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO); break;
        }
        ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
        char line[256];
        int max_cols = (int)dimx - 4;
        if (max_cols <= 0) max_cols = 1;
        snprintf(line, sizeof(line), "%.*s", max_cols, buf->lines[idx]);
        ncplane_printf_yx(win->_plane, 1 + offset + i, 2, "%s", line);
    }
}

static void log_filters_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Filter Levels:");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 7, 2, "Search:");
    if (s_search_input) {
        ncplane_move_top(s_search_input->plane);
        tui_text_input_render(s_search_input);
    }
}

static void log_stats_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    TuiLogBuffer* buf = tui_logger_get_buffer();
    int total = buf ? buf->count : 0;
    float usage = (float)total / (float)MAX_LOG_LINES;
    if (usage > 1.0f) usage = 1.0f;

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 2, 2, "Entries: %d / %d", total, MAX_LOG_LINES);
    ncplane_printf_yx(win->_plane, 3, 2, "Commands: %d", g_demo.console_log_count);

    if (s_buffer_bar) {
        tui_progress_set_value(s_buffer_bar, usage);
        tui_progress_render(s_buffer_bar);
    }
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_printf_yx(win->_plane, 6, 2, "Buffer: %.0f%%", usage * 100);
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_log_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "Log");
    if (!ws) return NULL;

    TuiTab* tab = tui_tab_create(ws, "Debug Log");
    tui_workspace_add_tab(ws, tab);

    /* Log viewer */
    TuiWindow* win_viewer = tui_window_create(tab, 50, 14);
    win_viewer->_user_data = "Log Viewer";
    win_viewer->_render_cb = log_viewer_render;
    tui_tab_add_window(tab, win_viewer);
    ncplane_move_yx(win_viewer->_plane, 2, 2);

    /* Filters */
    TuiWindow* win_filters = tui_window_create(tab, 25, 10);
    win_filters->_user_data = "Filters";
    win_filters->_render_cb = log_filters_render;
    tui_tab_add_window(tab, win_filters);
    ncplane_move_yx(win_filters->_plane, 2, 53);
    s_info_cb = tui_checkbox_create(win_filters->_plane, 3, 2, "INFO", true, NULL, NULL);
    if (s_info_cb) tui_window_add_widget(win_filters, s_info_cb, s_info_cb->plane, tui_checkbox_get_type_id());
    s_warn_cb = tui_checkbox_create(win_filters->_plane, 4, 2, "WARN", true, NULL, NULL);
    if (s_warn_cb) tui_window_add_widget(win_filters, s_warn_cb, s_warn_cb->plane, tui_checkbox_get_type_id());
    s_error_cb = tui_checkbox_create(win_filters->_plane, 5, 2, "ERROR", true, NULL, NULL);
    if (s_error_cb) tui_window_add_widget(win_filters, s_error_cb, s_error_cb->plane, tui_checkbox_get_type_id());
    s_debug_cb = tui_checkbox_create(win_filters->_plane, 6, 2, "DEBUG", false, NULL, NULL);
    if (s_debug_cb) tui_window_add_widget(win_filters, s_debug_cb, s_debug_cb->plane, tui_checkbox_get_type_id());
    s_search_input = tui_text_input_create(win_filters->_plane, 7, 9, 14, on_search_submit, NULL);
    if (s_search_input) tui_window_add_widget(win_filters, s_search_input, s_search_input->plane, tui_text_input_get_type_id());

    /* Stats */
    TuiWindow* win_stats = tui_window_create(tab, 50, 4);
    win_stats->_user_data = "Statistics";
    win_stats->_render_cb = log_stats_render;
    tui_tab_add_window(tab, win_stats);
    ncplane_move_yx(win_stats->_plane, 16, 2);
    s_buffer_bar = tui_progress_create(win_stats->_plane, 2, 14, 20);
    if (s_buffer_bar) tui_window_add_widget(win_stats, s_buffer_bar, s_buffer_bar->plane, tui_progress_get_type_id());

    return ws;
}
