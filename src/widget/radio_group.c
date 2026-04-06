#include "widget/radio_group.h"
#include "core/theme.h"
#include "core/widget.h"
#include <stdlib.h>
#include <string.h>

void tui_radio_group_ensure_registered(void);
int tui_radio_group_get_type_id(void);

static int s_radio_group_type_id = -1;

static void ensure_capacity(TuiRadioGroup* group) {
    if (group->count >= group->capacity) {
        int new_cap = group->capacity * 2;
        char** new_labels = (char**)realloc(group->labels, (size_t)new_cap * sizeof(char*));
        if (!new_labels) return;
        group->labels = new_labels;
        group->capacity = new_cap;
    }
}

TuiRadioGroup* tui_radio_group_create(struct ncplane* parent, int y, int x,
                                       int width, int height,
                                       TuiRadioGroupCb cb, void* userdata) {
    if (!parent || width < 4 || height < 1) return NULL;
    TuiRadioGroup* group = (TuiRadioGroup*)calloc(1, sizeof(TuiRadioGroup));
    if (!group) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = (unsigned)height, .cols = (unsigned)width,
    };
    group->plane = ncplane_create(parent, &opts);
    if (!group->plane) { free(group); return NULL; }

    group->capacity = 8;
    group->labels = (char**)calloc((size_t)group->capacity, sizeof(char*));
    if (!group->labels) { ncplane_destroy(group->plane); free(group); return NULL; }
    group->count = 0;
    group->selected = -1;
    group->height = height;
    group->width = width;
    group->focused = false;
    group->cb = cb;
    group->userdata = userdata;
    group->fg_selected = THEME_FG_TAB_ACTIVE;
    group->fg_unselected = THEME_FG_SEPARATOR;
    group->fg_label = THEME_FG_DEFAULT;
    group->bg_normal = THEME_BG_DARKEST;
    group->bg_selected = THEME_BG_TAB_ACTIVE;

    tui_radio_group_ensure_registered();
    group->_type_id = s_radio_group_type_id;

    tui_radio_group_render(group);
    return group;
}

void tui_radio_group_destroy(TuiRadioGroup* group) {
    if (!group) return;
    for (int i = 0; i < group->count; i++) free(group->labels[i]);
    free(group->labels);
    if (group->plane) ncplane_destroy(group->plane);
    free(group);
}

bool tui_radio_group_add_option(TuiRadioGroup* group, const char* label) {
    if (!group || !label) return false;
    ensure_capacity(group);
    if (group->count >= group->capacity) return false;
    group->labels[group->count] = strdup(label);
    if (!group->labels[group->count]) return false;
    if (group->selected == -1) group->selected = group->count;
    group->count++;
    tui_radio_group_render(group);
    return true;
}

int tui_radio_group_get_selected(const TuiRadioGroup* group) {
    return group ? group->selected : -1;
}

void tui_radio_group_set_selected(TuiRadioGroup* group, int index) {
    if (!group) return;
    if (index < -1) index = -1;
    if (index >= group->count) index = group->count > 0 ? group->count - 1 : -1;
    group->selected = index;
    tui_radio_group_render(group);
    if (group->cb) group->cb(group->selected, group->userdata);
}

void tui_radio_group_set_focused(TuiRadioGroup* group, bool focused) {
    if (!group) return;
    group->focused = focused;
    tui_radio_group_render(group);
}

int tui_radio_group_get_count(const TuiRadioGroup* group) {
    return group ? group->count : 0;
}

bool tui_radio_group_handle_key(TuiRadioGroup* group, uint32_t key, const struct ncinput* ni) {
    if (!group || !ni || group->count == 0) return false;

    if (key == NCKEY_UP) {
        if (group->selected > 0) {
            tui_radio_group_set_selected(group, group->selected - 1);
        } else {
            tui_radio_group_set_selected(group, group->count - 1);
        }
        return true;
    }
    if (key == NCKEY_DOWN) {
        if (group->selected < group->count - 1) {
            tui_radio_group_set_selected(group, group->selected + 1);
        } else {
            tui_radio_group_set_selected(group, 0);
        }
        return true;
    }
    if (key == ' ' || key == NCKEY_ENTER) {
        /* Space/Enter selects the current option */
        if (group->cb) group->cb(group->selected, group->userdata);
        return true;
    }
    return false;
}

bool tui_radio_group_handle_mouse(TuiRadioGroup* group, uint32_t key, const struct ncinput* ni) {
    if (!group || !ni || !group->plane) return false;
    if (key != NCKEY_BUTTON1) return false;

    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(group->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(group->plane, &rows, &cols);
    (void)cols;

    int local_y = ni->y - plane_abs_y;
    int local_x = ni->x - plane_abs_x;
    (void)local_x;
    if (local_y < 0 || local_y >= (int)rows) return false;

    int clicked_item = local_y;
    if (clicked_item >= 0 && clicked_item < group->count) {
        tui_radio_group_set_selected(group, clicked_item);
        return true;
    }
    return false;
}

void tui_radio_group_render(TuiRadioGroup* group) {
    if (!group || !group->plane) return;
    unsigned cols;
    ncplane_dim_yx(group->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, group->fg_label);
    ncchannels_set_bg_rgb(&base, group->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(group->plane, " ", 0, base);
    ncplane_erase(group->plane);

    int visible = group->height;
    if (visible > group->count) visible = group->count;

    for (int row = 0; row < visible; row++) {
        bool is_selected = (row == group->selected);

        /* Draw radio indicator */
        ncplane_set_fg_rgb(group->plane, is_selected ? group->fg_selected : group->fg_unselected);
        if (is_selected) {
            ncplane_putstr_yx(group->plane, row, 0, "(\u2022) ");
        } else {
            ncplane_putstr_yx(group->plane, row, 0, "( ) ");
        }

        /* Draw label */
        ncplane_set_fg_rgb(group->plane, group->fg_label);
        int max_text = (int)cols - 5;
        if (max_text > 0) {
            char buf[256];
            int len = (int)strlen(group->labels[row]);
            int copy = len < max_text ? len : max_text;
            memcpy(buf, group->labels[row], (size_t)copy);
            buf[copy] = '\0';
            ncplane_putstr_yx(group->plane, row, 4, buf);
        }
    }
}

/* === VTable === */

static bool v_radio_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_radio_group_handle_key((TuiRadioGroup*)widget, key, ni);
}

static bool v_radio_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_radio_group_handle_mouse((TuiRadioGroup*)widget, key, ni);
}

static void v_radio_render(void* widget) {
    tui_radio_group_render((TuiRadioGroup*)widget);
}

static bool v_radio_is_focusable(void* widget) { (void)widget; return true; }

static void v_radio_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)max_h; (void)max_w;
    TuiRadioGroup* group = (TuiRadioGroup*)widget;
    if (out_h) *out_h = group->count > 0 ? group->count : 1;
    int max_label_w = 4; /* "( ) " */
    for (int i = 0; i < group->count; i++) {
        int lw = group->labels[i] ? (int)strlen(group->labels[i]) : 0;
        if (lw + 4 > max_label_w) max_label_w = lw + 4;
    }
    if (out_w) *out_w = max_label_w;
}

static const TuiWidgetIface s_radio_group_iface = {
    .handle_key = v_radio_handle_key,
    .handle_mouse = v_radio_handle_mouse,
    .render = v_radio_render,
    .is_focusable = v_radio_is_focusable,
    .preferred_size = v_radio_preferred_size,
};

void tui_radio_group_ensure_registered(void) {
    if (s_radio_group_type_id < 0) {
        s_radio_group_type_id = tui_widget_register(&s_radio_group_iface);
    }
}

int tui_radio_group_get_type_id(void) {
    tui_radio_group_ensure_registered();
    return s_radio_group_type_id;
}
