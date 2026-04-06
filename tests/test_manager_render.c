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

/* Forward declaration for tearDown */
static void reset_render_count(void);

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init("/tmp/tmlcs_tui_test_manager_render.log");
    tui_reset_id(1);
}

void tearDown(void) {
    tui_logger_destroy();
    stub_reset_counters();
    reset_render_count();
    tui_reset_id(1);
}

/* ------------------------------------------------------------------ */
/*  Dummy helpers                                                       */
/* ------------------------------------------------------------------ */

static TuiWorkspace* make_dummy_ws(void) {
    TuiWorkspace* ws = (TuiWorkspace*)calloc(1, sizeof(TuiWorkspace));
    ws->id = tui_next_id();
    strncpy(ws->name, "RenderWS", sizeof(ws->name) - 1);
    ws->name[sizeof(ws->name) - 1] = '\0';
    ws->tab_count = 0;
    ws->tab_capacity = 4;
    ws->tabs = (TuiTab**)calloc(4, sizeof(TuiTab*));
    ws->active_tab_index = -1;
    ws->ws_plane = (struct ncplane*)0xDEAD;
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
    tab->tab_plane = (struct ncplane*)0xBEEF;
    return tab;
}

static TuiWindow* make_dummy_window(void) {
    TuiWindow* win = (TuiWindow*)calloc(1, sizeof(TuiWindow));
    win->id = tui_next_id();
    win->plane = (struct ncplane*)0xCAFE;
    win->needs_redraw = false;
    win->focused = false;
    win->render_cb = NULL;
    win->on_destroy = NULL;
    win->text_input = NULL;
    win->user_data = NULL;
    return win;
}

/* ------------------------------------------------------------------ */
/*  Render callback counter                                             */
/* ------------------------------------------------------------------ */

static int g_render_cb_count = 0;

static void count_render(TuiWindow* win) { (void)win; g_render_cb_count++; }

static void reset_render_count(void) { g_render_cb_count = 0; }

/* ================================================================== */
/*  Tests (6)                                                           */
/* ================================================================== */

/* 1. Render with no active workspace — draws taskbar only, no crash. */
void test_render_no_active_workspace(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* No workspaces added, active_workspace_index == -1 */
    tui_manager_render(mgr);

    tui_manager_destroy(mgr);
}

/* 2. Render with active workspace — 1 workspace, 1 tab, no crash. */
void test_render_with_active_workspace(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    ws->tabs[0] = tab;
    ws->tab_count = 1;
    ws->active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    tui_manager_render(mgr);

    tui_manager_destroy(mgr);
}

/* 3. Render with windows — 1 window with render_cb, should be called once,
 * and window should be focused after render. */
void test_render_with_windows(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win = make_dummy_window();
    win->render_cb = count_render;
    tui_tab_add_window(tab, win);
    tab->active_window_index = 0;
    ws->tabs[0] = tab;
    ws->tab_count = 1;
    ws->active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    g_render_cb_count = 0;
    tui_manager_render(mgr);

    TEST_ASSERT_EQUAL_INT(1, g_render_cb_count);
    TEST_ASSERT_TRUE(win->focused);

    tui_manager_destroy(mgr);
}

/* 4. Render window focus sync — 3 windows, active=1. After render:
 * only the active window has focused==true. */
void test_render_window_focus_sync(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    TuiWindow* wins[3];
    for (int i = 0; i < 3; i++) {
        wins[i] = make_dummy_window();
        wins[i]->render_cb = count_render;
        tui_tab_add_window(tab, wins[i]);
    }
    tab->active_window_index = 1;
    ws->tabs[0] = tab;
    ws->tab_count = 1;
    ws->active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    g_render_cb_count = 0;
    tui_manager_render(mgr);

    TEST_ASSERT_EQUAL_INT(3, g_render_cb_count);
    TEST_ASSERT_FALSE(wins[0]->focused);
    TEST_ASSERT_TRUE(wins[1]->focused);
    TEST_ASSERT_FALSE(wins[2]->focused);

    tui_manager_destroy(mgr);
}

/* 5. Render empty tab — tab with 0 windows, no crash, border still drawn
 * (stub doesn't crash on ncplane_erase/perimeter). */
void test_render_empty_tab(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    /* No windows added, window_count stays 0 */
    ws->tabs[0] = tab;
    ws->tab_count = 1;
    ws->active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    tui_manager_render(mgr);

    tui_manager_destroy(mgr);
}

/* 6. Render multiple workspaces — only active workspace's windows get
 * render_cb called. */
void test_render_multiple_workspaces(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Workspace 0 */
    TuiWorkspace* ws0 = make_dummy_ws();
    strncpy(ws0->name, "WS0", sizeof(ws0->name) - 1);
    TuiTab* tab0 = make_dummy_tab();
    TuiWindow* win0 = make_dummy_window();
    win0->render_cb = count_render;
    tui_tab_add_window(tab0, win0);
    tab0->active_window_index = 0;
    ws0->tabs[0] = tab0;
    ws0->tab_count = 1;
    ws0->active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws0);

    /* Workspace 1 */
    TuiWorkspace* ws1 = make_dummy_ws();
    strncpy(ws1->name, "WS1", sizeof(ws1->name) - 1);
    TuiTab* tab1 = make_dummy_tab();
    TuiWindow* win1 = make_dummy_window();
    win1->render_cb = count_render;
    tui_tab_add_window(tab1, win1);
    tab1->active_window_index = 0;
    ws1->tabs[0] = tab1;
    ws1->tab_count = 1;
    ws1->active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws1);

    /* Set active to workspace 1 */
    tui_manager_set_active_workspace(mgr, 1);

    g_render_cb_count = 0;
    tui_manager_render(mgr);

    /* Only ws1's window should have been rendered */
    TEST_ASSERT_EQUAL_INT(1, g_render_cb_count);

    tui_manager_destroy(mgr);
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_render_no_active_workspace);
    RUN_TEST(test_render_with_active_workspace);
    RUN_TEST(test_render_with_windows);
    RUN_TEST(test_render_window_focus_sync);
    RUN_TEST(test_render_empty_tab);
    RUN_TEST(test_render_multiple_workspaces);

    return UNITY_END();
}
