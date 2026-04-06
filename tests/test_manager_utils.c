#include "unity.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/types.h"
#include <stdlib.h>
#include <string.h>

int tests_run = 0;
int tests_failed = 0;

void stub_reset_counters(void);

/* Forward declaration for the internal utility function */
bool window_belongs_to_workspace(TuiWindow* win, TuiWorkspace* ws);

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init("/tmp/tmlcs_tui_test_manager_utils.log");
    tui_reset_id(1);
}

void tearDown(void) {
    tui_logger_destroy();
    stub_reset_counters();
    tui_reset_id(1);
}

/* ------------------------------------------------------------------ */
/*  Dummy helpers                                                       */
/* ------------------------------------------------------------------ */

static TuiWorkspace* make_dummy_ws(void) {
    TuiWorkspace* ws = (TuiWorkspace*)calloc(1, sizeof(TuiWorkspace));
    ws->id = tui_next_id();
    strncpy(ws->name, "UtilsWS", sizeof(ws->name) - 1);
    ws->name[sizeof(ws->name) - 1] = '\0';
    ws->tab_count = 0;
    ws->tab_capacity = 4;
    ws->tabs = (TuiTab**)calloc(4, sizeof(TuiTab*));
    ws->active_tab_index = -1;
    /* No ncplane sentinel needed — utils.c doesn't call notcurses */
    ws->ws_plane = NULL;
    return ws;
}

static TuiTab* make_dummy_tab(void) {
    TuiTab* tab = (TuiTab*)calloc(1, sizeof(TuiTab));
    tab->id = tui_next_id();
    strncpy(tab->name, "Tab", sizeof(tab->name) - 1);
    tab->name[sizeof(tab->name) - 1] = '\0';
    tab->window_count = 0;
    tab->window_capacity = 4;
    tab->windows = (TuiWindow**)calloc(4, sizeof(TuiWindow*));
    tab->active_window_index = -1;
    tab->tab_plane = NULL;
    return tab;
}

static TuiWindow* make_dummy_window(void) {
    TuiWindow* win = (TuiWindow*)calloc(1, sizeof(TuiWindow));
    win->id = tui_next_id();
    win->plane = NULL;
    win->needs_redraw = false;
    win->focused = false;
    win->render_cb = NULL;
    win->on_destroy = NULL;
    win->text_input = NULL;
    win->user_data = NULL;
    return win;
}

/* ================================================================== */
/*  Tests (7)                                                           */
/* ================================================================== */

/* 1. Null window returns false */
void test_window_belongs_null_win_returns_false(void) {
    TuiWorkspace* ws = make_dummy_ws();
    TEST_ASSERT_FALSE(window_belongs_to_workspace(NULL, ws));
    free(ws->tabs);
    free(ws);
}

/* 2. Null workspace returns false */
void test_window_belongs_null_ws_returns_false(void) {
    TuiWindow* win = make_dummy_window();
    TEST_ASSERT_FALSE(window_belongs_to_workspace(win, NULL));
    free(win);
}

/* 3. Both null returns false */
void test_window_belongs_both_null_returns_false(void) {
    TEST_ASSERT_FALSE(window_belongs_to_workspace(NULL, NULL));
}

/* 4. Window present in workspace's tab returns true */
void test_window_belongs_when_present(void) {
    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = make_dummy_window();
    tui_tab_add_window(tab, win);
    ws->tabs[0] = tab;
    ws->tab_count = 1;

    TEST_ASSERT_TRUE(window_belongs_to_workspace(win, ws));

    free(tab->windows);
    free(tab);
    free(ws->tabs);
    free(ws);
    free(win);
}

/* 5. Window not present in workspace returns false */
void test_window_belongs_when_not_present(void) {
    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    /* tab has 0 windows */
    ws->tabs[0] = tab;
    ws->tab_count = 1;

    TuiWindow* win2 = make_dummy_window();

    TEST_ASSERT_FALSE(window_belongs_to_workspace(win2, ws));

    free(tab->windows);
    free(tab);
    free(ws->tabs);
    free(ws);
    free(win2);
}

/* 6. Window in second tab of multi-tab workspace returns true */
void test_window_belongs_in_multi_tab_workspace(void) {
    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab0 = make_dummy_tab();
    TuiTab* tab1 = make_dummy_tab();
    TuiWindow* win = make_dummy_window();
    tui_tab_add_window(tab1, win);
    ws->tabs[0] = tab0;
    ws->tabs[1] = tab1;
    ws->tab_count = 2;

    TEST_ASSERT_TRUE(window_belongs_to_workspace(win, ws));

    free(tab0->windows);
    free(tab0);
    free(tab1->windows);
    free(tab1);
    free(ws->tabs);
    free(ws);
    free(win);
}

/* 7. Empty workspace (0 tabs) returns false */
void test_window_belongs_empty_workspace(void) {
    TuiWorkspace* ws = make_dummy_ws();
    /* ws->tab_count is 0 */
    TuiWindow* win = make_dummy_window();

    TEST_ASSERT_FALSE(window_belongs_to_workspace(win, ws));

    free(ws->tabs);
    free(ws);
    free(win);
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_window_belongs_null_win_returns_false);
    RUN_TEST(test_window_belongs_null_ws_returns_false);
    RUN_TEST(test_window_belongs_both_null_returns_false);
    RUN_TEST(test_window_belongs_when_present);
    RUN_TEST(test_window_belongs_when_not_present);
    RUN_TEST(test_window_belongs_in_multi_tab_workspace);
    RUN_TEST(test_window_belongs_empty_workspace);

    return UNITY_END();
}
