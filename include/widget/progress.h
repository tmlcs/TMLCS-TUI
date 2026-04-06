#ifndef WIDGET_PROGRESS_H
#define WIDGET_PROGRESS_H

#include <notcurses/notcurses.h>

/**
 * @brief A progress bar widget showing 0.0 - 1.0 fill level.
 */
typedef struct TuiProgressBar {
    struct ncplane* plane;
    int width;
    float value;
    unsigned fill_color;
    unsigned empty_color;
    unsigned text_color;
} TuiProgressBar;

/**
 * @brief Create a progress bar.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Bar width in columns.
 * @return New progress bar, or NULL on failure.
 */
TuiProgressBar* tui_progress_create(struct ncplane* parent, int y, int x, int width);

/**
 * @brief Destroy the progress bar.
 * @param pb Progress bar, or NULL.
 */
void tui_progress_destroy(TuiProgressBar* pb);

/**
 * @brief Set the fill value.
 * @param pb Progress bar.
 * @param value 0.0 (empty) to 1.0 (full). Clamped.
 */
void tui_progress_set_value(TuiProgressBar* pb, float value);

/**
 * @brief Get the current fill value.
 * @param pb Progress bar.
 * @return Value 0.0-1.0.
 */
float tui_progress_get_value(const TuiProgressBar* pb);

/**
 * @brief Render the progress bar.
 * @param pb Progress bar, or NULL.
 */
void tui_progress_render(TuiProgressBar* pb);

#endif /* WIDGET_PROGRESS_H */
