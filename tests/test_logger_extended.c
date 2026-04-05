#include "unity.h"
#include "core/logger.h"

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_extended.log";

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

/* --- Log level filtering --- */

void test_logger_default_level_is_debug(void) {
    // Default level is LOG_DEBUG, so all messages should be logged
    tui_log(LOG_DEBUG, "Debug message");
    tui_log(LOG_INFO, "Info message");
    tui_log(LOG_WARN, "Warn message");
    tui_log(LOG_ERROR, "Error message");
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    // init (1) + 4 logs = 5
    TEST_ASSERT_EQUAL_INT(5, buf->count);
}

void test_logger_set_level_filters_debug(void) {
    tui_logger_set_level(LOG_INFO);
    tui_log(LOG_DEBUG, "This should be filtered");
    tui_log(LOG_INFO, "Info passes");
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    // init (1) + info (1) = 2, debug filtered
    TEST_ASSERT_EQUAL_INT(2, buf->count);
}

void test_logger_set_level_filters_below_warn(void) {
    tui_logger_set_level(LOG_WARN);
    tui_log(LOG_DEBUG, "filtered");
    tui_log(LOG_INFO, "filtered");
    tui_log(LOG_WARN, "warn passes");
    tui_log(LOG_ERROR, "error passes");
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    // init (1) + warn (1) + error (1) = 3
    TEST_ASSERT_EQUAL_INT(3, buf->count);
}

void test_logger_set_level_error_only(void) {
    tui_logger_set_level(LOG_ERROR);
    tui_log(LOG_DEBUG, "filtered");
    tui_log(LOG_INFO, "filtered");
    tui_log(LOG_WARN, "filtered");
    tui_log(LOG_ERROR, "error passes");
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_EQUAL_INT(2, buf->count);
}

void test_logger_get_level_returns_current(void) {
    tui_logger_set_level(LOG_WARN);
    TEST_ASSERT_EQUAL_INT(LOG_WARN, tui_logger_get_level());
    tui_logger_set_level(LOG_DEBUG);
    TEST_ASSERT_EQUAL_INT(LOG_DEBUG, tui_logger_get_level());
    tui_logger_set_level(LOG_ERROR);
    TEST_ASSERT_EQUAL_INT(LOG_ERROR, tui_logger_get_level());
}

/* --- Buffer edge cases --- */

void test_logger_buffer_exact_capacity(void) {
    // Buffer starts with 1 (init message). Fill to exactly MAX_LOG_LINES.
    for (int i = 0; i < MAX_LOG_LINES - 1; i++) {
        tui_log(LOG_INFO, "Line %d", i);
    }
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_EQUAL_INT(MAX_LOG_LINES, buf->count);
}

void test_logger_buffer_double_wrap(void) {
    // Write 250 lines = 2.5x the buffer size
    for (int i = 0; i < 250; i++) {
        tui_log(LOG_INFO, "Wrap %d", i);
    }
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_EQUAL_INT(MAX_LOG_LINES, buf->count);
    // Most recent should be "Wrap 249"
    int idx = (buf->head - 1 + MAX_LOG_LINES) % MAX_LOG_LINES;
    TEST_ASSERT_TRUE(strstr(buf->lines[idx], "Wrap 249") != NULL);
}

void test_logger_level_preserved_in_buffer(void) {
    tui_log(LOG_DEBUG, "debug");
    tui_log(LOG_WARN, "warning");
    tui_log(LOG_ERROR, "error");
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    // init + 3 = 4, last is error
    int idx = (buf->head - 1 + MAX_LOG_LINES) % MAX_LOG_LINES;
    TEST_ASSERT_EQUAL_INT(LOG_ERROR, buf->levels[idx]);
}

/* --- Log level enum ordering --- */

void test_logger_level_ordering(void) {
    TEST_ASSERT_TRUE(LOG_DEBUG < LOG_INFO);
    TEST_ASSERT_TRUE(LOG_INFO < LOG_WARN);
    TEST_ASSERT_TRUE(LOG_WARN < LOG_ERROR);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_logger_default_level_is_debug);
    RUN_TEST(test_logger_set_level_filters_debug);
    RUN_TEST(test_logger_set_level_filters_below_warn);
    RUN_TEST(test_logger_set_level_error_only);
    RUN_TEST(test_logger_get_level_returns_current);
    RUN_TEST(test_logger_buffer_exact_capacity);
    RUN_TEST(test_logger_buffer_double_wrap);
    RUN_TEST(test_logger_level_preserved_in_buffer);
    RUN_TEST(test_logger_level_ordering);
    return UNITY_END();
}
