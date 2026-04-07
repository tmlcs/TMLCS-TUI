#include "widget/table.h"
#include "core/theme.h"
#include "core/widget.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

void tui_table_ensure_registered(void);
int tui_table_get_type_id(void);

static int s_table_type_id = -1;

static void ensure_row_capacity(TuiTable* table) {
    if (table->row_count >= table->row_capacity) {
        int new_cap = table->row_capacity * 2;
        char** new_cells = (char**)realloc(table->cells,
            (size_t)new_cap * (size_t)table->col_count * sizeof(char*));
        if (!new_cells) {
            tui_log(LOG_ERROR, "OOM in table ensure_row_capacity");
            return;
        }
        /* Zero out new row slots */
        for (int i = table->row_count * table->col_count;
             i < new_cap * table->col_count; i++) {
            new_cells[i] = NULL;
        }
        table->cells = new_cells;
        table->row_capacity = new_cap;
    }
}

TuiTable* tui_table_create(struct ncplane* parent, int y, int x,
                             int width, int height, int col_count) {
    if (!parent || width < 4 || height < 2 || col_count < 1) return NULL;
    TuiTable* table = (TuiTable*)calloc(1, sizeof(TuiTable));
    if (!table) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = (unsigned)height, .cols = (unsigned)width,
    };
    table->plane = ncplane_create(parent, &opts);
    if (!table->plane) { free(table); return NULL; }

    table->col_capacity = col_count;
    table->headers = (char**)calloc((size_t)col_count, sizeof(char*));
    table->col_widths = (int*)calloc((size_t)col_count, sizeof(int));
    table->col_sort = (TuiSortOrder*)calloc((size_t)col_count, sizeof(TuiSortOrder));
    if (!table->headers || !table->col_widths || !table->col_sort) {
        free(table->headers); free(table->col_widths); free(table->col_sort);
        ncplane_destroy(table->plane); free(table); return NULL;
    }
    table->col_count = col_count;

    table->row_capacity = 8;
    table->cells = (char**)calloc((size_t)table->row_capacity * (size_t)col_count, sizeof(char*));
    if (!table->cells) {
        free(table->headers); free(table->col_widths); free(table->col_sort);
        ncplane_destroy(table->plane); free(table); return NULL;
    }

    table->row_count = 0;
    table->selected_row = -1;
    table->scroll_off = 0;
    table->height = height;
    table->width = width;
    table->focused = false;
    table->fg_header = THEME_FG_TAB_ACTIVE;
    table->fg_text = THEME_FG_DEFAULT;
    table->fg_selected = THEME_FG_TAB_ACTIVE;
    table->fg_separator = THEME_FG_SEPARATOR;
    table->bg_normal = THEME_BG_DARKEST;
    table->bg_header = THEME_BG_DARK;
    table->bg_selected = THEME_BG_TAB_ACTIVE;
    table->bg_alt = THEME_BG_TASKBAR;

    tui_table_ensure_registered();
    table->_type_id = s_table_type_id;

    tui_table_render(table);
    return table;
}

void tui_table_destroy(TuiTable* table) {
    if (!table) return;
    for (int i = 0; i < table->row_count * table->col_count; i++) free(table->cells[i]);
    free(table->cells);
    for (int i = 0; i < table->col_count; i++) free(table->headers[i]);
    free(table->headers);
    free(table->col_widths);
    free(table->col_sort);
    if (table->plane) ncplane_destroy(table->plane);
    free(table);
}

bool tui_table_set_header(TuiTable* table, int col, const char* header) {
    if (!table || col < 0 || col >= table->col_count || !header) return false;
    char* copy = strdup(header);
    if (!copy) {
        tui_log(LOG_ERROR, "Out of memory in tui_table_set_header");
        return false;
    }
    free(table->headers[col]);
    table->headers[col] = copy;
    int hl = (int)strlen(header);
    if (hl > table->col_widths[col]) table->col_widths[col] = hl;
    return true;
}

int tui_table_add_row(TuiTable* table, const char** values) {
    if (!table || !values) return -1;
    ensure_row_capacity(table);
    if (table->row_count >= table->row_capacity) return -1;

    int row_idx = table->row_count;
    for (int col = 0; col < table->col_count; col++) {
        int idx = row_idx * table->col_count + col;
        if (values[col]) {
            char* copy = strdup(values[col]);
            if (!copy) {
                tui_log(LOG_ERROR, "Out of memory in tui_table_add_row");
                for (int c = 0; c < col; c++) {
                    free(table->cells[row_idx * table->col_count + c]);
                    table->cells[row_idx * table->col_count + c] = NULL;
                }
                return -1;
            }
            table->cells[idx] = copy;
            int vl = (int)strlen(values[col]);
            if (vl > table->col_widths[col]) table->col_widths[col] = vl;
        } else {
            char* empty = strdup("");
            if (!empty) {
                tui_log(LOG_ERROR, "Out of memory in tui_table_add_row");
                for (int c = 0; c < col; c++) {
                    free(table->cells[row_idx * table->col_count + c]);
                    table->cells[row_idx * table->col_count + c] = NULL;
                }
                return -1;
            }
            table->cells[idx] = empty;
        }
    }
    table->row_count++;
    if (table->selected_row == -1) table->selected_row = 0;
    tui_table_render(table);
    return row_idx;
}

int tui_table_get_selected(const TuiTable* table) {
    return table ? table->selected_row : -1;
}

void tui_table_set_selected(TuiTable* table, int index) {
    if (!table) return;
    if (index < 0) index = 0;
    if (index >= table->row_count) index = table->row_count > 0 ? table->row_count - 1 : -1;
    table->selected_row = index;

    /* Adjust scroll */
    if (table->selected_row < table->scroll_off) {
        table->scroll_off = table->selected_row;
    } else if (table->selected_row >= table->scroll_off + table->height - 1) {
        table->scroll_off = table->selected_row - table->height + 2;
    }
    tui_table_render(table);
}

void tui_table_set_focused(TuiTable* table, bool focused) {
    if (!table) return;
    table->focused = focused;
    tui_table_render(table);
}

int tui_table_get_row_count(const TuiTable* table) {
    return table ? table->row_count : 0;
}

const char* tui_table_get_cell(const TuiTable* table, int row, int col) {
    if (!table || row < 0 || row >= table->row_count || col < 0 || col >= table->col_count) return NULL;
    return table->cells[row * table->col_count + col];
}

bool tui_table_handle_key(TuiTable* table, uint32_t key, const struct ncinput* ni) {
    if (!table || !ni || table->row_count == 0) return false;

    if (key == NCKEY_UP) {
        if (table->selected_row > 0) {
            tui_table_set_selected(table, table->selected_row - 1);
        }
        return true;
    }
    if (key == NCKEY_DOWN) {
        if (table->selected_row < table->row_count - 1) {
            tui_table_set_selected(table, table->selected_row + 1);
        }
        return true;
    }
    if (key == NCKEY_PGUP) {
        int page = table->height - 1;
        int new_row = table->selected_row - page;
        if (new_row < 0) new_row = 0;
        tui_table_set_selected(table, new_row);
        return true;
    }
    if (key == NCKEY_PGDOWN) {
        int page = table->height - 1;
        int new_row = table->selected_row + page;
        if (new_row >= table->row_count) new_row = table->row_count - 1;
        tui_table_set_selected(table, new_row);
        return true;
    }
    if (key == NCKEY_HOME) {
        tui_table_set_selected(table, 0);
        return true;
    }
    if (key == NCKEY_END) {
        tui_table_set_selected(table, table->row_count - 1);
        return true;
    }
    return false;
}

bool tui_table_handle_mouse(TuiTable* table, uint32_t key, const struct ncinput* ni) {
    if (!table || !ni || !table->plane) return false;
    if (key != NCKEY_BUTTON1) return false;

    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(table->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(table->plane, &rows, &cols);
    (void)cols;

    int local_y = ni->y - plane_abs_y;
    /* Row 0 is header, data starts at row 1 */
    if (local_y < 1) return false;

    int data_row = local_y - 1 + table->scroll_off;
    if (data_row >= 0 && data_row < table->row_count) {
        tui_table_set_selected(table, data_row);
        return true;
    }
    return false;
}

void tui_table_render(TuiTable* table) {
    if (!table || !table->plane) return;
    unsigned cols;
    ncplane_dim_yx(table->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, table->fg_text);
    ncchannels_set_bg_rgb(&base, table->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(table->plane, " ", 0, base);
    ncplane_erase(table->plane);

    /* Calculate column widths */
    int total_w = 0;
    for (int c = 0; c < table->col_count; c++) {
        int w = table->col_widths[c];
        if (w < 4) w = 4;
        table->col_widths[c] = w;
        total_w += w + 1; /* +1 for separator */
    }
    if (total_w > (int)cols) {
        /* Scale down */
        float scale = (float)(cols - 1) / (float)total_w;
        for (int c = 0; c < table->col_count; c++) {
            int w = (int)((float)table->col_widths[c] * scale);
            if (w < 3) w = 3;
            table->col_widths[c] = w;
        }
    }

    /* Draw header row */
    ncplane_set_bg_rgb(table->plane, table->bg_header);
    ncplane_set_fg_rgb(table->plane, table->fg_header);
    int x_pos = 0;
    for (int c = 0; c < table->col_count; c++) {
        const char* hdr = table->headers[c] ? table->headers[c] : "";
        int w = table->col_widths[c];
        int pad = w - (int)strlen(hdr);
        if (pad < 0) pad = 0;
        ncplane_putstr_yx(table->plane, 0, x_pos, hdr);
        for (int p = 0; p < pad; p++) ncplane_putstr_yx(table->plane, 0, x_pos + (int)strlen(hdr) + p, " ");
        x_pos += w + 1;
    }

    /* Draw separator under header — bulk write */
    ncplane_set_fg_rgb(table->plane, table->fg_separator);
    {
        char sep[1024];
        int n = (int)cols < 1023 ? (int)cols : 1023;
        memset(sep, '-', (size_t)n);
        sep[n] = '\0';
        ncplane_putstr_yx(table->plane, 1, 0, sep);
    }

    /* Draw data rows */
    int data_rows = table->height - 2;
    if (data_rows < 1) data_rows = 1;
    for (int row = 0; row < data_rows; row++) {
        int data_idx = table->scroll_off + row;
        if (data_idx >= table->row_count) break;

        bool is_selected = (data_idx == table->selected_row);

        if (is_selected) {
            ncplane_set_fg_rgb(table->plane, table->fg_selected);
            ncplane_set_bg_rgb(table->plane, table->bg_selected);
        } else if (data_idx % 2 == 1) {
            ncplane_set_fg_rgb(table->plane, table->fg_text);
            ncplane_set_bg_rgb(table->plane, table->bg_alt);
        } else {
            ncplane_set_fg_rgb(table->plane, table->fg_text);
            ncplane_set_bg_rgb(table->plane, table->bg_normal);
        }

        x_pos = 0;
        for (int c = 0; c < table->col_count; c++) {
            int idx = data_idx * table->col_count + c;
            const char* cell = table->cells[idx] ? table->cells[idx] : "";
            int w = table->col_widths[c];
            int len = (int)strlen(cell);
            if (len > w) len = w;
            ncplane_putstr(table->plane, cell);
            for (int p = len; p < w; p++) ncplane_putstr(table->plane, " ");
            x_pos += w + 1;
        }
    }
}

/* === VTable === */

static bool v_tbl_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_table_handle_key((TuiTable*)widget, key, ni);
}

static bool v_tbl_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_table_handle_mouse((TuiTable*)widget, key, ni);
}

static void v_tbl_render(void* widget) {
    tui_table_render((TuiTable*)widget);
}

static bool v_tbl_is_focusable(void* widget) { (void)widget; return true; }

static void v_tbl_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)widget; (void)max_h; (void)max_w;
    if (out_h) *out_h = 5;
    if (out_w) *out_w = 20;
}

static const TuiWidgetIface s_table_iface = {
    .handle_key = v_tbl_handle_key,
    .handle_mouse = v_tbl_handle_mouse,
    .render = v_tbl_render,
    .is_focusable = v_tbl_is_focusable,
    .preferred_size = v_tbl_preferred_size,
};

void tui_table_ensure_registered(void) {
    if (s_table_type_id < 0) {
        s_table_type_id = tui_widget_register(&s_table_iface);
    }
}

int tui_table_get_type_id(void) {
    tui_table_ensure_registered();
    return s_table_type_id;
}
