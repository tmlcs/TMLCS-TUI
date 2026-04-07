#include "core/utf8.h"
#include <stdlib.h>
#include <string.h>

int utf8_encode(uint32_t cp, char* out) {
    if (cp < 0x80) {
        out[0] = (char)cp;
        return 1;
    } else if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
}

uint32_t utf8_decode(const char** in) {
    const unsigned char* s = (const unsigned char*)*in;
    uint32_t cp;

    if (s[0] < 0x80) {
        cp = s[0];
        (*in)++;
    } else if ((s[0] & 0xE0) == 0xC0) {
        /* 2-byte sequence — check continuation byte */
        if (s[1] == '\0' || (s[1] & 0xC0) != 0x80) { (*in)++; return 0xFFFD; }
        cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
        /* Reject overlong encoding */
        if (cp < 0x80) { (*in) += 2; return 0xFFFD; }
        (*in) += 2;
    } else if ((s[0] & 0xF0) == 0xE0) {
        /* 3-byte sequence — check continuation bytes */
        if (s[1] == '\0' || (s[1] & 0xC0) != 0x80 || s[2] == '\0' || (s[2] & 0xC0) != 0x80) { (*in)++; return 0xFFFD; }
        cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
        /* Reject overlong encoding and surrogates */
        if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) { (*in) += 3; return 0xFFFD; }
        (*in) += 3;
    } else {
        /* 4-byte sequence — check continuation bytes */
        if (s[1] == '\0' || (s[1] & 0xC0) != 0x80 || s[2] == '\0' || (s[2] & 0xC0) != 0x80 || s[3] == '\0' || (s[3] & 0xC0) != 0x80) { (*in)++; return 0xFFFD; }
        cp = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
        /* Reject overlong or out-of-range */
        if (cp < 0x10000 || cp > 0x10FFFF) { (*in) += 4; return 0xFFFD; }
        (*in) += 4;
    }
    return cp;
}

int utf8_codepoint_count(const char* str, int byte_len) {
    int count = 0;
    const char* p = str;
    const char* end = str + byte_len;
    while (p < end) {
        utf8_decode(&p);
        count++;
    }
    return count;
}

int utf8_cp_to_byte_offset(const char* str, int cp_index) {
    const char* p = str;
    for (int i = 0; i < cp_index; i++) {
        if (*p == '\0') break;
        utf8_decode(&p);
    }
    return (int)(p - str);
}

int utf8_byte_to_cp_offset(const char* str, int byte_offset) {
    const char* p = str;
    const char* end = str + byte_offset;
    int cp = 0;
    while (p < end) {
        utf8_decode(&p);
        cp++;
    }
    return cp;
}

int utf8_cp_display_width(const char* str, int cp_index) {
    const char* p = str;
    uint32_t cp = 0;
    for (int i = 0; i <= cp_index; i++) {
        if (*p == '\0') break;
        cp = utf8_decode(&p);
    }
    return utf8_cp_width(cp);
}

/** Check if a codepoint is a wide character (CJK, emoji, etc.) */
int utf8_cp_width(uint32_t cp) {
    if (cp >= 0x1100 && cp <= 0x115F) return 2;  /* Hangul Jamo */
    if (cp >= 0x2E80 && cp <= 0x9FFF) return 2;  /* CJK */
    if (cp >= 0xAC00 && cp <= 0xD7AF) return 2;  /* Hangul Syllables */
    if (cp >= 0xF900 && cp <= 0xFAFF) return 2;  /* CJK Compatibility Ideographs */
    if (cp >= 0x1F300 && cp <= 0x1F9FF) return 2; /* Emoji */
    return 1;
}

int utf8_display_width(const char* str, int cp_start, int cp_end) {
    int width = 0;
    for (int i = cp_start; i < cp_end; i++) {
        width += utf8_cp_display_width(str, i);
    }
    return width;
}

char* utf8_substring(const char* str, int cp_start, int cp_end) {
    int start_byte = utf8_cp_to_byte_offset(str, cp_start);
    int end_byte = utf8_cp_to_byte_offset(str, cp_end);
    int len = end_byte - start_byte;
    char* result = (char*)malloc((size_t)len + 1);
    if (result) {
        memcpy(result, str + start_byte, (size_t)len);
        result[len] = '\0';
    }
    return result;
}

int utf8_delete_range(char* buffer, int byte_len, int cp_start, int cp_end) {
    int start_byte = utf8_cp_to_byte_offset(buffer, cp_start);
    int end_byte = utf8_cp_to_byte_offset(buffer, cp_end);
    int to_delete = end_byte - start_byte;
    int remaining = byte_len - end_byte;
    memmove(buffer + start_byte, buffer + end_byte, (size_t)remaining + 1);
    return byte_len - to_delete;
}

int utf8_insert(char* buffer, int byte_len, int max_bytes, int cp_index, const char* src) {
    int insert_byte = utf8_cp_to_byte_offset(buffer, cp_index);
    int src_len = (int)strlen(src);
    if (byte_len + src_len >= max_bytes) return -1;
    int remaining = byte_len - insert_byte;
    memmove(buffer + insert_byte + src_len, buffer + insert_byte, (size_t)remaining + 1);
    memcpy(buffer + insert_byte, src, (size_t)src_len);
    return byte_len + src_len;
}
