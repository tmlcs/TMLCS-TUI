/**
 * @file keymap.c
 * @brief Keymap system implementation.
 *
 * Maintains a table of key-to-action bindings with modifier support,
 * and a separate table of action-to-handler callbacks.
 */

#include "core/keymap.h"
#include "core/manager.h"
#include "core/types_private.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief A single key binding entry.
 */
typedef struct TuiKeyBinding {
    TuiAction action;
    uint32_t key;
    bool ctrl;
    bool alt;
    bool shift;
    TuiKeymapCallback custom_cb;
    void* custom_userdata;
} TuiKeyBinding;

#define MAX_KEY_BINDINGS 64

/**
 * @brief Internal keymap state attached to the manager.
 */
struct TuiKeymap {
    TuiKeyBinding bindings[MAX_KEY_BINDINGS];
    int binding_count;
    TuiKeymapCallback action_handlers[ACTION_COUNT];
};

void tui_keymap_init(TuiManager* mgr) {
    if (!mgr) return;
    mgr->_keymap = (struct TuiKeymap*)calloc(1, sizeof(struct TuiKeymap));
    if (!mgr->_keymap) return;

    /* Default bindings */
    tui_keymap_bind(mgr, ACTION_QUIT, 'q', false, false, false);
    tui_keymap_bind(mgr, ACTION_QUIT, 'Q', false, false, false);
    tui_keymap_bind(mgr, ACTION_NEW_TAB, 't', true, false, false);
    tui_keymap_bind(mgr, ACTION_NEW_WINDOW, 'n', true, false, false);
    tui_keymap_bind(mgr, ACTION_CLOSE_WINDOW, 'x', false, false, false);
    tui_keymap_bind(mgr, ACTION_CLOSE_TAB, 'w', false, false, false);
    tui_keymap_bind(mgr, ACTION_CLOSE_WORKSPACE, 'd', false, false, false);
    tui_keymap_bind(mgr, ACTION_NEXT_TAB, NCKEY_RIGHT, false, false, false);
    tui_keymap_bind(mgr, ACTION_PREV_TAB, NCKEY_LEFT, false, false, false);
    tui_keymap_bind(mgr, ACTION_NEXT_WINDOW, NCKEY_TAB, false, false, false);
    tui_keymap_bind(mgr, ACTION_PREV_WINDOW, NCKEY_TAB, false, false, true);
    tui_keymap_bind(mgr, ACTION_TOGGLE_HELP, NCKEY_F01, false, false, false);
    tui_keymap_bind(mgr, ACTION_TOGGLE_HELP, '?', false, false, false);
    tui_keymap_bind(mgr, ACTION_CANCEL, 0x1B, false, false, false);
}

void tui_keymap_bind(TuiManager* mgr, TuiAction action, uint32_t key, bool ctrl, bool alt, bool shift) {
    if (!mgr || !mgr->_keymap) return;
    if (mgr->_keymap->binding_count >= MAX_KEY_BINDINGS) return;

    TuiKeyBinding* b = &mgr->_keymap->bindings[mgr->_keymap->binding_count++];
    b->action = action;
    b->key = key;
    b->ctrl = ctrl;
    b->alt = alt;
    b->shift = shift;
    b->custom_cb = NULL;
    b->custom_userdata = NULL;
}

void tui_keymap_bind_custom(TuiManager* mgr, TuiAction action, TuiKeymapCallback cb, void* userdata) {
    if (!mgr || !mgr->_keymap || !cb) return;
    if (mgr->_keymap->binding_count >= MAX_KEY_BINDINGS) return;

    TuiKeyBinding* b = &mgr->_keymap->bindings[mgr->_keymap->binding_count++];
    b->action = action;
    b->key = 0;  /* Custom actions have no key */
    b->ctrl = false;
    b->alt = false;
    b->shift = false;
    b->custom_cb = cb;
    b->custom_userdata = userdata;
}

TuiAction tui_keymap_match(TuiManager* mgr, uint32_t key, bool ctrl, bool alt, bool shift) {
    if (!mgr || !mgr->_keymap) return ACTION_COUNT;

    for (int i = 0; i < mgr->_keymap->binding_count; i++) {
        TuiKeyBinding* b = &mgr->_keymap->bindings[i];
        if (b->key == key && b->ctrl == ctrl && b->alt == alt && b->shift == shift) {
            return b->action;
        }
    }
    return ACTION_COUNT;
}

void tui_keymap_set_handler(TuiManager* mgr, TuiAction action, TuiKeymapCallback handler) {
    if (!mgr || !mgr->_keymap || action < 0 || action >= ACTION_COUNT) return;
    mgr->_keymap->action_handlers[action] = handler;
}
