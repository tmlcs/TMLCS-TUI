#include "widget/file_picker.h"
#include "core/theme.h"
#include "core/widget.h"
#include <dirent.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void tui_file_picker_ensure_registered(void);
int tui_file_picker_get_type_id(void);

static int s_file_picker_type_id = -1;

static void ensure_capacity(TuiFilePicker* picker) {
    if (picker->count >= picker->capacity) {
        int new_cap = picker->capacity * 2;
        TuiFilePickerEntry* new_entries = (TuiFilePickerEntry*)realloc(picker->entries,
            (size_t)new_cap * sizeof(TuiFilePickerEntry));
        if (!new_entries) return;
        picker->entries = new_entries;
        picker->capacity = new_cap;
    }
}

static void clear_entries(TuiFilePicker* picker) {
    for (int i = 0; i < picker->count; i++) {
        free(picker->entries[i].name);
    }
    picker->count = 0;
}

/* Sort: directories first, then alphabetically */
static int compare_entries(const void* a, const void* b) {
    const TuiFilePickerEntry* ea = (const TuiFilePickerEntry*)a;
    const TuiFilePickerEntry* eb = (const TuiFilePickerEntry*)b;

    /* Directories first */
    if (ea->is_directory && !eb->is_directory) return -1;
    if (!ea->is_directory && eb->is_directory) return 1;

    return strcmp(ea->name, eb->name);
}

static int load_directory(TuiFilePicker* picker, const char* path) {
    if (!picker || !path) return -1;

    DIR* dir = opendir(path);
    if (!dir) return -1;

    clear_entries(picker);

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        ensure_capacity(picker);
        if (picker->count >= picker->capacity) {
            closedir(dir);
            return -1;
        }

        picker->entries[picker->count].name = strdup(entry->d_name);
        picker->entries[picker->count].is_directory = false;

        /* Determine if directory */
        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
        struct stat st;
        if (stat(fullpath, &st) == 0 && S_ISDIR(st.st_mode)) {
            picker->entries[picker->count].is_directory = true;
        }

        picker->count++;
    }
    closedir(dir);

    /* Sort entries */
    qsort(picker->entries, (size_t)picker->count, sizeof(TuiFilePickerEntry), compare_entries);

    picker->selected = picker->count > 0 ? 0 : -1;
    picker->scroll_off = 0;
    return 0;
}

TuiFilePicker* tui_file_picker_create(struct ncplane* parent, int y, int x,
                                       int width, int height,
                                       const char* initial_path,
                                       TuiFilePickerCb cb, void* userdata) {
    if (!parent || width < 10 || height < 3) return NULL;
    if (!initial_path) initial_path = ".";

    TuiFilePicker* picker = (TuiFilePicker*)calloc(1, sizeof(TuiFilePicker));
    if (!picker) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = (unsigned)height, .cols = (unsigned)width,
    };
    picker->plane = ncplane_create(parent, &opts);
    if (!picker->plane) { free(picker); return NULL; }

    picker->current_path = strdup(initial_path);
    picker->capacity = 32;
    picker->entries = (TuiFilePickerEntry*)calloc((size_t)picker->capacity, sizeof(TuiFilePickerEntry));
    if (!picker->entries) {
        free(picker->current_path);
        ncplane_destroy(picker->plane); free(picker); return NULL;
    }

    picker->count = 0;
    picker->selected = -1;
    picker->scroll_off = 0;
    picker->height = height;
    picker->width = width;
    picker->focused = false;
    picker->cb = cb;
    picker->userdata = userdata;
    picker->fg_dir = THEME_FG_TAB_ACTIVE;
    picker->fg_file = THEME_FG_DEFAULT;
    picker->fg_selected = THEME_FG_TAB_ACTIVE;
    picker->fg_path = THEME_FG_TIMESTAMP;
    picker->bg_normal = THEME_BG_DARKEST;
    picker->bg_selected = THEME_BG_TAB_ACTIVE;

    tui_file_picker_ensure_registered();
    picker->_type_id = s_file_picker_type_id;

    /* Load initial directory */
    load_directory(picker, initial_path);
    tui_file_picker_render(picker);
    return picker;
}

void tui_file_picker_destroy(TuiFilePicker* picker) {
    if (!picker) return;
    clear_entries(picker);
    free(picker->entries);
    free(picker->current_path);
    if (picker->plane) ncplane_destroy(picker->plane);
    free(picker);
}

bool tui_file_picker_navigate(TuiFilePicker* picker, const char* path) {
    if (!picker || !path) return false;
    free(picker->current_path);
    picker->current_path = strdup(path);
    if (load_directory(picker, path) != 0) return false;
    tui_file_picker_render(picker);
    return true;
}

bool tui_file_picker_go_up(TuiFilePicker* picker) {
    if (!picker) return false;

    /* Check if already at root */
    int len = (int)strlen(picker->current_path);
    if (len <= 1) return false;

    char* parent = strdup(picker->current_path);
    /* Remove trailing slash if present */
    if (parent[len - 1] == '/' && len > 1) parent[len - 1] = '\0';

    char* dir = dirname(parent);
    if (strcmp(dir, picker->current_path) == 0) {
        free(parent);
        return false; /* Already at root */
    }

    bool result = tui_file_picker_navigate(picker, dir);
    free(parent);
    return result;
}

const char* tui_file_picker_get_selected_name(const TuiFilePicker* picker) {
    if (!picker || picker->selected < 0 || picker->selected >= picker->count) return NULL;
    return picker->entries[picker->selected].name;
}

const char* tui_file_picker_get_path(const TuiFilePicker* picker) {
    return picker ? picker->current_path : NULL;
}

void tui_file_picker_set_focused(TuiFilePicker* picker, bool focused) {
    if (!picker) return;
    picker->focused = focused;
    tui_file_picker_render(picker);
}

int tui_file_picker_get_count(const TuiFilePicker* picker) {
    return picker ? picker->count : 0;
}

bool tui_file_picker_handle_key(TuiFilePicker* picker, uint32_t key, const struct ncinput* ni) {
    if (!picker || !ni) return false;

    if (key == NCKEY_UP) {
        if (picker->selected > 0) {
            picker->selected--;
            if (picker->selected < picker->scroll_off) {
                picker->scroll_off = picker->selected;
            }
            tui_file_picker_render(picker);
        }
        return true;
    }
    if (key == NCKEY_DOWN) {
        if (picker->selected < picker->count - 1) {
            picker->selected++;
            int data_rows = picker->height - 1; /* Minus path bar */
            if (data_rows < 1) data_rows = 1;
            if (picker->selected >= picker->scroll_off + data_rows) {
                picker->scroll_off = picker->selected - data_rows + 1;
            }
            tui_file_picker_render(picker);
        }
        return true;
    }
    if (key == NCKEY_ENTER) {
        /* Open directory or select file */
        if (picker->selected >= 0 && picker->selected < picker->count) {
            TuiFilePickerEntry* entry = &picker->entries[picker->selected];
            if (entry->is_directory) {
                char newpath[1024];
                snprintf(newpath, sizeof(newpath), "%s/%s", picker->current_path, entry->name);
                tui_file_picker_navigate(picker, newpath);
            } else {
                if (picker->cb) {
                    char fullpath[1024];
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", picker->current_path, entry->name);
                    picker->cb(fullpath, picker->userdata);
                }
            }
        }
        return true;
    }
    if (key == NCKEY_BACKSPACE || key == 127) {
        tui_file_picker_go_up(picker);
        return true;
    }
    return false;
}

bool tui_file_picker_handle_mouse(TuiFilePicker* picker, uint32_t key, const struct ncinput* ni) {
    if (!picker || !ni || !picker->plane) return false;
    if (key != NCKEY_BUTTON1) return false;

    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(picker->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(picker->plane, &rows, &cols);
    (void)cols;

    int local_y = ni->y - plane_abs_y;
    /* Row 0 is path bar, data starts at row 1 */
    if (local_y < 1) return false;

    int data_row = picker->scroll_off + (local_y - 1);
    if (data_row >= 0 && data_row < picker->count) {
        picker->selected = data_row;
        /* Double-click behavior: open dir or select file */
        TuiFilePickerEntry* entry = &picker->entries[data_row];
        if (entry->is_directory) {
            char newpath[1024];
            snprintf(newpath, sizeof(newpath), "%s/%s", picker->current_path, entry->name);
            tui_file_picker_navigate(picker, newpath);
        } else {
            if (picker->cb) {
                char fullpath[1024];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", picker->current_path, entry->name);
                picker->cb(fullpath, picker->userdata);
            }
        }
        return true;
    }
    return false;
}

void tui_file_picker_render(TuiFilePicker* picker) {
    if (!picker || !picker->plane) return;
    unsigned cols;
    ncplane_dim_yx(picker->plane, NULL, &cols);

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, picker->fg_file);
    ncchannels_set_bg_rgb(&base, picker->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(picker->plane, " ", 0, base);
    ncplane_erase(picker->plane);

    /* Draw path bar on row 0 */
    ncplane_set_fg_rgb(picker->plane, picker->fg_path);
    int path_len = (int)strlen(picker->current_path);
    int max_path = (int)cols - 2;
    if (max_path < 1) max_path = 1;
    int path_start = 1;
    if (path_len > max_path) {
        int skip = path_len - max_path + 2;
        ncplane_putstr_yx(picker->plane, 0, path_start, "..");
        ncplane_putstr_yx(picker->plane, 0, path_start + 2, picker->current_path + skip);
    } else {
        ncplane_putstr_yx(picker->plane, 0, path_start, picker->current_path);
    }
    /* Border under path */
    ncplane_set_fg_rgb(picker->plane, THEME_FG_SEPARATOR);
    for (unsigned i = 0; i < cols; i++) {
        ncplane_putstr_yx(picker->plane, 1, (int)i, "-");
    }

    /* Draw entries starting at row 2 */
    int data_rows = picker->height - 2;
    if (data_rows < 1) data_rows = 1;
    int start = picker->scroll_off;
    if (start < 0) start = 0;

    for (int row = 0; row < data_rows && (start + row) < picker->count; row++) {
        int entry_idx = start + row;
        TuiFilePickerEntry* entry = &picker->entries[entry_idx];
        bool is_selected = (entry_idx == picker->selected);
        int render_row = row + 2;

        if (is_selected) {
            ncplane_set_fg_rgb(picker->plane, picker->fg_selected);
            ncplane_set_bg_rgb(picker->plane, picker->bg_selected);
        } else {
            ncplane_set_fg_rgb(picker->plane, entry->is_directory ? picker->fg_dir : picker->fg_file);
            ncplane_set_bg_rgb(picker->plane, picker->bg_normal);
        }

        /* Draw directory indicator */
        if (entry->is_directory) {
            ncplane_putstr_yx(picker->plane, render_row, 1, "/");
        } else {
            ncplane_putstr_yx(picker->plane, render_row, 1, " ");
        }

        /* Draw name */
        int max_name = (int)cols - 2;
        if (max_name < 1) max_name = 1;
        int len = (int)strlen(entry->name);
        int copy = len < max_name ? len : max_name;
        char buf[512];
        memcpy(buf, entry->name, (size_t)copy);
        buf[copy] = '\0';
        ncplane_putstr_yx(picker->plane, render_row, 2, buf);
    }
}

/* === VTable === */

static bool v_fp_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_file_picker_handle_key((TuiFilePicker*)widget, key, ni);
}

static bool v_fp_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    return tui_file_picker_handle_mouse((TuiFilePicker*)widget, key, ni);
}

static void v_fp_render(void* widget) {
    tui_file_picker_render((TuiFilePicker*)widget);
}

static bool v_fp_is_focusable(void* widget) { (void)widget; return true; }

static void v_fp_preferred_size(void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    (void)widget; (void)max_h; (void)max_w;
    if (out_h) *out_h = 10;
    if (out_w) *out_w = 40;
}

static const TuiWidgetIface s_file_picker_iface = {
    .handle_key = v_fp_handle_key,
    .handle_mouse = v_fp_handle_mouse,
    .render = v_fp_render,
    .is_focusable = v_fp_is_focusable,
    .preferred_size = v_fp_preferred_size,
};

void tui_file_picker_ensure_registered(void) {
    if (s_file_picker_type_id < 0) {
        s_file_picker_type_id = tui_widget_register(&s_file_picker_iface);
    }
}

int tui_file_picker_get_type_id(void) {
    tui_file_picker_ensure_registered();
    return s_file_picker_type_id;
}
