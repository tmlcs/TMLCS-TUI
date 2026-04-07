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
#include "widget/tree.h"
#include "widget/spinner.h"
#include "widget/progress.h"
#include "widget/label.h"
#include "widget/tab_container.h"
#include <stdio.h>

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_tree_select(int node_index, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Tree] Selected node %d", node_index);
}

static void on_tab_change(int tab_index, void* userdata) {
    (void)userdata;
    tui_log(LOG_INFO, "[Tabs] Switched to tab %d", tab_index);
}

/* ============================================================
 * Persistent widget pointers
 * ============================================================ */

static TuiTree* g_dev_file_tree = NULL;
static TuiTextArea* g_dev_editor = NULL;
static TuiTabContainer* g_dev_tabs = NULL;
static TuiSpinner* g_dev_spinner = NULL;
static TuiProgressBar* g_dev_build_prog = NULL;
static TuiLabel* g_dev_build_label = NULL;

/* ============================================================
 * Development workspace renderers
 * ============================================================ */

static void dev_files_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "File Browser:");

    if (g_dev_file_tree) {
        tui_tree_render(g_dev_file_tree);
    }

    tui_window_mark_dirty(win);
}

static void dev_editor_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (g_dev_editor) {
        tui_textarea_render(g_dev_editor);
    }

    tui_window_mark_dirty(win);
}

static void dev_git_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Git Status:");

    ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO);
    ncplane_printf_yx(p, 3, 2, "On branch main");
    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 4, 2, "Modified: src/core/manager/render.c");
    ncplane_printf_yx(p, 5, 2, "Modified: examples/demo.c");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN);
    ncplane_printf_yx(p, 6, 2, "Untracked: tests/new_test.c");

    tui_window_mark_dirty(win);
}

static void dev_build_progress_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Build Progress:");

    if (g_dev_build_label) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Compiling... %.0f%%", g_demo.build_progress);
        tui_label_set_text(g_dev_build_label, buf);
        tui_label_render(g_dev_build_label);
    }

    if (g_dev_build_prog) {
        tui_progress_set_value(g_dev_build_prog, g_demo.build_progress / 100.0f);
        tui_progress_render(g_dev_build_prog);
    }

    if (g_dev_spinner) {
        if (g_demo.build_frame % 3 == 0) {
            tui_spinner_tick(g_dev_spinner);
        }
        tui_spinner_render(g_dev_spinner);
    }

    tui_window_mark_dirty(win);
}

static void dev_build_log_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    struct ncplane* p = tui_window_get_plane(win);
    ncplane_set_bg_rgb(p, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(p, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(p, 2, 2, "Build Output:");

    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 3, 2, "gcc -Wall -O2 -c main.c");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO);
    ncplane_printf_yx(p, 4, 2, "[OK] Compiled successfully");
    ncplane_set_fg_rgb(p, THEME_FG_DEFAULT);
    ncplane_printf_yx(p, 5, 2, "gcc -Wall -O2 -c utils.c");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_INFO);
    ncplane_printf_yx(p, 6, 2, "[OK] Compiled successfully");
    ncplane_set_fg_rgb(p, THEME_FG_LOG_WARN);
    ncplane_printf_yx(p, 7, 2, "Warning: unused variable");

    tui_window_mark_dirty(win);
}

static void dev_tabs_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);

    if (g_dev_tabs) {
        tui_tab_container_render(g_dev_tabs);
    }

    tui_window_mark_dirty(win);
}

/* ============================================================
 * Factory function
 * ============================================================ */

TuiWorkspace* create_development_workspace(TuiManager* mgr) {
    TuiWorkspace* ws_dev = tui_workspace_create(mgr, "Development");
    tui_manager_add_workspace(mgr, ws_dev);

    TuiTab* tab_dev_editor = tui_tab_create(ws_dev, "Editor");
    tui_workspace_add_tab(ws_dev, tab_dev_editor);

    /* File browser: Tree */
    TuiWindow* win_dev_files = tui_window_create(tab_dev_editor, 25, 18);
    win_dev_files->_user_data = "Files";
    win_dev_files->_render_cb = dev_files_render;
    tui_tab_add_window(tab_dev_editor, win_dev_files);
    ncplane_move_yx(win_dev_files->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_dev_files);
        g_dev_file_tree = tui_tree_create(p, 3, 2, 20, 14, on_tree_select, NULL);
        if (g_dev_file_tree) {
            int src = tui_tree_add_node(g_dev_file_tree, "src/", false);
            int core = tui_tree_add_child(g_dev_file_tree, src, "core/", false);
            tui_tree_add_child(g_dev_file_tree, core, "manager.c", true);
            tui_tree_add_child(g_dev_file_tree, core, "workspace.c", true);
            tui_tree_add_child(g_dev_file_tree, core, "tab.c", true);
            tui_tree_add_child(g_dev_file_tree, core, "window.c", true);
            int widget_d = tui_tree_add_child(g_dev_file_tree, src, "widget/", false);
            tui_tree_add_child(g_dev_file_tree, widget_d, "button.c", true);
            tui_tree_add_child(g_dev_file_tree, widget_d, "slider.c", true);
            tui_tree_add_child(g_dev_file_tree, widget_d, "table.c", true);
            int include = tui_tree_add_node(g_dev_file_tree, "include/", false);
            tui_tree_add_child(g_dev_file_tree, include, "tmlcs_tui.h", true);
            tui_tree_add_node(g_dev_file_tree, "CMakeLists.txt", true);
            tui_tree_add_node(g_dev_file_tree, "README.md", true);
            tui_tree_add_node(g_dev_file_tree, "Makefile", true);
            tui_tree_toggle(g_dev_file_tree, src);
            tui_tree_set_selected(g_dev_file_tree, 0);
            tui_tree_ensure_registered();
            tui_window_add_widget(win_dev_files, g_dev_file_tree, g_dev_file_tree->plane, tui_tree_get_type_id());
        }
    }

    /* Code editor: TextArea */
    TuiWindow* win_dev_code = tui_window_create(tab_dev_editor, 45, 12);
    win_dev_code->_user_data = "main.c";
    win_dev_code->_render_cb = dev_editor_render;
    tui_tab_add_window(tab_dev_editor, win_dev_code);
    ncplane_move_yx(win_dev_code->_plane, 2, 28);

    {
        struct ncplane* p = tui_window_get_plane(win_dev_code);
        g_dev_editor = tui_textarea_create(p, 2, 2, 40, 9);
        if (g_dev_editor) {
            tui_textarea_ensure_registered();
            tui_window_add_widget(win_dev_code, g_dev_editor, g_dev_editor->plane, tui_textarea_get_type_id());
        }
    }

    /* Git status */
    TuiWindow* win_dev_git = tui_window_create(tab_dev_editor, 45, 8);
    win_dev_git->_user_data = "Git Status";
    win_dev_git->_render_cb = dev_git_render;
    tui_tab_add_window(tab_dev_editor, win_dev_git);
    ncplane_move_yx(win_dev_git->_plane, 14, 28);

    /* Build tab */
    TuiTab* tab_dev_build = tui_tab_create(ws_dev, "Build");
    tui_workspace_add_tab(ws_dev, tab_dev_build);

    TuiWindow* win_dev_build_prog = tui_window_create(tab_dev_build, 45, 10);
    win_dev_build_prog->_user_data = "Build Progress";
    win_dev_build_prog->_render_cb = dev_build_progress_render;
    tui_tab_add_window(tab_dev_build, win_dev_build_prog);
    ncplane_move_yx(win_dev_build_prog->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_dev_build_prog);
        g_dev_build_label = tui_label_create(p, 3, 2, "Compiling... 0%");
        if (g_dev_build_label) {
            tui_label_ensure_registered();
            tui_window_add_widget(win_dev_build_prog, g_dev_build_label, g_dev_build_label->plane, tui_label_get_type_id());
        }

        g_dev_build_prog = tui_progress_create(p, 5, 2, 38);
        if (g_dev_build_prog) {
            tui_progress_ensure_registered();
            tui_window_add_widget(win_dev_build_prog, g_dev_build_prog, g_dev_build_prog->plane, tui_progress_get_type_id());
        }

        g_dev_spinner = tui_spinner_create(p, 7, 2, SPINNER_DOTS);
        if (g_dev_spinner) {
            tui_spinner_start(g_dev_spinner);
            tui_spinner_ensure_registered();
            tui_window_add_widget(win_dev_build_prog, g_dev_spinner, g_dev_spinner->plane, tui_spinner_get_type_id());
        }

        TuiLabel* spin_label = tui_label_create(p, 7, 5, " Building...");
        if (spin_label) {
            tui_label_ensure_registered();
            tui_window_add_widget(win_dev_build_prog, spin_label, spin_label->plane, tui_label_get_type_id());
        }
    }

    TuiWindow* win_dev_build_log = tui_window_create(tab_dev_build, 40, 10);
    win_dev_build_log->_user_data = "Build Log";
    win_dev_build_log->_render_cb = dev_build_log_render;
    tui_tab_add_window(tab_dev_build, win_dev_build_log);
    ncplane_move_yx(win_dev_build_log->_plane, 2, 48);

    /* Dev TabContainer workspace */
    TuiWindow* win_dev_tabs = tui_window_create(tab_dev_build, 40, 8);
    win_dev_tabs->_user_data = "Tabs Demo";
    win_dev_tabs->_render_cb = dev_tabs_render;
    tui_tab_add_window(tab_dev_build, win_dev_tabs);
    ncplane_move_yx(win_dev_tabs->_plane, 12, 48);

    {
        struct ncplane* p = tui_window_get_plane(win_dev_tabs);
        g_dev_tabs = tui_tab_container_create(p, 2, 2, 35, 5, on_tab_change, NULL);
        if (g_dev_tabs) {
            tui_tab_container_add_tab(g_dev_tabs, "Editor");
            tui_tab_container_add_tab(g_dev_tabs, "Terminal");
            tui_tab_container_add_tab(g_dev_tabs, "Output");
            tui_tab_container_add_tab(g_dev_tabs, "Problems");
            tui_tab_container_set_active(g_dev_tabs, 0);
            tui_tab_container_ensure_registered();
            tui_window_add_widget(win_dev_tabs, g_dev_tabs, g_dev_tabs->plane, tui_tab_container_get_type_id());
        }
    }

    return ws_dev;
}
