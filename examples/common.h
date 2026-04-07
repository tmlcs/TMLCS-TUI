#ifndef DEMO_COMMON_H
#define DEMO_COMMON_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

/* Access to internal fields for demo purposes */
#include "core/types_private.h"

/* Forward declarations for widget types not in types_private.h */
typedef struct TuiDialog TuiDialog;
typedef struct TuiTextInput TuiTextInput;
typedef struct TuiProgressBar TuiProgressBar;
typedef struct TuiSpinner TuiSpinner;
typedef struct TuiSlider TuiSlider;
typedef struct TuiLabel TuiLabel;
typedef struct TuiRadioGroup TuiRadioGroup;
typedef struct TuiCheckbox TuiCheckbox;
typedef struct TuiButton TuiButton;
typedef struct TuiList TuiList;
typedef struct TuiDropdown TuiDropdown;
typedef struct TuiTable TuiTable;
typedef struct TuiTree TuiTree;
typedef struct TuiFilePicker TuiFilePicker;
typedef struct TuiTextArea TuiTextArea;
typedef struct TuiTabContainer TuiTabContainer;

/* ============================================================
 * Shared demo state
 * ============================================================ */

typedef struct DemoState {
    TuiTextInput* active_input;
    int console_log_count;
    float cpu_usage;
    float stock_prices[5];
    float build_progress;
    int build_frame;
    bool agents_running[4];
} DemoState;

extern DemoState g_demo;

/* ============================================================
 * Window frame renderer (shared by all workspaces)
 * ============================================================ */

void demo_render_window_frame(TuiWindow* win);

/* ============================================================
 * Mock data updater
 * ============================================================ */

void demo_update_mock_data(void);

/* ============================================================
 * Workspace factory prototypes
 * ============================================================ */

TuiWorkspace* create_system_workspace(TuiManager* mgr);
TuiWorkspace* create_console_workspace(TuiManager* mgr);
TuiWorkspace* create_desktop_workspace(TuiManager* mgr);
TuiWorkspace* create_finance_workspace(TuiManager* mgr);
TuiWorkspace* create_browse_workspace(TuiManager* mgr);
TuiWorkspace* create_development_workspace(TuiManager* mgr);
TuiWorkspace* create_agents_workspace(TuiManager* mgr);
TuiWorkspace* create_log_workspace(TuiManager* mgr);
TuiWorkspace* create_files_workspace(TuiManager* mgr);

/* Dialog accessors (agents workspace) */
bool agents_get_dialog_visible(void);
TuiDialog* agents_get_dialog(void);
void agents_set_dialog_visible(bool vis);
struct ncplane* agents_get_dialog_plane(void);
struct ncplane* agents_get_dialog_parent(void);
void agents_set_dialog_parent(struct ncplane* p);

/* ============================================================
 * Internal widget type registration functions (not in public headers)
 * ============================================================ */

void tui_button_ensure_registered(void);        int tui_button_get_type_id(void);
void tui_checkbox_ensure_registered(void);      int tui_checkbox_get_type_id(void);
void tui_label_ensure_registered(void);         int tui_label_get_type_id(void);
void tui_list_ensure_registered(void);          int tui_list_get_type_id(void);
void tui_progress_ensure_registered(void);      int tui_progress_get_type_id(void);
void tui_textarea_ensure_registered(void);      int tui_textarea_get_type_id(void);
void tui_text_input_ensure_registered(void);    int tui_text_input_get_type_id(void);
void tui_slider_ensure_registered(void);        int tui_slider_get_type_id(void);
void tui_radio_group_ensure_registered(void);   int tui_radio_group_get_type_id(void);
void tui_spinner_ensure_registered(void);       int tui_spinner_get_type_id(void);
void tui_tab_container_ensure_registered(void); int tui_tab_container_get_type_id(void);
void tui_dropdown_ensure_registered(void);      int tui_dropdown_get_type_id(void);
void tui_dialog_ensure_registered(void);        int tui_dialog_get_type_id(void);
void tui_table_ensure_registered(void);         int tui_table_get_type_id(void);
void tui_tree_ensure_registered(void);          int tui_tree_get_type_id(void);
void tui_file_picker_ensure_registered(void);   int tui_file_picker_get_type_id(void);

#endif /* DEMO_COMMON_H */
