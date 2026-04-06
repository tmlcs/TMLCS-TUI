#ifndef CORE_WINDOW_H
#define CORE_WINDOW_H

#include "core/types.h"

/**
 * @brief Create a new window within the tab.
 * @param tab Parent tab. Must not be NULL.
 * @param width Window width in columns.
 * @param height Window height in rows.
 * @return New window instance, or NULL on failure.
 */
TuiWindow* tui_window_create(TuiTab* tab, int width, int height);

/**
 * @brief Destroy the window. Calls on_destroy callback before plane destruction.
 * @param win Window instance, or NULL.
 */
void tui_window_destroy(TuiWindow* win);

/**
 * @brief Get the window ID.
 * @param win Window instance, or NULL.
 * @return Window ID, or -1 if win is NULL.
 */
int tui_window_get_id(const TuiWindow* win);

/**
 * @brief Check if the window has focus.
 * @param win Window instance, or NULL.
 * @return true if focused, false otherwise.
 */
bool tui_window_is_focused(const TuiWindow* win);

#endif
