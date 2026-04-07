#include "widget/textarea.h"
#include "core/theme.h"
#include "core/widget.h"
#include "core/utf8.h"
#include "core/clipboard.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declarations */
void tui_textarea_ensure_registered(void);
int tui_textarea_get_type_id(void);

static int s_textarea_type_id = -1;

static void textarea_update_line_stats(TuiTextArea* ta, int line_idx) {
    ta->line_len_bytes[line_idx] = (int)strlen(ta->lines[line_idx]);
    ta->line_len_codepoints[line_idx] = utf8_codepoint_count(
        ta->lines[line_idx], ta->line_len_bytes[line_idx]);
}

TuiTextArea* tui_textarea_create(struct ncplane* parent, int y, int x, int width, int height) {
    if (!parent || height < 2 || width < 4) return NULL;
    TuiTextArea* ta = (TuiTextArea*)calloc(1, sizeof(TuiTextArea));
    if (!ta) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = (unsigned)height, .cols = (unsigned)width,
    };
    ta->plane = ncplane_create(parent, &opts);
    if (!ta->plane) { free(ta); return NULL; }

    ta->line_count = 1;
    ta->lines[0][0] = '\0';
    ta->line_len_bytes[0] = 0;
    ta->line_len_codepoints[0] = 0;
    ta->cursor_line = 0;
    ta->cursor_col = 0;
    ta->scroll_line = 0;
    ta->scroll_col = 0;
    ta->height = height;
    ta->focused = false;
    ta->selecting = false;
    ta->select_start_col = 0;
    ta->select_end_col = 0;
    ta->bg_normal = THEME_BG_TASKBAR;
    ta->bg_focused = THEME_BG_TEXT_FOC;
    ta->fg_text = THEME_FG_DEFAULT;
    ta->fg_cursor = THEME_FG_CURSOR;

    tui_textarea_ensure_registered();
    ta->_type_id = s_textarea_type_id;

    tui_textarea_render(ta);
    return ta;
}

void tui_textarea_destroy(TuiTextArea* ta) {
    if (!ta) return;
    if (ta->plane) ncplane_destroy(ta->plane);
    free(ta);
}

void tui_textarea_clear(TuiTextArea* ta) {
    if (!ta) return;
    ta->line_count = 1;
    ta->lines[0][0] = '\0';
    ta->line_len_bytes[0] = 0;
    ta->line_len_codepoints[0] = 0;
    ta->cursor_line = 0;
    ta->cursor_col = 0;
    ta->scroll_line = 0;
    ta->scroll_col = 0;
    ta->selecting = false;
    tui_textarea_render(ta);
}

int tui_textarea_get_line_count(const TuiTextArea* ta) {
    return ta ? ta->line_count : 0;
}

void tui_textarea_set_focused(TuiTextArea* ta, bool focused) {
    if (!ta) return;
    ta->focused = focused;
    tui_textarea_render(ta);
}

int tui_textarea_get(TuiTextArea* ta, char* buf, int max_len) {
    if (!ta || !buf || max_len <= 0) return -1;
    char* p = buf;
    size_t remaining = (size_t)max_len - 1;
    for (int i = 0; i < ta->line_count && remaining > 0; i++) {
        if (i > 0) { *p++ = '\n'; remaining--; }
        size_t line_len = (size_t)ta->line_len_bytes[i];
        if (line_len > remaining) line_len = remaining;
        memcpy(p, ta->lines[i], line_len);
        p += line_len;
        remaining -= line_len;
    }
    *p = '\0';
    return (int)(p - buf);
}

static void textarea_clear_selection(TuiTextArea* ta) {
    ta->selecting = false;
    ta->select_start_col = 0;
    ta->select_end_col = 0;
}

static void textarea_delete_selection(TuiTextArea* ta) {
    if (!ta->selecting) return;
    int start = ta->select_start_col < ta->select_end_col ? ta->select_start_col : ta->select_end_col;
    int end = ta->select_start_col < ta->select_end_col ? ta->select_end_col : ta->select_start_col;
    ta->line_len_bytes[ta->cursor_line] = utf8_delete_range(
        ta->lines[ta->cursor_line], ta->line_len_bytes[ta->cursor_line], start, end);
    ta->line_len_codepoints[ta->cursor_line] -= (end - start);
    ta->cursor_col = start;
    textarea_clear_selection(ta);
}

bool tui_textarea_handle_key(TuiTextArea* ta, uint32_t key, const struct ncinput* ni) {
    if (!ta || !ta->focused || ni->evtype == NCTYPE_RELEASE) return false;

    if (key == NCKEY_ENTER || key == '\n' || key == '\r') {
        if (ta->line_count < TEXTAREA_MAX_LINES - 1) {
            if (ta->selecting) textarea_delete_selection(ta);
            /* Shift lines down */
            for (int i = ta->line_count; i > ta->cursor_line; i--) {
                memcpy(ta->lines[i], ta->lines[i - 1], (size_t)ta->line_len_bytes[i - 1] + 1);
                ta->line_len_bytes[i] = ta->line_len_bytes[i - 1];
                ta->line_len_codepoints[i] = ta->line_len_codepoints[i - 1];
            }
            /* Split current line at cursor */
            int split_byte = utf8_cp_to_byte_offset(ta->lines[ta->cursor_line], ta->cursor_col);
            char* rest = ta->lines[ta->cursor_line] + split_byte;
            size_t rest_len = strlen(rest);
            size_t max_len = sizeof(ta->lines[0]) - 1;
            if (rest_len > max_len) rest_len = max_len;
            memcpy(ta->lines[ta->cursor_line + 1], rest, rest_len);
            ta->lines[ta->cursor_line + 1][rest_len] = '\0';
            ta->lines[ta->cursor_line][split_byte] = '\0';
            textarea_update_line_stats(ta, ta->cursor_line);
            textarea_update_line_stats(ta, ta->cursor_line + 1);
            ta->line_count++;
            ta->cursor_line++;
            ta->cursor_col = 0;
            textarea_clear_selection(ta);
            tui_textarea_render(ta);
        }
        return true;
    }

    /* Delete key */
    if (key == NCKEY_DEL) {
        if (ta->selecting) {
            textarea_delete_selection(ta);
        } else if (ta->cursor_col < ta->line_len_codepoints[ta->cursor_line]) {
            ta->line_len_bytes[ta->cursor_line] = utf8_delete_range(
                ta->lines[ta->cursor_line], ta->line_len_bytes[ta->cursor_line],
                ta->cursor_col, ta->cursor_col + 1);
            ta->line_len_codepoints[ta->cursor_line]--;
        } else if (ta->cursor_line < ta->line_count - 1) {
            /* Join with next line */
            int cur_len = ta->line_len_bytes[ta->cursor_line];
            int next_len = ta->line_len_bytes[ta->cursor_line + 1];
            if (cur_len + next_len < (int)sizeof(ta->lines[0]) - 1) {
                memcpy(ta->lines[ta->cursor_line] + cur_len,
                       ta->lines[ta->cursor_line + 1], (size_t)next_len + 1);
                ta->line_len_bytes[ta->cursor_line] += next_len;
                ta->line_len_codepoints[ta->cursor_line] += ta->line_len_codepoints[ta->cursor_line + 1];
            }
            for (int i = ta->cursor_line + 1; i < ta->line_count - 1; i++) {
                memcpy(ta->lines[i], ta->lines[i + 1], (size_t)ta->line_len_bytes[i + 1] + 1);
                ta->line_len_bytes[i] = ta->line_len_bytes[i + 1];
                ta->line_len_codepoints[i] = ta->line_len_codepoints[i + 1];
            }
            ta->line_count--;
        }
        tui_textarea_render(ta);
        return true;
    }

    /* Backspace */
    if (key == NCKEY_BACKSPACE || key == 127) {
        if (ta->selecting) {
            textarea_delete_selection(ta);
        } else if (ta->cursor_col > 0) {
            ta->line_len_bytes[ta->cursor_line] = utf8_delete_range(
                ta->lines[ta->cursor_line], ta->line_len_bytes[ta->cursor_line],
                ta->cursor_col - 1, ta->cursor_col);
            ta->line_len_codepoints[ta->cursor_line]--;
            ta->cursor_col--;
        } else if (ta->cursor_line > 0) {
            /* Join with previous line */
            int prev_len = ta->line_len_bytes[ta->cursor_line - 1];
            int prev_cp = ta->line_len_codepoints[ta->cursor_line - 1];
            int cur_len = ta->line_len_bytes[ta->cursor_line];
            if (prev_len + cur_len < (int)sizeof(ta->lines[0]) - 1) {
                memcpy(ta->lines[ta->cursor_line - 1] + prev_len,
                       ta->lines[ta->cursor_line], (size_t)cur_len + 1);
                ta->line_len_bytes[ta->cursor_line - 1] += cur_len;
                ta->line_len_codepoints[ta->cursor_line - 1] += ta->line_len_codepoints[ta->cursor_line];
            }
            for (int i = ta->cursor_line; i < ta->line_count - 1; i++) {
                memcpy(ta->lines[i], ta->lines[i + 1], (size_t)ta->line_len_bytes[i + 1] + 1);
                ta->line_len_bytes[i] = ta->line_len_bytes[i + 1];
                ta->line_len_codepoints[i] = ta->line_len_codepoints[i + 1];
            }
            ta->line_count--;
            ta->cursor_line--;
            ta->cursor_col = prev_cp;
        }
        tui_textarea_render(ta);
        return true;
    }

    if (key == NCKEY_UP) {
        textarea_clear_selection(ta);
        if (ta->cursor_line > 0) {
            ta->cursor_line--;
            int line_cp = ta->line_len_codepoints[ta->cursor_line];
            if (ta->cursor_col > line_cp) ta->cursor_col = line_cp;
            tui_textarea_render(ta);
        }
        return true;
    }
    if (key == NCKEY_DOWN) {
        textarea_clear_selection(ta);
        if (ta->cursor_line < ta->line_count - 1) {
            ta->cursor_line++;
            int line_cp = ta->line_len_codepoints[ta->cursor_line];
            if (ta->cursor_col > line_cp) ta->cursor_col = line_cp;
            tui_textarea_render(ta);
        }
        return true;
    }

    /* Shift+Left: extend selection */
    if (key == NCKEY_LEFT && ni->shift) {
        if (!ta->selecting) {
            ta->selecting = true;
            ta->select_start_col = ta->cursor_col;
        }
        if (ta->cursor_col > 0) ta->cursor_col--;
        ta->select_end_col = ta->cursor_col;
        tui_textarea_render(ta);
        return true;
    }

    /* Shift+Right: extend selection */
    if (key == NCKEY_RIGHT && ni->shift) {
        if (!ta->selecting) {
            ta->selecting = true;
            ta->select_start_col = ta->cursor_col;
        }
        int line_cp = ta->line_len_codepoints[ta->cursor_line];
        if (ta->cursor_col < line_cp) ta->cursor_col++;
        ta->select_end_col = ta->cursor_col;
        tui_textarea_render(ta);
        return true;
    }

    if (key == NCKEY_LEFT) {
        textarea_clear_selection(ta);
        if (ta->cursor_col > 0) {
            ta->cursor_col--;
            tui_textarea_render(ta);
        }
        return true;
    }
    if (key == NCKEY_RIGHT) {
        textarea_clear_selection(ta);
        int line_cp = ta->line_len_codepoints[ta->cursor_line];
        if (ta->cursor_col < line_cp) {
            ta->cursor_col++;
            tui_textarea_render(ta);
        }
        return true;
    }

    /* Ctrl+A: select all (current line) */
    if (ni->ctrl && (key == 'a' || key == 'A')) {
        ta->selecting = true;
        ta->select_start_col = 0;
        ta->select_end_col = ta->line_len_codepoints[ta->cursor_line];
        ta->cursor_col = ta->line_len_codepoints[ta->cursor_line];
        tui_textarea_render(ta);
        return true;
    }

    /* Ctrl+C: copy */
    if (ni->ctrl && (key == 'c' || key == 'C')) {
        if (ta->selecting) {
            int start = ta->select_start_col < ta->select_end_col ? ta->select_start_col : ta->select_end_col;
            int end = ta->select_start_col < ta->select_end_col ? ta->select_end_col : ta->select_start_col;
            char* selected = utf8_substring(ta->lines[ta->cursor_line], start, end);
            tui_clipboard_copy(selected);
            free(selected);
        } else {
            tui_clipboard_copy(ta->lines[ta->cursor_line]);
        }
        return true;
    }

    /* Ctrl+V: paste */
    if (ni->ctrl && (key == 'v' || key == 'V')) {
        const char* clip = tui_clipboard_get();
        if (clip && clip[0]) {
            if (ta->selecting) textarea_delete_selection(ta);
            int clip_cp = utf8_codepoint_count(clip, (int)strlen(clip));
            int line_cp = ta->line_len_codepoints[ta->cursor_line];
            if (line_cp + clip_cp < TEXTAREA_MAX_LINE_LEN - 1) {
                int result = utf8_insert(ta->lines[ta->cursor_line],
                                          ta->line_len_bytes[ta->cursor_line],
                                          (int)sizeof(ta->lines[0]),
                                          ta->cursor_col, clip);
                if (result >= 0) {
                    ta->line_len_bytes[ta->cursor_line] = result;
                    ta->line_len_codepoints[ta->cursor_line] = utf8_codepoint_count(
                        ta->lines[ta->cursor_line], result);
                    ta->cursor_col += clip_cp;
                }
            }
        }
        tui_textarea_render(ta);
        return true;
    }

    /* Ctrl+X: cut */
    if (ni->ctrl && (key == 'x' || key == 'X')) {
        if (ta->selecting) {
            int start = ta->select_start_col < ta->select_end_col ? ta->select_start_col : ta->select_end_col;
            int end = ta->select_start_col < ta->select_end_col ? ta->select_end_col : ta->select_start_col;
            char* selected = utf8_substring(ta->lines[ta->cursor_line], start, end);
            tui_clipboard_copy(selected);
            free(selected);
            textarea_delete_selection(ta);
        } else {
            tui_clipboard_copy(ta->lines[ta->cursor_line]);
            ta->lines[ta->cursor_line][0] = '\0';
            ta->line_len_bytes[ta->cursor_line] = 0;
            ta->line_len_codepoints[ta->cursor_line] = 0;
            ta->cursor_col = 0;
        }
        tui_textarea_render(ta);
        return true;
    }

    /* Insert printable Unicode characters (exclude C1 controls 0x80-0x9F) */
    if (key >= 0x20 && key <= 0x10FFFF && !(key >= 0x80 && key <= 0x9F) && !ni->ctrl && !ni->alt) {
        int line_cp = ta->line_len_codepoints[ta->cursor_line];
        if (line_cp >= TEXTAREA_MAX_LINE_LEN - 1) return false;

        if (ta->selecting) textarea_delete_selection(ta);

        char encoded[4];
        int byte_len = utf8_encode(key, encoded);

        int insert_pos = utf8_cp_to_byte_offset(ta->lines[ta->cursor_line], ta->cursor_col);
        int remaining = ta->line_len_bytes[ta->cursor_line] - insert_pos;
        memmove(ta->lines[ta->cursor_line] + insert_pos + byte_len,
                ta->lines[ta->cursor_line] + insert_pos,
                (size_t)remaining + 1);
        memcpy(ta->lines[ta->cursor_line] + insert_pos, encoded, (size_t)byte_len);
        ta->line_len_bytes[ta->cursor_line] += byte_len;
        ta->line_len_codepoints[ta->cursor_line]++;
        ta->cursor_col++;
        textarea_clear_selection(ta);
        tui_textarea_render(ta);
        return true;
    }

    return false;
}

void tui_textarea_render(TuiTextArea* ta) {
    if (!ta || !ta->plane) return;
    unsigned cols;
    ncplane_dim_yx(ta->plane, NULL, &cols);

    unsigned bg = ta->focused ? ta->bg_focused : ta->bg_normal;
    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, ta->fg_text);
    ncchannels_set_bg_rgb(&base, bg);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(ta->plane, " ", 0, base);
    ncplane_erase(ta->plane);

    int visible = ta->height;

    /* Adjust scroll to keep cursor visible */
    if (ta->cursor_line < ta->scroll_line) {
        ta->scroll_line = ta->cursor_line;
    } else if (ta->cursor_line >= ta->scroll_line + visible) {
        ta->scroll_line = ta->cursor_line - visible + 1;
    }

    /* Selection range for current line */
    int sel_start = -1, sel_end = -1;
    if (ta->selecting) {
        sel_start = ta->select_start_col < ta->select_end_col ? ta->select_start_col : ta->select_end_col;
        sel_end = ta->select_start_col < ta->select_end_col ? ta->select_end_col : ta->select_start_col;
    }

    for (int row = 0; row < visible; row++) {
        int line_idx = ta->scroll_line + row;
        if (line_idx >= ta->line_count) break;

        char* line = ta->lines[line_idx];
        int line_cp_count = ta->line_len_codepoints[line_idx];
        int start_col = ta->scroll_col;
        if (start_col >= line_cp_count) start_col = line_cp_count > 0 ? line_cp_count - 1 : 0;

        int max_cols = (int)cols - 1;
        int screen_col = 0;

        for (int cp_i = start_col; cp_i < line_cp_count && screen_col < max_cols; ) {
            int buf_byte = utf8_cp_to_byte_offset(line, cp_i);
            const char* cp_ptr = line + buf_byte;
            uint32_t cp = utf8_decode(&cp_ptr);
            int cp_width = utf8_cp_width(cp);

            if (screen_col + cp_width > max_cols) break;

            bool in_selection = (line_idx == ta->cursor_line && sel_start >= 0 &&
                                  cp_i >= sel_start && cp_i < sel_end);

            if (in_selection) {
                ncplane_set_bg_rgb(ta->plane, ta->fg_text);
                ncplane_set_fg_rgb(ta->plane, bg);
            } else if (line_idx == ta->cursor_line && cp_i == ta->cursor_col && ta->focused) {
                ncplane_set_bg_rgb(ta->plane, ta->fg_cursor);
                ncplane_set_fg_rgb(ta->plane, ta->bg_normal);
            } else {
                ncplane_set_bg_rgb(ta->plane, bg);
                ncplane_set_fg_rgb(ta->plane, ta->fg_text);
            }

            char cp_buf[8];
            int cp_byte_len = (int)(cp_ptr - (line + buf_byte));
            memcpy(cp_buf, line + buf_byte, (size_t)cp_byte_len);
            cp_buf[cp_byte_len] = '\0';
            ncplane_putstr_yx(ta->plane, row, screen_col, cp_buf);

            screen_col += cp_width;
            cp_i++;
        }

        /* Draw cursor at end of line */
        if (ta->focused && line_idx == ta->cursor_line && ta->cursor_col == line_cp_count) {
            int cursor_screen_col = 0;
            for (int cp_i = start_col; cp_i < ta->cursor_col; ) {
                int buf_byte = utf8_cp_to_byte_offset(line, cp_i);
                const char* cp_ptr = line + buf_byte;
                uint32_t cp = utf8_decode(&cp_ptr);
                int cp_width = (cp >= 0x1100 && cp <= 0x115F) ||
                               (cp >= 0x2E80 && cp <= 0x9FFF) ||
                               (cp >= 0xAC00 && cp <= 0xD7AF) ||
                               (cp >= 0xF900 && cp <= 0xFAFF) ||
                               (cp >= 0x1F300 && cp <= 0x1F9FF) ? 2 : 1;
                cursor_screen_col += cp_width;
                cp_i++;
            }
            cursor_screen_col -= ta->scroll_col;
            /* Recalculate properly */
            cursor_screen_col = 0;
            for (int cp_i = 0; cp_i < ta->cursor_col; ) {
                int buf_byte = utf8_cp_to_byte_offset(line, cp_i);
                const char* cp_ptr = line + buf_byte;
                uint32_t cp = utf8_decode(&cp_ptr);
                int cp_width = (cp >= 0x1100 && cp <= 0x115F) ||
                               (cp >= 0x2E80 && cp <= 0x9FFF) ||
                               (cp >= 0xAC00 && cp <= 0xD7AF) ||
                               (cp >= 0xF900 && cp <= 0xFAFF) ||
                               (cp >= 0x1F300 && cp <= 0x1F9FF) ? 2 : 1;
                cursor_screen_col += cp_width;
                cp_i++;
            }
            cursor_screen_col -= ta->scroll_col;
            if (cursor_screen_col >= 0 && cursor_screen_col < (int)cols) {
                ncplane_set_fg_rgb(ta->plane, ta->bg_normal);
                ncplane_set_bg_rgb(ta->plane, ta->fg_cursor);
                ncplane_putchar_yx(ta->plane, row, cursor_screen_col, ' ');
            }
        }
    }
}

/* === VTable Implementation === */

static bool v_textarea_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiTextArea* ta = (TuiTextArea*)widget;
    return tui_textarea_handle_key(ta, key, ni);
}

static bool v_textarea_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    (void)widget; (void)key; (void)ni;
    return false;
}

static void v_textarea_render(void* widget) {
    tui_textarea_render((TuiTextArea*)widget);
}

static bool v_textarea_is_focusable(void* widget) { (void)widget; return true; }

static void v_textarea_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)widget; (void)max_h; (void)max_w;
    if (out_h) *out_h = -1;
    if (out_w) *out_w = -1;
}

static const TuiWidgetIface s_textarea_iface = {
    .handle_key = v_textarea_handle_key,
    .handle_mouse = v_textarea_handle_mouse,
    .render = v_textarea_render,
    .is_focusable = v_textarea_is_focusable,
    .preferred_size = v_textarea_preferred_size,
};

void tui_textarea_ensure_registered(void) {
    if (s_textarea_type_id < 0) {
        s_textarea_type_id = tui_widget_register(&s_textarea_iface);
    }
}

int tui_textarea_get_type_id(void) {
    tui_textarea_ensure_registered();
    return s_textarea_type_id;
}
