#ifndef WIDGET_RADIO_GROUP_H
#define WIDGET_RADIO_GROUP_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiRadioGroupCb)(int selected, void* userdata);

/**
 * @brief A mutual-exclusion radio button group.
 */
typedef struct TuiRadioGroup {
    struct ncplane* plane;
    char** labels;
    int count;
    int capacity;
    int selected;
    int height;
    int width;
    bool focused;
    TuiRadioGroupCb cb;
    void* userdata;
    unsigned fg_selected;
    unsigned fg_unselected;
    unsigned fg_label;
    unsigned bg_normal;
    unsigned bg_selected;
    int _type_id;
} TuiRadioGroup;

/**
 * @brief Create a radio group.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns.
 * @param height Height in rows.
 * @param cb Callback on selection change. May be NULL.
 * @param userdata Passed to callback.
 * @return New radio group, or NULL on failure.
 */
TuiRadioGroup* tui_radio_group_create(struct ncplane* parent, int y, int x,
                                       int width, int height,
                                       TuiRadioGroupCb cb, void* userdata);

/**
 * @brief Destroy the radio group.
 * @param group Radio group, or NULL.
 */
void tui_radio_group_destroy(TuiRadioGroup* group);

/**
 * @brief Add an option to the group.
 * @param group Radio group.
 * @param label Option label. Copied internally.
 * @return true on success.
 */
bool tui_radio_group_add_option(TuiRadioGroup* group, const char* label);

/**
 * @brief Get the selected index.
 * @param group Radio group.
 * @return Selected index, or -1 if none.
 */
int tui_radio_group_get_selected(const TuiRadioGroup* group);

/**
 * @brief Set the selected index.
 * @param group Radio group.
 * @param index New selection (clamped to valid range).
 */
void tui_radio_group_set_selected(TuiRadioGroup* group, int index);

/**
 * @brief Handle a key event.
 * @param group Radio group.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_radio_group_handle_key(TuiRadioGroup* group, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event.
 * @param group Radio group.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_radio_group_handle_mouse(TuiRadioGroup* group, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the radio group.
 * @param group Radio group, or NULL.
 */
void tui_radio_group_render(TuiRadioGroup* group);

/**
 * @brief Set focus state.
 * @param group Radio group.
 * @param focused Focus state.
 */
void tui_radio_group_set_focused(TuiRadioGroup* group, bool focused);

/**
 * @brief Get the number of options.
 * @param group Radio group.
 * @return Option count.
 */
int tui_radio_group_get_count(const TuiRadioGroup* group);

#endif /* WIDGET_RADIO_GROUP_H */
