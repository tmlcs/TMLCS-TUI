#include "widget/button.h"
#include "core/theme.h"
#include "core/widget.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
void tui_button_ensure_registered(void);
int tui_button_get_type_id(void);

static int s_button_type_id = -1;

TuiButton* tui_button_create(struct ncplane* parent, int y, int x, int width,
                              const char* label, TuiButtonCb cb, void* userdata) {
    if (!parent || !label || width < 4) return NULL;
    TuiButton* btn = (TuiButton*)calloc(1, sizeof(TuiButton));
    if (!btn) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = (unsigned)width,
    };
    btn->plane = ncplane_create(parent, &opts);
    if (!btn->plane) { free(btn); return NULL; }

    btn->label = strdup(label);
    if (!btn->label) { ncplane_destroy(btn->plane); free(btn); return NULL; }
    btn->width = width;
    btn->focused = false;
    btn->pressed = false;
    btn->cb = cb;
    btn->userdata = userdata;
    btn->bg_normal = THEME_BG_TASKBAR;
    btn->bg_focused = THEME_BG_TEXT_FOC;
    btn->bg_pressed = THEME_BG_TAB_ACTIVE;
    btn->fg_text = THEME_FG_DEFAULT;

    tui_button_ensure_registered();
    btn->_type_id = s_button_type_id;

    tui_button_render(btn);
    return btn;
}

void tui_button_destroy(TuiButton* btn) {
    if (!btn) return;
    free(btn->label);
    if (btn->plane) ncplane_destroy(btn->plane);
    free(btn);
}

bool tui_button_set_label(TuiButton* btn, const char* label) {
    if (!btn || !label) return false;
    char* copy = strdup(label);
    if (!copy) {
        tui_log(LOG_ERROR, "Out of memory in tui_button_set_label");
        return false;
    }
    free(btn->label);
    btn->label = copy;
    tui_button_render(btn);
    return true;
}

bool tui_button_is_pressed(const TuiButton* btn) {
    return btn ? btn->pressed : false;
}

void tui_button_set_focused(TuiButton* btn, bool focused) {
    if (!btn) return;
    btn->focused = focused;
    tui_button_render(btn);
}

bool tui_button_handle_key(TuiButton* btn, uint32_t key, const struct ncinput* ni) {
    if (!btn || !ni || ni->evtype == NCTYPE_RELEASE) return false;

    if (key == NCKEY_ENTER || key == '\n' || key == '\r') {
        if (btn->cb) btn->cb(btn->userdata);
        return true;
    }
    return false;
}

bool tui_button_handle_mouse(TuiButton* btn, uint32_t key, const struct ncinput* ni) {
    if (!btn || !ni || !btn->plane || ni->evtype == NCTYPE_RELEASE) return false;
    if (key != NCKEY_BUTTON1) return false;

    /* Check if click is within this button's plane */
    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(btn->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(btn->plane, &rows, &cols);

    int local_y = ni->y - plane_abs_y;
    int local_x = ni->x - plane_abs_x;

    if (local_y < 0 || local_y >= (int)rows || local_x < 0 || local_x >= (int)cols) return false;

    /* Activate button — set pressed, render, fire callback, then reset.
     * Skip second render: both renders happen before screen refresh,
     * so the pressed state is never visible. The next frame will render
     * the unpressed state naturally via the render loop. */
    btn->pressed = true;
    tui_button_render(btn);
    if (btn->cb) btn->cb(btn->userdata);
    btn->pressed = false;
    return true;
}

void tui_button_render(TuiButton* btn) {
    if (!btn || !btn->plane) return;
    unsigned cols;
    ncplane_dim_yx(btn->plane, NULL, &cols);

    unsigned bg = btn->pressed ? btn->bg_pressed : (btn->focused ? btn->bg_focused : btn->bg_normal);
    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, btn->fg_text);
    ncchannels_set_bg_rgb(&base, bg);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncchannels_set_fg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(btn->plane, " ", 0, base);
    ncplane_erase(btn->plane);

    ncplane_set_fg_rgb(btn->plane, THEME_FG_TAB_INA);
    ncplane_putstr_yx(btn->plane, 0, 0, "[");
    ncplane_putstr_yx(btn->plane, 0, (int)cols - 1, "]");

    int label_len = (int)strlen(btn->label);
    int start = ((int)cols - label_len) / 2;
    if (start < 1) start = 1;
    ncplane_putstr_yx(btn->plane, 0, start, btn->label);
}

/* === VTable Implementation === */

static bool v_button_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiButton* btn = (TuiButton*)widget;
    if (!btn || ni->evtype == NCTYPE_RELEASE) return false;
    if (key == NCKEY_ENTER || key == '\n' || key == '\r') {
        btn->pressed = true;
        tui_button_render(btn);
        if (btn->cb) btn->cb(btn->userdata);
        btn->pressed = false;
        tui_button_render(btn);
        return true;
    }
    return false;
}

static bool v_button_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiButton* btn = (TuiButton*)widget;
    return tui_button_handle_mouse(btn, key, ni);
}

static void v_button_render(void* widget) {
    tui_button_render((TuiButton*)widget);
}

static bool v_button_is_focusable(void* widget) { (void)widget; return true; }

static void v_button_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)max_h; (void)max_w;
    TuiButton* btn = (TuiButton*)widget;
    if (out_h) *out_h = 1;
    if (out_w) *out_w = btn->label ? (int)strlen(btn->label) + 4 : 6;
}

static const TuiWidgetIface s_button_iface = {
    .handle_key = v_button_handle_key,
    .handle_mouse = v_button_handle_mouse,
    .render = v_button_render,
    .is_focusable = v_button_is_focusable,
    .preferred_size = v_button_preferred_size,
};

void tui_button_ensure_registered(void) {
    if (s_button_type_id < 0) {
        s_button_type_id = tui_widget_register(&s_button_iface);
    }
}

int tui_button_get_type_id(void) {
    tui_button_ensure_registered();
    return s_button_type_id;
}
