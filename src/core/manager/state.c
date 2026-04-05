#include "core/manager.h"
#include "core/workspace.h"
#include "core/logger.h"
#include "core/theme.h"
#include <stdlib.h>

int s_next_id = 1;

TuiManager* tui_manager_create(struct notcurses* nc) {
    TuiManager* mgr = (TuiManager*)calloc(1, sizeof(TuiManager));
    if (!mgr) {
        tui_log(LOG_ERROR, "OOM en tui_manager_create");
        return NULL;
    }
    mgr->nc = nc;
    mgr->stdplane = notcurses_stdplane(nc);
    if (!mgr->stdplane) {
        free(mgr);
        tui_log(LOG_ERROR, "notcurses_stdplane devolvió NULL en tui_manager_create");
        return NULL;
    }
    mgr->workspace_capacity = 4;
    mgr->workspaces = (TuiWorkspace**)calloc(mgr->workspace_capacity, sizeof(TuiWorkspace*));
    if (!mgr->workspaces) {
        free(mgr);
        tui_log(LOG_ERROR, "OOM en tui_manager_create (workspaces array)");
        return NULL;
    }
    mgr->active_workspace_index = -1;
    mgr->running = true;
    return mgr;
}

void tui_manager_destroy(TuiManager* manager) {
    if (!manager) return;
    for (int i = 0; i < manager->workspace_count; i++) {
        tui_workspace_destroy(manager->workspaces[i]);
    }
    free(manager->workspaces);
    free(manager);
}

void tui_manager_add_workspace(TuiManager* manager, TuiWorkspace* ws) {
    if (manager->workspace_count >= manager->workspace_capacity) {
        manager->workspace_capacity *= 2;
        TuiWorkspace** new_workspaces = (TuiWorkspace**)realloc(manager->workspaces, manager->workspace_capacity * sizeof(TuiWorkspace*));
        if (!new_workspaces) {
            tui_log(LOG_ERROR, "OOM en tui_manager_add_workspace (realloc)");
            return; // No agregamos el workspace
        }
        manager->workspaces = new_workspaces;
    }
    manager->workspaces[manager->workspace_count++] = ws;
    if (manager->active_workspace_index == -1) {
        tui_manager_set_active_workspace(manager, manager->workspace_count - 1);
    }
}

void tui_manager_set_active_workspace(TuiManager* manager, int index) {
    if (index < 0 || index >= manager->workspace_count) return;
    
    // Ocultar enviándolo fuera de pantalla (Off-screen parking)
    if (manager->active_workspace_index != -1) {
        TuiWorkspace* prev = manager->workspaces[manager->active_workspace_index];
        ncplane_move_yx(prev->ws_plane, OFFSCREEN_Y, 0);
    }
    
    manager->active_workspace_index = index;
    TuiWorkspace* next = manager->workspaces[index];
    ncplane_move_yx(next->ws_plane, 0, 0);
    ncplane_move_top(next->ws_plane);
}

void tui_manager_remove_active_workspace(TuiManager* mgr) {
    if (!mgr || mgr->workspace_count == 0 || mgr->active_workspace_index < 0) return;
    int idx = mgr->active_workspace_index;
    tui_workspace_destroy(mgr->workspaces[idx]);
    for (int i = idx; i < mgr->workspace_count - 1; i++) {
        mgr->workspaces[i] = mgr->workspaces[i + 1];
    }
    mgr->workspace_count--;
    mgr->active_workspace_index = -1;
    if (mgr->workspace_count > 0) {
        tui_manager_set_active_workspace(mgr, mgr->workspace_count - 1);
    }
}
