#include "common.h"
#include "core/types.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/theme.h"
#include "core/types_private.h"
#include "widget/file_picker.h"

/* ============================================================
 * Callbacks
 * ============================================================ */

static void on_file_pick(const char* path, void* userdata) {
    (void)userdata;
    if (path) {
        tui_log(LOG_INFO, "[FilePicker] Selected: %s", path);
    }
}

/* ============================================================
 * Factory function
 * ============================================================ */

TuiWorkspace* create_files_workspace(TuiManager* mgr) {
    TuiWorkspace* ws_filepicker = tui_workspace_create(mgr, "Files");
    tui_manager_add_workspace(mgr, ws_filepicker);

    TuiTab* tab_filepicker = tui_tab_create(ws_filepicker, "File Picker");
    tui_workspace_add_tab(ws_filepicker, tab_filepicker);

    TuiWindow* win_fp = tui_window_create(tab_filepicker, 40, 18);
    win_fp->_user_data = "File Browser";
    win_fp->_render_cb = NULL;

    tui_tab_add_window(tab_filepicker, win_fp);
    ncplane_move_yx(win_fp->_plane, 2, 2);

    {
        struct ncplane* p = tui_window_get_plane(win_fp);
        /* Create a file picker starting at current directory */
        TuiFilePicker* fp = tui_file_picker_create(p, 2, 2, 35, 14, ".", on_file_pick, NULL);
        if (fp) {
            tui_file_picker_ensure_registered();
            tui_window_add_widget(win_fp, fp, fp->plane, tui_file_picker_get_type_id());
        }
    }

    return ws_filepicker;
}
