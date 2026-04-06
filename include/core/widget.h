/**
 * @file widget.h
 * @brief Polymorphic VTable-based widget dispatch system.
 *
 * Each widget type registers a TuiWidgetIface containing function pointers
 * for key handling, mouse handling, rendering, and focus management.
 * The core dispatch system routes input events without a giant switch.
 */

#ifndef CORE_WIDGET_H
#define CORE_WIDGET_H

#include "core/types.h"
#include <notcurses/notcurses.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief VTable interface for widget types.
 *
 * All function pointers receive the widget instance as a void* and
 * the registered type_id. Implementations cast to their concrete type.
 */
typedef struct TuiWidgetIface TuiWidgetIface;

struct TuiWidgetIface {
    bool (*handle_key)(void* widget, uint32_t key, const struct ncinput* ni);
    bool (*handle_mouse)(void* widget, uint32_t key, const struct ncinput* ni);
    void (*render)(void* widget);
    void (*on_focus)(void* widget);
    void (*on_blur)(void* widget);
    void (*on_resize)(void* widget, int new_height, int new_width);
    bool (*is_focusable)(void* widget);
    bool (*needs_periodic_render)(void* widget);
    void (*preferred_size)(void* widget, int max_h, int max_w, int* out_h, int* out_w);
    bool (*is_enabled)(void* widget);
    void (*set_enabled)(void* widget, bool enabled);
    bool (*is_read_only)(void* widget);
    void (*set_read_only)(void* widget, bool read_only);
};

/* ---- Registry ---- */

/**
 * @brief Register a widget VTable interface.
 * @param iface Pointer to a static const TuiWidgetIface. Must not be NULL.
 * @return Registered type_id (>= 0), or -1 on failure.
 */
int tui_widget_register(const TuiWidgetIface* iface);

/* ---- Dispatch ---- */

/**
 * @brief Dispatch a key event to the widget's VTable.
 * @param type_id Registered widget type_id.
 * @param widget Widget instance pointer.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if the event was consumed.
 */
bool tui_widget_handle_key(int type_id, void* widget, uint32_t key, const struct ncinput* ni);

/**
 * @brief Dispatch a mouse event to the widget's VTable.
 * @param type_id Registered widget type_id.
 * @param widget Widget instance pointer.
 * @param key Notcurses key code (button id).
 * @param ni Input details (contains absolute coordinates).
 * @return true if the event was consumed.
 */
bool tui_widget_handle_mouse(int type_id, void* widget, uint32_t key, const struct ncinput* ni);

/**
 * @brief Get the VTable interface for a type_id.
 * @param type_id Registered widget type_id.
 * @return The TuiWidgetIface pointer, or NULL if type_id is invalid.
 */
const TuiWidgetIface* tui_widget_get_iface(int type_id);

/**
 * @brief Render a widget via its VTable.
 * @param type_id Registered widget type_id.
 * @param widget Widget instance pointer.
 */
void tui_widget_render(int type_id, void* widget);

/**
 * @brief Check if a widget can receive keyboard focus.
 * @param type_id Registered widget type_id.
 * @param widget Widget instance pointer.
 * @return true if focusable.
 */
bool tui_widget_is_focusable(int type_id, void* widget);

/**
 * @brief Query a widget's preferred dimensions.
 * @param type_id Registered widget type_id.
 * @param widget Widget instance pointer.
 * @param max_h Maximum available height (-1 for unbounded).
 * @param max_w Maximum available width (-1 for unbounded).
 * @param out_h Output for preferred height. May be NULL.
 * @param out_w Output for preferred width. May be NULL.
 */
void tui_widget_preferred_size(int type_id, void* widget, int max_h, int max_w, int* out_h, int* out_w);

/**
 * @brief Check if a widget is currently enabled.
 * @param type_id Registered widget type_id.
 * @param widget Widget instance pointer.
 * @return true if enabled (defaults to true if not implemented).
 */
bool tui_widget_is_enabled(int type_id, void* widget);

/**
 * @brief Check if a widget requires periodic re-rendering (e.g., animations).
 * @param type_id Registered widget type_id.
 * @param widget Widget instance pointer.
 * @return true if periodic render is needed (defaults to false).
 */
bool tui_widget_needs_periodic_render(int type_id, void* widget);

#endif /* CORE_WIDGET_H */
