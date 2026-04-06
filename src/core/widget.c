#include "core/widget.h"
#include "core/logger.h"
#include <stdlib.h>

#define MAX_WIDGET_TYPES 32

static const TuiWidgetIface* s_ifaces[MAX_WIDGET_TYPES] = {0};
static int s_type_count = 0;

int tui_widget_register(const TuiWidgetIface* iface) {
    if (!iface || s_type_count >= MAX_WIDGET_TYPES) {
        tui_log(LOG_ERROR, "tui_widget_register: failed (limit reached or NULL iface)");
        return -1;
    }
    int id = s_type_count++;
    s_ifaces[id] = iface;
    return id;
}

const TuiWidgetIface* tui_widget_get_iface(int type_id) {
    if (type_id < 0 || type_id >= s_type_count) return NULL;
    return s_ifaces[type_id];
}

bool tui_widget_handle_key(int type_id, void* widget, uint32_t key, const struct ncinput* ni) {
    const TuiWidgetIface* iface = tui_widget_get_iface(type_id);
    if (!iface || !iface->handle_key) return false;
    return iface->handle_key(widget, key, ni);
}

bool tui_widget_handle_mouse(int type_id, void* widget, uint32_t key, const struct ncinput* ni) {
    const TuiWidgetIface* iface = tui_widget_get_iface(type_id);
    if (!iface || !iface->handle_mouse) return false;
    return iface->handle_mouse(widget, key, ni);
}

void tui_widget_render(int type_id, void* widget) {
    const TuiWidgetIface* iface = tui_widget_get_iface(type_id);
    if (!iface || !iface->render) return;
    iface->render(widget);
}

bool tui_widget_is_focusable(int type_id, void* widget) {
    const TuiWidgetIface* iface = tui_widget_get_iface(type_id);
    if (!iface || !iface->is_focusable) return false;
    return iface->is_focusable(widget);
}

void tui_widget_preferred_size(int type_id, void* widget, int max_h, int max_w, int* out_h, int* out_w) {
    if (out_h) *out_h = -1;
    if (out_w) *out_w = -1;
    const TuiWidgetIface* iface = tui_widget_get_iface(type_id);
    if (iface && iface->preferred_size) {
        iface->preferred_size(widget, max_h, max_w, out_h, out_w);
    }
}

bool tui_widget_is_enabled(int type_id, void* widget) {
    const TuiWidgetIface* iface = tui_widget_get_iface(type_id);
    if (!iface || !iface->is_enabled) return true;
    return iface->is_enabled(widget);
}

bool tui_widget_needs_periodic_render(int type_id, void* widget) {
    const TuiWidgetIface* iface = tui_widget_get_iface(type_id);
    if (!iface || !iface->needs_periodic_render) return false;
    return iface->needs_periodic_render(widget);
}
