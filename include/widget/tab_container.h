#ifndef WIDGET_TAB_CONTAINER_H
#define WIDGET_TAB_CONTAINER_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiTabContainerCb)(int tab_index, void* userdata);

/**
 * @brief A tabbed container widget.
 */
typedef struct TuiTabContainer {
    struct ncplane* plane;
    char** tab_labels;
    int tab_count;
    int tab_capacity;
    int active_tab;
    int width;
    int height;
    bool focused;
    TuiTabContainerCb cb;
    void* userdata;
    unsigned fg_active;
    unsigned fg_inactive;
    unsigned fg_separator;
    unsigned bg_normal;
    unsigned bg_active;
    int _type_id;
} TuiTabContainer;

/**
 * @brief Create a tab container.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns.
 * @param height Height in rows (>= 3).
 * @param cb Callback on tab change. May be NULL.
 * @param userdata Passed to callback.
 * @return New tab container, or NULL on failure.
 */
TuiTabContainer* tui_tab_container_create(struct ncplane* parent, int y, int x,
                                           int width, int height,
                                           TuiTabContainerCb cb, void* userdata);

/**
 * @brief Destroy the tab container.
 * @param container Tab container, or NULL.
 */
void tui_tab_container_destroy(TuiTabContainer* container);

/**
 * @brief Add a tab.
 * @param container Tab container.
 * @param label Tab label. Copied internally.
 * @return true on success.
 */
bool tui_tab_container_add_tab(TuiTabContainer* container, const char* label);

/**
 * @brief Get the active tab index.
 * @param container Tab container.
 * @return Active tab index, or -1 if no tabs.
 */
int tui_tab_container_get_active(const TuiTabContainer* container);

/**
 * @brief Set the active tab.
 * @param container Tab container.
 * @param index Tab index (clamped to valid range).
 */
void tui_tab_container_set_active(TuiTabContainer* container, int index);

/**
 * @brief Handle a key event.
 * @param container Tab container.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_tab_container_handle_key(TuiTabContainer* container, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event.
 * @param container Tab container.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_tab_container_handle_mouse(TuiTabContainer* container, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the tab container.
 * @param container Tab container, or NULL.
 */
void tui_tab_container_render(TuiTabContainer* container);

/**
 * @brief Set focus state.
 * @param container Tab container.
 * @param focused Focus state.
 */
void tui_tab_container_set_focused(TuiTabContainer* container, bool focused);

/**
 * @brief Get the tab count.
 * @param container Tab container.
 * @return Number of tabs.
 */
int tui_tab_container_get_count(const TuiTabContainer* container);

#endif /* WIDGET_TAB_CONTAINER_H */
