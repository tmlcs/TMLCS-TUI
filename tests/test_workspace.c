#include "unity.h"
#include "core/logger.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

/* ------------------------------------------------------------------ */
/*  Dummy helpers                                                       */
/* ------------------------------------------------------------------ */

static TuiManager* make_dummy_manager(void) {
    TuiManager* mgr = (TuiManager*)calloc(1, sizeof(TuiManager));
    mgr->nc = NULL;
    mgr->stdplane = (struct ncplane*)0xCAFE;
    mgr->workspace_count = 0;
    mgr->workspace_capacity = 4;
    mgr->workspaces = (TuiWorkspace**)calloc(4, sizeof(TuiWorkspace*));
    mgr->active_workspace_index = -1;
    mgr->running = true;
    mgr->dragged_window = NULL;
    mgr->resizing_window = NULL;
    return mgr;
}

static TuiTab* make_dummy_tab(void) {
    TuiTab* tab = (TuiTab*)calloc(1, sizeof(TuiTab));
    tab->id = 999;
    strncpy(tab->name, "DummyTab", sizeof(tab->name) - 1);
    tab->name[sizeof(tab->name) - 1] = '\0';
    tab->window_count = 0;
    tab->window_capacity = 4;
    tab->windows = (TuiWindow**)calloc(4, sizeof(TuiWindow*));
    tab->active_window_index = -1;
    tab->tab_plane = (struct ncplane*)0xBEEF;
    return tab;
}

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init("/tmp/tmlcs_tui_test_workspace.log");
}

void tearDown(void) {
    tui_logger_destroy();
}

/* ------------------------------------------------------------------ */
/*  Tests                                                               */
/* ------------------------------------------------------------------ */

void test_workspace_create_destroy(void) {
    TuiManager* mgr = make_dummy_manager();
    TuiWorkspace* ws = tui_workspace_create(mgr, "TestWS");
    TEST_ASSERT_NOT_NULL(ws);
    tui_workspace_destroy(ws);
    free(mgr->workspaces);
    free(mgr);
}

void test_workspace_create_null_name_returns_null(void) {
    TuiManager* mgr = make_dummy_manager();
    TuiWorkspace* ws = tui_workspace_create(mgr, NULL);
    TEST_ASSERT_NULL(ws);
    free(mgr->workspaces);
    free(mgr);
}

void test_workspace_create_null_manager_returns_null(void) {
    TuiWorkspace* ws = tui_workspace_create(NULL, "name");
    TEST_ASSERT_NULL(ws);
}

void test_workspace_name_is_set(void) {
    TuiManager* mgr = make_dummy_manager();
    TuiWorkspace* ws = tui_workspace_create(mgr, "DevWS");
    TEST_ASSERT_NOT_NULL(ws);
    TEST_ASSERT_EQUAL_STRING("DevWS", ws->name);
    tui_workspace_destroy(ws);
    free(mgr->workspaces);
    free(mgr);
}

void test_workspace_add_tab_increments_count(void) {
    TuiManager* mgr = make_dummy_manager();
    TuiWorkspace* ws = tui_workspace_create(mgr, "TabWS");
    TEST_ASSERT_NOT_NULL(ws);
    TEST_ASSERT_EQUAL_INT(0, ws->tab_count);

    TuiTab* tab = make_dummy_tab();
    tui_workspace_add_tab(ws, tab);
    TEST_ASSERT_EQUAL_INT(1, ws->tab_count);

    /* tui_workspace_destroy destroys all tabs — don't free tab separately */
    tui_workspace_destroy(ws);
    free(mgr->workspaces);
    free(mgr);
}

void test_workspace_set_active_tab(void) {
    TuiManager* mgr = make_dummy_manager();
    TuiWorkspace* ws = tui_workspace_create(mgr, "MultiTabWS");
    TEST_ASSERT_NOT_NULL(ws);

    TuiTab* tab0 = make_dummy_tab();
    TuiTab* tab1 = make_dummy_tab();
    tui_workspace_add_tab(ws, tab0);
    tui_workspace_add_tab(ws, tab1);
    TEST_ASSERT_EQUAL_INT(2, ws->tab_count);

    tui_workspace_set_active_tab(ws, 1);
    TEST_ASSERT_EQUAL_INT(1, ws->active_tab_index);

    tui_workspace_destroy(ws); /* destroys tab0 and tab1 */
    free(mgr->workspaces);
    free(mgr);
}

void test_workspace_set_active_tab_invalid_index(void) {
    TuiManager* mgr = make_dummy_manager();
    TuiWorkspace* ws = tui_workspace_create(mgr, "InvalidTabWS");
    TEST_ASSERT_NOT_NULL(ws);

    TuiTab* tab = make_dummy_tab();
    tui_workspace_add_tab(ws, tab);
    TEST_ASSERT_EQUAL_INT(1, ws->tab_count);
    ws->active_tab_index = 0;

    tui_workspace_set_active_tab(ws, 99);
    TEST_ASSERT_TRUE(ws->active_tab_index == 0);

    tui_workspace_destroy(ws); /* destroys tab */
    free(mgr->workspaces);
    free(mgr);
}

void test_workspace_remove_active_tab(void) {
    TuiManager* mgr = make_dummy_manager();
    TuiWorkspace* ws = tui_workspace_create(mgr, "RemoveTabWS");
    TEST_ASSERT_NOT_NULL(ws);

    TuiTab* tab = make_dummy_tab();
    tui_workspace_add_tab(ws, tab);
    TEST_ASSERT_EQUAL_INT(1, ws->tab_count);

    tui_workspace_remove_active_tab(ws);
    TEST_ASSERT_EQUAL_INT(0, ws->tab_count);
    /* tab was destroyed by tui_workspace_remove_active_tab — don't free it again */

    tui_workspace_destroy(ws);
    free(mgr->workspaces);
    free(mgr);
}

void test_workspace_destroy_null_safe(void) {
    tui_workspace_destroy(NULL);  // must not crash
    TEST_ASSERT_TRUE(1);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_workspace_create_destroy);
    RUN_TEST(test_workspace_create_null_name_returns_null);
    RUN_TEST(test_workspace_create_null_manager_returns_null);
    RUN_TEST(test_workspace_name_is_set);
    RUN_TEST(test_workspace_add_tab_increments_count);
    RUN_TEST(test_workspace_set_active_tab);
    RUN_TEST(test_workspace_set_active_tab_invalid_index);
    RUN_TEST(test_workspace_remove_active_tab);
    RUN_TEST(test_workspace_destroy_null_safe);

    return UNITY_END();
}
