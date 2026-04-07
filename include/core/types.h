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

/** @ingroup core_types Maximum number of workspaces. */
#define MAX_WORKSPACES 9
/** @ingroup core_types Maximum workspace name length. */
#define MAX_WORKSPACE_NAME 64

/* ============================================================
 * Opaque Type Definitions
 * ============================================================ */

typedef struct TuiManager TuiManager;
typedef struct TuiWindow TuiWindow;
typedef struct TuiTab TuiTab;
typedef struct TuiWorkspace TuiWorkspace;
typedef struct TuiLayout TuiLayout;

#endif /* CORE_TYPES_H */
