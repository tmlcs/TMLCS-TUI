#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/theme.h"
#include "core/types_private.h"
#include "widget/textarea.h"
#include "widget/button.h"
#include "widget/checkbox.h"
#include "widget/radio_group.h"

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_radio_theme(int selected, void* userdata) {
    (void)userdata;
    static const char* g_theme_names[] = {"Dracula", "Solarized", "High Contrast", "Monochrome"};
    if (selected >= 0 && selected < 4) {
        tui_log(LOG_INFO, "[Theme] Selected: %s", g_theme_names[selected]);
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

/* ============================================================
 * Persistent widget pointers
 * ============================================================ */

static TuiTextArea* g_desk_notes = NULL;
static TuiButton* g_desk_btn_refresh = NULL;
static TuiButton* g_desk_btn_settings = NULL;
static TuiButton* g_desk_btn_help = NULL;
static TuiCheckbox* g_desk_cb_network = NULL;
static TuiCheckbox* g_desk_cb_bluetooth = NULL;
static TuiCheckbox* g_desk_cb_autoupdate = NULL;
static TuiRadioGroup* g_desk_radio_theme = NULL;

/* ============================================================
 * Desktop workspace renderers
 * ============================================================ */

static void desktop_notes_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (g_desk_notes) {
        tui_textarea_render(g_desk_notes);
    }

    tui_window_mark_dirty(win);
}

static void desktop_shortcuts_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

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
    demo_render_window_frame(win);

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

static void desktop_theme_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);

    if (g_desk_radio_theme) {
        tui_radio_group_render(g_desk_radio_theme);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Factory function
 * ============================================================ */

TuiWorkspace* create_desktop_workspace(TuiManager* mgr) {
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

    return ws_desktop;
}
