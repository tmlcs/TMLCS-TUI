#ifndef CORE_HELP_H
#define CORE_HELP_H

#include "core/manager.h"

/**
 * @brief Show the help overlay on the given plane.
 * @param plane Plane to render the help on (usually stdplane or a dedicated overlay plane).
 */
void tui_help_show(struct ncplane* plane);

/**
 * @brief Hide the help overlay.
 */
void tui_help_hide(void);

/**
 * @brief Check if the help overlay is currently visible.
 * @return true if help is being shown.
 */
bool tui_help_is_visible(void);

#endif /* CORE_HELP_H */
