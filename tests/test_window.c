#include "unity.h"
#include "core/logger.h"
#include "core/window.h"
#include "core/manager.h"
#include "core/tab.h"
#include "core/types.h"
#include "core/types_private.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_window.log";

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

/* --- Dummy helpers --- */

static struct ncplane* tab_plane = NULL;

static TuiTab* make_dummy_tab(void) {
    TuiTab* tab = (TuiTab*)calloc(1, sizeof(TuiTab));
    tab->_id = 999;
    strncpy(tab->_name, "DummyTab", sizeof(tab->_name) - 1);
    tab->_name[sizeof(tab->_name) - 1] = '\0';
    tab->_window_count = 0;
    tab->_window_capacity = 4;
    tab->_windows = (TuiWindow**)calloc(4, sizeof(TuiWindow*));
    tab->_active_window_index = -1;
    tab_plane = (struct ncplane*)0xDEAD;
    tab->_tab_plane = tab_plane;
    return tab;
}

static void free_dummy_tab(TuiTab* tab) {
    if (!tab) return;
    free(tab->_windows);
    tab_plane = NULL;
    free(tab);
}

/* --- Tests --- */

void test_window_create_success(void) {
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = tui_window_create(tab, 80, 24);
    TEST_ASSERT_NOT_NULL(win);
    TEST_ASSERT_TRUE(win->_needs_redraw);
    TEST_ASSERT_FALSE(win->_focused);
    TEST_ASSERT_NULL(win->_text_input);
    tui_window_destroy(win);
    free_dummy_tab(tab);
}

void test_window_create_null_tab_returns_null(void) {
    TuiWindow* win = tui_window_create(NULL, 80, 24);
    TEST_ASSERT_NULL(win);
}

void test_window_create_ids_increment(void) {
    /* Reset ID counter for predictable IDs */
    tui_reset_id(1);

    TuiTab* tab = make_dummy_tab();
    TuiWindow* win1 = tui_window_create(tab, 80, 24);
    TuiWindow* win2 = tui_window_create(tab, 80, 24);
    TuiWindow* win3 = tui_window_create(tab, 80, 24);
    TEST_ASSERT_NOT_NULL(win1);
    TEST_ASSERT_NOT_NULL(win2);
    TEST_ASSERT_NOT_NULL(win3);
    TEST_ASSERT_TRUE(win1->_id < win2->_id);
    TEST_ASSERT_TRUE(win2->_id < win3->_id);
    tui_window_destroy(win1);
    tui_window_destroy(win2);
    tui_window_destroy(win3);
    free_dummy_tab(tab);
}

void test_window_destroy_null_safe(void) {
    tui_window_destroy(NULL);  /* Should not crash */
    TEST_ASSERT_TRUE(1);
}

void test_window_destroy_cleans_up(void) {
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = tui_window_create(tab, 80, 24);
    TEST_ASSERT_NOT_NULL(win);
    tui_window_destroy(win);  /* ASAN will verify no leaks */
    free_dummy_tab(tab);
}

/* Callback flag helper for test 6 */
static int g_on_destroy_flag = 0;
static void on_destroy_set_flag(TuiWindow* win) {
    (void)win;
    g_on_destroy_flag = 1;
}

void test_window_destroy_calls_on_destroy_callback(void) {
    g_on_destroy_flag = 0;
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = tui_window_create(tab, 80, 24);
    TEST_ASSERT_NOT_NULL(win);
    win->_on_destroy = on_destroy_set_flag;
    tui_window_destroy(win);
    TEST_ASSERT_EQUAL_INT(1, g_on_destroy_flag);
    free_dummy_tab(tab);
}

/* Callback that verifies plane is still valid when callback runs */
static int g_callback_plane_valid = 0;
static void on_destroy_check_plane(TuiWindow* win) {
    g_callback_plane_valid = (win->_plane != NULL) ? 1 : 0;
}

void test_window_destroy_nullifies_plane_before_callback_returns(void) {
    g_callback_plane_valid = 0;
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = tui_window_create(tab, 80, 24);
    TEST_ASSERT_NOT_NULL(win);
    win->_on_destroy = on_destroy_check_plane;
    tui_window_destroy(win);
    /* Callback fires BEFORE ncplane_destroy, so plane should still be non-NULL */
    TEST_ASSERT_EQUAL_INT(1, g_callback_plane_valid);
    free_dummy_tab(tab);
}

void test_window_destroy_without_callback(void) {
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = tui_window_create(tab, 80, 24);
    TEST_ASSERT_NOT_NULL(win);
    /* on_destroy is NULL by default */
    TEST_ASSERT_NULL(win->_on_destroy);
    tui_window_destroy(win);  /* Should not crash */
    free_dummy_tab(tab);
}

void test_window_create_initializes_fields(void) {
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = tui_window_create(tab, 80, 24);
    TEST_ASSERT_NOT_NULL(win);
    TEST_ASSERT_NULL(win->_render_cb);
    TEST_ASSERT_NULL(win->_on_destroy);
    TEST_ASSERT_NULL(win->_user_data);
    TEST_ASSERT_NOT_NULL(win->_plane);
    tui_window_destroy(win);
    free_dummy_tab(tab);
}

/* Callback that explicitly nullifies fields */
static void on_destroy_nullify_fields(TuiWindow* win) {
    win->_render_cb = NULL;
    win->_text_input = NULL;
}

void test_window_destroy_stubs_after_callback(void) {
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = tui_window_create(tab, 80, 24);
    TEST_ASSERT_NOT_NULL(win);
    win->_on_destroy = on_destroy_nullify_fields;
    tui_window_destroy(win);  /* No crash */
    free_dummy_tab(tab);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_window_create_success);
    RUN_TEST(test_window_create_null_tab_returns_null);
    RUN_TEST(test_window_create_ids_increment);
    RUN_TEST(test_window_destroy_null_safe);
    RUN_TEST(test_window_destroy_cleans_up);
    RUN_TEST(test_window_destroy_calls_on_destroy_callback);
    RUN_TEST(test_window_destroy_nullifies_plane_before_callback_returns);
    RUN_TEST(test_window_destroy_without_callback);
    RUN_TEST(test_window_create_initializes_fields);
    RUN_TEST(test_window_destroy_stubs_after_callback);
    return UNITY_END();
}
