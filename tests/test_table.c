#include "unity.h"
#include "core/logger.h"
#include "widget/table.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_table.log";

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ---- CREATE/DESTROY ---- */

void test_table_create_destroy(void) {
    TuiTable* tbl = tui_table_create(dummy_parent(), 0, 0, 40, 10, 3);
    TEST_ASSERT_NOT_NULL(tbl);
    TEST_ASSERT_EQUAL_INT(0, tbl->row_count);
    TEST_ASSERT_EQUAL_INT(-1, tbl->selected_row);
    tui_table_destroy(tbl);
}

void test_table_create_null_parent(void) {
    TuiTable* tbl = tui_table_create(NULL, 0, 0, 40, 10, 3);
    TEST_ASSERT_NULL(tbl);
}

void test_table_create_invalid_cols(void) {
    TuiTable* tbl = tui_table_create(dummy_parent(), 0, 0, 40, 10, 0);
    TEST_ASSERT_NULL(tbl);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_table_set_header(void) {
    TuiTable* tbl = tui_table_create(dummy_parent(), 0, 0, 40, 10, 3);
    TEST_ASSERT_TRUE(tui_table_set_header(tbl, 0, "Name"));
    TEST_ASSERT_TRUE(tui_table_set_header(tbl, 1, "Age"));
    TEST_ASSERT_TRUE(tui_table_set_header(tbl, 2, "City"));
    tui_table_destroy(tbl);
}

void test_table_add_row(void) {
    TuiTable* tbl = tui_table_create(dummy_parent(), 0, 0, 40, 10, 3);
    tui_table_set_header(tbl, 0, "Name");
    tui_table_set_header(tbl, 1, "Age");
    tui_table_set_header(tbl, 2, "City");

    const char* row1[] = {"Alice", "30", "NYC"};
    int idx = tui_table_add_row(tbl, row1);
    TEST_ASSERT_EQUAL_INT(0, idx);
    TEST_ASSERT_EQUAL_INT(1, tui_table_get_row_count(tbl));

    const char* row2[] = {"Bob", "25", "LA"};
    idx = tui_table_add_row(tbl, row2);
    TEST_ASSERT_EQUAL_INT(1, idx);
    TEST_ASSERT_EQUAL_INT(2, tui_table_get_row_count(tbl));
    tui_table_destroy(tbl);
}

void test_table_get_cell(void) {
    TuiTable* tbl = tui_table_create(dummy_parent(), 0, 0, 40, 10, 3);
    const char* row[] = {"Alice", "30", "NYC"};
    tui_table_add_row(tbl, row);

    const char* cell = tui_table_get_cell(tbl, 0, 0);
    TEST_ASSERT_NOT_NULL(cell);
    TEST_ASSERT_EQUAL_STRING("Alice", cell);
    tui_table_destroy(tbl);
}

void test_table_handle_key_up_down(void) {
    TuiTable* tbl = tui_table_create(dummy_parent(), 0, 0, 40, 10, 3);
    const char* r1[] = {"A", "1", "X"};
    const char* r2[] = {"B", "2", "Y"};
    const char* r3[] = {"C", "3", "Z"};
    tui_table_add_row(tbl, r1);
    tui_table_add_row(tbl, r2);
    tui_table_add_row(tbl, r3);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_table_handle_key(tbl, NCKEY_DOWN, &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_table_get_selected(tbl));

    tui_table_handle_key(tbl, NCKEY_UP, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_table_get_selected(tbl));
    tui_table_destroy(tbl);
}

void test_table_handle_key_page_home_end(void) {
    TuiTable* tbl = tui_table_create(dummy_parent(), 0, 0, 40, 10, 3);
    for (int i = 0; i < 20; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Row%d", i);
        const char* vals[] = {buf, "1", "X"};
        tui_table_add_row(tbl, vals);
    }

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_table_handle_key(tbl, NCKEY_END, &ni);
    TEST_ASSERT_EQUAL_INT(19, tui_table_get_selected(tbl));

    tui_table_handle_key(tbl, NCKEY_HOME, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_table_get_selected(tbl));
    tui_table_destroy(tbl);
}

void test_table_render_null_safe(void) {
    tui_table_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_table_null_safety(void) {
    tui_table_destroy(NULL);
    TEST_ASSERT_EQUAL_INT(-1, tui_table_get_selected(NULL));
    tui_table_set_selected(NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, tui_table_get_row_count(NULL));
    TEST_ASSERT_NULL(tui_table_get_cell(NULL, 0, 0));
    tui_table_set_focused(NULL, true);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_table_handle_key(NULL, NCKEY_DOWN, &ni));
    TEST_ASSERT_FALSE(tui_table_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_table_create_destroy);
    RUN_TEST(test_table_create_null_parent);
    RUN_TEST(test_table_create_invalid_cols);
    RUN_TEST(test_table_set_header);
    RUN_TEST(test_table_add_row);
    RUN_TEST(test_table_get_cell);
    RUN_TEST(test_table_handle_key_up_down);
    RUN_TEST(test_table_handle_key_page_home_end);
    RUN_TEST(test_table_render_null_safe);
    RUN_TEST(test_table_null_safety);
    return UNITY_END();
}
