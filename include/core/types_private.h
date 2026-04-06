/**
 * @file types_private.h
 * @brief PRIVATE internal struct definitions for tmlcs-tui core types.
 *
 * This header exposes the full field layout of opaque types for
 * internal compilation units and test code only. NEVER install
 * this header — it is deliberately excluded from the installed
 * include directory.
 *
 * @see include/core/types.h for the public opaque typedefs.
 */

#ifndef CORE_TYPES_PRIVATE_H
#define CORE_TYPES_PRIVATE_H

#include "core/types.h"

/* ============================================================
 * Window — PRIVATE definition
 * ============================================================ */

struct TuiWindow {
    int _id;                         /**< Unique window identifier, auto-incremented */
    struct ncplane* _plane;          /**< Notcurses rendering plane for this window */
    void* _user_data;                /**< Opaque pointer for user-defined data */
    void (*_render_cb)(struct TuiWindow* win);   /**< Called every frame to render content */
    void (*_on_destroy)(struct TuiWindow* win);  /**< Called before window destruction */
    struct TuiTextInput* _text_input;/**< Legacy: primary text input (backward compat) */
    bool _needs_redraw;              /**< If true, window content has changed and needs repainting */
    bool _focused;                   /**< True if this window has active focus (different border) */

    /* Widget registry for mouse event routing */
    void*  _widgets[MAX_WINDOW_WIDGETS];         /**< Registered widget instance pointers */
    int    _widget_type_ids[MAX_WINDOW_WIDGETS]; /**< Registered widget type_ids */
    struct ncplane* _widget_planes[MAX_WINDOW_WIDGETS]; /**< Widget planes for hit testing */
    bool   _widget_focusable[MAX_WINDOW_WIDGETS];/**< Whether each widget can receive focus */
    int    _widget_count;                        /**< Number of registered widgets */
    int    _focused_widget_index;                /**< Index of the currently focused widget (-1 if none) */

    struct TuiLayout* _attached_layout; /**< Layout this window is attached to, if any */
};

/* ============================================================
 * Tab — PRIVATE definition
 * ============================================================ */

struct TuiTab {
    int _id;                         /**< Unique tab identifier, auto-incremented */
    struct ncplane* _plane;          /**< Tab rendering plane (deprecated) */
    struct ncplane* _tab_plane;      /**< Tab container plane (parent of window planes) */
    char _name[64];                  /**< Tab display name */
    TuiWindow** _windows;            /**< Array of window pointers */
    int _window_count;               /**< Current number of windows */
    int _window_capacity;            /**< Allocated capacity of windows array */
    int _active_window_index;        /**< Index of the currently focused window (-1 if none) */
};

/* ============================================================
 * Workspace — PRIVATE definition
 * ============================================================ */

struct TuiWorkspace {
    int _id;                         /**< Unique workspace identifier, auto-incremented */
    struct ncplane* _plane;          /**< Primary workspace rendering plane */
    struct ncplane* _ws_plane;       /**< Workspace container plane (parent of tab planes) */
    char _name[MAX_WORKSPACE_NAME];  /**< Workspace display name */
    TuiTab** _tabs;                  /**< Array of tab pointers */
    int _tab_count;                  /**< Current number of tabs */
    int _tab_capacity;               /**< Allocated capacity of tabs array */
    int _active_tab_index;           /**< Index of the currently active tab (-1 if none) */
};

/* ============================================================
 * Manager — PRIVATE definition
 * ============================================================ */

struct TuiManager {
    struct notcurses* _nc;
    struct ncplane* _stdplane;
    TuiWorkspace** _workspaces;
    int _workspace_count;
    int _workspace_capacity;
    int _active_workspace_index;
    bool _running;
    bool _toolbar_needs_redraw;
    bool _taskbar_needs_redraw;
    bool _tabs_needs_redraw;
    int _last_active_workspace;
    int _last_active_tab;
    int _last_active_window;
    char _last_clock[10];
    TuiWindow* _dragged_window;
    TuiWindow* _resizing_window;
    int _drag_start_y, _drag_start_x;
    int _drag_start_wly, _drag_start_wlx;
    int _drag_start_win_y, _drag_start_win_x;
    int _drag_start_win_height, _drag_start_win_width;
    unsigned _resize_start_dimy, _resize_start_dimx;
    int _resize_start_mouse_y, _resize_start_mouse_x;
};

#endif /* CORE_TYPES_PRIVATE_H */
