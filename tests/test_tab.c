#include "unity.h"
#include "core/logger.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_tab.log";

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

/* --- Dummy object helpers --- */

static TuiWorkspace* make_dummy_workspace(void) {
    TuiWorkspace* ws = (TuiWorkspace*)calloc(1, sizeof(TuiWorkspace));
    ws->id = 999;
    strncpy(ws->name, "TestWS", sizeof(ws->name)-1);
    ws->name[sizeof(ws->name)-1] = '\0';
    ws->tab_count = 0;
    ws->tab_capacity = 4;
    ws->tabs = (TuiTab**)calloc(4, sizeof(TuiTab*));
    ws->active_tab_index = -1;
    /* Don't call ncplane_create — it returns a pointer that the real
       ncplane_destroy will try to dereference. Use a safe sentinel. */
    ws->ws_plane = (struct ncplane*)0xDEAD;
    return ws;
}

static void free_dummy_workspace(TuiWorkspace* ws) {
    if (!ws) return;
    free(ws->tabs);
    /* Don't call ncplane_destroy — it would crash on our sentinel */
    ws->ws_plane = NULL;
    free(ws);
}

static TuiWindow* make_dummy_window(void) {
    TuiWindow* win = (TuiWindow*)calloc(1, sizeof(TuiWindow));
    win->id = 999;
    win->plane = (struct ncplane*)0xBEEF;
    win->needs_redraw = true;
    win->focused = false;
    win->render_cb = NULL;
    win->on_destroy = NULL;
    win->text_input = NULL;
    win->user_data = NULL;
    return win;
}

/* free_dummy_window intentionally not defined — windows are freed via tui_tab_destroy */

/* --- Tests --- */

void test_tab_create_destroy(void) {
    TuiWorkspace* ws = make_dummy_workspace();
    TuiTab* tab = tui_tab_create(ws, "TestTab");
    TEST_ASSERT_NOT_NULL(tab);
    tui_tab_destroy(tab);
    free_dummy_workspace(ws);
}

void test_tab_create_null_name_returns_null(void) {
    TuiWorkspace* ws = make_dummy_workspace();
    TuiTab* tab = tui_tab_create(ws, NULL);
    TEST_ASSERT_NULL(tab);
    free_dummy_workspace(ws);
}

void test_tab_create_null_ws_returns_null(void) {
    TuiTab* tab = tui_tab_create(NULL, "TestTab");
    TEST_ASSERT_NULL(tab);
}

void test_tab_name_is_set(void) {
    TuiWorkspace* ws = make_dummy_workspace();
    TuiTab* tab = tui_tab_create(ws, "MyCustomTab");
    TEST_ASSERT_NOT_NULL(tab);
    TEST_ASSERT_TRUE(strstr(tui_tab_get_name(tab), "MyCustomTab") != NULL);
    tui_tab_destroy(tab);
    free_dummy_workspace(ws);
}

void test_tab_id_increments(void) {
    TuiWorkspace* ws1 = make_dummy_workspace();
    TuiWorkspace* ws2 = make_dummy_workspace();
    TuiTab* tab1 = tui_tab_create(ws1, "Tab1");
    TuiTab* tab2 = tui_tab_create(ws2, "Tab2");
    TEST_ASSERT_NOT_NULL(tab1);
    TEST_ASSERT_NOT_NULL(tab2);
    TEST_ASSERT_TRUE(tui_tab_get_id(tab1) != tui_tab_get_id(tab2));
    tui_tab_destroy(tab1);
    tui_tab_destroy(tab2);
    free_dummy_workspace(ws1);
    free_dummy_workspace(ws2);
}

void test_tab_getters(void) {
    TuiWorkspace* ws = make_dummy_workspace();
    TuiTab* tab = tui_tab_create(ws, "GetterTest");
    TEST_ASSERT_NOT_NULL(tab);
    TEST_ASSERT_TRUE(tui_tab_get_id(tab) > 0);
    TEST_ASSERT_NOT_NULL(tui_tab_get_name(tab));
    TEST_ASSERT_EQUAL_INT(0, tui_tab_get_window_count(tab));
    tui_tab_destroy(tab);
    free_dummy_workspace(ws);
}

void test_tab_add_window_increments_count(void) {
    TuiWorkspace* ws = make_dummy_workspace();
    TuiTab* tab = tui_tab_create(ws, "WindowTest");
    TEST_ASSERT_NOT_NULL(tab);
    TEST_ASSERT_EQUAL_INT(0, tui_tab_get_window_count(tab));

    TuiWindow* win = make_dummy_window();
    tui_tab_add_window(tab, win);
    TEST_ASSERT_EQUAL_INT(1, tui_tab_get_window_count(tab));

    tui_tab_destroy(tab);
    free_dummy_workspace(ws);
}

void test_tab_destroy_null_safe(void) {
    tui_tab_destroy(NULL);  // Should not crash
    TEST_ASSERT_TRUE(1);
}

void test_tab_add_null_window_does_not_crash(void) {
    TuiWorkspace* ws = make_dummy_workspace();
    TuiTab* tab = tui_tab_create(ws, "NullWinTest");
    tui_tab_add_window(tab, NULL);  // Should handle gracefully
    tui_tab_destroy(tab);
    free_dummy_workspace(ws);
}

void test_tab_get_null_tab_returns_safe_values(void) {
    TEST_ASSERT_NULL(tui_tab_get_name(NULL));
    TEST_ASSERT_EQUAL_INT(-1, tui_tab_get_id(NULL));
    TEST_ASSERT_EQUAL_INT(0, tui_tab_get_window_count(NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tab_create_destroy);
    RUN_TEST(test_tab_create_null_name_returns_null);
    RUN_TEST(test_tab_create_null_ws_returns_null);
    RUN_TEST(test_tab_name_is_set);
    RUN_TEST(test_tab_id_increments);
    RUN_TEST(test_tab_getters);
    RUN_TEST(test_tab_add_window_increments_count);
    RUN_TEST(test_tab_destroy_null_safe);
    RUN_TEST(test_tab_add_null_window_does_not_crash);
    RUN_TEST(test_tab_get_null_tab_returns_safe_values);
    return UNITY_END();
}
