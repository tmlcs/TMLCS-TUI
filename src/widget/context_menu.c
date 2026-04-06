#include "widget/context_menu.h"
#include "core/theme.h"
#include "core/widget.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
void tui_menu_ensure_registered(void);
int tui_menu_get_type_id(void);

static int s_menu_type_id = -1;

TuiContextMenu* tui_menu_create(struct ncplane* parent, int y, int x, int width) {
    if (!parent || width < 10) return NULL;
    TuiContextMenu* menu = (TuiContextMenu*)calloc(1, sizeof(TuiContextMenu));
    if (!menu) return NULL;

    struct ncplane_options opts = {
        .y = y, .x = x,
        .rows = 1, .cols = (unsigned)width,
    };
    menu->plane = ncplane_create(parent, &opts);
    if (!menu->plane) { free(menu); return NULL; }

    menu->count = 0;
    menu->selected = 0;
    menu->visible = false;
    menu->width = width;
    menu->height = 0;
    menu->bg_normal = THEME_BG_WINDOW;
    menu->bg_selected = THEME_BG_TAB_ACTIVE;
    menu->fg_text = THEME_FG_DEFAULT;
    menu->fg_selected = THEME_FG_TAB_ACTIVE;

    tui_menu_ensure_registered();
    menu->_type_id = s_menu_type_id;

    return menu;
}

void tui_menu_destroy(TuiContextMenu* menu) {
    if (!menu) return;
    for (int i = 0; i < menu->count; i++) free(menu->items[i]);
    if (menu->plane) ncplane_destroy(menu->plane);
    free(menu);
}

bool tui_menu_add_item(TuiContextMenu* menu, const char* label, TuiMenuCb cb, void* userdata) {
    if (!menu || !label || menu->count >= CONTEXT_MENU_MAX_ITEMS) return false;
    menu->items[menu->count] = strdup(label);
    menu->callbacks[menu->count] = cb;
    menu->userdatas[menu->count] = userdata;
    menu->count++;
    menu->height = menu->count;
    return true;
}

void tui_menu_show(TuiContextMenu* menu) {
    if (!menu || menu->count == 0) return;
    menu->visible = true;
    menu->selected = 0;
    ncplane_resize_simple(menu->plane, (unsigned)menu->height, (unsigned)menu->width);
    ncplane_move_top(menu->plane);
    tui_menu_render(menu);
}

void tui_menu_hide(TuiContextMenu* menu) {
    if (!menu) return;
    menu->visible = false;
}

bool tui_menu_is_visible(const TuiContextMenu* menu) {
    return menu ? menu->visible : false;
}

bool tui_menu_handle_key(TuiContextMenu* menu, uint32_t key, const struct ncinput* ni) {
    if (!menu || !menu->visible || ni->evtype == NCTYPE_RELEASE) return false;

    if (key == NCKEY_UP && menu->selected > 0) {
        menu->selected--;
        tui_menu_render(menu);
        return true;
    }
    if (key == NCKEY_DOWN && menu->selected < menu->count - 1) {
        menu->selected++;
        tui_menu_render(menu);
        return true;
    }
    if (key == NCKEY_ENTER || key == '\n' || key == '\r') {
        if (menu->selected >= 0 && menu->selected < menu->count) {
            if (menu->callbacks[menu->selected]) menu->callbacks[menu->selected](menu->userdatas[menu->selected]);
        }
        tui_menu_hide(menu);
        return true;
    }
    if (key == 0x1B || key == 'q') {  /* 0x1B = Escape */
        tui_menu_hide(menu);
        return true;
    }
    return false;
}

void tui_menu_render(TuiContextMenu* menu) {
    if (!menu || !menu->plane || !menu->visible) return;

    uint64_t base = 0;
    ncchannels_set_fg_rgb(&base, menu->fg_text);
    ncchannels_set_bg_rgb(&base, menu->bg_normal);
    ncchannels_set_bg_alpha(&base, NCALPHA_OPAQUE);
    ncplane_set_base(menu->plane, " ", 0, base);
    ncplane_erase(menu->plane);

    for (int i = 0; i < menu->count; i++) {
        if (i == menu->selected) {
            ncplane_set_fg_rgb(menu->plane, menu->fg_selected);
            ncplane_set_bg_rgb(menu->plane, menu->bg_selected);
        } else {
            ncplane_set_fg_rgb(menu->plane, menu->fg_text);
            ncplane_set_bg_rgb(menu->plane, menu->bg_normal);
        }
        ncplane_putstr_yx(menu->plane, i, 1, menu->items[i]);
    }
}

bool tui_menu_handle_mouse(TuiContextMenu* menu, uint32_t key, const struct ncinput* ni) {
    if (!menu || !ni || !menu->plane || !menu->visible || ni->evtype == NCTYPE_RELEASE) return false;
    if (key != NCKEY_BUTTON1) return false;

    /* Check if click is within this menu's plane */
    int plane_abs_y, plane_abs_x;
    ncplane_abs_yx(menu->plane, &plane_abs_y, &plane_abs_x);
    unsigned rows, cols;
    ncplane_dim_yx(menu->plane, &rows, &cols);

    int local_y = ni->y - plane_abs_y;
    int local_x = ni->x - plane_abs_x;

    if (local_y < 0 || local_y >= (int)rows || local_x < 0 || local_x >= (int)cols) return false;

    /* Select and activate clicked item */
    if (local_y >= 0 && local_y < menu->count) {
        menu->selected = local_y;
        if (menu->callbacks[menu->selected]) menu->callbacks[menu->selected](menu->userdatas[menu->selected]);
    }
    tui_menu_hide(menu);
    return true;
}

/* === VTable Implementation === */

static bool v_menu_handle_key(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiContextMenu* menu = (TuiContextMenu*)widget;
    return tui_menu_handle_key(menu, key, ni);
}

static bool v_menu_handle_mouse(void* widget, uint32_t key, const struct ncinput* ni) {
    TuiContextMenu* menu = (TuiContextMenu*)widget;
    return tui_menu_handle_mouse(menu, key, ni);
}

static void v_menu_render(void* widget) {
    tui_menu_render((TuiContextMenu*)widget);
}

static bool v_menu_is_focusable(void* widget) {
    TuiContextMenu* menu = (TuiContextMenu*)widget;
    return menu && menu->visible;
}

static const TuiWidgetIface s_menu_iface = {
    .handle_key = v_menu_handle_key,
    .handle_mouse = v_menu_handle_mouse,
    .render = v_menu_render,
    .is_focusable = v_menu_is_focusable,
};

void tui_menu_ensure_registered(void) {
    if (s_menu_type_id < 0) {
        s_menu_type_id = tui_widget_register(&s_menu_iface);
    }
}

int tui_menu_get_type_id(void) {
    tui_menu_ensure_registered();
    return s_menu_type_id;
}
