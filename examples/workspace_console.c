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

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_console_submit(const char* text, void* userdata) {
    (void)userdata;
    if (!text || text[0] == '\0') return;
    g_demo.console_log_count++;
    tui_log(LOG_INFO, "[CMD #%d] >> %s", g_demo.console_log_count, text);
}

void console_on_destroy(TuiWindow* win) {
    if (!win) return;
    if (win->_text_input) {
        if (g_demo.active_input == (TuiTextInput*)win->_text_input) {
            g_demo.active_input = NULL;
        }
        tui_text_input_destroy((TuiTextInput*)win->_text_input);
        win->_text_input = NULL;
    }
}

/* ============================================================
 * Console workspace renderers
 * ============================================================ */

static void console_log_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    unsigned dimy, dimx;
    ncplane_dim_yx(p, &dimy, &dimx);

    TuiLogBuffer* buf = tui_logger_get_buffer();
    if (!buf || buf->count == 0) {
        ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
        ncplane_printf_yx(p, 2, 2, "No logs yet. Type commands below.");
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

static void console_input_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_BG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "$ Command Input");

    if (win->_text_input) {
        TuiTextInput* ti = (TuiTextInput*)win->_text_input;
        ncplane_move_top(ti->plane);
        tui_text_input_render(ti);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Factory function
 * ============================================================ */

TuiWorkspace* create_console_workspace(TuiManager* mgr) {
    TuiWorkspace* ws_console = tui_workspace_create(mgr, "Console");
    tui_manager_add_workspace(mgr, ws_console);

    TuiTab* tab_console = tui_tab_create(ws_console, "Terminal");
    tui_workspace_add_tab(ws_console, tab_console);

    TuiWindow* win_con_log = tui_window_create(tab_console, 60, 15);
    win_con_log->_user_data = "Log Viewer";
    win_con_log->_render_cb = console_log_render;
    tui_tab_add_window(tab_console, win_con_log);
    ncplane_move_yx(win_con_log->_plane, 2, 2);

    TuiWindow* win_con_input = tui_window_create(tab_console, 60, 5);
    win_con_input->_user_data = "Command Input";
    win_con_input->_render_cb = console_input_render;
    tui_tab_add_window(tab_console, win_con_input);
    ncplane_move_yx(win_con_input->_plane, 17, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_con_input);
        g_demo.active_input = tui_text_input_create(p, 3, 2, 55, on_console_submit, NULL);
        if (g_demo.active_input) {
            g_demo.active_input->focused = true;
            win_con_input->_text_input = g_demo.active_input;
            win_con_input->_on_destroy = console_on_destroy;
            tui_text_input_ensure_registered();
            tui_window_add_widget(win_con_input, g_demo.active_input, g_demo.active_input->plane, tui_text_input_get_type_id());
        }
    }

    return ws_console;
}
