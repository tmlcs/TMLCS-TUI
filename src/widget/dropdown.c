#include "widget/dropdown.h"
#include "core/theme.h"
#include "core/widget.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

void tui_dropdown_ensure_registered(void);
int tui_dropdown_get_type_id(void);

static int s_dropdown_type_id = -1;

static void ensure_capacity(TuiDropdown* dropdown) {
    if (dropdown->count >= dropdown->capacity) {
        int new_cap = dropdown->capacity * 2;
        char** new_items = (char**)realloc(dropdown->items, (size_t)new_cap * sizeof(char*));
        if (!new_items) {
            tui_log(LOG_ERROR, "OOM in dropdown ensure_capacity");
            return;
        }
        dropdown->items = new_items;
        dropdown->capacity = new_cap;
    }
}

TuiDropdown* tui_dropdown_create(struct ncplane* parent, int y, int x, int width,
                                  TuiDropdownCb cb, void* userdata) {
    if (!parent || width < 4) return NULL;
    TuiDropdown* dropdown = (TuiDropdown*)calloc(1, sizeof(TuiDropdown));
    if (!dropdown) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = (unsigned)width,
    };
    dropdown->plane = ncplane_create(parent, &opts);
    if (!dropdown->plane) { free(dropdown); return NULL; }

    dropdown->popup_plane = NULL;
    dropdown->capacity = 8;
    dropdown->items = (char**)calloc((size_t)dropdown->capacity, sizeof(char*));
    if (!dropdown->items) { ncplane_destroy(dropdown->plane); free(dropdown); return NULL; }
    dropdown->count = 0;
    dropdown->selected = -1;
    dropdown->width = width;
    dropdown->focused = false;
    dropdown->open = false;
    dropdown->cb = cb;
    dropdown->userdata = userdata;
    dropdown->fg_text = THEME_FG_DEFAULT;
    dropdown->fg_selected = THEME_FG_TAB_ACTIVE;
    dropdown->bg_normal = THEME_BG_DARKEST;
    dropdown->bg_selected = THEME_BG_TAB_ACTIVE;
    dropdown->bg_popup = THEME_BG_DARKEST;

    tui_dropdown_ensure_registered();
    dropdown->_type_id = s_dropdown_type_id;

    tui_dropdown_render(dropdown);
    return dropdown;
}

void tui_dropdown_destroy(TuiDropdown* dropdown) {
    if (!dropdown) return;
    for (int i = 0; i < dropdown->count; i++) free(dropdown->items[i]);
    free(dropdown->items);
    if (dropdown->popup_plane) ncplane_destroy(dropdown->popup_plane);
    if (dropdown->plane) ncplane_destroy(dropdown->plane);
    free(dropdown);
}

bool tui_dropdown_add_item(TuiDropdown* dropdown, const char* item) {
    if (!dropdown || !item) return false;
    ensure_capacity(dropdown);
    if (dropdown->count >= dropdown->capacity) return false;
    dropdown->items[dropdown->count] = strdup(item);
    if (!dropdown->items[dropdown->count]) return false;
    if (dropdown->selected == -1) dropdown->selected = dropdown->count;
    dropdown->count++;
    tui_dropdown_render(dropdown);
    return true;
}

int tui_dropdown_get_selected(const TuiDropdown* dropdown) {
    return dropdown ? dropdown->selected : -1;
}

void tui_dropdown_set_selected(TuiDropdown* dropdown, int index) {
    if (!dropdown) return;
    if (index < 0) index = 0;
    if (index >= dropdown->count) index = dropdown->count > 0 ? dropdown->count - 1 : -1;
    dropdown->selected = index;
    tui_dropdown_render(dropdown);
}

void tui_dropdown_set_focused(TuiDropdown* dropdown, bool focused) {
    if (!dropdown) return;
    dropdown->focused = focused;
    tui_dropdown_render(dropdown);
}

int tui_dropdown_get_count(const TuiDropdown* dropdown) {
    return dropdown ? dropdown->count : 0;
}

bool tui_dropdown_is_open(const TuiDropdown* dropdown) {
    return dropdown ? dropdown->open : false;
}

static void close_popup(TuiDropdown* dropdown) {
    if (!dropdown) return;
    dropdown->open = false;
    if (dropdown->popup_plane) {
        ncplane_destroy(dropdown->popup_plane);
        dropdown->popup_plane = NULL;
    }
    tui_dropdown_render(dropdown);
}

bool tui_dropdown_handle_key(TuiDropdown* dropdown, uint32_t key, const struct ncinput* ni) {
    if (!dropdown || !ni) return false;

    if (dropdown->open) {
        if (key == NCKEY_ESC) {
            close_popup(dropdown);
            return true;
        }
        if (key == NCKEY_UP) {
            if (dropdown->selected > 0) {
                dropdown->selected--;
                tui_dropdown_render(dropdown);
            }
            return true;
        }
        if (key == NCKEY_DOWN) {
            if (dropdown->selected < dropdown->count - 1) {
                dropdown->selected++;
                tui_dropdown_render(dropdown);
            }
            return true;
        }
        if (key == NCKEY_ENTER || key == ' ') {
            if (dropdown->selected >= 0 && dropdown->selected < dropdown->count) {
                if (dropdown->cb) dropdown->cb(dropdown->selected, dropdown->userdata);
            }
            close_popup(dropdown);
            return true;
        }
    } else {
        if (key == NCKEY_ENTER || key == ' ' || key == NCKEY_DOWN) {
            if (dropdown->count > 0) {
                dropdown->open = true;
                tui_dropdown_render(dropdown);
                return true;
            }
        }
    }
    return false;
}

bool tui_dropdown_handle_mouse(TuiDropdown* dropdown, uint32_t key, const struct ncinput* ni) {
    if (!dropdown || !ni || !dropdown->plane) return false;
    if (key != NCKEY_BUTTON1) return false;

    /* Check if click is on the dropdown itself */
    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(dropdown->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(dropdown->plane, &rows, &cols);

    int local_y = ni->y - plane_abs_y;
    int local_x = ni->x - plane_abs_x;

    if (dropdown->open && dropdown->popup_plane) {
        int popup_abs_y, popup_abs_x;
        ncplane_abs_yx(dropdown->popup_plane, &popup_abs_y, &popup_abs_x);
        unsigned p_rows, p_cols;
        ncplane_dim_yx(dropdown->popup_plane, &p_rows, &p_cols);

        int popup_local_y = ni->y - popup_abs_y;
        int popup_local_x = ni->x - popup_abs_x;

        if (popup_local_y >= 0 && popup_local_y < (int)p_rows &&
            popup_local_x >= 0 && popup_local_x < (int)p_cols) {
            /* Click on popup - select item */
            if (popup_local_y >= 0 && popup_local_y < dropdown->count) {
                dropdown->selected = popup_local_y;
                if (dropdown->cb) dropdown->cb(dropdown->selected, dropdown->userdata);
                close_popup(dropdown);
                return true;
            }
        }
    }

    if (local_y >= 0 && local_y < (int)rows && local_x >= 0 && local_x < (int)cols) {
        if (!dropdown->open) {
            dropdown->open = true;
            tui_dropdown_render(dropdown);
            return true;
        } else {
            close_popup(dropdown);
            return true;
        }
    }
    return false;
}

void tui_dropdown_render(TuiDropdown* dropdown) {
    if (!dropdown || !dropdown->plane) return;
    unsigned cols;
    ncplane_dim_yx(dropdown->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, dropdown->fg_text);
    ncchannels_set_bg_rgb(&base, dropdown->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(dropdown->plane, " ", 0, base);
    ncplane_erase(dropdown->plane);

    /* Draw current selection or placeholder */
    const char* display = dropdown->selected >= 0 && dropdown->selected < dropdown->count
                          ? dropdown->items[dropdown->selected] : "[None]";

    /* Draw brackets */
    ncplane_putstr_yx(dropdown->plane, 0, 0, "[");
    ncplane_set_fg_rgb(dropdown->plane, dropdown->fg_text);
    int max_text = (int)cols - 4; /* "[", " ", "v", "]" */
    if (max_text < 1) max_text = 1;
    int len = (int)strlen(display);
    int copy = len < max_text ? len : max_text;
    char buf[256];
    memcpy(buf, display, (size_t)copy);
    buf[copy] = '\0';
    ncplane_putstr_yx(dropdown->plane, 0, 1, buf);

    /* Draw dropdown arrow */
    ncplane_set_fg_rgb(dropdown->plane, dropdown->fg_text);
    ncplane_putstr_yx(dropdown->plane, 0, (int)cols - 2, "v");
    ncplane_putstr_yx(dropdown->plane, 0, (int)cols - 1, "]");

    /* Draw popup if open */
    if (dropdown->open && !dropdown->popup_plane) {
        int abs_y, abs_x;
        ncplane_abs_yx(dropdown->plane, &abs_y, &abs_x);
        struct ncplane_options opts = {
            .y = 1, .x = 0,
            .rows = (unsigned)dropdown->count, .cols = cols,
        };
        dropdown->popup_plane = ncplane_create(dropdown->plane, &opts);
    }

    if (dropdown->open && dropdown->popup_plane) {
        uint64_t pbase = 0;
        ncchannels_set_fg_rgb(&pbase, dropdown->fg_text);
        ncchannels_set_bg_rgb(&pbase, dropdown->bg_popup);
        ncchannels_set_bg_alpha(&pbase, NCALPHA_OPAQUE);
        ncplane_set_base(dropdown->popup_plane, " ", 0, pbase);
        ncplane_erase(dropdown->popup_plane);

        for (int i = 0; i < dropdown->count; i++) {
            bool is_selected = (i == dropdown->selected);
            if (is_selected) {
                ncplane_set_fg_rgb(dropdown->popup_plane, dropdown->fg_selected);
                ncplane_set_bg_rgb(dropdown->popup_plane, dropdown->bg_selected);
            } else {
                ncplane_set_fg_rgb(dropdown->popup_plane, dropdown->fg_text);
                ncplane_set_bg_rgb(dropdown->popup_plane, dropdown->bg_popup);
            }
            ncplane_putstr_yx(dropdown->popup_plane, i, 1, dropdown->items[i]);
        }
    }
}

/* === VTable === */

static bool v_dd_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_dropdown_handle_key((TuiDropdown*)widget, key, ni);
}

static bool v_dd_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_dropdown_handle_mouse((TuiDropdown*)widget, key, ni);
}

static void v_dd_render(void* widget) {
    tui_dropdown_render((TuiDropdown*)widget);
}

static bool v_dd_is_focusable(void* widget) { (void)widget; return true; }

static void v_dd_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)max_h; (void)max_w;
    TuiDropdown* dd = (TuiDropdown*)widget;
    if (out_h) *out_h = dd->open ? dd->count + 1 : 1;
    if (out_w) *out_w = dd->width;
}

static const TuiWidgetIface s_dropdown_iface = {
    .handle_key = v_dd_handle_key,
    .handle_mouse = v_dd_handle_mouse,
    .render = v_dd_render,
    .is_focusable = v_dd_is_focusable,
    .preferred_size = v_dd_preferred_size,
};

void tui_dropdown_ensure_registered(void) {
    if (s_dropdown_type_id < 0) {
        s_dropdown_type_id = tui_widget_register(&s_dropdown_iface);
    }
}

int tui_dropdown_get_type_id(void) {
    tui_dropdown_ensure_registered();
    return s_dropdown_type_id;
}
