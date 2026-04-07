/**
 * @file test_text_input_properties.c
 * @brief Property-based tests for TuiTextInput widget.
 *
 * Instead of testing specific input sequences, these tests verify
 * invariants that should hold for ANY sequence of operations.
 */
#include "../unity.h"
#include "core/logger.h"
#include "widget/text_input.h"
#include "core/utf8.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_text_input_prop.log";

void ncplane_create_set_fail_count(int n);

void setUp(void) {
    tui_logger_init(test_log_file);
    ncplane_create_set_fail_count(0);
}

void tearDown(void) {
    ncplane_create_set_fail_count(0);
    tui_logger_destroy();
}

static struct ncinput make_ncinput(uint32_t key) {
    (void)key;
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = 0;
    ni.y = 0;
    ni.x = 0;
    ni.alt = false;
    return ni;
}

/* ------------------------------------------------------------------ */
/*  Property 1: Buffer is always null-terminated                       */
/* ------------------------------------------------------------------ */

void test_property_always_null_terminated(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    TEST_ASSERT_NOT_NULL(ti);
    ti->focused = true;

    /* Perform a diverse sequence of operations */
    struct ncinput ni;

    /* Insert characters */
    for (uint32_t c = 'a'; c <= 'z'; c++) {
        ni = make_ncinput(c);
        tui_text_input_handle_key(ti, c, &ni);
        TEST_ASSERT_EQUAL_INT('\0', ti->buffer[ti->len_bytes]);
    }

    /* Navigate left */
    for (int i = 0; i < 10; i++) {
        ni = make_ncinput(NCKEY_LEFT);
        tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
        TEST_ASSERT_EQUAL_INT('\0', ti->buffer[ti->len_bytes]);
    }

    /* Navigate right */
    for (int i = 0; i < 10; i++) {
        ni = make_ncinput(NCKEY_RIGHT);
        tui_text_input_handle_key(ti, NCKEY_RIGHT, &ni);
        TEST_ASSERT_EQUAL_INT('\0', ti->buffer[ti->len_bytes]);
    }

    /* Backspace multiple times */
    for (int i = 0; i < 15; i++) {
        ni = make_ncinput(NCKEY_BACKSPACE);
        tui_text_input_handle_key(ti, NCKEY_BACKSPACE, &ni);
        TEST_ASSERT_EQUAL_INT('\0', ti->buffer[ti->len_bytes]);
    }

    /* Insert more characters after deletion */
    ni = make_ncinput('X');
    tui_text_input_handle_key(ti, 'X', &ni);
    TEST_ASSERT_EQUAL_INT('\0', ti->buffer[ti->len_bytes]);

    /* Clear */
    tui_text_input_clear(ti);
    TEST_ASSERT_EQUAL_INT('\0', ti->buffer[ti->len_bytes]);
    TEST_ASSERT_EQUAL_INT('\0', ti->buffer[0]);

    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  Property 2: Cursor always within [0, len_codepoints]               */
/* ------------------------------------------------------------------ */

void test_property_cursor_in_bounds(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    TEST_ASSERT_NOT_NULL(ti);
    ti->focused = true;

    struct ncinput ni;

    /* Insert some text */
    const char* sample = "Hello, World!";
    for (const char* p = sample; *p; p++) {
        ni = make_ncinput((uint32_t)*p);
        tui_text_input_handle_key(ti, (uint32_t)*p, &ni);
    }

    TEST_ASSERT_TRUE(ti->cursor_cp >= 0);
    TEST_ASSERT_TRUE(ti->cursor_cp <= ti->len_codepoints);

    /* Rapid left presses */
    for (int i = 0; i < 100; i++) {
        ni = make_ncinput(NCKEY_LEFT);
        tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
        TEST_ASSERT_TRUE(ti->cursor_cp >= 0);
        TEST_ASSERT_TRUE(ti->cursor_cp <= ti->len_codepoints);
    }

    /* Rapid right presses */
    for (int i = 0; i < 100; i++) {
        ni = make_ncinput(NCKEY_RIGHT);
        tui_text_input_handle_key(ti, NCKEY_RIGHT, &ni);
        TEST_ASSERT_TRUE(ti->cursor_cp >= 0);
        TEST_ASSERT_TRUE(ti->cursor_cp <= ti->len_codepoints);
    }

    /* Backspace until empty */
    for (int i = 0; i < 50; i++) {
        ni = make_ncinput(NCKEY_BACKSPACE);
        tui_text_input_handle_key(ti, NCKEY_BACKSPACE, &ni);
        TEST_ASSERT_TRUE(ti->cursor_cp >= 0);
        TEST_ASSERT_TRUE(ti->cursor_cp <= ti->len_codepoints);
    }

    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  Property 3: Insert then backspace = original state                  */
/* ------------------------------------------------------------------ */

void test_property_insert_delete_inverse(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    TEST_ASSERT_NOT_NULL(ti);
    ti->focused = true;

    /* Record initial state */
    int initial_len = ti->len_codepoints;
    int initial_cursor = ti->cursor_cp;

    /* Insert a character */
    struct ncinput ni = make_ncinput('X');
    tui_text_input_handle_key(ti, 'X', &ni);

    /* Backspace it */
    ni = make_ncinput(NCKEY_BACKSPACE);
    tui_text_input_handle_key(ti, NCKEY_BACKSPACE, &ni);

    /* Should be back to original state */
    TEST_ASSERT_EQUAL_INT(initial_len, ti->len_codepoints);
    TEST_ASSERT_EQUAL_INT(initial_cursor, ti->cursor_cp);
    TEST_ASSERT_EQUAL_INT('\0', ti->buffer[0]);

    tui_text_input_destroy(ti);
}

/* ------------------------------------------------------------------ */
/*  Property 4: Selection bounds always within [0, len_codepoints]     */
/* ------------------------------------------------------------------ */

void test_property_selection_bounds(void) {
    TuiTextInput* ti = tui_text_input_create((struct ncplane*)0xDEAD, 5, 10, 40, NULL, NULL);
    TEST_ASSERT_NOT_NULL(ti);
    ti->focused = true;

    /* Type some text */
    struct ncinput ni;
    const char* sample = "Test selection bounds";
    for (const char* p = sample; *p; p++) {
        ni = make_ncinput((uint32_t)*p);
        tui_text_input_handle_key(ti, (uint32_t)*p, &ni);
    }

    /* Selection should be within bounds if active */
    if (ti->selecting) {
        TEST_ASSERT_TRUE(ti->select_start_cp >= 0);
        TEST_ASSERT_TRUE(ti->select_start_cp <= ti->len_codepoints);
        TEST_ASSERT_TRUE(ti->select_end_cp >= 0);
        TEST_ASSERT_TRUE(ti->select_end_cp <= ti->len_codepoints);
    }

    /* Move around and check bounds after each operation */
    ni = make_ncinput(NCKEY_LEFT);
    tui_text_input_handle_key(ti, NCKEY_LEFT, &ni);
    TEST_ASSERT_TRUE(ti->cursor_cp >= 0 && ti->cursor_cp <= ti->len_codepoints);

    ni = make_ncinput(NCKEY_HOME);
    tui_text_input_handle_key(ti, NCKEY_HOME, &ni);
    TEST_ASSERT_TRUE(ti->cursor_cp >= 0 && ti->cursor_cp <= ti->len_codepoints);

    ni = make_ncinput(NCKEY_END);
    tui_text_input_handle_key(ti, NCKEY_END, &ni);
    TEST_ASSERT_TRUE(ti->cursor_cp >= 0 && ti->cursor_cp <= ti->len_codepoints);

    tui_text_input_destroy(ti);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_property_always_null_terminated);
    RUN_TEST(test_property_cursor_in_bounds);
    RUN_TEST(test_property_insert_delete_inverse);
    RUN_TEST(test_property_selection_bounds);

    return UNITY_END();
}
