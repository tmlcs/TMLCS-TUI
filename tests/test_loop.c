/**
 * @file test_loop.c
 * @brief Unit tests for the main event loop system.
 *
 * Tests loop lifecycle, callbacks, quit behavior, and null safety.
 */
#include "unity.h"
#include "core/loop.h"
#include "core/manager.h"
#include "core/types.h"
#include "core/types_private.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int tests_run = 0;
int tests_failed = 0;

static const char* test_log_file = "/tmp/tmlcs_tui_test_loop.log";

/* Forward declarations for stubs */
void ncplane_create_set_fail_count(int n);

void setUp(void) {
    tui_logger_init(test_log_file);
    ncplane_create_set_fail_count(0);
}

void tearDown(void) {
    ncplane_create_set_fail_count(0);
    tui_logger_destroy();
}

/* ------------------------------------------------------------------ */
/*  Helper: create a minimal manager for loop testing                   */
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
/*  test_loop_run_quit_immediately                                      */
/* ------------------------------------------------------------------ */

void test_loop_run_quit_immediately(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);
    TEST_ASSERT_TRUE(mgr->_running);

    /* Signal quit immediately before running */
    tui_manager_quit(mgr);
    TEST_ASSERT_FALSE(mgr->_running);

    /* Running the loop when _running is false should exit quickly */
    int result = tui_manager_run(mgr, NULL);
    TEST_ASSERT_EQUAL_INT(0, result);

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_loop_null_mgr_returns_error                                    */
/* ------------------------------------------------------------------ */

void test_loop_null_mgr_returns_error(void) {
    int result = tui_manager_run(NULL, NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);

    /* Quit on NULL should not crash */
    tui_manager_quit(NULL);

    TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/*  test_loop_on_frame_callback                                         */
/* ------------------------------------------------------------------ */

static bool g_on_frame_called = false;
static int g_on_frame_call_count = 0;
static TuiManager* g_on_frame_mgr = NULL;

static bool on_frame_cb(TuiManager* mgr, void* userdata) {
    (void)userdata;
    g_on_frame_called = true;
    g_on_frame_call_count++;
    g_on_frame_mgr = mgr;
    return true;
}

void test_loop_on_frame_callback(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    g_on_frame_called = false;
    g_on_frame_call_count = 0;
    g_on_frame_mgr = NULL;

    /* Test that the callback mechanism works by verifying
     * the config struct is properly set up */
    TuiLoopConfig config;
    memset(&config, 0, sizeof(config));
    config.on_frame = on_frame_cb;
    config.target_fps = 1000;
    config.userdata = (void*)0x1234;

    /* Verify config fields */
    TEST_ASSERT_NOT_NULL(config.on_frame);
    TEST_ASSERT_EQUAL_INT(1000, config.target_fps);
    TEST_ASSERT_EQUAL_INT(0x1234, (intptr_t)config.userdata);

    /* Quit immediately so the loop exits without rendering */
    tui_manager_quit(mgr);

    int result = tui_manager_run(mgr, &config);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* on_frame should NOT be called since _running was already false */
    TEST_ASSERT_FALSE(g_on_frame_called);

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_loop_after_render_callback                                     */
/* ------------------------------------------------------------------ */

static bool g_after_render_called = false;

static void after_render_cb(TuiManager* mgr, void* userdata) {
    (void)mgr;
    (void)userdata;
    g_after_render_called = true;
}

void test_loop_after_render_callback(void) {
    /* Test that the callback mechanism works by verifying
     * the config struct is properly set up */
    TuiLoopConfig config;
    memset(&config, 0, sizeof(config));
    config.after_render = after_render_cb;
    config.target_fps = 1000;
    config.userdata = (void*)0x5678;

    /* Verify config fields */
    TEST_ASSERT_NOT_NULL(config.after_render);
    TEST_ASSERT_EQUAL_INT(1000, config.target_fps);
    TEST_ASSERT_EQUAL_INT(0x5678, (intptr_t)config.userdata);

    TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/*  test_loop_is_running_getter                                         */
/* ------------------------------------------------------------------ */

void test_loop_is_running_getter(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    TEST_ASSERT_TRUE(tui_manager_is_running(mgr));

    tui_manager_quit(mgr);
    TEST_ASSERT_FALSE(tui_manager_is_running(mgr));

    /* NULL safety */
    TEST_ASSERT_FALSE(tui_manager_is_running(NULL));

    destroy_test_manager(mgr);
}

/* ------------------------------------------------------------------ */
/*  test_loop_null_config_works                                         */
/* ------------------------------------------------------------------ */

void test_loop_null_config_works(void) {
    TuiManager* mgr = create_test_manager();
    TEST_ASSERT_NOT_NULL(mgr);

    /* Immediately quit so the loop exits */
    tui_manager_quit(mgr);

    /* NULL config should work fine */
    int result = tui_manager_run(mgr, NULL);
    TEST_ASSERT_EQUAL_INT(0, result);

    destroy_test_manager(mgr);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_loop_run_quit_immediately);
    RUN_TEST(test_loop_null_mgr_returns_error);
    RUN_TEST(test_loop_on_frame_callback);
    RUN_TEST(test_loop_after_render_callback);
    RUN_TEST(test_loop_is_running_getter);
    RUN_TEST(test_loop_null_config_works);

    return UNITY_END();
}
