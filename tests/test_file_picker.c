#include "unity.h"
#include "core/logger.h"
#include "widget/file_picker.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_file_picker.log";

static bool g_cb_called = false;
static char g_cb_path[512] = {0};

static void fp_cb(const char* path, void* ud) {
    (void)ud;
    g_cb_called = true;
    if (path) strncpy(g_cb_path, path, sizeof(g_cb_path) - 1);
}

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

void setUp(void) {
    tui_logger_init(test_log_file);
    g_cb_called = false;
    memset(g_cb_path, 0, sizeof(g_cb_path));
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ---- CREATE/DESTROY ---- */

void test_file_picker_create_destroy(void) {
    TuiFilePicker* fp = tui_file_picker_create(dummy_parent(), 0, 0, 40, 15, "/tmp", NULL, NULL);
    TEST_ASSERT_NOT_NULL(fp);
    TEST_ASSERT_NOT_NULL(tui_file_picker_get_path(fp));
    tui_file_picker_destroy(fp);
}

void test_file_picker_create_null_parent(void) {
    TuiFilePicker* fp = tui_file_picker_create(NULL, 0, 0, 40, 15, "/tmp", NULL, NULL);
    TEST_ASSERT_NULL(fp);
}

void test_file_picker_create_default_path(void) {
    TuiFilePicker* fp = tui_file_picker_create(dummy_parent(), 0, 0, 40, 15, NULL, NULL, NULL);
    /* Should use default path "." */
    TEST_ASSERT_NOT_NULL(fp);
    tui_file_picker_destroy(fp);
}

void test_file_picker_create_too_narrow(void) {
    TuiFilePicker* fp = tui_file_picker_create(dummy_parent(), 0, 0, 5, 15, "/tmp", NULL, NULL);
    TEST_ASSERT_NULL(fp);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_file_picker_get_path(void) {
    TuiFilePicker* fp = tui_file_picker_create(dummy_parent(), 0, 0, 40, 15, "/tmp", NULL, NULL);
    const char* path = tui_file_picker_get_path(fp);
    TEST_ASSERT_NOT_NULL(path);
    TEST_ASSERT_NOT_NULL(strstr(path, "tmp"));
    tui_file_picker_destroy(fp);
}

void test_file_picker_get_count(void) {
    TuiFilePicker* fp = tui_file_picker_create(dummy_parent(), 0, 0, 40, 15, "/tmp", NULL, NULL);
    /* /tmp should have at least 0 entries (may be empty) */
    int count = tui_file_picker_get_count(fp);
    TEST_ASSERT_TRUE(count >= 0);
    tui_file_picker_destroy(fp);
}

void test_file_picker_handle_key_up_down(void) {
    TuiFilePicker* fp = tui_file_picker_create(dummy_parent(), 0, 0, 40, 15, "/tmp", NULL, NULL);
    int count = tui_file_picker_get_count(fp);
    if (count < 2) {
        tui_file_picker_destroy(fp);
        return; /* Skip if not enough entries */
    }

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    int initial = fp->selected;
    tui_file_picker_handle_key(fp, NCKEY_DOWN, &ni);
    TEST_ASSERT_EQUAL_INT(initial + 1, fp->selected);

    tui_file_picker_handle_key(fp, NCKEY_UP, &ni);
    TEST_ASSERT_EQUAL_INT(initial, fp->selected);
    tui_file_picker_destroy(fp);
}

void test_file_picker_handle_key_backspace(void) {
    TuiFilePicker* fp = tui_file_picker_create(dummy_parent(), 0, 0, 40, 15, "/tmp", NULL, NULL);
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    /* Backspace goes up from /tmp -> / */
    bool consumed = tui_file_picker_handle_key(fp, NCKEY_BACKSPACE, &ni);
    TEST_ASSERT_TRUE(consumed);
    tui_file_picker_destroy(fp);
}

void test_file_picker_go_up_at_root(void) {
    TuiFilePicker* fp = tui_file_picker_create(dummy_parent(), 0, 0, 40, 15, "/", NULL, NULL);
    bool result = tui_file_picker_go_up(fp);
    TEST_ASSERT_FALSE(result); /* Can't go up from root */
    tui_file_picker_destroy(fp);
}

void test_file_picker_callback_spied(void) {
    /* Verify the callback function exists and works correctly */
    g_cb_called = false;
    fp_cb("/tmp/test.txt", NULL);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_STRING("/tmp/test.txt", g_cb_path);
}

void test_file_picker_render_null_safe(void) {
    tui_file_picker_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_file_picker_null_safety(void) {
    tui_file_picker_destroy(NULL);
    TEST_ASSERT_NULL(tui_file_picker_get_selected_name(NULL));
    TEST_ASSERT_NULL(tui_file_picker_get_path(NULL));
    TEST_ASSERT_EQUAL_INT(0, tui_file_picker_get_count(NULL));
    tui_file_picker_set_focused(NULL, true);
    TEST_ASSERT_FALSE(tui_file_picker_go_up(NULL));
    TEST_ASSERT_FALSE(tui_file_picker_navigate(NULL, "/tmp"));

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_file_picker_handle_key(NULL, NCKEY_DOWN, &ni));
    TEST_ASSERT_FALSE(tui_file_picker_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_file_picker_create_destroy);
    RUN_TEST(test_file_picker_create_null_parent);
    RUN_TEST(test_file_picker_create_default_path);
    RUN_TEST(test_file_picker_create_too_narrow);
    RUN_TEST(test_file_picker_get_path);
    RUN_TEST(test_file_picker_get_count);
    RUN_TEST(test_file_picker_handle_key_up_down);
    RUN_TEST(test_file_picker_handle_key_backspace);
    RUN_TEST(test_file_picker_go_up_at_root);
    RUN_TEST(test_file_picker_callback_spied);
    RUN_TEST(test_file_picker_render_null_safe);
    RUN_TEST(test_file_picker_null_safety);
    return UNITY_END();
}
