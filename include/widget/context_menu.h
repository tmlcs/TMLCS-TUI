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
} TuiContextMenu;

/**
 * @brief Create a context menu (initially hidden).
 */
TuiContextMenu* tui_menu_create(struct ncplane* parent, int y, int x, int width);

/**
 * @brief Destroy the context menu.
 */
void tui_menu_destroy(TuiContextMenu* menu);

/**
 * @brief Add a menu item.
 */
bool tui_menu_add_item(TuiContextMenu* menu, const char* label, TuiMenuCb cb, void* userdata);

/**
 * @brief Show the menu.
 */
void tui_menu_show(TuiContextMenu* menu);

/**
 * @brief Hide the menu.
 */
void tui_menu_hide(TuiContextMenu* menu);

/**
 * @brief Check if menu is visible.
 */
bool tui_menu_is_visible(const TuiContextMenu* menu);

/**
 * @brief Handle a key event.
 */
bool tui_menu_handle_key(TuiContextMenu* menu, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the menu.
 */
void tui_menu_render(TuiContextMenu* menu);

#endif /* WIDGET_CONTEXT_MENU_H */
