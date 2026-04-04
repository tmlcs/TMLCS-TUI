#include "core/workspace.h"
#include "core/tab.h"
#include <stdlib.h>
#include <string.h>

TuiWorkspace* tui_workspace_create(TuiManager* manager, const char* name) {
    TuiWorkspace* ws = (TuiWorkspace*)calloc(1, sizeof(TuiWorkspace));
    ws->id = s_next_id++;
    strncpy(ws->name, name, sizeof(ws->name)-1);
    
    ws->tab_capacity = 4;
    ws->tabs = (TuiTab**)calloc(ws->tab_capacity, sizeof(TuiTab*));
    ws->active_tab_index = -1;
    
    unsigned dimy, dimx;
    ncplane_dim_yx(manager->stdplane, &dimy, &dimx);
    
    struct ncplane_options opts = {
        .y = -10000, .x = 0, // Oculto por defecto
        .rows = dimy - 1, .cols = dimx, // Se reserva la última fila inferior
        .flags = 0
    };
    ws->ws_plane = ncplane_create(manager->stdplane, &opts);
    
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
    if (ws->tab_count >= ws->tab_capacity) {
        ws->tab_capacity *= 2;
        ws->tabs = (TuiTab**)realloc(ws->tabs, ws->tab_capacity * sizeof(TuiTab*));
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
        ncplane_move_yx(ws->tabs[ws->active_tab_index]->tab_plane, -10000, 0);
    }
    ws->active_tab_index = index;
    ncplane_move_yx(ws->tabs[index]->tab_plane, 3, 0); // Restaurar posición (después del header global del main)
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
