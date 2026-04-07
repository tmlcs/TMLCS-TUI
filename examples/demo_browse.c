/**
 * @file demo_browse.c
 * @brief Browse workspace: URL bar, content area, bookmark tree, history list.
 */

#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/logger.h"
#include "widget/text_input.h"
#include "widget/textarea.h"
#include "widget/tree.h"
#include "widget/list.h"
#include <stdio.h>

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiTextInput* s_url_input;
static TuiTextArea* s_content_area;
static TuiTree* s_bookmarks_tree;
static TuiList* s_history_list;

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_url_submit(const char* text, void* ud) {
    (void)ud;
    if (text && text[0]) {
        tui_log(LOG_INFO, "Navigating to: %s", text);
        if (s_history_list) tui_list_add_item(s_history_list, text);
    }
}

/* ============================================================
 * Renderers
 * ============================================================ */

static void browse_urlbar_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    if (s_url_input) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
        ncplane_putstr_yx(win->_plane, 2, 2, "URL:");
        ncplane_move_top(s_url_input->plane);
        tui_text_input_render(s_url_input);
    }
}

static void browse_content_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    if (s_content_area) {
        tui_textarea_render(s_content_area);
    }
}

static void browse_bookmarks_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
}

static void browse_history_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_browse_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "Browse");
    if (!ws) return NULL;

    TuiTab* tab = tui_tab_create(ws, "Web");
    tui_workspace_add_tab(ws, tab);

    /* URL bar */
    TuiWindow* win_url = tui_window_create(tab, 50, 3);
    win_url->_user_data = "Navigation";
    win_url->_render_cb = browse_urlbar_render;
    tui_tab_add_window(tab, win_url);
    ncplane_move_yx(win_url->_plane, 2, 2);
    s_url_input = tui_text_input_create(win_url->_plane, 1, 7, 40, on_url_submit, NULL);
    if (s_url_input) tui_window_add_widget(win_url, s_url_input, s_url_input->plane, tui_text_input_get_type_id());

    /* Content area */
    TuiWindow* win_content = tui_window_create(tab, 50, 12);
    win_content->_user_data = "Page Content";
    win_content->_render_cb = browse_content_render;
    tui_tab_add_window(tab, win_content);
    ncplane_move_yx(win_content->_plane, 5, 2);
    s_content_area = tui_textarea_create(win_content->_plane, 1, 1, 48, 10);
    if (s_content_area) {
        tui_window_add_widget(win_content, s_content_area, s_content_area->plane, tui_textarea_get_type_id());
    }

    /* Bookmarks tree */
    TuiWindow* win_bookmarks = tui_window_create(tab, 25, 10);
    win_bookmarks->_user_data = "Bookmarks";
    win_bookmarks->_render_cb = browse_bookmarks_render;
    tui_tab_add_window(tab, win_bookmarks);
    ncplane_move_yx(win_bookmarks->_plane, 5, 53);
    s_bookmarks_tree = tui_tree_create(win_bookmarks->_plane, 1, 1, 22, 8, NULL, NULL);
    if (s_bookmarks_tree) {
        int dev = tui_tree_add_node(s_bookmarks_tree, "Development", -1);
        tui_tree_add_node(s_bookmarks_tree, "GitHub", dev);
        tui_tree_add_node(s_bookmarks_tree, "Stack Overflow", dev);
        int news = tui_tree_add_node(s_bookmarks_tree, "News", -1);
        tui_tree_add_node(s_bookmarks_tree, "Hacker News", news);
        tui_tree_add_node(s_bookmarks_tree, "Reddit", news);
        tui_tree_add_node(s_bookmarks_tree, "Google", -1);
        tui_tree_toggle(s_bookmarks_tree, dev);
        tui_window_add_widget(win_bookmarks, s_bookmarks_tree, s_bookmarks_tree->plane, tui_tree_get_type_id());
    }

    /* History list */
    TuiWindow* win_history = tui_window_create(tab, 25, 5);
    win_history->_user_data = "History";
    win_history->_render_cb = browse_history_render;
    tui_tab_add_window(tab, win_history);
    ncplane_move_yx(win_history->_plane, 17, 53);
    s_history_list = tui_list_create(win_history->_plane, 1, 1, 22, 8);
    if (s_history_list) tui_window_add_widget(win_history, s_history_list, s_history_list->plane, tui_list_get_type_id());

    return ws;
}
