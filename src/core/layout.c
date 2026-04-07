#include "core/layout.h"
#include "core/widget.h"
#include "core/logger.h"
#include <stdlib.h>
#include <string.h>

struct TuiLayout {
    TuiLayoutDirection direction;
    int spacing;
    int pad_top, pad_right, pad_bottom, pad_left;
    TuiLayoutChild* children;
    int child_count;
    int child_capacity;
    bool needs_compute;
};

TuiLayout* tui_layout_create(TuiLayoutDirection direction) {
    TuiLayout* layout = (TuiLayout*)calloc(1, sizeof(TuiLayout));
    if (!layout) return NULL;
    layout->direction = direction;
    layout->child_capacity = 8;
    layout->children = (TuiLayoutChild*)calloc((size_t)layout->child_capacity, sizeof(TuiLayoutChild));
    if (!layout->children) { free(layout); return NULL; }
    layout->needs_compute = true;
    return layout;
}

void tui_layout_destroy(TuiLayout* layout) {
    if (!layout) return;
    for (int i = 0; i < layout->child_count; i++) {
        if (layout->children[i].nested_layout) {
            tui_layout_destroy(layout->children[i].nested_layout);
        }
    }
    free(layout->children);
    free(layout);
}

void tui_layout_set_spacing(TuiLayout* layout, int spacing) {
    if (layout) layout->spacing = spacing;
}

void tui_layout_set_padding(TuiLayout* layout, int top, int right, int bottom, int left) {
    if (!layout) return;
    layout->pad_top = top;
    layout->pad_right = right;
    layout->pad_bottom = bottom;
    layout->pad_left = left;
}

static void ensure_layout_capacity(TuiLayout* layout) {
    if (layout->child_count < layout->child_capacity) return;
    int new_capacity = layout->child_capacity * 2;
    TuiLayoutChild* tmp = (TuiLayoutChild*)realloc(layout->children,
        (size_t)new_capacity * sizeof(TuiLayoutChild));
    if (!tmp) {
        tui_log(LOG_ERROR, "OOM in ensure_layout_capacity");
        return;
    }
    layout->children = tmp;
    layout->child_capacity = new_capacity;
}

void tui_layout_add_widget(TuiLayout* layout, void* widget, int type_id, struct ncplane* plane,
                           TuiSizeConstraint size, float size_value, TuiAlignment alignment) {
    if (!layout || !widget || !plane) return;
    ensure_layout_capacity(layout);
    TuiLayoutChild* child = &layout->children[layout->child_count++];
    memset(child, 0, sizeof(TuiLayoutChild));
    child->widget = widget;
    child->type_id = type_id;
    child->plane = plane;
    child->size = size;
    child->size_value = size_value;
    child->alignment = alignment;
    child->visible = true;
    layout->needs_compute = true;
}

void tui_layout_add_layout(TuiLayout* layout, TuiLayout* child_layout,
                           TuiSizeConstraint size, float size_value, TuiAlignment alignment) {
    if (!layout || !child_layout) return;
    ensure_layout_capacity(layout);
    TuiLayoutChild* child = &layout->children[layout->child_count++];
    memset(child, 0, sizeof(TuiLayoutChild));
    child->nested_layout = child_layout;
    child->size = size;
    child->size_value = size_value;
    child->alignment = alignment;
    child->visible = true;
    layout->needs_compute = true;
}

void tui_layout_remove_child(TuiLayout* layout, int index) {
    if (!layout || index < 0 || index >= layout->child_count) return;
    if (layout->children[index].nested_layout) {
        tui_layout_destroy(layout->children[index].nested_layout);
    }
    for (int i = index; i < layout->child_count - 1; i++) {
        layout->children[i] = layout->children[i + 1];
    }
    layout->child_count--;
    layout->needs_compute = true;
}

int tui_layout_get_child_count(const TuiLayout* layout) {
    return layout ? layout->child_count : 0;
}

void tui_layout_mark_dirty(TuiLayout* layout) {
    if (layout) layout->needs_compute = true;
}

void tui_layout_compute(TuiLayout* layout, int available_height, int available_width) {
    if (!layout || !layout->needs_compute) return;

    int content_h = available_height - layout->pad_top - layout->pad_bottom;
    int content_w = available_width - layout->pad_left - layout->pad_right;
    if (content_h < 0) content_h = 0;
    if (content_w < 0) content_w = 0;

    /* Step 1: Measure FIXED and AUTO children */
    int fixed_total = 0;
    int fill_count = 0;
    float weight_total = 0.0f;
    float percent_total = 0.0f;

    for (int i = 0; i < layout->child_count; i++) {
        TuiLayoutChild* child = &layout->children[i];
        if (!child->visible) continue;

        int child_h = -1, child_w = -1;

        switch (child->size) {
            case SIZE_FIXED:
                child_h = (int)child->size_value;
                child_w = -1;
                break;
            case SIZE_AUTO:
                if (child->widget) {
                    tui_widget_preferred_size(child->type_id, child->widget, content_h, content_w, &child_h, &child_w);
                }
                if (child_h < 0) child_h = 1;
                if (child_w < 0) child_w = 10;
                break;
            case SIZE_FILL:
                fill_count++;
                break;
            case SIZE_WEIGHT:
                weight_total += child->size_value;
                break;
            case SIZE_PERCENT:
                percent_total += child->size_value;
                break;
        }

        if (child_h > 0 && layout->direction == LAYOUT_VERTICAL) {
            fixed_total += child_h;
        } else if (child_w > 0 && layout->direction == LAYOUT_HORIZONTAL) {
            fixed_total += child_w;
        }
    }

    /* Account for spacing between children */
    int visible_count = 0;
    for (int i = 0; i < layout->child_count; i++) {
        if (layout->children[i].visible) visible_count++;
    }
    int spacing_total = visible_count > 0 ? layout->spacing * (visible_count - 1) : 0;

    /* Step 2: Calculate remaining space */
    int main_size = (layout->direction == LAYOUT_VERTICAL) ? content_h : content_w;
    int remaining = main_size - fixed_total - spacing_total;
    if (remaining < 0) remaining = 0;

    /* Step 3: Position children */
    int offset = (layout->direction == LAYOUT_VERTICAL) ? layout->pad_top : layout->pad_left;

    for (int i = 0; i < layout->child_count; i++) {
        TuiLayoutChild* child = &layout->children[i];
        if (!child->visible) continue;

        int child_main = 0;

        switch (child->size) {
            case SIZE_FIXED:
                child_main = (int)child->size_value;
                break;
            case SIZE_AUTO:
                if (child->widget) {
                    int h = -1, w = -1;
                    tui_widget_preferred_size(child->type_id, child->widget, content_h, content_w, &h, &w);
                    child_main = (layout->direction == LAYOUT_VERTICAL) ? h : w;
                }
                if (child_main <= 0) child_main = (layout->direction == LAYOUT_VERTICAL) ? 1 : 10;
                break;
            case SIZE_PERCENT:
                child_main = (int)(main_size * child->size_value / 100.0f);
                break;
            case SIZE_WEIGHT:
                child_main = weight_total > 0 ? (int)(remaining * child->size_value / weight_total) : 0;
                break;
            case SIZE_FILL:
                child_main = fill_count > 0 ? remaining / fill_count : 0;
                break;
        }

        /* Compute cross-axis size based on alignment */
        int cross_size = (layout->direction == LAYOUT_VERTICAL) ? content_w : content_h;
        int child_cross = cross_size;

        /* Position the plane */
        int plane_y, plane_x;
        if (layout->direction == LAYOUT_VERTICAL) {
            plane_y = offset;
            plane_x = layout->pad_left;
            if (child->alignment == ALIGN_CENTER) {
                plane_x = (content_w - child_cross) / 2 + layout->pad_left;
            } else if (child->alignment == ALIGN_END) {
                plane_x = content_w - child_cross + layout->pad_left;
            }
        } else {
            plane_y = layout->pad_top;
            plane_x = offset;
            if (child->alignment == ALIGN_CENTER) {
                plane_y = (content_h - child_cross) / 2 + layout->pad_top;
            } else if (child->alignment == ALIGN_END) {
                plane_y = content_h - child_cross + layout->pad_top;
            }
        }

        /* Resize and move the plane */
        if (child->plane) {
            int plane_h = (layout->direction == LAYOUT_VERTICAL) ? child_main : child_cross;
            int plane_w = (layout->direction == LAYOUT_VERTICAL) ? child_cross : child_main;
            if (plane_h > 0 && plane_w > 0) {
                ncplane_resize_simple(child->plane, (unsigned)plane_h, (unsigned)plane_w);
                ncplane_move_yx(child->plane, plane_y, plane_x);
            }
        }

        /* Recursively compute nested layout */
        if (child->nested_layout) {
            int nested_h = (layout->direction == LAYOUT_VERTICAL) ? child_main : child_cross;
            int nested_w = (layout->direction == LAYOUT_VERTICAL) ? child_cross : child_main;
            tui_layout_compute(child->nested_layout, nested_h, nested_w);
        }

        offset += child_main + layout->spacing;
    }

    layout->needs_compute = false;
}

void tui_layout_render(TuiLayout* layout) {
    if (!layout) return;
    for (int i = 0; i < layout->child_count; i++) {
        TuiLayoutChild* child = &layout->children[i];
        if (!child->visible) continue;
        if (child->nested_layout) {
            tui_layout_render(child->nested_layout);
        } else if (child->widget) {
            tui_widget_render(child->type_id, child->widget);
        }
    }
}
