#include "unity.h"
#include "core/logger.h"
#include "widget/dialog.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_dialog.log";

static bool g_cb_called = false;
static int g_cb_button = 0;

static void dialog_cb(int button_index, void* ud) {
    (void)ud;
    g_cb_called = true;
    g_cb_button = button_index;
}

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

void setUp(void) {
    tui_logger_init(test_log_file);
    g_cb_called = false;
    g_cb_button = 0;
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ---- CREATE/DESTROY ---- */

void test_dialog_create_destroy(void) {
    TuiDialog* dlg = tui_dialog_create(dummy_parent(), 0, 0, 40, 10, "Title", "Message", NULL, NULL);
    TEST_ASSERT_NOT_NULL(dlg);
    TEST_ASSERT_EQUAL_INT(0, dlg->button_count);
    TEST_ASSERT_EQUAL_INT(-1, dlg->active_button);
    tui_dialog_destroy(dlg);
}

void test_dialog_create_null_parent(void) {
    TuiDialog* dlg = tui_dialog_create(NULL, 0, 0, 40, 10, "Title", "Message", NULL, NULL);
    TEST_ASSERT_NULL(dlg);
}

void test_dialog_create_null_title(void) {
    TuiDialog* dlg = tui_dialog_create(dummy_parent(), 0, 0, 40, 10, NULL, "Message", NULL, NULL);
    TEST_ASSERT_NULL(dlg);
}

void test_dialog_create_null_message(void) {
    TuiDialog* dlg = tui_dialog_create(dummy_parent(), 0, 0, 40, 10, "Title", NULL, NULL, NULL);
    TEST_ASSERT_NULL(dlg);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_dialog_add_button(void) {
    TuiDialog* dlg = tui_dialog_create(dummy_parent(), 0, 0, 40, 10, "Title", "Message", NULL, NULL);
    TEST_ASSERT_TRUE(tui_dialog_add_button(dlg, "OK"));
    TEST_ASSERT_EQUAL_INT(1, tui_dialog_get_button_count(dlg));
    TEST_ASSERT_EQUAL_INT(0, tui_dialog_get_active_button(dlg));
    tui_dialog_destroy(dlg);
}

void test_dialog_add_multiple_buttons(void) {
    TuiDialog* dlg = tui_dialog_create(dummy_parent(), 0, 0, 40, 10, "Confirm", "Are you sure?", NULL, NULL);
    tui_dialog_add_button(dlg, "Yes");
    tui_dialog_add_button(dlg, "No");
    tui_dialog_add_button(dlg, "Cancel");
    TEST_ASSERT_EQUAL_INT(3, tui_dialog_get_button_count(dlg));
    tui_dialog_destroy(dlg);
}

void test_dialog_handle_key_left_right(void) {
    TuiDialog* dlg = tui_dialog_create(dummy_parent(), 0, 0, 40, 10, "Title", "Msg", NULL, NULL);
    tui_dialog_add_button(dlg, "Yes");
    tui_dialog_add_button(dlg, "No");

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_dialog_handle_key(dlg, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_dialog_get_active_button(dlg));

    tui_dialog_handle_key(dlg, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_dialog_get_active_button(dlg));
    tui_dialog_destroy(dlg);
}

void test_dialog_handle_key_enter_activate(void) {
    TuiDialog* dlg = tui_dialog_create(dummy_parent(), 0, 0, 40, 10, "Title", "Msg", dialog_cb, NULL);
    tui_dialog_add_button(dlg, "OK");
    tui_dialog_add_button(dlg, "Cancel");
    tui_dialog_set_active_button(dlg, 0);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_dialog_handle_key(dlg, NCKEY_ENTER, &ni);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_INT(0, g_cb_button);
    tui_dialog_destroy(dlg);
}

void test_dialog_handle_key_esc_cancel(void) {
    TuiDialog* dlg = tui_dialog_create(dummy_parent(), 0, 0, 40, 10, "Title", "Msg", dialog_cb, NULL);
    tui_dialog_add_button(dlg, "OK");

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_dialog_handle_key(dlg, NCKEY_ESC, &ni);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_INT(-1, g_cb_button);
    tui_dialog_destroy(dlg);
}

void test_dialog_render_null_safe(void) {
    tui_dialog_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_dialog_null_safety(void) {
    tui_dialog_destroy(NULL);
    TEST_ASSERT_EQUAL_INT(-1, tui_dialog_get_active_button(NULL));
    tui_dialog_set_active_button(NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, tui_dialog_get_button_count(NULL));
    TEST_ASSERT_FALSE(tui_dialog_add_button(NULL, "OK"));

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_dialog_handle_key(NULL, NCKEY_ENTER, &ni));
    TEST_ASSERT_FALSE(tui_dialog_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_dialog_create_destroy);
    RUN_TEST(test_dialog_create_null_parent);
    RUN_TEST(test_dialog_create_null_title);
    RUN_TEST(test_dialog_create_null_message);
    RUN_TEST(test_dialog_add_button);
    RUN_TEST(test_dialog_add_multiple_buttons);
    RUN_TEST(test_dialog_handle_key_left_right);
    RUN_TEST(test_dialog_handle_key_enter_activate);
    RUN_TEST(test_dialog_handle_key_esc_cancel);
    RUN_TEST(test_dialog_render_null_safe);
    RUN_TEST(test_dialog_null_safety);
    return UNITY_END();
}
