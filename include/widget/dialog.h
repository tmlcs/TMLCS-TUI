#ifndef WIDGET_DIALOG_H
#define WIDGET_DIALOG_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiDialogCb)(int button_index, void* userdata);

/**
 * @brief A modal dialog with title, message, and buttons.
 */
typedef struct TuiDialog {
    struct ncplane* plane;
    char* title;
    char* message;
    char** buttons;
    int button_count;
    int button_capacity;
    int active_button;
    int width;
    int height;
    bool focused;
    TuiDialogCb cb;
    void* userdata;
    unsigned fg_title;
    unsigned fg_message;
    unsigned fg_button_active;
    unsigned fg_button_inactive;
    unsigned bg_normal;
    unsigned bg_button_active;
    unsigned border_color;
    int _type_id;
} TuiDialog;

/**
 * @brief Create a dialog widget.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Dialog width.
 * @param height Dialog height.
 * @param title Dialog title. Copied internally.
 * @param message Dialog message. Copied internally.
 * @param cb Callback on button activation. May be NULL.
 * @param userdata Passed to callback.
 * @return New dialog, or NULL on failure.
 */
TuiDialog* tui_dialog_create(struct ncplane* parent, int y, int x,
                              int width, int height,
                              const char* title, const char* message,
                              TuiDialogCb cb, void* userdata);

/**
 * @brief Destroy the dialog.
 * @param dialog Dialog, or NULL.
 */
void tui_dialog_destroy(TuiDialog* dialog);

/**
 * @brief Add a button to the dialog.
 * @param dialog Dialog.
 * @param label Button label. Copied internally.
 * @return true on success.
 */
bool tui_dialog_add_button(TuiDialog* dialog, const char* label);

/**
 * @brief Handle a key event.
 * @param dialog Dialog.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_dialog_handle_key(TuiDialog* dialog, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event.
 * @param dialog Dialog.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_dialog_handle_mouse(TuiDialog* dialog, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the dialog.
 * @param dialog Dialog, or NULL.
 */
void tui_dialog_render(TuiDialog* dialog);

/**
 * @brief Get the currently active button index.
 * @param dialog Dialog.
 * @return Button index, or -1.
 */
int tui_dialog_get_active_button(const TuiDialog* dialog);

/**
 * @brief Set the active button.
 * @param dialog Dialog.
 * @param index Button index.
 */
void tui_dialog_set_active_button(TuiDialog* dialog, int index);

/**
 * @brief Get the button count.
 * @param dialog Dialog.
 * @return Number of buttons.
 */
int tui_dialog_get_button_count(const TuiDialog* dialog);

#endif /* WIDGET_DIALOG_H */
