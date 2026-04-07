/**
 * @file loop.h
 * @brief Main event loop API for tmlcs-tui.
 *
 * Provides a configurable event loop with callbacks for per-frame logic,
 * post-render hooks, unhandled key dispatch, and exit confirmation.
 */

#ifndef CORE_LOOP_H
#define CORE_LOOP_H

#include "core/types.h"
#include <stdbool.h>

/**
 * @brief Configuration for the main event loop.
 *
 * All callbacks are optional (NULL = skip). The userdata pointer is passed
 * through to every callback invocation.
 */
typedef struct TuiLoopConfig {
    /**
     * Called before each render frame. Return false to skip this frame
     * (useful for rate-limiting expensive pre-frame work).
     */
    bool (*on_frame)(TuiManager* mgr, void* userdata);
    /** Called after render completes. */
    void (*after_render)(TuiManager* mgr, void* userdata);
    /**
     * Called when a keyboard event is not handled by the manager's
     * built-in keymap or any widget.
     */
    void (*on_unhandled_key)(TuiManager* mgr, uint32_t key, const struct ncinput* ni, void* userdata);
    /**
     * Called when the quit action is triggered. Return false to cancel quitting.
     */
    bool (*on_exit_request)(TuiManager* mgr, void* userdata);
    /** Target frames per second (0 = default ~60 fps). */
    int target_fps;
    /** Opaque pointer passed to all callbacks. */
    void* userdata;
} TuiLoopConfig;

/**
 * @brief Run the main event loop.
 * @param mgr Manager instance. Must not be NULL.
 * @param config Loop configuration, or NULL for defaults (no callbacks, ~60 fps).
 * @return 0 on normal exit, -1 on error.
 */
int tui_manager_run(TuiManager* mgr, const TuiLoopConfig* config);

/**
 * @brief Signal the event loop to stop.
 * @param mgr Manager instance, or NULL.
 */
void tui_manager_quit(TuiManager* mgr);

#endif /* CORE_LOOP_H */
