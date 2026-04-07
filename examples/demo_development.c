/**
 * @file demo_development.c
 * @brief Development workspace: file tree, code editor, build progress with spinner.
 */

#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/logger.h"
#include "widget/tree.h"
#include "widget/textarea.h"
#include "widget/tab_container.h"
#include "widget/spinner.h"
#include "widget/progress.h"
#include "widget/label.h"
#include <stdio.h>

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiTree* s_file_tree;
static TuiTextArea* s_code_editor;
static TuiTabContainer* s_dev_tabs;
static TuiSpinner* s_build_spinner;
static TuiProgressBar* s_build_progress;
static TuiLabel* s_build_label;

/* ============================================================
 * Renderers
 * ============================================================ */

static void dev_explorer_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Project Files:");
}

static void dev_editor_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Code Editor:");
    if (s_code_editor) tui_textarea_render(s_code_editor);
}

static void dev_build_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (s_build_progress) {
        tui_progress_set_value(s_build_progress, g_demo.build_progress / 100.0f);
        tui_progress_render(s_build_progress);
    }
    if (s_build_label) {
        ncplane_set_fg_rgb(win->_plane, THEME_FG_DEFAULT);
        ncplane_printf_yx(win->_plane, 2, 2, "Build: %.0f%%", g_demo.build_progress);
    }
    if (s_build_spinner) {
        tui_spinner_render(s_build_spinner);
        ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
        ncplane_putstr_yx(win->_plane, 4, 4, "Compiling...");
    }
}

static void dev_tabs_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
    ncplane_set_fg_rgb(win->_plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(win->_plane, 2, 2, "Views:");
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_development_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "Development");
    if (!ws) return NULL;

    TuiTab* tab = tui_tab_create(ws, "IDE");
    tui_workspace_add_tab(ws, tab);

    /* File tree */
    TuiWindow* win_tree = tui_window_create(tab, 22, 14);
    win_tree->_user_data = "Files";
    win_tree->_render_cb = dev_explorer_render;
    tui_tab_add_window(tab, win_tree);
    ncplane_move_yx(win_tree->_plane, 2, 2);
    s_file_tree = tui_tree_create(win_tree->_plane, 3, 1, 19, 11, NULL, NULL);
    if (s_file_tree) {
        int src = tui_tree_add_node(s_file_tree, "src", -1);
        tui_tree_add_node(s_file_tree, "core", src);
        tui_tree_add_node(s_file_tree, "widget", src);
        int inc = tui_tree_add_node(s_file_tree, "include", -1);
        tui_tree_add_node(s_file_tree, "core", inc);
        tui_tree_add_node(s_file_tree, "widget", inc);
        tui_tree_add_node(s_file_tree, "tests", -1);
        tui_tree_add_node(s_file_tree, "examples", -1);
        tui_tree_add_node(s_file_tree, "Makefile", -1);
        tui_tree_toggle(s_file_tree, src);
        tui_tree_toggle(s_file_tree, inc);
        tui_window_add_widget(win_tree, s_file_tree, s_file_tree->plane, tui_tree_get_type_id());
    }

    /* Code editor */
    TuiWindow* win_editor = tui_window_create(tab, 45, 14);
    win_editor->_user_data = "main.c";
    win_editor->_render_cb = dev_editor_render;
    tui_tab_add_window(tab, win_editor);
    ncplane_move_yx(win_editor->_plane, 2, 25);
    s_code_editor = tui_textarea_create(win_editor->_plane, 3, 1, 43, 10);
    if (s_code_editor) {
        tui_window_add_widget(win_editor, s_code_editor, s_code_editor->plane, tui_textarea_get_type_id());
    }

    /* Build status */
    TuiWindow* win_build = tui_window_create(tab, 45, 8);
    win_build->_user_data = "Build";
    win_build->_render_cb = dev_build_render;
    tui_tab_add_window(tab, win_build);
    ncplane_move_yx(win_build->_plane, 16, 25);
    s_build_progress = tui_progress_create(win_build->_plane, 5, 2, 38);
    if (s_build_progress) tui_window_add_widget(win_build, s_build_progress, s_build_progress->plane, tui_progress_get_type_id());
    s_build_label = tui_label_create(win_build->_plane, 2, 2, "");
    if (s_build_label) tui_window_add_widget(win_build, s_build_label, s_build_label->plane, tui_label_get_type_id());
    s_build_spinner = tui_spinner_create(win_build->_plane, 6, 2, SPINNER_LINE);
    if (s_build_spinner) {
        tui_spinner_start(s_build_spinner);
        tui_window_add_widget(win_build, s_build_spinner, s_build_spinner->plane, tui_spinner_get_type_id());
    }

    /* Tab container with multiple views */
    TuiWindow* win_tabs = tui_window_create(tab, 22, 6);
    win_tabs->_user_data = "Views";
    win_tabs->_render_cb = dev_tabs_render;
    tui_tab_add_window(tab, win_tabs);
    ncplane_move_yx(win_tabs->_plane, 16, 2);
    s_dev_tabs = tui_tab_container_create(win_tabs->_plane, 3, 1, 19, 4, NULL, NULL);
    if (s_dev_tabs) {
        tui_tab_container_add_tab(s_dev_tabs, "Editor");
        tui_tab_container_add_tab(s_dev_tabs, "Terminal");
        tui_tab_container_add_tab(s_dev_tabs, "Debug");
        tui_tab_container_add_tab(s_dev_tabs, "Problems");
        tui_window_add_widget(win_tabs, s_dev_tabs, s_dev_tabs->plane, tui_tab_container_get_type_id());
    }

    return ws;
}
