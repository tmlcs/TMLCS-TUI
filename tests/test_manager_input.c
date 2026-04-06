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

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init("/tmp/tmlcs_tui_test_manager_input.log");
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
    ws->_id = tui_next_id();
    strncpy(ws->_name, "TestWS", sizeof(ws->_name) - 1);
    ws->_name[sizeof(ws->_name) - 1] = '\0';
    ws->_tab_count = 0;
    ws->_tab_capacity = 4;
    ws->_tabs = (TuiTab**)calloc(4, sizeof(TuiTab*));
    ws->_active_tab_index = -1;
    ws->_ws_plane = (struct ncplane*)0xDEAD;
    return ws;
}

static TuiTab* make_dummy_tab_for_input(void) {
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

static TuiWindow* make_dummy_window_for_input(void) {
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

static struct ncinput make_ni(int y, int x, int evtype, bool alt) {
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.y = y;
    ni.x = x;
    ni.evtype = evtype;
    ni.alt = alt;
    return ni;
}

/* ================================================================== */
/*  Tests: get_tab_at (5)                                               */
/* ================================================================== */

void test_get_tab_at_y_not_zero_returns_minus_one(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    strncpy(tab->_name, "Test", sizeof(tab->_name) - 1);
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    int result = tui_manager_get_tab_at(mgr, 1, 32);
    TEST_ASSERT_EQUAL_INT(-1, result);

    tui_manager_destroy(mgr);
}

void test_get_tab_at_no_active_ws_returns_minus_one(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    int result = tui_manager_get_tab_at(mgr, 0, 32);
    TEST_ASSERT_EQUAL_INT(-1, result);

    tui_manager_destroy(mgr);
}

void test_get_tab_at_on_tab_returns_index(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    strncpy(tab->_name, "Test", sizeof(tab->_name) - 1);
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    /* Tab "Test" (4 chars) + TAB_LABEL_PADDING(6) = 10 wide, range [30, 40) */
    int result = tui_manager_get_tab_at(mgr, 0, 32);
    TEST_ASSERT_EQUAL_INT(0, result);

    tui_manager_destroy(mgr);
}

void test_get_tab_at_outside_tab_returns_minus_one(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    strncpy(tab->_name, "Test", sizeof(tab->_name) - 1);
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    /* x=5 is before TAB_BAR_START_X=30 */
    int result = tui_manager_get_tab_at(mgr, 0, 5);
    TEST_ASSERT_EQUAL_INT(-1, result);

    tui_manager_destroy(mgr);
}

void test_get_tab_at_multiple_tabs(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab0 = make_dummy_tab_for_input();
    strncpy(tab0->_name, "TabA", sizeof(tab0->_name) - 1);
    TuiTab* tab1 = make_dummy_tab_for_input();
    strncpy(tab1->_name, "TabB", sizeof(tab1->_name) - 1);
    ws->_tabs[0] = tab0;
    ws->_tabs[1] = tab1;
    ws->_tab_count = 2;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    /* TabA: [30, 40), TabB: [40, 50) */
    int result = tui_manager_get_tab_at(mgr, 0, 45);
    TEST_ASSERT_EQUAL_INT(1, result);

    tui_manager_destroy(mgr);
}

/* ================================================================== */
/*  Tests: Mouse Processing (8)                                         */
/* ================================================================== */

void test_mouse_tab_click_changes_active_tab(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab0 = make_dummy_tab_for_input();
    strncpy(tab0->_name, "TabA", sizeof(tab0->_name) - 1);
    TuiTab* tab1 = make_dummy_tab_for_input();
    strncpy(tab1->_name, "TabB", sizeof(tab1->_name) - 1);
    ws->_tabs[0] = tab0;
    ws->_tabs[1] = tab1;
    ws->_tab_count = 2;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    /* Click at x=45 which falls in TabB range [40, 50) */
    struct ncinput ni = make_ni(0, 45, NCTYPE_PRESS, false);
    bool result = tui_manager_process_mouse(mgr, NCKEY_BUTTON1, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, ws->_active_tab_index);

    tui_manager_destroy(mgr);
}

void test_mouse_release_clears_drag(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWindow* dummy_win = make_dummy_window_for_input();
    mgr->_dragged_window = dummy_win;

    struct ncinput ni = make_ni(0, 0, NCTYPE_RELEASE, false);
    bool result = tui_manager_process_mouse(mgr, NCKEY_BUTTON1, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(mgr->_dragged_window);

    free(dummy_win);
    tui_manager_destroy(mgr);
}

void test_mouse_release_clears_resize(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWindow* dummy_win = make_dummy_window_for_input();
    mgr->_resizing_window = dummy_win;

    struct ncinput ni = make_ni(0, 0, NCTYPE_RELEASE, false);
    bool result = tui_manager_process_mouse(mgr, NCKEY_BUTTON1, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(mgr->_resizing_window);

    free(dummy_win);
    tui_manager_destroy(mgr);
}

void test_mouse_button_outside_tab_no_window(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    /* Click at y=5 which is not y=0 (tab bar), and tab has no windows */
    struct ncinput ni = make_ni(5, 5, NCTYPE_PRESS, false);
    bool result = tui_manager_process_mouse(mgr, NCKEY_BUTTON1, &ni);

    TEST_ASSERT_TRUE(result);

    tui_manager_destroy(mgr);
}

void test_mouse_window_focus(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    TuiWindow* win = make_dummy_window_for_input();
    tui_tab_add_window(tab, win);
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    /* Click at abs (5,5): stub ncplane_abs_yx returns (0,0), stub ncplane_dim_yx
     * returns (24,80), so window covers [0,24)x[0,80). wly=5, wlx=5. */
    struct ncinput ni = make_ni(5, 5, NCTYPE_PRESS, false);
    bool result = tui_manager_process_mouse(mgr, NCKEY_BUTTON1, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(win->_needs_redraw);

    tui_manager_destroy(mgr);
}

void test_mouse_close_button_zone(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    TuiWindow* win = make_dummy_window_for_input();
    tui_tab_add_window(tab, win);
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    /* stub ncplane_abs_yx returns y=0, ncplane_dim_yx returns 24x80.
     * Close zone: wly==0 && wlx >= 80-4=76 && wlx <= 79.
     * ni->y=0, ni->x=78 → wly=0, wlx=78 → in close zone. */
    struct ncinput ni = make_ni(0, 78, NCTYPE_PRESS, false);
    bool result = tui_manager_process_mouse(mgr, NCKEY_BUTTON1, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, tab->_window_count);

    tui_manager_destroy(mgr);
}

void test_mouse_resize_grip_zone(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    TuiWindow* win = make_dummy_window_for_input();
    tui_tab_add_window(tab, win);
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    /* stub ncplane_abs_yx returns y=0, ncplane_dim_yx returns 24x80.
     * Grip zone: wly==23 && wlx >= 80-3=77 && wlx <= 79.
     * ni->y=23, ni->x=78 → wly=23, wlx=78 → in grip zone. */
    struct ncinput ni = make_ni(23, 78, NCTYPE_PRESS, false);
    bool result = tui_manager_process_mouse(mgr, NCKEY_BUTTON1, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(mgr->_resizing_window == win);

    tui_manager_destroy(mgr);
}

void test_mouse_button_always_returns_true(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    struct ncinput ni = make_ni(50, 50, NCTYPE_PRESS, false);
    bool result = tui_manager_process_mouse(mgr, NCKEY_BUTTON5, &ni);

    TEST_ASSERT_TRUE(result);

    tui_manager_destroy(mgr);
}

/* ================================================================== */
/*  Tests: Keyboard Processing (12)                                     */
/* ================================================================== */

void test_key_q_stops_manager(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, 'q', &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(mgr->_running);

    tui_manager_destroy(mgr);
}

void test_key_Q_stops_manager(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, 'Q', &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(mgr->_running);

    tui_manager_destroy(mgr);
}

void test_key_release_returns_false(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    struct ncinput ni = make_ni(0, 0, NCTYPE_RELEASE, false);
    bool result = tui_manager_process_keyboard(mgr, 'a', &ni);

    TEST_ASSERT_FALSE(result);

    tui_manager_destroy(mgr);
}

void test_alt_1_switches_to_workspace_0(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Add 3 workspaces so index 0,1,2 exist */
    for (int i = 0; i < 3; i++) {
        TuiWorkspace* ws = make_dummy_ws();
        tui_manager_add_workspace(mgr, ws);
    }
    /* Set active to workspace 2 */
    tui_manager_set_active_workspace(mgr, 2);
    TEST_ASSERT_EQUAL_INT(2, mgr->_active_workspace_index);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, true);
    bool result = tui_manager_process_keyboard(mgr, '1', &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, mgr->_active_workspace_index);

    tui_manager_destroy(mgr);
}

void test_right_arrow_cycles_tabs_forward(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    for (int i = 0; i < 3; i++) {
        TuiTab* tab = make_dummy_tab_for_input();
        tui_workspace_add_tab(ws, tab);
    }
    ws->_active_tab_index = 1;
    tui_manager_add_workspace(mgr, ws);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, NCKEY_RIGHT, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, ws->_active_tab_index);

    tui_manager_destroy(mgr);
}

void test_right_arrow_wraps_to_zero(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    for (int i = 0; i < 3; i++) {
        TuiTab* tab = make_dummy_tab_for_input();
        tui_workspace_add_tab(ws, tab);
    }
    ws->_active_tab_index = 2;
    tui_manager_add_workspace(mgr, ws);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, NCKEY_RIGHT, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, ws->_active_tab_index);

    tui_manager_destroy(mgr);
}

void test_left_arrow_cycles_tabs_backward(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    for (int i = 0; i < 3; i++) {
        TuiTab* tab = make_dummy_tab_for_input();
        tui_workspace_add_tab(ws, tab);
    }
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, NCKEY_LEFT, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, ws->_active_tab_index);

    tui_manager_destroy(mgr);
}

void test_tab_key_cycles_windows(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    tui_workspace_add_tab(ws, tab);
    /* Add 3 windows using proper tui_window_create flow */
    TuiWindow* win0 = make_dummy_window_for_input();
    TuiWindow* win1 = make_dummy_window_for_input();
    TuiWindow* win2 = make_dummy_window_for_input();
    tui_tab_add_window(tab, win0);
    tui_tab_add_window(tab, win1);
    tui_tab_add_window(tab, win2);
    tab->_active_window_index = 1;
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, NCKEY_TAB, &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, tab->_active_window_index);

    tui_manager_destroy(mgr);
}

void test_x_key_closes_active_window(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    tui_workspace_add_tab(ws, tab);
    TuiWindow* win = make_dummy_window_for_input();
    tui_tab_add_window(tab, win);
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, 'x', &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, tab->_window_count);

    tui_manager_destroy(mgr);
}

void test_w_key_closes_active_tab(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, 'w', &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, ws->_tab_count);

    tui_manager_destroy(mgr);
}

void test_d_key_closes_active_workspace(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    tui_manager_add_workspace(mgr, ws);
    TEST_ASSERT_EQUAL_INT(1, mgr->_workspace_count);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, 'd', &ni);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, mgr->_workspace_count);

    tui_manager_destroy(mgr);
}

void test_unknown_key_returns_false(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    TuiTab* tab = make_dummy_tab_for_input();
    ws->_tabs[0] = tab;
    ws->_tab_count = 1;
    ws->_active_tab_index = 0;
    tui_manager_add_workspace(mgr, ws);

    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN, false);
    bool result = tui_manager_process_keyboard(mgr, 'z', &ni);

    TEST_ASSERT_FALSE(result);

    tui_manager_destroy(mgr);
}

/* ================================================================== */
/*  main                                                                */
/* ================================================================== */

int main(void) {
    UNITY_BEGIN();

    /* get_tab_at (5) */
    RUN_TEST(test_get_tab_at_y_not_zero_returns_minus_one);
    RUN_TEST(test_get_tab_at_no_active_ws_returns_minus_one);
    RUN_TEST(test_get_tab_at_on_tab_returns_index);
    RUN_TEST(test_get_tab_at_outside_tab_returns_minus_one);
    RUN_TEST(test_get_tab_at_multiple_tabs);

    /* Mouse Processing (8) */
    RUN_TEST(test_mouse_tab_click_changes_active_tab);
    RUN_TEST(test_mouse_release_clears_drag);
    RUN_TEST(test_mouse_release_clears_resize);
    RUN_TEST(test_mouse_button_outside_tab_no_window);
    RUN_TEST(test_mouse_window_focus);
    RUN_TEST(test_mouse_close_button_zone);
    RUN_TEST(test_mouse_resize_grip_zone);
    RUN_TEST(test_mouse_button_always_returns_true);

    /* Keyboard Processing (12) */
    RUN_TEST(test_key_q_stops_manager);
    RUN_TEST(test_key_Q_stops_manager);
    RUN_TEST(test_key_release_returns_false);
    RUN_TEST(test_alt_1_switches_to_workspace_0);
    RUN_TEST(test_right_arrow_cycles_tabs_forward);
    RUN_TEST(test_right_arrow_wraps_to_zero);
    RUN_TEST(test_left_arrow_cycles_tabs_backward);
    RUN_TEST(test_tab_key_cycles_windows);
    RUN_TEST(test_x_key_closes_active_window);
    RUN_TEST(test_w_key_closes_active_tab);
    RUN_TEST(test_d_key_closes_active_workspace);
    RUN_TEST(test_unknown_key_returns_false);

    return UNITY_END();
}
