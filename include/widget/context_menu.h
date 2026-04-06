#ifndef WIDGET_CONTEXT_MENU_H
#define WIDGET_CONTEXT_MENU_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

#define CONTEXT_MENU_MAX_ITEMS 16

typedef void (*TuiMenuCb)(void* userdata);

/**
 * @brief A right-click context menu widget.
 */
typedef struct TuiContextMenu {
    struct ncplane* plane;
    char* items[CONTEXT_MENU_MAX_ITEMS];
    TuiMenuCb callbacks[CONTEXT_MENU_MAX_ITEMS];
    void* userdatas[CONTEXT_MENU_MAX_ITEMS];
    int count;
    int selected;
    bool visible;
    int width;
    int height;
    unsigned bg_normal;
    unsigned bg_selected;
    unsigned fg_text;
    unsigned fg_selected;
    int _type_id;
} TuiContextMenu;

/**
 * @brief Create a context menu (initially hidden).
 * @param parent Parent ncplane. Must not be NULL.
 * @param y Y position within the parent plane.
 * @param x X position within the parent plane.
 * @param width Menu width in columns. Must be ≥ 10.
 * @return New context menu instance, or NULL on failure.
 */
TuiContextMenu* tui_menu_create(struct ncplane* parent, int y, int x, int width);

/**
 * @brief Destroy the context menu and all item label strings.
 * @param menu Context menu instance, or NULL (safe no-op).
 */
void tui_menu_destroy(TuiContextMenu* menu);

/**
 * @brief Add a selectable item to the menu.
 * @param menu Context menu instance. Must not be NULL.
 * @param label Display text for the item. Copied internally. Must not be NULL.
 * @param cb Callback invoked when the item is activated. May be NULL.
 * @param userdata Pointer passed to the callback. May be NULL.
 * @return true if the item was added, false if CONTEXT_MENU_MAX_ITEMS (16) reached.
 */
bool tui_menu_add_item(TuiContextMenu* menu, const char* label, TuiMenuCb cb, void* userdata);

/**
 * @brief Make the menu visible and bring it to the front of the z-order.
 *        Resets selection to the first item.
 * @param menu Context menu instance, or NULL.
 */
void tui_menu_show(TuiContextMenu* menu);

/**
 * @brief Hide the menu without destroying it. Can be shown again with tui_menu_show().
 * @param menu Context menu instance, or NULL.
 */
void tui_menu_hide(TuiContextMenu* menu);

/**
 * @brief Check if the menu is currently visible.
 * @param menu Context menu instance, or NULL.
 * @return true if visible, false otherwise.
 */
bool tui_menu_is_visible(const TuiContextMenu* menu);

/**
 * @brief Process a keyboard event for menu navigation.
 *         UP/DOWN move selection, ENTER activates, ESC hides.
 * @param menu Context menu instance, or NULL.
 * @param key Notcurses key code (NCKEY_UP, NCKEY_DOWN, NCKEY_ENTER, ESC, etc.).
 * @param ni Input details (event type, modifiers).
 * @return true if the event was consumed by the menu.
 */
bool tui_menu_handle_key(TuiContextMenu* menu, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the menu items with selection highlighting.
 *        Only renders if the menu is visible.
 * @param menu Context menu instance, or NULL.
 */
void tui_menu_render(TuiContextMenu* menu);

/**
 * @brief Handle a mouse event on the menu.
 *        Click selects and activates the item under the cursor, then hides the menu.
 * @param menu Context menu instance, or NULL.
 * @param key Notcurses key code (NCKEY_BUTTON1, etc.).
 * @param ni Input details (event type, absolute coordinates).
 * @return true if the event was consumed.
 */
bool tui_menu_handle_mouse(TuiContextMenu* menu, uint32_t key, const struct ncinput* ni);

#endif /* WIDGET_CONTEXT_MENU_H */
