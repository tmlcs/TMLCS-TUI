#ifndef WIDGET_CHECKBOX_H
#define WIDGET_CHECKBOX_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiCheckboxCb)(bool checked, void* userdata);

/**
 * @brief A checkbox/toggle widget.
 */
typedef struct TuiCheckbox {
    struct ncplane* plane;
    char* label;
    bool checked;
    bool focused;
    TuiCheckboxCb cb;
    void* userdata;
    unsigned fg_check;
    unsigned fg_unchecked;
    unsigned fg_label;
    unsigned bg_normal;
    unsigned bg_focused;
    int _type_id;
} TuiCheckbox;

/**
 * @brief Create a checkbox.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param label Label text.
 * @param checked Initial checked state.
 * @param cb Callback on state change. May be NULL.
 * @param userdata Passed to callback.
 * @return New checkbox, or NULL on failure.
 */
TuiCheckbox* tui_checkbox_create(struct ncplane* parent, int y, int x,
                                  const char* label, bool checked,
                                  TuiCheckboxCb cb, void* userdata);

/**
 * @brief Destroy the checkbox.
 * @param cb Checkbox, or NULL.
 */
void tui_checkbox_destroy(TuiCheckbox* cb);

/**
 * @brief Get the checked state.
 * @param cb Checkbox.
 * @return true if checked.
 */
bool tui_checkbox_get_state(const TuiCheckbox* cb);

/**
 * @brief Set the checked state.
 * @param cb Checkbox.
 * @param checked New state.
 */
void tui_checkbox_set_state(TuiCheckbox* cb, bool checked);

/**
 * @brief Handle a key event.
 * @param cb Checkbox.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_checkbox_handle_key(TuiCheckbox* cb, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the checkbox.
 * @param cb Checkbox, or NULL.
 */
void tui_checkbox_render(TuiCheckbox* cb);

/**
 * @brief Set focus state.
 * @param cb Checkbox.
 * @param focused Focus state.
 */
void tui_checkbox_set_focused(TuiCheckbox* cb, bool focused);

/**
 * @brief Handle a mouse event.
 * @param cb Checkbox.
 * @param key Notcurses key code (NCKEY_BUTTON1, etc.).
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_checkbox_handle_mouse(TuiCheckbox* cb, uint32_t key, const struct ncinput* ni);

#endif /* WIDGET_CHECKBOX_H */
