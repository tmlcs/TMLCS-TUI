#ifndef CORE_DEBUG_H
#define CORE_DEBUG_H

#include "core/types.h"

/**
 * @brief Dump workspace and plane information to stderr.
 * @param mgr Manager instance.
 */
void tui_debug_dump_planes(TuiManager* mgr);

/**
 * @brief Dump widget information from a window to stderr.
 * @param win Window instance.
 */
void tui_debug_dump_widgets(const TuiWindow* win);

/**
 * @brief Dump dirty tracking flags to stderr.
 * @param mgr Manager instance.
 */
void tui_debug_dump_dirty_flags(const TuiManager* mgr);

/**
 * @brief Enable or disable render timing measurement.
 * @param enable true to enable timing, false to disable.
 */
void tui_debug_enable_timing(bool enable);

/**
 * @brief Get the last render time in microseconds.
 * @return Last render time in us, or 0 if timing is disabled.
 */
int tui_debug_get_last_render_time_us(void);

#endif /* CORE_DEBUG_H */
