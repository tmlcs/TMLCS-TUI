#include "widget/text_input.h"
#include "core/theme.h"
#include "core/clipboard.h"
#include <stdlib.h>
#include <string.h>

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
    ti->cursor    = 0;
    ti->len       = 0;
    ti->scroll_off = 0;

    // Paleta de colores por defecto (tema Dracula mejorado para visibilidad)
    ti->bg_normal  = THEME_BG_TASKBAR;   // Fondo oscuro base
    ti->fg_normal  = THEME_FG_DEFAULT;   // Blanco texto
    ti->bg_focused = THEME_BG_TEXT_FOC;  // Fondo activo mas claro
    ti->fg_cursor  = THEME_FG_CURSOR;    // Cursor naranja vibrante

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
    memset(ti->buffer, 0, TEXT_INPUT_MAX_LEN);
    ti->cursor     = 0;
    ti->len        = 0;
    ti->scroll_off = 0;
}

const char* tui_text_input_get(TuiTextInput* ti) {
    return ti ? ti->buffer : NULL;
}

int tui_text_input_get_cursor(const TuiTextInput* ti) {
    return ti ? ti->cursor : -1;
}

int tui_text_input_get_len(const TuiTextInput* ti) {
    return ti ? ti->len : 0;
}

bool tui_text_input_is_focused(const TuiTextInput* ti) {
    return ti ? ti->focused : false;
}

bool tui_text_input_copy(TuiTextInput* ti) {
    if (!ti || ti->len == 0) return false;
    return tui_clipboard_copy(ti->buffer);
}

bool tui_text_input_paste(TuiTextInput* ti) {
    if (!ti) return false;
    char buf[CLIPBOARD_MAX_LEN];
    int len = tui_clipboard_paste(buf, sizeof(buf));
    if (len <= 0) return false;
    if (ti->len + len >= TEXT_INPUT_MAX_LEN - 1) return false;

    memmove(&ti->buffer[ti->cursor + len],
            &ti->buffer[ti->cursor],
            (size_t)(ti->len - ti->cursor + 1));
    memcpy(&ti->buffer[ti->cursor], buf, (size_t)len);
    ti->cursor += len;
    ti->len += len;
    tui_text_input_render(ti);
    return true;
}

bool tui_text_input_cut(TuiTextInput* ti) {
    if (!ti || ti->len == 0) return false;
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

    if (key == NCKEY_BACKSPACE || key == 127) {
        if (ti->cursor > 0) {
            // Eliminar el carácter anterior al cursor
            memmove(&ti->buffer[ti->cursor - 1],
                    &ti->buffer[ti->cursor],
                    ti->len - ti->cursor + 1);
            ti->cursor--;
            ti->len--;
        }
        tui_text_input_render(ti);
        return true;
    }

    if (key == NCKEY_LEFT) {
        if (ti->cursor > 0) ti->cursor--;
        tui_text_input_render(ti);
        return true;
    }

    if (key == NCKEY_RIGHT) {
        if (ti->cursor < ti->len) ti->cursor++;
        tui_text_input_render(ti);
        return true;
    }

    /* Home / Ctrl+A: move cursor to beginning */
    if (key == NCKEY_HOME || (ni->ctrl && (key == 'a' || key == 'A'))) {
        ti->cursor = 0;
        tui_text_input_render(ti);
        return true;
    }

    /* End / Ctrl+E: move cursor to end */
    if (key == NCKEY_END || (ni->ctrl && (key == 'e' || key == 'E'))) {
        ti->cursor = ti->len;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+Left: jump word left */
    if (ni->ctrl && key == NCKEY_LEFT) {
        while (ti->cursor > 0 && ti->buffer[ti->cursor - 1] == ' ') ti->cursor--;
        while (ti->cursor > 0 && ti->buffer[ti->cursor - 1] != ' ') ti->cursor--;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+Right: jump word right */
    if (ni->ctrl && key == NCKEY_RIGHT) {
        while (ti->cursor < ti->len && ti->buffer[ti->cursor] == ' ') ti->cursor++;
        while (ti->cursor < ti->len && ti->buffer[ti->cursor] != ' ') ti->cursor++;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+K: cut from cursor to end */
    if (ni->ctrl && (key == 'k' || key == 'K')) {
        ti->buffer[ti->cursor] = '\0';
        ti->len = ti->cursor;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+U: cut from beginning to cursor */
    if (ni->ctrl && (key == 'u' || key == 'U')) {
        memmove(ti->buffer, &ti->buffer[ti->cursor], (size_t)(ti->len - ti->cursor + 1));
        ti->len -= ti->cursor;
        ti->cursor = 0;
        tui_text_input_render(ti);
        return true;
    }

    /* Ctrl+W: delete word before cursor */
    if (ni->ctrl && (key == 'w' || key == 'W')) {
        int end = ti->cursor;
        while (end > 0 && ti->buffer[end - 1] == ' ') end--;
        while (end > 0 && ti->buffer[end - 1] != ' ') end--;
        memmove(&ti->buffer[end], &ti->buffer[ti->cursor], (size_t)(ti->len - ti->cursor + 1));
        ti->len -= (ti->cursor - end);
        ti->cursor = end;
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

    /* Ctrl+X: cut all text */
    if (ni->ctrl && (key == 'x' || key == 'X')) {
        tui_text_input_cut(ti);
        return true;
    }

    // Inserción de caracteres ASCII imprimibles
    if (key >= 0x20 && key < 0x7f && ti->len < TEXT_INPUT_MAX_LEN - 1) {
        // Desplazar el contenido a la derecha para insertar
        memmove(&ti->buffer[ti->cursor + 1],
                &ti->buffer[ti->cursor],
                ti->len - ti->cursor + 1);
        ti->buffer[ti->cursor] = (char)key;
        ti->cursor++;
        ti->len++;
        tui_text_input_render(ti);
        return true;
    }

    return false;
}

void tui_text_input_render(TuiTextInput* ti) {
    if (!ti || !ti->plane) return;

    unsigned cols;
    ncplane_dim_yx(ti->plane, NULL, &cols);

    int visible_width = (int)cols - 2; // Margen de 1 px a cada lado

    // Ajustar scroll para mantener el cursor visible
    if (ti->cursor < ti->scroll_off) {
        ti->scroll_off = ti->cursor;
    } else if (ti->cursor >= ti->scroll_off + visible_width) {
        ti->scroll_off = ti->cursor - visible_width + 1;
    }

    // Fondo opaco base (garantiza que las celdas vacías no sean transparentes)
    unsigned int bg = ti->focused ? ti->bg_focused : ti->bg_normal;
    uint64_t base_channels = 0;
    ncchannels_set_bg_rgb(&base_channels, bg);
    ncchannels_set_fg_rgb(&base_channels, ti->fg_normal);
    ncchannels_set_bg_alpha(&base_channels, NCALPHA_OPAQUE);
    ncchannels_set_fg_alpha(&base_channels, NCALPHA_OPAQUE);
    ncplane_set_base(ti->plane, " ", 0, base_channels);
    ncplane_erase(ti->plane);

    // Borde visual sutil: [ texto ]
    ncplane_set_fg_rgb(ti->plane, THEME_FG_TAB_INA);
    ncplane_putstr_yx(ti->plane, 0, 0, "[");
    ncplane_putstr_yx(ti->plane, 0, (int)cols - 1, "]");

    // Pintar los caracteres visibles
    for (int i = 0; i < visible_width; i++) {
        int buf_idx = ti->scroll_off + i;
        if (buf_idx >= ti->len) break;

        bool is_cursor = (buf_idx == ti->cursor);

        if (is_cursor && ti->focused) {
            ncplane_set_bg_rgb(ti->plane, ti->fg_cursor); // cursor block
            ncplane_set_fg_rgb(ti->plane, ti->bg_normal);
        } else {
            ncplane_set_bg_rgb(ti->plane, bg);
            ncplane_set_fg_rgb(ti->plane, ti->fg_normal);
        }
        ncplane_putchar_yx(ti->plane, 0, 1 + i, ti->buffer[buf_idx]);
    }

    // Cursor al final si está después del último carácter
    if (ti->focused) {
        int screen_cursor = ti->cursor - ti->scroll_off;
        if (screen_cursor >= 0 && screen_cursor < visible_width && ti->cursor == ti->len) {
            ncplane_set_bg_rgb(ti->plane, ti->fg_cursor);
            ncplane_set_fg_rgb(ti->plane, ti->bg_normal);
            ncplane_putchar_yx(ti->plane, 0, 1 + screen_cursor, ' ');
        }
    }
}
