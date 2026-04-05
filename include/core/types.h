#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

struct TuiWindow;
struct TuiTab;
struct TuiWorkspace;
struct TuiManager;

typedef struct TuiWindow {
    int id;
    struct ncplane* plane;
    void* user_data;
    void (*render_cb)(struct TuiWindow* win);
    void (*on_destroy)(struct TuiWindow* win);
    struct TuiTextInput* text_input;
    bool needs_redraw;   // Solo se repinta si true
    bool focused;        // Ventana con foco activo (borde diferente)
} TuiWindow;

typedef struct TuiTab {
    int id;
    char name[64];
    TuiWindow** windows;
    int window_count;
    int window_capacity;
    int active_window_index;
    struct ncplane* tab_plane; 
} TuiTab;

typedef struct TuiWorkspace {
    int id;
    char name[64];
    TuiTab** tabs;
    int tab_count;
    int tab_capacity;
    int active_tab_index;
    struct ncplane* ws_plane; 
} TuiWorkspace;

typedef struct TuiManager {
    struct notcurses* nc;
    struct ncplane* stdplane;
    TuiWorkspace** workspaces;
    int workspace_count;
    int workspace_capacity;
    int active_workspace_index;
    bool running;
    
    struct TuiWindow* dragged_window;
    int drag_start_wly;
    int drag_start_wlx;
    
    struct TuiWindow* resizing_window;
    int resize_start_dimy;
    int resize_start_dimx;
    int resize_start_mouse_y;
    int resize_start_mouse_x;
} TuiManager;

extern int s_next_id;

#endif // CORE_TYPES_H
