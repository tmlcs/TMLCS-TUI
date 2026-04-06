#include "widget/spinner.h"
#include "core/theme.h"
#include "core/widget.h"
#include <stdlib.h>

void tui_spinner_ensure_registered(void);
int tui_spinner_get_type_id(void);

static int s_spinner_type_id = -1;

static const char* s_dot_frames[] = {
    "\xe2\xa0\x8b", "\xe2\xa0\x99", "\xe2\xa0\xb9", "\xe2\xa0\xb8",
    "\xe2\xa0\xbc", "\xe2\xa0\xb4", "\xe2\xa0\xa6", "\xe2\xa0\xa7",
    "\xe2\xa0\x87", "\xe2\xa0\x8f"
};
static const int s_dot_count = 10;

static const char* s_line_frames[] = { "|", "/", "-", "\\" };
static const int s_line_count = 4;

static const char* s_arrow_frames[] = {
    "\xe2\x86\x90", "\xe2\x86\x96", "\xe2\x86\x91", "\xe2\x86\x97",
    "\xe2\x86\x92", "\xe2\x86\x98", "\xe2\x86\x93", "\xe2\x86\x99"
};
static const int s_arrow_count = 8;

static const char* get_frame_text(TuiSpinnerType type, int frame) {
    switch (type) {
        case SPINNER_DOTS:
            return s_dot_frames[frame % s_dot_count];
        case SPINNER_LINE:
            return s_line_frames[frame % s_line_count];
        case SPINNER_ARROW:
            return s_arrow_frames[frame % s_arrow_count];
        default:
            return s_line_frames[frame % s_line_count];
    }
}

static int get_frame_count(TuiSpinnerType type) {
    switch (type) {
        case SPINNER_DOTS: return s_dot_count;
        case SPINNER_LINE: return s_line_count;
        case SPINNER_ARROW: return s_arrow_count;
        default: return s_line_count;
    }
}

TuiSpinner* tui_spinner_create(struct ncplane* parent, int y, int x, TuiSpinnerType type) {
    if (!parent) return NULL;
    if (type < SPINNER_DOTS || type > SPINNER_ARROW) type = SPINNER_LINE;

    TuiSpinner* spinner = (TuiSpinner*)calloc(1, sizeof(TuiSpinner));
    if (!spinner) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = 2,
    };
    spinner->plane = ncplane_create(parent, &opts);
    if (!spinner->plane) { free(spinner); return NULL; }

    spinner->type = type;
    spinner->frame = 0;
    spinner->frame_count = get_frame_count(type);
    spinner->running = false;

    tui_spinner_ensure_registered();
    spinner->_type_id = s_spinner_type_id;

    tui_spinner_render(spinner);
    return spinner;
}

void tui_spinner_destroy(TuiSpinner* spinner) {
    if (!spinner) return;
    if (spinner->plane) ncplane_destroy(spinner->plane);
    free(spinner);
}

void tui_spinner_start(TuiSpinner* spinner) {
    if (!spinner) return;
    spinner->running = true;
}

void tui_spinner_stop(TuiSpinner* spinner) {
    if (!spinner) return;
    spinner->running = false;
}

bool tui_spinner_is_running(const TuiSpinner* spinner) {
    return spinner ? spinner->running : false;
}

void tui_spinner_tick(TuiSpinner* spinner) {
    if (!spinner || !spinner->running) return;
    spinner->frame = (spinner->frame + 1) % spinner->frame_count;
    tui_spinner_render(spinner);
}

void tui_spinner_render(TuiSpinner* spinner) {
    if (!spinner || !spinner->plane) return;

    ncplane_erase(spinner->plane);
    const char* frame = get_frame_text(spinner->type, spinner->frame);
    ncplane_set_fg_rgb(spinner->plane, THEME_FG_TAB_ACTIVE);
    ncplane_putstr_yx(spinner->plane, 0, 0, frame);
}

bool tui_spinner_handle_key(TuiSpinner* spinner, uint32_t key, const struct ncinput* ni) {
    (void)spinner; (void)key; (void)ni;
    return false;
}

bool tui_spinner_handle_mouse(TuiSpinner* spinner, uint32_t key, const struct ncinput* ni) {
    (void)spinner; (void)key; (void)ni;
    return false;
}

/* === VTable === */

static bool v_spinner_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_spinner_handle_key((TuiSpinner*)widget, key, ni);
}

static bool v_spinner_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_spinner_handle_mouse((TuiSpinner*)widget, key, ni);
}

static void v_spinner_render(void* widget) {
    tui_spinner_render((TuiSpinner*)widget);
}

static bool v_spinner_is_focusable(void* widget) { (void)widget; return false; }

static bool v_spinner_needs_periodic_render(void* widget) {
    TuiSpinner* spinner = (TuiSpinner*)widget;
    return spinner ? spinner->running : false;
}

static void v_spinner_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)widget; (void)max_h; (void)max_w;
    if (out_h) *out_h = 1;
    if (out_w) *out_w = 2;
}

static const TuiWidgetIface s_spinner_iface = {
    .handle_key = v_spinner_handle_key,
    .handle_mouse = v_spinner_handle_mouse,
    .render = v_spinner_render,
    .is_focusable = v_spinner_is_focusable,
    .needs_periodic_render = v_spinner_needs_periodic_render,
    .preferred_size = v_spinner_preferred_size,
};

void tui_spinner_ensure_registered(void) {
    if (s_spinner_type_id < 0) {
        s_spinner_type_id = tui_widget_register(&s_spinner_iface);
    }
}

int tui_spinner_get_type_id(void) {
    tui_spinner_ensure_registered();
    return s_spinner_type_id;
}
