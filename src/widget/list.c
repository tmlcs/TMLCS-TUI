#include "widget/list.h"
#include "core/theme.h"
#include "core/widget.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
static void ensure_capacity(TuiList* list);
void tui_list_ensure_registered(void);
int tui_list_get_type_id(void);

static int s_list_type_id = -1;

TuiList* tui_list_create(struct ncplane* parent, int y, int x, int width, int height) {
    if (!parent || height < 1 || width < 4) return NULL;
    TuiList* list = (TuiList*)calloc(1, sizeof(TuiList));
    if (!list) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = (unsigned)height, .cols = (unsigned)width,
    };
    list->plane = ncplane_create(parent, &opts);
    if (!list->plane) { free(list); return NULL; }

    list->capacity = 8;
    list->items = (char**)calloc((size_t)list->capacity, sizeof(char*));
    list->count = 0;
    list->selected = -1;
    list->scroll_off = 0;
    list->height = height;
    list->focused = false;
    list->bg_normal = THEME_BG_DARKEST;
    list->bg_selected = THEME_BG_TAB_ACTIVE;
    list->fg_text = THEME_FG_DEFAULT;
    list->fg_selected = THEME_FG_TAB_ACTIVE;

    tui_list_ensure_registered();
    list->_type_id = s_list_type_id;

    tui_list_render(list);
    return list;
}

void tui_list_destroy(TuiList* list) {
    if (!list) return;
    tui_list_clear(list);
    free(list->items);
    if (list->plane) ncplane_destroy(list->plane);
    free(list);
}

static void ensure_capacity(TuiList* list) {
    if (list->count >= list->capacity) {
        int new_cap = list->capacity * 2;
        char** new_items = (char**)realloc(list->items, (size_t)new_cap * sizeof(char*));
        if (!new_items) return;
        list->items = new_items;
        list->capacity = new_cap;
    }
}

void tui_list_add_item(TuiList* list, const char* item) {
    if (!list || !item) return;
    ensure_capacity(list);
    list->items[list->count] = strdup(item);
    if (list->selected == -1) list->selected = 0;
    list->count++;
    tui_list_render(list);
}

void tui_list_remove_item(TuiList* list, int index) {
    if (!list || index < 0 || index >= list->count) return;
    free(list->items[index]);
    for (int i = index; i < list->count - 1; i++) {
        list->items[i] = list->items[i + 1];
    }
    list->count--;
    if (list->selected >= list->count) list->selected = list->count - 1;
    if (list->selected < 0) list->selected = -1;
    tui_list_render(list);
}

void tui_list_clear(TuiList* list) {
    if (!list) return;
    for (int i = 0; i < list->count; i++) free(list->items[i]);
    list->count = 0;
    list->selected = -1;
    list->scroll_off = 0;
    tui_list_render(list);
}

int tui_list_get_selected(const TuiList* list) {
    return list ? list->selected : -1;
}

void tui_list_set_selected(TuiList* list, int index) {
    if (!list) return;
    if (index < 0) index = 0;
    if (index >= list->count) index = list->count > 0 ? list->count - 1 : -1;
    list->selected = index;

    /* Adjust scroll to keep selection visible */
    if (list->selected < list->scroll_off) {
        list->scroll_off = list->selected;
    } else if (list->selected >= list->scroll_off + list->height) {
        list->scroll_off = list->selected - list->height + 1;
    }
    tui_list_render(list);
}

int tui_list_get_count(const TuiList* list) {
    return list ? list->count : 0;
}

void tui_list_set_focused(TuiList* list, bool focused) {
    if (!list) return;
    list->focused = focused;
    tui_list_render(list);
}

bool tui_list_handle_key(TuiList* list, uint32_t key, const struct ncinput* ni) {
    if (!list || !list->focused || ni->evtype == NCTYPE_RELEASE) return false;

    if (key == NCKEY_UP) {
        if (list->selected > 0) tui_list_set_selected(list, list->selected - 1);
        return true;
    }
    if (key == NCKEY_DOWN) {
        if (list->selected < list->count - 1) tui_list_set_selected(list, list->selected + 1);
        return true;
    }
    return false;
}

void tui_list_render(TuiList* list) {
    if (!list || !list->plane) return;
    unsigned cols;
    ncplane_dim_yx(list->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, list->fg_text);
    ncchannels_set_bg_rgb(&base, list->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(list->plane, " ", 0, base);
    ncplane_erase(list->plane);

    int visible = list->height;
    for (int row = 0; row < visible; row++) {
        int idx = list->scroll_off + row;
        if (idx >= list->count) break;

        bool is_selected = (idx == list->selected);
        if (is_selected) {
            ncplane_set_fg_rgb(list->plane, list->fg_selected);
            ncplane_set_bg_rgb(list->plane, list->bg_selected);
        } else {
            ncplane_set_fg_rgb(list->plane, list->fg_text);
            ncplane_set_bg_rgb(list->plane, list->bg_normal);
        }

        /* Draw border for selected */
        if (is_selected) {
            ncplane_putchar_yx(list->plane, row, 0, '>');
            ncplane_putchar_yx(list->plane, row, (int)cols - 1, '<');
        }

        /* Draw item text */
        int max_text = (int)cols - 2;
        if (max_text > 0) {
            char buf[256];
            int len = (int)strlen(list->items[idx]);
            int copy = len < max_text ? len : max_text;
            memcpy(buf, list->items[idx], (size_t)copy);
            buf[copy] = '\0';
            ncplane_putstr_yx(list->plane, row, 1, buf);
        }
    }
}

bool tui_list_handle_mouse(TuiList* list, uint32_t key, const struct ncinput* ni) {
    if (!list || !ni || !list->plane || ni->evtype == NCTYPE_RELEASE) return false;
    if (key != NCKEY_BUTTON1) return false;

    /* Check if click is within this list's plane */
    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(list->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(list->plane, &rows, &cols);

    int local_y = ni->y - plane_abs_y;
    int local_x = ni->x - plane_abs_x;

    if (local_y < 0 || local_y >= (int)rows || local_x < 0 || local_x >= (int)cols) return false;

    /* Calculate which item was clicked */
    int clicked_item = list->scroll_off + local_y;
    if (clicked_item >= 0 && clicked_item < list->count) {
        tui_list_set_selected(list, clicked_item);
    }
    return true;
}

/* === VTable Implementation === */

static bool v_list_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiList* list = (TuiList*)widget;
    return tui_list_handle_key(list, key, ni);
}

static bool v_list_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiList* list = (TuiList*)widget;
    return tui_list_handle_mouse(list, key, ni);
}

static void v_list_render(void* widget) {
    tui_list_render((TuiList*)widget);
}

static bool v_list_is_focusable(void* widget) { (void)widget; return true; }

static void v_list_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)max_h; (void)max_w;
    TuiList* list = (TuiList*)widget;
    if (out_h) *out_h = list->count > 0 ? list->count : 1;
    if (out_w) *out_w = -1;  /* fill available width */
}

static const TuiWidgetIface s_list_iface = {
    .handle_key = v_list_handle_key,
    .handle_mouse = v_list_handle_mouse,
    .render = v_list_render,
    .is_focusable = v_list_is_focusable,
    .preferred_size = v_list_preferred_size,
};

void tui_list_ensure_registered(void) {
    if (s_list_type_id < 0) {
        s_list_type_id = tui_widget_register(&s_list_iface);
    }
}

int tui_list_get_type_id(void) {
    tui_list_ensure_registered();
    return s_list_type_id;
}
