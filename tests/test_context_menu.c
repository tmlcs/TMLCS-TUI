#include "unity.h"
#include "core/logger.h"
#include "widget/context_menu.h"
#include <string.h>
#include <stdint.h>

static const char* test_log_file = "/tmp/tmlcs_tui_test_context_menu.log";

int tests_run = 0;
int tests_failed = 0;

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
static void* g_cb_userdata = NULL;

static void test_menu_cb(void* ud) {
    g_cb_called = true;
    g_cb_userdata = ud;
}

static void reset_cb(void) {
    g_cb_called = false;
    g_cb_userdata = NULL;
}

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init(test_log_file);
    reset_cb();
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ================================================================== */
/*  CREATE / DESTROY TESTS                                              */
/* ================================================================== */

void test_menu_create_destroy(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    TEST_ASSERT_NOT_NULL(menu);
    TEST_ASSERT_FALSE(tui_menu_is_visible(menu));
    TEST_ASSERT_EQUAL_INT(0, menu->count);
    TEST_ASSERT_EQUAL_INT(0, menu->selected);

    tui_menu_destroy(menu);
}

void test_menu_destroy_null_safe(void) {
    tui_menu_destroy(NULL);  /* Should not crash */
}

void test_menu_create_null_parent_returns_null(void) {
    TuiContextMenu* menu = tui_menu_create(NULL, 0, 0, 20);
    TEST_ASSERT_NULL(menu);
}

void test_menu_create_width_too_small_returns_null(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 5);
    TEST_ASSERT_NULL(menu);  /* Minimum width is 10 */
}

/* ================================================================== */
/*  ADD ITEM TESTS                                                      */
/* ================================================================== */

void test_menu_add_item_and_activate(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);

    bool r1 = tui_menu_add_item(menu, "Open", test_menu_cb, (void*)1);
    bool r2 = tui_menu_add_item(menu, "Save", test_menu_cb, (void*)2);
    bool r3 = tui_menu_add_item(menu, "Close", test_menu_cb, (void*)3);

    TEST_ASSERT_TRUE(r1);
    TEST_ASSERT_TRUE(r2);
    TEST_ASSERT_TRUE(r3);
    TEST_ASSERT_EQUAL_INT(3, menu->count);

    /* Simulate ENTER on selected item (default selected=0) */
    struct ncinput ni = make_ncinput(NCKEY_ENTER, 0x01 /* NCTYPE_UNKNOWN */);
    tui_menu_show(menu);
    bool consumed = tui_menu_handle_key(menu, NCKEY_ENTER, &ni);
    TEST_ASSERT_TRUE(consumed);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_INT(1, (int)(intptr_t)g_cb_userdata);

    /* Menu should be hidden after ENTER */
    TEST_ASSERT_FALSE(tui_menu_is_visible(menu));

    tui_menu_destroy(menu);
}

void test_menu_add_item_null_label_returns_false(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    bool r = tui_menu_add_item(menu, NULL, NULL, NULL);
    TEST_ASSERT_FALSE(r);
    tui_menu_destroy(menu);
}

void test_menu_add_item_max_reached(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);

    /* Add 16 items (CONTEXT_MENU_MAX_ITEMS) */
    for (int i = 0; i < 16; i++) {
        bool r = tui_menu_add_item(menu, "Item", NULL, NULL);
        TEST_ASSERT_TRUE(r);
    }
    TEST_ASSERT_EQUAL_INT(16, menu->count);

    /* 17th item should fail */
    bool r = tui_menu_add_item(menu, "Overflow", NULL, NULL);
    TEST_ASSERT_FALSE(r);
    TEST_ASSERT_EQUAL_INT(16, menu->count);

    tui_menu_destroy(menu);
}

/* ================================================================== */
/*  SHOW / HIDE / IS_VISIBLE TESTS                                      */
/* ================================================================== */

void test_menu_show_hide_visible(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    tui_menu_add_item(menu, "Item 1", NULL, NULL);
    tui_menu_add_item(menu, "Item 2", NULL, NULL);

    /* Initially hidden */
    TEST_ASSERT_FALSE(tui_menu_is_visible(menu));

    /* Show */
    tui_menu_show(menu);
    TEST_ASSERT_TRUE(tui_menu_is_visible(menu));
    TEST_ASSERT_EQUAL_INT(0, menu->selected);  /* Resets to first item */

    /* Hide */
    tui_menu_hide(menu);
    TEST_ASSERT_FALSE(tui_menu_is_visible(menu));

    /* Show again — selection resets */
    menu->selected = 1;
    tui_menu_show(menu);
    TEST_ASSERT_EQUAL_INT(0, menu->selected);

    tui_menu_destroy(menu);
}

void test_menu_show_hide_null_safe(void) {
    tui_menu_show(NULL);
    tui_menu_hide(NULL);
    TEST_ASSERT_FALSE(tui_menu_is_visible(NULL));
}

/* ================================================================== */
/*  KEYBOARD NAVIGATION TESTS                                           */
/* ================================================================== */

void test_menu_handle_key_navigation(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    tui_menu_add_item(menu, "Alpha", NULL, NULL);
    tui_menu_add_item(menu, "Beta", NULL, NULL);
    tui_menu_add_item(menu, "Gamma", NULL, NULL);
    tui_menu_show(menu);

    struct ncinput ni;

    /* DOWN navigation */
    ni = make_ncinput(NCKEY_DOWN, 0x01);
    TEST_ASSERT_TRUE(tui_menu_handle_key(menu, NCKEY_DOWN, &ni));
    TEST_ASSERT_EQUAL_INT(1, menu->selected);

    ni = make_ncinput(NCKEY_DOWN, 0x01);
    TEST_ASSERT_TRUE(tui_menu_handle_key(menu, NCKEY_DOWN, &ni));
    TEST_ASSERT_EQUAL_INT(2, menu->selected);

    /* DOWN at last item — stays */
    ni = make_ncinput(NCKEY_DOWN, 0x01);
    TEST_ASSERT_FALSE(tui_menu_handle_key(menu, NCKEY_DOWN, &ni));
    TEST_ASSERT_EQUAL_INT(2, menu->selected);

    /* UP navigation */
    ni = make_ncinput(NCKEY_UP, 0x01);
    TEST_ASSERT_TRUE(tui_menu_handle_key(menu, NCKEY_UP, &ni));
    TEST_ASSERT_EQUAL_INT(1, menu->selected);

    ni = make_ncinput(NCKEY_UP, 0x01);
    TEST_ASSERT_TRUE(tui_menu_handle_key(menu, NCKEY_UP, &ni));
    TEST_ASSERT_EQUAL_INT(0, menu->selected);

    /* UP at first item — stays */
    ni = make_ncinput(NCKEY_UP, 0x01);
    TEST_ASSERT_FALSE(tui_menu_handle_key(menu, NCKEY_UP, &ni));
    TEST_ASSERT_EQUAL_INT(0, menu->selected);

    tui_menu_destroy(menu);
}

void test_menu_handle_key_esc_dismiss(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    tui_menu_add_item(menu, "Item", NULL, NULL);
    tui_menu_show(menu);

    struct ncinput ni = make_ncinput(0x1B /* ESC */, 0x01);
    TEST_ASSERT_TRUE(tui_menu_handle_key(menu, 0x1B, &ni));
    TEST_ASSERT_FALSE(tui_menu_is_visible(menu));

    tui_menu_destroy(menu);
}

void test_menu_handle_key_when_hidden_returns_false(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    tui_menu_add_item(menu, "Item", NULL, NULL);

    /* Menu is hidden — all keys should return false */
    struct ncinput ni = make_ncinput(NCKEY_UP, 0x01);
    TEST_ASSERT_FALSE(tui_menu_handle_key(menu, NCKEY_UP, &ni));

    ni = make_ncinput(NCKEY_DOWN, 0x01);
    TEST_ASSERT_FALSE(tui_menu_handle_key(menu, NCKEY_DOWN, &ni));

    ni = make_ncinput(NCKEY_ENTER, 0x01);
    TEST_ASSERT_FALSE(tui_menu_handle_key(menu, NCKEY_ENTER, &ni));

    tui_menu_destroy(menu);
}

void test_menu_handle_key_null_menu_returns_false(void) {
    struct ncinput ni = make_ncinput(NCKEY_DOWN, 0x01);
    TEST_ASSERT_FALSE(tui_menu_handle_key(NULL, NCKEY_DOWN, &ni));
}

/* ================================================================== */
/*  MOUSE HANDLING TESTS                                                */
/* ================================================================== */

void stub_set_plane_position(struct ncplane* plane, int y, int x);
void stub_set_default_plane_position(int y, int x);

void test_menu_handle_mouse_click_selects_and_activates(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    tui_menu_add_item(menu, "Open", test_menu_cb, (void*)100);
    tui_menu_add_item(menu, "Save", test_menu_cb, (void*)200);
    tui_menu_add_item(menu, "Close", test_menu_cb, (void*)300);
    tui_menu_show(menu);

    /* Simulate click on row 1 (Save) */
    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, 0x02 /* NCTYPE_PRESS */);
    ni.y = 1;
    ni.x = 3;

    bool consumed = tui_menu_handle_mouse(menu, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_TRUE(consumed);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_INT(200, (int)(intptr_t)g_cb_userdata);

    /* Menu should be hidden after click */
    TEST_ASSERT_FALSE(tui_menu_is_visible(menu));

    tui_menu_destroy(menu);
}

void test_menu_handle_mouse_when_hidden_returns_false(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    tui_menu_add_item(menu, "Item", NULL, NULL);

    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, 0x02);
    ni.y = 0; ni.x = 3;

    bool consumed = tui_menu_handle_mouse(menu, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_FALSE(consumed);

    tui_menu_destroy(menu);
}

void test_menu_handle_mouse_release_ignored(void) {
    TuiContextMenu* menu = tui_menu_create(dummy_parent(), 0, 0, 20);
    tui_menu_add_item(menu, "Item", NULL, NULL);
    tui_menu_show(menu);

    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, 0x03 /* NCTYPE_RELEASE */);
    ni.y = 0; ni.x = 3;

    bool consumed = tui_menu_handle_mouse(menu, NCKEY_BUTTON1, &ni);
    TEST_ASSERT_FALSE(consumed);

    tui_menu_destroy(menu);
}

void test_menu_handle_mouse_null_menu_returns_false(void) {
    stub_set_default_plane_position(0, 0);
    struct ncinput ni = make_ncinput(NCKEY_BUTTON1, 0x02);
    ni.y = 0; ni.x = 3;

    TEST_ASSERT_FALSE(tui_menu_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

/* ================================================================== */
/*  RENDER NULL-SAFE                                                    */
/* ================================================================== */

void test_menu_render_null_safe(void) {
    tui_menu_render(NULL);  /* Should not crash */
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

int main(void) {
    UNITY_BEGIN();

    /* Create / Destroy */
    RUN_TEST(test_menu_create_destroy);
    RUN_TEST(test_menu_destroy_null_safe);
    RUN_TEST(test_menu_create_null_parent_returns_null);
    RUN_TEST(test_menu_create_width_too_small_returns_null);

    /* Add Item */
    RUN_TEST(test_menu_add_item_and_activate);
    RUN_TEST(test_menu_add_item_null_label_returns_false);
    RUN_TEST(test_menu_add_item_max_reached);

    /* Show / Hide */
    RUN_TEST(test_menu_show_hide_visible);
    RUN_TEST(test_menu_show_hide_null_safe);

    /* Keyboard */
    RUN_TEST(test_menu_handle_key_navigation);
    RUN_TEST(test_menu_handle_key_esc_dismiss);
    RUN_TEST(test_menu_handle_key_when_hidden_returns_false);
    RUN_TEST(test_menu_handle_key_null_menu_returns_false);

    /* Mouse */
    RUN_TEST(test_menu_handle_mouse_click_selects_and_activates);
    RUN_TEST(test_menu_handle_mouse_when_hidden_returns_false);
    RUN_TEST(test_menu_handle_mouse_release_ignored);
    RUN_TEST(test_menu_handle_mouse_null_menu_returns_false);

    /* Render */
    RUN_TEST(test_menu_render_null_safe);

    return UNITY_END();
}
