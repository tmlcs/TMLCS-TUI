#ifndef CORE_TAB_H
#define CORE_TAB_H

#include "core/types.h"

TuiTab* tui_tab_create(struct TuiWorkspace* ws, const char* name);
void tui_tab_destroy(TuiTab* tab);

void tui_tab_add_window(TuiTab* tab, struct TuiWindow* win);
void tui_tab_remove_window(TuiTab* tab, struct TuiWindow* win);
void tui_tab_remove_active_window(TuiTab* tab);
TuiWindow* tui_tab_get_window_at(TuiTab* tab, int abs_y, int abs_x, int* wly, int* wlx);

#endif
