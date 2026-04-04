#include "core/window.h"
#include <stdlib.h>

TuiWindow* tui_window_create(TuiTab* tab, int width, int height) {
    TuiWindow* win = (TuiWindow*)calloc(1, sizeof(TuiWindow));
    win->id = s_next_id++;
    
    struct ncplane_options opts = {
        .y = 0, .x = 0,
        .rows = height, .cols = width,
        .flags = 0
    };
    win->plane = ncplane_create(tab->tab_plane, &opts);
    ncplane_set_bg_rgb(win->plane, 0x111111); // Fondo por defecto
    return win;
}

void tui_window_destroy(TuiWindow* win) {
    if (!win) return;
    ncplane_destroy(win->plane);
    free(win);
}
