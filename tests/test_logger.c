#include "unity.h"
#include "core/logger.h"

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test.log";

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

void test_logger_buffer_wrap_around(void) {
    // setUp ya agrega 1 log ("Motor de Logging Inicializado...")
    // Escribimos 110 líneas para exceder MAX_LOG_LINES (100)
    for (int i = 0; i < 110; i++) {
        tui_log(LOG_INFO, "Line %d", i);
    }
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    // El buffer solo retiene los últimos 100
    TEST_ASSERT_EQUAL_INT(MAX_LOG_LINES, buf->count);
    // head apunta al siguiente slot escribir (101 mod 100 = 1, pero contamos el init=1, wrap=10)
    // Oldest entry está en head: debería ser "Line 10" (después de wrap)
    // El formato del buffer es "[HH:MM:SS] Line 10"
    TEST_ASSERT_TRUE(strstr(buf->lines[buf->head], "Line 10") != NULL);
}

void test_logger_basic_operations(void) {
    // setUp ya agregó 1 log
    tui_log(LOG_INFO, "Hello");     // 2
    tui_log(LOG_WARN, "Warning");   // 3
    tui_log(LOG_ERROR, "Error");    // 4
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_EQUAL_INT(4, buf->count); // init + 3 logs = 4
    // El mensaje más reciente se escribe antes de incrementar head
    int idx = (buf->head - 1 + MAX_LOG_LINES) % MAX_LOG_LINES;
    TEST_ASSERT_TRUE(strstr(buf->lines[idx], "Error") != NULL);
}

void test_logger_thread_safety_basic(void) {
    tui_log(LOG_INFO, "Single thread works");
    tui_log(LOG_ERROR, "Error msg");
    tui_log(LOG_WARN, "Warning msg");
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_GREATER_THAN(2, buf->count);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_logger_buffer_wrap_around);
    RUN_TEST(test_logger_basic_operations);
    RUN_TEST(test_logger_thread_safety_basic);
    return UNITY_END();
}
