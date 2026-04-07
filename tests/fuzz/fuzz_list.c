/**
 * @file fuzz_list.c
 * @brief Fuzz test harness for TuiList widget.
 *
 * Exercises the list widget with arbitrary byte sequences
 * to find crashes, buffer overflows, and assertion violations.
 */
#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include "widget/list.h"

/* Provided by nc_stubs.c */
extern struct ncplane* stub_get_dummy_plane(void);

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    TuiList* list = tui_list_create(stub_get_dummy_plane(), 0, 0, 40, 10);
    if (!list) return 0;

    /* Seed some initial items from the fuzz data */
    char item_buf[256];
    size_t pos = 0;
    while (pos + 4 < size && list->count < 100) {
        size_t item_len = data[pos] % 32 + 1;
        if (pos + 1 + item_len > size) break;
        memcpy(item_buf, (const char*)data + pos + 1, item_len);
        item_buf[item_len] = '\0';
        tui_list_add_item(list, item_buf);
        pos += 1 + item_len;
    }

    struct ncinput ni;
    for (size_t i = 0; i < size - pos && i < 256; i++) {
        uint32_t key = data[pos + i];
        ni.ctrl = false;
        ni.shift = false;
        ni.alt = false;
        ni.evtype = 0;
        ni.y = 0;
        ni.x = 0;

        if (key == 0x01) key = 0x1000D;  /* NCKEY_UP */
        else if (key == 0x02) key = 0x1000E;  /* NCKEY_DOWN */
        else if (key == 0x03) key = 0x10018;  /* NCKEY_ENTER */
        else if (key == 0x04) key = 0x1B;  /* NCKEY_ESCAPE */
        else if (key == 0x05) key = 0x09;  /* NCKEY_TAB */

        tui_list_handle_key(list, key, &ni);
        tui_list_render(list);

        /* Invariants */
        assert(list->selected >= -1);
        assert(list->selected < list->capacity);
        assert(list->count >= 0);
        assert(list->count <= list->capacity);
        assert(list->scroll_off >= 0);
    }

    tui_list_destroy(list);
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
