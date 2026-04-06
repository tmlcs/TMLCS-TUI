#ifndef CORE_TAB_H
#define CORE_TAB_H

#include "core/types.h"

/**
 * @brief Create a new tab within the workspace.
 * @param ws Parent workspace. Must not be NULL.
 * @param name Tab display name. Must not be NULL.
 * @return New tab instance, or NULL on failure.
 */
TuiTab* tui_tab_create(struct TuiWorkspace* ws, const char* name);

/**
 * @brief Destroy the tab and all owned windows.
 * @param tab Tab instance, or NULL.
 */
void tui_tab_destroy(TuiTab* tab);

/**
 * @brief Add a window to the tab. The tab takes ownership of the window.
 * @param tab Tab instance.
 * @param win Window to add. Must not be NULL.
 */
void tui_tab_add_window(TuiTab* tab, struct TuiWindow* win);

/**
 * @brief Remove a window from the tab without destroying it.
 * @param tab Tab instance.
 * @param win Window to remove. Must not be NULL.
 */
void tui_tab_remove_window(TuiTab* tab, struct TuiWindow* win);

/**
 * @brief Remove and destroy the active window in the tab.
 * @param tab Tab instance.
 */
void tui_tab_remove_active_window(TuiTab* tab);

/**
 * @brief Find the window at the given absolute screen coordinates.
 * @param tab Tab instance.
 * @param abs_y Absolute screen Y coordinate.
 * @param abs_x Absolute screen X coordinate.
 * @param wly[out] Window-local Y coordinate, or NULL.
 * @param wlx[out] Window-local X coordinate, or NULL.
 * @return Window pointer, or NULL if no window at position.
 */
TuiWindow* tui_tab_get_window_at(TuiTab* tab, int abs_y, int abs_x, int* wly, int* wlx);

/**
 * @brief Get the tab name.
 * @param tab Tab instance, or NULL.
 * @return Name string, or NULL if tab is NULL.
 */
const char* tui_tab_get_name(const TuiTab* tab);

/**
 * @brief Get the tab ID.
 * @param tab Tab instance, or NULL.
 * @return Tab ID, or -1 if tab is NULL.
 */
int tui_tab_get_id(const TuiTab* tab);

/**
 * @brief Get the number of windows in the tab.
 * @param tab Tab instance, or NULL.
 * @return Window count, or 0 if tab is NULL.
 */
int tui_tab_get_window_count(const TuiTab* tab);

#endif
