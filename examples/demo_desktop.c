/**
 * @file demo_desktop.c
 * @brief Desktop workspace: theme selector (radio), status toggles (checkbox),
 *         quick actions (buttons), language selector (dropdown).
 */

#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/logger.h"
#include "core/theme_loader.h"
#include "widget/radio_group.h"
#include "widget/checkbox.h"
#include "widget/button.h"
#include "widget/dropdown.h"
#include <stdio.h>

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiRadioGroup* s_theme_radio;
static TuiCheckbox* s_net_cb;
static TuiCheckbox* s_bt_cb;
static TuiCheckbox* s_update_cb;
static TuiDropdown* s_lang_dropdown;

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_theme_change(int selected, void* userdata) {
    (void)userdata;
    const char* themes[] = {"dracula", "solarized", "highcontrast", "monochrome"};
    const char* labels[] = {"Dracula", "Solarized", "High Contrast", "Monochrome"};
    if (selected >= 0 && selected < 4) {
        tui_theme_apply(themes[selected]);
        tui_log(LOG_INFO, "Theme changed to: %s", labels[selected]);
    }
}

static void on_btn_refresh(void* ud) { (void)ud; tui_log(LOG_INFO, "Action: Refresh"); }
static void on_btn_settings(void* ud) { (void)ud; tui_log(LOG_INFO, "Action: Settings"); }
static void on_btn_help(void* ud) { (void)ud; tui_log(LOG_INFO, "Action: Help"); }
static void on_dropdown_select(int idx, void* ud) {
    (void)ud;
    tui_log(LOG_INFO, "Language selected: %s (#%d)", idx);
}

/* ============================================================
 * Renderers
 * ============================================================ */

static void desktop_theme_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Select Theme:");
}

static void desktop_status_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "System Status:");
}

static void desktop_actions_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Quick Actions:");
}

static void desktop_settings_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Settings:");
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_desktop_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "Desktop");
    if (!ws) return NULL;

    TuiTab* tab = tui_tab_create(ws, "Widgets");
    tui_workspace_add_tab(ws, tab);

    /* Theme selector */
    TuiWindow* win_theme = tui_window_create(tab, 25, 8);
    win_theme->_user_data = "Theme";
    win_theme->_render_cb = desktop_theme_render;
    tui_tab_add_window(tab, win_theme);
    ncplane_move_yx(win_theme->_plane, 2, 2);
    s_theme_radio = tui_radio_group_create(win_theme->_plane, 3, 2, 20, 6, on_theme_change, NULL);
    if (s_theme_radio) {
        tui_radio_group_add_option(s_theme_radio, "Dracula");
        tui_radio_group_add_option(s_theme_radio, "Solarized");
        tui_radio_group_add_option(s_theme_radio, "High Contrast");
        tui_radio_group_add_option(s_theme_radio, "Monochrome");
        tui_window_add_widget(win_theme, s_theme_radio, s_theme_radio->plane, tui_radio_group_get_type_id());
    }

    /* Status toggles */
    TuiWindow* win_status = tui_window_create(tab, 25, 8);
    win_status->_user_data = "Status";
    win_status->_render_cb = desktop_status_render;
    tui_tab_add_window(tab, win_status);
    ncplane_move_yx(win_status->_plane, 2, 28);
    s_net_cb = tui_checkbox_create(win_status->_plane, 3, 2, "Network", true, NULL, NULL);
    if (s_net_cb) tui_window_add_widget(win_status, s_net_cb, s_net_cb->plane, tui_checkbox_get_type_id());
    s_bt_cb = tui_checkbox_create(win_status->_plane, 4, 2, "Bluetooth", false, NULL, NULL);
    if (s_bt_cb) tui_window_add_widget(win_status, s_bt_cb, s_bt_cb->plane, tui_checkbox_get_type_id());
    s_update_cb = tui_checkbox_create(win_status->_plane, 5, 2, "Auto-update", true, NULL, NULL);
    if (s_update_cb) tui_window_add_widget(win_status, s_update_cb, s_update_cb->plane, tui_checkbox_get_type_id());

    /* Quick actions */
    TuiWindow* win_actions = tui_window_create(tab, 25, 8);
    win_actions->_user_data = "Actions";
    win_actions->_render_cb = desktop_actions_render;
    tui_tab_add_window(tab, win_actions);
    ncplane_move_yx(win_actions->_plane, 10, 2);
    TuiButton* btn_r = tui_button_create(win_actions->_plane, 3, 2, 18, "Refresh", on_btn_refresh, NULL);
    if (btn_r) tui_window_add_widget(win_actions, btn_r, btn_r->plane, tui_button_get_type_id());
    TuiButton* btn_s = tui_button_create(win_actions->_plane, 4, 2, 18, "Settings", on_btn_settings, NULL);
    if (btn_s) tui_window_add_widget(win_actions, btn_s, btn_s->plane, tui_button_get_type_id());
    TuiButton* btn_h = tui_button_create(win_actions->_plane, 5, 2, 18, "Help", on_btn_help, NULL);
    if (btn_h) tui_window_add_widget(win_actions, btn_h, btn_h->plane, tui_button_get_type_id());

    /* Language dropdown */
    TuiWindow* win_settings = tui_window_create(tab, 25, 6);
    win_settings->_user_data = "Settings";
    win_settings->_render_cb = desktop_settings_render;
    tui_tab_add_window(tab, win_settings);
    ncplane_move_yx(win_settings->_plane, 10, 28);
    s_lang_dropdown = tui_dropdown_create(win_settings->_plane, 3, 2, 20, on_dropdown_select, NULL);
    if (s_lang_dropdown) {
        tui_dropdown_add_item(s_lang_dropdown, "English");
        tui_dropdown_add_item(s_lang_dropdown, "Español");
        tui_dropdown_add_item(s_lang_dropdown, "日本語");
        tui_window_add_widget(win_settings, s_lang_dropdown, s_lang_dropdown->plane, tui_dropdown_get_type_id());
    }

    return ws;
}
