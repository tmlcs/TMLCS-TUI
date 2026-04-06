#include "unity.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

/* Forward declarations for stub extensions */
void notcurses_stdplane_set_fail(int fail);
void stub_reset_counters(void);

/* ------------------------------------------------------------------ */
/*  Dummy helpers                                                       */
/* ------------------------------------------------------------------ */

static TuiWorkspace* make_dummy_workspace_for_manager(void) {
    TuiWorkspace* ws = (TuiWorkspace*)calloc(1, sizeof(TuiWorkspace));
    ws->id = 777;
    strncpy(ws->name, "MgrWS", sizeof(ws->name)-1);
    ws->name[sizeof(ws->name)-1] = '\0';
    ws->tab_count = 0;
    ws->tab_capacity = 4;
    ws->tabs = (TuiTab**)calloc(4, sizeof(TuiTab*));
    ws->active_tab_index = -1;
    ws->ws_plane = (struct ncplane*)0xDEAD;
    return ws;
}

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init("/tmp/tmlcs_tui_test_manager_state.log");
    tui_reset_id(1);
}

void tearDown(void) {
    tui_logger_destroy();
    notcurses_stdplane_set_fail(0);
    stub_reset_counters();
    tui_reset_id(1);
}

/* ------------------------------------------------------------------ */
/*  Tests: Manager Create (3)                                           */
/* ------------------------------------------------------------------ */

void test_manager_create_success(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);
    TEST_ASSERT_TRUE(mgr->nc == (struct notcurses*)0xBEEF);
    TEST_ASSERT_TRUE(mgr->running);
    TEST_ASSERT_EQUAL_INT(0, mgr->workspace_count);
    TEST_ASSERT_EQUAL_INT(-1, mgr->active_workspace_index);
    TEST_ASSERT_EQUAL_INT(4, mgr->workspace_capacity);
    TEST_ASSERT_NOT_NULL(mgr->workspaces);
    tui_manager_destroy(mgr);
}

void test_manager_create_stdplane_fails(void) {
    notcurses_stdplane_set_fail(1);
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NULL(mgr);
    /* Reset flag so tearDown doesn't interfere */
    notcurses_stdplane_set_fail(0);
}

void test_manager_destroy_null_safe(void) {
    tui_manager_destroy(NULL);  /* must not crash */
    TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/*  Tests: Add Workspace (4)                                            */
/* ------------------------------------------------------------------ */

void test_manager_add_workspace_first_sets_active(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_workspace_for_manager();
    tui_manager_add_workspace(mgr, ws);

    TEST_ASSERT_EQUAL_INT(1, mgr->workspace_count);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    /* Manager owns ws — destroy via manager */
    tui_manager_destroy(mgr);
}

void test_manager_add_workspace_second_does_not_change_active(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws0 = make_dummy_workspace_for_manager();
    tui_manager_add_workspace(mgr, ws0);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    TuiWorkspace* ws1 = make_dummy_workspace_for_manager();
    tui_manager_add_workspace(mgr, ws1);

    TEST_ASSERT_EQUAL_INT(2, mgr->workspace_count);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    tui_manager_destroy(mgr);
}

void test_manager_add_workspace_realloc_trigger(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    for (int i = 0; i < 5; i++) {
        TuiWorkspace* ws = make_dummy_workspace_for_manager();
        tui_manager_add_workspace(mgr, ws);
    }

    TEST_ASSERT_EQUAL_INT(5, mgr->workspace_count);
    TEST_ASSERT_TRUE(mgr->workspace_capacity >= 8);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    tui_manager_destroy(mgr);
}

void test_manager_add_workspace_null_is_safe(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    tui_manager_add_workspace(mgr, NULL);  /* Should handle gracefully */
    TEST_ASSERT_EQUAL_INT(0, mgr->workspace_count);

    tui_manager_destroy(mgr);
}

/* ------------------------------------------------------------------ */
/*  Tests: Set Active Workspace (3)                                     */
/* ------------------------------------------------------------------ */

void test_manager_set_active_valid_index(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    for (int i = 0; i < 3; i++) {
        TuiWorkspace* ws = make_dummy_workspace_for_manager();
        tui_manager_add_workspace(mgr, ws);
    }

    tui_manager_set_active_workspace(mgr, 2);
    TEST_ASSERT_EQUAL_INT(2, mgr->active_workspace_index);

    tui_manager_destroy(mgr);
}

void test_manager_set_active_negative_returns_early(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_workspace_for_manager();
    tui_manager_add_workspace(mgr, ws);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    tui_manager_set_active_workspace(mgr, -1);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    tui_manager_destroy(mgr);
}

void test_manager_set_active_out_of_bounds_returns_early(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_workspace_for_manager();
    tui_manager_add_workspace(mgr, ws);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    tui_manager_set_active_workspace(mgr, 99);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    tui_manager_destroy(mgr);
}

/* ------------------------------------------------------------------ */
/*  Tests: Remove Active Workspace (4)                                  */
/* ------------------------------------------------------------------ */

void test_manager_remove_active_single(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_workspace_for_manager();
    tui_manager_add_workspace(mgr, ws);
    TEST_ASSERT_EQUAL_INT(1, mgr->workspace_count);

    tui_manager_remove_active_workspace(mgr);

    TEST_ASSERT_EQUAL_INT(0, mgr->workspace_count);
    TEST_ASSERT_EQUAL_INT(-1, mgr->active_workspace_index);

    /* ws was destroyed by remove — manager has no workspaces left */
    tui_manager_destroy(mgr);
}

void test_manager_remove_active_multiple(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    for (int i = 0; i < 3; i++) {
        TuiWorkspace* ws = make_dummy_workspace_for_manager();
        tui_manager_add_workspace(mgr, ws);
    }
    TEST_ASSERT_EQUAL_INT(3, mgr->workspace_count);
    TEST_ASSERT_EQUAL_INT(0, mgr->active_workspace_index);

    tui_manager_remove_active_workspace(mgr);

    TEST_ASSERT_EQUAL_INT(2, mgr->workspace_count);
    TEST_ASSERT_EQUAL_INT(1, mgr->active_workspace_index);  /* last remaining = count-1 */

    tui_manager_destroy(mgr);  /* destroys remaining 2 workspaces */
}

void test_manager_remove_active_null_manager(void) {
    tui_manager_remove_active_workspace(NULL);  /* must not crash */
    TEST_ASSERT_TRUE(1);
}

void test_manager_remove_active_empty(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);
    TEST_ASSERT_EQUAL_INT(0, mgr->workspace_count);

    tui_manager_remove_active_workspace(mgr);  /* no workspaces, should be safe */

    TEST_ASSERT_EQUAL_INT(0, mgr->workspace_count);

    tui_manager_destroy(mgr);
}

/* ------------------------------------------------------------------ */
/*  Tests: Manager Destroy (1)                                          */
/* ------------------------------------------------------------------ */

void test_manager_destroy_frees_all_workspaces(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws0 = make_dummy_workspace_for_manager();
    TuiWorkspace* ws1 = make_dummy_workspace_for_manager();
    tui_manager_add_workspace(mgr, ws0);
    tui_manager_add_workspace(mgr, ws1);
    TEST_ASSERT_EQUAL_INT(2, mgr->workspace_count);

    /* Destroying the manager destroys both workspaces internally.
     * Do NOT free ws0/ws1 separately — manager owns them. */
    tui_manager_destroy(mgr);

    TEST_ASSERT_TRUE(1);  /* reached here = no crash */
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();

    /* Manager Create */
    RUN_TEST(test_manager_create_success);
    RUN_TEST(test_manager_create_stdplane_fails);
    RUN_TEST(test_manager_destroy_null_safe);

    /* Add Workspace */
    RUN_TEST(test_manager_add_workspace_first_sets_active);
    RUN_TEST(test_manager_add_workspace_second_does_not_change_active);
    RUN_TEST(test_manager_add_workspace_realloc_trigger);
    RUN_TEST(test_manager_add_workspace_null_is_safe);

    /* Set Active Workspace */
    RUN_TEST(test_manager_set_active_valid_index);
    RUN_TEST(test_manager_set_active_negative_returns_early);
    RUN_TEST(test_manager_set_active_out_of_bounds_returns_early);

    /* Remove Active Workspace */
    RUN_TEST(test_manager_remove_active_single);
    RUN_TEST(test_manager_remove_active_multiple);
    RUN_TEST(test_manager_remove_active_null_manager);
    RUN_TEST(test_manager_remove_active_empty);

    /* Manager Destroy */
    RUN_TEST(test_manager_destroy_frees_all_workspaces);

    return UNITY_END();
}
