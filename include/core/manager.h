#ifndef CORE_MANAGER_H
#define CORE_MANAGER_H

#include "core/types.h"

/**
 * @brief Create a new TUI manager.
 * @param nc Initialized notcurses instance.
 * @return New manager instance, or NULL on failure.
 */
TuiManager* tui_manager_create(struct notcurses* nc);

/**
 * @brief Destroy the manager and all owned workspaces.
 * @param manager Manager instance, or NULL.
 */
void tui_manager_destroy(TuiManager* manager);

/**
 * @brief Add a workspace to the manager.
 * @param manager Manager instance.
 * @param ws Workspace to add. Must not be NULL.
 */
void tui_manager_add_workspace(TuiManager* manager, TuiWorkspace* ws);

/**
 * @brief Set the active workspace by index.
 * @param manager Manager instance.
 * @param index Zero-based workspace index. Out-of-bounds is a no-op.
 */
void tui_manager_set_active_workspace(TuiManager* manager, int index);

/**
 * @brief Remove the active workspace. Focus shifts to the last remaining.
 * @param manager Manager instance, or NULL.
 */
void tui_manager_remove_active_workspace(TuiManager* manager);

/**
 * @brief Find which tab is at the given screen position.
 * @param mgr Manager instance.
 * @param y Screen Y coordinate.
 * @param x Screen X coordinate.
 * @return Tab index, or -1 if no tab at position.
 */
int tui_manager_get_tab_at(TuiManager* mgr, int y, int x);

/**
 * @brief Process a mouse event.
 * @param mgr Manager instance.
 * @param key Notcurses key code (BUTTON1, MOTION, etc.).
 * @param ni Mouse input details.
 * @return true if event was consumed.
 */
bool tui_manager_process_mouse(TuiManager* mgr, uint32_t key, const struct ncinput* ni);

/**
 * @brief Process a keyboard event.
 * @param mgr Manager instance.
 * @param key Notcurses key code.
 * @param ni Keyboard input details.
 * @return true if event was consumed.
 */
bool tui_manager_process_keyboard(TuiManager* mgr, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the full UI tree.
 * @param manager Manager instance.
 */
void tui_manager_render(TuiManager* manager);

/**
 * @brief Apply a built-in theme by name.
 * @param mgr Manager instance.
 * @param name "dracula", "solarized", or "highcontrast".
 * @return true if the theme was applied.
 */
bool tui_manager_set_theme(TuiManager* mgr, const char* name);

/**
 * @brief Update hover position (called from input.c).
 * @param y Mouse Y coordinate.
 * @param x Mouse X coordinate.
 */
void tui_manager_update_hover(int y, int x);

/* ID management */

/**
 * @brief Get the next available entity ID.
 * @return Monotonically increasing ID value.
 */
int tui_next_id(void);

/**
 * @brief Reset the ID counter (useful for testing).
 * @param value New counter value.
 */
void tui_reset_id(int value);

#endif
