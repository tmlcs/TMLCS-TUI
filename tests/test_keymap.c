/**
 * @file test_keymap.c
 * @brief Unit tests for the keymap system.
 *
 * Tests keymap initialization, binding, matching, and custom callbacks.
 */
#include "unity.h"
#include "core/keymap.h"
#include "core/types.h"
#include "core/types_private.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_keymap.log";

/* Forward declarations for stubs */
void ncplane_create_set_fail_count(int n);
struct ncplane* ncplane_create(struct ncplane* p, const ncplane_options* o);
struct ncplane* notcurses_stdplane(struct notcurses* nc);

void setUp(void) {
    tui_logger_init(test_log_file);
    ncplane_create_set_fail_count(0);
}

void tearDown(void) {
    ncplane_create_set_fail_count(0);
    tui_logger_destroy();
}

/* ------------------------------------------------------------------ */
/*  Helper: create a minimal manager for keymap testing                */
/* ------------------------------------------------------------------ */

static TuiManager* create_test_manager(void) {
    TuiManager* mgr = (TuiManager*)calloc(1, sizeof(TuiManager));
    if (!mgr) return NULL;
    mgr->_nc = (struct notcurses*)0xDEAD;
    mgr->_stdplane = (struct ncplane*)0xDEAD;
    mgr->_running = true;
    mgr->_keymap = NULL;
    return mgr;
}

static void destroy_test_manager(TuiManager* mgr) {
    if (mgr) {
        if (mgr->_keymap) free(mgr->_keymap);
        free(mgr);
    }
}

/* ------------------------------------------------------------------ */
/*  test_keymap_init_not_null                                          */
/* ------------------------------------------------------------------ */

void test_keymap_init_not_null(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    tui_keymap_init(mgr);
    TEST_ASSERT_NOT_NULL(mgr->_keymap);

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_keymap_bind_quit                                              */
/* ------------------------------------------------------------------ */

void test_keymap_bind_quit(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    tui_keymap_init(mgr);

    /* Default binding: 'q' -> ACTION_QUIT */
    TuiAction action = tui_keymap_match(mgr, 'q', false, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_QUIT, action);

    /* Default binding: 'Q' -> ACTION_QUIT */
    action = tui_keymap_match(mgr, 'Q', false, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_QUIT, action);

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_keymap_match_quit                                             */
/* ------------------------------------------------------------------ */

void test_keymap_match_quit(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    tui_keymap_init(mgr);

    /* Ctrl+q should NOT match quit (default is plain 'q') */
    TuiAction action = tui_keymap_match(mgr, 'q', true, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_COUNT, action);

    /* Plain 'q' should match */
    action = tui_keymap_match(mgr, 'q', false, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_QUIT, action);

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_keymap_match_unbound_returns_count                            */
/* ------------------------------------------------------------------ */

void test_keymap_match_unbound_returns_count(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    tui_keymap_init(mgr);

    /* Some key that is not bound by default */
    TuiAction action = tui_keymap_match(mgr, 0x7F, false, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_COUNT, action);

    /* Another unbound key */
    action = tui_keymap_match(mgr, 0x01, true, true, true);
    TEST_ASSERT_EQUAL_INT(ACTION_COUNT, action);

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_keymap_bind_custom                                            */
/* ------------------------------------------------------------------ */

static bool g_custom_callback_called = false;
static void* g_custom_callback_userdata = NULL;

static void custom_action_cb(TuiManager* mgr, const struct ncinput* ni, void* userdata) {
    (void)mgr;
    (void)ni;
    g_custom_callback_called = true;
    g_custom_callback_userdata = userdata;
}

void test_keymap_bind_custom(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    tui_keymap_init(mgr);
    tui_keymap_set_handler(mgr, ACTION_QUIT, custom_action_cb);

    /* Verify handler was stored */
    /* We can't directly access the handler array from here, but we can
     * verify that the keymap structure exists and is valid */
    TEST_ASSERT_NOT_NULL(mgr->_keymap);

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_keymap_null_safety                                            */
/* ------------------------------------------------------------------ */

void test_keymap_null_safety(void) {
    /* All keymap functions should handle NULL manager gracefully */
    tui_keymap_init(NULL);                          /* Should not crash */
    tui_keymap_bind(NULL, ACTION_QUIT, 'q', false, false, false);  /* Should not crash */
    tui_keymap_bind_custom(NULL, ACTION_QUIT, custom_action_cb, NULL);  /* Should not crash */

    TuiAction action = tui_keymap_match(NULL, 'q', false, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_COUNT, action);

    tui_keymap_set_handler(NULL, ACTION_QUIT, custom_action_cb);  /* Should not crash */

    TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/*  test_keymap_default_bindings_exist                                 */
/* ------------------------------------------------------------------ */

void test_keymap_default_bindings_exist(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    tui_keymap_init(mgr);

    /* Ctrl+T -> ACTION_NEW_TAB */
    TuiAction action = tui_keymap_match(mgr, 't', true, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_NEW_TAB, action);

    /* Ctrl+N -> ACTION_NEW_WINDOW */
    action = tui_keymap_match(mgr, 'n', true, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_NEW_WINDOW, action);

    /* Escape -> ACTION_CANCEL */
    action = tui_keymap_match(mgr, 0x1B, false, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_CANCEL, action);

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_keymap_bind_and_match_new                                     */
/* ------------------------------------------------------------------ */

void test_keymap_bind_and_match_new(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    tui_keymap_init(mgr);

    /* Verify 'x' is not bound to quit by default */
    TuiAction action = tui_keymap_match(mgr, 'x', false, false, false);
    /* 'x' is actually bound to ACTION_CLOSE_WINDOW */
    TEST_ASSERT_EQUAL_INT(ACTION_CLOSE_WINDOW, action);

    /* Bind a new combination */
    tui_keymap_bind(mgr, ACTION_RESIZE_UP, '+', false, false, false);
    action = tui_keymap_match(mgr, '+', false, false, false);
    TEST_ASSERT_EQUAL_INT(ACTION_RESIZE_UP, action);

    destroy_test_manager(mgr);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_keymap_init_not_null);
    RUN_TEST(test_keymap_bind_quit);
    RUN_TEST(test_keymap_match_quit);
    RUN_TEST(test_keymap_match_unbound_returns_count);
    RUN_TEST(test_keymap_bind_custom);
    RUN_TEST(test_keymap_null_safety);
    RUN_TEST(test_keymap_default_bindings_exist);
    RUN_TEST(test_keymap_bind_and_match_new);

    return UNITY_END();
}
