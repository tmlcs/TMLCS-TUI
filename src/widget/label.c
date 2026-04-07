#include "widget/label.h"
#include "core/theme.h"
#include "core/widget.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
void tui_label_ensure_registered(void);
int tui_label_get_type_id(void);

static int s_label_type_id = -1;

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
    if (!lbl->text) { ncplane_destroy(lbl->plane); free(lbl); return NULL; }
    lbl->fg_color = THEME_FG_DEFAULT;

    tui_label_ensure_registered();
    lbl->_type_id = s_label_type_id;

    tui_label_render(lbl);
    return lbl;
}

void tui_label_destroy(TuiLabel* lbl) {
    if (!lbl) return;
    free(lbl->text);
    if (lbl->plane) ncplane_destroy(lbl->plane);
    free(lbl);
}

bool tui_label_set_text(TuiLabel* lbl, const char* text) {
    if (!lbl || !text) return false;
    char* copy = strdup(text);
    if (!copy) {
        tui_log(LOG_ERROR, "Out of memory in tui_label_set_text");
        return false;
    }
    free(lbl->text);
    lbl->text = copy;
    tui_label_render(lbl);
    return true;
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

/* === VTable Implementation === */

static bool v_label_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    (void)widget; (void)key; (void)ni;
    return false;
}

static bool v_label_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    (void)widget; (void)key; (void)ni;
    return false;
}

static void v_label_render(void* widget) {
    tui_label_render((TuiLabel*)widget);
}

static bool v_label_is_focusable(void* widget) { (void)widget; return false; }

static void v_label_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)max_h; (void)max_w;
    TuiLabel* lbl = (TuiLabel*)widget;
    if (out_h) *out_h = 1;
    if (out_w) *out_w = lbl->text ? (int)strlen(lbl->text) + 2 : 4;
}

static const TuiWidgetIface s_label_iface = {
    .handle_key = v_label_handle_key,
    .handle_mouse = v_label_handle_mouse,
    .render = v_label_render,
    .is_focusable = v_label_is_focusable,
    .preferred_size = v_label_preferred_size,
};

void tui_label_ensure_registered(void) {
    if (s_label_type_id < 0) {
        s_label_type_id = tui_widget_register(&s_label_iface);
    }
}

int tui_label_get_type_id(void) {
    tui_label_ensure_registered();
    return s_label_type_id;
}
