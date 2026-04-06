#include "core/workspace.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/tab.h"
#include "core/theme.h"
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
        tui_log(LOG_ERROR, "OOM en tui_workspace_create");
        return NULL;
    }
    ws->id = tui_next_id();
    strncpy(ws->name, name, sizeof(ws->name)-1);
    ws->name[sizeof(ws->name)-1] = '\0'; // Null-terminate explicitly
    
    ws->tab_capacity = 4;
    ws->tabs = (TuiTab**)calloc(ws->tab_capacity, sizeof(TuiTab*));
    if (!ws->tabs) {
        free(ws);
        tui_log(LOG_ERROR, "OOM en tui_workspace_create (tabs array)");
        return NULL;
    }
    ws->active_tab_index = -1;
    
    unsigned dimy, dimx;
    ncplane_dim_yx(manager->stdplane, &dimy, &dimx);
    
    struct ncplane_options opts = {
        .y = OFFSCREEN_Y, .x = 0, // Oculto por defecto
        .rows = dimy - 1, .cols = dimx, // Se reserva la última fila inferior
        .flags = 0
    };
    ws->ws_plane = ncplane_create(manager->stdplane, &opts);
    if (!ws->ws_plane) {
        free(ws->tabs);
        free(ws);
        tui_log(LOG_ERROR, "Fallo al crear ncplane en tui_workspace_create");
        return NULL;
    }
    
    return ws;
}

void tui_workspace_destroy(TuiWorkspace* ws) {
    if (!ws) return;
    for (int i = 0; i < ws->tab_count; i++) {
        tui_tab_destroy(ws->tabs[i]);
    }
    free(ws->tabs);
    ncplane_destroy(ws->ws_plane);
    free(ws);
}

void tui_workspace_add_tab(TuiWorkspace* ws, TuiTab* tab) {
    if (!ws || !tab) return;
    if (ws->tab_count >= ws->tab_capacity) {
        int new_capacity = ws->tab_capacity * 2;
        TuiTab** new_tabs = (TuiTab**)realloc(ws->tabs, new_capacity * sizeof(TuiTab*));
        if (!new_tabs) {
            tui_log(LOG_ERROR, "OOM en tui_workspace_add_tab (realloc)");
            return;
        }
        ws->tab_capacity = new_capacity;
        ws->tabs = new_tabs;
    }
    ws->tabs[ws->tab_count++] = tab;
    if (ws->active_tab_index == -1) {
        tui_workspace_set_active_tab(ws, ws->tab_count - 1);
    }
}

void tui_workspace_set_active_tab(TuiWorkspace* ws, int index) {
    if (index < 0 || index >= ws->tab_count) return;
    
    // Ocultación off-screen
    if (ws->active_tab_index != -1) {
        ncplane_move_yx(ws->tabs[ws->active_tab_index]->tab_plane, OFFSCREEN_Y, 0);
    }
    ws->active_tab_index = index;
    ncplane_move_yx(ws->tabs[index]->tab_plane, 2, 0); // Restaurar posición bajo el header modificado
    ncplane_move_top(ws->tabs[index]->tab_plane);
}

void tui_workspace_remove_active_tab(TuiWorkspace* ws) {
    if (!ws || ws->tab_count == 0 || ws->active_tab_index < 0) return;
    int idx = ws->active_tab_index;
    tui_tab_destroy(ws->tabs[idx]);
    for (int i = idx; i < ws->tab_count - 1; i++) {
        ws->tabs[i] = ws->tabs[i + 1];
    }
    ws->tab_count--;
    ws->active_tab_index = -1;
    if (ws->tab_count > 0) {
        tui_workspace_set_active_tab(ws, ws->tab_count - 1);
    }
}

// Getters

const char* tui_workspace_get_name(const TuiWorkspace* ws) {
    return ws ? ws->name : NULL;
}

int tui_workspace_get_id(const TuiWorkspace* ws) {
    return ws ? ws->id : -1;
}

int tui_workspace_get_tab_count(const TuiWorkspace* ws) {
    return ws ? ws->tab_count : 0;
}
