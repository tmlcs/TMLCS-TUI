#include "core/window.h"
#include "core/layout.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/tab.h"
#include "core/theme.h"
#include "core/types_private.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/*  Dynamic widget registry helper                                    */
/* ------------------------------------------------------------------ */

#define WIDGET_INITIAL_CAPACITY 8

static void ensure_widget_capacity(TuiWindow* win) {
    if (win->_widget_count < win->_widget_capacity) return;

    win->_widget_capacity = win->_widget_capacity
        ? win->_widget_capacity * 2
        : WIDGET_INITIAL_CAPACITY;

    win->_widgets = (void**)realloc(win->_widgets,
        (size_t)win->_widget_capacity * sizeof(void*));
    win->_widget_type_ids = (int*)realloc(win->_widget_type_ids,
        (size_t)win->_widget_capacity * sizeof(int));
    win->_widget_planes = (struct ncplane**)realloc(win->_widget_planes,
        (size_t)win->_widget_capacity * sizeof(struct ncplane*));
    win->_widget_focusable = (bool*)realloc(win->_widget_focusable,
        (size_t)win->_widget_capacity * sizeof(bool));

    if (!win->_widgets || !win->_widget_type_ids ||
        !win->_widget_planes || !win->_widget_focusable) {
        tui_log(LOG_ERROR, "OOM in ensure_widget_capacity");
    }
}

TuiWindow* tui_window_create(TuiTab* tab, int width, int height) {
    if (!tab) {
        tui_log(LOG_ERROR, "tui_window_create: tab is NULL");
        return NULL;
    }
    TuiWindow* win = (TuiWindow*)calloc(1, sizeof(TuiWindow));
    if (!win) {
        tui_log(LOG_ERROR, "OOM in tui_window_create");
        return NULL;
    }
    win->_id = tui_next_id();

    struct ncplane_options opts = {
        .y = WINDOW_PLANE_DEF_Y, .x = WINDOW_PLANE_DEF_X,
        .rows = height, .cols = width,
        .flags = 0
    };
    win->_plane = ncplane_create(tab->_tab_plane, &opts);
    if (!win->_plane) {
        free(win);
        tui_log(LOG_ERROR, "Failed to create ncplane in tui_window_create");
        return NULL;
    }
    ncplane_set_bg_rgb(win->_plane, THEME_BG_DARKEST);
    win->_needs_redraw = true;
    win->_focused = false;
    return win;
}

void tui_window_destroy(TuiWindow* win) {
    if (!win) return;
    /* Call cleanup callback BEFORE destroying plane (e.g., nullify g_active_input) */
    if (win->_on_destroy) {
        win->_on_destroy(win);
    }
    if (win->_plane) {
        ncplane_destroy(win->_plane);
        win->_plane = NULL;
    }
    win->_text_input = NULL;
    win->_render_cb = NULL;
    win->_on_destroy = NULL;
    free(win->_widgets);
    free(win->_widget_type_ids);
    free(win->_widget_planes);
    free(win->_widget_focusable);
    win->_widgets = NULL;
    win->_widget_type_ids = NULL;
    win->_widget_planes = NULL;
    win->_widget_focusable = NULL;
    win->_widget_count = 0;
    win->_widget_capacity = 0;
    free(win);
}

/* Getters */

int tui_window_get_id(const TuiWindow* win) {
    return win ? win->_id : -1;
}

bool tui_window_is_focused(const TuiWindow* win) {
    return win ? win->_focused : false;
}

struct ncplane* tui_window_get_plane(const TuiWindow* win) {
    return win ? win->_plane : NULL;
}

void* tui_window_get_user_data(const TuiWindow* win) {
    return win ? win->_user_data : NULL;
}

void tui_window_set_user_data(TuiWindow* win, void* data) {
    if (win) win->_user_data = data;
}

void tui_window_mark_dirty(TuiWindow* win) {
    if (win) win->_needs_redraw = true;
}

int tui_window_get_focused_widget_index(const TuiWindow* win) {
    return win ? win->_focused_widget_index : -1;
}

void tui_window_set_focused_widget_index(TuiWindow* win, int index) {
    if (win) win->_focused_widget_index = index;
}

/* ------------------------------------------------------------------ */
/*  Widget registry                                                    */
/* ------------------------------------------------------------------ */

void tui_window_add_widget(TuiWindow* win, void* widget, struct ncplane* plane, int type_id) {
    if (!win || !widget || !plane) return;
    ensure_widget_capacity(win);
    if (win->_widget_count >= win->_widget_capacity) {
        tui_log(LOG_ERROR, "tui_window_add_widget: failed to allocate widget storage");
        return;
    }
    int widx = win->_widget_count;
    win->_widgets[widx] = widget;
    win->_widget_planes[widx] = plane;
    win->_widget_type_ids[widx] = type_id;
    win->_widget_focusable[widx] = false;
    win->_widget_count++;
}

void* tui_window_get_widget_at(TuiWindow* win, int local_y, int local_x, int* out_type) {
    if (!win) return NULL;

    /* Iterate in reverse order (z-order: last added = on top) */
    for (int i = win->_widget_count - 1; i >= 0; i--) {
        void* widget = win->_widgets[i];
        struct ncplane* plane = win->_widget_planes[i];
        int type_id = win->_widget_type_ids[i];
        if (!widget || !plane) continue;

        /* Check if the click falls within this widget's plane */
        int w_abs_y, w_abs_x;
        ncplane_abs_yx(plane, &w_abs_y, &w_abs_x);
        unsigned w_rows, w_cols;
        ncplane_dim_yx(plane, &w_rows, &w_cols);

        /* local_y and local_x are relative to window; need absolute click coords */
        int p_abs_y, p_abs_x;
        ncplane_abs_yx(win->_plane, &p_abs_y, &p_abs_x);
        int click_abs_y = p_abs_y + local_y;
        int click_abs_x = p_abs_x + local_x;

        int w_local_y = click_abs_y - w_abs_y;
        int w_local_x = click_abs_x - w_abs_x;

        if (w_local_y >= 0 && w_local_y < (int)w_rows && w_local_x >= 0 && w_local_x < (int)w_cols) {
            if (out_type) *out_type = type_id;
            return widget;
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Layout attachment                                                  */
/* ------------------------------------------------------------------ */

void tui_window_attach_layout(TuiWindow* win, TuiLayout* layout) {
    if (!win) return;
    win->_attached_layout = layout;
    tui_window_mark_dirty(win);
}

TuiLayout* tui_window_get_layout(const TuiWindow* win) {
    return win ? win->_attached_layout : NULL;
}
