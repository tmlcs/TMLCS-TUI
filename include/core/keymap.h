/**
 * @file keymap.h
 * @brief Configurable keybinding system for tmlcs-tui.
 *
 * Maps key combinations to logical actions with support for
 * Ctrl, Alt, and Shift modifiers. Provides default bindings
 * and allows custom callback registration per action.
 */

#ifndef CORE_KEYMAP_H
#define CORE_KEYMAP_H

#include "core/types.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Logical actions that can be bound to keys.
 *
 * Each action represents a user intent independent of the
 * specific key combination that triggers it.
 */
typedef enum {
    ACTION_QUIT,             /**< Exit the application */
    ACTION_NEW_TAB,          /**< Create a new tab */
    ACTION_NEW_WINDOW,       /**< Create a new window in the active tab */
    ACTION_CLOSE_WINDOW,     /**< Close the focused window */
    ACTION_CLOSE_TAB,        /**< Close the active tab */
    ACTION_CLOSE_WORKSPACE,  /**< Close the active workspace */
    ACTION_NEXT_TAB,         /**< Switch to the next tab */
    ACTION_PREV_TAB,         /**< Switch to the previous tab */
    ACTION_NEXT_WINDOW,      /**< Focus the next window */
    ACTION_PREV_WINDOW,      /**< Focus the previous window */
    ACTION_RESIZE_UP,        /**< Increase window height */
    ACTION_RESIZE_DOWN,      /**< Decrease window height */
    ACTION_RESIZE_LEFT,      /**< Decrease window width */
    ACTION_RESIZE_RIGHT,     /**< Increase window width */
    ACTION_TOGGLE_HELP,      /**< Show/hide the help overlay */
    ACTION_CANCEL,           /**< Cancel current operation / dismiss overlay */
    ACTION_COUNT             /**< Number of actions (sentinel) */
} TuiAction;

/**
 * @brief Callback signature for keymap action handlers.
 * @param mgr Manager instance.
 * @param ni Input details from notcurses.
 * @param userdata Opaque pointer set at binding time.
 */
typedef void (*TuiKeymapCallback)(TuiManager* mgr, const struct ncinput* ni, void* userdata);

/**
 * @brief Initialize the keymap with default bindings.
 * @param mgr Manager instance. Must not be NULL.
 */
void tui_keymap_init(TuiManager* mgr);

/**
 * @brief Bind a key combination to a logical action.
 * @param mgr Manager instance.
 * @param action The logical action to trigger.
 * @param key Notcurses key code (character or NCKEY_*).
 * @param ctrl Require Ctrl modifier.
 * @param alt Require Alt modifier.
 * @param shift Require Shift modifier.
 */
void tui_keymap_bind(TuiManager* mgr, TuiAction action, uint32_t key, bool ctrl, bool alt, bool shift);

/**
 * @brief Register a custom callback for an action.
 * @param mgr Manager instance.
 * @param action The logical action to trigger.
 * @param cb Callback function (must not be NULL).
 * @param userdata Passed to the callback when invoked.
 */
void tui_keymap_bind_custom(TuiManager* mgr, TuiAction action, TuiKeymapCallback cb, void* userdata);

/**
 * @brief Look up the action for a given key combination.
 * @param mgr Manager instance.
 * @param key Notcurses key code.
 * @param ctrl Ctrl modifier state.
 * @param alt Alt modifier state.
 * @param shift Shift modifier state.
 * @return Matching TuiAction, or ACTION_COUNT if no binding found.
 */
TuiAction tui_keymap_match(TuiManager* mgr, uint32_t key, bool ctrl, bool alt, bool shift);

/**
 * @brief Set the handler callback for a specific action.
 * @param mgr Manager instance.
 * @param action The action whose handler to set.
 * @param handler Callback function.
 */
void tui_keymap_set_handler(TuiManager* mgr, TuiAction action, TuiKeymapCallback handler);

#endif /* CORE_KEYMAP_H */
