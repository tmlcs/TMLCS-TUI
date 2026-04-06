#include "unity.h"
#include "core/logger.h"
#include "widget/spinner.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_spinner.log";

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ---- CREATE/DESTROY ---- */

void test_spinner_create_destroy(void) {
    TuiSpinner* sp = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_DOTS);
    TEST_ASSERT_NOT_NULL(sp);
    TEST_ASSERT_FALSE(sp->running);
    tui_spinner_destroy(sp);
}

void test_spinner_create_null_parent(void) {
    TuiSpinner* sp = tui_spinner_create(NULL, 0, 0, SPINNER_LINE);
    TEST_ASSERT_NULL(sp);
}

void test_spinner_create_all_types(void) {
    TuiSpinner* s1 = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_DOTS);
    TEST_ASSERT_NOT_NULL(s1);
    tui_spinner_destroy(s1);

    TuiSpinner* s2 = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_LINE);
    TEST_ASSERT_NOT_NULL(s2);
    tui_spinner_destroy(s2);

    TuiSpinner* s3 = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_ARROW);
    TEST_ASSERT_NOT_NULL(s3);
    tui_spinner_destroy(s3);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_spinner_start_stop(void) {
    TuiSpinner* sp = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_LINE);
    TEST_ASSERT_FALSE(tui_spinner_is_running(sp));

    tui_spinner_start(sp);
    TEST_ASSERT_TRUE(tui_spinner_is_running(sp));

    tui_spinner_stop(sp);
    TEST_ASSERT_FALSE(tui_spinner_is_running(sp));
    tui_spinner_destroy(sp);
}

void test_spinner_tick(void) {
    TuiSpinner* sp = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_LINE);
    int initial_frame = sp->frame;
    tui_spinner_start(sp);
    tui_spinner_tick(sp);
    TEST_ASSERT_EQUAL_INT((initial_frame + 1) % sp->frame_count, sp->frame);
    tui_spinner_destroy(sp);
}

void test_spinner_tick_not_running(void) {
    TuiSpinner* sp = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_LINE);
    int initial_frame = sp->frame;
    tui_spinner_tick(sp); /* Not running, should not change */
    TEST_ASSERT_EQUAL_INT(initial_frame, sp->frame);
    tui_spinner_destroy(sp);
}

void test_spinner_render_null_safe(void) {
    tui_spinner_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_spinner_handle_key_returns_false(void) {
    TuiSpinner* sp = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_LINE);
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_spinner_handle_key(sp, NCKEY_ENTER, &ni));
    tui_spinner_destroy(sp);
}

void test_spinner_handle_mouse_returns_false(void) {
    TuiSpinner* sp = tui_spinner_create(dummy_parent(), 0, 0, SPINNER_LINE);
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_spinner_handle_mouse(sp, NCKEY_BUTTON1, &ni));
    tui_spinner_destroy(sp);
}

/* ---- NULL SAFETY ---- */

void test_spinner_null_safety(void) {
    tui_spinner_destroy(NULL);
    TEST_ASSERT_FALSE(tui_spinner_is_running(NULL));
    tui_spinner_start(NULL);
    tui_spinner_stop(NULL);
    tui_spinner_tick(NULL);
    tui_spinner_render(NULL);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_spinner_handle_key(NULL, 'x', &ni));
    TEST_ASSERT_FALSE(tui_spinner_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_spinner_create_destroy);
    RUN_TEST(test_spinner_create_null_parent);
    RUN_TEST(test_spinner_create_all_types);
    RUN_TEST(test_spinner_start_stop);
    RUN_TEST(test_spinner_tick);
    RUN_TEST(test_spinner_tick_not_running);
    RUN_TEST(test_spinner_render_null_safe);
    RUN_TEST(test_spinner_handle_key_returns_false);
    RUN_TEST(test_spinner_handle_mouse_returns_false);
    RUN_TEST(test_spinner_null_safety);
    return UNITY_END();
}
