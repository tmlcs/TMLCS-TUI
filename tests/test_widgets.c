#include "unity.h"
#include "core/logger.h"
#include "widget/label.h"
#include "widget/button.h"
#include "widget/progress.h"
#include "widget/list.h"
#include "widget/textarea.h"
#include "widget/checkbox.h"
#include "core/types.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_widgets.log";

/* ------------------------------------------------------------------ */
/*  Dummy parent plane                                                  */
/* ------------------------------------------------------------------ */

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

/* ------------------------------------------------------------------ */
/*  ncinput helper                                                      */
/* ------------------------------------------------------------------ */

static struct ncinput make_ncinput(uint32_t key, int evtype) {
    (void)key;
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = evtype;
    ni.y = 0;
    ni.x = 0;
    ni.alt = false;
    return ni;
}

/* ------------------------------------------------------------------ */
/*  Callback spies                                                      */
/* ------------------------------------------------------------------ */

static bool g_cb_called = false;

static void test_button_cb(void* ud) {
    (void)ud;
    g_cb_called = true;
}

static void test_checkbox_cb(bool checked, void* ud) {
    (void)checked;
    (void)ud;
    g_cb_called = true;
}

static void reset_cb(void) {
    g_cb_called = false;
}

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init(test_log_file);
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ================================================================== */
/*  LABEL TESTS                                                         */
/* ================================================================== */

void test_label_create_destroy(void) {
    TuiLabel* lbl = tui_label_create(dummy_parent(), 0, 0, "Hello");
    TEST_ASSERT_NOT_NULL(lbl);
    tui_label_destroy(lbl);
}

void test_label_get_text(void) {
    TuiLabel* lbl = tui_label_create(dummy_parent(), 0, 0, "Test");
    TEST_ASSERT_NOT_NULL(lbl);
    const char* text = tui_label_get_text(lbl);
    TEST_ASSERT_NOT_NULL(text);
    TEST_ASSERT_EQUAL_STRING("Test", text);
    tui_label_destroy(lbl);
}

void test_label_set_text(void) {
    TuiLabel* lbl = tui_label_create(dummy_parent(), 0, 0, "Old");
    TEST_ASSERT_NOT_NULL(lbl);
    TEST_ASSERT_EQUAL_STRING("Old", tui_label_get_text(lbl));
    tui_label_set_text(lbl, "New");
    TEST_ASSERT_EQUAL_STRING("New", tui_label_get_text(lbl));
    tui_label_destroy(lbl);
}

void test_label_render_null_safe(void) {
    tui_label_render(NULL);  /* Should not crash */
    TEST_ASSERT_TRUE(1);
}

/* ================================================================== */
/*  BUTTON TESTS                                                        */
/* ================================================================== */

void test_button_create_destroy(void) {
    TuiButton* btn = tui_button_create(dummy_parent(), 0, 0, 10, "OK", NULL, NULL);
    TEST_ASSERT_NOT_NULL(btn);
    tui_button_destroy(btn);
}

void test_button_handle_key_enter(void) {
    reset_cb();
    TuiButton* btn = tui_button_create(dummy_parent(), 0, 0, 10, "OK", test_button_cb, NULL);
    TEST_ASSERT_NOT_NULL(btn);
    btn->focused = true;
    struct ncinput ni = make_ncinput(NCKEY_ENTER, NCTYPE_UNKNOWN);
    bool consumed = tui_button_handle_key(btn, NCKEY_ENTER, &ni);
    TEST_ASSERT_TRUE(consumed);
    TEST_ASSERT_TRUE(g_cb_called);
    tui_button_destroy(btn);
}

void test_button_handle_key_release_ignored(void) {
    reset_cb();
    TuiButton* btn = tui_button_create(dummy_parent(), 0, 0, 10, "OK", test_button_cb, NULL);
    TEST_ASSERT_NOT_NULL(btn);
    btn->focused = true;
    struct ncinput ni = make_ncinput(NCKEY_ENTER, NCTYPE_RELEASE);
    bool consumed = tui_button_handle_key(btn, NCKEY_ENTER, &ni);
    TEST_ASSERT_FALSE(consumed);
    TEST_ASSERT_FALSE(g_cb_called);
    tui_button_destroy(btn);
}

void test_button_set_label(void) {
    TuiButton* btn = tui_button_create(dummy_parent(), 0, 0, 10, "Old", NULL, NULL);
    TEST_ASSERT_NOT_NULL(btn);
    tui_button_set_label(btn, "New");
    tui_button_render(btn);
    tui_button_destroy(btn);
}

void test_button_is_pressed(void) {
    TuiButton* btn = tui_button_create(dummy_parent(), 0, 0, 10, "OK", NULL, NULL);
    TEST_ASSERT_NOT_NULL(btn);
    TEST_ASSERT_FALSE(tui_button_is_pressed(btn));
    tui_button_destroy(btn);
}

void test_button_set_focused(void) {
    TuiButton* btn = tui_button_create(dummy_parent(), 0, 0, 10, "OK", NULL, NULL);
    TEST_ASSERT_NOT_NULL(btn);
    tui_button_set_focused(btn, true);
    tui_button_render(btn);
    tui_button_destroy(btn);
}

/* ================================================================== */
/*  PROGRESS BAR TESTS                                                  */
/* ================================================================== */

void test_progress_create_destroy(void) {
    TuiProgressBar* pb = tui_progress_create(dummy_parent(), 0, 0, 20);
    TEST_ASSERT_NOT_NULL(pb);
    TEST_ASSERT_EQUAL_INT(0.0f, pb->value);
    tui_progress_destroy(pb);
}

void test_progress_set_value(void) {
    TuiProgressBar* pb = tui_progress_create(dummy_parent(), 0, 0, 20);
    TEST_ASSERT_NOT_NULL(pb);
    tui_progress_set_value(pb, 0.5f);
    float val = tui_progress_get_value(pb);
    TEST_ASSERT_TRUE(fabsf(val - 0.5f) < 0.001f);
    tui_progress_destroy(pb);
}

void test_progress_value_clamped_low(void) {
    TuiProgressBar* pb = tui_progress_create(dummy_parent(), 0, 0, 20);
    TEST_ASSERT_NOT_NULL(pb);
    tui_progress_set_value(pb, -1.0f);
    float val = tui_progress_get_value(pb);
    TEST_ASSERT_TRUE(fabsf(val - 0.0f) < 0.001f);
    tui_progress_destroy(pb);
}

void test_progress_value_clamped_high(void) {
    TuiProgressBar* pb = tui_progress_create(dummy_parent(), 0, 0, 20);
    TEST_ASSERT_NOT_NULL(pb);
    tui_progress_set_value(pb, 2.0f);
    float val = tui_progress_get_value(pb);
    TEST_ASSERT_TRUE(fabsf(val - 1.0f) < 0.001f);
    tui_progress_destroy(pb);
}

void test_progress_render_null_safe(void) {
    tui_progress_render(NULL);  /* Should not crash */
    TEST_ASSERT_TRUE(1);
}

/* ================================================================== */
/*  LIST TESTS                                                          */
/* ================================================================== */

void test_list_create_destroy(void) {
    TuiList* list = tui_list_create(dummy_parent(), 0, 0, 20, 5);
    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_EQUAL_INT(0, list->count);
    TEST_ASSERT_EQUAL_INT(-1, list->selected);
    tui_list_destroy(list);
}

void test_list_add_item(void) {
    TuiList* list = tui_list_create(dummy_parent(), 0, 0, 20, 5);
    TEST_ASSERT_NOT_NULL(list);
    tui_list_add_item(list, "One");
    TEST_ASSERT_EQUAL_INT(1, tui_list_get_count(list));
    tui_list_destroy(list);
}

void test_list_add_multiple(void) {
    TuiList* list = tui_list_create(dummy_parent(), 0, 0, 20, 5);
    TEST_ASSERT_NOT_NULL(list);
    tui_list_add_item(list, "One");
    tui_list_add_item(list, "Two");
    tui_list_add_item(list, "Three");
    TEST_ASSERT_EQUAL_INT(3, tui_list_get_count(list));
    TEST_ASSERT_EQUAL_INT(0, tui_list_get_selected(list));
    tui_list_destroy(list);
}

void test_list_remove_item(void) {
    TuiList* list = tui_list_create(dummy_parent(), 0, 0, 20, 5);
    TEST_ASSERT_NOT_NULL(list);
    tui_list_add_item(list, "One");
    tui_list_add_item(list, "Two");
    tui_list_add_item(list, "Three");
    TEST_ASSERT_EQUAL_INT(3, tui_list_get_count(list));
    tui_list_remove_item(list, 1);  /* Remove "Two" */
    TEST_ASSERT_EQUAL_INT(2, tui_list_get_count(list));
    tui_list_destroy(list);
}

void test_list_get_selected(void) {
    TuiList* list = tui_list_create(dummy_parent(), 0, 0, 20, 5);
    TEST_ASSERT_NOT_NULL(list);
    tui_list_add_item(list, "One");
    tui_list_add_item(list, "Two");
    tui_list_add_item(list, "Three");
    tui_list_set_selected(list, 2);
    TEST_ASSERT_EQUAL_INT(2, tui_list_get_selected(list));
    tui_list_destroy(list);
}

void test_list_handle_key_up(void) {
    TuiList* list = tui_list_create(dummy_parent(), 0, 0, 20, 5);
    TEST_ASSERT_NOT_NULL(list);
    tui_list_add_item(list, "One");
    tui_list_add_item(list, "Two");
    tui_list_add_item(list, "Three");
    tui_list_set_selected(list, 1);
    TEST_ASSERT_EQUAL_INT(1, tui_list_get_selected(list));
    list->focused = true;
    struct ncinput ni = make_ncinput(NCKEY_UP, NCTYPE_UNKNOWN);
    tui_list_handle_key(list, NCKEY_UP, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_list_get_selected(list));
    tui_list_destroy(list);
}

void test_list_handle_key_down(void) {
    TuiList* list = tui_list_create(dummy_parent(), 0, 0, 20, 5);
    TEST_ASSERT_NOT_NULL(list);
    tui_list_add_item(list, "One");
    tui_list_add_item(list, "Two");
    tui_list_add_item(list, "Three");
    tui_list_set_selected(list, 0);
    TEST_ASSERT_EQUAL_INT(0, tui_list_get_selected(list));
    list->focused = true;
    struct ncinput ni = make_ncinput(NCKEY_DOWN, NCTYPE_UNKNOWN);
    tui_list_handle_key(list, NCKEY_DOWN, &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_list_get_selected(list));
    tui_list_destroy(list);
}

/* ================================================================== */
/*  TEXTAREA TESTS                                                      */
/* ================================================================== */

void test_textarea_create_destroy(void) {
    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    TEST_ASSERT_NOT_NULL(ta);
    TEST_ASSERT_EQUAL_INT(1, ta->line_count);  /* Starts with one empty line */
    tui_textarea_destroy(ta);
}

void test_textarea_handle_key_insert(void) {
    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    TEST_ASSERT_NOT_NULL(ta);
    ta->focused = true;
    struct ncinput ni = make_ncinput('a', NCTYPE_UNKNOWN);
    tui_textarea_handle_key(ta, 'a', &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_textarea_get_line_count(ta));
    tui_textarea_destroy(ta);
}

void test_textarea_handle_key_enter(void) {
    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    TEST_ASSERT_NOT_NULL(ta);
    ta->focused = true;
    /* Insert "hi" */
    struct ncinput ni = make_ncinput('h', NCTYPE_UNKNOWN);
    tui_textarea_handle_key(ta, 'h', &ni);
    ni = make_ncinput('i', NCTYPE_UNKNOWN);
    tui_textarea_handle_key(ta, 'i', &ni);
    /* Press Enter to create new line */
    ni = make_ncinput(NCKEY_ENTER, NCTYPE_UNKNOWN);
    tui_textarea_handle_key(ta, NCKEY_ENTER, &ni);
    TEST_ASSERT_EQUAL_INT(2, tui_textarea_get_line_count(ta));
    tui_textarea_destroy(ta);
}

void test_textarea_clear(void) {
    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    TEST_ASSERT_NOT_NULL(ta);
    ta->focused = true;
    /* Insert some text */
    struct ncinput ni = make_ncinput('h', NCTYPE_UNKNOWN);
    tui_textarea_handle_key(ta, 'h', &ni);
    ni = make_ncinput('i', NCTYPE_UNKNOWN);
    tui_textarea_handle_key(ta, 'i', &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_textarea_get_line_count(ta));
    /* Clear */
    tui_textarea_clear(ta);
    TEST_ASSERT_EQUAL_INT(1, tui_textarea_get_line_count(ta));  /* One empty line */
    tui_textarea_destroy(ta);
}

/* ================================================================== */
/*  CHECKBOX TESTS                                                      */
/* ================================================================== */

void test_checkbox_create_destroy(void) {
    TuiCheckbox* cb = tui_checkbox_create(dummy_parent(), 0, 0, "Option", false, NULL, NULL);
    TEST_ASSERT_NOT_NULL(cb);
    tui_checkbox_destroy(cb);
}

void test_checkbox_get_state(void) {
    TuiCheckbox* cb = tui_checkbox_create(dummy_parent(), 0, 0, "Option", true, NULL, NULL);
    TEST_ASSERT_NOT_NULL(cb);
    TEST_ASSERT_TRUE(tui_checkbox_get_state(cb));
    tui_checkbox_destroy(cb);
}

void test_checkbox_handle_key_space(void) {
    reset_cb();
    TuiCheckbox* cb = tui_checkbox_create(dummy_parent(), 0, 0, "Option", false, test_checkbox_cb, NULL);
    TEST_ASSERT_NOT_NULL(cb);
    TEST_ASSERT_FALSE(tui_checkbox_get_state(cb));
    cb->focused = true;
    struct ncinput ni = make_ncinput(' ', NCTYPE_UNKNOWN);
    bool consumed = tui_checkbox_handle_key(cb, ' ', &ni);
    TEST_ASSERT_TRUE(consumed);
    TEST_ASSERT_TRUE(tui_checkbox_get_state(cb));  /* Toggled to true */
    TEST_ASSERT_TRUE(g_cb_called);
    tui_checkbox_destroy(cb);
}

void test_checkbox_set_state(void) {
    TuiCheckbox* cb = tui_checkbox_create(dummy_parent(), 0, 0, "Option", false, NULL, NULL);
    TEST_ASSERT_NOT_NULL(cb);
    TEST_ASSERT_FALSE(tui_checkbox_get_state(cb));
    tui_checkbox_set_state(cb, true);
    TEST_ASSERT_TRUE(tui_checkbox_get_state(cb));
    tui_checkbox_destroy(cb);
}

/* ================================================================== */
/*  MOUSE HANDLING TESTS                                                */
/* ================================================================== */

void stub_set_default_plane_position(int y, int x);

void test_button_handle_mouse_click(void) {
    reset_cb();
    TuiButton* btn = tui_button_create(dummy_parent(), 0, 0, 12, "OK", test_button_cb, NULL);
    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, 0x02 /* NCTYPE_PRESS */);
    ni.y = 0; ni.x = 5;
    bool r = tui_button_handle_mouse(btn, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_TRUE(r);
    TEST_ASSERT_TRUE(g_cb_called);
    tui_button_destroy(btn);
}

void test_button_handle_mouse_outside(void) {
    TuiButton* btn = tui_button_create(dummy_parent(), 0, 0, 12, "OK", NULL, NULL);
    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, 0x02);
    ni.y = 99; ni.x = 99;
    bool r = tui_button_handle_mouse(btn, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_FALSE(r);
    tui_button_destroy(btn);
}

void test_checkbox_handle_mouse_toggle(void) {
    reset_cb();
    TuiCheckbox* cb = tui_checkbox_create(dummy_parent(), 0, 0, "Enable", false, test_checkbox_cb, NULL);
    TEST_ASSERT_FALSE(tui_checkbox_get_state(cb));
    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, 0x02);
    ni.y = 0; ni.x = 2;
    bool r = tui_checkbox_handle_mouse(cb, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_TRUE(r);
    TEST_ASSERT_TRUE(tui_checkbox_get_state(cb));
    TEST_ASSERT_TRUE(g_cb_called);
    tui_checkbox_destroy(cb);
}

void test_list_handle_mouse_select(void) {
    TuiList* list = tui_list_create(dummy_parent(), 0, 0, 20, 5);
    tui_list_add_item(list, "Alpha");
    tui_list_add_item(list, "Beta");
    tui_list_add_item(list, "Gamma");
    TEST_ASSERT_EQUAL_INT(0, tui_list_get_selected(list));

    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, 0x02);
    ni.y = 1; ni.x = 3;  /* Click on row 1 => item "Beta" (index 1) */
    bool r = tui_list_handle_mouse(list, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_TRUE(r);
    TEST_ASSERT_EQUAL_INT(1, tui_list_get_selected(list));
    tui_list_destroy(list);
}

/* ================================================================== */
/*  TEXTAREA GET TESTS                                                  */
/* ================================================================== */

void test_textarea_get_single_line(void) {
    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    /* Directly set line content (handle_key may not work with stubs) */
    strncpy(ta->lines[0], "hello", TEXTAREA_MAX_LINE_LEN - 1);
    ta->line_count = 1;
    ta->lines[0][5] = '\0';

    char buf[64];
    int n = tui_textarea_get(ta, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(5, n);
    TEST_ASSERT_EQUAL_STRING("hello", buf);
    tui_textarea_destroy(ta);
}

void test_textarea_get_multi_line(void) {
    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    strncpy(ta->lines[0], "hi", TEXTAREA_MAX_LINE_LEN - 1);
    strncpy(ta->lines[1], "bye", TEXTAREA_MAX_LINE_LEN - 1);
    ta->line_count = 2;

    char buf[64];
    int n = tui_textarea_get(ta, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(6, n);  /* "hi\nbye" = 2 + 1 + 3 */
    TEST_ASSERT_EQUAL_STRING("hi\nbye", buf);
    tui_textarea_destroy(ta);
}

void test_textarea_get_empty(void) {
    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    char buf[64];
    int n = tui_textarea_get(ta, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, n);
    TEST_ASSERT_EQUAL_STRING("", buf);
    tui_textarea_destroy(ta);
}

void test_textarea_get_buffer_overflow(void) {
    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    strncpy(ta->lines[0], "aaaaaaaaaa", TEXTAREA_MAX_LINE_LEN - 1);
    ta->line_count = 1;

    char small_buf[4];
    int n = tui_textarea_get(ta, small_buf, sizeof(small_buf));
    TEST_ASSERT_EQUAL_INT(3, n);  /* max_len-1 = 3 bytes written */
    TEST_ASSERT_EQUAL_INT(0, small_buf[3]);  /* null terminated */
    tui_textarea_destroy(ta);
}

void test_textarea_get_null_params(void) {
    char buf[64];
    TEST_ASSERT_EQUAL_INT(-1, tui_textarea_get(NULL, buf, sizeof(buf)));

    TuiTextArea* ta = tui_textarea_create(dummy_parent(), 0, 0, 40, 5);
    TEST_ASSERT_EQUAL_INT(-1, tui_textarea_get(ta, NULL, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(-1, tui_textarea_get(ta, buf, 0));
    tui_textarea_destroy(ta);
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

int main(void) {
    UNITY_BEGIN();

    /* Label tests */
    RUN_TEST(test_label_create_destroy);
    RUN_TEST(test_label_get_text);
    RUN_TEST(test_label_set_text);
    RUN_TEST(test_label_render_null_safe);

    /* Button tests */
    RUN_TEST(test_button_create_destroy);
    RUN_TEST(test_button_handle_key_enter);
    RUN_TEST(test_button_handle_key_release_ignored);
    RUN_TEST(test_button_set_label);
    RUN_TEST(test_button_is_pressed);
    RUN_TEST(test_button_set_focused);

    /* Progress bar tests */
    RUN_TEST(test_progress_create_destroy);
    RUN_TEST(test_progress_set_value);
    RUN_TEST(test_progress_value_clamped_low);
    RUN_TEST(test_progress_value_clamped_high);
    RUN_TEST(test_progress_render_null_safe);

    /* List tests */
    RUN_TEST(test_list_create_destroy);
    RUN_TEST(test_list_add_item);
    RUN_TEST(test_list_add_multiple);
    RUN_TEST(test_list_remove_item);
    RUN_TEST(test_list_get_selected);
    RUN_TEST(test_list_handle_key_up);
    RUN_TEST(test_list_handle_key_down);

    /* TextArea tests */
    RUN_TEST(test_textarea_create_destroy);
    RUN_TEST(test_textarea_handle_key_insert);
    RUN_TEST(test_textarea_handle_key_enter);
    RUN_TEST(test_textarea_clear);

    /* Checkbox tests */
    RUN_TEST(test_checkbox_create_destroy);
    RUN_TEST(test_checkbox_get_state);
    RUN_TEST(test_checkbox_handle_key_space);
    RUN_TEST(test_checkbox_set_state);

    /* Mouse handling tests */
    RUN_TEST(test_button_handle_mouse_click);
    RUN_TEST(test_button_handle_mouse_outside);
    RUN_TEST(test_checkbox_handle_mouse_toggle);
    RUN_TEST(test_list_handle_mouse_select);

    /* TextArea get tests */
    RUN_TEST(test_textarea_get_single_line);
    RUN_TEST(test_textarea_get_multi_line);
    RUN_TEST(test_textarea_get_empty);
    RUN_TEST(test_textarea_get_buffer_overflow);
    RUN_TEST(test_textarea_get_null_params);

    return UNITY_END();
}
