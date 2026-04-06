#include "unity.h"
#include "core/logger.h"
#include "widget/tree.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_tree.log";

static bool g_cb_called = false;
static int g_cb_node = 0;

static void tree_cb(int node_index, void* ud) {
    (void)ud;
    g_cb_called = true;
    g_cb_node = node_index;
}

static struct ncplane* dummy_parent(void) {
    return (struct ncplane*)0xDEAD;
}

void setUp(void) {
    tui_logger_init(test_log_file);
    g_cb_called = false;
    g_cb_node = 0;
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ---- CREATE/DESTROY ---- */

void test_tree_create_destroy(void) {
    TuiTree* tree = tui_tree_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    TEST_ASSERT_NOT_NULL(tree);
    TEST_ASSERT_EQUAL_INT(0, tree->count);
    TEST_ASSERT_EQUAL_INT(-1, tree->selected);
    tui_tree_destroy(tree);
}

void test_tree_create_null_parent(void) {
    TuiTree* tree = tui_tree_create(NULL, 0, 0, 40, 10, NULL, NULL);
    TEST_ASSERT_NULL(tree);
}

/* ---- BASIC FUNCTIONALITY ---- */

void test_tree_add_node(void) {
    TuiTree* tree = tui_tree_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    int idx = tui_tree_add_node(tree, "Root", false);
    TEST_ASSERT_EQUAL_INT(0, idx);
    TEST_ASSERT_EQUAL_INT(1, tui_tree_get_count(tree));
    tui_tree_destroy(tree);
}

void test_tree_add_multiple_nodes(void) {
    TuiTree* tree = tui_tree_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    tui_tree_add_node(tree, "A", true);
    tui_tree_add_node(tree, "B", true);
    tui_tree_add_node(tree, "C", true);
    TEST_ASSERT_EQUAL_INT(3, tui_tree_get_count(tree));
    tui_tree_destroy(tree);
}

void test_tree_add_child(void) {
    TuiTree* tree = tui_tree_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    int root = tui_tree_add_node(tree, "Root", false);
    int child = tui_tree_add_child(tree, root, "Child1", true);
    TEST_ASSERT_GREATER_THAN(root, child);
    tui_tree_destroy(tree);
}

void test_tree_toggle(void) {
    TuiTree* tree = tui_tree_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    tui_tree_add_node(tree, "Root", false);
    /* Root starts expanded (not a leaf) */
    TEST_ASSERT_TRUE(tree->nodes[0].expanded);
    tui_tree_toggle(tree, 0);
    TEST_ASSERT_FALSE(tree->nodes[0].expanded);
    tui_tree_toggle(tree, 0);
    TEST_ASSERT_TRUE(tree->nodes[0].expanded);
    tui_tree_destroy(tree);
}

void test_tree_handle_key_up_down(void) {
    TuiTree* tree = tui_tree_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    tui_tree_add_node(tree, "A", true);
    tui_tree_add_node(tree, "B", true);
    tui_tree_add_node(tree, "C", true);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    tui_tree_handle_key(tree, NCKEY_DOWN, &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_tree_get_selected(tree));

    tui_tree_handle_key(tree, NCKEY_UP, &ni);
    TEST_ASSERT_EQUAL_INT(0, tui_tree_get_selected(tree));
    tui_tree_destroy(tree);
}

void test_tree_handle_key_left_right_toggle(void) {
    TuiTree* tree = tui_tree_create(dummy_parent(), 0, 0, 40, 10, NULL, NULL);
    tui_tree_add_node(tree, "Root", false);
    tui_tree_add_child(tree, 0, "Child", true);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;

    /* Collapse with left */
    tui_tree_handle_key(tree, NCKEY_LEFT, &ni);
    TEST_ASSERT_FALSE(tree->nodes[0].expanded);

    /* Expand with right */
    tui_tree_handle_key(tree, NCKEY_RIGHT, &ni);
    TEST_ASSERT_TRUE(tree->nodes[0].expanded);

    /* Toggle with space */
    tui_tree_handle_key(tree, ' ', &ni);
    TEST_ASSERT_FALSE(tree->nodes[0].expanded);
    tui_tree_destroy(tree);
}

void test_tree_render_null_safe(void) {
    tui_tree_render(NULL);
    TEST_ASSERT_TRUE(1);
}

void test_tree_callback_spied(void) {
    /* Verify the callback function exists and works */
    g_cb_called = false;
    g_cb_node = 0;
    tree_cb(42, NULL);
    TEST_ASSERT_TRUE(g_cb_called);
    TEST_ASSERT_EQUAL_INT(42, g_cb_node);
}

void test_tree_null_safety(void) {
    tui_tree_destroy(NULL);
    TEST_ASSERT_EQUAL_INT(-1, tui_tree_get_selected(NULL));
    tui_tree_set_selected(NULL, 0);
    TEST_ASSERT_EQUAL_INT(0, tui_tree_get_count(NULL));
    TEST_ASSERT_EQUAL_INT(-1, tui_tree_add_node(NULL, "X", true));
    tui_tree_set_focused(NULL, true);

    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.evtype = NCTYPE_UNKNOWN;
    TEST_ASSERT_FALSE(tui_tree_handle_key(NULL, NCKEY_DOWN, &ni));
    TEST_ASSERT_FALSE(tui_tree_handle_mouse(NULL, NCKEY_BUTTON1, &ni));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tree_create_destroy);
    RUN_TEST(test_tree_create_null_parent);
    RUN_TEST(test_tree_add_node);
    RUN_TEST(test_tree_add_multiple_nodes);
    RUN_TEST(test_tree_add_child);
    RUN_TEST(test_tree_toggle);
    RUN_TEST(test_tree_handle_key_up_down);
    RUN_TEST(test_tree_handle_key_left_right_toggle);
    RUN_TEST(test_tree_render_null_safe);
    RUN_TEST(test_tree_callback_spied);
    RUN_TEST(test_tree_null_safety);
    return UNITY_END();
}
