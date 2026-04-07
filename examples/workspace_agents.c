#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/theme.h"
#include "core/types_private.h"
#include "widget/list.h"
#include "widget/checkbox.h"
#include "widget/button.h"
#include "widget/label.h"
#include "widget/dialog.h"

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_dialog_result(int button_index, void* userdata) {
    (void)userdata;
    const char* action = (button_index == 0) ? "Confirmed" : "Cancelled";
    tui_log(LOG_INFO, "[Dialog] %s (button %d)", action, button_index);
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
    tui_log(LOG_INFO, "[Button] Dialog shown");
}

/* ============================================================
 * Dialog state (shared with demo.c for rendering)
 * ============================================================ */

static bool g_dialog_visible = false;
static TuiDialog* g_dialog = NULL;

static struct ncplane* g_dialog_parent = NULL;

/* Expose dialog state to demo.c for key routing and overlay rendering */
bool agents_get_dialog_visible(void) { return g_dialog_visible; }
TuiDialog* agents_get_dialog(void) { return g_dialog; }
void agents_set_dialog_visible(bool vis) { g_dialog_visible = vis; }
struct ncplane* agents_get_dialog_parent(void) { return g_dialog_parent; }
void agents_set_dialog_parent(struct ncplane* p) { g_dialog_parent = p; }

/* ============================================================
 * Persistent widget pointers
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

/* ============================================================
 * Agents workspace renderers
 * ============================================================ */

static void agents_list_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

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
    demo_render_window_frame(win);

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
    demo_render_window_frame(win);

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
 * Factory function
 * ============================================================ */

TuiWorkspace* create_agents_workspace(TuiManager* mgr) {
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
        struct ncplane* std = notcurses_stdplane(mgr->_nc);
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

    return ws_agents;
}
