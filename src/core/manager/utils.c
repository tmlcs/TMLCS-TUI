#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"

// Helper para verificar si una ventana pertenece a un workspace
// (recorre todos los tabs del workspace buscando la ventana)
bool window_belongs_to_workspace(TuiWindow* win, TuiWorkspace* ws) {
    if (!win || !ws) return false;
    for (int i = 0; i < ws->tab_count; i++) {
        TuiTab* tab = ws->tabs[i];
        if (!tab) continue;
        for (int j = 0; j < tab->window_count; j++) {
            if (tab->windows[j] == win) {
                return true;
            }
        }
    }
    return false;
}
