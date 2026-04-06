#ifndef CORE_CLIPBOARD_H
#define CORE_CLIPBOARD_H

#include <stdbool.h>

#define CLIPBOARD_MAX_LEN 1024

/**
 * @brief Copy text to clipboard (OSC 52 + internal buffer).
 * @param text Text to copy.
 * @return true if successful.
 */
bool tui_clipboard_copy(const char* text);

/**
 * @brief Paste from clipboard internal buffer.
 * @param buf Output buffer.
 * @param max_buf Maximum buffer size.
 * @return Number of characters pasted, or -1 if empty.
 */
int tui_clipboard_paste(char* buf, int max_buf);

/**
 * @brief Get clipboard content (do NOT free).
 * @return Clipboard string, or NULL if empty.
 */
const char* tui_clipboard_get(void);

/**
 * @brief Clear clipboard content.
 */
void tui_clipboard_clear(void);

/**
 * @brief Check if clipboard has content.
 * @return true if clipboard is non-empty.
 */
bool tui_clipboard_has_content(void);

#endif /* CORE_CLIPBOARD_H */
