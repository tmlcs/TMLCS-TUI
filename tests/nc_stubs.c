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

/* Minimal type stubs */
struct ncplane { int _dummy; };
struct notcurses { int _dummy; };
typedef struct ncplane_options {
    int y, x;
    unsigned rows, cols;
    uint64_t flags;
} ncplane_options;

/* Channels API */
typedef uint64_t uint64_t_;

static char g_dummy_data[4096];

struct ncplane* ncplane_create(struct ncplane* p, const ncplane_options* o) {
    (void)p; (void)o;
    return (struct ncplane*)g_dummy_data;
}
int ncplane_destroy(struct ncplane* n) { (void)n; return 0; }
void ncplane_dim_yx(const struct ncplane* n, unsigned* restrict rows, unsigned* restrict cols) {
    (void)n; if(rows) *rows=24; if(cols) *cols=80;
}
void ncplane_yx(const struct ncplane* n, int* restrict y, int* restrict x) {
    (void)n; if(y) *y=0; if(x) *x=0;
}
void ncplane_abs_yx(const struct ncplane* n, int* restrict y, int* restrict x) {
    (void)n; if(y) *y=0; if(x) *x=0;
}
struct ncplane* ncplane_parent(struct ncplane* n) {
    (void)n; return (struct ncplane*)g_dummy_data;
}
int ncplane_move_yx(struct ncplane* n, int y, int x) { (void)n;(void)y;(void)x; return 0; }
int ncplane_move_below(struct ncplane* n, struct ncplane* p) { (void)n;(void)p; return 0; }
void ncplane_move_top(struct ncplane* n) { (void)n; }
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
int ncplane_printf_yx(struct ncplane* n, int y, int x, const char* fmt, ...) {
    (void)n;(void)y;(void)x;(void)fmt; return 0;
}
int ncplane_putchar_yx(struct ncplane* n, int y, int x, char c) {
    (void)n;(void)y;(void)x;(void)c; return 0;
}
int ncplane_perimeter_rounded(struct ncplane* n, unsigned s, uint64_t c, unsigned e) {
    (void)n;(void)s;(void)c;(void)e; return 0;
}
int ncplane_resize_simple(struct ncplane* n, unsigned y, unsigned x) {
    (void)n;(void)y;(void)x; return 0;
}
struct ncplane* ncplane_reparent(struct ncplane* n, struct ncplane* p) {
    (void)n;(void)p; return n;
}
int ncchannels_set_fg_rgb(uint64_t* c, unsigned rgb) { (void)c;(void)rgb; return 0; }
int ncchannels_set_bg_rgb(uint64_t* c, unsigned rgb) { (void)c;(void)rgb; return 0; }
int ncchannels_set_bg_alpha(uint64_t* c, unsigned a) { (void)c;(void)a; return 0; }
int ncchannels_set_fg_alpha(uint64_t* c, unsigned a) { (void)c;(void)a; return 0; }
struct ncplane* notcurses_stdplane(struct notcurses* nc) {
    (void)nc; return (struct ncplane*)g_dummy_data;
}
int notcurses_render(struct notcurses* nc) { (void)nc; return 0; }
int notcurses_mice_enable(struct notcurses* nc, unsigned m) { (void)nc;(void)m; return 0; }
