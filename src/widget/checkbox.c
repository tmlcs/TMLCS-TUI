#include "widget/checkbox.h"
#include "core/theme.h"
#include <stdlib.h>
#include <string.h>

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
    c->checked = checked;
    c->focused = false;
    c->cb = cb;
    c->userdata = userdata;
    c->fg_check = THEME_FG_LOG_INFO;     /* Green check */
    c->fg_unchecked = THEME_FG_TAB_INA;   /* Gray box */
    c->fg_label = THEME_FG_DEFAULT;
    c->bg_normal = THEME_BG_DARKEST;
    c->bg_focused = THEME_BG_TEXT_FOC;

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
