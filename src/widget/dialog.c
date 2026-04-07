#include "widget/dialog.h"
#include "core/theme.h"
#include "core/widget.h"
#include "core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void tui_dialog_ensure_registered(void);
int tui_dialog_get_type_id(void);

static int s_dialog_type_id = -1;

static void ensure_capacity(TuiDialog* dialog) {
    if (dialog->button_count >= dialog->button_capacity) {
        int new_cap = dialog->button_capacity * 2;
        char** new_btn = (char**)realloc(dialog->buttons, (size_t)new_cap * sizeof(char*));
        if (!new_btn) {
            tui_log(LOG_ERROR, "OOM in dialog ensure_capacity");
            return;
        }
        dialog->buttons = new_btn;
        dialog->button_capacity = new_cap;
    }
}

TuiDialog* tui_dialog_create(struct ncplane* parent, int y, int x,
                              int width, int height,
                              const char* title, const char* message,
                              TuiDialogCb cb, void* userdata) {
    if (!parent || width < 10 || height < 4 || !title || !message) return NULL;
    TuiDialog* dialog = (TuiDialog*)calloc(1, sizeof(TuiDialog));
    if (!dialog) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = (unsigned)height, .cols = (unsigned)width,
    };
    dialog->plane = ncplane_create(parent, &opts);
    if (!dialog->plane) { free(dialog); return NULL; }

    dialog->title = strdup(title);
    if (!dialog->title) { free(dialog); return NULL; }
    dialog->message = strdup(message);
    if (!dialog->message) { free(dialog->title); ncplane_destroy(dialog->plane); free(dialog); return NULL; }
    dialog->button_capacity = 4;
    dialog->buttons = (char**)calloc((size_t)dialog->button_capacity, sizeof(char*));
    if (!dialog->buttons) {
        free(dialog->message); free(dialog->title);
        ncplane_destroy(dialog->plane); free(dialog); return NULL;
    }
    dialog->button_count = 0;
    dialog->active_button = -1;
    dialog->width = width;
    dialog->height = height;
    dialog->focused = false;
    dialog->cb = cb;
    dialog->userdata = userdata;
    dialog->fg_title = THEME_FG_TAB_ACTIVE;
    dialog->fg_message = THEME_FG_DEFAULT;
    dialog->fg_button_active = THEME_FG_TAB_ACTIVE;
    dialog->fg_button_inactive = THEME_FG_TAB_INA;
    dialog->bg_normal = THEME_BG_DARKEST;
    dialog->bg_button_active = THEME_BG_TAB_ACTIVE;
    dialog->border_color = THEME_FG_SEPARATOR;

    tui_dialog_ensure_registered();
    dialog->_type_id = s_dialog_type_id;

    tui_dialog_render(dialog);
    return dialog;
}

void tui_dialog_destroy(TuiDialog* dialog) {
    if (!dialog) return;
    free(dialog->title);
    free(dialog->message);
    for (int i = 0; i < dialog->button_count; i++) free(dialog->buttons[i]);
    free(dialog->buttons);
    if (dialog->plane) ncplane_destroy(dialog->plane);
    free(dialog);
}

bool tui_dialog_add_button(TuiDialog* dialog, const char* label) {
    if (!dialog || !label) return false;
    ensure_capacity(dialog);
    if (dialog->button_count >= dialog->button_capacity) return false;
    dialog->buttons[dialog->button_count] = strdup(label);
    if (!dialog->buttons[dialog->button_count]) return false;
    if (dialog->active_button == -1) dialog->active_button = dialog->button_count;
    dialog->button_count++;
    tui_dialog_render(dialog);
    return true;
}

int tui_dialog_get_active_button(const TuiDialog* dialog) {
    return dialog ? dialog->active_button : -1;
}

void tui_dialog_set_active_button(TuiDialog* dialog, int index) {
    if (!dialog) return;
    if (dialog->button_count == 0) { dialog->active_button = -1; return; }
    if (index < 0) index = 0;
    if (index >= dialog->button_count) index = dialog->button_count - 1;
    dialog->active_button = index;
    tui_dialog_render(dialog);
}

int tui_dialog_get_button_count(const TuiDialog* dialog) {
    return dialog ? dialog->button_count : 0;
}

static void activate_button(TuiDialog* dialog, int index) {
    if (!dialog || index < 0 || index >= dialog->button_count) return;
    if (dialog->cb) dialog->cb(index, dialog->userdata);
}

bool tui_dialog_handle_key(TuiDialog* dialog, uint32_t key, const struct ncinput* ni) {
    if (!dialog || !ni) return false;

    if (key == NCKEY_ESC) {
        if (dialog->cb) dialog->cb(-1, dialog->userdata);
        return true;
    }

    if (key == NCKEY_LEFT) {
        if (dialog->button_count > 0) {
            int new_btn = dialog->active_button - 1;
            if (new_btn < 0) new_btn = dialog->button_count - 1;
            tui_dialog_set_active_button(dialog, new_btn);
        }
        return true;
    }
    if (key == NCKEY_RIGHT) {
        if (dialog->button_count > 0) {
            int new_btn = dialog->active_button + 1;
            if (new_btn >= dialog->button_count) new_btn = 0;
            tui_dialog_set_active_button(dialog, new_btn);
        }
        return true;
    }

    if (key == NCKEY_ENTER || key == ' ') {
        if (dialog->active_button >= 0) {
            activate_button(dialog, dialog->active_button);
        }
        return true;
    }
    return false;
}

bool tui_dialog_handle_mouse(TuiDialog* dialog, uint32_t key, const struct ncinput* ni) {
    if (!dialog || !ni || !dialog->plane) return false;
    if (key != NCKEY_BUTTON1) return false;

    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(dialog->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(dialog->plane, &rows, &cols);
    (void)rows;

    int local_y = ni->y - plane_abs_y;
    if (local_y != dialog->height - 1) return false; /* Buttons are on bottom row */

    int local_x = ni->x - plane_abs_x;
    int cur_x = 1;
    for (int i = 0; i < dialog->button_count; i++) {
        int btn_w = (int)strlen(dialog->buttons[i]) + 4; /* "[Label] " */
        if (local_x >= cur_x && local_x < cur_x + btn_w) {
            tui_dialog_set_active_button(dialog, i);
            activate_button(dialog, i);
            return true;
        }
        cur_x += btn_w;
    }
    return false;
}

void tui_dialog_render(TuiDialog* dialog) {
    if (!dialog || !dialog->plane) return;
    unsigned cols;
    ncplane_dim_yx(dialog->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, dialog->fg_message);
    ncchannels_set_bg_rgb(&base, dialog->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(dialog->plane, " ", 0, base);
    ncplane_erase(dialog->plane);

    /* Draw border — bulk writes instead of char-by-char */
    ncplane_set_fg_rgb(dialog->plane, dialog->border_color);
    {
        char hline[1024];
        int n = (int)cols < 1023 ? (int)cols : 1023;
        memset(hline, '-', (size_t)n);
        hline[n] = '\0';
        ncplane_putstr_yx(dialog->plane, 0, 0, hline);
        ncplane_putstr_yx(dialog->plane, dialog->height - 1, 0, hline);
    }
    for (int row = 0; row < dialog->height; row++) {
        ncplane_putstr_yx(dialog->plane, row, 0, "|");
        ncplane_putstr_yx(dialog->plane, row, (int)cols - 1, "|");
    }

    /* Draw title centered on row 1 */
    ncplane_set_fg_rgb(dialog->plane, dialog->fg_title);
    int title_len = (int)strlen(dialog->title);
    int title_x = ((int)cols - title_len) / 2;
    if (title_x < 1) title_x = 1;
    ncplane_putstr_yx(dialog->plane, 1, title_x, dialog->title);

    /* Draw message centered in middle rows */
    ncplane_set_fg_rgb(dialog->plane, dialog->fg_message);
    int msg_start_row = 2;
    int msg_end_row = dialog->height - 2;
    int msg_len = (int)strlen(dialog->message);
    int msg_x = ((int)cols - msg_len) / 2;
    if (msg_x < 1) msg_x = 1;
    int msg_center_y = (msg_start_row + msg_end_row) / 2;
    if (msg_center_y >= msg_start_row && msg_center_y < msg_end_row) {
        ncplane_putstr_yx(dialog->plane, msg_center_y, msg_x, dialog->message);
    }

    /* Draw buttons on bottom row */
    int btn_x = 1;
    for (int i = 0; i < dialog->button_count; i++) {
        bool is_active = (i == dialog->active_button);
        const char* label = dialog->buttons[i];

        if (is_active) {
            ncplane_set_fg_rgb(dialog->plane, dialog->fg_button_active);
            ncplane_set_bg_rgb(dialog->plane, dialog->bg_button_active);
        } else {
            ncplane_set_fg_rgb(dialog->plane, dialog->fg_button_inactive);
            ncplane_set_bg_rgb(dialog->plane, dialog->bg_normal);
        }

        int btn_w = (int)strlen(label) + 4;
        ncplane_putstr_yx(dialog->plane, dialog->height - 1, btn_x, "[");
        ncplane_putstr_yx(dialog->plane, dialog->height - 1, btn_x + 1, label);
        ncplane_putstr_yx(dialog->plane, dialog->height - 1, btn_x + 1 + (int)strlen(label), "]");

        btn_x += btn_w;
    }
}

/* === VTable === */

static bool v_dlg_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_dialog_handle_key((TuiDialog*)widget, key, ni);
}

static bool v_dlg_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_dialog_handle_mouse((TuiDialog*)widget, key, ni);
}

static void v_dlg_render(void* widget) {
    tui_dialog_render((TuiDialog*)widget);
}

static bool v_dlg_is_focusable(void* widget) { (void)widget; return true; }

static void v_dlg_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)max_h; (void)max_w;
    TuiDialog* dlg = (TuiDialog*)widget;
    int msg_len = dlg->message ? (int)strlen(dlg->message) : 0;
    int btn_w = 0;
    for (int i = 0; i < dlg->button_count; i++) {
        btn_w += (int)strlen(dlg->buttons[i]) + 5;
    }
    if (out_h) *out_h = 4;
    if (out_w) *out_w = (msg_len > btn_w ? msg_len + 6 : btn_w);
}

static const TuiWidgetIface s_dialog_iface = {
    .handle_key = v_dlg_handle_key,
    .handle_mouse = v_dlg_handle_mouse,
    .render = v_dlg_render,
    .is_focusable = v_dlg_is_focusable,
    .preferred_size = v_dlg_preferred_size,
};

void tui_dialog_ensure_registered(void) {
    if (s_dialog_type_id < 0) {
        s_dialog_type_id = tui_widget_register(&s_dialog_iface);
    }
}

int tui_dialog_get_type_id(void) {
    tui_dialog_ensure_registered();
    return s_dialog_type_id;
}
