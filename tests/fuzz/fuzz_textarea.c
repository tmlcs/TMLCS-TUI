/**
 * @file fuzz_textarea.c
 * @brief Fuzz test harness for TuiTextArea widget.
 *
 * Exercises the text area widget with arbitrary byte sequences
 * to find crashes, buffer overflows, and assertion violations.
 */
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "widget/textarea.h"

/* Provided by nc_stubs.c */
extern struct ncplane* stub_get_dummy_plane(void);

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    TuiTextArea* ta = tui_textarea_create(stub_get_dummy_plane(), 0, 0, 40, 10);
    if (!ta) return 0;

    struct ncinput ni;
    for (size_t i = 0; i < size; i++) {
        uint32_t key = data[i];
        ni.ctrl = (i % 5 == 0);
        ni.shift = (i % 7 == 0);
        ni.alt = false;
        ni.evtype = 0;
        ni.y = 0;
        ni.x = 0;

        if (key == 0x01) key = 0x1000B;  /* NCKEY_LEFT */
        else if (key == 0x02) key = 0x1000C;  /* NCKEY_RIGHT */
        else if (key == 0x03) key = 0x08;  /* NCKEY_BACKSPACE */
        else if (key == 0x04) key = 0x7F;  /* NCKEY_DELETE */
        else if (key == 0x05) key = 0x1000D;  /* NCKEY_UP */
        else if (key == 0x06) key = 0x1000E;  /* NCKEY_DOWN */
        else if (key == 0x07) key = 0x09;  /* NCKEY_TAB */
        else if (key == 0x08) key = 0x10018;  /* NCKEY_ENTER */
        else if (key == 0x09) key = 0x1B;  /* NCKEY_ESCAPE */

        tui_textarea_handle_key(ta, key, &ni);
        tui_textarea_render(ta);

        /* Invariants */
        assert(ta->cursor_line >= 0);
        assert(ta->cursor_line < TEXTAREA_MAX_LINES);
        assert(ta->cursor_col >= 0);
        assert(ta->line_count >= 0);
        assert(ta->line_count <= TEXTAREA_MAX_LINES);
        if (ta->cursor_line < ta->line_count) {
            assert(ta->cursor_col <= ta->line_len_codepoints[ta->cursor_line]);
            assert(ta->lines[ta->cursor_line][ta->line_len_bytes[ta->cursor_line]] == '\0');
        }
    }

    tui_textarea_destroy(ta);
    return 0;
}

/* Simple standalone runner: reads from stdin for testing without libFuzzer */
#ifndef LIBFUZZER
#include <stdio.h>
int main(void) {
    uint8_t buf[65536];
    size_t n = fread(buf, 1, sizeof(buf), stdin);
    if (n == 0) return 0;
    return LLVMFuzzerTestOneInput(buf, n);
}
#endif
