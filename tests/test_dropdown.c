#include "unity.h"
#include "core/logger.h"
#include "widget/dropdown.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_dropdown.log";

static bool g_cb_called = false;
static int g_cb_selected = 0;

static void dd_cb(int selected, void* ud) {
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

void test_dropdown_create_destroy(void) {
    TuiDropdown* dd = tui_dropdown_create(dummy_parent(), 0, 0, 20, NULL, NULL);
    TEST_ASSERT_NOT_NULL(dd);
    TEST_ASSERT_EQUAL_INT(0, dd->count);
    TEST_ASSERT_FALSE(dd->open);
    tui_dropdown_destroy(dd);
}

void test_dropdown_create_null_parent(void) {
    TuiDropdown* dd = tui_dropdown_create(NULL, 0, 0, 20, NULL, NULL);
    TEST_ASSERT_NULL(dd);
}

void test_dropdown_create_too_narrow(void) {
    TuiDropdown* dd = tui_dropdown_create(dummy_parent(), 0, 0, 2, NULL, NULL);
    TEST_ASSERT_NULL(dd);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_dropdown_add_item(void) {
    TuiDropdown* dd = tui_dropdown_create(dummy_parent(), 0, 0, 20, NULL, NULL);
    TEST_ASSERT_TRUE(tui_dropdown_add_item(dd, "Item 1"));
    TEST_ASSERT_EQUAL_INT(1, tui_dropdown_get_count(dd));
    TEST_ASSERT_EQUAL_INT(0, tui_dropdown_get_selected(dd));
    tui_dropdown_destroy(dd);
}

void test_dropdown_add_multiple_items(void) {
    TuiDropdown* dd = tui_dropdown_create(dummy_parent(), 0, 0, 20, NULL, NULL);
    tui_dropdown_add_item(dd, "Alpha");
    tui_dropdown_add_item(dd, "Beta");
    tui_dropdown_add_item(dd, "Gamma");
    TEST_ASSERT_EQUAL_INT(3, tui_dropdown_get_count(dd));
    tui_dropdown_destroy(dd);
}

void test_dropdown_open_close_key(void) {
    TuiDropdown* dd = tui_dropdown_create(dummy_parent(), 0, 0, 20, NULL, NULL);
    tui_dropdown_add_item(dd, "A");
    tui_dropdown_add_item(dd, "B");
    TEST_ASSERT_FALSE(tui_dropdown_is_open(dd));

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_dropdown_handle_key(dd, NCKEY_ENTER, &ni);
    TEST_ASSERT_TRUE(tui_dropdown_is_open(dd));

    tui_dropdown_handle_key(dd, NCKEY_ESC, &ni);
    TEST_ASSERT_FALSE(tui_dropdown_is_open(dd));
    tui_dropdown_destroy(dd);
}

void test_dropdown_navigate_up_down(void) {
    TuiDropdown* dd = tui_dropdown_create(dummy_parent(), 0, 0, 20, NULL, NULL);
    tui_dropdown_add_item(dd, "A");
    tui_dropdown_add_item(dd, "B");
    tui_dropdown_add_item(dd, "C");

    /* Open dropdown */
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    tui_dropdown_handle_key(dd, NCKEY_ENTER, &ni);

    tui_dropdown_handle_key(dd, NCKEY_DOWN, &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_dropdown_get_selected(dd));

    tui_dropdown_handle_key(dd, NCKEY_UP, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_dropdown_get_selected(dd));
    tui_dropdown_destroy(dd);
}

void test_dropdown_select_with_enter(void) {
    TuiDropdown* dd = tui_dropdown_create(dummy_parent(), 0, 0, 20, dd_cb, NULL);
    tui_dropdown_add_item(dd, "A");
    tui_dropdown_add_item(dd, "B");

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_dropdown_handle_key(dd, NCKEY_DOWN, &ni); /* Open */
    tui_dropdown_handle_key(dd, NCKEY_DOWN, &ni); /* Navigate down */
    tui_dropdown_handle_key(dd, NCKEY_ENTER, &ni); /* Select and close */
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_INT(1, g_cb_selected);
    TEST_ASSERT_FALSE(tui_dropdown_is_open(dd));
    tui_dropdown_destroy(dd);
}

void test_dropdown_render_null_safe(void) {
    tui_dropdown_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_dropdown_null_safety(void) {
    tui_dropdown_destroy(NULL);
    TEST_ASSERT_EQUAL_INT(-1, tui_dropdown_get_selected(NULL));
    tui_dropdown_set_selected(NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, tui_dropdown_get_count(NULL));
    TEST_ASSERT_FALSE(tui_dropdown_add_item(NULL, "X"));
    tui_dropdown_set_focused(NULL, true);
    TEST_ASSERT_FALSE(tui_dropdown_is_open(NULL));

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_dropdown_handle_key(NULL, NCKEY_ENTER, &ni));
    TEST_ASSERT_FALSE(tui_dropdown_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dropdown_create_destroy);
    RUN_TEST(test_dropdown_create_null_parent);
    RUN_TEST(test_dropdown_create_too_narrow);
    RUN_TEST(test_dropdown_add_item);
    RUN_TEST(test_dropdown_add_multiple_items);
    RUN_TEST(test_dropdown_open_close_key);
    RUN_TEST(test_dropdown_navigate_up_down);
    RUN_TEST(test_dropdown_select_with_enter);
    RUN_TEST(test_dropdown_render_null_safe);
    RUN_TEST(test_dropdown_null_safety);
    return UNITY_END();
}
