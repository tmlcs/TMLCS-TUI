#ifndef CORE_WINDOW_H
#define CORE_WINDOW_H

#include "core/types.h"

TuiWindow* tui_window_create(TuiTab* tab, int width, int height);
void tui_window_destroy(TuiWindow* win);

#endif
