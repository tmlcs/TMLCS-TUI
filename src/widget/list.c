#include "widget/list.h"
#include "core/theme.h"
#include <stdlib.h>
#include <string.h>

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
