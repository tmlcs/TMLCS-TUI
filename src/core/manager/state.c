#include "core/manager.h"
#include "core/keymap.h"
#include "core/workspace.h"
#include "core/logger.h"
#include "core/theme.h"
#include "core/theme_loader.h"
#include "core/types_private.h"
#include <stdatomic.h>
#include <stdlib.h>

static atomic_int s_next_id = 1;

int tui_next_id(void) {
    return (int)atomic_fetch_add_explicit(&s_next_id, 1, memory_order_relaxed) + 1;
}

void tui_reset_id(int value) {
    atomic_store(&s_next_id, value);
}

TuiManager* tui_manager_create(struct notcurses* nc) {
    TuiManager* mgr = (TuiManager*)calloc(1, sizeof(TuiManager));
    if (!mgr) {
        tui_log(LOG_ERROR, "OOM in tui_manager_create");
        return NULL;
    }
    mgr->_nc = nc;
    mgr->_stdplane = notcurses_stdplane(nc);
    if (!mgr->_stdplane) {
        free(mgr);
        tui_log(LOG_ERROR, "notcurses_stdplane returned NULL in tui_manager_create");
        return NULL;
    }
    mgr->_workspace_capacity = 4;
    mgr->_workspaces = (TuiWorkspace**)calloc(mgr->_workspace_capacity, sizeof(TuiWorkspace*));
    if (!mgr->_workspaces) {
        free(mgr);
        tui_log(LOG_ERROR, "OOM in tui_manager_create (workspaces array)");
        return NULL;
    }
    mgr->_active_workspace_index = -1;
    mgr->_running = true;

    /* Initialize dirty tracking state */
    mgr->_toolbar_needs_redraw = false;
    mgr->_tabs_needs_redraw = false;
    mgr->_taskbar_needs_redraw = false;
    mgr->_last_active_workspace = -1;
    mgr->_last_active_tab = -1;
    mgr->_last_active_window = -1;
    mgr->_last_clock[0] = '\0';

    /* Initialize keymap with default bindings */
    tui_keymap_init(mgr);

    return mgr;
}

void tui_manager_destroy(TuiManager* manager) {
    if (!manager) return;

    /* Free keymap before workspaces */
    free(manager->_keymap);
    manager->_keymap = NULL;

    for (int i = 0; i < manager->_workspace_count; i++) {
        tui_workspace_destroy(manager->_workspaces[i]);
    }
    free(manager->_workspaces);
    free(manager);
}

void tui_manager_add_workspace(TuiManager* manager, TuiWorkspace* ws) {
    if (!manager || !ws) {
        tui_log(LOG_ERROR, "tui_manager_add_workspace: manager or ws is NULL");
        return;
    }
    if (manager->_workspace_count >= manager->_workspace_capacity) {
        int new_capacity = manager->_workspace_capacity * 2;
        TuiWorkspace** new_workspaces = (TuiWorkspace**)realloc(manager->_workspaces,
            (size_t)new_capacity * sizeof(TuiWorkspace*));
        if (!new_workspaces) {
            tui_log(LOG_ERROR, "OOM in tui_manager_add_workspace (realloc)");
            return; /* Capacity NOT updated — buffer unchanged */
        }
        manager->_workspace_capacity = new_capacity;
        manager->_workspaces = new_workspaces;
    }
    manager->_workspaces[manager->_workspace_count++] = ws;
    if (manager->_active_workspace_index == -1) {
        tui_manager_set_active_workspace(manager, manager->_workspace_count - 1);
    }
}

void tui_manager_set_active_workspace(TuiManager* manager, int index) {
    if (index < 0 || index >= manager->_workspace_count) return;

    /* Off-screen parking */
    if (manager->_active_workspace_index != -1) {
        TuiWorkspace* prev = manager->_workspaces[manager->_active_workspace_index];
        ncplane_move_yx(prev->_ws_plane, OFFSCREEN_Y, 0);
    }

    manager->_active_workspace_index = index;
    TuiWorkspace* next = manager->_workspaces[index];
    ncplane_move_yx(next->_ws_plane, 0, 0);
    ncplane_move_top(next->_ws_plane);
}

void tui_manager_remove_active_workspace(TuiManager* mgr) {
    if (!mgr || mgr->_workspace_count == 0 || mgr->_active_workspace_index < 0) return;
    int idx = mgr->_active_workspace_index;
    tui_workspace_destroy(mgr->_workspaces[idx]);
    for (int i = idx; i < mgr->_workspace_count - 1; i++) {
        mgr->_workspaces[i] = mgr->_workspaces[i + 1];
    }
    mgr->_workspace_count--;
    mgr->_active_workspace_index = -1;
    if (mgr->_workspace_count > 0) {
        tui_manager_set_active_workspace(mgr, mgr->_workspace_count - 1);
    }
}

bool tui_manager_set_theme(TuiManager* mgr, const char* name) {
    (void)mgr; /* theme is global */
    return tui_theme_apply(name);
}

/* Opaque type getters */

bool tui_manager_is_running(const TuiManager* mgr) {
    return mgr ? mgr->_running : false;
}

int tui_manager_get_workspace_count(const TuiManager* mgr) {
    return mgr ? mgr->_workspace_count : 0;
}

int tui_manager_get_active_workspace_index(const TuiManager* mgr) {
    return mgr ? mgr->_active_workspace_index : -1;
}
