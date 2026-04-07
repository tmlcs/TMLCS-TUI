#ifndef WIDGET_BUTTON_H
#define WIDGET_BUTTON_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiButtonCb)(void* userdata);

/**
 * @brief A clickable button widget.
 */
typedef struct TuiButton {
    struct ncplane* plane;
    char* label;
    int width;
    bool focused;
    bool pressed;
    TuiButtonCb cb;
    void* userdata;
    unsigned bg_normal;
    unsigned bg_focused;
    unsigned bg_pressed;
    unsigned fg_text;
    int _type_id;
} TuiButton;

/**
 * @brief Create a button.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Button width in columns.
 * @param label Button text.
 * @param cb Callback on activation. May be NULL.
 * @param userdata Passed to callback.
 * @return New button, or NULL on failure.
 */
TuiButton* tui_button_create(struct ncplane* parent, int y, int x, int width,
                              const char* label, TuiButtonCb cb, void* userdata);

/**
 * @brief Destroy the button.
 * @param btn Button, or NULL.
 */
void tui_button_destroy(TuiButton* btn);

/**
 * @brief Handle a key event.
 * @param btn Button.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_button_handle_key(TuiButton* btn, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the button.
 * @param btn Button, or NULL.
 */
void tui_button_render(TuiButton* btn);

/**
 * @brief Change the button label.
 * @param btn Button.
 * @param label New label text.
 * @return true on success, false on OOM or invalid arguments.
 */
bool tui_button_set_label(TuiButton* btn, const char* label);

/**
 * @brief Check if the button is currently pressed.
 * @param btn Button.
 * @return true if pressed.
 */
bool tui_button_is_pressed(const TuiButton* btn);

/**
 * @brief Set focus state.
 * @param btn Button.
 * @param focused Focus state.
 */
void tui_button_set_focused(TuiButton* btn, bool focused);

/**
 * @brief Handle a mouse event.
 * @param btn Button.
 * @param key Notcurses key code (NCKEY_BUTTON1, etc.).
 * @param ni Input details (contains y, x coordinates).
 * @return true if the event was consumed.
 */
bool tui_button_handle_mouse(TuiButton* btn, uint32_t key, const struct ncinput* ni);

#endif /* WIDGET_BUTTON_H */
