#include "unity.h"
#include "core/logger.h"
#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/types.h"
#include "core/types_private.h"
#include "widget/text_input.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int tests_run = 0;
int tests_failed = 0;

/* Forward declarations for stub extensions */
void stub_reset_counters(void);

/* ------------------------------------------------------------------ */
/*  setUp / tearDown                                                    */
/* ------------------------------------------------------------------ */

void setUp(void) {
    tui_logger_init("/tmp/tmlcs_tui_test_integration.log");
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
    strncpy(ws->_name, "IntWS", sizeof(ws->_name) - 1);
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
    strncpy(tab->_name, "IntTab", sizeof(tab->_name) - 1);
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

static int g_render_count = 0;
static void count_render(TuiWindow* win) { (void)win; g_render_count++; }
static void reset_render_count(void) { g_render_count = 0; }

/* ------------------------------------------------------------------ */
/*  Submit callback spy                                                 */
/* ------------------------------------------------------------------ */

static char g_submit_buf[512];
static bool g_submit_called = false;
static void spy_submit(const char* text, void* userdata) {
    (void)userdata;
    g_submit_called = true;
    strncpy(g_submit_buf, text, sizeof(g_submit_buf) - 1);
    g_submit_buf[sizeof(g_submit_buf) - 1] = '\0';
}
static void reset_submit(void) {
    g_submit_called = false;
    memset(g_submit_buf, 0, sizeof(g_submit_buf));
}

/* ------------------------------------------------------------------ */
/*  ncinput helper                                                      */
/* ------------------------------------------------------------------ */

static struct ncinput make_ni(int y, int x, int evtype) {
    struct ncinput ni;
    memset(&ni, 0, sizeof(ni));
    ni.y = y;
    ni.x = x;
    ni.evtype = evtype;
    ni.alt = false;
    return ni;
}

/* ------------------------------------------------------------------ */
/*  Test 1: Full Lifecycle                                              */
/* ------------------------------------------------------------------ */

void test_integration_full_lifecycle(void) {
    reset_render_count();

    /* Create manager */
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Create workspace, add to manager */
    TuiWorkspace* ws = make_dummy_ws();
    tui_manager_add_workspace(mgr, ws);
    TEST_ASSERT_EQUAL_INT(1, mgr->_workspace_count);

    /* Create tab, add to workspace */
    TuiTab* tab = make_dummy_tab();
    tui_workspace_add_tab(ws, tab);
    TEST_ASSERT_EQUAL_INT(1, ws->_tab_count);

    /* Create window with render_cb, add to tab */
    TuiWindow* win = make_dummy_window();
    win->_render_cb = count_render;
    tui_tab_add_window(tab, win);
    TEST_ASSERT_EQUAL_INT(1, tab->_window_count);

    /* Call render — should invoke our callback */
    tui_manager_render(mgr);
    TEST_ASSERT_TRUE(g_render_count >= 1);

    /* Process keyboard: 'x' closes the active window */
    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN);
    tui_manager_process_keyboard(mgr, 'x', &ni);
    TEST_ASSERT_EQUAL_INT(0, tab->_window_count);

    /* Destroy manager — cascading destroy of ws -> tab (no windows left) */
    tui_manager_destroy(mgr);
}

/* ------------------------------------------------------------------ */
/*  Test 2: Rapid Create/Destroy (ASAN stress)                          */
/* ------------------------------------------------------------------ */

void test_integration_rapid_create_destroy(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Create 50 workspaces, each with 1 tab and 1 window */
    for (int i = 0; i < 50; i++) {
        TuiWorkspace* ws = make_dummy_ws();
        tui_manager_add_workspace(mgr, ws);

        TuiTab* tab = make_dummy_tab();
        tui_workspace_add_tab(ws, tab);

        TuiWindow* win = make_dummy_window();
        tui_tab_add_window(tab, win);
    }

    TEST_ASSERT_EQUAL_INT(50, mgr->_workspace_count);

    /* Destroy manager — cascading destroy of all 50 ws -> tabs -> windows */
    tui_manager_destroy(mgr);
    /* If we reach here without crash, ASAN will verify no leaks */
}

/* ------------------------------------------------------------------ */
/*  Test 3: Workspace Switching                                         */
/* ------------------------------------------------------------------ */

void test_integration_workspace_switching(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Add 3 workspaces */
    for (int i = 0; i < 3; i++) {
        char name[16];
        snprintf(name, sizeof(name), "WS%d", i);
        TuiWorkspace* ws = make_dummy_ws();
        strncpy(ws->_name, name, sizeof(ws->_name) - 1);
        ws->_name[sizeof(ws->_name) - 1] = '\0';
        tui_manager_add_workspace(mgr, ws);
    }

    TEST_ASSERT_EQUAL_INT(3, mgr->_workspace_count);

    /* First added workspace is active */
    TEST_ASSERT_EQUAL_INT(0, mgr->_active_workspace_index);

    /* Switch to workspace 2 */
    tui_manager_set_active_workspace(mgr, 2);
    TEST_ASSERT_EQUAL_INT(2, mgr->_active_workspace_index);

    /* Switch to workspace 1 */
    tui_manager_set_active_workspace(mgr, 1);
    TEST_ASSERT_EQUAL_INT(1, mgr->_active_workspace_index);

    /* Out-of-bounds index should be ignored — active stays at 1 */
    tui_manager_set_active_workspace(mgr, 99);
    TEST_ASSERT_EQUAL_INT(1, mgr->_active_workspace_index);

    /* Switch back to 0 */
    tui_manager_set_active_workspace(mgr, 0);
    TEST_ASSERT_EQUAL_INT(0, mgr->_active_workspace_index);

    tui_manager_destroy(mgr);
}

/* ------------------------------------------------------------------ */
/*  Test 4: Tab Cycling                                                 */
/* ------------------------------------------------------------------ */

void test_integration_tab_cycling(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Create workspace */
    TuiWorkspace* ws = make_dummy_ws();
    tui_manager_add_workspace(mgr, ws);

    /* Add 5 tabs */
    for (int i = 0; i < 5; i++) {
        TuiTab* tab = make_dummy_tab();
        tui_workspace_add_tab(ws, tab);
    }
    TEST_ASSERT_EQUAL_INT(5, ws->_tab_count);
    TEST_ASSERT_EQUAL_INT(0, ws->_active_tab_index);

    /* Manual tab switching */
    tui_workspace_set_active_tab(ws, 2);
    TEST_ASSERT_EQUAL_INT(2, ws->_active_tab_index);

    tui_workspace_set_active_tab(ws, 4);
    TEST_ASSERT_EQUAL_INT(4, ws->_active_tab_index);

    tui_workspace_set_active_tab(ws, 1);
    TEST_ASSERT_EQUAL_INT(1, ws->_active_tab_index);

    /* Keyboard cycling: RIGHT arrow wraps forward */
    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN);
    tui_manager_process_keyboard(mgr, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(2, ws->_active_tab_index); /* 1 -> 2 */

    tui_manager_process_keyboard(mgr, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(3, ws->_active_tab_index); /* 2 -> 3 */

    tui_manager_process_keyboard(mgr, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(4, ws->_active_tab_index); /* 3 -> 4 */

    tui_manager_process_keyboard(mgr, NCKEY_RIGHT, &ni);
    TEST_ASSERT_EQUAL_INT(0, ws->_active_tab_index); /* 4 -> 0 (wrap) */

    /* LEFT arrow wraps backward */
    tui_manager_process_keyboard(mgr, NCKEY_LEFT, &ni);
    TEST_ASSERT_EQUAL_INT(4, ws->_active_tab_index); /* 0 -> 4 (wrap) */

    tui_manager_destroy(mgr);
}

/* ------------------------------------------------------------------ */
/*  Test 5: Window Close and Reopen                                     */
/* ------------------------------------------------------------------ */

void test_integration_window_close_reopen(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Create workspace, tab */
    TuiWorkspace* ws = make_dummy_ws();
    tui_manager_add_workspace(mgr, ws);

    TuiTab* tab = make_dummy_tab();
    tui_workspace_add_tab(ws, tab);

    /* Add window1 */
    TuiWindow* win1 = make_dummy_window();
    tui_tab_add_window(tab, win1);
    TEST_ASSERT_EQUAL_INT(1, tab->_window_count);

    /* Add window2 */
    TuiWindow* win2 = make_dummy_window();
    tui_tab_add_window(tab, win2);
    TEST_ASSERT_EQUAL_INT(2, tab->_window_count);

    /* Remove window1 via tui_tab_remove_window */
    tui_tab_remove_window(tab, win1);
    TEST_ASSERT_EQUAL_INT(1, tab->_window_count);

    /* Destroy window1 manually after removal */
    tui_window_destroy(win1);

    /* Create and add window3 */
    TuiWindow* win3 = make_dummy_window();
    tui_tab_add_window(tab, win3);
    TEST_ASSERT_EQUAL_INT(2, tab->_window_count);
    TEST_ASSERT_EQUAL_INT(1, tab->_active_window_index); /* newest window is active */

    tui_manager_destroy(mgr); /* destroys ws -> tab -> win2, win3 */
}

/* ------------------------------------------------------------------ */
/*  Test 6: Text Input in Window                                        */
/* ------------------------------------------------------------------ */

void test_integration_text_input_in_window(void) {
    reset_submit();

    /* Create a minimal manager/workspace/tab/window hierarchy so the
     * text_input has a valid parent plane. */
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    tui_manager_add_workspace(mgr, ws);

    TuiTab* tab = make_dummy_tab();
    tui_workspace_add_tab(ws, tab);

    TuiWindow* win = make_dummy_window();
    tui_tab_add_window(tab, win);

    /* Create text_input using the window's plane as parent */
    TuiTextInput* ti = tui_text_input_create(win->_plane, 0, 0, 40, spy_submit, NULL);
    TEST_ASSERT_NOT_NULL(ti);

    /* Set focused so it accepts key events */
    ti->focused = true;
    TEST_ASSERT_TRUE(tui_text_input_is_focused(ti));

    /* Simulate typing "Hi" */
    struct ncinput ni = make_ni(0, 0, NCTYPE_UNKNOWN);
    tui_text_input_handle_key(ti, 'H', &ni);
    TEST_ASSERT_EQUAL_INT(1, tui_text_input_get_len(ti));

    tui_text_input_handle_key(ti, 'i', &ni);
    TEST_ASSERT_EQUAL_INT(2, tui_text_input_get_len(ti));

    /* Press Enter to submit */
    tui_text_input_handle_key(ti, NCKEY_ENTER, &ni);

    /* Verify callback received the text */
    TEST_ASSERT_TRUE(g_submit_called);
    TEST_ASSERT_EQUAL_STRING("Hi", g_submit_buf);

    /* Verify buffer was cleared after submit */
    TEST_ASSERT_EQUAL_INT(0, tui_text_input_get_len(ti));

    /* Clean up */
    tui_text_input_destroy(ti);
    tui_manager_destroy(mgr); /* cascades ws -> tab -> win */
}

/* ------------------------------------------------------------------ */
/*  Test 7: Manager Remove Workspace with Windows                       */
/* ------------------------------------------------------------------ */

void test_integration_remove_workspace_with_windows(void) {
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    /* Add ws0 with tab0 and two windows */
    TuiWorkspace* ws0 = make_dummy_ws();
    strncpy(ws0->_name, "WS0", sizeof(ws0->_name) - 1);
    tui_manager_add_workspace(mgr, ws0);

    TuiTab* tab0 = make_dummy_tab();
    tui_workspace_add_tab(ws0, tab0);

    TuiWindow* win0 = make_dummy_window();
    TuiWindow* win1 = make_dummy_window();
    tui_tab_add_window(tab0, win0);
    tui_tab_add_window(tab0, win1);

    /* Add ws1 with tab1 and one window */
    TuiWorkspace* ws1 = make_dummy_ws();
    strncpy(ws1->_name, "WS1", sizeof(ws1->_name) - 1);
    tui_manager_add_workspace(mgr, ws1);

    TuiTab* tab1 = make_dummy_tab();
    tui_workspace_add_tab(ws1, tab1);

    TuiWindow* win2 = make_dummy_window();
    tui_tab_add_window(tab1, win2);

    TEST_ASSERT_EQUAL_INT(2, mgr->_workspace_count);
    TEST_ASSERT_EQUAL_INT(0, mgr->_active_workspace_index); /* ws0 is active */

    /* Remove active workspace (ws0) */
    tui_manager_remove_active_workspace(mgr);

    TEST_ASSERT_EQUAL_INT(1, mgr->_workspace_count);
    /* After removal, the remaining workspace (ws1 at index 0) becomes active */
    TEST_ASSERT_EQUAL_INT(0, mgr->_active_workspace_index);

    tui_manager_destroy(mgr); /* destroys ws1 -> tab1 -> win2 */
}

/* ------------------------------------------------------------------ */
/*  Test 8: Logger During Full Lifecycle                                */
/* ------------------------------------------------------------------ */

void test_integration_logger_during_lifecycle(void) {
    /* Create full hierarchy */
    TuiManager* mgr = tui_manager_create((struct notcurses*)0xBEEF);
    TEST_ASSERT_NOT_NULL(mgr);

    TuiWorkspace* ws = make_dummy_ws();
    tui_manager_add_workspace(mgr, ws);

    TuiTab* tab = make_dummy_tab();
    tui_workspace_add_tab(ws, tab);

    TuiWindow* win = make_dummy_window();
    tui_tab_add_window(tab, win);

    /* Log 5 messages at different levels */
    tui_log(LOG_DEBUG, "Debug msg during lifecycle");
    tui_log(LOG_INFO, "Info msg during lifecycle");
    tui_log(LOG_WARN, "Warn msg during lifecycle");
    tui_log(LOG_ERROR, "Error msg during lifecycle");
    tui_log(LOG_DEBUG, "Final debug msg");

    /* Verify buffer has entries (1 from setUp init + 5 = 6) */
    TuiLogBuffer* buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_EQUAL_INT(6, buf->count);

    /* Verify last message is the final debug one */
    TEST_ASSERT_NOT_NULL(strstr(buf->lines[buf->count - 1], "Final debug msg"));

    /* Destroy all objects */
    tui_manager_destroy(mgr);

    /* Logger is global and independent — buffer should still be accessible */
    buf = tui_logger_get_buffer();
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_EQUAL_INT(6, buf->count);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_integration_full_lifecycle);
    RUN_TEST(test_integration_rapid_create_destroy);
    RUN_TEST(test_integration_workspace_switching);
    RUN_TEST(test_integration_tab_cycling);
    RUN_TEST(test_integration_window_close_reopen);
    RUN_TEST(test_integration_text_input_in_window);
    RUN_TEST(test_integration_remove_workspace_with_windows);
    RUN_TEST(test_integration_logger_during_lifecycle);

    return UNITY_END();
}
