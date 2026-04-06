#include "widget/tab_container.h"
#include "core/theme.h"
#include "core/widget.h"
#include <stdlib.h>
#include <string.h>

void tui_tab_container_ensure_registered(void);
int tui_tab_container_get_type_id(void);

static int s_tab_container_type_id = -1;

static void ensure_capacity(TuiTabContainer* container) {
    if (container->tab_count >= container->tab_capacity) {
        int new_cap = container->tab_capacity * 2;
        char** new_labels = (char**)realloc(container->tab_labels, (size_t)new_cap * sizeof(char*));
        if (!new_labels) return;
        container->tab_labels = new_labels;
        container->tab_capacity = new_cap;
    }
}

TuiTabContainer* tui_tab_container_create(struct ncplane* parent, int y, int x,
                                           int width, int height,
                                           TuiTabContainerCb cb, void* userdata) {
    if (!parent || width < 10 || height < 3) return NULL;
    TuiTabContainer* container = (TuiTabContainer*)calloc(1, sizeof(TuiTabContainer));
    if (!container) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = (unsigned)height, .cols = (unsigned)width,
    };
    container->plane = ncplane_create(parent, &opts);
    if (!container->plane) { free(container); return NULL; }

    container->tab_capacity = 8;
    container->tab_labels = (char**)calloc((size_t)container->tab_capacity, sizeof(char*));
    if (!container->tab_labels) { ncplane_destroy(container->plane); free(container); return NULL; }
    container->tab_count = 0;
    container->active_tab = -1;
    container->width = width;
    container->height = height;
    container->focused = false;
    container->cb = cb;
    container->userdata = userdata;
    container->fg_active = THEME_FG_TAB_ACTIVE;
    container->fg_inactive = THEME_FG_TAB_INA;
    container->fg_separator = THEME_FG_SEPARATOR;
    container->bg_normal = THEME_BG_DARKEST;
    container->bg_active = THEME_BG_TAB_ACTIVE;

    tui_tab_container_ensure_registered();
    container->_type_id = s_tab_container_type_id;

    tui_tab_container_render(container);
    return container;
}

void tui_tab_container_destroy(TuiTabContainer* container) {
    if (!container) return;
    for (int i = 0; i < container->tab_count; i++) free(container->tab_labels[i]);
    free(container->tab_labels);
    if (container->plane) ncplane_destroy(container->plane);
    free(container);
}

bool tui_tab_container_add_tab(TuiTabContainer* container, const char* label) {
    if (!container || !label) return false;
    ensure_capacity(container);
    if (container->tab_count >= container->tab_capacity) return false;
    container->tab_labels[container->tab_count] = strdup(label);
    if (!container->tab_labels[container->tab_count]) return false;
    if (container->active_tab == -1) container->active_tab = container->tab_count;
    container->tab_count++;
    tui_tab_container_render(container);
    return true;
}

int tui_tab_container_get_active(const TuiTabContainer* container) {
    return container ? container->active_tab : -1;
}

void tui_tab_container_set_active(TuiTabContainer* container, int index) {
    if (!container) return;
    if (container->tab_count == 0) { container->active_tab = -1; return; }
    if (index < 0) index = 0;
    if (index >= container->tab_count) index = container->tab_count - 1;
    container->active_tab = index;
    tui_tab_container_render(container);
    if (container->cb) container->cb(container->active_tab, container->userdata);
}

void tui_tab_container_set_focused(TuiTabContainer* container, bool focused) {
    if (!container) return;
    container->focused = focused;
    tui_tab_container_render(container);
}

int tui_tab_container_get_count(const TuiTabContainer* container) {
    return container ? container->tab_count : 0;
}

bool tui_tab_container_handle_key(TuiTabContainer* container, uint32_t key, const struct ncinput* ni) {
    if (!container || !ni || container->tab_count == 0) return false;

    if (key == NCKEY_LEFT) {
        int new_tab = container->active_tab - 1;
        if (new_tab < 0) new_tab = container->tab_count - 1;
        tui_tab_container_set_active(container, new_tab);
        return true;
    }
    if (key == NCKEY_RIGHT) {
        int new_tab = container->active_tab + 1;
        if (new_tab >= container->tab_count) new_tab = 0;
        tui_tab_container_set_active(container, new_tab);
        return true;
    }
    return false;
}

bool tui_tab_container_handle_mouse(TuiTabContainer* container, uint32_t key, const struct ncinput* ni) {
    if (!container || !ni || !container->plane) return false;
    if (key != NCKEY_BUTTON1) return false;

    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(container->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(container->plane, &rows, &cols);
    (void)rows;

    int local_y = ni->y - plane_abs_y;
    if (local_y != 0) return false; /* Only tab bar row */

    int local_x = ni->x - plane_abs_x;
    int cur_x = 0;
    for (int i = 0; i < container->tab_count; i++) {
        int label_len = (int)strlen(container->tab_labels[i]);
        int tab_w = label_len + 3; /* "[Label]" */
        if (local_x >= cur_x && local_x < cur_x + tab_w) {
            tui_tab_container_set_active(container, i);
            return true;
        }
        cur_x += tab_w + 1;
    }
    return false;
}

void tui_tab_container_render(TuiTabContainer* container) {
    if (!container || !container->plane) return;
    unsigned cols;
    ncplane_dim_yx(container->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, container->fg_inactive);
    ncchannels_set_bg_rgb(&base, container->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(container->plane, " ", 0, base);
    ncplane_erase(container->plane);

    /* Draw tab bar on row 0 */
    int cur_x = 0;
    for (int i = 0; i < container->tab_count; i++) {
        bool is_active = (i == container->active_tab);
        const char* label = container->tab_labels[i];
        int label_len = (int)strlen(label);

        if (cur_x + label_len + 3 > (int)cols) break;

        if (is_active) {
            ncplane_set_fg_rgb(container->plane, container->fg_active);
            ncplane_putstr_yx(container->plane, 0, cur_x, "[");
            ncplane_putstr_yx(container->plane, 0, cur_x + 1, label);
            ncplane_putstr_yx(container->plane, 0, cur_x + 1 + label_len, "]");
        } else {
            ncplane_set_fg_rgb(container->plane, container->fg_inactive);
            ncplane_putstr_yx(container->plane, 0, cur_x, " ");
            ncplane_putstr_yx(container->plane, 0, cur_x + 1, label);
            ncplane_putstr_yx(container->plane, 0, cur_x + 1 + label_len, " ");
        }
        cur_x += label_len + 3;
    }

    /* Draw separator line on row 1 */
    ncplane_set_fg_rgb(container->plane, container->fg_separator);
    for (unsigned i = 0; i < cols; i++) {
        ncplane_putstr_yx(container->plane, 1, (int)i, "-");
    }

    /* Draw content area from row 2 onward */
    if (container->active_tab >= 0 && container->height > 2) {
        ncplane_set_fg_rgb(container->plane, container->fg_inactive);
        const char* label = container->tab_labels[container->active_tab];
        char buf[128];
        int avail = (int)cols - 2;
        if (avail > 127) avail = 127;
        int len = (int)strlen(label);
        int copy = len < avail ? len : avail;
        memcpy(buf, label, (size_t)copy);
        buf[copy] = '\0';
        int max_row = (int)(container->height > 0 ? container->height : 3);
        if (max_row > 24) max_row = 24;
        for (int row = 2; row < max_row; row++) {
            ncplane_putstr_yx(container->plane, row, 1, buf);
            break;
        }
    }
}

/* === VTable === */

static bool v_tc_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_tab_container_handle_key((TuiTabContainer*)widget, key, ni);
}

static bool v_tc_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_tab_container_handle_mouse((TuiTabContainer*)widget, key, ni);
}

static void v_tc_render(void* widget) {
    tui_tab_container_render((TuiTabContainer*)widget);
}

static bool v_tc_is_focusable(void* widget) { (void)widget; return true; }

static void v_tc_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)widget; (void)max_h; (void)max_w;
    if (out_h) *out_h = 3;
    if (out_w) *out_w = 20;
}

static const TuiWidgetIface s_tab_container_iface = {
    .handle_key = v_tc_handle_key,
    .handle_mouse = v_tc_handle_mouse,
    .render = v_tc_render,
    .is_focusable = v_tc_is_focusable,
    .preferred_size = v_tc_preferred_size,
};

void tui_tab_container_ensure_registered(void) {
    if (s_tab_container_type_id < 0) {
        s_tab_container_type_id = tui_widget_register(&s_tab_container_iface);
    }
}

int tui_tab_container_get_type_id(void) {
    tui_tab_container_ensure_registered();
    return s_tab_container_type_id;
}
