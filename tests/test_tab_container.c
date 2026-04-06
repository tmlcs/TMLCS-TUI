#include "unity.h"
#include "core/logger.h"
#include "widget/tab_container.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_tab_container.log";

static bool g_cb_called = false;
static int g_cb_tab = 0;

static void tab_cb(int tab_index, void* ud) {
    (void)ud;
    g_cb_called = true;
    g_cb_tab = tab_index;
}

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

void setUp(void) {
    tui_logger_init(test_log_file);
    g_cb_called = false;
    g_cb_tab = 0;
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ---- CREATE/DESTROY ---- */

void test_tab_container_create_destroy(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    TEST_ASSERT_NOT_NULL(tc);
    TEST_ASSERT_EQUAL_INT(0, tc->tab_count);
    TEST_ASSERT_EQUAL_INT(-1, tc->active_tab);
    tui_tab_container_destroy(tc);
}

void test_tab_container_create_null_parent(void) {
    TuiTabContainer* tc = tui_tab_container_create(NULL, 0, 0, 40, 10, NULL, NULL);
    TEST_ASSERT_NULL(tc);
}

void test_tab_container_create_too_narrow(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 5, 10, NULL, NULL);
    TEST_ASSERT_NULL(tc);
}

void test_tab_container_create_too_short(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 40, 2, NULL, NULL);
    TEST_ASSERT_NULL(tc);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_tab_container_add_tab(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    TEST_ASSERT_TRUE(tui_tab_container_add_tab(tc, "Tab1"));
    TEST_ASSERT_EQUAL_INT(1, tui_tab_container_get_count(tc));
    TEST_ASSERT_EQUAL_INT(0, tui_tab_container_get_active(tc));
    tui_tab_container_destroy(tc);
}

void test_tab_container_add_multiple_tabs(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    tui_tab_container_add_tab(tc, "Tab1");
    tui_tab_container_add_tab(tc, "Tab2");
    tui_tab_container_add_tab(tc, "Tab3");
    TEST_ASSERT_EQUAL_INT(3, tui_tab_container_get_count(tc));
    tui_tab_container_destroy(tc);
}

void test_tab_container_set_active(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    tui_tab_container_add_tab(tc, "Tab1");
    tui_tab_container_add_tab(tc, "Tab2");
    tui_tab_container_add_tab(tc, "Tab3");

    tui_tab_container_set_active(tc, 2);
    TEST_ASSERT_EQUAL_INT(2, tui_tab_container_get_active(tc));
    tui_tab_container_destroy(tc);
}

void test_tab_container_handle_key_left_right(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    tui_tab_container_add_tab(tc, "Tab1");
    tui_tab_container_add_tab(tc, "Tab2");

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_tab_container_handle_key(tc, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_tab_container_get_active(tc));

    tui_tab_container_handle_key(tc, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_tab_container_get_active(tc));
    tui_tab_container_destroy(tc);
}

void test_tab_container_handle_key_cycle(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    tui_tab_container_add_tab(tc, "Tab1");
    tui_tab_container_add_tab(tc, "Tab2");

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    /* From last tab, right -> first */
    tui_tab_container_set_active(tc, 1);
    tui_tab_container_handle_key(tc, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_tab_container_get_active(tc));
    tui_tab_container_destroy(tc);
}

void test_tab_container_handle_key_callbacks(void) {
    TuiTabContainer* tc = tui_tab_container_create(dummy_parent(), 0, 0, 40, 10, tab_cb, NULL);
    tui_tab_container_add_tab(tc, "Tab1");
    tui_tab_container_add_tab(tc, "Tab2");

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_tab_container_handle_key(tc, NCKEY_RIGHT, &ni);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_INT(1, g_cb_tab);
    tui_tab_container_destroy(tc);
}

void test_tab_container_render_null_safe(void) {
    tui_tab_container_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_tab_container_null_safety(void) {
    tui_tab_container_destroy(NULL);
    TEST_ASSERT_EQUAL_INT(-1, tui_tab_container_get_active(NULL));
    tui_tab_container_set_active(NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, tui_tab_container_get_count(NULL));
    TEST_ASSERT_FALSE(tui_tab_container_add_tab(NULL, "X"));
    tui_tab_container_set_focused(NULL, true);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_tab_container_handle_key(NULL, NCKEY_LEFT, &ni));
    TEST_ASSERT_FALSE(tui_tab_container_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tab_container_create_destroy);
    RUN_TEST(test_tab_container_create_null_parent);
    RUN_TEST(test_tab_container_create_too_narrow);
    RUN_TEST(test_tab_container_create_too_short);
    RUN_TEST(test_tab_container_add_tab);
    RUN_TEST(test_tab_container_add_multiple_tabs);
    RUN_TEST(test_tab_container_set_active);
    RUN_TEST(test_tab_container_handle_key_left_right);
    RUN_TEST(test_tab_container_handle_key_cycle);
    RUN_TEST(test_tab_container_handle_key_callbacks);
    RUN_TEST(test_tab_container_render_null_safe);
    RUN_TEST(test_tab_container_null_safety);
    return UNITY_END();
}
