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

/**
 * @brief Register a widget with this window for mouse event routing.
 *        The widget type is determined by the registered type_id.
 * @param win Window instance. Must not be NULL.
 * @param widget Pointer to the widget instance. Must not be NULL.
 * @param plane Widget's ncplane (for hit testing). Must not be NULL.
 * @param type_id Registered widget type_id (from tui_*_get_type_id()).
 */
void tui_window_add_widget(TuiWindow* win, void* widget, struct ncplane* plane, int type_id);

/**
 * @brief Find the widget at the given local coordinates.
 * @param win Window instance. Must not be NULL.
 * @param local_y Y coordinate relative to the window plane.
 * @param local_x X coordinate relative to the window plane.
 * @param out_type Optional output for the registered widget type_id. May be NULL.
 * @return Pointer to the widget, or NULL if no widget at this position.
 */
void* tui_window_get_widget_at(TuiWindow* win, int local_y, int local_x, int* out_type);

/**
 * @brief Get the focused widget index within the window.
 * @param win Window instance, or NULL.
 * @return Index of focused widget, or -1 if none.
 */
int tui_window_get_focused_widget_index(const TuiWindow* win);

/**
 * @brief Set the focused widget index within the window.
 * @param win Window instance, or NULL.
 * @param index New focused widget index.
 */
void tui_window_set_focused_widget_index(TuiWindow* win, int index);

/**
 * @brief Mark the window for redraw on the next render cycle.
 * @param win Window instance, or NULL.
 */
void tui_window_mark_dirty(TuiWindow* win);

/* Opaque type getters */

/**
 * @brief Get the underlying notcurses plane.
 * @param win Window instance, or NULL.
 * @return The ncplane pointer, or NULL if win is NULL.
 */
struct ncplane* tui_window_get_plane(const TuiWindow* win);

/**
 * @brief Get the opaque user data pointer.
 * @param win Window instance, or NULL.
 * @return User data pointer, or NULL if win is NULL.
 */
void* tui_window_get_user_data(const TuiWindow* win);

/**
 * @brief Set the opaque user data pointer.
 * @param win Window instance, or NULL.
 * @param data Opaque pointer to store.
 */
void tui_window_set_user_data(TuiWindow* win, void* data);

#endif
