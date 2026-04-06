#ifndef WIDGET_FILE_PICKER_H
#define WIDGET_FILE_PICKER_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiFilePickerCb)(const char* path, void* userdata);

/**
 * @brief A directory navigation file picker widget.
 */
typedef struct TuiFilePickerEntry {
    char* name;
    bool is_directory;
} TuiFilePickerEntry;

typedef struct TuiFilePicker {
    struct ncplane* plane;
    char* current_path;
    TuiFilePickerEntry* entries;
    int count;
    int capacity;
    int selected;
    int scroll_off;
    int height;
    int width;
    bool focused;
    TuiFilePickerCb cb;
    void* userdata;
    unsigned fg_dir;
    unsigned fg_file;
    unsigned fg_selected;
    unsigned fg_path;
    unsigned bg_normal;
    unsigned bg_selected;
    int _type_id;
} TuiFilePicker;

/**
 * @brief Create a file picker widget.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns.
 * @param height Height in rows.
 * @param initial_path Starting directory.
 * @param cb Callback on file selection. May be NULL.
 * @param userdata Passed to callback.
 * @return New file picker, or NULL on failure.
 */
TuiFilePicker* tui_file_picker_create(struct ncplane* parent, int y, int x,
                                       int width, int height,
                                       const char* initial_path,
                                       TuiFilePickerCb cb, void* userdata);

/**
 * @brief Destroy the file picker.
 * @param picker File picker, or NULL.
 */
void tui_file_picker_destroy(TuiFilePicker* picker);

/**
 * @brief Navigate to a directory.
 * @param picker File picker.
 * @param path Directory path.
 * @return true on success.
 */
bool tui_file_picker_navigate(TuiFilePicker* picker, const char* path);

/**
 * @brief Go to the parent directory.
 * @param picker File picker.
 * @return true on success.
 */
bool tui_file_picker_go_up(TuiFilePicker* picker);

/**
 * @brief Get the currently selected entry name.
 * @param picker File picker.
 * @return Entry name, or NULL.
 */
const char* tui_file_picker_get_selected_name(const TuiFilePicker* picker);

/**
 * @brief Get the current directory path.
 * @param picker File picker.
 * @return Current path string (do not free).
 */
const char* tui_file_picker_get_path(const TuiFilePicker* picker);

/**
 * @brief Handle a key event.
 * @param picker File picker.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_file_picker_handle_key(TuiFilePicker* picker, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event.
 * @param picker File picker.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_file_picker_handle_mouse(TuiFilePicker* picker, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the file picker.
 * @param picker File picker, or NULL.
 */
void tui_file_picker_render(TuiFilePicker* picker);

/**
 * @brief Set focus state.
 * @param picker File picker.
 * @param focused Focus state.
 */
void tui_file_picker_set_focused(TuiFilePicker* picker, bool focused);

/**
 * @brief Get the entry count.
 * @param picker File picker.
 * @return Number of entries.
 */
int tui_file_picker_get_count(const TuiFilePicker* picker);

#endif /* WIDGET_FILE_PICKER_H */
