#include "widget/checkbox.h"
#include "core/theme.h"
#include "core/widget.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
void tui_checkbox_ensure_registered(void);
int tui_checkbox_get_type_id(void);

static int s_checkbox_type_id = -1;

TuiCheckbox* tui_checkbox_create(struct ncplane* parent, int y, int x,
                                  const char* label, bool checked,
                                  TuiCheckboxCb cb, void* userdata) {
    if (!parent || !label) return NULL;
    TuiCheckbox* c = (TuiCheckbox*)calloc(1, sizeof(TuiCheckbox));
    if (!c) return NULL;

    int width = (int)strlen(label) + 5;  /* [X] + space + label */
    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = (unsigned)width,
    };
    c->plane = ncplane_create(parent, &opts);
    if (!c->plane) { free(c); return NULL; }

    c->label = strdup(label);
    if (!c->label) { ncplane_destroy(c->plane); free(c); return NULL; }
    c->checked = checked;
    c->focused = false;
    c->cb = cb;
    c->userdata = userdata;
    c->fg_check = THEME_FG_LOG_INFO;     /* Green check */
    c->fg_unchecked = THEME_FG_TAB_INA;   /* Gray box */
    c->fg_label = THEME_FG_DEFAULT;
    c->bg_normal = THEME_BG_DARKEST;
    c->bg_focused = THEME_BG_TEXT_FOC;

    tui_checkbox_ensure_registered();
    c->_type_id = s_checkbox_type_id;

    tui_checkbox_render(c);
    return c;
}

void tui_checkbox_destroy(TuiCheckbox* c) {
    if (!c) return;
    free(c->label);
    if (c->plane) ncplane_destroy(c->plane);
    free(c);
}

bool tui_checkbox_get_state(const TuiCheckbox* c) {
    return c ? c->checked : false;
}

void tui_checkbox_set_state(TuiCheckbox* c, bool checked) {
    if (!c) return;
    c->checked = checked;
    if (c->cb) c->cb(c->checked, c->userdata);
    tui_checkbox_render(c);
}

void tui_checkbox_set_focused(TuiCheckbox* c, bool focused) {
    if (!c) return;
    c->focused = focused;
    tui_checkbox_render(c);
}

bool tui_checkbox_handle_key(TuiCheckbox* c, uint32_t key, const struct ncinput* ni) {
    if (!c || !c->focused || ni->evtype == NCTYPE_RELEASE) return false;

    if (key == NCKEY_ENTER || key == '\n' || key == '\r' || key == ' ') {
        c->checked = !c->checked;
        if (c->cb) c->cb(c->checked, c->userdata);
        tui_checkbox_render(c);
        return true;
    }
    return false;
}

void tui_checkbox_render(TuiCheckbox* c) {
    if (!c || !c->plane) return;
    unsigned cols;
    ncplane_dim_yx(c->plane, NULL, &cols);

    unsigned bg = c->focused ? c->bg_focused : c->bg_normal;
    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, c->fg_label);
    ncchannels_set_bg_rgb(&base, bg);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(c->plane, " ", 0, base);
    ncplane_erase(c->plane);

    /* Draw checkbox box */
    ncplane_set_fg_rgb(c->plane, c->checked ? c->fg_check : c->fg_unchecked);
    ncplane_putstr_yx(c->plane, 0, 0, "[");
    ncplane_putstr_yx(c->plane, 0, 2, "]");
    if (c->checked) {
        ncplane_putchar_yx(c->plane, 0, 1, '*');
    } else {
        ncplane_putchar_yx(c->plane, 0, 1, ' ');
    }

    /* Draw label */
    ncplane_set_fg_rgb(c->plane, c->fg_label);
    ncplane_putstr_yx(c->plane, 0, 4, c->label);
}

bool tui_checkbox_handle_mouse(TuiCheckbox* c, uint32_t key, const struct ncinput* ni) {
    if (!c || !ni || !c->plane || ni->evtype == NCTYPE_RELEASE) return false;
    if (key != NCKEY_BUTTON1) return false;

    /* Check if click is within this checkbox's plane */
    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(c->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(c->plane, &rows, &cols);

    int local_y = ni->y - plane_abs_y;
    int local_x = ni->x - plane_abs_x;

    if (local_y < 0 || local_y >= (int)rows || local_x < 0 || local_x >= (int)cols) return false;

    /* Toggle checkbox */
    c->checked = !c->checked;
    if (c->cb) c->cb(c->checked, c->userdata);
    tui_checkbox_render(c);
    return true;
}

/* === VTable Implementation === */

static bool v_checkbox_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiCheckbox* c = (TuiCheckbox*)widget;
    return tui_checkbox_handle_key(c, key, ni);
}

static bool v_checkbox_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiCheckbox* c = (TuiCheckbox*)widget;
    return tui_checkbox_handle_mouse(c, key, ni);
}

static void v_checkbox_render(void* widget) {
    tui_checkbox_render((TuiCheckbox*)widget);
}

static bool v_checkbox_is_focusable(void* widget) { (void)widget; return true; }

static void v_checkbox_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)max_h; (void)max_w;
    TuiCheckbox* c = (TuiCheckbox*)widget;
    if (out_h) *out_h = 1;
    if (out_w) *out_w = c->label ? (int)strlen(c->label) + 4 : 6;
}

static const TuiWidgetIface s_checkbox_iface = {
    .handle_key = v_checkbox_handle_key,
    .handle_mouse = v_checkbox_handle_mouse,
    .render = v_checkbox_render,
    .is_focusable = v_checkbox_is_focusable,
    .preferred_size = v_checkbox_preferred_size,
};

void tui_checkbox_ensure_registered(void) {
    if (s_checkbox_type_id < 0) {
        s_checkbox_type_id = tui_widget_register(&s_checkbox_iface);
    }
}

int tui_checkbox_get_type_id(void) {
    tui_checkbox_ensure_registered();
    return s_checkbox_type_id;
}
