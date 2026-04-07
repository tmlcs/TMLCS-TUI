/**
 * @file fuzz_text_input.c
 * @brief Fuzz test harness for TuiTextInput widget.
 *
 * Exercises the text input widget with arbitrary byte sequences
 * to find crashes, buffer overflows, and assertion violations.
 */
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "widget/text_input.h"

/* Provided by nc_stubs.c */
extern struct ncplane* stub_get_dummy_plane(void);

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    TuiTextInput* ti = tui_text_input_create(stub_get_dummy_plane(), 0, 0, 40, NULL, NULL);
    if (!ti) return 0;

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

        tui_text_input_handle_key(ti, key, &ni);
        tui_text_input_render(ti);

        /* Invariants */
        assert(ti->buffer[ti->len_bytes] == '\0');
        assert(ti->cursor_cp >= 0);
        assert(ti->cursor_cp <= ti->len_codepoints);
        assert(ti->len_bytes >= 0);
        assert(ti->len_codepoints >= 0);
    }

    tui_text_input_destroy(ti);
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
