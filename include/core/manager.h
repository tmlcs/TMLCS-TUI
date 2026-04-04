#ifndef CORE_MANAGER_H
#define CORE_MANAGER_H

#include "core/types.h"

int tui_manager_get_tab_at(TuiManager* mgr, int y, int x);
bool tui_manager_process_mouse(TuiManager* mgr, uint32_t key, const struct ncinput* ni);
TuiManager* tui_manager_create(struct notcurses* nc);
void tui_manager_destroy(TuiManager* manager);
void tui_manager_add_workspace(TuiManager* manager, TuiWorkspace* ws);
void tui_manager_set_active_workspace(TuiManager* manager, int index);
void tui_manager_remove_active_workspace(TuiManager* manager);
void tui_manager_render(TuiManager* manager);

#endif
