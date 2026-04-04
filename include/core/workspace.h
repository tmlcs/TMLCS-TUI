#ifndef CORE_WORKSPACE_H
#define CORE_WORKSPACE_H

#include "core/types.h"

TuiWorkspace* tui_workspace_create(TuiManager* manager, const char* name);
void tui_workspace_destroy(TuiWorkspace* ws);
void tui_workspace_add_tab(TuiWorkspace* ws, TuiTab* tab);
void tui_workspace_set_active_tab(TuiWorkspace* ws, int index);
void tui_workspace_remove_active_tab(TuiWorkspace* ws);

#endif
