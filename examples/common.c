#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/theme.h"
#include "core/logger.h"
#include "core/types_private.h"
#include "widget/text_input.h"
#include <stdio.h>
#include <stdlib.h>

/* ============================================================
 * Global demo state
 * ============================================================ */

DemoState g_demo;

/* ============================================================
 * Window frame helper — renders border, title, and close button
 * ============================================================ */

void demo_render_window_frame(TuiWindow* win) {
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
 * Log buffer renderer (shared by console and log viewer)
 * ============================================================ */

void demo_render_log_buffer(TuiWindow* win) {
    if (!win || !win->_plane) return;

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

/* ============================================================
 * Mock data updater — called every frame
 * ============================================================ */

void demo_update_mock_data(void) {
    /* CPU usage: random walk between 5-95% */
    g_demo.cpu_usage += ((rand() % 10) - 5) * 0.5f;
    if (g_demo.cpu_usage < 5.0f) g_demo.cpu_usage = 5.0f;
    if (g_demo.cpu_usage > 95.0f) g_demo.cpu_usage = 95.0f;

    /* Build progress: cycles 0->100% */
    g_demo.build_progress += 0.5f;
    if (g_demo.build_progress > 100.0f) g_demo.build_progress = 0.0f;

    /* Stock prices: small random fluctuations */
    for (int i = 0; i < 5; i++) {
        g_demo.stock_prices[i] += ((rand() % 20) - 10) * 0.05f;
        if (g_demo.stock_prices[i] < 10.0f) g_demo.stock_prices[i] = 10.0f;
    }

    g_demo.build_frame++;
}
