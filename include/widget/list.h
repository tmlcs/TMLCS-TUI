#ifndef WIDGET_LIST_H
#define WIDGET_LIST_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

/**
 * @brief A scrollable list widget with selection.
 */
typedef struct TuiList {
    struct ncplane* plane;
    char** items;
    int count;
    int capacity;
    int selected;
    int scroll_off;
    int height;
    bool focused;
    unsigned bg_normal;
    unsigned bg_selected;
    unsigned fg_text;
    unsigned fg_selected;
    int _type_id;
} TuiList;

/**
 * @brief Create a list widget.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns.
 * @param height Height in rows (visible items).
 * @return New list, or NULL on failure.
 */
TuiList* tui_list_create(struct ncplane* parent, int y, int x, int width, int height);

/**
 * @brief Destroy the list.
 * @param list List, or NULL.
 */
void tui_list_destroy(TuiList* list);

/**
 * @brief Add an item to the list.
 * @param list List.
 * @param item Text to add. Copied internally.
 */
void tui_list_add_item(TuiList* list, const char* item);

/**
 * @brief Remove an item by index.
 * @param list List.
 * @param index Item index. Out-of-bounds is a no-op.
 */
void tui_list_remove_item(TuiList* list, int index);

/**
 * @brief Clear all items.
 * @param list List.
 */
void tui_list_clear(TuiList* list);

/**
 * @brief Get the selected index.
 * @param list List.
 * @return Selected index, or -1 if empty.
 */
int tui_list_get_selected(const TuiList* list);

/**
 * @brief Set the selected index.
 * @param list List.
 * @param index New selection. Clamped to valid range.
 */
void tui_list_set_selected(TuiList* list, int index);

/**
 * @brief Handle a key event.
 * @param list List.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_list_handle_key(TuiList* list, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the list.
 * @param list List, or NULL.
 */
void tui_list_render(TuiList* list);

/**
 * @brief Get the number of items.
 * @param list List.
 * @return Item count.
 */
int tui_list_get_count(const TuiList* list);

/**
 * @brief Set focus state.
 * @param list List.
 * @param focused Focus state.
 */
void tui_list_set_focused(TuiList* list, bool focused);

/**
 * @brief Handle a mouse event.
 * @param list List.
 * @param key Notcurses key code (NCKEY_BUTTON1, etc.).
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_list_handle_mouse(TuiList* list, uint32_t key, const struct ncinput* ni);

#endif /* WIDGET_LIST_H */
