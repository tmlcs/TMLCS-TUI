#include "core/tab.h"
#include "core/window.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/theme.h"
#include "core/types_private.h"
#include <stdlib.h>
#include <string.h>

TuiTab* tui_tab_create(TuiWorkspace* ws, const char* name) {
    if (!ws) {
        tui_log(LOG_ERROR, "tui_tab_create: ws is NULL");
        return NULL;
    }
    if (!name) {
        tui_log(LOG_ERROR, "tui_tab_create: name is NULL");
        return NULL;
    }
    TuiTab* tab = (TuiTab*)calloc(1, sizeof(TuiTab));
    if (!tab) {
        tui_log(LOG_ERROR, "OOM in tui_tab_create");
        return NULL;
    }
    tab->_id = tui_next_id();
    strncpy(tab->_name, name, sizeof(tab->_name)-1);
    tab->_name[sizeof(tab->_name)-1] = '\0';

    tab->_window_capacity = 4;
    tab->_windows = (TuiWindow**)calloc(tab->_window_capacity, sizeof(TuiWindow*));
    if (!tab->_windows) {
        free(tab);
        tui_log(LOG_ERROR, "OOM in tui_tab_create");
        return NULL;
    }
    tab->_active_window_index = -1;

    unsigned dimy, dimx;
    ncplane_dim_yx(ws->_ws_plane, &dimy, &dimx);

    struct ncplane_options opts = {
        .y = OFFSCREEN_Y, .x = 0, /* Hidden by default */
        .rows = dimy - 2, .cols = dimx,
        .flags = 0
    };
    tab->_tab_plane = ncplane_create(ws->_ws_plane, &opts);
    return tab;
}

void tui_tab_destroy(TuiTab* tab) {
    if (!tab) return;
    for (int i = 0; i < tab->_window_count; i++) {
        tui_window_destroy(tab->_windows[i]);
    }
    free(tab->_windows);
    ncplane_destroy(tab->_tab_plane);
    free(tab);
}

void tui_tab_add_window(TuiTab* tab, TuiWindow* win) {
    if (!tab || !win) return;
    if (tab->_window_count >= tab->_window_capacity) {
        int new_capacity = tab->_window_capacity * 2;
        TuiWindow** new_windows = (TuiWindow**)realloc(tab->_windows, new_capacity * sizeof(TuiWindow*));
        if (!new_windows) {
            tui_log(LOG_ERROR, "OOM in tui_tab_add_window (realloc)");
            return;
        }
        tab->_window_capacity = new_capacity;
        tab->_windows = new_windows;
    }
    tab->_windows[tab->_window_count++] = win;
    tab->_active_window_index = tab->_window_count - 1; /* Focus the new window */
    if (win->_plane) ncplane_reparent(win->_plane, tab->_tab_plane);
}

void tui_tab_remove_window(TuiTab* tab, TuiWindow* win) {
    if (!tab || !win) return;
    int idx = -1;
    for(int i = 0; i < tab->_window_count; i++) {
        if(tab->_windows[i] == win) {
            idx = i;
            break;
        }
    }
    if (idx != -1) {
        for(int i = idx; i < tab->_window_count - 1; i++) {
            tab->_windows[i] = tab->_windows[i+1];
        }
        tab->_window_count--;
        if (tab->_active_window_index == idx) {
            tab->_active_window_index = (tab->_window_count > 0) ? tab->_window_count - 1 : -1;
        } else if (tab->_active_window_index > idx) {
            tab->_active_window_index--;
        }
    }
}

void tui_tab_remove_active_window(TuiTab* tab) {
    if (!tab || tab->_window_count == 0 || tab->_active_window_index < 0) return;
    int idx = tab->_active_window_index;
    tui_window_destroy(tab->_windows[idx]);
    for (int i = idx; i < tab->_window_count - 1; i++) {
        tab->_windows[i] = tab->_windows[i + 1];
    }
    tab->_window_count--;
    if (tab->_window_count > 0) {
        tab->_active_window_index = tab->_window_count - 1; /* Focus the previous window */
    } else {
        tab->_active_window_index = -1;
    }
}

TuiWindow* tui_tab_get_window_at(TuiTab* tab, int abs_y, int abs_x, int* wly, int* wlx) {
    if (!tab) return NULL;
    /* Iterate in reverse because we want the topmost window in case of overlap */
    for (int i = tab->_window_count - 1; i >= 0; i--) {
        TuiWindow* win = tab->_windows[i];
        int py, px;
        unsigned dimy, dimx;
        ncplane_abs_yx(win->_plane, &py, &px);
        ncplane_dim_yx(win->_plane, &dimy, &dimx);
        if (abs_y >= py && abs_y < py + (int)dimy && abs_x >= px && abs_x < px + (int)dimx) {
            if (wly) *wly = abs_y - py;
            if (wlx) *wlx = abs_x - px;
            return win;
        }
    }
    return NULL;
}

/* --- Getters --- */

const char* tui_tab_get_name(const TuiTab* tab) {
    return tab ? tab->_name : NULL;
}

int tui_tab_get_id(const TuiTab* tab) {
    return tab ? tab->_id : -1;
}

int tui_tab_get_window_count(const TuiTab* tab) {
    return tab ? tab->_window_count : 0;
}
