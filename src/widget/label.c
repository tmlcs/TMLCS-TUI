#include "widget/label.h"
#include "core/theme.h"
#include <stdlib.h>
#include <string.h>

TuiLabel* tui_label_create(struct ncplane* parent, int y, int x, const char* text) {
    if (!parent || !text) return NULL;
    TuiLabel* lbl = (TuiLabel*)calloc(1, sizeof(TuiLabel));
    if (!lbl) return NULL;

    int len = (int)strlen(text);
    int width = len + 2;
    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = (unsigned)width,
    };
    lbl->plane = ncplane_create(parent, &opts);
    if (!lbl->plane) { free(lbl); return NULL; }

    lbl->text = strdup(text);
    lbl->fg_color = THEME_FG_DEFAULT;

    tui_label_render(lbl);
    return lbl;
}

void tui_label_destroy(TuiLabel* lbl) {
    if (!lbl) return;
    free(lbl->text);
    if (lbl->plane) ncplane_destroy(lbl->plane);
    free(lbl);
}

void tui_label_set_text(TuiLabel* lbl, const char* text) {
    if (!lbl || !text) return;
    free(lbl->text);
    lbl->text = strdup(text);
    tui_label_render(lbl);
}

void tui_label_render(TuiLabel* lbl) {
    if (!lbl || !lbl->plane) return;
    unsigned cols;
    ncplane_dim_yx(lbl->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, lbl->fg_color);
    ncchannels_set_bg_rgb(&base, THEME_BG_DARKEST);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(lbl->plane, " ", 0, base);
    ncplane_erase(lbl->plane);

    ncplane_set_fg_rgb(lbl->plane, lbl->fg_color);
    ncplane_putstr_yx(lbl->plane, 0, 1, lbl->text);
}

const char* tui_label_get_text(const TuiLabel* lbl) {
    return lbl ? lbl->text : NULL;
}
