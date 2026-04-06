#ifndef WIDGET_LABEL_H
#define WIDGET_LABEL_H

#include <notcurses/notcurses.h>

/**
 * @brief A non-editable text label widget.
 */
typedef struct TuiLabel {
    struct ncplane* plane;
    char* text;
    unsigned fg_color;
} TuiLabel;

/**
 * @brief Create a text label.
 * @param parent Parent ncplane.
 * @param y Y position within parent.
 * @param x X position within parent.
 * @param text Label text. Copied internally.
 * @return New label, or NULL on failure.
 */
TuiLabel* tui_label_create(struct ncplane* parent, int y, int x, const char* text);

/**
 * @brief Destroy the label.
 * @param lbl Label, or NULL.
 */
void tui_label_destroy(TuiLabel* lbl);

/**
 * @brief Change the label text.
 * @param lbl Label.
 * @param text New text. Copied internally.
 */
void tui_label_set_text(TuiLabel* lbl, const char* text);

/**
 * @brief Render the label.
 * @param lbl Label, or NULL.
 */
void tui_label_render(TuiLabel* lbl);

/**
 * @brief Get the label text.
 * @param lbl Label, or NULL.
 * @return Text string, or NULL.
 */
const char* tui_label_get_text(const TuiLabel* lbl);

#endif /* WIDGET_LABEL_H */
