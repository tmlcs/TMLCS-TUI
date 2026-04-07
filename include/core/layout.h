#ifndef CORE_LAYOUT_H
#define CORE_LAYOUT_H

#include <stdbool.h>

typedef struct TuiLayout TuiLayout;

typedef enum {
    LAYOUT_HORIZONTAL,
    LAYOUT_VERTICAL
} TuiLayoutDirection;

typedef enum {
    SIZE_FIXED,
    SIZE_FILL,
    SIZE_WEIGHT,
    SIZE_PERCENT,
    SIZE_AUTO
} TuiSizeConstraint;

typedef enum {
    ALIGN_START,
    ALIGN_CENTER,
    ALIGN_END,
    ALIGN_STRETCH
} TuiAlignment;

typedef struct TuiLayoutChild TuiLayoutChild;
struct TuiLayoutChild {
    void* widget;
    struct ncplane* plane;
    int type_id;                  /**< Registered widget type_id for dispatch */
    TuiLayout* nested_layout;
    TuiSizeConstraint size;
    float size_value;
    TuiAlignment alignment;
    bool visible;
};

TuiLayout* tui_layout_create(TuiLayoutDirection direction);
void tui_layout_destroy(TuiLayout* layout);
void tui_layout_set_spacing(TuiLayout* layout, int spacing);
void tui_layout_set_padding(TuiLayout* layout, int top, int right, int bottom, int left);
void tui_layout_add_widget(TuiLayout* layout, void* widget, int type_id, struct ncplane* plane,
                           TuiSizeConstraint size, float size_value, TuiAlignment alignment);
void tui_layout_add_layout(TuiLayout* layout, TuiLayout* child,
                           TuiSizeConstraint size, float size_value, TuiAlignment alignment);
void tui_layout_remove_child(TuiLayout* layout, int index);
void tui_layout_compute(TuiLayout* layout, int available_height, int available_width);
void tui_layout_render(TuiLayout* layout);
void tui_layout_mark_dirty(TuiLayout* layout);
int tui_layout_get_child_count(const TuiLayout* layout);

#endif
