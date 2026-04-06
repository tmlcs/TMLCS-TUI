#include "widget/slider.h"
#include "core/theme.h"
#include "core/widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tui_slider_ensure_registered(void);
int tui_slider_get_type_id(void);

static int s_slider_type_id = -1;

TuiSlider* tui_slider_create(struct ncplane* parent, int y, int x, int width,
                              float min, float max, float step,
                              TuiSliderCb cb, void* userdata) {
    if (!parent || width < 4 || max <= min || step < 0.0f) return NULL;
    TuiSlider* slider = (TuiSlider*)calloc(1, sizeof(TuiSlider));
    if (!slider) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = (unsigned)width,
    };
    slider->plane = ncplane_create(parent, &opts);
    if (!slider->plane) { free(slider); return NULL; }

    slider->min = min;
    slider->max = max;
    slider->value = min;
    slider->step = step;
    slider->width = width;
    slider->focused = false;
    slider->cb = cb;
    slider->userdata = userdata;
    slider->fg_track = THEME_FG_SEPARATOR;
    slider->fg_fill = THEME_FG_TAB_ACTIVE;
    slider->fg_thumb = THEME_FG_DEFAULT;
    slider->bg_normal = THEME_BG_DARKEST;
    slider->bg_focused = THEME_BG_TEXT_FOC;

    tui_slider_ensure_registered();
    slider->_type_id = s_slider_type_id;

    tui_slider_render(slider);
    return slider;
}

void tui_slider_destroy(TuiSlider* slider) {
    if (!slider) return;
    if (slider->plane) ncplane_destroy(slider->plane);
    free(slider);
}

float tui_slider_get_value(const TuiSlider* slider) {
    return slider ? slider->value : 0.0f;
}

void tui_slider_set_value(TuiSlider* slider, float value) {
    if (!slider) return;
    if (value < slider->min) value = slider->min;
    if (value > slider->max) value = slider->max;
    slider->value = value;
    tui_slider_render(slider);
}

void tui_slider_set_focused(TuiSlider* slider, bool focused) {
    if (!slider) return;
    slider->focused = focused;
    tui_slider_render(slider);
}

bool tui_slider_handle_key(TuiSlider* slider, uint32_t key, const struct ncinput* ni) {
    if (!slider || !ni) return false;

    float new_val = slider->value;
    bool changed = false;

    if (key == NCKEY_LEFT || key == 'h') {
        new_val = slider->value - slider->step;
        if (new_val < slider->min) new_val = slider->min;
        changed = true;
    } else if (key == NCKEY_RIGHT || key == 'l') {
        new_val = slider->value + slider->step;
        if (new_val > slider->max) new_val = slider->max;
        changed = true;
    } else if (key == NCKEY_HOME) {
        new_val = slider->min;
        changed = true;
    } else if (key == NCKEY_END) {
        new_val = slider->max;
        changed = true;
    }

    if (changed) {
        slider->value = new_val;
        tui_slider_render(slider);
        if (slider->cb) slider->cb(slider->value, slider->userdata);
        return true;
    }
    return false;
}

bool tui_slider_handle_mouse(TuiSlider* slider, uint32_t key, const struct ncinput* ni) {
    if (!slider || !ni || !slider->plane) return false;
    if (key != NCKEY_BUTTON1) return false;

    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(slider->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(slider->plane, &rows, &cols);

    int local_x = ni->x - plane_abs_x;
    int local_y = ni->y - plane_abs_y;
    if (local_y < 0 || local_y >= (int)rows) return false;
    if (local_x < 1 || local_x >= (int)cols - 1) return false;

    float ratio = (float)(local_x - 1) / (float)(cols - 2);
    float new_val = slider->min + ratio * (slider->max - slider->min);
    if (new_val < slider->min) new_val = slider->min;
    if (new_val > slider->max) new_val = slider->max;

    slider->value = new_val;
    tui_slider_render(slider);
    if (slider->cb) slider->cb(slider->value, slider->userdata);
    return true;
}

void tui_slider_render(TuiSlider* slider) {
    if (!slider || !slider->plane) return;
    unsigned cols;
    ncplane_dim_yx(slider->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, slider->fg_track);
    ncchannels_set_bg_rgb(&base, slider->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(slider->plane, " ", 0, base);
    ncplane_erase(slider->plane);

    /* Draw brackets */
    ncplane_set_fg_rgb(slider->plane, slider->fg_track);
    ncplane_putstr_yx(slider->plane, 0, 0, "[");
    ncplane_putstr_yx(slider->plane, 0, (int)cols - 1, "]");

    /* Calculate fill */
    float ratio = (slider->value - slider->min) / (slider->max - slider->min);
    int track_w = (int)cols - 2;
    int fill_w = (int)(ratio * (float)track_w);
    if (fill_w > track_w) fill_w = track_w;

    /* Draw fill */
    for (int i = 0; i < fill_w; i++) {
        ncplane_set_fg_rgb(slider->plane, slider->fg_fill);
        ncplane_set_bg_rgb(slider->plane, slider->fg_fill);
        ncplane_putstr_yx(slider->plane, 0, 1 + i, " ");
    }

    /* Draw remaining track */
    for (int i = fill_w; i < track_w; i++) {
        ncplane_set_fg_rgb(slider->plane, slider->bg_normal);
        ncplane_set_bg_rgb(slider->plane, slider->bg_normal);
        ncplane_putstr_yx(slider->plane, 0, 1 + i, "-");
    }

    /* Draw thumb at fill position */
    if (fill_w > 0 && fill_w < track_w) {
        ncplane_set_fg_rgb(slider->plane, slider->fg_thumb);
        ncplane_set_bg_rgb(slider->plane, slider->fg_fill);
        ncplane_putstr_yx(slider->plane, 0, fill_w, "|");
    }
}

/* === VTable === */

static bool v_slider_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_slider_handle_key((TuiSlider*)widget, key, ni);
}

static bool v_slider_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_slider_handle_mouse((TuiSlider*)widget, key, ni);
}

static void v_slider_render(void* widget) {
    tui_slider_render((TuiSlider*)widget);
}

static bool v_slider_is_focusable(void* widget) { (void)widget; return true; }

static void v_slider_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)max_h; (void)max_w;
    TuiSlider* slider = (TuiSlider*)widget;
    if (out_h) *out_h = 1;
    if (out_w) *out_w = slider->width > 0 ? slider->width : 10;
}

static const TuiWidgetIface s_slider_iface = {
    .handle_key = v_slider_handle_key,
    .handle_mouse = v_slider_handle_mouse,
    .render = v_slider_render,
    .is_focusable = v_slider_is_focusable,
    .preferred_size = v_slider_preferred_size,
};

void tui_slider_ensure_registered(void) {
    if (s_slider_type_id < 0) {
        s_slider_type_id = tui_widget_register(&s_slider_iface);
    }
}

int tui_slider_get_type_id(void) {
    tui_slider_ensure_registered();
    return s_slider_type_id;
}
