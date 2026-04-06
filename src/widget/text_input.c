#include "widget/text_input.h"
#include "core/theme.h"
#include "core/clipboard.h"
#include "core/widget.h"
#include "core/utf8.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
void tui_text_input_ensure_registered(void);
int tui_text_input_get_type_id(void);

static int s_text_input_type_id = -1;

TuiTextInput* tui_text_input_create(struct ncplane* parent,
                                     int y, int x, int width,
                                     TextInputSubmitCb on_submit,
                                     void* userdata) {
    TuiTextInput* ti = (TuiTextInput*)calloc(1, sizeof(TuiTextInput));
    if (!ti) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = (unsigned)width,
    };
    ti->plane = ncplane_create(parent, &opts);
    if (!ti->plane) { free(ti); return NULL; }

    ti->on_submit = on_submit;
    ti->userdata  = userdata;
    ti->focused   = false;
    ti->len_bytes = 0;
    ti->len_codepoints = 0;
    ti->cursor_cp = 0;
    ti->scroll_off = 0;
    ti->selecting = false;
    ti->select_start_cp = 0;
    ti->select_end_cp = 0;
    ti->buffer[0] = '\0';

    // Paleta de colores por defecto (tema Dracula mejorado para visibilidad)
    ti->bg_normal  = THEME_BG_TASKBAR;   // Fondo oscuro base
    ti->fg_normal  = THEME_FG_DEFAULT;   // Blanco texto
    ti->bg_focused = THEME_BG_TEXT_FOC;  // Fondo activo mas claro
    ti->fg_cursor  = THEME_FG_CURSOR;    // Cursor naranja vibrante

    tui_text_input_ensure_registered();
    ti->_type_id = s_text_input_type_id;

    tui_text_input_render(ti);
    return ti;
}

void tui_text_input_destroy(TuiTextInput* ti) {
    if (!ti) return;
    if (ti->plane) ncplane_destroy(ti->plane);
    free(ti);
}

void tui_text_input_clear(TuiTextInput* ti) {
    if (!ti) return;
    ti->buffer[0] = '\0';
    ti->len_bytes = 0;
    ti->len_codepoints = 0;
    ti->cursor_cp = 0;
    ti->scroll_off = 0;
    ti->selecting = false;
    ti->select_start_cp = 0;
    ti->select_end_cp = 0;
}

const char* tui_text_input_get(TuiTextInput* ti) {
    return ti ? ti->buffer : NULL;
}

int tui_text_input_get_cursor(const TuiTextInput* ti) {
    return ti ? ti->cursor_cp : -1;
}

int tui_text_input_get_len(const TuiTextInput* ti) {
    return ti ? ti->len_codepoints : 0;
}

bool tui_text_input_is_focused(const TuiTextInput* ti) {
    return ti ? ti->focused : false;
}

bool tui_text_input_is_selecting(const TuiTextInput* ti) {
    return ti ? ti->selecting : false;
}

bool tui_text_input_get_selection(const TuiTextInput* ti, int* out_start, int* out_end) {
    if (!ti || !ti->selecting) return false;
    int s = ti->select_start_cp < ti->select_end_cp ? ti->select_start_cp : ti->select_end_cp;
    int e = ti->select_start_cp < ti->select_end_cp ? ti->select_end_cp : ti->select_start_cp;
    if (out_start) *out_start = s;
    if (out_end) *out_end = e;
    return true;
}

static void clear_selection(TuiTextInput* ti) {
    ti->selecting = false;
    ti->select_start_cp = 0;
    ti->select_end_cp = 0;
}

static void delete_selection(TuiTextInput* ti) {
    if (!ti->selecting) return;
    int start = ti->select_start_cp < ti->select_end_cp ? ti->select_start_cp : ti->select_end_cp;
    int end = ti->select_start_cp < ti->select_end_cp ? ti->select_end_cp : ti->select_start_cp;
    ti->len_bytes = utf8_delete_range(ti->buffer, ti->len_bytes, start, end);
    ti->len_codepoints -= (end - start);
    ti->cursor_cp = start;
    clear_selection(ti);
}

bool tui_text_input_copy(TuiTextInput* ti) {
    if (!ti || ti->len_codepoints == 0) return false;
    if (ti->selecting) {
        int start = ti->select_start_cp < ti->select_end_cp ? ti->select_start_cp : ti->select_end_cp;
        int end = ti->select_start_cp < ti->select_end_cp ? ti->select_end_cp : ti->select_start_cp;
        char* selected = utf8_substring(ti->buffer, start, end);
        bool ok = tui_clipboard_copy(selected);
        free(selected);
        return ok;
    }
    return tui_clipboard_copy(ti->buffer);
}

bool tui_text_input_paste(TuiTextInput* ti) {
    if (!ti) return false;
    const char* clip = tui_clipboard_get();
    if (!clip || clip[0] == '\0') return false;

    if (ti->selecting) {
        delete_selection(ti);
    }

    int clip_cp = utf8_codepoint_count(clip, (int)strlen(clip));
    if (ti->len_codepoints + clip_cp >= TEXT_INPUT_MAX_LEN - 1) return false;

    int result = utf8_insert(ti->buffer, ti->len_bytes,
                             (int)sizeof(ti->buffer), ti->cursor_cp, clip);
    if (result < 0) return false;

    ti->len_bytes = result;
    ti->len_codepoints = utf8_codepoint_count(ti->buffer, ti->len_bytes);
    ti->cursor_cp += clip_cp;
    tui_text_input_render(ti);
    return true;
}

bool tui_text_input_cut(TuiTextInput* ti) {
    if (!ti || ti->len_codepoints == 0) return false;
    if (ti->selecting) {
        int start = ti->select_start_cp < ti->select_end_cp ? ti->select_start_cp : ti->select_end_cp;
        int end = ti->select_start_cp < ti->select_end_cp ? ti->select_end_cp : ti->select_start_cp;
        char* selected = utf8_substring(ti->buffer, start, end);
        tui_clipboard_copy(selected);
        free(selected);
        delete_selection(ti);
        tui_text_input_render(ti);
        return true;
    }
    tui_clipboard_copy(ti->buffer);
    tui_text_input_clear(ti);
    return true;
}

bool tui_text_input_handle_key(TuiTextInput* ti, uint32_t key, const struct ncinput* ni) {
    if (!ti || !ti->focused) return false;
    if (ni->evtype == NCTYPE_RELEASE) return false;

    if (key == NCKEY_ENTER || key == '\n' || key == '\r') {
        if (ti->on_submit) ti->on_submit(ti->buffer, ti->userdata);
        tui_text_input_clear(ti);
        tui_text_input_render(ti);
        return true;
    }

    /* Delete key */
    if (key == NCKEY_DEL) {
        if (ti->selecting) {
            delete_selection(ti);
        } else if (ti->cursor_cp < ti->len_codepoints) {
            ti->len_bytes = utf8_delete_range(ti->buffer, ti->len_bytes,
                                               ti->cursor_cp, ti->cursor_cp + 1);
            ti->len_codepoints--;
        }
        tui_text_input_render(ti);
        return true;
    }

    /* Backspace */
    if (key == NCKEY_BACKSPACE || key == 127) {
        if (ti->selecting) {
            delete_selection(ti);
        } else if (ti->cursor_cp > 0) {
            ti->len_bytes = utf8_delete_range(ti->buffer, ti->len_bytes,
                                               ti->cursor_cp - 1, ti->cursor_cp);
            ti->len_codepoints--;
            ti->cursor_cp--;
        }
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+Left: jump word left */
    if (ni->ctrl && key == NCKEY_LEFT) {
        clear_selection(ti);
        while (ti->cursor_cp > 0) {
            int byte_off = utf8_cp_to_byte_offset(ti->buffer, ti->cursor_cp - 1);
            if ((unsigned char)ti->buffer[byte_off] == ' ') break;
            ti->cursor_cp--;
        }
        while (ti->cursor_cp > 0) {
            int byte_off = utf8_cp_to_byte_offset(ti->buffer, ti->cursor_cp - 1);
            if ((unsigned char)ti->buffer[byte_off] != ' ') break;
            ti->cursor_cp--;
        }
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+Right: jump word right */
    if (ni->ctrl && key == NCKEY_RIGHT) {
        clear_selection(ti);
        while (ti->cursor_cp < ti->len_codepoints) {
            int byte_off = utf8_cp_to_byte_offset(ti->buffer, ti->cursor_cp);
            if (ti->buffer[byte_off] == '\0' || (unsigned char)ti->buffer[byte_off] == ' ') break;
            ti->cursor_cp++;
        }
        while (ti->cursor_cp < ti->len_codepoints) {
            int byte_off = utf8_cp_to_byte_offset(ti->buffer, ti->cursor_cp);
            if (ti->buffer[byte_off] == '\0' || (unsigned char)ti->buffer[byte_off] != ' ') break;
            ti->cursor_cp++;
        }
        tui_text_input_render(ti);
        return true;
    }

    /* Shift+Left: extend selection left */
    if (key == NCKEY_LEFT && ni->shift) {
        if (!ti->selecting) {
            ti->selecting = true;
            ti->select_start_cp = ti->cursor_cp;
        }
        if (ti->cursor_cp > 0) ti->cursor_cp--;
        ti->select_end_cp = ti->cursor_cp;
        tui_text_input_render(ti);
        return true;
    }

    /* Shift+Right: extend selection right */
    if (key == NCKEY_RIGHT && ni->shift) {
        if (!ti->selecting) {
            ti->selecting = true;
            ti->select_start_cp = ti->cursor_cp;
        }
        if (ti->cursor_cp < ti->len_codepoints) ti->cursor_cp++;
        ti->select_end_cp = ti->cursor_cp;
        tui_text_input_render(ti);
        return true;
    }

    if (key == NCKEY_LEFT) {
        clear_selection(ti);
        if (ti->cursor_cp > 0) ti->cursor_cp--;
        tui_text_input_render(ti);
        return true;
    }

    if (key == NCKEY_RIGHT) {
        clear_selection(ti);
        if (ti->cursor_cp < ti->len_codepoints) ti->cursor_cp++;
        tui_text_input_render(ti);
        return true;
    }

    /* Home: move cursor to beginning */
    if (key == NCKEY_HOME) {
        clear_selection(ti);
        ti->cursor_cp = 0;
        tui_text_input_render(ti);
        return true;
    }

    /* End: move cursor to end */
    if (key == NCKEY_END) {
        clear_selection(ti);
        ti->cursor_cp = ti->len_codepoints;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+A: select all */
    if (ni->ctrl && (key == 'a' || key == 'A')) {
        ti->selecting = true;
        ti->select_start_cp = 0;
        ti->select_end_cp = ti->len_codepoints;
        ti->cursor_cp = ti->len_codepoints;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+E: move cursor to end */
    if (ni->ctrl && (key == 'e' || key == 'E')) {
        clear_selection(ti);
        ti->cursor_cp = ti->len_codepoints;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+K: cut from cursor to end */
    if (ni->ctrl && (key == 'k' || key == 'K')) {
        int byte_off = utf8_cp_to_byte_offset(ti->buffer, ti->cursor_cp);
        ti->buffer[byte_off] = '\0';
        ti->len_bytes = byte_off;
        ti->len_codepoints = ti->cursor_cp;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+U: cut from beginning to cursor */
    if (ni->ctrl && (key == 'u' || key == 'U')) {
        int byte_off = utf8_cp_to_byte_offset(ti->buffer, ti->cursor_cp);
        int remaining = ti->len_bytes - byte_off;
        memmove(ti->buffer, ti->buffer + byte_off, (size_t)remaining + 1);
        ti->len_bytes -= byte_off;
        ti->len_codepoints -= ti->cursor_cp;
        ti->cursor_cp = 0;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+W: delete word before cursor */
    if (ni->ctrl && (key == 'w' || key == 'W')) {
        int end = ti->cursor_cp;
        while (end > 0) {
            int byte_off = utf8_cp_to_byte_offset(ti->buffer, end - 1);
            if ((unsigned char)ti->buffer[byte_off] != ' ') break;
            end--;
        }
        while (end > 0) {
            int byte_off = utf8_cp_to_byte_offset(ti->buffer, end - 1);
            if ((unsigned char)ti->buffer[byte_off] == ' ') break;
            end--;
        }
        int end_byte = utf8_cp_to_byte_offset(ti->buffer, ti->cursor_cp);
        int remaining = ti->len_bytes - end_byte;
        int start_byte = utf8_cp_to_byte_offset(ti->buffer, end);
        memmove(ti->buffer + start_byte, ti->buffer + end_byte, (size_t)remaining + 1);
        ti->len_bytes -= (end_byte - start_byte);
        ti->len_codepoints = utf8_codepoint_count(ti->buffer, ti->len_bytes);
        ti->cursor_cp = end;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+C: copy */
    if (ni->ctrl && (key == 'c' || key == 'C')) {
        tui_text_input_copy(ti);
        return true;
    }

    /* Ctrl+V: paste */
    if (ni->ctrl && (key == 'v' || key == 'V')) {
        tui_text_input_paste(ti);
        return true;
    }

    /* Ctrl+X: cut */
    if (ni->ctrl && (key == 'x' || key == 'X')) {
        tui_text_input_cut(ti);
        return true;
    }

    /* Insert printable Unicode characters (exclude C1 controls 0x80-0x9F) */
    if (key >= 0x20 && key <= 0x10FFFF && !(key >= 0x80 && key <= 0x9F) && !ni->ctrl && !ni->alt) {
        if (ti->len_codepoints >= TEXT_INPUT_MAX_LEN - 1) return false;

        /* If selecting, delete selection first */
        if (ti->selecting) {
            delete_selection(ti);
        }

        char encoded[4];
        int byte_len = utf8_encode(key, encoded);

        /* Insert at cursor position */
        int insert_pos = utf8_cp_to_byte_offset(ti->buffer, ti->cursor_cp);
        int remaining = ti->len_bytes - insert_pos;
        memmove(ti->buffer + insert_pos + byte_len,
                ti->buffer + insert_pos,
                (size_t)remaining + 1);
        memcpy(ti->buffer + insert_pos, encoded, (size_t)byte_len);
        ti->len_bytes += byte_len;
        ti->len_codepoints++;
        ti->cursor_cp++;
        clear_selection(ti);
        tui_text_input_render(ti);
        return true;
    }

    return false;
}

void tui_text_input_render(TuiTextInput* ti) {
    if (!ti || !ti->plane) return;

    unsigned cols;
    ncplane_dim_yx(ti->plane, NULL, &cols);

    int visible_width = (int)cols - 2; /* Margen de 1 px a cada lado */

    /* Adjust scroll to keep cursor visible */
    if (ti->cursor_cp < ti->scroll_off) {
        ti->scroll_off = ti->cursor_cp;
    } else if (ti->cursor_cp >= ti->scroll_off + visible_width) {
        ti->scroll_off = ti->cursor_cp - visible_width + 1;
    }

    /* Background */
    unsigned int bg = ti->focused ? ti->bg_focused : ti->bg_normal;
    uint64_t base_channels = 0;
    ncchannels_set_bg_rgb(&base_channels, bg);
    ncchannels_set_fg_rgb(&base_channels, ti->fg_normal);
    ncchannels_set_bg_alpha(&base_channels, NCALPHA_OPAQUE);
    ncchannels_set_fg_alpha(&base_channels, NCALPHA_OPAQUE);
    ncplane_set_base(ti->plane, " ", 0, base_channels);
    ncplane_erase(ti->plane);

    /* Border */
    ncplane_set_fg_rgb(ti->plane, THEME_FG_TAB_INA);
    ncplane_putstr_yx(ti->plane, 0, 0, "[");
    ncplane_putstr_yx(ti->plane, 0, (int)cols - 1, "]");

    /* Selection range (normalized) */
    int sel_start = -1, sel_end = -1;
    if (ti->selecting) {
        sel_start = ti->select_start_cp < ti->select_end_cp ? ti->select_start_cp : ti->select_end_cp;
        sel_end = ti->select_start_cp < ti->select_end_cp ? ti->select_end_cp : ti->select_start_cp;
    }

    /* Render visible codepoints using display width */
    int screen_col = 0;
    for (int cp_i = ti->scroll_off; cp_i < ti->len_codepoints && screen_col < visible_width; ) {
        int buf_byte = utf8_cp_to_byte_offset(ti->buffer, cp_i);
        const char* cp_ptr = ti->buffer + buf_byte;
        uint32_t cp = utf8_decode(&cp_ptr);
        int cp_width = (cp >= 0x1100 && cp <= 0x115F) ||
                       (cp >= 0x2E80 && cp <= 0x9FFF) ||
                       (cp >= 0xAC00 && cp <= 0xD7AF) ||
                       (cp >= 0xF900 && cp <= 0xFAFF) ||
                       (cp >= 0x1F300 && cp <= 0x1F9FF) ? 2 : 1;

        if (screen_col + cp_width > visible_width) break; /* Don't partially draw */

        bool in_selection = (sel_start >= 0 && cp_i >= sel_start && cp_i < sel_end);

        if (in_selection) {
            ncplane_set_bg_rgb(ti->plane, ti->fg_normal);
            ncplane_set_fg_rgb(ti->plane, bg);
        } else if (cp_i == ti->cursor_cp && ti->focused) {
            ncplane_set_bg_rgb(ti->plane, ti->fg_cursor);
            ncplane_set_fg_rgb(ti->plane, ti->bg_normal);
        } else {
            ncplane_set_bg_rgb(ti->plane, bg);
            ncplane_set_fg_rgb(ti->plane, ti->fg_normal);
        }

        /* Copy the codepoint bytes into a null-terminated buffer for putegc */
        char cp_buf[8];
        int cp_byte_len = (int)(cp_ptr - (ti->buffer + buf_byte));
        memcpy(cp_buf, ti->buffer + buf_byte, (size_t)cp_byte_len);
        cp_buf[cp_byte_len] = '\0';
        ncplane_putstr_yx(ti->plane, 0, 1 + screen_col, cp_buf);

        screen_col += cp_width;
        cp_i++;
    }

    /* Draw cursor at end if cursor is at end of text */
    if (ti->focused) {
        int screen_cursor = 0;
        for (int cp_i = ti->scroll_off; cp_i < ti->cursor_cp; ) {
            int buf_byte = utf8_cp_to_byte_offset(ti->buffer, cp_i);
            const char* cp_ptr = ti->buffer + buf_byte;
            uint32_t cp = utf8_decode(&cp_ptr);
            int cp_width = (cp >= 0x1100 && cp <= 0x115F) ||
                           (cp >= 0x2E80 && cp <= 0x9FFF) ||
                           (cp >= 0xAC00 && cp <= 0xD7AF) ||
                           (cp >= 0xF900 && cp <= 0xFAFF) ||
                           (cp >= 0x1F300 && cp <= 0x1F9FF) ? 2 : 1;
            screen_cursor += cp_width;
            cp_i++;
        }
        if (screen_cursor >= 0 && screen_cursor < visible_width && ti->cursor_cp == ti->len_codepoints) {
            ncplane_set_bg_rgb(ti->plane, ti->fg_cursor);
            ncplane_set_fg_rgb(ti->plane, ti->bg_normal);
            ncplane_putchar_yx(ti->plane, 0, 1 + screen_cursor, ' ');
        }
    }
}

bool tui_text_input_handle_mouse(TuiTextInput* ti, uint32_t key, const struct ncinput* ni) {
    if (!ti || !ni || !ti->plane || ni->evtype == NCTYPE_RELEASE) return false;
    if (key != NCKEY_BUTTON1) return false;

    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(ti->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(ti->plane, &rows, &cols);

    int local_y = ni->y - plane_abs_y;
    int local_x = ni->x - plane_abs_x;

    if (local_y < 0 || local_y >= (int)rows || local_x < 0 || local_x >= (int)cols) return false;

    /* Calculate cursor position from click. Text starts at x=1 (after "[" border). */
    int click_screen_col = local_x - 1;
    if (click_screen_col < 0) click_screen_col = 0;

    /* Map screen column back to codepoint index */
    int cp_i = ti->scroll_off;
    int screen_col = 0;
    while (cp_i < ti->len_codepoints && screen_col < click_screen_col) {
        int buf_byte = utf8_cp_to_byte_offset(ti->buffer, cp_i);
        const char* cp_ptr = ti->buffer + buf_byte;
        uint32_t cp = utf8_decode(&cp_ptr);
        int cp_width = (cp >= 0x1100 && cp <= 0x115F) ||
                       (cp >= 0x2E80 && cp <= 0x9FFF) ||
                       (cp >= 0xAC00 && cp <= 0xD7AF) ||
                       (cp >= 0xF900 && cp <= 0xFAFF) ||
                       (cp >= 0x1F300 && cp <= 0x1F9FF) ? 2 : 1;
        if (screen_col + cp_width > click_screen_col) break;
        screen_col += cp_width;
        cp_i++;
    }

    clear_selection(ti);
    ti->cursor_cp = cp_i;
    ti->focused = true;
    tui_text_input_render(ti);
    return true;
}

void tui_text_input_set_focused(TuiTextInput* ti, bool focused) {
    if (!ti) return;
    ti->focused = focused;
    tui_text_input_render(ti);
}

/* === VTable Implementation === */

static bool v_text_input_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiTextInput* ti = (TuiTextInput*)widget;
    return tui_text_input_handle_key(ti, key, ni);
}

static bool v_text_input_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiTextInput* ti = (TuiTextInput*)widget;
    return tui_text_input_handle_mouse(ti, key, ni);
}

static void v_text_input_render(void* widget) {
    tui_text_input_render((TuiTextInput*)widget);
}

static bool v_text_input_is_focusable(void* widget) { (void)widget; return true; }

static void v_text_input_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)widget; (void)max_h; (void)max_w;
    if (out_h) *out_h = 1;
    if (out_w) *out_w = -1;  /* fill available width */
}

static const TuiWidgetIface s_text_input_iface = {
    .handle_key = v_text_input_handle_key,
    .handle_mouse = v_text_input_handle_mouse,
    .render = v_text_input_render,
    .is_focusable = v_text_input_is_focusable,
    .preferred_size = v_text_input_preferred_size,
};

void tui_text_input_ensure_registered(void) {
    if (s_text_input_type_id < 0) {
        s_text_input_type_id = tui_widget_register(&s_text_input_iface);
    }
}

int tui_text_input_get_type_id(void) {
    tui_text_input_ensure_registered();
    return s_text_input_type_id;
}
