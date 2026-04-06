#include "unity.h"
#include "core/logger.h"
#include "widget/radio_group.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_radio_group.log";

static bool g_cb_called = false;
static int g_cb_selected = 0;

static void radio_cb(int selected, void* ud) {
    (void)ud;
    g_cb_called = true;
    g_cb_selected = selected;
}

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

void setUp(void) {
    tui_logger_init(test_log_file);
    g_cb_called = false;
    g_cb_selected = 0;
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ---- CREATE/DESTROY ---- */

void test_radio_group_create_destroy(void) {
    TuiRadioGroup* rg = tui_radio_group_create(dummy_parent(), 0, 0, 20, 5, NULL, NULL);
    TEST_ASSERT_NOT_NULL(rg);
    TEST_ASSERT_EQUAL_INT(0, rg->count);
    TEST_ASSERT_EQUAL_INT(-1, rg->selected);
    tui_radio_group_destroy(rg);
}

void test_radio_group_create_null_parent(void) {
    TuiRadioGroup* rg = tui_radio_group_create(NULL, 0, 0, 20, 5, NULL, NULL);
    TEST_ASSERT_NULL(rg);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_radio_group_add_option(void) {
    TuiRadioGroup* rg = tui_radio_group_create(dummy_parent(), 0, 0, 20, 5, NULL, NULL);
    TEST_ASSERT_TRUE(tui_radio_group_add_option(rg, "Option A"));
    TEST_ASSERT_EQUAL_INT(1, tui_radio_group_get_count(rg));
    TEST_ASSERT_EQUAL_INT(0, tui_radio_group_get_selected(rg));
    tui_radio_group_destroy(rg);
}

void test_radio_group_add_multiple_options(void) {
    TuiRadioGroup* rg = tui_radio_group_create(dummy_parent(), 0, 0, 20, 5, NULL, NULL);
    tui_radio_group_add_option(rg, "A");
    tui_radio_group_add_option(rg, "B");
    tui_radio_group_add_option(rg, "C");
    TEST_ASSERT_EQUAL_INT(3, tui_radio_group_get_count(rg));
    TEST_ASSERT_EQUAL_INT(0, tui_radio_group_get_selected(rg));
    tui_radio_group_destroy(rg);
}

void test_radio_group_set_selected(void) {
    TuiRadioGroup* rg = tui_radio_group_create(dummy_parent(), 0, 0, 20, 5, NULL, NULL);
    tui_radio_group_add_option(rg, "A");
    tui_radio_group_add_option(rg, "B");
    tui_radio_group_add_option(rg, "C");

    tui_radio_group_set_selected(rg, 2);
    TEST_ASSERT_EQUAL_INT(2, tui_radio_group_get_selected(rg));
    tui_radio_group_destroy(rg);
}

void test_radio_group_handle_key_up_down(void) {
    TuiRadioGroup* rg = tui_radio_group_create(dummy_parent(), 0, 0, 20, 5, NULL, NULL);
    tui_radio_group_add_option(rg, "A");
    tui_radio_group_add_option(rg, "B");
    tui_radio_group_add_option(rg, "C");

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_radio_group_handle_key(rg, NCKEY_DOWN, &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_radio_group_get_selected(rg));

    tui_radio_group_handle_key(rg, NCKEY_UP, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_radio_group_get_selected(rg));
    tui_radio_group_destroy(rg);
}

void test_radio_group_handle_key_cycle(void) {
    TuiRadioGroup* rg = tui_radio_group_create(dummy_parent(), 0, 0, 20, 5, NULL, NULL);
    tui_radio_group_add_option(rg, "A");
    tui_radio_group_add_option(rg, "B");

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    /* From last, down -> first */
    tui_radio_group_set_selected(rg, 1);
    tui_radio_group_handle_key(rg, NCKEY_DOWN, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_radio_group_get_selected(rg));
    tui_radio_group_destroy(rg);
}

void test_radio_group_handle_key_space_select(void) {
    TuiRadioGroup* rg = tui_radio_group_create(dummy_parent(), 0, 0, 20, 5, radio_cb, NULL);
    tui_radio_group_add_option(rg, "A");
    tui_radio_group_add_option(rg, "B");
    tui_radio_group_set_selected(rg, 1);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_radio_group_handle_key(rg, ' ', &ni);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_INT(1, g_cb_selected);
    tui_radio_group_destroy(rg);
}

void test_radio_group_render_null_safe(void) {
    tui_radio_group_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_radio_group_null_safety(void) {
    tui_radio_group_destroy(NULL);
    TEST_ASSERT_EQUAL_INT(-1, tui_radio_group_get_selected(NULL));
    tui_radio_group_set_selected(NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, tui_radio_group_get_count(NULL));
    TEST_ASSERT_FALSE(tui_radio_group_add_option(NULL, "X"));
    tui_radio_group_set_focused(NULL, true);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_radio_group_handle_key(NULL, NCKEY_DOWN, &ni));
    TEST_ASSERT_FALSE(tui_radio_group_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_radio_group_create_destroy);
    RUN_TEST(test_radio_group_create_null_parent);
    RUN_TEST(test_radio_group_add_option);
    RUN_TEST(test_radio_group_add_multiple_options);
    RUN_TEST(test_radio_group_set_selected);
    RUN_TEST(test_radio_group_handle_key_up_down);
    RUN_TEST(test_radio_group_handle_key_cycle);
    RUN_TEST(test_radio_group_handle_key_space_select);
    RUN_TEST(test_radio_group_render_null_safe);
    RUN_TEST(test_radio_group_null_safety);
    return UNITY_END();
}
