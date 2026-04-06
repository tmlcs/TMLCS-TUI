#include "unity.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/types.h"
#include "core/types_private.h"
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
    ws->_id = tui_next_id();
    strncpy(ws->_name, "RenderWS", sizeof(ws->_name) - 1);
    ws->_name[sizeof(ws->_name) - 1] = '\0';
    ws->_tab_count = 0;
    ws->_tab_capacity = 4;
    ws->_tabs = (TuiTab**)calloc(4, sizeof(TuiTab*));
    ws->_active_tab_index = -1;
    ws->_ws_plane = (struct ncplane*)0xDEAD;
    return ws;
}

static TuiTab* make_dummy_tab(void) {
    TuiTab* tab = (TuiTab*)calloc(1, sizeof(TuiTab));
    tab->_id = tui_next_id();
    strncpy(tab->_name, "Tab", sizeof(tab->_name) - 1);
    tab->_name[sizeof(tab->_name) - 1] = '\0';
    tab->_window_count = 0;
    tab->_window_capacity = 4;
    tab->_windows = (TuiWindow**)calloc(4, sizeof(TuiWindow*));
    tab->_active_window_index = -1;
    tab->_tab_plane = (struct ncplane*)0xBEEF;
    return tab;
}

static TuiWindow* make_dummy_window(void) {
    TuiWindow* win = (TuiWindow*)calloc(1, sizeof(TuiWindow));
    win->_id = tui_next_id();
    win->_plane = (struct ncplane*)0xCAFE;
    win->_needs_redraw = false;
    win->_focused = false;
    win->_render_cb = NULL;
    win->_on_destroy = NULL;
    win->_text_input = NULL;
    win->_user_data = NULL;
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
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
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
    win->_render_cb = count_render;
    tui_tab_add_window(tab, win);
    tab->_active_window_index = 0;
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    g_render_cb_count = 0;
    tui_manager_render(mgr);

    TEST_ASSERT_EQUAL_INT(1, g_render_cb_count);
    TEST_ASSERT_TRUE(win->_focused);

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
        wins[i]->_render_cb = count_render;
        tui_tab_add_window(tab, wins[i]);
    }
    tab->_active_window_index = 1;
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    g_render_cb_count = 0;
    tui_manager_render(mgr);

    TEST_ASSERT_EQUAL_INT(3, g_render_cb_count);
    TEST_ASSERT_FALSE(wins[0]->_focused);
    TEST_ASSERT_TRUE(wins[1]->_focused);
    TEST_ASSERT_FALSE(wins[2]->_focused);

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
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
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
    strncpy(ws0->_name, "WS0", sizeof(ws0->_name) - 1);
    TuiTab* tab0 = make_dummy_tab();
    TuiWindow* win0 = make_dummy_window();
    win0->_render_cb = count_render;
    tui_tab_add_window(tab0, win0);
    tab0->_active_window_index = 0;
    ws0->_tabs[0] = tab0;
    ws0->_tab_count = 1;
    ws0->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws0);

    /* Workspace 1 */
    TuiWorkspace* ws1 = make_dummy_ws();
    strncpy(ws1->_name, "WS1", sizeof(ws1->_name) - 1);
    TuiTab* tab1 = make_dummy_tab();
    TuiWindow* win1 = make_dummy_window();
    win1->_render_cb = count_render;
    tui_tab_add_window(tab1, win1);
    tab1->_active_window_index = 0;
    ws1->_tabs[0] = tab1;
    ws1->_tab_count = 1;
    ws1->_active_tab_index = 0;
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
/*  Dirty Tracking Tests (6 new)                                        */
/* ================================================================== */

/* 7. Dirty flags initial state — new manager has all dirty flags false */
void test_dirty_flags_initial_state(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TEST_ASSERT_FALSE(mgr->_toolbar_needs_redraw);
    TEST_ASSERT_FALSE(mgr->_tabs_needs_redraw);
    TEST_ASSERT_FALSE(mgr->_taskbar_needs_redraw);
    TEST_ASSERT_EQUAL_INT(-1, mgr->_last_active_workspace);
    TEST_ASSERT_EQUAL_INT(-1, mgr->_last_active_tab);
    TEST_ASSERT_EQUAL_INT(-1, mgr->_last_active_window);

    tui_manager_destroy(mgr);
}

/* 8. Dirty marking API — calling mark functions sets flags */
void test_dirty_marking_api(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    tui_manager_mark_toolbar_dirty(mgr);
    TEST_ASSERT_TRUE(mgr->_toolbar_needs_redraw);
    TEST_ASSERT_FALSE(mgr->_tabs_needs_redraw);
    TEST_ASSERT_FALSE(mgr->_taskbar_needs_redraw);

    tui_manager_mark_tabs_dirty(mgr);
    TEST_ASSERT_TRUE(mgr->_tabs_needs_redraw);

    tui_manager_mark_taskbar_dirty(mgr);
    TEST_ASSERT_TRUE(mgr->_taskbar_needs_redraw);

    tui_manager_mark_full_redraw(mgr);
    TEST_ASSERT_TRUE(mgr->_toolbar_needs_redraw);
    TEST_ASSERT_TRUE(mgr->_tabs_needs_redraw);
    TEST_ASSERT_TRUE(mgr->_taskbar_needs_redraw);

    tui_manager_destroy(mgr);
}

/* 9. Render clears dirty flags — after render, all flags are false */
void test_render_clears_dirty_flags(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    /* Mark all dirty */
    tui_manager_mark_full_redraw(mgr);

    tui_manager_render(mgr);

    /* All flags should be cleared after render */
    TEST_ASSERT_FALSE(mgr->_toolbar_needs_redraw);
    TEST_ASSERT_FALSE(mgr->_tabs_needs_redraw);
    TEST_ASSERT_FALSE(mgr->_taskbar_needs_redraw);

    tui_manager_destroy(mgr);
}

/* 10. Selective window render — only windows with needs_redraw=true get rendered */
void test_selective_window_render(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win1 = make_dummy_window();
    TuiWindow* win2 = make_dummy_window();
    win1->_render_cb = count_render;
    win2->_render_cb = count_render;
    tui_tab_add_window(tab, win1);
    tui_tab_add_window(tab, win2);
    tab->_active_window_index = 0;
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    /* First render: both windows get rendered (focus change detected from -1 to 0) */
    g_render_cb_count = 0;
    tui_manager_render(mgr);
    TEST_ASSERT_EQUAL_INT(2, g_render_cb_count);

    /* Second render: neither marked, no focus change → no render */
    g_render_cb_count = 0;
    tui_manager_render(mgr);
    TEST_ASSERT_EQUAL_INT(0, g_render_cb_count);

    /* Mark only win2 dirty → only win2 rendered */
    win2->_needs_redraw = true;
    g_render_cb_count = 0;
    tui_manager_render(mgr);
    TEST_ASSERT_EQUAL_INT(1, g_render_cb_count);

    tui_manager_destroy(mgr);
}

/* 11. Workspace change triggers full redraw */
void test_workspace_change_triggers_full_redraw(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Workspace 0 */
    TuiWorkspace* ws0 = make_dummy_ws();
    strncpy(ws0->_name, "WS0", sizeof(ws0->_name) - 1);
    TuiTab* tab0 = make_dummy_tab();
    TuiWindow* win0 = make_dummy_window();
    win0->_render_cb = count_render;
    tui_tab_add_window(tab0, win0);
    tab0->_active_window_index = 0;
    ws0->_tabs[0] = tab0;
    ws0->_tab_count = 1;
    ws0->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws0);

    /* Workspace 1 */
    TuiWorkspace* ws1 = make_dummy_ws();
    strncpy(ws1->_name, "WS1", sizeof(ws1->_name) - 1);
    TuiTab* tab1 = make_dummy_tab();
    TuiWindow* win1 = make_dummy_window();
    win1->_render_cb = count_render;
    tui_tab_add_window(tab1, win1);
    tab1->_active_window_index = 0;
    ws1->_tabs[0] = tab1;
    ws1->_tab_count = 1;
    ws1->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws1);

    /* Set active to workspace 0 */
    tui_manager_set_active_workspace(mgr, 0);

    /* First render */
    g_render_cb_count = 0;
    tui_manager_render(mgr);
    TEST_ASSERT_EQUAL_INT(1, g_render_cb_count);

    /* Switch to workspace 1 */
    tui_manager_set_active_workspace(mgr, 1);
    g_render_cb_count = 0;
    tui_manager_render(mgr);
    TEST_ASSERT_EQUAL_INT(1, g_render_cb_count); /* ws1's window rendered */

    tui_manager_destroy(mgr);
}

/* 12. Window focus change triggers all windows redraw */
void test_window_focus_change_triggers_redraw(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab();
    TuiWindow* win1 = make_dummy_window();
    TuiWindow* win2 = make_dummy_window();
    win1->_render_cb = count_render;
    win2->_render_cb = count_render;
    tui_tab_add_window(tab, win1);
    tui_tab_add_window(tab, win2);
    tab->_active_window_index = 0;
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);
    tui_manager_set_active_workspace(mgr, 0);

    /* First render: both windows get rendered (focus change detected) */
    g_render_cb_count = 0;
    tui_manager_render(mgr);
    TEST_ASSERT_EQUAL_INT(2, g_render_cb_count);

    /* Change focus to window 2 */
    tab->_active_window_index = 1;
    g_render_cb_count = 0;
    tui_manager_render(mgr);
    /* Both windows should be re-rendered due to focus change */
    TEST_ASSERT_EQUAL_INT(2, g_render_cb_count);

    tui_manager_destroy(mgr);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_render_no_active_workspace);
    RUN_TEST(test_render_with_active_workspace);
    RUN_TEST(test_render_with_windows);
    RUN_TEST(test_render_window_focus_sync);
    RUN_TEST(test_render_empty_tab);
    RUN_TEST(test_render_multiple_workspaces);

    RUN_TEST(test_dirty_flags_initial_state);
    RUN_TEST(test_dirty_marking_api);
    RUN_TEST(test_render_clears_dirty_flags);
    RUN_TEST(test_selective_window_render);
    RUN_TEST(test_workspace_change_triggers_full_redraw);
    RUN_TEST(test_window_focus_change_triggers_redraw);

    return UNITY_END();
}
