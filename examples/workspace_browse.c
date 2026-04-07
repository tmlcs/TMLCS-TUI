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
#include "widget/textarea.h"
#include "widget/tree.h"

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_tree_select(int node_index, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Tree] Selected node %d", node_index);
}

/* ============================================================
 * Persistent widget pointers
 * ============================================================ */

static TuiTextInput* g_browse_url = NULL;
static TuiTextArea* g_browse_content = NULL;
static TuiTree* g_browse_bookmarks = NULL;

/* ============================================================
 * Browse workspace renderers
 * ============================================================ */

static void browse_url_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

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
    demo_render_window_frame(win);

    if (g_browse_content) {
        tui_textarea_render(g_browse_content);
    }

    tui_window_mark_dirty(win);
}

static void browse_bookmarks_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (g_browse_bookmarks) {
        tui_tree_render(g_browse_bookmarks);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Factory function
 * ============================================================ */

TuiWorkspace* create_browse_workspace(TuiManager* mgr) {
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

    return ws_browse;
}
