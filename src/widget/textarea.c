#include "widget/textarea.h"
#include "core/theme.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    ta->cursor_line = 0;
    ta->cursor_col = 0;
    ta->scroll_line = 0;
    ta->scroll_col = 0;
    ta->height = height;
    ta->focused = false;
    ta->bg_normal = THEME_BG_TASKBAR;
    ta->bg_focused = THEME_BG_TEXT_FOC;
    ta->fg_text = THEME_FG_DEFAULT;
    ta->fg_cursor = THEME_FG_CURSOR;

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
    ta->cursor_line = 0;
    ta->cursor_col = 0;
    ta->scroll_line = 0;
    ta->scroll_col = 0;
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

static char s_textarea_buf[TEXTAREA_MAX_LINES * TEXTAREA_MAX_LINE_LEN];

const char* tui_textarea_get(TuiTextArea* ta) {
    if (!ta) return NULL;
    s_textarea_buf[0] = '\0';
    for (int i = 0; i < ta->line_count; i++) {
        if (i > 0) strncat(s_textarea_buf, "\n", sizeof(s_textarea_buf) - strlen(s_textarea_buf) - 1);
        strncat(s_textarea_buf, ta->lines[i], sizeof(s_textarea_buf) - strlen(s_textarea_buf) - 1);
    }
    return s_textarea_buf;
}

bool tui_textarea_handle_key(TuiTextArea* ta, uint32_t key, const struct ncinput* ni) {
    if (!ta || !ta->focused || ni->evtype == NCTYPE_RELEASE) return false;

    if (key == NCKEY_ENTER || key == '\n' || key == '\r') {
        /* Insert new line */
        if (ta->line_count < TEXTAREA_MAX_LINES - 1) {
            /* Shift lines down */
            for (int i = ta->line_count; i > ta->cursor_line; i--) {
                memmove(ta->lines[i], ta->lines[i-1], TEXTAREA_MAX_LINE_LEN);
            }
            /* Split current line at cursor */
            char* rest = ta->lines[ta->cursor_line] + ta->cursor_col;
            strncpy(ta->lines[ta->cursor_line + 1], rest, TEXTAREA_MAX_LINE_LEN - 1);
            ta->lines[ta->cursor_line + 1][TEXTAREA_MAX_LINE_LEN - 1] = '\0';
            ta->lines[ta->cursor_line][ta->cursor_col] = '\0';
            ta->line_count++;
            ta->cursor_line++;
            ta->cursor_col = 0;
            tui_textarea_render(ta);
        }
        return true;
    }

    if (key == NCKEY_BACKSPACE || key == 127) {
        if (ta->cursor_col > 0) {
            memmove(&ta->lines[ta->cursor_line][ta->cursor_col - 1],
                    &ta->lines[ta->cursor_line][ta->cursor_col],
                    strlen(&ta->lines[ta->cursor_line][ta->cursor_col]) + 1);
            ta->cursor_col--;
            tui_textarea_render(ta);
        } else if (ta->cursor_line > 0) {
            /* Join with previous line */
            int prev_len = (int)strlen(ta->lines[ta->cursor_line - 1]);
            int remaining = TEXTAREA_MAX_LINE_LEN - prev_len - 1;
            if (remaining > 0) {
                int src_len = (int)strlen(ta->lines[ta->cursor_line]);
                int copy = src_len < remaining ? src_len : remaining;
                memcpy(ta->lines[ta->cursor_line - 1] + prev_len, ta->lines[ta->cursor_line], (size_t)copy);
                ta->lines[ta->cursor_line - 1][prev_len + copy] = '\0';
            }
            for (int i = ta->cursor_line; i < ta->line_count - 1; i++) {
                memmove(ta->lines[i], ta->lines[i + 1], TEXTAREA_MAX_LINE_LEN);
            }
            ta->line_count--;
            ta->cursor_line--;
            ta->cursor_col = prev_len;
            tui_textarea_render(ta);
        }
        return true;
    }

    if (key == NCKEY_UP) {
        if (ta->cursor_line > 0) {
            ta->cursor_line--;
            int line_len = (int)strlen(ta->lines[ta->cursor_line]);
            if (ta->cursor_col > line_len) ta->cursor_col = line_len;
            tui_textarea_render(ta);
        }
        return true;
    }
    if (key == NCKEY_DOWN) {
        if (ta->cursor_line < ta->line_count - 1) {
            ta->cursor_line++;
            int line_len = (int)strlen(ta->lines[ta->cursor_line]);
            if (ta->cursor_col > line_len) ta->cursor_col = line_len;
            tui_textarea_render(ta);
        }
        return true;
    }
    if (key == NCKEY_LEFT) {
        if (ta->cursor_col > 0) {
            ta->cursor_col--;
            tui_textarea_render(ta);
        }
        return true;
    }
    if (key == NCKEY_RIGHT) {
        int line_len = (int)strlen(ta->lines[ta->cursor_line]);
        if (ta->cursor_col < line_len) {
            ta->cursor_col++;
            tui_textarea_render(ta);
        }
        return true;
    }

    /* ASCII insert */
    if (key >= 0x20 && key < 0x7f) {
        int line_len = (int)strlen(ta->lines[ta->cursor_line]);
        if (line_len < TEXTAREA_MAX_LINE_LEN - 1) {
            memmove(&ta->lines[ta->cursor_line][ta->cursor_col + 1],
                    &ta->lines[ta->cursor_line][ta->cursor_col],
                    (size_t)(line_len - ta->cursor_col + 1));
            ta->lines[ta->cursor_line][ta->cursor_col] = (char)key;
            ta->cursor_col++;
            tui_textarea_render(ta);
        }
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

    for (int row = 0; row < visible; row++) {
        int line_idx = ta->scroll_line + row;
        if (line_idx >= ta->line_count) break;

        /* Draw line text */
        int max_cols = (int)cols - 1;
        char* line = ta->lines[line_idx];
        int line_len = (int)strlen(line);
        int start_col = ta->scroll_col;
        if (start_col >= line_len) start_col = line_len > 0 ? line_len - 1 : 0;

        char buf[TEXTAREA_MAX_LINE_LEN];
        int copy = line_len - start_col;
        if (copy > max_cols) copy = max_cols;
        if (copy < 0) copy = 0;
        memcpy(buf, line + start_col, (size_t)copy);
        buf[copy] = '\0';
        ncplane_putstr_yx(ta->plane, row, 0, buf);

        /* Draw cursor */
        if (ta->focused && line_idx == ta->cursor_line) {
            int cursor_screen_col = ta->cursor_col - ta->scroll_col;
            if (cursor_screen_col >= 0 && cursor_screen_col < (int)cols) {
                ncplane_set_fg_rgb(ta->plane, ta->bg_normal);
                ncplane_set_bg_rgb(ta->plane, ta->fg_cursor);
                ncplane_putchar_yx(ta->plane, row, cursor_screen_col, ' ');
            }
        }
    }
}
