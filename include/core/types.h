/**
 * @file types.h
 * @brief Core type definitions for the tmlcs-tui framework.
 *
 * Defines opaque types for the primary data structures used
 * throughout the library: TuiWindow (content containers),
 * TuiTab (window collections), TuiWorkspace (tab collections),
 * and TuiManager (workspace collections).
 *
 * All struct fields are hidden behind opaque typedefs for ABI
 * stability. Internal field access requires types_private.h.
 */

#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <stdbool.h>
#include <notcurses/notcurses.h>

/* ============================================================
 * Constants
 * ============================================================ */

/** @ingroup core_types Maximum number of widgets per window. */
#define MAX_WINDOW_WIDGETS 16
/** @ingroup core_types Maximum number of workspaces. */
#define MAX_WORKSPACES 9
/** @ingroup core_types Maximum workspace name length. */
#define MAX_WORKSPACE_NAME 64

/* ============================================================
 * Widget Type Enumeration
 * ============================================================ */

/**
 * @brief Widget type identifiers for mouse event routing.
 *
 * Each widget registered on a window has a type that determines
 * how mouse events are dispatched to its handler function.
 */
typedef enum {
    WIDGET_NONE = -1,              /**< Invalid/uninitialized widget type */
    WIDGET_LABEL = 0,              /**< Static text label */
    WIDGET_BUTTON,                 /**< Clickable button */
    WIDGET_CHECKBOX,               /**< Toggle checkbox */
    WIDGET_PROGRESS,               /**< Progress bar */
    WIDGET_LIST,                   /**< Scrollable item list */
    WIDGET_TEXT_INPUT,             /**< Single-line text input */
    WIDGET_TEXTAREA,               /**< Multi-line text area */
    WIDGET_CONTEXT_MENU,           /**< Popup context menu */
    WIDGET_COUNT                   /**< Number of widget types (for bounds checking) */
} TuiWidgetType;

/* Backward-compatible aliases */
#define TUI_WIDGET_NONE           WIDGET_NONE
#define TUI_WIDGET_LABEL          WIDGET_LABEL
#define TUI_WIDGET_BUTTON         WIDGET_BUTTON
#define TUI_WIDGET_CHECKBOX       WIDGET_CHECKBOX
#define TUI_WIDGET_PROGRESS       WIDGET_PROGRESS
#define TUI_WIDGET_LIST           WIDGET_LIST
#define TUI_WIDGET_TEXT_INPUT     WIDGET_TEXT_INPUT
#define TUI_WIDGET_TEXTAREA       WIDGET_TEXTAREA
#define TUI_WIDGET_CONTEXT_MENU   WIDGET_CONTEXT_MENU
#define TUI_WIDGET_COUNT          WIDGET_COUNT

/* ============================================================
 * Opaque Type Definitions
 * ============================================================ */

typedef struct TuiManager TuiManager;
typedef struct TuiWindow TuiWindow;
typedef struct TuiTab TuiTab;
typedef struct TuiWorkspace TuiWorkspace;
typedef struct TuiLayout TuiLayout;

#endif /* CORE_TYPES_H */
