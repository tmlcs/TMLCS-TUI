/**
 * @file demo.c
 * @brief Comprehensive demo showcasing all workspaces with real interactive widgets.
 *
 * Workspaces: System, Console, Desktop, Finance, Browse, Development, Agents, Log
 * Navigation: Alt+1..9 to switch workspaces
 *
 * Showcases: Text Input, TextArea, Button, Checkbox, List, ProgressBar, Label,
 *            Slider, RadioGroup, Spinner, TabContainer, Dropdown, Dialog,
 *            Table, Tree, FilePicker
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
#include "core/loop.h"
#include "core/types_private.h"
#include "core/layout.h"
#include "core/theme_loader.h"
#include "widget/text_input.h"
#include "widget/textarea.h"
#include "widget/button.h"
#include "widget/checkbox.h"
#include "widget/list.h"
#include "widget/progress.h"
#include "widget/label.h"
#include "widget/slider.h"
#include "widget/radio_group.h"
#include "widget/spinner.h"
#include "widget/tab_container.h"
#include "widget/dropdown.h"
#include "widget/dialog.h"
#include "widget/table.h"
#include "widget/tree.h"
#include "widget/file_picker.h"

/* Forward declarations for ensure_registered / get_type_id (not in public headers) */
void tui_button_ensure_registered(void);
int tui_button_get_type_id(void);
void tui_checkbox_ensure_registered(void);
int tui_checkbox_get_type_id(void);
void tui_label_ensure_registered(void);
int tui_label_get_type_id(void);
void tui_list_ensure_registered(void);
int tui_list_get_type_id(void);
void tui_progress_ensure_registered(void);
int tui_progress_get_type_id(void);
void tui_textarea_ensure_registered(void);
int tui_textarea_get_type_id(void);
void tui_slider_ensure_registered(void);
int tui_slider_get_type_id(void);
void tui_radio_group_ensure_registered(void);
int tui_radio_group_get_type_id(void);
void tui_spinner_ensure_registered(void);
int tui_spinner_get_type_id(void);
void tui_tab_container_ensure_registered(void);
int tui_tab_container_get_type_id(void);
void tui_dropdown_ensure_registered(void);
int tui_dropdown_get_type_id(void);
void tui_dialog_ensure_registered(void);
int tui_dialog_get_type_id(void);
void tui_table_ensure_registered(void);
int tui_table_get_type_id(void);
void tui_tree_ensure_registered(void);
int tui_tree_get_type_id(void);
void tui_file_picker_ensure_registered(void);
int tui_file_picker_get_type_id(void);

/* ============================================================
 * Global state
 * ============================================================ */

static TuiTextInput* g_active_input = NULL;
static int g_console_log_count = 0;

/* Dynamic mock data */
static float g_cpu_usage = 45.0f;
static float g_build_progress = 0.0f;
static int g_spinner_tick = 0;
static float g_stock_prices[4] = {178.35f, 141.80f, 238.45f, 378.90f};
static const char* g_stock_names[4] = {"AAPL", "GOOGL", "TSLA", "MSFT"};
static float g_stock_changes[4] = {1.2f, 0.8f, -2.1f, 0.5f};

/* Theme names for radio group */
static const char* g_theme_names[] = {"Dracula", "Solarized", "High Contrast", "Monochrome"};

/* Dialog visibility flag */
static bool g_dialog_visible = false;
static TuiDialog* g_dialog = NULL;
static struct ncplane* g_dialog_parent = NULL;

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
    ncplane_putstr_yx(win->_plane, dimy - 1, dimx - 2, "\xe2\x8c\x9f");
}

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_slider_change(float value, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Slider] CPU threshold: %.0f%%", value);
}

static void on_radio_theme(int selected, void* userdata) {
    (void)userdata;
    if (selected >= 0 && selected < 4) {
        tui_log(LOG_INFO, "[Theme] Selected: %s", g_theme_names[selected]);
    }
}

static void on_tab_change(int tab_index, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Tabs] Switched to tab %d", tab_index);
}

static void on_dropdown_select(int selected, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Dropdown] Selected index: %d", selected);
}

static void on_dialog_result(int button_index, void* userdata) {
    (void)userdata;
    const char* action = (button_index == 0) ? "Confirmed" : "Cancelled";
    tui_log(LOG_INFO, "[Dialog] %s (button %d)", action, button_index);
    g_dialog_visible = false;
}

static void on_tree_select(int node_index, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Tree] Selected node %d", node_index);
}

static void on_file_pick(const char* path, void* userdata) {
    (void)userdata;
    if (path) {
        tui_log(LOG_INFO, "[FilePicker] Selected: %s", path);
    }
}

static void on_button_refresh(void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Button] Refresh triggered");
}

static void on_button_settings(void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Button] Settings opened");
}

static void on_button_help(void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Button] Help requested");
}

static void on_button_start_agents(void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Button] All agents started");
}

static void on_button_stop_agents(void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Button] All agents stopped");
}

static void on_button_review(void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Button] Code review initiated");
}

static void on_button_show_dialog(void* userdata) {
    (void)userdata;
    g_dialog_visible = true;
    tui_log(LOG_INFO, "[Button] Dialog shown");
}

static void on_checkbox_network(bool checked, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Checkbox] Network: %s", checked ? "ON" : "OFF");
}

static void on_checkbox_bluetooth(bool checked, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Checkbox] Bluetooth: %s", checked ? "ON" : "OFF");
}

static void on_checkbox_autoupdate(bool checked, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Checkbox] Auto-update: %s", checked ? "ON" : "OFF");
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

/* ============================================================
 * Workspace rendering helpers — update dynamic mock data each frame
 * ============================================================ */

static void update_mock_data(void) {
    /* CPU usage: random walk between 5-95% */
    g_cpu_usage += ((rand() % 10) - 5) * 0.5f;
    if (g_cpu_usage < 5.0f) g_cpu_usage = 5.0f;
    if (g_cpu_usage > 95.0f) g_cpu_usage = 95.0f;

    /* Build progress: cycles 0->100% */
    g_build_progress += 0.5f;
    if (g_build_progress > 100.0f) g_build_progress = 0.0f;

    /* Stock prices: small random fluctuations */
    for (int i = 0; i < 4; i++) {
        g_stock_prices[i] += ((rand() % 20) - 10) * 0.05f;
        if (g_stock_prices[i] < 10.0f) g_stock_prices[i] = 10.0f;
        g_stock_changes[i] += ((rand() % 10) - 5) * 0.1f;
    }

    /* Spinner tick counter */
    g_spinner_tick++;
}

/* ============================================================
 * System workspace renderers
 * ============================================================ */

/* Persistent widget pointers for System workspace */
static TuiProgressBar* g_sys_cpu_prog = NULL;
static TuiSlider* g_sys_cpu_slider = NULL;
static TuiList* g_sys_disk_list = NULL;

static void system_cpu_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 2, 2, "CPU: %.1f%%", g_cpu_usage);
    ncplane_printf_yx(p, 3, 2, "Cores: 8  Threads: 16");

    /* Update progress bar */
    if (g_sys_cpu_prog) {
        tui_progress_set_value(g_sys_cpu_prog, g_cpu_usage / 100.0f);
        tui_progress_render(g_sys_cpu_prog);
    }

    /* Update slider */
    if (g_sys_cpu_slider) {
        /* The slider value is user-controlled; don't overwrite it */
    }

    tui_window_mark_dirty(win);
}

static void system_memory_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

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
    render_window_frame(win);

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
 * Console workspace renderers
 * ============================================================ */

static void console_log_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

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
    render_window_frame(win);

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
 * Desktop workspace renderers
 * ============================================================ */

static TuiTextArea* g_desk_notes = NULL;
static TuiButton* g_desk_btn_refresh = NULL;
static TuiButton* g_desk_btn_settings = NULL;
static TuiButton* g_desk_btn_help = NULL;
static TuiCheckbox* g_desk_cb_network = NULL;
static TuiCheckbox* g_desk_cb_bluetooth = NULL;
static TuiCheckbox* g_desk_cb_autoupdate = NULL;
static TuiRadioGroup* g_desk_radio_theme = NULL;

static void desktop_notes_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    if (g_desk_notes) {
        tui_textarea_render(g_desk_notes);
    }

    tui_window_mark_dirty(win);
}

static void desktop_shortcuts_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Quick Actions:");

    if (g_desk_btn_refresh) {
        tui_button_render(g_desk_btn_refresh);
        tui_button_render(g_desk_btn_settings);
        tui_button_render(g_desk_btn_help);
    }

    tui_window_mark_dirty(win);
}

static void desktop_status_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "System Status:");

    if (g_desk_cb_network) {
        tui_checkbox_render(g_desk_cb_network);
        tui_checkbox_render(g_desk_cb_bluetooth);
        tui_checkbox_render(g_desk_cb_autoupdate);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Desktop workspace — theme selector (radio group)
 * ============================================================ */

static void desktop_theme_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);

    if (g_desk_radio_theme) {
        tui_radio_group_render(g_desk_radio_theme);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Finance workspace renderers
 * ============================================================ */

static TuiTable* g_fin_table = NULL;
static TuiProgressBar* g_fin_portfolio_prog = NULL;
static TuiDropdown* g_fin_dropdown = NULL;
static TuiTextInput* g_fin_input = NULL;

static void finance_stocks_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Stock Watchlist:");

    /* Update table rows with live prices */
    if (g_fin_table) {
        char sym[32], price[32], change[32];
        for (int i = 0; i < 4 && i < g_fin_table->row_count; i++) {
            snprintf(sym, sizeof(sym), "%s", g_stock_names[i]);
            g_fin_table->cells[i * g_fin_table->col_count + 0] = strdup(sym);
            snprintf(price, sizeof(price), "$%.2f", g_stock_prices[i]);
            g_fin_table->cells[i * g_fin_table->col_count + 1] = strdup(price);
            snprintf(change, sizeof(change), "%+.1f%%", g_stock_changes[i]);
            g_fin_table->cells[i * g_fin_table->col_count + 2] = strdup(change);
        }
        tui_table_render(g_fin_table);
    }

    tui_window_mark_dirty(win);
}

static void finance_portfolio_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Portfolio Summary:");

    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 3, 2, "Total Value: $125,430");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO);
    ncplane_putstr_yx(p, 4, 2, "Today: +$1,245 (+1.0%)");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN);
    ncplane_printf_yx(p, 5, 2, "Cash: $15,200");

    if (g_fin_portfolio_prog) {
        tui_progress_set_value(g_fin_portfolio_prog, 0.88f);
        tui_progress_render(g_fin_portfolio_prog);
    }

    tui_window_mark_dirty(win);
}

static void finance_watchlist_input_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Add Symbol:");

    if (g_fin_input) {
        tui_text_input_render(g_fin_input);
    }

    /* Dropdown for quick symbol selection */
    if (g_fin_dropdown) {
        tui_dropdown_render(g_fin_dropdown);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Browse workspace renderers
 * ============================================================ */

static TuiTextInput* g_browse_url = NULL;
static TuiTextArea* g_browse_content = NULL;
static TuiTree* g_browse_bookmarks = NULL;

static void browse_url_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "URL:");

    if (g_browse_url) {
        tui_text_input_render(g_browse_url);
    }

    tui_window_mark_dirty(win);
}

static void browse_content_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    if (g_browse_content) {
        tui_textarea_render(g_browse_content);
    }

    tui_window_mark_dirty(win);
}

static void browse_bookmarks_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    if (g_browse_bookmarks) {
        tui_tree_render(g_browse_bookmarks);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Development workspace renderers
 * ============================================================ */

static TuiTree* g_dev_file_tree = NULL;
static TuiTextArea* g_dev_editor = NULL;
static TuiTabContainer* g_dev_tabs = NULL;
static TuiSpinner* g_dev_spinner = NULL;
static TuiProgressBar* g_dev_build_prog = NULL;
static TuiLabel* g_dev_build_label = NULL;

static void dev_files_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "File Browser:");

    if (g_dev_file_tree) {
        tui_tree_render(g_dev_file_tree);
    }

    tui_window_mark_dirty(win);
}

static void dev_editor_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    if (g_dev_editor) {
        tui_textarea_render(g_dev_editor);
    }

    tui_window_mark_dirty(win);
}

static void dev_git_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Git Status:");

    ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO);
    ncplane_printf_yx(p, 3, 2, "On branch main");
    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 4, 2, "Modified: src/core/manager/render.c");
    ncplane_printf_yx(p, 5, 2, "Modified: examples/demo.c");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN);
    ncplane_printf_yx(p, 6, 2, "Untracked: tests/new_test.c");

    tui_window_mark_dirty(win);
}

static void dev_build_progress_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Build Progress:");

    if (g_dev_build_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Compiling... %.0f%%", g_build_progress);
        tui_label_set_text(g_dev_build_label, buf);
        tui_label_render(g_dev_build_label);
    }

    if (g_dev_build_prog) {
        tui_progress_set_value(g_dev_build_prog, g_build_progress / 100.0f);
        tui_progress_render(g_dev_build_prog);
    }

    /* Animate spinner as activity indicator */
    if (g_dev_spinner) {
        if (g_spinner_tick % 3 == 0) {
            tui_spinner_tick(g_dev_spinner);
        }
        tui_spinner_render(g_dev_spinner);
    }

    tui_window_mark_dirty(win);
}

static void dev_build_log_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Build Output:");

    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 3, 2, "gcc -Wall -O2 -c main.c");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO);
    ncplane_printf_yx(p, 4, 2, "[OK] Compiled successfully");
    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 5, 2, "gcc -Wall -O2 -c utils.c");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO);
    ncplane_printf_yx(p, 6, 2, "[OK] Compiled successfully");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN);
    ncplane_printf_yx(p, 7, 2, "Warning: unused variable");

    tui_window_mark_dirty(win);
}

static void dev_tabs_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    if (g_dev_tabs) {
        tui_tab_container_render(g_dev_tabs);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Agents workspace renderers
 * ============================================================ */

static TuiList* g_agents_list = NULL;
static TuiCheckbox* g_agents_cb_review = NULL;
static TuiCheckbox* g_agents_cb_testgen = NULL;
static TuiCheckbox* g_agents_cb_autodoc = NULL;
static TuiButton* g_agents_btn_start = NULL;
static TuiButton* g_agents_btn_stop = NULL;
static TuiButton* g_agents_btn_review = NULL;
static TuiButton* g_agents_btn_dialog = NULL;
static TuiLabel* g_agents_stats = NULL;

static void agents_list_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Active Agents:");

    if (g_agents_list) {
        tui_list_render(g_agents_list);
    }

    tui_window_mark_dirty(win);
}

static void agents_status_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Agent Configuration:");

    if (g_agents_cb_review) {
        tui_checkbox_render(g_agents_cb_review);
        tui_checkbox_render(g_agents_cb_testgen);
        tui_checkbox_render(g_agents_cb_autodoc);
    }

    if (g_agents_stats) {
        tui_label_render(g_agents_stats);
    }

    tui_window_mark_dirty(win);
}

static void agents_controls_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Agent Controls:");

    if (g_agents_btn_start) {
        tui_button_render(g_agents_btn_start);
        tui_button_render(g_agents_btn_stop);
        tui_button_render(g_agents_btn_review);
        tui_button_render(g_agents_btn_dialog);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Log workspace renderers
 * ============================================================ */

static TuiCheckbox* g_log_cb_debug = NULL;
static TuiTextInput* g_log_search = NULL;
static TuiProgressBar* g_log_buffer_prog = NULL;
static TuiLabel* g_log_stats_label = NULL;

static void log_viewer_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    render_window_frame(win);

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
    render_window_frame(win);

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
    render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Log Statistics:");

    TuiLogBuffer* buf = tui_logger_get_buffer();
    int total = buf ? buf->count : 0;

    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 3, 2, "Total entries: %d", total);
    ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN);
    ncplane_printf_yx(p, 4, 2, "Console commands: %d", g_console_log_count);

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
 * Event loop callback: route unhandled keys to active text input
 * ============================================================ */

static void demo_on_unhandled_key(TuiManager* mgr, uint32_t key, const struct ncinput* ni, void* userdata) {
    (void)mgr; (void)ni; (void)userdata;

    /* If dialog is visible, route keys to it */
    if (g_dialog_visible && g_dialog) {
        tui_dialog_handle_key(g_dialog, key, ni);
        return;
    }

    if (g_active_input) {
        tui_text_input_handle_key(g_active_input, key, ni);
    }
}

/* ============================================================
 * on_frame callback: update mock data each frame
 * ============================================================ */

static bool demo_on_frame(TuiManager* mgr, void* userdata) {
    (void)mgr; (void)userdata;
    update_mock_data();

    /* Render dialog overlay if visible */
    if (g_dialog_visible && g_dialog && g_dialog_parent) {
        tui_dialog_render(g_dialog);
        ncplane_move_top(g_dialog->plane);
    }

    return true;
}

/* ============================================================
 * Demo main - Create all 8 workspaces with real widgets
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

    /* CPU window: progress bar + slider + label */
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

    /* Disk window: List widget */
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

    {
        struct ncplane* p = tui_window_get_plane(win_con_input);
        g_active_input = tui_text_input_create(p, 3, 2, 55, on_console_submit, NULL);
        if (g_active_input) {
            g_active_input->focused = true;
            win_con_input->_text_input = g_active_input;
            win_con_input->_on_destroy = console_on_destroy;
            tui_text_input_ensure_registered();
            tui_window_add_widget(win_con_input, g_active_input, g_active_input->plane, tui_text_input_get_type_id());
        }
    }

    /* ============================================================
     * Workspace 3: Desktop
     * ============================================================ */
    TuiWorkspace* ws_desktop = tui_workspace_create(mgr, "Desktop");
    tui_manager_add_workspace(mgr, ws_desktop);

    TuiTab* tab_desktop = tui_tab_create(ws_desktop, "Widgets");
    tui_workspace_add_tab(ws_desktop, tab_desktop);

    /* Notes: TextArea */
    TuiWindow* win_desk_notes = tui_window_create(tab_desktop, 35, 12);
    win_desk_notes->_user_data = "Quick Notes";
    win_desk_notes->_render_cb = desktop_notes_render;
    tui_tab_add_window(tab_desktop, win_desk_notes);
    ncplane_move_yx(win_desk_notes->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_desk_notes);
        g_desk_notes = tui_textarea_create(p, 3, 2, 30, 8);
        if (g_desk_notes) {
            tui_textarea_ensure_registered();
            tui_window_add_widget(win_desk_notes, g_desk_notes, g_desk_notes->plane, tui_textarea_get_type_id());
        }
    }

    /* Shortcuts: Buttons */
    TuiWindow* win_desk_shortcuts = tui_window_create(tab_desktop, 25, 10);
    win_desk_shortcuts->_user_data = "Shortcuts";
    win_desk_shortcuts->_render_cb = desktop_shortcuts_render;
    tui_tab_add_window(tab_desktop, win_desk_shortcuts);
    ncplane_move_yx(win_desk_shortcuts->_plane, 2, 38);

    {
        struct ncplane* p = tui_window_get_plane(win_desk_shortcuts);
        g_desk_btn_refresh = tui_button_create(p, 3, 2, 18, "Refresh", on_button_refresh, NULL);
        if (g_desk_btn_refresh) {
            tui_button_ensure_registered();
            tui_window_add_widget(win_desk_shortcuts, g_desk_btn_refresh, g_desk_btn_refresh->plane, tui_button_get_type_id());
        }

        g_desk_btn_settings = tui_button_create(p, 5, 2, 18, "Settings", on_button_settings, NULL);
        if (g_desk_btn_settings) {
            tui_button_ensure_registered();
            tui_window_add_widget(win_desk_shortcuts, g_desk_btn_settings, g_desk_btn_settings->plane, tui_button_get_type_id());
        }

        g_desk_btn_help = tui_button_create(p, 7, 2, 18, "Help", on_button_help, NULL);
        if (g_desk_btn_help) {
            tui_button_ensure_registered();
            tui_window_add_widget(win_desk_shortcuts, g_desk_btn_help, g_desk_btn_help->plane, tui_button_get_type_id());
        }
    }

    /* System Status: Checkboxes */
    TuiWindow* win_desk_status = tui_window_create(tab_desktop, 25, 10);
    win_desk_status->_user_data = "System Status";
    win_desk_status->_render_cb = desktop_status_render;
    tui_tab_add_window(tab_desktop, win_desk_status);
    ncplane_move_yx(win_desk_status->_plane, 12, 38);

    {
        struct ncplane* p = tui_window_get_plane(win_desk_status);
        g_desk_cb_network = tui_checkbox_create(p, 3, 2, " Network", true, on_checkbox_network, NULL);
        if (g_desk_cb_network) {
            tui_checkbox_ensure_registered();
            tui_window_add_widget(win_desk_status, g_desk_cb_network, g_desk_cb_network->plane, tui_checkbox_get_type_id());
        }

        g_desk_cb_bluetooth = tui_checkbox_create(p, 5, 2, " Bluetooth", false, on_checkbox_bluetooth, NULL);
        if (g_desk_cb_bluetooth) {
            tui_checkbox_ensure_registered();
            tui_window_add_widget(win_desk_status, g_desk_cb_bluetooth, g_desk_cb_bluetooth->plane, tui_checkbox_get_type_id());
        }

        g_desk_cb_autoupdate = tui_checkbox_create(p, 7, 2, " Auto-update", true, on_checkbox_autoupdate, NULL);
        if (g_desk_cb_autoupdate) {
            tui_checkbox_ensure_registered();
            tui_window_add_widget(win_desk_status, g_desk_cb_autoupdate, g_desk_cb_autoupdate->plane, tui_checkbox_get_type_id());
        }
    }

    /* Theme Selector: Radio Group */
    TuiWindow* win_desk_theme = tui_window_create(tab_desktop, 25, 10);
    win_desk_theme->_user_data = "Theme";
    win_desk_theme->_render_cb = desktop_theme_render;
    tui_tab_add_window(tab_desktop, win_desk_theme);
    ncplane_move_yx(win_desk_theme->_plane, 12, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_desk_theme);
        g_desk_radio_theme = tui_radio_group_create(p, 3, 2, 20, 5, on_radio_theme, NULL);
        if (g_desk_radio_theme) {
            tui_radio_group_add_option(g_desk_radio_theme, "Dracula");
            tui_radio_group_add_option(g_desk_radio_theme, "Solarized");
            tui_radio_group_add_option(g_desk_radio_theme, "High Contrast");
            tui_radio_group_add_option(g_desk_radio_theme, "Monochrome");
            tui_radio_group_set_selected(g_desk_radio_theme, 0);
            tui_radio_group_ensure_registered();
            tui_window_add_widget(win_desk_theme, g_desk_radio_theme, g_desk_radio_theme->plane, tui_radio_group_get_type_id());
        }
    }

    /* ============================================================
     * Workspace 4: Finance
     * ============================================================ */
    TuiWorkspace* ws_finance = tui_workspace_create(mgr, "Finance");
    tui_manager_add_workspace(mgr, ws_finance);

    TuiTab* tab_finance = tui_tab_create(ws_finance, "Markets");
    tui_workspace_add_tab(ws_finance, tab_finance);

    /* Stocks Table */
    TuiWindow* win_fin_stocks = tui_window_create(tab_finance, 40, 10);
    win_fin_stocks->_user_data = "Stock Watchlist";
    win_fin_stocks->_render_cb = finance_stocks_render;
    tui_tab_add_window(tab_finance, win_fin_stocks);
    ncplane_move_yx(win_fin_stocks->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_fin_stocks);
        g_fin_table = tui_table_create(p, 3, 2, 35, 6, 3);
        if (g_fin_table) {
            tui_table_set_header(g_fin_table, 0, "Symbol");
            tui_table_set_header(g_fin_table, 1, "Price");
            tui_table_set_header(g_fin_table, 2, "Change");

            for (int i = 0; i < 4; i++) {
                const char* row[3];
                char price[32], change[32];
                row[0] = g_stock_names[i];
                snprintf(price, sizeof(price), "$%.2f", g_stock_prices[i]);
                row[1] = price;
                snprintf(change, sizeof(change), "%+.1f%%", g_stock_changes[i]);
                row[2] = change;
                tui_table_add_row(g_fin_table, row);
            }
            tui_table_set_selected(g_fin_table, 0);
            tui_table_ensure_registered();
            tui_window_add_widget(win_fin_stocks, g_fin_table, g_fin_table->plane, tui_table_get_type_id());
        }
    }

    /* Portfolio */
    TuiWindow* win_fin_portfolio = tui_window_create(tab_finance, 30, 10);
    win_fin_portfolio->_user_data = "Portfolio";
    win_fin_portfolio->_render_cb = finance_portfolio_render;
    tui_tab_add_window(tab_finance, win_fin_portfolio);
    ncplane_move_yx(win_fin_portfolio->_plane, 2, 43);

    {
        struct ncplane* p = tui_window_get_plane(win_fin_portfolio);
        g_fin_portfolio_prog = tui_progress_create(p, 7, 2, 25);
        if (g_fin_portfolio_prog) {
            tui_progress_set_value(g_fin_portfolio_prog, 0.88f);
            tui_progress_ensure_registered();
            tui_window_add_widget(win_fin_portfolio, g_fin_portfolio_prog, g_fin_portfolio_prog->plane, tui_progress_get_type_id());
        }
    }

    /* Watchlist Input + Dropdown */
    TuiWindow* win_fin_input = tui_window_create(tab_finance, 25, 8);
    win_fin_input->_user_data = "Add Symbol";
    win_fin_input->_render_cb = finance_watchlist_input_render;
    tui_tab_add_window(tab_finance, win_fin_input);
    ncplane_move_yx(win_fin_input->_plane, 12, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_fin_input);
        g_fin_input = tui_text_input_create(p, 3, 2, 20, NULL, NULL);
        if (g_fin_input) {
            tui_text_input_ensure_registered();
            tui_window_add_widget(win_fin_input, g_fin_input, g_fin_input->plane, tui_text_input_get_type_id());
        }

        g_fin_dropdown = tui_dropdown_create(p, 5, 2, 20, on_dropdown_select, NULL);
        if (g_fin_dropdown) {
            tui_dropdown_add_item(g_fin_dropdown, "AAPL");
            tui_dropdown_add_item(g_fin_dropdown, "GOOGL");
            tui_dropdown_add_item(g_fin_dropdown, "TSLA");
            tui_dropdown_add_item(g_fin_dropdown, "MSFT");
            tui_dropdown_add_item(g_fin_dropdown, "AMZN");
            tui_dropdown_set_selected(g_fin_dropdown, 0);
            tui_dropdown_ensure_registered();
            tui_window_add_widget(win_fin_input, g_fin_dropdown, g_fin_dropdown->plane, tui_dropdown_get_type_id());
        }
    }

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

    {
        struct ncplane* p = tui_window_get_plane(win_browse_url);
        g_browse_url = tui_text_input_create(p, 2, 6, 40, NULL, NULL);
        if (g_browse_url) {
            tui_text_input_ensure_registered();
            tui_window_add_widget(win_browse_url, g_browse_url, g_browse_url->plane, tui_text_input_get_type_id());
        }
    }

    TuiWindow* win_browse_content = tui_window_create(tab_browse, 50, 12);
    win_browse_content->_user_data = "Page Content";
    win_browse_content->_render_cb = browse_content_render;
    tui_tab_add_window(tab_browse, win_browse_content);
    ncplane_move_yx(win_browse_content->_plane, 6, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_browse_content);
        g_browse_content = tui_textarea_create(p, 2, 2, 45, 9);
        if (g_browse_content) {
            tui_textarea_ensure_registered();
            tui_window_add_widget(win_browse_content, g_browse_content, g_browse_content->plane, tui_textarea_get_type_id());
        }
    }

    /* Bookmarks: Tree widget */
    TuiWindow* win_browse_bookmarks = tui_window_create(tab_browse, 25, 12);
    win_browse_bookmarks->_user_data = "Bookmarks";
    win_browse_bookmarks->_render_cb = browse_bookmarks_render;
    tui_tab_add_window(tab_browse, win_browse_bookmarks);
    ncplane_move_yx(win_browse_bookmarks->_plane, 6, 53);

    {
        struct ncplane* p = tui_window_get_plane(win_browse_bookmarks);
        g_browse_bookmarks = tui_tree_create(p, 2, 2, 20, 9, on_tree_select, NULL);
        if (g_browse_bookmarks) {
            int github = tui_tree_add_node(g_browse_bookmarks, "github.com", false);
            tui_tree_add_child(g_browse_bookmarks, github, "/tmlcs/tmlcs-tui", true);
            tui_tree_add_child(g_browse_bookmarks, github, "/torvalds/linux", true);
            (void)tui_tree_add_node(g_browse_bookmarks, "stackoverflow.com", true);
            (void)tui_tree_add_node(g_browse_bookmarks, "news.ycombinator.com", true);
            int reddit = tui_tree_add_node(g_browse_bookmarks, "reddit.com", false);
            tui_tree_add_child(g_browse_bookmarks, reddit, "/r/programming", true);
            tui_tree_add_child(g_browse_bookmarks, reddit, "/r/linux", true);
            tui_tree_set_selected(g_browse_bookmarks, 0);
            tui_tree_ensure_registered();
            tui_window_add_widget(win_browse_bookmarks, g_browse_bookmarks, g_browse_bookmarks->plane, tui_tree_get_type_id());
        }
    }

    /* ============================================================
     * Workspace 6: Development
     * ============================================================ */
    TuiWorkspace* ws_dev = tui_workspace_create(mgr, "Development");
    tui_manager_add_workspace(mgr, ws_dev);

    TuiTab* tab_dev_editor = tui_tab_create(ws_dev, "Editor");
    tui_workspace_add_tab(ws_dev, tab_dev_editor);

    /* File browser: Tree */
    TuiWindow* win_dev_files = tui_window_create(tab_dev_editor, 25, 18);
    win_dev_files->_user_data = "Files";
    win_dev_files->_render_cb = dev_files_render;
    tui_tab_add_window(tab_dev_editor, win_dev_files);
    ncplane_move_yx(win_dev_files->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_dev_files);
        g_dev_file_tree = tui_tree_create(p, 3, 2, 20, 14, on_tree_select, NULL);
        if (g_dev_file_tree) {
            int src = tui_tree_add_node(g_dev_file_tree, "src/", false);
            int core = tui_tree_add_child(g_dev_file_tree, src, "core/", false);
            tui_tree_add_child(g_dev_file_tree, core, "manager.c", true);
            tui_tree_add_child(g_dev_file_tree, core, "workspace.c", true);
            tui_tree_add_child(g_dev_file_tree, core, "tab.c", true);
            tui_tree_add_child(g_dev_file_tree, core, "window.c", true);
            int widget_d = tui_tree_add_child(g_dev_file_tree, src, "widget/", false);
            tui_tree_add_child(g_dev_file_tree, widget_d, "button.c", true);
            tui_tree_add_child(g_dev_file_tree, widget_d, "slider.c", true);
            tui_tree_add_child(g_dev_file_tree, widget_d, "table.c", true);
            int include = tui_tree_add_node(g_dev_file_tree, "include/", false);
            tui_tree_add_child(g_dev_file_tree, include, "tmlcs_tui.h", true);
            tui_tree_add_node(g_dev_file_tree, "CMakeLists.txt", true);
            tui_tree_add_node(g_dev_file_tree, "README.md", true);
            tui_tree_add_node(g_dev_file_tree, "Makefile", true);
            tui_tree_toggle(g_dev_file_tree, src);
            tui_tree_set_selected(g_dev_file_tree, 0);
            tui_tree_ensure_registered();
            tui_window_add_widget(win_dev_files, g_dev_file_tree, g_dev_file_tree->plane, tui_tree_get_type_id());
        }
    }

    /* Code editor: TextArea */
    TuiWindow* win_dev_code = tui_window_create(tab_dev_editor, 45, 12);
    win_dev_code->_user_data = "main.c";
    win_dev_code->_render_cb = dev_editor_render;
    tui_tab_add_window(tab_dev_editor, win_dev_code);
    ncplane_move_yx(win_dev_code->_plane, 2, 28);

    {
        struct ncplane* p = tui_window_get_plane(win_dev_code);
        g_dev_editor = tui_textarea_create(p, 2, 2, 40, 9);
        if (g_dev_editor) {
            tui_textarea_ensure_registered();
            tui_window_add_widget(win_dev_code, g_dev_editor, g_dev_editor->plane, tui_textarea_get_type_id());
        }
    }

    /* Git status */
    TuiWindow* win_dev_git = tui_window_create(tab_dev_editor, 45, 8);
    win_dev_git->_user_data = "Git Status";
    win_dev_git->_render_cb = dev_git_render;
    tui_tab_add_window(tab_dev_editor, win_dev_git);
    ncplane_move_yx(win_dev_git->_plane, 14, 28);

    /* Build tab */
    TuiTab* tab_dev_build = tui_tab_create(ws_dev, "Build");
    tui_workspace_add_tab(ws_dev, tab_dev_build);

    TuiWindow* win_dev_build_prog = tui_window_create(tab_dev_build, 45, 10);
    win_dev_build_prog->_user_data = "Build Progress";
    win_dev_build_prog->_render_cb = dev_build_progress_render;
    tui_tab_add_window(tab_dev_build, win_dev_build_prog);
    ncplane_move_yx(win_dev_build_prog->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_dev_build_prog);
        g_dev_build_label = tui_label_create(p, 3, 2, "Compiling... 0%");
        if (g_dev_build_label) {
            tui_label_ensure_registered();
            tui_window_add_widget(win_dev_build_prog, g_dev_build_label, g_dev_build_label->plane, tui_label_get_type_id());
        }

        g_dev_build_prog = tui_progress_create(p, 5, 2, 38);
        if (g_dev_build_prog) {
            tui_progress_ensure_registered();
            tui_window_add_widget(win_dev_build_prog, g_dev_build_prog, g_dev_build_prog->plane, tui_progress_get_type_id());
        }

        g_dev_spinner = tui_spinner_create(p, 7, 2, SPINNER_DOTS);
        if (g_dev_spinner) {
            tui_spinner_start(g_dev_spinner);
            tui_spinner_ensure_registered();
            tui_window_add_widget(win_dev_build_prog, g_dev_spinner, g_dev_spinner->plane, tui_spinner_get_type_id());
        }

        TuiLabel* spin_label = tui_label_create(p, 7, 5, " Building...");
        if (spin_label) {
            tui_label_ensure_registered();
            tui_window_add_widget(win_dev_build_prog, spin_label, spin_label->plane, tui_label_get_type_id());
        }
    }

    TuiWindow* win_dev_build_log = tui_window_create(tab_dev_build, 40, 10);
    win_dev_build_log->_user_data = "Build Log";
    win_dev_build_log->_render_cb = dev_build_log_render;
    tui_tab_add_window(tab_dev_build, win_dev_build_log);
    ncplane_move_yx(win_dev_build_log->_plane, 2, 48);

    /* Dev TabContainer workspace */
    TuiWindow* win_dev_tabs = tui_window_create(tab_dev_build, 40, 8);
    win_dev_tabs->_user_data = "Tabs Demo";
    win_dev_tabs->_render_cb = dev_tabs_render;
    tui_tab_add_window(tab_dev_build, win_dev_tabs);
    ncplane_move_yx(win_dev_tabs->_plane, 12, 48);

    {
        struct ncplane* p = tui_window_get_plane(win_dev_tabs);
        g_dev_tabs = tui_tab_container_create(p, 2, 2, 35, 5, on_tab_change, NULL);
        if (g_dev_tabs) {
            tui_tab_container_add_tab(g_dev_tabs, "Editor");
            tui_tab_container_add_tab(g_dev_tabs, "Terminal");
            tui_tab_container_add_tab(g_dev_tabs, "Output");
            tui_tab_container_add_tab(g_dev_tabs, "Problems");
            tui_tab_container_set_active(g_dev_tabs, 0);
            tui_tab_container_ensure_registered();
            tui_window_add_widget(win_dev_tabs, g_dev_tabs, g_dev_tabs->plane, tui_tab_container_get_type_id());
        }
    }

    /* ============================================================
     * Workspace 7: Agents
     * ============================================================ */
    TuiWorkspace* ws_agents = tui_workspace_create(mgr, "Agents");
    tui_manager_add_workspace(mgr, ws_agents);

    TuiTab* tab_agents = tui_tab_create(ws_agents, "Active Agents");
    tui_workspace_add_tab(ws_agents, tab_agents);

    TuiWindow* win_agents_list = tui_window_create(tab_agents, 30, 12);
    win_agents_list->_user_data = "Agent List";
    win_agents_list->_render_cb = agents_list_render;
    tui_tab_add_window(tab_agents, win_agents_list);
    ncplane_move_yx(win_agents_list->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_agents_list);
        g_agents_list = tui_list_create(p, 3, 2, 25, 8);
        if (g_agents_list) {
            tui_list_add_item(g_agents_list, "Code Review Agent");
            tui_list_add_item(g_agents_list, "Test Generator Agent");
            tui_list_add_item(g_agents_list, "Doc Writer Agent");
            tui_list_add_item(g_agents_list, "Security Scanner Agent");
            tui_list_add_item(g_agents_list, "Performance Analyzer");
            tui_list_set_selected(g_agents_list, 0);
            tui_list_ensure_registered();
            tui_window_add_widget(win_agents_list, g_agents_list, g_agents_list->plane, tui_list_get_type_id());
        }
    }

    TuiWindow* win_agents_status = tui_window_create(tab_agents, 30, 12);
    win_agents_status->_user_data = "Agent Status";
    win_agents_status->_render_cb = agents_status_render;
    tui_tab_add_window(tab_agents, win_agents_status);
    ncplane_move_yx(win_agents_status->_plane, 2, 33);

    {
        struct ncplane* p = tui_window_get_plane(win_agents_status);
        g_agents_cb_review = tui_checkbox_create(p, 3, 2, " Auto-review", true, NULL, NULL);
        if (g_agents_cb_review) {
            tui_checkbox_ensure_registered();
            tui_window_add_widget(win_agents_status, g_agents_cb_review, g_agents_cb_review->plane, tui_checkbox_get_type_id());
        }

        g_agents_cb_testgen = tui_checkbox_create(p, 5, 2, " Auto-test gen", true, NULL, NULL);
        if (g_agents_cb_testgen) {
            tui_checkbox_ensure_registered();
            tui_window_add_widget(win_agents_status, g_agents_cb_testgen, g_agents_cb_testgen->plane, tui_checkbox_get_type_id());
        }

        g_agents_cb_autodoc = tui_checkbox_create(p, 7, 2, " Auto-document", false, NULL, NULL);
        if (g_agents_cb_autodoc) {
            tui_checkbox_ensure_registered();
            tui_window_add_widget(win_agents_status, g_agents_cb_autodoc, g_agents_cb_autodoc->plane, tui_checkbox_get_type_id());
        }

        g_agents_stats = tui_label_create(p, 9, 2, "Tasks completed: 142 | Pending: 3");
        if (g_agents_stats) {
            tui_label_ensure_registered();
            tui_window_add_widget(win_agents_status, g_agents_stats, g_agents_stats->plane, tui_label_get_type_id());
        }
    }

    TuiWindow* win_agents_controls = tui_window_create(tab_agents, 25, 12);
    win_agents_controls->_user_data = "Controls";
    win_agents_controls->_render_cb = agents_controls_render;
    tui_tab_add_window(tab_agents, win_agents_controls);
    ncplane_move_yx(win_agents_controls->_plane, 12, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_agents_controls);
        g_agents_btn_start = tui_button_create(p, 3, 2, 20, "Start All Agents", on_button_start_agents, NULL);
        if (g_agents_btn_start) {
            tui_button_ensure_registered();
            tui_window_add_widget(win_agents_controls, g_agents_btn_start, g_agents_btn_start->plane, tui_button_get_type_id());
        }

        g_agents_btn_stop = tui_button_create(p, 5, 2, 20, "Stop All Agents", on_button_stop_agents, NULL);
        if (g_agents_btn_stop) {
            tui_button_ensure_registered();
            tui_window_add_widget(win_agents_controls, g_agents_btn_stop, g_agents_btn_stop->plane, tui_button_get_type_id());
        }

        g_agents_btn_review = tui_button_create(p, 7, 2, 20, "Run Review Now", on_button_review, NULL);
        if (g_agents_btn_review) {
            tui_button_ensure_registered();
            tui_window_add_widget(win_agents_controls, g_agents_btn_review, g_agents_btn_review->plane, tui_button_get_type_id());
        }

        g_agents_btn_dialog = tui_button_create(p, 9, 2, 20, "Show Dialog", on_button_show_dialog, NULL);
        if (g_agents_btn_dialog) {
            tui_button_ensure_registered();
            tui_window_add_widget(win_agents_controls, g_agents_btn_dialog, g_agents_btn_dialog->plane, tui_button_get_type_id());
        }
    }

    /* Create the dialog (hidden until button pressed) */
    {
        struct ncplane* std = notcurses_stdplane(nc);
        g_dialog_parent = std;
        unsigned sy, sx;
        ncplane_dim_yx(std, &sy, &sx);
        int dlg_w = 40;
        int dlg_h = 8;
        int dlg_y = (int)sy / 2 - dlg_h / 2;
        int dlg_x = (int)sx / 2 - dlg_w / 2;
        if (dlg_y < 0) dlg_y = 2;
        if (dlg_x < 0) dlg_x = 2;

        g_dialog = tui_dialog_create(std, dlg_y, dlg_x, dlg_w, dlg_h,
                                      "Confirm Action",
                                      "Are you sure you want to proceed?\nThis action cannot be undone.",
                                      on_dialog_result, NULL);
        if (g_dialog) {
            tui_dialog_add_button(g_dialog, "Confirm");
            tui_dialog_add_button(g_dialog, "Cancel");
            tui_dialog_ensure_registered();
        }
        g_dialog_visible = false;
    }

    /* ============================================================
     * Workspace 8: Log
     * ============================================================ */
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

    /* ============================================================
     * Workspace 9: Browse (File Picker)
     * ============================================================ */
    TuiWorkspace* ws_filepicker = tui_workspace_create(mgr, "Files");
    tui_manager_add_workspace(mgr, ws_filepicker);

    TuiTab* tab_filepicker = tui_tab_create(ws_filepicker, "File Picker");
    tui_workspace_add_tab(ws_filepicker, tab_filepicker);

    TuiWindow* win_fp = tui_window_create(tab_filepicker, 40, 18);
    win_fp->_user_data = "File Browser";
    win_fp->_render_cb = browse_bookmarks_render; /* reuse bookmarks render pattern */
    tui_tab_add_window(tab_filepicker, win_fp);
    ncplane_move_yx(win_fp->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_fp);
        /* Create a file picker starting at current directory */
        TuiFilePicker* fp = tui_file_picker_create(p, 2, 2, 35, 14, ".", on_file_pick, NULL);
        if (fp) {
            tui_file_picker_ensure_registered();
            tui_window_add_widget(win_fp, fp, fp->plane, tui_file_picker_get_type_id());
        }
    }

    /* ============================================================
     * Activate first workspace
     * ============================================================ */
    tui_manager_set_active_workspace(mgr, 0);

    tui_log(LOG_INFO, "TMLCS-TUI Demo started with 9 workspaces.");
    tui_log(LOG_INFO, "Use Alt+1..9 to switch workspaces.");

    /* ============================================================
     * Event Loop
     * ============================================================ */
    TuiLoopConfig loop_config = {
        .on_frame = demo_on_frame,
        .on_unhandled_key = demo_on_unhandled_key,
        .target_fps = 30,
        .userdata = NULL,
    };
    tui_manager_run(mgr, &loop_config);

    /* Cleanup persistent widgets */
    if (g_dialog) tui_dialog_destroy(g_dialog);

    tui_manager_destroy(mgr);
    return 0;
}
