#include "core/clipboard.h"
#include <stdio.h>
#include <string.h>

static char s_clipboard_buf[CLIPBOARD_MAX_LEN];
static int s_clipboard_len = 0;

bool tui_clipboard_copy(const char* text) {
    if (!text) return false;
    int len = (int)strlen(text);
    if (len >= CLIPBOARD_MAX_LEN) len = CLIPBOARD_MAX_LEN - 1;

    memcpy(s_clipboard_buf, text, (size_t)len);
    s_clipboard_buf[len] = '\0';
    s_clipboard_len = len;

    /* OSC 52: send to terminal via Base64 */
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char b64_out[CLIPBOARD_MAX_LEN * 2];
    int out_idx = 0;
    for (int i = 0; i < len; i += 3) {
        unsigned char b0 = (unsigned char)s_clipboard_buf[i];
        unsigned char b1 = (i + 1 < len) ? (unsigned char)s_clipboard_buf[i + 1] : 0;
        unsigned char b2 = (i + 2 < len) ? (unsigned char)s_clipboard_buf[i + 2] : 0;

        b64_out[out_idx++] = b64[b0 >> 2];
        b64_out[out_idx++] = b64[((b0 & 0x03) << 4) | (b1 >> 4)];
        b64_out[out_idx++] = (i + 1 < len) ? b64[((b1 & 0x0F) << 2) | (b2 >> 6)] : '=';
        b64_out[out_idx++] = (i + 2 < len) ? b64[b2 & 0x3F] : '=';
    }
    b64_out[out_idx] = '\0';

    printf("\033]52;c;%s\a", b64_out);
    fflush(stdout);
    return true;
}

int tui_clipboard_paste(char* buf, int max_buf) {
    if (!buf || max_buf <= 0 || s_clipboard_len <= 0) return -1;
    int copy = s_clipboard_len < max_buf - 1 ? s_clipboard_len : max_buf - 1;
    memcpy(buf, s_clipboard_buf, (size_t)copy);
    buf[copy] = '\0';
    return copy;
}

const char* tui_clipboard_get(void) {
    return s_clipboard_len > 0 ? s_clipboard_buf : NULL;
}

void tui_clipboard_clear(void) {
    s_clipboard_buf[0] = '\0';
    s_clipboard_len = 0;
}

bool tui_clipboard_has_content(void) {
    return s_clipboard_len > 0;
}
