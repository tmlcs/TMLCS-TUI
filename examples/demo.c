/**
 * @file demo.c
 * @brief TMLCS-TUI Showcase Demo -- Entry point.
 *
 * Demonstrates all 17 widget types across 9 workspaces.
 * Navigation: Alt+1..9 to switch workspaces.
 */

#include "common.h"
#include "core/loop.h"
#include "core/manager.h"
#include "core/logger.h"
#include "core/types_private.h"
#include "widget/text_input.h"
#include "widget/dialog.h"
#include <notcurses/notcurses.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/* ============================================================
 * Event loop callbacks
 * ============================================================ */

static void demo_on_unhandled_key(TuiManager* mgr, uint32_t key, const struct ncinput* ni, void* userdata) {
    (void)mgr; (void)ni; (void)userdata;

    /* If dialog is visible, route keys to it */
    if (agents_get_dialog_visible() && agents_get_dialog()) {
        tui_dialog_handle_key(agents_get_dialog(), key, ni);
        return;
    }

    if (g_demo.active_input) {
        tui_text_input_handle_key(g_demo.active_input, key, ni);
    }
}

static bool demo_on_frame(TuiManager* mgr, void* userdata) {
    (void)mgr; (void)userdata;
    demo_update_mock_data();

    /* Render dialog overlay if visible */
    if (agents_get_dialog_visible() && agents_get_dialog() && agents_get_dialog_plane()) {
        tui_dialog_render(agents_get_dialog());
        ncplane_move_top(agents_get_dialog_plane());
    }

    return true;
}

/* ============================================================
 * Demo main
 * ============================================================ */

int demo_main(struct notcurses* nc) {
    srand((unsigned int)time(NULL));

    TuiManager* mgr = tui_manager_create(nc);

    g_demo = (DemoState){0};
    g_demo.cpu_usage = 45.0f;
    g_demo.build_progress = 0.0f;
    g_demo.stock_prices[0] = 178.35f;
    g_demo.stock_prices[1] = 141.80f;
    g_demo.stock_prices[2] = 238.45f;
    g_demo.stock_prices[3] = 378.90f;
    g_demo.stock_prices[4] = 150.00f;

    /* Create all 9 workspaces */
    tui_manager_add_workspace(mgr, create_system_workspace(mgr));
    tui_manager_add_workspace(mgr, create_console_workspace(mgr));
    tui_manager_add_workspace(mgr, create_desktop_workspace(mgr));
    tui_manager_add_workspace(mgr, create_finance_workspace(mgr));
    tui_manager_add_workspace(mgr, create_browse_workspace(mgr));
    tui_manager_add_workspace(mgr, create_development_workspace(mgr));
    tui_manager_add_workspace(mgr, create_agents_workspace(mgr));
    tui_manager_add_workspace(mgr, create_log_workspace(mgr));
    tui_manager_add_workspace(mgr, create_files_workspace(mgr));

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

    /* Cleanup persistent dialog */
    if (agents_get_dialog()) tui_dialog_destroy(agents_get_dialog());

    tui_manager_destroy(mgr);
    return 0;
}
