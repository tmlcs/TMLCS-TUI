#include "unity.h"
#include "core/logger.h"
#include "widget/slider.h"
#include "core/types.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_slider.log";

static bool g_cb_called = false;
static float g_cb_value = 0.0f;

static void slider_cb(float value, void* ud) {
    (void)ud;
    g_cb_called = true;
    g_cb_value = value;
}

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

void setUp(void) {
    tui_logger_init(test_log_file);
    g_cb_called = false;
    g_cb_value = 0.0f;
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ---- CREATE/DESTROY ---- */

void test_slider_create_destroy(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 1.0f, NULL, NULL);
    TEST_ASSERT_NOT_NULL(sl);
    TEST_ASSERT_EQUAL_INT(0.0f, sl->min);
    TEST_ASSERT_EQUAL_INT(100.0f, sl->max);
    tui_slider_destroy(sl);
}

void test_slider_create_null_parent(void) {
    TuiSlider* sl = tui_slider_create(NULL, 0, 0, 20, 0.0f, 100.0f, 1.0f, NULL, NULL);
    TEST_ASSERT_NULL(sl);
}

void test_slider_create_invalid_range(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 100.0f, 0.0f, 1.0f, NULL, NULL);
    TEST_ASSERT_NULL(sl);
}

void test_slider_create_narrow_width(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 2, 0.0f, 100.0f, 1.0f, NULL, NULL);
    TEST_ASSERT_NULL(sl);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_slider_get_set_value(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 5.0f, NULL, NULL);
    TEST_ASSERT_NOT_NULL(sl);
    TEST_ASSERT_TRUE(tui_slider_get_value(sl) >= 0.0f - 0.01f && tui_slider_get_value(sl) <= 0.0f + 0.01f);
    tui_slider_set_value(sl, 50.0f);
    TEST_ASSERT_TRUE(tui_slider_get_value(sl) >= 50.0f - 0.01f && tui_slider_get_value(sl) <= 50.0f + 0.01f);
    tui_slider_destroy(sl);
}

void test_slider_value_clamped_low(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 1.0f, NULL, NULL);
    tui_slider_set_value(sl, -50.0f);
    TEST_ASSERT_TRUE(tui_slider_get_value(sl) >= 0.0f - 0.01f && tui_slider_get_value(sl) <= 0.0f + 0.01f);
    tui_slider_destroy(sl);
}

void test_slider_value_clamped_high(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 1.0f, NULL, NULL);
    tui_slider_set_value(sl, 200.0f);
    TEST_ASSERT_TRUE(tui_slider_get_value(sl) >= 100.0f - 0.01f && tui_slider_get_value(sl) <= 100.0f + 0.01f);
    tui_slider_destroy(sl);
}

void test_slider_handle_key_left_right(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 5.0f, NULL, NULL);
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_slider_set_value(sl, 50.0f);
    tui_slider_set_focused(sl, true);
    bool consumed = tui_slider_handle_key(sl, NCKEY_RIGHT, &ni);
    TEST_ASSERT_TRUE(consumed);
    TEST_ASSERT_TRUE(tui_slider_get_value(sl) >= 55.0f - 0.01f && tui_slider_get_value(sl) <= 55.0f + 0.01f);

    consumed = tui_slider_handle_key(sl, NCKEY_LEFT, &ni);
    TEST_ASSERT_TRUE(consumed);
    TEST_ASSERT_TRUE(tui_slider_get_value(sl) >= 50.0f - 0.01f && tui_slider_get_value(sl) <= 50.0f + 0.01f);
    tui_slider_destroy(sl);
}

void test_slider_handle_key_home_end(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 5.0f, NULL, NULL);
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_slider_set_focused(sl, true);
    tui_slider_handle_key(sl, NCKEY_END, &ni);
    TEST_ASSERT_TRUE(tui_slider_get_value(sl) >= 100.0f - 0.01f && tui_slider_get_value(sl) <= 100.0f + 0.01f);

    tui_slider_handle_key(sl, NCKEY_HOME, &ni);
    TEST_ASSERT_TRUE(tui_slider_get_value(sl) >= 0.0f - 0.01f && tui_slider_get_value(sl) <= 0.0f + 0.01f);
    tui_slider_destroy(sl);
}

void test_slider_handle_key_callbacks(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 5.0f, slider_cb, NULL);
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_slider_set_focused(sl, true);
    tui_slider_handle_key(sl, NCKEY_RIGHT, &ni);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_TRUE(g_cb_value >= 5.0f - 0.01f && g_cb_value <= 5.0f + 0.01f);
    tui_slider_destroy(sl);
}

void test_slider_handle_key_unknown_key(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 1.0f, NULL, NULL);
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    bool consumed = tui_slider_handle_key(sl, 'z', &ni);
    TEST_ASSERT_FALSE(consumed);
    tui_slider_destroy(sl);
}

void test_slider_render_null_safe(void) {
    tui_slider_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_slider_set_focused(void) {
    TuiSlider* sl = tui_slider_create(dummy_parent(), 0, 0, 20, 0.0f, 100.0f, 1.0f, NULL, NULL);
    tui_slider_set_focused(sl, true);
    TEST_ASSERT_TRUE(sl->focused);
    tui_slider_set_focused(sl, false);
    TEST_ASSERT_FALSE(sl->focused);
    tui_slider_destroy(sl);
}

/* ---- NULL SAFETY ---- */

void test_slider_null_safety(void) {
    tui_slider_destroy(NULL);
    TEST_ASSERT_TRUE(tui_slider_get_value(NULL) == 0.0f);
    tui_slider_set_value(NULL, 50.0f);
    tui_slider_set_focused(NULL, true);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_slider_handle_key(NULL, NCKEY_RIGHT, &ni));
    TEST_ASSERT_FALSE(tui_slider_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_slider_create_destroy);
    RUN_TEST(test_slider_create_null_parent);
    RUN_TEST(test_slider_create_invalid_range);
    RUN_TEST(test_slider_create_narrow_width);
    RUN_TEST(test_slider_get_set_value);
    RUN_TEST(test_slider_value_clamped_low);
    RUN_TEST(test_slider_value_clamped_high);
    RUN_TEST(test_slider_handle_key_left_right);
    RUN_TEST(test_slider_handle_key_home_end);
    RUN_TEST(test_slider_handle_key_callbacks);
    RUN_TEST(test_slider_handle_key_unknown_key);
    RUN_TEST(test_slider_render_null_safe);
    RUN_TEST(test_slider_set_focused);
    RUN_TEST(test_slider_null_safety);
    return UNITY_END();
}
