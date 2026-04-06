#include "core/window.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/tab.h"
#include "core/theme.h"
#include <stdlib.h>

TuiWindow* tui_window_create(TuiTab* tab, int width, int height) {
    if (!tab) {
        tui_log(LOG_ERROR, "tui_window_create: tab is NULL");
        return NULL;
    }
    TuiWindow* win = (TuiWindow*)calloc(1, sizeof(TuiWindow));
    if (!win) {
        tui_log(LOG_ERROR, "OOM en tui_window_create");
        return NULL;
    }
    win->id = tui_next_id();
    
    struct ncplane_options opts = {
        .y = WINDOW_PLANE_DEF_Y, .x = WINDOW_PLANE_DEF_X,
        .rows = height, .cols = width,
        .flags = 0
    };
    win->plane = ncplane_create(tab->tab_plane, &opts);
    if (!win->plane) {
        free(win);
        tui_log(LOG_ERROR, "Fallo al crear ncplane en tui_window_create");
        return NULL;
    }
    ncplane_set_bg_rgb(win->plane, THEME_BG_DARKEST);
    win->needs_redraw = true;
    win->focused = false;
    return win;
}

void tui_window_destroy(TuiWindow* win) {
    if (!win) return;
    // Call cleanup callback BEFORE destroying plane (e.g., nullify g_active_input)
    if (win->on_destroy) {
        win->on_destroy(win);
    }
    if (win->plane) {
        ncplane_destroy(win->plane);
        win->plane = NULL;
    }
    win->text_input = NULL;
    win->render_cb = NULL;
    win->on_destroy = NULL;
    free(win);
}

// Getters

int tui_window_get_id(const TuiWindow* win) {
    return win ? win->id : -1;
}

bool tui_window_is_focused(const TuiWindow* win) {
    return win ? win->focused : false;
}
