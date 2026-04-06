#ifndef WIDGET_SPINNER_H
#define WIDGET_SPINNER_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

/**
 * @brief Spinner animation types.
 */
typedef enum {
    SPINNER_DOTS = 0,   /* Braille dots: ⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏ */
    SPINNER_LINE,       /* Line chars: |/-\ */
    SPINNER_ARROW       /* Arrows: ←↖↑↗→↘↓↙ */
} TuiSpinnerType;

/**
 * @brief An animated loading indicator.
 */
typedef struct TuiSpinner {
    struct ncplane* plane;
    TuiSpinnerType type;
    int frame;
    int frame_count;
    bool running;
    int _type_id;
} TuiSpinner;

/**
 * @brief Create a spinner widget.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param type Animation type.
 * @return New spinner, or NULL on failure.
 */
TuiSpinner* tui_spinner_create(struct ncplane* parent, int y, int x, TuiSpinnerType type);

/**
 * @brief Destroy the spinner.
 * @param spinner Spinner, or NULL.
 */
void tui_spinner_destroy(TuiSpinner* spinner);

/**
 * @brief Start the animation.
 * @param spinner Spinner.
 */
void tui_spinner_start(TuiSpinner* spinner);

/**
 * @brief Stop the animation.
 * @param spinner Spinner.
 */
void tui_spinner_stop(TuiSpinner* spinner);

/**
 * @brief Check if the spinner is running.
 * @param spinner Spinner.
 * @return true if running.
 */
bool tui_spinner_is_running(const TuiSpinner* spinner);

/**
 * @brief Advance to the next frame.
 * @param spinner Spinner.
 */
void tui_spinner_tick(TuiSpinner* spinner);

/**
 * @brief Render the spinner.
 * @param spinner Spinner, or NULL.
 */
void tui_spinner_render(TuiSpinner* spinner);

/**
 * @brief Handle a key event (always returns false).
 * @param spinner Spinner.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return false (spinner does not handle keys).
 */
bool tui_spinner_handle_key(TuiSpinner* spinner, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event (always returns false).
 * @param spinner Spinner.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return false (spinner does not handle mouse).
 */
bool tui_spinner_handle_mouse(TuiSpinner* spinner, uint32_t key, const struct ncinput* ni);

#endif /* WIDGET_SPINNER_H */
