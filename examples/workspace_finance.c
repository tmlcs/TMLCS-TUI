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
#include "widget/dropdown.h"
#include "widget/progress.h"
#include "widget/table.h"
#include <stdio.h>

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_dropdown_select(int selected, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Dropdown] Selected index: %d", selected);
}

/* ============================================================
 * Static data
 * ============================================================ */

static const char* g_stock_names[4] = {"AAPL", "GOOGL", "TSLA", "MSFT"};
static float g_stock_changes[4] = {1.2f, 0.8f, -2.1f, 0.5f};

/* ============================================================
 * Persistent widget pointers
 * ============================================================ */

static TuiTable* g_fin_table = NULL;
static TuiProgressBar* g_fin_portfolio_prog = NULL;
static TuiDropdown* g_fin_dropdown = NULL;
static TuiTextInput* g_fin_input = NULL;

/* ============================================================
 * Finance workspace renderers
 * ============================================================ */

static void finance_stocks_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Stock Watchlist:");

    if (g_fin_table) {
        char sym[32], price[32], change[32];
        for (int i = 0; i < 4 && i < g_fin_table->row_count; i++) {
            snprintf(sym, sizeof(sym), "%s", g_stock_names[i]);
            g_fin_table->cells[i * g_fin_table->col_count + 0] = strdup(sym);
            snprintf(price, sizeof(price), "$%.2f", g_demo.stock_prices[i]);
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
    demo_render_window_frame(win);

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
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Add Symbol:");

    if (g_fin_input) {
        tui_text_input_render(g_fin_input);
    }

    if (g_fin_dropdown) {
        tui_dropdown_render(g_fin_dropdown);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Factory function
 * ============================================================ */

TuiWorkspace* create_finance_workspace(TuiManager* mgr) {
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
                snprintf(price, sizeof(price), "$%.2f", g_demo.stock_prices[i]);
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

    return ws_finance;
}
