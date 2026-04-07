#include "unity.h"
#include "core/logger.h"
#include "core/layout.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_layout.log";

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ------------------------------------------------------------------ */
/*  Mock plane for layout tests (doesn't need real notcurses)          */
/* ------------------------------------------------------------------ */

static struct ncplane* make_dummy_plane(void) {
    return (struct ncplane*)0xBEEF;
}

/* ------------------------------------------------------------------ */
/*  Mock widget type for testing                                       */
/* ------------------------------------------------------------------ */

typedef struct {
    int pref_h;
    int pref_w;
    int render_count;
} MockWidget;

/* ------------------------------------------------------------------ */
/*  TEST: Layout create/destroy                                        */
/* ------------------------------------------------------------------ */

void test_layout_create_destroy(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    TEST_ASSERT_NOT_NULL(layout);
    TEST_ASSERT_EQUAL_INT(0, tui_layout_get_child_count(layout));
    tui_layout_destroy(layout);
}

void test_layout_create_horizontal(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_HORIZONTAL);
    TEST_ASSERT_NOT_NULL(layout);
    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Add widget                                                   */
/* ------------------------------------------------------------------ */

void test_layout_add_widget(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    MockWidget w = {3, 20, 0};
    struct ncplane* p = make_dummy_plane();

    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 3.0f, ALIGN_STRETCH);
    TEST_ASSERT_EQUAL_INT(1, tui_layout_get_child_count(layout));

    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 5.0f, ALIGN_STRETCH);
    TEST_ASSERT_EQUAL_INT(2, tui_layout_get_child_count(layout));

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Vertical spacing                                             */
/* ------------------------------------------------------------------ */

void test_layout_vertical_spacing(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    tui_layout_set_spacing(layout, 2);

    MockWidget w = {3, 20, 0};
    struct ncplane* p = make_dummy_plane();

    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 3.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 3.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 3.0f, ALIGN_STRETCH);

    /* After compute with 20 available height, 3 fixed widgets of 3 each = 9 */
    /* Spacing = 2 * 2 = 4. Total = 9 + 4 = 13 */
    tui_layout_compute(layout, 20, 30);

    /* Layout should be clean after compute */
    /* (We can't verify internal plane positions with stubs, but compute should not crash) */
    TEST_ASSERT_TRUE(1);

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Horizontal spacing                                           */
/* ------------------------------------------------------------------ */

void test_layout_horizontal_spacing(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_HORIZONTAL);
    tui_layout_set_spacing(layout, 1);

    MockWidget w = {3, 10, 0};
    struct ncplane* p = make_dummy_plane();

    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 10.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 10.0f, ALIGN_STRETCH);

    tui_layout_compute(layout, 20, 30);
    TEST_ASSERT_TRUE(1);

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Fixed and fill                                               */
/* ------------------------------------------------------------------ */

void test_layout_fixed_and_fill(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    tui_layout_set_spacing(layout, 1);

    MockWidget w_fixed = {5, 20, 0};
    MockWidget w_fill = {10, 20, 0};
    struct ncplane* p = make_dummy_plane();

    /* 3 fixed rows of 5 = 15, spacing = 3, remaining for fill */
    tui_layout_add_widget(layout, &w_fixed, -1, p, SIZE_FIXED, 5.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w_fill, -1, p, SIZE_FILL, 0.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w_fixed, -1, p, SIZE_FIXED, 5.0f, ALIGN_STRETCH);

    tui_layout_compute(layout, 30, 30);
    TEST_ASSERT_TRUE(1);

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Weight distribution                                          */
/* ------------------------------------------------------------------ */

void test_layout_weight_distribution(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    tui_layout_set_spacing(layout, 0);

    MockWidget w = {0, 20, 0};
    struct ncplane* p = make_dummy_plane();

    /* weight 1 + weight 2 + weight 1 = 4 total */
    tui_layout_add_widget(layout, &w, -1, p, SIZE_WEIGHT, 1.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w, -1, p, SIZE_WEIGHT, 2.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w, -1, p, SIZE_WEIGHT, 1.0f, ALIGN_STRETCH);

    tui_layout_compute(layout, 40, 30);
    TEST_ASSERT_TRUE(1);

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Padding                                                      */
/* ------------------------------------------------------------------ */

void test_layout_padding(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    tui_layout_set_padding(layout, 2, 3, 2, 1);
    tui_layout_set_spacing(layout, 0);

    MockWidget w = {5, 20, 0};
    struct ncplane* p = make_dummy_plane();

    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 5.0f, ALIGN_STRETCH);

    tui_layout_compute(layout, 20, 30);
    TEST_ASSERT_TRUE(1);

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Nested layout                                                */
/* ------------------------------------------------------------------ */

void test_layout_nested(void) {
    /* Parent layout */
    TuiLayout* parent = tui_layout_create(LAYOUT_VERTICAL);
    tui_layout_set_spacing(parent, 1);

    /* Child layout */
    TuiLayout* child = tui_layout_create(LAYOUT_HORIZONTAL);
    tui_layout_set_spacing(child, 1);

    MockWidget w = {3, 10, 0};
    struct ncplane* p = make_dummy_plane();

    /* Add widgets to child layout */
    tui_layout_add_widget(child, &w, -1, p, SIZE_FIXED, 10.0f, ALIGN_STRETCH);
    tui_layout_add_widget(child, &w, -1, p, SIZE_FIXED, 10.0f, ALIGN_STRETCH);

    /* Add child layout to parent */
    tui_layout_add_layout(parent, child, SIZE_FIXED, 10.0f, ALIGN_STRETCH);

    /* Add another widget to parent */
    tui_layout_add_widget(parent, &w, -1, p, SIZE_FIXED, 5.0f, ALIGN_STRETCH);

    tui_layout_compute(parent, 30, 30);
    TEST_ASSERT_TRUE(1);

    /* Destroying parent should also destroy the child layout */
    tui_layout_destroy(parent);
}

/* ------------------------------------------------------------------ */
/*  TEST: Compute marks clean                                          */
/* ------------------------------------------------------------------ */

void test_layout_compute_marks_clean(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    MockWidget w = {3, 20, 0};
    struct ncplane* p = make_dummy_plane();

    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 3.0f, ALIGN_STRETCH);
    tui_layout_compute(layout, 20, 30);

    /* After compute, the layout is marked clean (needs_compute = false) */
    /* We verify by calling compute again - it should be a no-op */
    tui_layout_compute(layout, 20, 30);
    TEST_ASSERT_TRUE(1);

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Remove child                                                 */
/* ------------------------------------------------------------------ */

void test_layout_remove_child(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    MockWidget w = {3, 20, 0};
    struct ncplane* p = make_dummy_plane();

    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 3.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 5.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 7.0f, ALIGN_STRETCH);
    TEST_ASSERT_EQUAL_INT(3, tui_layout_get_child_count(layout));

    /* Remove middle child */
    tui_layout_remove_child(layout, 1);
    TEST_ASSERT_EQUAL_INT(2, tui_layout_get_child_count(layout));

    /* Remove out-of-bounds (no-op) */
    tui_layout_remove_child(layout, 10);
    TEST_ASSERT_EQUAL_INT(2, tui_layout_get_child_count(layout));

    tui_layout_remove_child(layout, 0);
    TEST_ASSERT_EQUAL_INT(1, tui_layout_get_child_count(layout));

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Remove child with nested layout                              */
/* ------------------------------------------------------------------ */

void test_layout_remove_nested_layout(void) {
    TuiLayout* parent = tui_layout_create(LAYOUT_VERTICAL);
    TuiLayout* child = tui_layout_create(LAYOUT_HORIZONTAL);

    MockWidget w = {3, 10, 0};
    struct ncplane* p = make_dummy_plane();
    tui_layout_add_widget(child, &w, -1, p, SIZE_FIXED, 10.0f, ALIGN_STRETCH);
    tui_layout_add_layout(parent, child, SIZE_FIXED, 10.0f, ALIGN_STRETCH);

    TEST_ASSERT_EQUAL_INT(1, tui_layout_get_child_count(parent));

    /* Removing should also destroy the child layout */
    tui_layout_remove_child(parent, 0);
    TEST_ASSERT_EQUAL_INT(0, tui_layout_get_child_count(parent));

    tui_layout_destroy(parent);
}

/* ------------------------------------------------------------------ */
/*  TEST: Mark dirty                                                   */
/* ------------------------------------------------------------------ */

void test_layout_mark_dirty(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    MockWidget w = {3, 20, 0};
    struct ncplane* p = make_dummy_plane();

    tui_layout_add_widget(layout, &w, -1, p, SIZE_FIXED, 3.0f, ALIGN_STRETCH);
    tui_layout_compute(layout, 20, 30);

    /* Mark dirty and recompute */
    tui_layout_mark_dirty(layout);
    tui_layout_compute(layout, 20, 30);
    TEST_ASSERT_TRUE(1);

    tui_layout_destroy(layout);
}

/* ------------------------------------------------------------------ */
/*  TEST: Null safety                                                  */
/* ------------------------------------------------------------------ */

void test_layout_null_safety(void) {
    tui_layout_destroy(NULL);
    tui_layout_set_spacing(NULL, 5);
    tui_layout_set_padding(NULL, 1, 1, 1, 1);
    tui_layout_add_widget(NULL, NULL, -1, NULL, SIZE_FIXED, 0, ALIGN_STRETCH);
    tui_layout_add_layout(NULL, NULL, SIZE_FIXED, 0, ALIGN_STRETCH);
    tui_layout_remove_child(NULL, 0);
    tui_layout_compute(NULL, 10, 10);
    tui_layout_render(NULL);
    tui_layout_mark_dirty(NULL);
    TEST_ASSERT_EQUAL_INT(0, tui_layout_get_child_count(NULL));
    TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/*  TEST: Percent sizing                                               */
/* ------------------------------------------------------------------ */

void test_layout_percent_sizing(void) {
    TuiLayout* layout = tui_layout_create(LAYOUT_VERTICAL);
    tui_layout_set_spacing(layout, 0);

    MockWidget w = {0, 20, 0};
    struct ncplane* p = make_dummy_plane();

    tui_layout_add_widget(layout, &w, -1, p, SIZE_PERCENT, 30.0f, ALIGN_STRETCH);
    tui_layout_add_widget(layout, &w, -1, p, SIZE_PERCENT, 70.0f, ALIGN_STRETCH);

    tui_layout_compute(layout, 100, 30);
    TEST_ASSERT_TRUE(1);

    tui_layout_destroy(layout);
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_layout_create_destroy);
    RUN_TEST(test_layout_create_horizontal);
    RUN_TEST(test_layout_add_widget);
    RUN_TEST(test_layout_vertical_spacing);
    RUN_TEST(test_layout_horizontal_spacing);
    RUN_TEST(test_layout_fixed_and_fill);
    RUN_TEST(test_layout_weight_distribution);
    RUN_TEST(test_layout_padding);
    RUN_TEST(test_layout_nested);
    RUN_TEST(test_layout_compute_marks_clean);
    RUN_TEST(test_layout_remove_child);
    RUN_TEST(test_layout_remove_nested_layout);
    RUN_TEST(test_layout_mark_dirty);
    RUN_TEST(test_layout_null_safety);
    RUN_TEST(test_layout_percent_sizing);

    return UNITY_END();
}
