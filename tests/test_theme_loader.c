#include "unity.h"
#include "core/theme_loader.h"
#include "core/logger.h"

static const char* test_log_file = "/tmp/tmlcs_tui_test_theme.log";

int tests_run = 0;
int tests_failed = 0;

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ================================================================== */
/*  THEME LOADER TESTS                                                  */
/* ================================================================== */

void test_theme_get_color_valid_name(void) {
    unsigned rgb = 0;
    bool r = tui_theme_get_color("THEME_BG_DARK", &rgb);
    TEST_ASSERT_TRUE(r);
    TEST_ASSERT_EQUAL_INT(0x1A1A24, (int)rgb);
}

void test_theme_get_color_invalid_name(void) {
    unsigned rgb = 0;
    bool r = tui_theme_get_color("NONEXISTENT_THEME", &rgb);
    TEST_ASSERT_FALSE(r);
}

void test_theme_get_color_null_name(void) {
    unsigned rgb = 0;
    bool r = tui_theme_get_color(NULL, &rgb);
    TEST_ASSERT_FALSE(r);
}

void test_theme_get_color_null_rgb(void) {
    bool r = tui_theme_get_color("THEME_FG_DEFAULT", NULL);
    TEST_ASSERT_TRUE(r);
}

void test_theme_load_null_path(void) {
    bool r = tui_theme_load_file(NULL);
    TEST_ASSERT_FALSE(r);
}

void test_theme_load_nonexistent_file(void) {
    bool r = tui_theme_load_file("/tmp/nonexistent_theme_12345.theme");
    TEST_ASSERT_FALSE(r);
}

void test_theme_get_multiple_colors(void) {
    unsigned rgb = 0;

    TEST_ASSERT_TRUE(tui_theme_get_color("THEME_BG_DARKEST", &rgb));
    TEST_ASSERT_EQUAL_INT(0x111116, (int)rgb);

    TEST_ASSERT_TRUE(tui_theme_get_color("THEME_BG_WINDOW", &rgb));
    TEST_ASSERT_EQUAL_INT(0x282a36, (int)rgb);

    TEST_ASSERT_TRUE(tui_theme_get_color("THEME_FG_LOG_ERROR", &rgb));
    TEST_ASSERT_EQUAL_INT(0xff5555, (int)rgb);

    TEST_ASSERT_TRUE(tui_theme_get_color("THEME_FG_LOG_INFO", &rgb));
    TEST_ASSERT_EQUAL_INT(0x50fa7b, (int)rgb);
}

void test_theme_set_color_valid_name(void) {
    bool r = tui_theme_set_color("THEME_BG_DARK", 0xABCDEF);
    TEST_ASSERT_TRUE(r);

    unsigned rgb = 0;
    r = tui_theme_get_color("THEME_BG_DARK", &rgb);
    TEST_ASSERT_TRUE(r);
    TEST_ASSERT_EQUAL_INT(0xABCDEF, (int)rgb);
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_theme_get_color_valid_name);
    RUN_TEST(test_theme_get_color_invalid_name);
    RUN_TEST(test_theme_get_color_null_name);
    RUN_TEST(test_theme_get_color_null_rgb);
    RUN_TEST(test_theme_load_null_path);
    RUN_TEST(test_theme_load_nonexistent_file);
    RUN_TEST(test_theme_get_multiple_colors);
    RUN_TEST(test_theme_set_color_valid_name);

    return UNITY_END();
}
