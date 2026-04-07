#ifndef CORE_UTF8_H
#define CORE_UTF8_H

#include <stdint.h>

/**
 * @file utf8.h
 * @brief UTF-8 utility functions for codepoint encoding/decoding and manipulation.
 */

/* Encode a Unicode codepoint as UTF-8. Returns byte length (1-4). */
int utf8_encode(uint32_t cp, char* out);

/* Decode one UTF-8 codepoint from *in. Advances *in past the character. */
uint32_t utf8_decode(const char** in);

/* Count codepoints in a UTF-8 string of given byte length. */
int utf8_codepoint_count(const char* str, int byte_len);

/* Convert codepoint index to byte offset within str. */
int utf8_cp_to_byte_offset(const char* str, int cp_index);

/* Convert byte offset to codepoint index. */
int utf8_byte_to_cp_offset(const char* str, int byte_offset);

/* Get display column (visual width) for codepoint at given index.
 * Uses ncstrwidth if available, otherwise estimates. */
int utf8_cp_display_width(const char* str, int cp_index);

/* Total display width of string from cp_start to cp_end. */
int utf8_display_width(const char* str, int cp_start, int cp_end);

/* Get display width of a single codepoint (used by widgets for width heuristics). */
int utf8_cp_width(uint32_t cp);

/* Extract substring from cp_start to cp_end (codepoint indices).
 * Caller must free(). */
char* utf8_substring(const char* str, int cp_start, int cp_end);

/* Delete codepoints from cp_start to cp_end in-place.
 * Updates *byte_len. Returns new byte_len. */
int utf8_delete_range(char* buffer, int byte_len, int cp_start, int cp_end);

/* Insert src UTF-8 string at cp_index in buffer.
 * max_bytes is total buffer size. Returns new byte_len, or -1 if overflow. */
int utf8_insert(char* buffer, int byte_len, int max_bytes, int cp_index, const char* src);

#endif /* CORE_UTF8_H */
