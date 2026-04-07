/**
 * @file demo_agents.c
 * @brief Agents workspace: agent table, config checkboxes, control buttons,
 *         activity spinner, and confirmation dialog.
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
#include "widget/checkbox.h"
#include "widget/button.h"
#include "widget/spinner.h"
#include "widget/dialog.h"
#include "widget/label.h"
#include <stdio.h>

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiTable* s_agents_table;
static TuiCheckbox* s_review_cb;
static TuiCheckbox* s_test_cb;
static TuiCheckbox* s_doc_cb;
static TuiSpinner* s_activity_spinner;
static TuiDialog* s_dialog;
static struct ncplane* s_dialog_parent;
static bool s_dialog_visible = false;

/* ============================================================
 * Dialog accessors (used by demo.c for overlay rendering)
 * ============================================================ */

bool agents_get_dialog_visible(void) { return s_dialog_visible; }
TuiDialog* agents_get_dialog(void) { return s_dialog; }
void agents_set_dialog_visible(bool vis) { s_dialog_visible = vis; }
struct ncplane* agents_get_dialog_plane(void) { return s_dialog ? s_dialog->plane : NULL; }
struct ncplane* agents_get_dialog_parent(void) { return s_dialog_parent; }
void agents_set_dialog_parent(struct ncplane* p) { s_dialog_parent = p; }

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_btn_start(void* ud) {
    (void)ud;
    g_demo.agents_running[0] = true;
    tui_log(LOG_INFO, "All agents started");
}
static void on_btn_stop(void* ud) {
    (void)ud;
    g_demo.agents_running[0] = false;
    tui_log(LOG_INFO, "All agents stopped");
}
static void on_btn_review(void* ud) {
    (void)ud;
    tui_log(LOG_INFO, "Running code review now");
}
static void on_btn_dialog(void* ud) {
    (void)ud;
    s_dialog_visible = true;
    tui_log(LOG_INFO, "Showing dialog");
}
static void on_dialog_close(int result, void* ud) {
    (void)ud;
    s_dialog_visible = false;
    tui_log(LOG_INFO, "Dialog closed with result: %d", result);
}

/* ============================================================
 * Renderers
 * ============================================================ */

static void agents_table_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 0, 2, "Active Agents");
    if (s_agents_table) tui_table_render(s_agents_table);
}

static void agents_config_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Configuration:");
    if (s_activity_spinner) {
        tui_spinner_render(s_activity_spinner);
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 7, 2, "Tasks completed: %d", g_demo.build_frame / 10);
    }
}

static void agents_controls_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Controls:");
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_agents_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "Agents");
    if (!ws) return NULL;

    TuiTab* tab = tui_tab_create(ws, "Active Agents");
    tui_workspace_add_tab(ws, tab);

    /* Agents table */
    TuiWindow* win_table = tui_window_create(tab, 35, 12);
    win_table->_user_data = "Agents";
    win_table->_render_cb = agents_table_render;
    tui_tab_add_window(tab, win_table);
    ncplane_move_yx(win_table->_plane, 2, 2);
    s_agents_table = tui_table_create(win_table->_plane, 2, 2, 30, 9, 2);
    if (s_agents_table) {
        tui_table_set_header(s_agents_table, 0, "Agent");
        tui_table_set_header(s_agents_table, 1, "Status");
        const char* r1[] = {"Code Review", "Running"};
        const char* r2[] = {"Test Gen", "Running"};
        const char* r3[] = {"Doc Writer", "Idle"};
        const char* r4[] = {"Security", "Stopped"};
        tui_table_add_row(s_agents_table, r1);
        tui_table_add_row(s_agents_table, r2);
        tui_table_add_row(s_agents_table, r3);
        tui_table_add_row(s_agents_table, r4);
        tui_window_add_widget(win_table, s_agents_table, s_agents_table->plane, tui_table_get_type_id());
    }

    /* Config */
    TuiWindow* win_config = tui_window_create(tab, 25, 10);
    win_config->_user_data = "Config";
    win_config->_render_cb = agents_config_render;
    tui_tab_add_window(tab, win_config);
    ncplane_move_yx(win_config->_plane, 2, 38);
    s_review_cb = tui_checkbox_create(win_config->_plane, 3, 2, "Auto-review", true, NULL, NULL);
    if (s_review_cb) tui_window_add_widget(win_config, s_review_cb, s_review_cb->plane, tui_checkbox_get_type_id());
    s_test_cb = tui_checkbox_create(win_config->_plane, 4, 2, "Auto-test", false, NULL, NULL);
    if (s_test_cb) tui_window_add_widget(win_config, s_test_cb, s_test_cb->plane, tui_checkbox_get_type_id());
    s_doc_cb = tui_checkbox_create(win_config->_plane, 5, 2, "Auto-docs", true, NULL, NULL);
    if (s_doc_cb) tui_window_add_widget(win_config, s_doc_cb, s_doc_cb->plane, tui_checkbox_get_type_id());
    s_activity_spinner = tui_spinner_create(win_config->_plane, 6, 2, SPINNER_DOTS);
    if (s_activity_spinner) {
        tui_spinner_start(s_activity_spinner);
        tui_window_add_widget(win_config, s_activity_spinner, s_activity_spinner->plane, tui_spinner_get_type_id());
    }

    /* Controls */
    TuiWindow* win_controls = tui_window_create(tab, 25, 8);
    win_controls->_user_data = "Controls";
    win_controls->_render_cb = agents_controls_render;
    tui_tab_add_window(tab, win_controls);
    ncplane_move_yx(win_controls->_plane, 12, 2);
    TuiButton* btn_start = tui_button_create(win_controls->_plane, 3, 2, 18, "Start All", on_btn_start, NULL);
    if (btn_start) tui_window_add_widget(win_controls, btn_start, btn_start->plane, tui_button_get_type_id());
    TuiButton* btn_stop = tui_button_create(win_controls->_plane, 4, 2, 18, "Stop All", on_btn_stop, NULL);
    if (btn_stop) tui_window_add_widget(win_controls, btn_stop, btn_stop->plane, tui_button_get_type_id());
    TuiButton* btn_review = tui_button_create(win_controls->_plane, 5, 2, 18, "Review Now", on_btn_review, NULL);
    if (btn_review) tui_window_add_widget(win_controls, btn_review, btn_review->plane, tui_button_get_type_id());
    TuiButton* btn_dialog = tui_button_create(win_controls->_plane, 6, 2, 18, "Show Dialog", on_btn_dialog, NULL);
    if (btn_dialog) tui_window_add_widget(win_controls, btn_dialog, btn_dialog->plane, tui_button_get_type_id());

    /* Confirmation dialog (created but not shown by default) */
    s_dialog = tui_dialog_create(win_controls->_plane, 1, 2, 22, 6, "Confirm", "Stop all agents?", on_dialog_close, NULL);
    if (s_dialog) {
        tui_dialog_add_button(s_dialog, "Yes");
        tui_dialog_add_button(s_dialog, "No");
        s_dialog_parent = win_controls->_plane;
    }

    return ws;
}
