#include "widget/button.h"
#include "core/theme.h"
#include <stdlib.h>
#include <string.h>

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
    btn->width = width;
    btn->focused = false;
    btn->pressed = false;
    btn->cb = cb;
    btn->userdata = userdata;
    btn->bg_normal = THEME_BG_TASKBAR;
    btn->bg_focused = THEME_BG_TEXT_FOC;
    btn->bg_pressed = THEME_BG_TAB_ACTIVE;
    btn->fg_text = THEME_FG_DEFAULT;

    tui_button_render(btn);
    return btn;
}

void tui_button_destroy(TuiButton* btn) {
    if (!btn) return;
    free(btn->label);
    if (btn->plane) ncplane_destroy(btn->plane);
    free(btn);
}

void tui_button_set_label(TuiButton* btn, const char* label) {
    if (!btn || !label) return;
    free(btn->label);
    btn->label = strdup(label);
    tui_button_render(btn);
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
    if (!btn || ni->evtype == NCTYPE_RELEASE) return false;

    if (key == NCKEY_ENTER || key == '\n' || key == '\r') {
        if (btn->cb) btn->cb(btn->userdata);
        return true;
    }
    return false;
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
