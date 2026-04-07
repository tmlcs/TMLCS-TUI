/**
 * Comprehensive notcurses stubs for unit testing.
 *
 * We do NOT include <notcurses/notcurses.h> because it defines many functions
 * as static inline, which would conflict with our definitions.
 * Instead, we forward-declare the types and stub the functions.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Minimal type stubs */
struct ncplane { int _dummy; };
struct notcurses { int _dummy; };
typedef uint32_t NcKeyType;
/* Notcurses key constants */
#define NCKEY_BUTTON1 0x10000
#define NCKEY_BUTTON2 0x10001
#define NCKEY_BUTTON3 0x10002
#define NCKEY_BUTTON4 0x10003
#define NCKEY_BUTTON5 0x10004
#define NCKEY_BUTTON6 0x10005
#define NCKEY_BUTTON7 0x10006
#define NCKEY_BUTTON8 0x10007
#define NCKEY_BUTTON9 0x10008
#define NCKEY_BUTTON10 0x10009
#define NCKEY_BUTTON11 0x1000A
#define NCKEY_MOTION 0x10010
#define NCKEY_SCROLL_UP 0x10011
#define NCKEY_SCROLL_DOWN 0x10012
#define NCKEY_ESCAPE 0x1B
#define NCKEY_LEFT 0x1000B
#define NCKEY_RIGHT 0x1000C
#define NCKEY_UP 0x1000D
#define NCKEY_DOWN 0x1000E
#define NCKEY_TAB 0x09
#define NCKEY_F1 0x1003F
#define NCKEY_RESIZE 0x10040
#define NCKEY_BACKSPACE 0x08
#define NCKEY_DELETE 0x7F
#define NCKEY_HOME 0x10016
#define NCKEY_END 0x10017
#define MAX_MOUSE_BUTTON NCKEY_BUTTON11

typedef enum {
    NCTYPE_UNKNOWN = 0,
    NCTYPE_PRESS = 1,
    NCTYPE_RELEASE = 2,
    NCTYPE_REPEAT = 3,
    NCTYPE_MOTION = 4
} NcInputType;

typedef struct ncinput {
    uint32_t id;
    int y, x;
    NcInputType evtype;
    bool ctrl, alt, shift;
    bool super_, meta, hyper;
    uint64_t modifiers;
} ncinput;

typedef struct ncplane_options {
    int y, x;
    unsigned rows, cols;
    uint64_t flags;
} ncplane_options;

/* Channels API */
typedef uint64_t uint64_t_;

static char g_dummy_data[4096];

/* Channel accessor stubs (required by notcurses inline functions) */
uint64_t* ncplane_channels(struct ncplane* n) { (void)n; return (uint64_t*)g_dummy_data; }
const char* ncplane_styles(struct ncplane* n) { (void)n; return g_dummy_data; }

/* --- Stub extensions for testing --- */

/* Forward declarations */
void ncplane_create_set_fail_count(int n);
void notcurses_stdplane_set_fail(int fail);
void stub_reset_counters(void);
int stub_get_resize_count(void);
int stub_get_move_count(void);
int stub_get_move_top_count(void);

/* ncplane_create failure counter */
static int g_ncplane_create_fail_count = 0;
void ncplane_create_set_fail_count(int n) { g_ncplane_create_fail_count = n; }

/* notcurses_stdplane failure control */
static int g_stdplane_fail = 0;
void notcurses_stdplane_set_fail(int fail) { g_stdplane_fail = fail; }

/* Call counters */
static int g_ncplane_resize_simple_count = 0;
static int g_ncplane_move_yx_count = 0;
static int g_ncplane_move_top_count = 0;

/* Configurable absolute position per plane (for mouse hit testing) */
#define MAX_PLANE_POSITIONS 32
static struct { struct ncplane* plane; int y; int x; } g_plane_positions[MAX_PLANE_POSITIONS];
static int g_plane_position_count = 0;
/* Default fallback position */
static int g_default_plane_y = 0;
static int g_default_plane_x = 0;

void stub_set_plane_position(struct ncplane* plane, int y, int x) {
    /* Find existing entry or add new one */
    for (int i = 0; i < g_plane_position_count; i++) {
        if (g_plane_positions[i].plane == plane) {
            g_plane_positions[i].y = y;
            g_plane_positions[i].x = x;
            return;
        }
    }
    if (g_plane_position_count < MAX_PLANE_POSITIONS) {
        g_plane_positions[g_plane_position_count].plane = plane;
        g_plane_positions[g_plane_position_count].y = y;
        g_plane_positions[g_plane_position_count].x = x;
        g_plane_position_count++;
    }
}
void stub_set_default_plane_position(int y, int x) {
    g_default_plane_y = y;
    g_default_plane_x = x;
}

void stub_reset_counters(void) {
    g_ncplane_resize_simple_count = 0;
    g_ncplane_move_yx_count = 0;
    g_ncplane_move_top_count = 0;
    g_ncplane_create_fail_count = 0;
    g_stdplane_fail = 0;
}
int stub_get_resize_count(void) { return g_ncplane_resize_simple_count; }
int stub_get_move_count(void) { return g_ncplane_move_yx_count; }
int stub_get_move_top_count(void) { return g_ncplane_move_top_count; }

/* ncplane_create: check failure counter before returning sentinel */
struct ncplane* ncplane_create(struct ncplane* p, const ncplane_options* o) {
    (void)p; (void)o;
    if (g_ncplane_create_fail_count > 0) {
        g_ncplane_create_fail_count--;
        return NULL;
    }
    return (struct ncplane*)g_dummy_data;
}
int ncplane_resize_simple(struct ncplane* n, unsigned y, unsigned x) {
    (void)n;(void)y;(void)x;
    g_ncplane_resize_simple_count++;
    return 0;
}
/* ncplane_resize is called by the inline ncplane_resize_simple from notcurses.h.
 * We provide it here so the linker resolves it from our stubs. */
int ncplane_resize(struct ncplane* n, int keepy, int keepx, unsigned keepleny, unsigned keeplenx, int yoff, int xoff, unsigned ylen, unsigned xlen) {
    (void)n;(void)keepy;(void)keepx;(void)keepleny;(void)keeplenx;(void)yoff;(void)xoff;(void)ylen;(void)xlen;
    g_ncplane_resize_simple_count++;
    return 0;
}
int ncplane_move_yx(struct ncplane* n, int y, int x) {
    (void)n;(void)y;(void)x;
    g_ncplane_move_yx_count++;
    return 0;
}
void ncplane_move_top(struct ncplane* n) {
    (void)n;
    g_ncplane_move_top_count++;
}

/* --- Standard stub functions --- */

int ncplane_destroy(struct ncplane* n) { (void)n; return 0; }
void ncplane_dim_yx(const struct ncplane* n, unsigned* restrict rows, unsigned* restrict cols) {
    (void)n; if(rows) *rows=24; if(cols) *cols=80;
}
void ncplane_yx(const struct ncplane* n, int* restrict y, int* restrict x) {
    (void)n; if(y) *y=0; if(x) *x=0;
}
void ncplane_abs_yx(const struct ncplane* n, int* restrict y, int* restrict x) {
    /* Check per-plane position registry first */
    for (int i = 0; i < g_plane_position_count; i++) {
        if ((const struct ncplane*)g_plane_positions[i].plane == n) {
            if (y) *y = g_plane_positions[i].y;
            if (x) *x = g_plane_positions[i].x;
            return;
        }
    }
    /* Fallback to default */
    if (y) *y = g_default_plane_y;
    if (x) *x = g_default_plane_x;
}
struct ncplane* ncplane_parent(struct ncplane* n) {
    (void)n; return (struct ncplane*)g_dummy_data;
}
int ncplane_move_below(struct ncplane* n, struct ncplane* p) { (void)n;(void)p; return 0; }
int ncplane_set_bg_rgb(struct ncplane* n, unsigned rgb) { (void)n;(void)rgb; return 0; }
int ncplane_set_fg_rgb(struct ncplane* n, unsigned rgb) { (void)n;(void)rgb; return 0; }
int ncplane_set_bg_alpha(struct ncplane* n, int a) { (void)n;(void)a; return 0; }
int ncplane_set_fg_alpha(struct ncplane* n, int a) { (void)n;(void)a; return 0; }
int ncplane_erase(struct ncplane* n) { (void)n; return 0; }
int ncplane_set_base(struct ncplane* n, const char* e, unsigned s, uint64_t c) {
    (void)n;(void)e;(void)s;(void)c; return 0;
}
int ncplane_putstr_yx(struct ncplane* n, int y, int x, const char* s) {
    (void)n;(void)y;(void)x;(void)s; return s?(int)strlen(s):0;
}
int ncplane_putegc_yx(struct ncplane* n, int y, int x, const char* gcl, int* sbytes) {
    (void)n;(void)y;(void)x;(void)gcl;(void)sbytes;
    /* Return -1 so the inline ncplane_putstr_yx loop hits the error path
     * (if cols < 0 → return -ret) and exits immediately. */
    return -1;
}
int ncplane_putc_yx(struct ncplane* n, int y, int x, const uint64_t* c) {
    (void)n;(void)y;(void)x;(void)c; return 0;
}
int ncplane_printf_yx(struct ncplane* n, int y, int x, const char* fmt, ...) {
    (void)n;(void)y;(void)x;(void)fmt; return 0;
}
int ncplane_putchar_yx(struct ncplane* n, int y, int x, char c) {
    (void)n;(void)y;(void)x;(void)c; return 0;
}
int ncplane_perimeter_rounded(struct ncplane* n, unsigned s, uint64_t c, unsigned e) {
    (void)n;(void)s;(void)c;(void)e; return 0;
}
struct ncplane* ncplane_reparent(struct ncplane* n, struct ncplane* p) {
    (void)n;(void)p; return n;
}
int ncchannels_set_fg_rgb(uint64_t* c, unsigned rgb) { (void)c;(void)rgb; return 0; }
int ncchannels_set_bg_rgb(uint64_t* c, unsigned rgb) { (void)c;(void)rgb; return 0; }
int ncchannels_set_bg_alpha(uint64_t* c, unsigned a) { (void)c;(void)a; return 0; }
int ncchannels_set_fg_alpha(uint64_t* c, unsigned a) { (void)c;(void)a; return 0; }
struct ncplane* notcurses_stdplane(struct notcurses* nc) {
    if (g_stdplane_fail) return NULL;
    (void)nc; return (struct ncplane*)g_dummy_data;
}
int notcurses_render(struct notcurses* nc) { (void)nc; return 0; }
int notcurses_mice_enable(struct notcurses* nc, unsigned m) { (void)nc;(void)m; return 0; }
uint32_t notcurses_get_nblock(const struct notcurses* nc, struct ncinput* ni) { (void)nc;(void)ni; return (uint32_t)-1; }
uint32_t notcurses_get(struct notcurses* n, const struct timespec* ts, struct ncinput* ni) { (void)n;(void)ts;(void)ni; return (uint32_t)-1; }

/* --- Additional stubs required by manager/render.c --- */

/* nccell API */
int nccell_load(struct ncplane* n, int y, int x, uint64_t* c) { (void)n;(void)y;(void)x;(void)c; return 0; }
void nccell_release(struct ncplane* n, void* c) { (void)n;(void)c; }

/* ncpile API */
int ncpile_render(struct notcurses* nc) { (void)nc; return 0; }
int ncpile_rasterize(struct notcurses* nc) { (void)nc; return 0; }

/* notcurses capabilities — return a static struct with utf8=true so inline
 * notcurses_canutf8() can dereference ->utf8 without segfaulting. */
static struct {
    unsigned colors;
    int utf8;
    int rgb;
    int can_change_colors;
    int halfblocks;
    int quadrants;
    int sextants;
    int octants;
    int braille;
} g_stub_caps = {
    .colors = 256,
    .utf8 = 1,
    .rgb = 1,
    .can_change_colors = 0,
    .halfblocks = 1,
    .quadrants = 1,
    .sextants = 1,
    .octants = 1,
    .braille = 1,
};

const void* notcurses_capabilities(const struct notcurses* nc) { (void)nc; return &g_stub_caps; }
int notcurses_canutf8(const struct notcurses* nc) { (void)nc; return 1; }

/* ncplane cursor and printf */
int ncplane_vprintf_yx(struct ncplane* n, int y, int x, const char* fmt, va_list ap) { (void)n;(void)y;(void)x;(void)fmt;(void)ap; return 0; }
int ncplane_cursor_yx(const struct ncplane* n, int* y, int* x) { (void)n; if(y) *y=0; if(x) *x=0; return 0; }
int ncplane_cursor_move_yx(struct ncplane* n, int y, int x) { (void)n;(void)y;(void)x; return 0; }
struct notcurses* ncplane_notcurses(const struct ncplane* n) { (void)n; return (struct notcurses*)g_dummy_data; }

/* ncplane_box / ncplane_box_sized */
int ncplane_box(struct ncplane* n, const void* ul, const void* ur, const void* ll, const void* lr, const void* hl, const void* vl, int ystop, int xstop, unsigned ctlword) {
    (void)n;(void)ul;(void)ur;(void)ll;(void)lr;(void)hl;(void)vl;(void)ystop;(void)xstop;(void)ctlword; return 0;
}

/* Return a dummy plane pointer for fuzz test usage */
struct ncplane* stub_get_dummy_plane(void) {
    return (struct ncplane*)g_dummy_data;
}
