/**
 * @file demo_finance.c
 * @brief Finance workspace: stock watchlist table, symbol input,
 *         portfolio slider and allocation progress.
 */

#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/logger.h"
#include "widget/table.h"
#include "widget/slider.h"
#include "widget/progress.h"
#include "widget/dropdown.h"
#include "widget/text_input.h"
#include <stdio.h>

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiTable* s_stock_table;
static TuiSlider* s_price_filter;
static TuiProgressBar* s_alloc_bar;
static TuiDropdown* s_portfolio_dropdown;
static TuiTextInput* s_symbol_input;

/* ============================================================
 * Callbacks
 * ============================================================ */


static void on_symbol_submit(const char* text, void* ud) {
    (void)ud;
    if (text && text[0]) tui_log(LOG_INFO, "Added symbol to watchlist: %s", text);
}

/* ============================================================
 * Renderers
 * ============================================================ */

static void finance_table_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (s_stock_table) {
        ncplane_set_bg_rgb(win->_plane, THEME_BG_WINDOW);
        ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
        ncplane_putstr_yx(win->_plane, 0, 2, "Stock Watchlist");
        tui_table_render(s_stock_table);
    }
}

static void finance_portfolio_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
    ncplane_printf_yx(win->_plane, 2, 2, "Total: $125,430");
    ncplane_set_fg_rgb(win->_plane, THEME_FG_LOG_INFO);
    ncplane_printf_yx(win->_plane, 3, 2, "Today: +$1,245 (+1.0%%)");

    if (s_alloc_bar) {
        tui_progress_set_value(s_alloc_bar, 0.88f);
        tui_progress_render(s_alloc_bar);
        ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
        ncplane_putstr_yx(win->_plane, 5, 2, "Allocation:");
    }
    if (s_portfolio_dropdown) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_putstr_yx(win->_plane, 7, 2, "Portfolio:");
        tui_dropdown_render(s_portfolio_dropdown);
    }
}

static void finance_filter_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (s_price_filter) {
        float min_price = tui_slider_get_value(s_price_filter);
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 2, 2, "Min Price: $%.0f", min_price);
        tui_slider_render(s_price_filter);
    }
    if (s_symbol_input) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
        ncplane_putstr_yx(win->_plane, 4, 2, "Add Symbol:");
        ncplane_move_top(s_symbol_input->plane);
        tui_text_input_render(s_symbol_input);
    }
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_finance_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "Finance");
    if (!ws) return NULL;

    TuiTab* tab = tui_tab_create(ws, "Markets");
    tui_workspace_add_tab(ws, tab);

    /* Stock table */
    TuiWindow* win_table = tui_window_create(tab, 40, 12);
    win_table->_user_data = "Watchlist";
    win_table->_render_cb = finance_table_render;
    tui_tab_add_window(tab, win_table);
    ncplane_move_yx(win_table->_plane, 2, 2);

    s_stock_table = tui_table_create(win_table->_plane, 2, 2, 35, 9, 3);
    if (s_stock_table) {
        tui_table_set_header(s_stock_table, 0, "Symbol");
        tui_table_set_header(s_stock_table, 1, "Price");
        tui_table_set_header(s_stock_table, 2, "Change");
        const char* r1[] = {"AAPL", "$178.35", "+1.2%"};
        const char* r2[] = {"GOOGL", "$141.80", "+0.8%"};
        const char* r3[] = {"TSLA", "$238.45", "-2.1%"};
        const char* r4[] = {"MSFT", "$378.90", "+0.5%"};
        const char* r5[] = {"AMZN", "$150.00", "+1.5%"};
        tui_table_add_row(s_stock_table, r1);
        tui_table_add_row(s_stock_table, r2);
        tui_table_add_row(s_stock_table, r3);
        tui_table_add_row(s_stock_table, r4);
        tui_table_add_row(s_stock_table, r5);
        tui_window_add_widget(win_table, s_stock_table, s_stock_table->plane, tui_table_get_type_id());
    }

    /* Portfolio */
    TuiWindow* win_portfolio = tui_window_create(tab, 30, 10);
    win_portfolio->_user_data = "Portfolio";
    win_portfolio->_render_cb = finance_portfolio_render;
    tui_tab_add_window(tab, win_portfolio);
    ncplane_move_yx(win_portfolio->_plane, 2, 43);

    s_alloc_bar = tui_progress_create(win_portfolio->_plane, 6, 2, 25);
    if (s_alloc_bar) tui_window_add_widget(win_portfolio, s_alloc_bar, s_alloc_bar->plane, tui_progress_get_type_id());
    s_portfolio_dropdown = tui_dropdown_create(win_portfolio->_plane, 7, 11, 16, NULL, NULL);
    if (s_portfolio_dropdown) {
        tui_dropdown_add_item(s_portfolio_dropdown, "Growth");
        tui_dropdown_add_item(s_portfolio_dropdown, "Value");
        tui_dropdown_add_item(s_portfolio_dropdown, "Index");
        tui_window_add_widget(win_portfolio, s_portfolio_dropdown, s_portfolio_dropdown->plane, tui_dropdown_get_type_id());
    }

    /* Filter */
    TuiWindow* win_filter = tui_window_create(tab, 30, 6);
    win_filter->_user_data = "Filters";
    win_filter->_render_cb = finance_filter_render;
    tui_tab_add_window(tab, win_filter);
    ncplane_move_yx(win_filter->_plane, 14, 2);

    s_price_filter = tui_slider_create(win_filter->_plane, 2, 2, 24, 0.0f, 200.0f, 50.0f, NULL, NULL);
    if (s_price_filter) tui_window_add_widget(win_filter, s_price_filter, s_price_filter->plane, tui_slider_get_type_id());
    s_symbol_input = tui_text_input_create(win_filter->_plane, 4, 10, 16, on_symbol_submit, NULL);
    if (s_symbol_input) tui_window_add_widget(win_filter, s_symbol_input, s_symbol_input->plane, tui_text_input_get_type_id());

    return ws;
}
