# TMLCS-TUI — API Reference

> Referencia rápida de las 120+ funciones públicas del framework v0.6.0.

## Architecture Overview

```
TuiManager
  └── TuiWorkspace[]  (add_workspace, set_active_workspace)
        └── TuiTab[]  (add_tab, set_active_tab)
              └── TuiWindow[]  (add_window, remove_window)
                    ├── render_cb          (custom content)
                    ├── on_destroy         (cleanup callback)
                    ├── text_input         (legacy text input)
                    └── widgets[]          (widget registry for mouse routing)
                          ├── TuiButton
                          ├── TuiCheckbox
                          ├── TuiList
                          ├── TuiContextMenu
                          └── TuiTextInput
```

## Ownership Model

```
notcurses ─► TuiManager ─► TuiWorkspace[] ─► TuiTab[] ─► TuiWindow[]
                                                                   ├─ ncplane (auto-destroy)
                                                                   ├─ text_input (destroyed via on_destroy)
                                                                   └─ widgets[] (externally owned)
```

**Rules:**
1. **Parent destroys children** — Manager→Workspace→Tab→Window cascade
2. **Widgets are externally owned** — Caller creates and destroys widgets; window only tracks references
3. **`on_destroy` fires BEFORE** plane destruction — safe to nullify external references
4. **Safe realloc** — All dynamic arrays use temp-variable pattern

---

## Core Module

### TuiManager

| Function | Description |
|----------|-------------|
| `tui_manager_create(nc)` | Create the main orchestrator |
| `tui_manager_destroy(mgr)` | Free manager + all workspaces (cascade) |
| `tui_manager_add_workspace(mgr, ws)` | Register a workspace |
| `tui_manager_set_active_workspace(mgr, index)` | Switch to workspace by index |
| `tui_manager_remove_active_workspace(mgr)` | Remove the active workspace |
| `tui_manager_render(mgr)` | Full repaint of the UI tree |
| `tui_manager_process_mouse(mgr, key, ni)` | Mouse event dispatcher |
| `tui_manager_process_keyboard(mgr, key, ni)` | Keyboard event dispatcher |
| `tui_manager_set_theme(mgr, name)` | Apply built-in theme by name |
| `tui_manager_get_tab_at(mgr, y, x)` | Hit-test tab bar → tab index |
| `tui_manager_update_hover(y, x)` | Update hover position (internal) |

### TuiWorkspace

| Function | Description |
|----------|-------------|
| `tui_workspace_create(mgr, name)` | Create a workspace |
| `tui_workspace_destroy(ws)` | Free workspace + all tabs |
| `tui_workspace_add_tab(ws, tab)` | Add a tab to the workspace |
| `tui_workspace_set_active_tab(ws, index)` | Activate tab (others off-screen) |
| `tui_workspace_remove_active_tab(ws)` | Remove the active tab |
| `tui_workspace_get_name(ws)` | → `const char*` |
| `tui_workspace_get_id(ws)` | → `int` |
| `tui_workspace_get_tab_count(ws)` | → `int` |

### TuiTab

| Function | Description |
|----------|-------------|
| `tui_tab_create(ws, name)` | Create a tab within a workspace |
| `tui_tab_destroy(tab)` | Free tab + all windows |
| `tui_tab_add_window(tab, win)` | Register a window with the tab |
| `tui_tab_remove_window(tab, win)` | Remove a window from the tab |
| `tui_tab_get_window_at(tab, y, x, &wly, &wlx)` | Hit-test windows → window ptr |
| `tui_tab_get_name(tab)` | → `const char*` |
| `tui_tab_get_id(tab)` | → `int` |
| `tui_tab_get_window_count(tab)` | → `int` |
| `tui_tab_get_active_window_index(tab)` | → `int` |

### TuiWindow

| Function | Description |
|----------|-------------|
| `tui_window_create(tab, width, height)` | Create a window within a tab |
| `tui_window_destroy(win)` | Free window (calls `on_destroy` first) |
| `tui_window_add_widget(win, widget, plane, type)` | Register widget for mouse routing |
| `tui_window_get_widget_at(win, y, x, &type)` | Hit-test widgets → widget ptr |
| `tui_window_get_id(win)` | → `int` |
| `tui_window_is_focused(win)` | → `bool` |

### Widget Type Constants

| Constant | Widget Type |
|----------|-------------|
| `WIDGET_NONE` | No widget |
| `WIDGET_BUTTON` | `TuiButton` |
| `WIDGET_CHECKBOX` | `TuiCheckbox` |
| `WIDGET_LIST` | `TuiList` |
| `WIDGET_CONTEXT_MENU` | `TuiContextMenu` |
| `WIDGET_TEXT_INPUT` | `TuiTextInput` |

---

## Widget Module

### Text Input

Single-line text input with cursor navigation, clipboard support, and mouse positioning.

| Function | Description |
|----------|-------------|
| `tui_text_input_create(parent, y, x, width, on_submit, userdata)` | Create text input |
| `tui_text_input_destroy(ti)` | Free text input + ncplane |
| `tui_text_input_handle_key(ti, key, ni)` | Process key event → `bool` consumed |
| `tui_text_input_handle_mouse(ti, key, ni)` | Click positions cursor → `bool` |
| `tui_text_input_render(ti)` | Render the widget |
| `tui_text_input_clear(ti)` | Clear buffer, reset cursor |
| `tui_text_input_get(ti)` | → `const char*` buffer |
| `tui_text_input_get_cursor(ti)` | → `int` cursor position |
| `tui_text_input_get_len(ti)` | → `int` text length |
| `tui_text_input_is_focused(ti)` | → `bool` |
| `tui_text_input_copy(ti)` | → `bool` copy to clipboard |
| `tui_text_input_paste(ti)` | → `bool` paste from clipboard |
| `tui_text_input_cut(ti)` | → `bool` cut all text to clipboard |

**Key bindings:**

| Key | Action |
|-----|--------|
| `A-Z, a-z, 0-9` | Insert character |
| `Enter` | Submit callback + clear |
| `Backspace` | Delete char before cursor |
| `Left / Right` | Move cursor |
| `Home / Ctrl+A` | Jump to start |
| `End / Ctrl+E` | Jump to end |
| `Ctrl+Left / Ctrl+Right` | Jump by word |
| `Ctrl+K` | Cut from cursor to end |
| `Ctrl+U` | Cut from start to cursor |
| `Ctrl+W` | Delete word before cursor |
| `Ctrl+C` | Copy all to clipboard |
| `Ctrl+V` | Paste from clipboard |
| `Ctrl+X` | Cut all to clipboard |

### TextArea

Multi-line text input with Enter for newlines, arrow navigation, and scroll.

| Function | Description |
|----------|-------------|
| `tui_textarea_create(parent, y, x, width, height)` | Create text area |
| `tui_textarea_destroy(ta)` | Free text area + ncplane |
| `tui_textarea_handle_key(ta, key, ni)` | Process key event → `bool` |
| `tui_textarea_render(ta)` | Render the widget |
| `tui_textarea_clear(ta)` | Clear all text |
| `tui_textarea_get(ta, buf, max_len)` | → bytes written (joined by `\n`) |
| `tui_textarea_get_line_count(ta)` | → `int` line count |
| `tui_textarea_set_focused(ta, focused)` | Set focus state |

### Button

Clickable button with callback activation on Enter or mouse click.

| Function | Description |
|----------|-------------|
| `tui_button_create(parent, y, x, width, label, cb, userdata)` | Create button |
| `tui_button_destroy(btn)` | Free button + ncplane |
| `tui_button_handle_key(btn, key, ni)` | Enter → activate callback |
| `tui_button_handle_mouse(btn, key, ni)` | Click → press animation + callback |
| `tui_button_render(btn)` | Render the widget |
| `tui_button_set_label(btn, label)` | Change button text |
| `tui_button_is_pressed(btn)` | → `bool` |
| `tui_button_set_focused(btn, focused)` | Set focus state |

### Checkbox

Toggle checkbox with callback on state change.

| Function | Description |
|----------|-------------|
| `tui_checkbox_create(parent, y, x, label, checked, cb, userdata)` | Create checkbox |
| `tui_checkbox_destroy(cb)` | Free checkbox + ncplane |
| `tui_checkbox_handle_key(cb, key, ni)` | Space/Enter → toggle |
| `tui_checkbox_handle_mouse(cb, key, ni)` | Click → toggle |
| `tui_checkbox_render(cb)` | Render the widget |
| `tui_checkbox_get_state(cb)` | → `bool` checked |
| `tui_checkbox_set_state(cb, checked)` | Set state + fire callback |
| `tui_checkbox_set_focused(cb, focused)` | Set focus state |

### List

Scrollable list with keyboard and mouse selection.

| Function | Description |
|----------|-------------|
| `tui_list_create(parent, y, x, width, height)` | Create list widget |
| `tui_list_destroy(list)` | Free list + ncplane + items |
| `tui_list_add_item(list, item)` | Append item (copied) |
| `tui_list_remove_item(list, index)` | Remove item by index |
| `tui_list_clear(list)` | Remove all items |
| `tui_list_handle_key(list, key, ni)` | Up/Down → change selection |
| `tui_list_handle_mouse(list, key, ni)` | Click → select item |
| `tui_list_render(list)` | Render the widget |
| `tui_list_get_selected(list)` | → `int` selected index |
| `tui_list_set_selected(list, index)` | Set selection (clamped) |
| `tui_list_get_count(list)` | → `int` item count |
| `tui_list_set_focused(list, focused)` | Set focus state |

### Label

Simple text display widget.

| Function | Description |
|----------|-------------|
| `tui_label_create(parent, y, x, width, text)` | Create label |
| `tui_label_destroy(label)` | Free label + ncplane |
| `tui_label_render(label)` | Render the widget |
| `tui_label_get_text(label)` | → `const char*` |
| `tui_label_set_text(label, text)` | Change label text |

### Progress

Progress bar widget (0.0 – 1.0).

| Function | Description |
|----------|-------------|
| `tui_progress_create(parent, y, x, width)` | Create progress bar |
| `tui_progress_destroy(pb)` | Free progress bar + ncplane |
| `tui_progress_set_value(pb, value)` | Set value (clamped 0.0–1.0) |
| `tui_progress_get_value(pb)` | → `float` current value |
| `tui_progress_render(pb)` | Render the widget |

### Context Menu

Popup menu with keyboard navigation and mouse selection.

| Function | Description |
|----------|-------------|
| `tui_menu_create(parent, y, x, width)` | Create menu (initially hidden) |
| `tui_menu_destroy(menu)` | Free menu + item labels |
| `tui_menu_add_item(menu, label, cb, userdata)` | Add menu item → `bool` |
| `tui_menu_show(menu)` | Show menu, reset selection |
| `tui_menu_hide(menu)` | Hide without destroying |
| `tui_menu_is_visible(menu)` | → `bool` |
| `tui_menu_handle_key(menu, key, ni)` | Up/Down/Enter/ESC navigation |
| `tui_menu_handle_mouse(menu, key, ni)` | Click → select + activate + hide |
| `tui_menu_render(menu)` | Render the widget |

---

## Utility Module

### Logger

Thread-safe ring buffer logger with optional file output.

| Function | Description |
|----------|-------------|
| `tui_logger_init(filepath)` | Initialize logger (filepath may be NULL) |
| `tui_logger_destroy()` | Close log file |
| `tui_log(level, format, ...)` | Write formatted message |
| `tui_logger_set_level(level)` | Set minimum log level |
| `tui_logger_get_level()` | → `LogLevel` current level |
| `tui_logger_get_buffer()` | → `TuiLogBuffer*` ring buffer |

**Log levels:** `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`

### Clipboard

Clipboard integration via OSC 52 terminal escape sequences.

| Function | Description |
|----------|-------------|
| `tui_clipboard_copy(text)` | Copy text to clipboard (OSC 52) |
| `tui_clipboard_paste(buf, max_len)` | Paste from clipboard → bytes read |
| `tui_clipboard_get()` | → `const char*` current clipboard content |

### Help Overlay

Built-in help overlay showing keybindings.

| Function | Description |
|----------|-------------|
| `tui_help_show(plane)` | Show help overlay |
| `tui_help_hide()` | Hide help overlay |
| `tui_help_is_visible()` | → `bool` |

---

## Error Handling Convention

| Pattern | Behavior |
|---------|----------|
| Factory functions | Return `NULL` on failure, log `LOG_ERROR` |
| Destroy functions | NULL-safe no-op |
| Query functions | Return safe defaults for NULL (`-1`, `false`, `NULL`) |
| Mutator functions | Early return on NULL or invalid params |

---

## Keybindings Reference

### Manager

| Key | Action |
|-----|--------|
| `q / Q` | Quit application |
| `Alt+1..9` | Switch workspace |
| `← / →` | Navigate tabs |
| `Tab / Shift+Tab` | Cycle window focus (forward / reverse) |
| `x` | Close focused window |
| `w` | Close active tab |
| `d` | Close active workspace |
| `Ctrl+T` | New tab |
| `Ctrl+N` | New window |
| `Ctrl+Shift+Arrows` | Resize focused window |
| `F1 / ?` | Toggle help overlay |
| `ESC` | Cancel drag/resize/help |

### Mouse

| Action | How |
|--------|-----|
| Switch tab | Click tab name in tab bar |
| Focus window | Click anywhere on window |
| Drag window | Click and drag title bar |
| Resize window | Click and drag bottom-right grip (`⌟`) |
| Close window | Click `[X]` button in title bar |
| Click button | Click on button → activates callback |
| Toggle checkbox | Click on checkbox → toggles state |
| Select list item | Click on item → selects it |
| Context menu item | Click on item → activates + closes |
| Position cursor | Click in text input → sets cursor position |
| Scroll wheel | Scroll up/down → cycle tabs |

---

## Build & Test Commands

| Command | Description |
|---------|-------------|
| `make clean && make` | Clean build |
| `make run` | Build and run |
| `make test-all` | Run all unit test suites |
| `make asan` | Build with AddressSanitizer + UBSan + run tests |
| `make valgrind` | Run tests under Valgrind |
| `make coverage` | Generate lcov HTML coverage report |
| `make tidy` | Run clang-tidy static analysis |
| `make docs` | Generate Doxygen API docs |
| `make fmt` | Format code with clang-format |
| `make install PREFIX=/usr/local` | Install headers + pkg-config |
| `make hello` | Build and run hello world example |
| `make tabs-demo` | Build and run tabs demo |
| `make text-input-demo` | Build and run text input demo |
