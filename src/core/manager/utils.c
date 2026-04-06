#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/types_private.h"

/* Helper to verify that a window belongs to a workspace
 * (iterates all tabs of the workspace searching for the window) */
bool window_belongs_to_workspace(TuiWindow* win, TuiWorkspace* ws) {
    if (!win || !ws) return false;
    for (int i = 0; i < ws->_tab_count; i++) {
        TuiTab* tab = ws->_tabs[i];
        if (!tab) continue;
        for (int j = 0; j < tab->_window_count; j++) {
            if (tab->_windows[j] == win) {
                return true;
            }
        }
    }
    return false;
}
