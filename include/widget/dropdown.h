#ifndef WIDGET_DROPDOWN_H
#define WIDGET_DROPDOWN_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiDropdownCb)(int selected, void* userdata);

/**
 * @brief A dropdown/popup selection list.
 */
typedef struct TuiDropdown {
    struct ncplane* plane;
    struct ncplane* popup_plane;
    char** items;
    int count;
    int capacity;
    int selected;
    int width;
    bool focused;
    bool open;
    TuiDropdownCb cb;
    void* userdata;
    unsigned fg_text;
    unsigned fg_selected;
    unsigned bg_normal;
    unsigned bg_selected;
    unsigned bg_popup;
    int _type_id;
} TuiDropdown;

/**
 * @brief Create a dropdown widget.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns (>= 4).
 * @param cb Callback on selection change. May be NULL.
 * @param userdata Passed to callback.
 * @return New dropdown, or NULL on failure.
 */
TuiDropdown* tui_dropdown_create(struct ncplane* parent, int y, int x, int width,
                                  TuiDropdownCb cb, void* userdata);

/**
 * @brief Destroy the dropdown.
 * @param dropdown Dropdown, or NULL.
 */
void tui_dropdown_destroy(TuiDropdown* dropdown);

/**
 * @brief Add an item to the dropdown.
 * @param dropdown Dropdown.
 * @param item Item text. Copied internally.
 * @return true on success.
 */
bool tui_dropdown_add_item(TuiDropdown* dropdown, const char* item);

/**
 * @brief Get the selected index.
 * @param dropdown Dropdown.
 * @return Selected index, or -1 if none.
 */
int tui_dropdown_get_selected(const TuiDropdown* dropdown);

/**
 * @brief Set the selected index.
 * @param dropdown Dropdown.
 * @param index New selection (clamped to valid range).
 */
void tui_dropdown_set_selected(TuiDropdown* dropdown, int index);

/**
 * @brief Handle a key event.
 * @param dropdown Dropdown.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_dropdown_handle_key(TuiDropdown* dropdown, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event.
 * @param dropdown Dropdown.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_dropdown_handle_mouse(TuiDropdown* dropdown, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the dropdown.
 * @param dropdown Dropdown, or NULL.
 */
void tui_dropdown_render(TuiDropdown* dropdown);

/**
 * @brief Set focus state.
 * @param dropdown Dropdown.
 * @param focused Focus state.
 */
void tui_dropdown_set_focused(TuiDropdown* dropdown, bool focused);

/**
 * @brief Get the item count.
 * @param dropdown Dropdown.
 * @return Number of items.
 */
int tui_dropdown_get_count(const TuiDropdown* dropdown);

/**
 * @brief Check if the dropdown popup is currently open.
 * @param dropdown Dropdown.
 * @return true if open.
 */
bool tui_dropdown_is_open(const TuiDropdown* dropdown);

#endif /* WIDGET_DROPDOWN_H */
