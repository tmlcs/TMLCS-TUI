#include "core/tab.h"
#include "core/window.h"
#include <stdlib.h>
#include <string.h>

TuiTab* tui_tab_create(TuiWorkspace* ws, const char* name) {
    TuiTab* tab = (TuiTab*)calloc(1, sizeof(TuiTab));
    tab->id = s_next_id++;
    strncpy(tab->name, name, sizeof(tab->name)-1);
    
    tab->window_capacity = 4;
    tab->windows = (TuiWindow**)calloc(tab->window_capacity, sizeof(TuiWindow*));
    tab->active_window_index = -1;
    
    unsigned dimy, dimx;
    ncplane_dim_yx(ws->ws_plane, &dimy, &dimx);
    
    struct ncplane_options opts = {
        .y = -10000, .x = 0, // Oculto por defecto
        .rows = dimy - 2, .cols = dimx, // Alineado desde la línea 2
        .flags = 0
    };
    tab->tab_plane = ncplane_create(ws->ws_plane, &opts);
    return tab;
}

void tui_tab_destroy(TuiTab* tab) {
    if (!tab) return;
    for (int i = 0; i < tab->window_count; i++) {
        tui_window_destroy(tab->windows[i]);
    }
    free(tab->windows);
    ncplane_destroy(tab->tab_plane);
    free(tab);
}

void tui_tab_add_window(TuiTab* tab, TuiWindow* win) {
    if (tab->window_count >= tab->window_capacity) {
        tab->window_capacity *= 2;
        tab->windows = (TuiWindow**)realloc(tab->windows, tab->window_capacity * sizeof(TuiWindow*));
    }
    tab->windows[tab->window_count++] = win;
    tab->active_window_index = tab->window_count - 1; // Foco a la nueva
    ncplane_reparent(win->plane, tab->tab_plane); // Ascendiente del Tarea al Tab
}

void tui_tab_remove_window(TuiTab* tab, TuiWindow* win) {
    if (!tab || !win) return;
    int idx = -1;
    for(int i = 0; i < tab->window_count; i++) {
        if(tab->windows[i] == win) {
            idx = i;
            break;
        }
    }
    if (idx != -1) {
        for(int i = idx; i < tab->window_count - 1; i++) {
            tab->windows[i] = tab->windows[i+1];
        }
        tab->window_count--;
        if (tab->active_window_index == idx) {
            tab->active_window_index = (tab->window_count > 0) ? tab->window_count - 1 : -1;
        } else if (tab->active_window_index > idx) {
            tab->active_window_index--;
        }
    }
}

void tui_tab_remove_active_window(TuiTab* tab) {
    if (!tab || tab->window_count == 0 || tab->active_window_index < 0) return;
    int idx = tab->active_window_index;
    tui_window_destroy(tab->windows[idx]);
    for (int i = idx; i < tab->window_count - 1; i++) {
        tab->windows[i] = tab->windows[i + 1];
    }
    tab->window_count--;
    if (tab->window_count > 0) {
        tab->active_window_index = tab->window_count - 1; // Foco a la anterior
    } else {
        tab->active_window_index = -1;
    }
}

TuiWindow* tui_tab_get_window_at(TuiTab* tab, int abs_y, int abs_x, int* wly, int* wlx) {
    if (!tab) return NULL;
    // Invertimos porque queremos la superior en caso de solapamiento
    for (int i = tab->window_count - 1; i >= 0; i--) {
        TuiWindow* win = tab->windows[i];
        int py, px;
        unsigned dimy, dimx;
        ncplane_abs_yx(win->plane, &py, &px);
        ncplane_dim_yx(win->plane, &dimy, &dimx);
        if (abs_y >= py && abs_y < py + (int)dimy && abs_x >= px && abs_x < px + (int)dimx) {
            if (wly) *wly = abs_y - py;
            if (wlx) *wlx = abs_x - px;
            return win;
        }
    }
    return NULL;
}
