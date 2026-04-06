#include "widget/progress.h"
#include "core/theme.h"
#include <stdlib.h>
#include <stdio.h>

TuiProgressBar* tui_progress_create(struct ncplane* parent, int y, int x, int width) {
    if (!parent || width < 6) return NULL;
    TuiProgressBar* pb = (TuiProgressBar*)calloc(1, sizeof(TuiProgressBar));
    if (!pb) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = (unsigned)width,
    };
    pb->plane = ncplane_create(parent, &opts);
    if (!pb->plane) { free(pb); return NULL; }

    pb->width = width;
    pb->value = 0.0f;
    pb->fill_color = THEME_FG_LOG_INFO;
    pb->empty_color = THEME_FG_TAB_INA;
    pb->text_color = THEME_FG_DEFAULT;

    tui_progress_render(pb);
    return pb;
}

void tui_progress_destroy(TuiProgressBar* pb) {
    if (!pb) return;
    if (pb->plane) ncplane_destroy(pb->plane);
    free(pb);
}

void tui_progress_set_value(TuiProgressBar* pb, float value) {
    if (!pb) return;
    pb->value = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
    tui_progress_render(pb);
}

float tui_progress_get_value(const TuiProgressBar* pb) {
    return pb ? pb->value : 0.0f;
}

void tui_progress_render(TuiProgressBar* pb) {
    if (!pb || !pb->plane) return;
    unsigned cols;
    ncplane_dim_yx(pb->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_bg_rgb(&base, THEME_BG_DARKEST);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(pb->plane, " ", 0, base);
    ncplane_erase(pb->plane);

    int inner = (int)cols - 2;
    if (inner < 1) inner = 1;

    int fill = (int)(pb->value * inner + 0.5f);
    if (fill < 0) fill = 0;
    if (fill > inner) fill = inner;

    ncplane_set_fg_rgb(pb->plane, THEME_FG_TAB_INA);
    ncplane_putstr_yx(pb->plane, 0, 0, "[");
    ncplane_putstr_yx(pb->plane, 0, (int)cols - 1, "]");

    ncplane_set_fg_rgb(pb->plane, pb->fill_color);
    for (int i = 0; i < fill; i++) {
        ncplane_putchar_yx(pb->plane, 0, 1 + i, '#');
    }

    ncplane_set_fg_rgb(pb->plane, pb->empty_color);
    for (int i = fill; i < inner; i++) {
        ncplane_putchar_yx(pb->plane, 0, 1 + i, '-');
    }

    char pct[8];
    snprintf(pct, sizeof(pct), "%3.0f%%", pb->value * 100.0f);
    int pct_start = ((int)cols - 5) / 2;
    if (pct_start < 1) pct_start = 1;
    ncplane_set_fg_rgb(pb->plane, pb->text_color);
    ncplane_set_bg_rgb(pb->plane, fill > pct_start ? pb->fill_color : THEME_BG_DARKEST);
    ncplane_putstr_yx(pb->plane, 0, pct_start, pct);
}
