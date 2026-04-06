#include "core/workspace.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/tab.h"
#include "core/theme.h"
#include "core/types_private.h"
#include <stdlib.h>
#include <string.h>

TuiWorkspace* tui_workspace_create(TuiManager* manager, const char* name) {
    if (!manager) {
        tui_log(LOG_ERROR, "tui_workspace_create: manager is NULL");
        return NULL;
    }
    if (!name) {
        tui_log(LOG_ERROR, "tui_workspace_create: name is NULL");
        return NULL;
    }
    TuiWorkspace* ws = (TuiWorkspace*)calloc(1, sizeof(TuiWorkspace));
    if (!ws) {
        tui_log(LOG_ERROR, "OOM in tui_workspace_create");
        return NULL;
    }
    ws->_id = tui_next_id();
    strncpy(ws->_name, name, sizeof(ws->_name)-1);
    ws->_name[sizeof(ws->_name)-1] = '\0';

    ws->_tab_capacity = 4;
    ws->_tabs = (TuiTab**)calloc(ws->_tab_capacity, sizeof(TuiTab*));
    if (!ws->_tabs) {
        free(ws);
        tui_log(LOG_ERROR, "OOM in tui_workspace_create (tabs array)");
        return NULL;
    }
    ws->_active_tab_index = -1;

    unsigned dimy, dimx;
    ncplane_dim_yx(manager->_stdplane, &dimy, &dimx);

    struct ncplane_options opts = {
        .y = OFFSCREEN_Y, .x = 0, /* Hidden by default */
        .rows = dimy - 1, .cols = dimx, /* Reserve the last bottom row */
        .flags = 0
    };
    ws->_ws_plane = ncplane_create(manager->_stdplane, &opts);
    if (!ws->_ws_plane) {
        free(ws->_tabs);
        free(ws);
        tui_log(LOG_ERROR, "Failed to create ncplane in tui_workspace_create");
        return NULL;
    }

    return ws;
}

void tui_workspace_destroy(TuiWorkspace* ws) {
    if (!ws) return;
    for (int i = 0; i < ws->_tab_count; i++) {
        tui_tab_destroy(ws->_tabs[i]);
    }
    free(ws->_tabs);
    ncplane_destroy(ws->_ws_plane);
    free(ws);
}

void tui_workspace_add_tab(TuiWorkspace* ws, TuiTab* tab) {
    if (!ws || !tab) return;
    if (ws->_tab_count >= ws->_tab_capacity) {
        int new_capacity = ws->_tab_capacity * 2;
        TuiTab** new_tabs = (TuiTab**)realloc(ws->_tabs, new_capacity * sizeof(TuiTab*));
        if (!new_tabs) {
            tui_log(LOG_ERROR, "OOM in tui_workspace_add_tab (realloc)");
            return;
        }
        ws->_tab_capacity = new_capacity;
        ws->_tabs = new_tabs;
    }
    ws->_tabs[ws->_tab_count++] = tab;
    if (ws->_active_tab_index == -1) {
        tui_workspace_set_active_tab(ws, ws->_tab_count - 1);
    }
}

void tui_workspace_set_active_tab(TuiWorkspace* ws, int index) {
    if (!ws) return;
    if (index < 0 || index >= ws->_tab_count) return;

    /* Off-screen hiding */
    if (ws->_active_tab_index != -1) {
        ncplane_move_yx(ws->_tabs[ws->_active_tab_index]->_tab_plane, OFFSCREEN_Y, 0);
    }
    ws->_active_tab_index = index;
    ncplane_move_yx(ws->_tabs[index]->_tab_plane, 2, 0); /* Restore position under the modified header */
    ncplane_move_top(ws->_tabs[index]->_tab_plane);
}

void tui_workspace_remove_active_tab(TuiWorkspace* ws) {
    if (!ws || ws->_tab_count == 0 || ws->_active_tab_index < 0) return;
    int idx = ws->_active_tab_index;
    tui_tab_destroy(ws->_tabs[idx]);
    for (int i = idx; i < ws->_tab_count - 1; i++) {
        ws->_tabs[i] = ws->_tabs[i + 1];
    }
    ws->_tab_count--;
    ws->_active_tab_index = -1;
    if (ws->_tab_count > 0) {
        tui_workspace_set_active_tab(ws, ws->_tab_count - 1);
    }
}

/* Getters */

const char* tui_workspace_get_name(const TuiWorkspace* ws) {
    return ws ? ws->_name : NULL;
}

int tui_workspace_get_id(const TuiWorkspace* ws) {
    return ws ? ws->_id : -1;
}

int tui_workspace_get_tab_count(const TuiWorkspace* ws) {
    return ws ? ws->_tab_count : 0;
}
