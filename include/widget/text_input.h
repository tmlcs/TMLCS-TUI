#ifndef WIDGET_TEXT_INPUT_H
#define WIDGET_TEXT_INPUT_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

#define TEXT_INPUT_MAX_LEN 512

/**
 * @brief Callback invoked when Enter is pressed.
 * @param text Current buffer content.
 * @param userdata User data pointer from tui_text_input_create.
 */
typedef void (*TextInputSubmitCb)(const char* text, void* userdata);

typedef struct TuiTextInput {
    struct ncplane*    plane;
    char               buffer[TEXT_INPUT_MAX_LEN];
    int                cursor;
    int                len;
    int                scroll_off;
    bool               focused;

    TextInputSubmitCb  on_submit;
    void*              userdata;

    /* Colors */
    unsigned int bg_normal;
    unsigned int fg_normal;
    unsigned int bg_focused;
    unsigned int fg_cursor;
} TuiTextInput;

/**
 * @brief Create a text input widget.
 * @param parent Parent ncplane. Must not be NULL.
 * @param y Y position within the parent plane.
 * @param x X position within the parent plane.
 * @param width Widget width in columns.
 * @param on_submit Callback invoked when Enter is pressed. May be NULL.
 * @param userdata User data passed to the on_submit callback.
 * @return New text input instance, or NULL on failure.
 */
TuiTextInput* tui_text_input_create(struct ncplane* parent, int y, int x, int width,
                                     TextInputSubmitCb on_submit, void* userdata);

/**
 * @brief Destroy the text input widget and its ncplane.
 * @param ti Text input instance, or NULL.
 */
void tui_text_input_destroy(TuiTextInput* ti);

/**
 * @brief Process a key event for the text input.
 * @param ti Text input instance.
 * @param key Notcurses key code.
 * @param ni Input details (event type, modifiers).
 * @return true if the key was consumed by the widget.
 */
bool tui_text_input_handle_key(TuiTextInput* ti, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the text input widget.
 * @param ti Text input instance, or NULL.
 */
void tui_text_input_render(TuiTextInput* ti);

/**
 * @brief Clear the text buffer and reset cursor position.
 * @param ti Text input instance, or NULL.
 */
void tui_text_input_clear(TuiTextInput* ti);

/**
 * @brief Get the current text buffer content.
 * @param ti Text input instance, or NULL.
 * @return Pointer to the buffer string, or NULL if ti is NULL.
 */
const char* tui_text_input_get(TuiTextInput* ti);

/**
 * @brief Get the current cursor position within the buffer.
 * @param ti Text input instance, or NULL.
 * @return Cursor index (0-based), or -1 if ti is NULL.
 */
int tui_text_input_get_cursor(const TuiTextInput* ti);

/**
 * @brief Get the current text length.
 * @param ti Text input instance, or NULL.
 * @return Number of characters in the buffer, or 0 if ti is NULL.
 */
int tui_text_input_get_len(const TuiTextInput* ti);

/**
 * @brief Check if the text input has keyboard focus.
 * @param ti Text input instance, or NULL.
 * @return true if focused, false otherwise.
 */
bool tui_text_input_is_focused(const TuiTextInput* ti);

/* Clipboard integration */
bool tui_text_input_copy(TuiTextInput* ti);
bool tui_text_input_paste(TuiTextInput* ti);
bool tui_text_input_cut(TuiTextInput* ti);

#endif // WIDGET_TEXT_INPUT_H
