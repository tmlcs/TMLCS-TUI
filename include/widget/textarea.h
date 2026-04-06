#ifndef WIDGET_TEXTAREA_H
#define WIDGET_TEXTAREA_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

#define TEXTAREA_MAX_LINES 256
#define TEXTAREA_MAX_LINE_LEN 512

/**
 * @brief A multi-line text input widget.
 */
typedef struct TuiTextArea {
    struct ncplane* plane;
    char lines[TEXTAREA_MAX_LINES][TEXTAREA_MAX_LINE_LEN * 4];  /* UTF-8 */
    int line_len_bytes[TEXTAREA_MAX_LINES];    /* byte length per line */
    int line_len_codepoints[TEXTAREA_MAX_LINES]; /* codepoint count per line */
    int line_count;
    int cursor_line;
    int cursor_col;       /* cursor in codepoints within current line */
    int scroll_line;
    int scroll_col;       /* scroll offset in codepoints */
    int height;
    bool focused;
    bool selecting;
    int select_start_col;
    int select_end_col;
    unsigned bg_normal;
    unsigned bg_focused;
    unsigned fg_text;
    unsigned fg_cursor;
    int _type_id;
} TuiTextArea;

/**
 * @brief Create a multi-line text area.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns.
 * @param height Height in rows.
 * @return New text area, or NULL on failure.
 */
TuiTextArea* tui_textarea_create(struct ncplane* parent, int y, int x, int width, int height);

/**
 * @brief Destroy the text area.
 * @param ta Text area, or NULL.
 */
void tui_textarea_destroy(TuiTextArea* ta);

/**
 * @brief Handle a key event.
 * @param ta Text area.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_textarea_handle_key(TuiTextArea* ta, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the text area.
 * @param ta Text area, or NULL.
 */
void tui_textarea_render(TuiTextArea* ta);

/**
 * @brief Clear all text.
 * @param ta Text area, or NULL.
 */
void tui_textarea_clear(TuiTextArea* ta);

/**
 * @brief Get the full text content (joined by newlines).
 * @param ta Text area.
 * @param buf Caller-provided buffer. Must not be NULL.
 * @param max_len Size of buf in bytes.
 * @return Number of bytes written (excluding null terminator), or -1 if ta is NULL.
 */
int tui_textarea_get(TuiTextArea* ta, char* buf, int max_len);

/**
 * @brief Get the number of lines.
 * @param ta Text area.
 * @return Line count.
 */
int tui_textarea_get_line_count(const TuiTextArea* ta);

/**
 * @brief Set focus state.
 * @param ta Text area.
 * @param focused Focus state.
 */
void tui_textarea_set_focused(TuiTextArea* ta, bool focused);

#endif /* WIDGET_TEXTAREA_H */
