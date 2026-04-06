#ifndef CORE_THEME_LOADER_H
#define CORE_THEME_LOADER_H

#include "core/theme.h"
#include <stdbool.h>

/**
 * @brief Set a theme color at runtime.
 * @param color_name Theme constant name (e.g., "THEME_BG_DARKEST").
 * @param rgb Hex RGB value (e.g., 0x111116).
 * @return true if the color name is recognized and set.
 */
bool tui_theme_set_color(const char* color_name, unsigned rgb);

/**
 * @brief Get the current value of a theme color.
 * @param color_name Theme constant name.
 * @param[out] rgb Pointer to store the current RGB value.
 * @return true if the color name is recognized.
 */
bool tui_theme_get_color(const char* color_name, unsigned* rgb);

/**
 * @brief Load a theme from a simple key-value file.
 *
 * File format (one per line):
 *   THEME_BG_DARKEST 0x111116
 *   THEME_FG_DEFAULT 0xf8f8f2
 *   # comments start with #
 *
 * @param filepath Path to the theme file.
 * @return true if the file was loaded successfully.
 */
bool tui_theme_load_file(const char* filepath);

/**
 * @brief Apply a built-in theme by name.
 * @param name Theme name: "dracula", "solarized", "highcontrast".
 * @return true if the theme name is recognized.
 */
bool tui_theme_apply(const char* name);

#endif /* CORE_THEME_LOADER_H */
