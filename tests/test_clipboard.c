#include "unity.h"
#include "core/logger.h"
#include "core/clipboard.h"
#include <string.h>

static const char* test_log_file = "/tmp/tmlcs_tui_test_clipboard.log";

int tests_run = 0;
int tests_failed = 0;

void setUp(void) {
    tui_logger_init(test_log_file);
    tui_clipboard_clear();
}

void tearDown(void) {
    tui_logger_destroy();
    tui_clipboard_clear();
}

/* ================================================================== */
/*  CLIPBOARD TESTS                                                     */
/* ================================================================== */

void test_clipboard_set_get_roundtrip(void) {
    TEST_ASSERT_TRUE(tui_clipboard_copy("hello"));
    const char* got = tui_clipboard_get();
    TEST_ASSERT_NOT_NULL(got);
    TEST_ASSERT_EQUAL_STRING("hello", got);
}

void test_clipboard_overflow_protection(void) {
    /* Set a long string */
    char long_str[200];
    memset(long_str, 'A', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';
    TEST_ASSERT_TRUE(tui_clipboard_copy(long_str));

    /* Paste with small buffer — should truncate safely */
    char small_buf[50];
    int n = tui_clipboard_paste(small_buf, sizeof(small_buf));
    TEST_ASSERT_EQUAL_INT(49, n);  /* max_buf - 1 */
    TEST_ASSERT_EQUAL_INT(0, small_buf[49]);  /* null terminated */
}

void test_clipboard_clear(void) {
    TEST_ASSERT_TRUE(tui_clipboard_copy("test data"));
    TEST_ASSERT_TRUE(tui_clipboard_has_content());
    tui_clipboard_clear();
    TEST_ASSERT_FALSE(tui_clipboard_has_content());
    const char* got = tui_clipboard_get();
    TEST_ASSERT_NULL(got);
}

void test_clipboard_paste_null_params(void) {
    TEST_ASSERT_TRUE(tui_clipboard_copy("data"));
    TEST_ASSERT_EQUAL_INT(-1, tui_clipboard_paste(NULL, 64));
    TEST_ASSERT_EQUAL_INT(-1, tui_clipboard_paste(NULL, 0));
}

void test_clipboard_set_empty_string(void) {
    TEST_ASSERT_TRUE(tui_clipboard_copy("previous"));
    TEST_ASSERT_TRUE(tui_clipboard_copy(""));
    const char* got = tui_clipboard_get();
    /* Empty string results in NULL or empty */
    if (got != NULL) {
        TEST_ASSERT_EQUAL_STRING("", got);
    }
}

void test_clipboard_null_input_to_copy(void) {
    bool r = tui_clipboard_copy(NULL);
    TEST_ASSERT_FALSE(r);
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_clipboard_set_get_roundtrip);
    RUN_TEST(test_clipboard_overflow_protection);
    RUN_TEST(test_clipboard_clear);
    RUN_TEST(test_clipboard_paste_null_params);
    RUN_TEST(test_clipboard_set_empty_string);
    RUN_TEST(test_clipboard_null_input_to_copy);

    return UNITY_END();
}
