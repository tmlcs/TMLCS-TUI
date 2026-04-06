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
    char               buffer[TEXT_INPUT_MAX_LEN * 4];  /* UTF-8: up to 4 bytes per codepoint */
    int                len_bytes;       /* Actual byte length of buffer */
    int                len_codepoints;  /* Number of Unicode codepoints */
    int                cursor_cp;       /* Cursor position in codepoints */
    int                scroll_off;      /* Scroll offset in codepoints */
    bool               focused;
    bool               selecting;
    int                select_start_cp;
    int                select_end_cp;

    TextInputSubmitCb  on_submit;
    void*              userdata;

    /* Colors */
    unsigned int bg_normal;
    unsigned int fg_normal;
    unsigned int bg_focused;
    unsigned int fg_cursor;

    int _type_id;
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
 * @return Number of codepoints in the buffer, or 0 if ti is NULL.
 */
int tui_text_input_get_len(const TuiTextInput* ti);

/**
 * @brief Check if the text input has keyboard focus.
 * @param ti Text input instance, or NULL.
 * @return true if focused, false otherwise.
 */
bool tui_text_input_is_focused(const TuiTextInput* ti);

/**
 * @brief Check if there is an active text selection.
 * @param ti Text input instance, or NULL.
 * @return true if selecting, false otherwise.
 */
bool tui_text_input_is_selecting(const TuiTextInput* ti);

/**
 * @brief Get the current selection range.
 * @param ti Text input instance, or NULL.
 * @param out_start Output for selection start (codepoint index).
 * @param out_end Output for selection end (codepoint index).
 * @return true if selection is active, false otherwise.
 */
bool tui_text_input_get_selection(const TuiTextInput* ti, int* out_start, int* out_end);

/* Clipboard integration */
bool tui_text_input_copy(TuiTextInput* ti);
bool tui_text_input_paste(TuiTextInput* ti);
bool tui_text_input_cut(TuiTextInput* ti);

/**
 * @brief Handle a mouse event on the text input.
 *        Click sets cursor position within the buffer.
 * @param ti Text input instance, or NULL.
 * @param key Notcurses key code (NCKEY_BUTTON1, etc.).
 * @param ni Input details (event type, coordinates).
 * @return true if the event was consumed.
 */
bool tui_text_input_handle_mouse(TuiTextInput* ti, uint32_t key, const struct ncinput* ni);

/**
 * @brief Set focus state and re-render.
 * @param ti Text input instance, or NULL.
 * @param focused Focus state.
 */
void tui_text_input_set_focused(TuiTextInput* ti, bool focused);

/**
 * @brief Ensure the text input VTable is registered with the widget system.
 *        Called automatically during widget creation.
 */
void tui_text_input_ensure_registered(void);

/**
 * @brief Get the registered type_id for the text input widget type.
 * @return Registered type_id (>= 0).
 */
int tui_text_input_get_type_id(void);

#endif // WIDGET_TEXT_INPUT_H
