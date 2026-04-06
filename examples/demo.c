/**
 * @file demo.c
 * @brief Comprehensive demo showcasing all 8 workspaces with interactive widgets.
 *
 * Workspaces: System, Console, Desktop, Finance, Browse, Development, Agents, Log
 * Navigation: Alt+1..9 to switch workspaces
 */

#include <notcurses/notcurses.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/theme.h"
#include "core/types_private.h"
#include "widget/text_input.h"
#include "widget/textarea.h"
#include "widget/button.h"
#include "widget/checkbox.h"
#include "widget/list.h"
#include "widget/progress.h"
#include "widget/label.h"

/* ============================================================
 * Global state
 * ============================================================ */

static TuiTextInput* g_active_input = NULL;
static int g_console_log_count = 0;

/* ============================================================
 * Window frame helper
 * ============================================================ */

static void render_window_frame(TuiWindow* win) {
    if (!win || !win->_plane) return;

    uint64_t bg_channels = 0;
    ncchannels_set_bg_rgb(&bg_channels, THEME_BG_WINDOW);
    ncchannels_set_fg_rgb(&bg_channels, THEME_BG_TITLE_INA);
    ncchannels_set_bg_alpha(&bg_channels, NCALPHA_OPAQUE);
    ncchannels_set_fg_alpha(&bg_channels, NCALPHA_OPAQUE);
    ncplane_set_base(win->_plane, " ", 0, bg_channels);
    ncplane_erase(win->_plane);

    unsigned int border_col = win->_focused ? THEME_BG_TITLE_ACT : THEME_FG_BORDER_INA;
    uint64_t border_channels = 0;
    ncchannels_set_fg_rgb(&border_channels, border_col);
    ncchannels_set_bg_rgb(&border_channels, THEME_BG_WINDOW);
    ncplane_perimeter_rounded(win->_plane, 0, border_channels, 0);

    unsigned dimy, dimx;
    ncplane_dim_yx(win->_plane, &dimy, &dimx);

    unsigned int title_bg = win->_focused ? THEME_BG_TITLE_ACT : THEME_BG_TITLE_INA;
    unsigned int title_fg = THEME_FG_TAB_ACTIVE;
    ncplane_set_bg_rgb(win->_plane, title_bg);
    ncplane_set_fg_rgb(win->_plane, title_fg);
    ncplane_printf_yx(win->_plane, 0, 2, " %s ", (char*)win->_user_data);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_CLOSE_BTN);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_putstr_yx(win->_plane, 0, dimx - 4, "[X]");

    ncplane_set_bg_rgb(win->_plane, THEME_BG_DARKEST);
    ncplane_set_fg_rgb(win->_plane, border_col);
    ncplane_putstr_yx(win->_plane, dimy - 1, dimx - 2, "⌟");
}

/* ============================================================
 * System workspace renderers
 * ============================================================ */

static void system_cpu_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    /* Mock CPU usage that changes over time */
    static float cpu_usage = 45.0f;
    cpu_usage += ((rand() % 10) - 5) * 0.5f;
    if (cpu_usage < 5.0f) cpu_usage = 5.0f;
    if (cpu_usage > 95.0f) cpu_usage = 95.0f;

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 2, 2, "CPU Usage: %.1f%%", cpu_usage);
    ncplane_printf_yx(win->_plane, 3, 2, "Cores: 8");
    ncplane_printf_yx(win->_plane, 4, 2, "Threads: 16");

    /* Progress bar */
    TuiProgressBar* prog = (TuiProgressBar*)tui_window_get_widget_at(win, 6, 2, NULL);
    if (prog) {
        tui_progress_set_value(prog, cpu_usage / 100.0f);
        tui_progress_render(prog);
    }

    tui_window_mark_dirty(win);
}

static void system_memory_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 2, 2, "Total: 32 GB");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 3, 2, "Used:  18.4 GB");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN);
    ncplane_printf_yx(win->_plane, 4, 2, "Free:  13.6 GB");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_DEBUG);
    ncplane_printf_yx(win->_plane, 5, 2, "Swap:  2.1 GB");

    tui_window_mark_dirty(win);
}

static void system_disk_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Disk Usage:");

    TuiList* disk_list = (TuiList*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (!disk_list) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "/dev/sda1  450GB/1TB");
        ncplane_printf_yx(win->_plane, 4, 2, "/dev/sda2  200GB/500GB");
        ncplane_printf_yx(win->_plane, 5, 2, "/dev/nvme0 120GB/1TB");
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Console workspace renderers
 * ============================================================ */

static void console_log_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    unsigned dimy, dimx;
    ncplane_dim_yx(win->_plane, &dimy, &dimx);

    TuiLogBuffer* buf = tui_logger_get_buffer();
    if (!buf || buf->count == 0) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 2, 2, "No logs yet. Type commands below.");
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

    tui_window_mark_dirty(win);
}

static void on_console_submit(const char* text, void* userdata) {
    (void)userdata;
    if (!text || text[0] == '\0') return;
    g_console_log_count++;
    tui_log(LOG_INFO, "[CMD #%d] >> %s", g_console_log_count, text);
}

void console_on_destroy(TuiWindow* win) {
    if (!win) return;
    if (win->_text_input) {
        if (g_active_input == (TuiTextInput*)win->_text_input) {
            g_active_input = NULL;
        }
        tui_text_input_destroy((TuiTextInput*)win->_text_input);
        win->_text_input = NULL;
    }
}

static void console_input_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_BG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "$ Command Input");

    if (win->_text_input) {
        TuiTextInput* ti = (TuiTextInput*)win->_text_input;
        ncplane_move_top(ti->plane);
        tui_text_input_render(ti);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Desktop workspace renderers
 * ============================================================ */

static void desktop_notes_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Quick Notes:");

    TuiTextArea* textarea = (TuiTextArea*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (textarea) {
        tui_textarea_render(textarea);
    } else {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "[TextArea widget]");
    }

    tui_window_mark_dirty(win);
}

static void desktop_shortcuts_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Quick Actions:");

    TuiButton* btn = (TuiButton*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (!btn) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "[Button: Refresh]");
        ncplane_printf_yx(win->_plane, 4, 2, "[Button: Settings]");
        ncplane_printf_yx(win->_plane, 5, 2, "[Button: Help]");
    }

    tui_window_mark_dirty(win);
}

static void desktop_status_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "System Status:");

    TuiCheckbox* cb = (TuiCheckbox*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (!cb) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "[ ] Network");
        ncplane_printf_yx(win->_plane, 4, 2, "[ ] Bluetooth");
        ncplane_printf_yx(win->_plane, 5, 2, "[ ] Auto-update");
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Finance workspace renderers
 * ============================================================ */

static void finance_stocks_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Stock Watchlist:");

    TuiList* stock_list = (TuiList*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (!stock_list) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_putstr_yx(win->_plane, 3, 2, "AAPL  $178.35  ▲ +1.2%%");
        ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
        ncplane_putstr_yx(win->_plane, 4, 2, "GOOGL $141.80  ▲ +0.8%%");
        ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_ERROR);
        ncplane_putstr_yx(win->_plane, 5, 2, "TSLA  $238.45  ▼ -2.1%%");
        ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
        ncplane_putstr_yx(win->_plane, 6, 2, "MSFT  $378.90  ▲ +0.5%%");
    }

    tui_window_mark_dirty(win);
}

static void finance_portfolio_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Portfolio Summary:");

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "Total Value: $125,430");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_putstr_yx(win->_plane, 4, 2, "Today: +$1,245 (+1.0%%)");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN);
    ncplane_printf_yx(win->_plane, 5, 2, "Cash: $15,200");

    /* Progress bar for allocation */
    TuiProgressBar* prog = (TuiProgressBar*)tui_window_get_widget_at(win, 7, 2, NULL);
    if (prog) {
        tui_progress_set_value(prog, 0.88f);
        tui_progress_render(prog);
    }

    tui_window_mark_dirty(win);
}

static void finance_watchlist_input_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Add Symbol:");

    TuiTextInput* input = (TuiTextInput*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (input) {
        tui_text_input_render(input);
    } else {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "[text input]");
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Browse workspace renderers
 * ============================================================ */

static void browse_url_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "URL:");

    TuiTextInput* input = (TuiTextInput*)tui_window_get_widget_at(win, 2, 6, NULL);
    if (input) {
        tui_text_input_render(input);
    }

    tui_window_mark_dirty(win);
}

static void browse_content_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Page Content:");

    TuiTextArea* textarea = (TuiTextArea*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (textarea) {
        tui_textarea_render(textarea);
    } else {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "Welcome to TMLCS-TUI Browser");
        ncplane_printf_yx(win->_plane, 4, 2, "Enter a URL above to navigate.");
    }

    tui_window_mark_dirty(win);
}

static void browse_bookmarks_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Bookmarks:");

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "1. github.com");
    ncplane_printf_yx(win->_plane, 4, 2, "2. stackoverflow.com");
    ncplane_printf_yx(win->_plane, 5, 2, "3. news.ycombinator.com");
    ncplane_printf_yx(win->_plane, 6, 2, "4. reddit.com");

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Development workspace renderers
 * ============================================================ */

static void dev_files_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "File Browser:");

    TuiList* file_list = (TuiList*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (!file_list) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "📁 src/");
        ncplane_printf_yx(win->_plane, 4, 2, "📁 include/");
        ncplane_printf_yx(win->_plane, 5, 2, "📁 tests/");
        ncplane_printf_yx(win->_plane, 6, 2, "📄 Makefile");
        ncplane_printf_yx(win->_plane, 7, 2, "📄 README.md");
    }

    tui_window_mark_dirty(win);
}

static void dev_editor_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Code Editor:");

    TuiTextArea* editor = (TuiTextArea*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (editor) {
        tui_textarea_render(editor);
    } else {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "1  #include <stdio.h>");
        ncplane_printf_yx(win->_plane, 4, 2, "2");
        ncplane_printf_yx(win->_plane, 5, 2, "3  int main() {");
        ncplane_printf_yx(win->_plane, 6, 2, "4      printf(\"Hello, TUI!\\n\");");
        ncplane_printf_yx(win->_plane, 7, 2, "5      return 0;");
        ncplane_printf_yx(win->_plane, 8, 2, "6  }");
    }

    tui_window_mark_dirty(win);
}

static void dev_git_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Git Status:");

    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 3, 2, "On branch main");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 4, 2, "Modified: src/core/manager/render.c");
    ncplane_printf_yx(win->_plane, 5, 2, "Modified: examples/demo.c");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN);
    ncplane_printf_yx(win->_plane, 6, 2, "Untracked: tests/new_test.c");

    tui_window_mark_dirty(win);
}

static void dev_build_progress_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Build Progress:");

    static float build_progress = 0.65f;
    build_progress += 0.01f;
    if (build_progress > 1.0f) build_progress = 0.0f;

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "Compiling... %.0f%%", build_progress * 100);

    TuiProgressBar* prog = (TuiProgressBar*)tui_window_get_widget_at(win, 4, 2, NULL);
    if (prog) {
        tui_progress_set_value(prog, build_progress);
        tui_progress_render(prog);
    }

    tui_window_mark_dirty(win);
}

static void dev_build_log_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Build Output:");

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "gcc -Wall -O2 -c main.c");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 4, 2, "✓ Compiled successfully");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 5, 2, "gcc -Wall -O2 -c utils.c");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 6, 2, "✓ Compiled successfully");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN);
    ncplane_printf_yx(win->_plane, 7, 2, "Warning: unused variable");

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Agents workspace renderers
 * ============================================================ */

static void agents_list_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Active Agents:");

    TuiList* agent_list = (TuiList*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (!agent_list) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
        ncplane_printf_yx(win->_plane, 3, 2, "● Code Review Agent");
        ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN);
        ncplane_printf_yx(win->_plane, 4, 2, "● Test Generator Agent");
        ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_DEBUG);
        ncplane_printf_yx(win->_plane, 5, 2, "● Doc Writer Agent");
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 6, 2, "○ Security Scanner Agent");
    }

    tui_window_mark_dirty(win);
}

static void agents_status_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Agent Configuration:");

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "[✓] Auto-review enabled");

    TuiCheckbox* cb = (TuiCheckbox*)tui_window_get_widget_at(win, 4, 2, NULL);
    if (!cb) {
        ncplane_printf_yx(win->_plane, 4, 2, "[ ] Auto-test generation");
        ncplane_printf_yx(win->_plane, 5, 2, "[ ] Auto-documentation");
    }

    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 6, 2, "Tasks completed: 142");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN);
    ncplane_printf_yx(win->_plane, 7, 2, "Pending: 3");

    tui_window_mark_dirty(win);
}

static void agents_controls_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Agent Controls:");

    TuiButton* btn = (TuiButton*)tui_window_get_widget_at(win, 3, 2, NULL);
    if (!btn) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 3, 2, "[Start All Agents]");
        ncplane_printf_yx(win->_plane, 4, 2, "[Stop All Agents]");
        ncplane_printf_yx(win->_plane, 5, 2, "[Run Review Now]");
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Log workspace renderers
 * ============================================================ */

static void log_viewer_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    unsigned dimy, dimx;
    ncplane_dim_yx(win->_plane, &dimy, &dimx);

    TuiLogBuffer* buf = tui_logger_get_buffer();
    if (!buf || buf->count == 0) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 2, 2, "No logs yet.");
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

    tui_window_mark_dirty(win);
}

static void log_filter_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Log Filters:");

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "[✓] Show INFO");
    ncplane_printf_yx(win->_plane, 4, 2, "[✓] Show WARN");

    TuiCheckbox* cb = (TuiCheckbox*)tui_window_get_widget_at(win, 5, 2, NULL);
    if (!cb) {
        ncplane_printf_yx(win->_plane, 5, 2, "[ ] Show DEBUG");
    }

    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 7, 2, "Search:");
    TuiTextInput* input = (TuiTextInput*)tui_window_get_widget_at(win, 7, 9, NULL);
    if (input) {
        tui_text_input_render(input);
    }

    tui_window_mark_dirty(win);
}

static void log_stats_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Log Statistics:");

    TuiLogBuffer* buf = tui_logger_get_buffer();
    int total = buf ? buf->count : 0;

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 3, 2, "Total entries: %d", total);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 4, 2, "Buffer capacity: %d", MAX_LOG_LINES);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_WARN);
    ncplane_printf_yx(win->_plane, 5, 2, "Console commands: %d", g_console_log_count);

    TuiProgressBar* prog = (TuiProgressBar*)tui_window_get_widget_at(win, 7, 2, NULL);
    if (prog) {
        float usage = (total > 0) ? (float)total / MAX_LOG_LINES : 0.0f;
        tui_progress_set_value(prog, usage);
        tui_progress_render(prog);
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 8, 2, "Buffer usage: %.0f%%", usage * 100);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Demo main - Create all 8 workspaces
 * ============================================================ */

int demo_main(struct notcurses* nc) {
    srand((unsigned int)time(NULL));

    TuiManager* mgr = tui_manager_create(nc);

    /* ============================================================
     * Workspace 1: System
     * ============================================================ */
    TuiWorkspace* ws_system = tui_workspace_create(mgr, "System");
    tui_manager_add_workspace(mgr, ws_system);

    TuiTab* tab_system = tui_tab_create(ws_system, "Overview");
    tui_workspace_add_tab(ws_system, tab_system);

    TuiWindow* win_sys_cpu = tui_window_create(tab_system, 25, 10);
    win_sys_cpu->_user_data = "CPU Usage";
    win_sys_cpu->_render_cb = system_cpu_render;
    tui_tab_add_window(tab_system, win_sys_cpu);
    ncplane_move_yx(win_sys_cpu->_plane, 2, 2);

    TuiWindow* win_sys_mem = tui_window_create(tab_system, 25, 8);
    win_sys_mem->_user_data = "Memory";
    win_sys_mem->_render_cb = system_memory_render;
    tui_tab_add_window(tab_system, win_sys_mem);
    ncplane_move_yx(win_sys_mem->_plane, 2, 28);

    TuiWindow* win_sys_disk = tui_window_create(tab_system, 30, 8);
    win_sys_disk->_user_data = "Disk Usage";
    win_sys_disk->_render_cb = system_disk_render;
    tui_tab_add_window(tab_system, win_sys_disk);
    ncplane_move_yx(win_sys_disk->_plane, 12, 2);

    /* ============================================================
     * Workspace 2: Console
     * ============================================================ */
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

    g_active_input = tui_text_input_create(win_con_input->_plane, 3, 2, 55,
                                            on_console_submit, NULL);
    if (g_active_input) {
        g_active_input->focused = true;
        win_con_input->_text_input = g_active_input;
        win_con_input->_on_destroy = console_on_destroy;
        tui_text_input_ensure_registered();
        tui_window_add_widget(win_con_input, g_active_input, g_active_input->plane, tui_text_input_get_type_id());
    }

    /* ============================================================
     * Workspace 3: Desktop
     * ============================================================ */
    TuiWorkspace* ws_desktop = tui_workspace_create(mgr, "Desktop");
    tui_manager_add_workspace(mgr, ws_desktop);

    TuiTab* tab_desktop = tui_tab_create(ws_desktop, "Widgets");
    tui_workspace_add_tab(ws_desktop, tab_desktop);

    TuiWindow* win_desk_notes = tui_window_create(tab_desktop, 35, 10);
    win_desk_notes->_user_data = "Quick Notes";
    win_desk_notes->_render_cb = desktop_notes_render;
    tui_tab_add_window(tab_desktop, win_desk_notes);
    ncplane_move_yx(win_desk_notes->_plane, 2, 2);

    TuiWindow* win_desk_shortcuts = tui_window_create(tab_desktop, 25, 8);
    win_desk_shortcuts->_user_data = "Shortcuts";
    win_desk_shortcuts->_render_cb = desktop_shortcuts_render;
    tui_tab_add_window(tab_desktop, win_desk_shortcuts);
    ncplane_move_yx(win_desk_shortcuts->_plane, 2, 38);

    TuiWindow* win_desk_status = tui_window_create(tab_desktop, 25, 8);
    win_desk_status->_user_data = "System Status";
    win_desk_status->_render_cb = desktop_status_render;
    tui_tab_add_window(tab_desktop, win_desk_status);
    ncplane_move_yx(win_desk_status->_plane, 12, 38);

    /* ============================================================
     * Workspace 4: Finance
     * ============================================================ */
    TuiWorkspace* ws_finance = tui_workspace_create(mgr, "Finance");
    tui_manager_add_workspace(mgr, ws_finance);

    TuiTab* tab_finance = tui_tab_create(ws_finance, "Markets");
    tui_workspace_add_tab(ws_finance, tab_finance);

    TuiWindow* win_fin_stocks = tui_window_create(tab_finance, 30, 10);
    win_fin_stocks->_user_data = "Stock Watchlist";
    win_fin_stocks->_render_cb = finance_stocks_render;
    tui_tab_add_window(tab_finance, win_fin_stocks);
    ncplane_move_yx(win_fin_stocks->_plane, 2, 2);

    TuiWindow* win_fin_portfolio = tui_window_create(tab_finance, 30, 10);
    win_fin_portfolio->_user_data = "Portfolio";
    win_fin_portfolio->_render_cb = finance_portfolio_render;
    tui_tab_add_window(tab_finance, win_fin_portfolio);
    ncplane_move_yx(win_fin_portfolio->_plane, 2, 33);

    TuiWindow* win_fin_input = tui_window_create(tab_finance, 25, 5);
    win_fin_input->_user_data = "Add Symbol";
    win_fin_input->_render_cb = finance_watchlist_input_render;
    tui_tab_add_window(tab_finance, win_fin_input);
    ncplane_move_yx(win_fin_input->_plane, 12, 2);

    /* ============================================================
     * Workspace 5: Browse
     * ============================================================ */
    TuiWorkspace* ws_browse = tui_workspace_create(mgr, "Browse");
    tui_manager_add_workspace(mgr, ws_browse);

    TuiTab* tab_browse = tui_tab_create(ws_browse, "Web");
    tui_workspace_add_tab(ws_browse, tab_browse);

    TuiWindow* win_browse_url = tui_window_create(tab_browse, 50, 4);
    win_browse_url->_user_data = "Navigation";
    win_browse_url->_render_cb = browse_url_render;
    tui_tab_add_window(tab_browse, win_browse_url);
    ncplane_move_yx(win_browse_url->_plane, 2, 2);

    TuiWindow* win_browse_content = tui_window_create(tab_browse, 50, 12);
    win_browse_content->_user_data = "Page Content";
    win_browse_content->_render_cb = browse_content_render;
    tui_tab_add_window(tab_browse, win_browse_content);
    ncplane_move_yx(win_browse_content->_plane, 6, 2);

    TuiWindow* win_browse_bookmarks = tui_window_create(tab_browse, 25, 8);
    win_browse_bookmarks->_user_data = "Bookmarks";
    win_browse_bookmarks->_render_cb = browse_bookmarks_render;
    tui_tab_add_window(tab_browse, win_browse_bookmarks);
    ncplane_move_yx(win_browse_bookmarks->_plane, 6, 53);

    /* ============================================================
     * Workspace 6: Development
     * ============================================================ */
    TuiWorkspace* ws_dev = tui_workspace_create(mgr, "Development");
    tui_manager_add_workspace(mgr, ws_dev);

    TuiTab* tab_dev_editor = tui_tab_create(ws_dev, "Editor");
    tui_workspace_add_tab(ws_dev, tab_dev_editor);

    TuiWindow* win_dev_files = tui_window_create(tab_dev_editor, 20, 15);
    win_dev_files->_user_data = "Files";
    win_dev_files->_render_cb = dev_files_render;
    tui_tab_add_window(tab_dev_editor, win_dev_files);
    ncplane_move_yx(win_dev_files->_plane, 2, 2);

    TuiWindow* win_dev_code = tui_window_create(tab_dev_editor, 45, 10);
    win_dev_code->_user_data = "main.c";
    win_dev_code->_render_cb = dev_editor_render;
    tui_tab_add_window(tab_dev_editor, win_dev_code);
    ncplane_move_yx(win_dev_code->_plane, 2, 23);

    TuiWindow* win_dev_git = tui_window_create(tab_dev_editor, 45, 5);
    win_dev_git->_user_data = "Git Status";
    win_dev_git->_render_cb = dev_git_render;
    tui_tab_add_window(tab_dev_editor, win_dev_git);
    ncplane_move_yx(win_dev_git->_plane, 12, 23);

    TuiTab* tab_dev_build = tui_tab_create(ws_dev, "Build");
    tui_workspace_add_tab(ws_dev, tab_dev_build);

    TuiWindow* win_dev_build_prog = tui_window_create(tab_dev_build, 40, 8);
    win_dev_build_prog->_user_data = "Build Progress";
    win_dev_build_prog->_render_cb = dev_build_progress_render;
    tui_tab_add_window(tab_dev_build, win_dev_build_prog);
    ncplane_move_yx(win_dev_build_prog->_plane, 2, 2);

    TuiWindow* win_dev_build_log = tui_window_create(tab_dev_build, 40, 8);
    win_dev_build_log->_user_data = "Build Log";
    win_dev_build_log->_render_cb = dev_build_log_render;
    tui_tab_add_window(tab_dev_build, win_dev_build_log);
    ncplane_move_yx(win_dev_build_log->_plane, 2, 43);

    /* ============================================================
     * Workspace 7: Agents
     * ============================================================ */
    TuiWorkspace* ws_agents = tui_workspace_create(mgr, "Agents");
    tui_manager_add_workspace(mgr, ws_agents);

    TuiTab* tab_agents = tui_tab_create(ws_agents, "Active Agents");
    tui_workspace_add_tab(ws_agents, tab_agents);

    TuiWindow* win_agents_list = tui_window_create(tab_agents, 30, 10);
    win_agents_list->_user_data = "Agent List";
    win_agents_list->_render_cb = agents_list_render;
    tui_tab_add_window(tab_agents, win_agents_list);
    ncplane_move_yx(win_agents_list->_plane, 2, 2);

    TuiWindow* win_agents_status = tui_window_create(tab_agents, 30, 10);
    win_agents_status->_user_data = "Agent Status";
    win_agents_status->_render_cb = agents_status_render;
    tui_tab_add_window(tab_agents, win_agents_status);
    ncplane_move_yx(win_agents_status->_plane, 2, 33);

    TuiWindow* win_agents_controls = tui_window_create(tab_agents, 25, 8);
    win_agents_controls->_user_data = "Controls";
    win_agents_controls->_render_cb = agents_controls_render;
    tui_tab_add_window(tab_agents, win_agents_controls);
    ncplane_move_yx(win_agents_controls->_plane, 12, 2);

    /* ============================================================
     * Workspace 8: Log
     * ============================================================ */
    TuiWorkspace* ws_log = tui_workspace_create(mgr, "Log");
    tui_manager_add_workspace(mgr, ws_log);

    TuiTab* tab_log = tui_tab_create(ws_log, "Debug Log");
    tui_workspace_add_tab(ws_log, tab_log);

    TuiWindow* win_log_viewer = tui_window_create(tab_log, 50, 12);
    win_log_viewer->_user_data = "Log Viewer";
    win_log_viewer->_render_cb = log_viewer_render;
    tui_tab_add_window(tab_log, win_log_viewer);
    ncplane_move_yx(win_log_viewer->_plane, 2, 2);

    TuiWindow* win_log_filter = tui_window_create(tab_log, 25, 10);
    win_log_filter->_user_data = "Filters";
    win_log_filter->_render_cb = log_filter_render;
    tui_tab_add_window(tab_log, win_log_filter);
    ncplane_move_yx(win_log_filter->_plane, 2, 53);

    TuiWindow* win_log_stats = tui_window_create(tab_log, 50, 6);
    win_log_stats->_user_data = "Statistics";
    win_log_stats->_render_cb = log_stats_render;
    tui_tab_add_window(tab_log, win_log_stats);
    ncplane_move_yx(win_log_stats->_plane, 14, 2);

    /* ============================================================
     * Activate first workspace
     * ============================================================ */
    tui_manager_set_active_workspace(mgr, 0);

    tui_log(LOG_INFO, "TMLCS-TUI Demo started with 8 workspaces.");
    tui_log(LOG_INFO, "Use Alt+1..9 to switch workspaces.");

    /* ============================================================
     * Event Loop
     * ============================================================ */
    struct ncinput ni;
    while (mgr->_running) {
        tui_manager_render(mgr);

        uint32_t key = notcurses_get_nblock(nc, &ni);

        if (key == (uint32_t)-1) {
            struct timespec ts = {0, 16000000};
            nanosleep(&ts, NULL);
            continue;
        }

        if (key >= NCKEY_BUTTON1 && key <= NCKEY_BUTTON11) {
            tui_manager_process_mouse(mgr, key, &ni);
        } else if (key == NCKEY_MOTION) {
            tui_manager_process_mouse(mgr, key, &ni);
        } else {
            if (key == 'x') {
                tui_manager_process_keyboard(mgr, key, &ni);
            } else {
                bool consumed = g_active_input
                                ? tui_text_input_handle_key(g_active_input, key, &ni)
                                : false;
                if (!consumed) {
                    tui_manager_process_keyboard(mgr, key, &ni);
                }
            }
        }
    }

    tui_manager_destroy(mgr);
    return 0;
}
