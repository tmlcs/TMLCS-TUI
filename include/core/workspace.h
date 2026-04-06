#ifndef CORE_WORKSPACE_H
#define CORE_WORKSPACE_H

#include "core/types.h"

/**
 * @brief Create a new workspace within the manager.
 * @param manager Parent manager. Must not be NULL.
 * @param name Workspace display name. Must not be NULL.
 * @return New workspace instance, or NULL on failure.
 */
TuiWorkspace* tui_workspace_create(TuiManager* manager, const char* name);

/**
 * @brief Destroy the workspace and all owned tabs.
 * @param ws Workspace instance, or NULL.
 */
void tui_workspace_destroy(TuiWorkspace* ws);

/**
 * @brief Add a tab to the workspace.
 * @param ws Workspace instance.
 * @param tab Tab to add. Must not be NULL.
 */
void tui_workspace_add_tab(TuiWorkspace* ws, TuiTab* tab);

/**
 * @brief Set the active tab by index. Inactive tabs are moved off-screen.
 * @param ws Workspace instance.
 * @param index Zero-based tab index. Out-of-bounds is a no-op.
 */
void tui_workspace_set_active_tab(TuiWorkspace* ws, int index);

/**
 * @brief Remove the active tab. Focus shifts to the last remaining tab.
 * @param ws Workspace instance.
 */
void tui_workspace_remove_active_tab(TuiWorkspace* ws);

/**
 * @brief Get the workspace name.
 * @param ws Workspace instance, or NULL.
 * @return Name string, or NULL if ws is NULL.
 */
const char* tui_workspace_get_name(const TuiWorkspace* ws);

/**
 * @brief Get the workspace ID.
 * @param ws Workspace instance, or NULL.
 * @return Workspace ID, or -1 if ws is NULL.
 */
int tui_workspace_get_id(const TuiWorkspace* ws);

/**
 * @brief Get the number of tabs in the workspace.
 * @param ws Workspace instance, or NULL.
 * @return Tab count, or 0 if ws is NULL.
 */
int tui_workspace_get_tab_count(const TuiWorkspace* ws);

#endif
