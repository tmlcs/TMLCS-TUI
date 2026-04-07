/**
 * @file demo_files.c
 * @brief Files workspace: file picker for directory navigation.
 */

#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include "core/logger.h"
#include "widget/file_picker.h"
#include <stdio.h>

/* ============================================================
 * Workspace state
 * ============================================================ */

static TuiFilePicker* s_file_picker;

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_file_select(const char* path, void* ud) {
    (void)ud;
    tui_log(LOG_INFO, "File selected: %s", path);
}

/* ============================================================
 * Renderer
 * ============================================================ */

static void files_picker_render(TuiWindow* win) {
    if (!win || !win->_plane) return;
    demo_render_window_frame(win);
}

/* ============================================================
 * Workspace factory
 * ============================================================ */

TuiWorkspace* create_files_workspace(TuiManager* mgr) {
    TuiWorkspace* ws = tui_workspace_create(mgr, "Files");
    if (!ws) return NULL;

    TuiTab* tab = tui_tab_create(ws, "Explorer");
    tui_workspace_add_tab(ws, tab);

    TuiWindow* win_picker = tui_window_create(tab, 60, 18);
    win_picker->_user_data = "File Explorer";
    win_picker->_render_cb = files_picker_render;
    tui_tab_add_window(tab, win_picker);
    ncplane_move_yx(win_picker->_plane, 2, 2);

    s_file_picker = tui_file_picker_create(win_picker->_plane, 1, 1, 16, 56, ".", on_file_select, NULL);
    if (s_file_picker) tui_window_add_widget(win_picker, s_file_picker, s_file_picker->plane, tui_file_picker_get_type_id());

    return ws;
}
