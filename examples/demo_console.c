/**
 * @file demo_console.c
 * @brief Console workspace: command input with log viewer and history.
 */

#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/theme.h"
#include "widget/text_input.h"
#include "widget/textarea.h"
#include "widget/list.h"

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiTextInput* s_cmd_input;
static TuiList* s_history_list;
static char s_command_history[20][256];
static int s_history_count = 0;

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_command_submit(const char* text, void* userdata) {
    (void)userdata;
    if (!text || text[0] == '\0') return;
    g_demo.console_log_count++;
    tui_log(LOG_INFO, "[CMD #%d] >> %s", g_demo.console_log_count, text);

    /* Add to history */
    if (s_history_count < 20) {
        strncpy(s_command_history[s_history_count], text, 255);
        s_command_history[s_history_count][255] = '\0';
        if (s_history_list) tui_list_add_item(s_history_list, s_command_history[s_history_count]);
        s_history_count++;
    }
}

/* ============================================================
 * Renderers
 * ============================================================ */

static void console_log_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    unsigned dimy, dimx;
    ncplane_dim_yx(win->_plane, &dimy, &dimx);

    TuiLogBuffer* buf = tui_logger_get_buffer();
    if (!buf || buf->count == 0) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 2, 2, "No logs yet. Type commands below.");
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

static void console_input_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    ncplane_set_fg_rgb(win->_plane, THEME_BG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "$ Command Input");

    if (s_cmd_input) {
        ncplane_move_top(s_cmd_input->plane);
        tui_text_input_render(s_cmd_input);
        g_demo.active_input = s_cmd_input;
    }
}

static void console_history_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_console_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "Console");
    if (!ws) return NULL;

    TuiTab* tab = tui_tab_create(ws, "Terminal");
    tui_workspace_add_tab(ws, tab);

    TuiWindow* win_log = tui_window_create(tab, 60, 12);
    win_log->_user_data = "Log Viewer";
    win_log->_render_cb = console_log_render;
    tui_tab_add_window(tab, win_log);
    ncplane_move_yx(win_log->_plane, 2, 2);

    TuiWindow* win_input = tui_window_create(tab, 60, 5);
    win_input->_user_data = "Command Input";
    win_input->_render_cb = console_input_render;
    tui_tab_add_window(tab, win_input);
    ncplane_move_yx(win_input->_plane, 14, 2);

    s_cmd_input = tui_text_input_create(win_input->_plane, 3, 2, 54, on_command_submit, NULL);
    if (s_cmd_input) {
        s_cmd_input->focused = true;
        win_input->_text_input = s_cmd_input;
        tui_window_add_widget(win_input, s_cmd_input, s_cmd_input->plane, tui_text_input_get_type_id());
    }

    TuiWindow* win_history = tui_window_create(tab, 25, 12);
    win_history->_user_data = "History";
    win_history->_render_cb = console_history_render;
    tui_tab_add_window(tab, win_history);
    ncplane_move_yx(win_history->_plane, 2, 63);

    s_history_list = tui_list_create(win_history->_plane, 3, 2, 20, 8);
    if (s_history_list) tui_window_add_widget(win_history, s_history_list, s_history_list->plane, tui_list_get_type_id());

    return ws;
}
