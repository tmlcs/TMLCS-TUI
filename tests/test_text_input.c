#include "unity.h"
#include "core/logger.h"
#include "widget/text_input.h"
#include "core/types.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_text_input.log";

/* ------------------------------------------------------------------ */
/*  Stub declarations                                                   */
/* ------------------------------------------------------------------ */

void ncplane_create_set_fail_count(int n);

/* ------------------------------------------------------------------ */
/*  Submit callback spy                                                 */
/* ------------------------------------------------------------------ */

static char g_last_submit[TEXT_INPUT_MAX_LEN];
static bool g_submit_called = false;

static void test_submit_cb(const char* text, void* userdata) {
    (void)userdata;
    g_submit_called = true;
    strncpy(g_last_submit, text, sizeof(g_last_submit) - 1);
    g_last_submit[sizeof(g_last_submit) - 1] = '\0';
}

static void reset_submit(void) {
    g_submit_called = false;
    memset(g_last_submit, 0, sizeof(g_last_submit));
}

/* ------------------------------------------------------------------ */
/*  ncinput helper                                                      */
/* ------------------------------------------------------------------ */

static struct ncinput make_ncinput(uint32_t key, int evtype) {
    (void)key;
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = evtype;
    ni.y = 0;
    ni.x = 0;
    ni.alt = false;
    return ni;
}

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init(test_log_file);
    ncplane_create_set_fail_count(0);
}

void tearDown(void) {
    ncplane_create_set_fail_count(0);
    tui_logger_destroy();
}

/* ------------------------------------------------------------------ */
/*  Create / Destroy (5 tests)                                          */
/* ------------------------------------------------------------------ */

void test_text_input_create_success(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    TEST_ASSERT_NOT_NULL(ti);
    TEST_ASSERT_EQUAL_INT(0, ti->cursor);
    TEST_ASSERT_EQUAL_INT(0, ti->len);
    TEST_ASSERT_EQUAL_INT(0, ti->scroll_off);
    TEST_ASSERT_FALSE(ti->focused);
    tui_text_input_destroy(ti);
}

void test_text_input_create_oom(void) {
    ncplane_create_set_fail_count(1);
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    TEST_ASSERT_NULL(ti);
    ncplane_create_set_fail_count(0);
}

void test_text_input_destroy_null_safe(void) {
    tui_text_input_destroy(NULL);  /* Should not crash */
    TEST_ASSERT_TRUE(1);
}

void test_text_input_destroy_cleans_up(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    TEST_ASSERT_NOT_NULL(ti);
    tui_text_input_destroy(ti);  /* ASAN will verify no leaks */
}

void test_text_input_create_stores_callback_and_userdata(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40,
                                              test_submit_cb, (void*)0x1234);
    TEST_ASSERT_NOT_NULL(ti);
    TEST_ASSERT_EQUAL_INT((intptr_t)test_submit_cb, (intptr_t)ti->on_submit);
    TEST_ASSERT_EQUAL_INT(0x1234, (intptr_t)ti->userdata);
    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  Get / Clear (4 tests)                                               */
/* ------------------------------------------------------------------ */

void test_text_input_get_returns_buffer(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni = make_ncinput('h', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'h', &ni);
    ni = make_ncinput('i', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'i', &ni);
    TEST_ASSERT_EQUAL_STRING("hi", tui_text_input_get(ti));
    tui_text_input_destroy(ti);
}

void test_text_input_get_null_returns_null(void) {
    TEST_ASSERT_NULL(tui_text_input_get(NULL));
}

void test_text_input_clear_resets_all(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    ni = make_ncinput('c', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'c', &ni);
    TEST_ASSERT_EQUAL_INT(3, ti->cursor);
    TEST_ASSERT_EQUAL_INT(3, ti->len);

    tui_text_input_clear(ti);
    TEST_ASSERT_EQUAL_INT('\0', ti->buffer[0]);
    TEST_ASSERT_EQUAL_INT(0, ti->cursor);
    TEST_ASSERT_EQUAL_INT(0, ti->len);
    TEST_ASSERT_EQUAL_INT(0, ti->scroll_off);
    tui_text_input_destroy(ti);
}

void test_text_input_clear_null_safe(void) {
    tui_text_input_clear(NULL);  /* Should not crash */
    TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/*  Character Insertion (5 tests)                                       */
/* ------------------------------------------------------------------ */

void test_text_input_insert_single_char(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    TEST_ASSERT_EQUAL_STRING("a", tui_text_input_get(ti));
    TEST_ASSERT_EQUAL_INT(1, ti->len);
    TEST_ASSERT_EQUAL_INT(1, ti->cursor);
    tui_text_input_destroy(ti);
}

void test_text_input_insert_multiple_chars(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    ni = make_ncinput('c', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'c', &ni);
    TEST_ASSERT_EQUAL_STRING("abc", tui_text_input_get(ti));
    TEST_ASSERT_EQUAL_INT(3, ti->len);
    TEST_ASSERT_EQUAL_INT(3, ti->cursor);
    tui_text_input_destroy(ti);
}

void test_text_input_insert_at_cursor_mid(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('c', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'c', &ni);
    /* cursor is at 2, move left to 1 */
    ni = make_ncinput(NCKEY_LEFT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(1, ti->cursor);
    /* insert 'b' at cursor position 1 */
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    TEST_ASSERT_EQUAL_STRING("abc", tui_text_input_get(ti));
    TEST_ASSERT_EQUAL_INT(2, ti->cursor);
    tui_text_input_destroy(ti);
}

void test_text_input_non_printable_not_inserted(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;

    ni = make_ncinput(0x01, NCTYPE_UNKNOWN);
    TEST_ASSERT_FALSE(tui_text_input_handle_key(ti, 0x01, &ni));

    ni = make_ncinput(0x1F, NCTYPE_UNKNOWN);
    TEST_ASSERT_FALSE(tui_text_input_handle_key(ti, 0x1F, &ni));

    ni = make_ncinput(0x80, NCTYPE_UNKNOWN);
    TEST_ASSERT_FALSE(tui_text_input_handle_key(ti, 0x80, &ni));

    TEST_ASSERT_EQUAL_STRING("", tui_text_input_get(ti));
    tui_text_input_destroy(ti);
}

void test_text_input_not_focused_ignores_keys(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    /* focused is false by default */
    struct ncinput ni = make_ncinput('a', NCTYPE_UNKNOWN);
    TEST_ASSERT_FALSE(tui_text_input_handle_key(ti, 'a', &ni));
    TEST_ASSERT_EQUAL_STRING("", tui_text_input_get(ti));
    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  Navigation (4 tests)                                                */
/* ------------------------------------------------------------------ */

void test_text_input_cursor_left(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    ni = make_ncinput('c', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'c', &ni);
    TEST_ASSERT_EQUAL_INT(3, ti->cursor);

    ni = make_ncinput(NCKEY_LEFT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(2, ti->cursor);
    tui_text_input_destroy(ti);
}

void test_text_input_cursor_left_at_zero(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    TEST_ASSERT_EQUAL_INT(1, ti->cursor);

    ni = make_ncinput(NCKEY_LEFT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(0, ti->cursor);

    ni = make_ncinput(NCKEY_LEFT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(0, ti->cursor);  /* clamped */
    tui_text_input_destroy(ti);
}

void test_text_input_cursor_right(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    ni = make_ncinput('c', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'c', &ni);
    /* cursor at 3, go left twice to 1 */
    ni = make_ncinput(NCKEY_LEFT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    ni = make_ncinput(NCKEY_LEFT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(1, ti->cursor);

    ni = make_ncinput(NCKEY_RIGHT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(2, ti->cursor);
    tui_text_input_destroy(ti);
}

void test_text_input_cursor_right_at_end(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    ni = make_ncinput('c', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'c', &ni);
    TEST_ASSERT_EQUAL_INT(3, ti->cursor);

    ni = make_ncinput(NCKEY_RIGHT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(3, ti->cursor);  /* clamped */
    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  Backspace (3 tests)                                                 */
/* ------------------------------------------------------------------ */

void test_text_input_backspace_deletes_char(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    ni = make_ncinput('c', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'c', &ni);
    TEST_ASSERT_EQUAL_INT(3, ti->cursor);
    TEST_ASSERT_EQUAL_INT(3, ti->len);

    ni = make_ncinput(NCKEY_BACKSPACE, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_BACKSPACE, &ni);
    TEST_ASSERT_EQUAL_STRING("ab", tui_text_input_get(ti));
    TEST_ASSERT_EQUAL_INT(2, ti->cursor);
    TEST_ASSERT_EQUAL_INT(2, ti->len);
    tui_text_input_destroy(ti);
}

void test_text_input_backspace_at_start(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    /* move cursor to 0 */
    ni = make_ncinput(NCKEY_LEFT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(0, ti->cursor);

    ni = make_ncinput(NCKEY_BACKSPACE, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_BACKSPACE, &ni);
    TEST_ASSERT_EQUAL_STRING("a", tui_text_input_get(ti));
    TEST_ASSERT_EQUAL_INT(0, ti->cursor);
    TEST_ASSERT_EQUAL_INT(1, ti->len);
    tui_text_input_destroy(ti);
}

void test_text_input_backspace_mid_text(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    ni = make_ncinput('c', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'c', &ni);
    /* move left to cursor=2 */
    ni = make_ncinput(NCKEY_LEFT, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(2, ti->cursor);

    ni = make_ncinput(NCKEY_BACKSPACE, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_BACKSPACE, &ni);
    TEST_ASSERT_EQUAL_STRING("ac", tui_text_input_get(ti));
    TEST_ASSERT_EQUAL_INT(1, ti->cursor);
    TEST_ASSERT_EQUAL_INT(2, ti->len);
    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  Enter / Submit (3 tests)                                            */
/* ------------------------------------------------------------------ */

void test_text_input_enter_calls_on_submit(void) {
    reset_submit();
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40,
                                              test_submit_cb, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('h', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'h', &ni);
    ni = make_ncinput('e', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'e', &ni);
    ni = make_ncinput('l', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'l', &ni);
    ni = make_ncinput('l', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'l', &ni);
    ni = make_ncinput('o', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'o', &ni);

    ni = make_ncinput(NCKEY_ENTER, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_ENTER, &ni);

    TEST_ASSERT_TRUE(g_submit_called);
    TEST_ASSERT_EQUAL_STRING("hello", g_last_submit);
    /* Buffer cleared after submit */
    TEST_ASSERT_EQUAL_STRING("", tui_text_input_get(ti));
    tui_text_input_destroy(ti);
}

void test_text_input_enter_null_callback(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40,
                                              NULL, NULL);
    ti->focused = true;
    struct ncinput ni;
    ni = make_ncinput('t', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 't', &ni);
    ni = make_ncinput('e', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'e', &ni);
    ni = make_ncinput('s', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 's', &ni);
    ni = make_ncinput('t', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 't', &ni);

    ni = make_ncinput(NCKEY_ENTER, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, NCKEY_ENTER, &ni);  /* No crash */

    /* Buffer cleared even with NULL callback */
    TEST_ASSERT_EQUAL_STRING("", tui_text_input_get(ti));
    tui_text_input_destroy(ti);
}

void test_text_input_newline_variants(void) {
    reset_submit();
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40,
                                              test_submit_cb, NULL);
    ti->focused = true;
    struct ncinput ni;

    /* Test '\n' triggers submit */
    ni = make_ncinput('f', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'f', &ni);
    ni = make_ncinput('o', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'o', &ni);
    ni = make_ncinput('o', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'o', &ni);
    ni = make_ncinput('\n', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, '\n', &ni);
    TEST_ASSERT_TRUE(g_submit_called);
    TEST_ASSERT_EQUAL_STRING("foo", g_last_submit);

    /* Reset and test '\r' */
    reset_submit();
    ni = make_ncinput('b', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'b', &ni);
    ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    ni = make_ncinput('r', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'r', &ni);
    ni = make_ncinput('\r', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, '\r', &ni);
    TEST_ASSERT_TRUE(g_submit_called);
    TEST_ASSERT_EQUAL_STRING("bar", g_last_submit);

    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  Edge Cases (1 test)                                                 */
/* ------------------------------------------------------------------ */

void stub_set_plane_position(struct ncplane* plane, int y, int x);
void stub_set_default_plane_position(int y, int x);

void test_text_input_handle_mouse_click_sets_cursor(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    /* Type "hello" */
    struct ncinput ni;
    for (int i = 0; i < 5; i++) {
        ni = make_ncinput("hello"[i], NCTYPE_UNKNOWN);
        tui_text_input_handle_key(ti, "hello"[i], &ni);
    }
    TEST_ASSERT_EQUAL_INT(5, ti->len);
    TEST_ASSERT_EQUAL_INT(5, ti->cursor);

    /* Click at position to move cursor to index 2 */
    /* ncplane is at default (0,0), text starts at x=1, so click at x=3 => buf_pos=2 */
    stub_set_default_plane_position(0, 0);
    ni = make_ncinput(NCKEY_BUTTON1, NCTYPE_PRESS);
    ni.y = 0;
    ni.x = 3;
    bool result = tui_text_input_handle_mouse(ti, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, ti->cursor);
    tui_text_input_destroy(ti);
}

void test_text_input_handle_mouse_click_at_start(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'a', &ni);
    tui_text_input_handle_key(ti, 'b', &ni);
    tui_text_input_handle_key(ti, 'c', &ni);
    TEST_ASSERT_EQUAL_INT(3, ti->cursor);

    /* Click at x=1 (after "[" border, scroll_off=0) => buf_pos=0 */
    stub_set_default_plane_position(0, 0);
    ni = make_ncinput(NCKEY_BUTTON1, NCTYPE_PRESS);
    ni.y = 0; ni.x = 1;
    tui_text_input_handle_mouse(ti, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_EQUAL_INT(0, ti->cursor);
    tui_text_input_destroy(ti);
}

void test_text_input_handle_mouse_click_past_end(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;
    struct ncinput ni = make_ncinput('x', NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'x', &ni);
    TEST_ASSERT_EQUAL_INT(1, ti->len);

    /* Click at x=50 (way past text) => cursor clamped to len=1 */
    stub_set_default_plane_position(0, 0);
    ni = make_ncinput(NCKEY_BUTTON1, NCTYPE_PRESS);
    ni.y = 0; ni.x = 50;
    tui_text_input_handle_mouse(ti, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_EQUAL_INT(1, ti->cursor);
    tui_text_input_destroy(ti);
}

void test_text_input_handle_mouse_outside_plane_returns_false(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, NCTYPE_PRESS);
    ni.y = 99; ni.x = 99;  /* Way outside */
    bool result = tui_text_input_handle_mouse(ti, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_FALSE(result);
    tui_text_input_destroy(ti);
}

void test_text_input_handle_mouse_release_ignored(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, NCTYPE_RELEASE);
    ni.y = 0; ni.x = 3;
    bool result = tui_text_input_handle_mouse(ti, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_FALSE(result);
    tui_text_input_destroy(ti);
}

void test_text_input_release_event_ignored(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    ti->focused = true;

    /* Insert 'a' with normal event */
    struct ncinput ni = make_ncinput('a', NCTYPE_UNKNOWN);
    TEST_ASSERT_TRUE(tui_text_input_handle_key(ti, 'a', &ni));
    TEST_ASSERT_EQUAL_STRING("a", tui_text_input_get(ti));

    /* Release event should be ignored */
    ni = make_ncinput('b', NCTYPE_RELEASE);
    TEST_ASSERT_FALSE(tui_text_input_handle_key(ti, 'b', &ni));
    TEST_ASSERT_EQUAL_STRING("a", tui_text_input_get(ti));

    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();

    /* Create / Destroy */
    RUN_TEST(test_text_input_create_success);
    RUN_TEST(test_text_input_create_oom);
    RUN_TEST(test_text_input_destroy_null_safe);
    RUN_TEST(test_text_input_destroy_cleans_up);
    RUN_TEST(test_text_input_create_stores_callback_and_userdata);

    /* Get / Clear */
    RUN_TEST(test_text_input_get_returns_buffer);
    RUN_TEST(test_text_input_get_null_returns_null);
    RUN_TEST(test_text_input_clear_resets_all);
    RUN_TEST(test_text_input_clear_null_safe);

    /* Character Insertion */
    RUN_TEST(test_text_input_insert_single_char);
    RUN_TEST(test_text_input_insert_multiple_chars);
    RUN_TEST(test_text_input_insert_at_cursor_mid);
    RUN_TEST(test_text_input_non_printable_not_inserted);
    RUN_TEST(test_text_input_not_focused_ignores_keys);

    /* Navigation */
    RUN_TEST(test_text_input_cursor_left);
    RUN_TEST(test_text_input_cursor_left_at_zero);
    RUN_TEST(test_text_input_cursor_right);
    RUN_TEST(test_text_input_cursor_right_at_end);

    /* Backspace */
    RUN_TEST(test_text_input_backspace_deletes_char);
    RUN_TEST(test_text_input_backspace_at_start);
    RUN_TEST(test_text_input_backspace_mid_text);

    /* Enter / Submit */
    RUN_TEST(test_text_input_enter_calls_on_submit);
    RUN_TEST(test_text_input_enter_null_callback);
    RUN_TEST(test_text_input_newline_variants);

    /* Edge Cases */
    RUN_TEST(test_text_input_release_event_ignored);

    /* Mouse Handling */
    RUN_TEST(test_text_input_handle_mouse_click_sets_cursor);
    RUN_TEST(test_text_input_handle_mouse_click_at_start);
    RUN_TEST(test_text_input_handle_mouse_click_past_end);
    RUN_TEST(test_text_input_handle_mouse_outside_plane_returns_false);
    RUN_TEST(test_text_input_handle_mouse_release_ignored);

    return UNITY_END();
}
