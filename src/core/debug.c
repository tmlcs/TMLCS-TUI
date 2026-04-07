#include "core/debug.h"
#include "core/types_private.h"
#include <stdio.h>
#include <time.h>

static bool s_timing_enabled = false;
static int s_last_render_time_us = 0;

void tui_debug_dump_planes(TuiManager* mgr) {
    if (!mgr) {
        fprintf(stderr, "[debug] mgr is NULL\n");
        return;
    }
    fprintf(stderr, "[debug] Workspaces: %d/%d (active: %d)\n",
            mgr->_active_workspace_index >= 0 && mgr->_active_workspace_index < mgr->_workspace_count ? 1 : 0,
            mgr->_workspace_count,
            mgr->_workspace_capacity);
    if (mgr->_active_workspace_index >= 0 && mgr->_active_workspace_index < mgr->_workspace_count) {
        TuiWorkspace* ws = mgr->_workspaces[mgr->_active_workspace_index];
        fprintf(stderr, "[debug] Active workspace: \"%s\" (tabs: %d, active_tab: %d)\n",
                ws->_name, ws->_tab_count, ws->_active_tab_index);
        if (ws->_active_tab_index >= 0 && ws->_active_tab_index < ws->_tab_count) {
            TuiTab* tab = ws->_tabs[ws->_active_tab_index];
            fprintf(stderr, "[debug]   Active tab: \"%s\" (windows: %d, active_window: %d)\n",
                    tab->_name, tab->_window_count, tab->_active_window_index);
        }
    }
}

void tui_debug_dump_widgets(const TuiWindow* win) {
    if (!win) {
        fprintf(stderr, "[debug] win is NULL\n");
        return;
    }
    fprintf(stderr, "[debug] Window %d: %d widgets (focused: %d)\n",
            win->_id, win->_widget_count, win->_focused_widget_index);
    for (int i = 0; i < win->_widget_count; i++) {
        int type_id = win->_widget_type_ids[i];
        fprintf(stderr, "[debug]   Widget[%d]: type_id=%d, focusable=%s\n",
                i, type_id, win->_widget_focusable[i] ? "yes" : "no");
    }
}

void tui_debug_dump_dirty_flags(const TuiManager* mgr) {
    if (!mgr) {
        fprintf(stderr, "[debug] mgr is NULL\n");
        return;
    }
    fprintf(stderr, "[debug] Dirty flags: toolbar=%s, tabs=%s, taskbar=%s\n",
            mgr->_toolbar_needs_redraw ? "YES" : "no",
            mgr->_tabs_needs_redraw ? "YES" : "no",
            mgr->_taskbar_needs_redraw ? "YES" : "no");
    fprintf(stderr, "[debug] Last state: ws=%d, tab=%d, win=%d, clock=%s\n",
            mgr->_last_active_workspace,
            mgr->_last_active_tab,
            mgr->_last_active_window,
            mgr->_last_clock);
}

void tui_debug_enable_timing(bool enable) {
    s_timing_enabled = enable;
}

int tui_debug_get_last_render_time_us(void) {
    return s_last_render_time_us;
}

/* Internal: call this from render.c to time renders */
void tui_debug_mark_render_start(struct timespec* out) {
    if (s_timing_enabled) {
        clock_gettime(CLOCK_MONOTONIC, out);
    }
}

void tui_debug_mark_render_end(const struct timespec* start) {
    if (s_timing_enabled && start) {
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        long us = (end.tv_sec - start->tv_sec) * 1000000L + (end.tv_nsec - start->tv_nsec) / 1000L;
        s_last_render_time_us = (int)us;
    }
}
