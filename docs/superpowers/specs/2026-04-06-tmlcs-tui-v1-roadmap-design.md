# TMLCS-TUI Roadmap v1.0 — Design Specification

**Date:** 2026-04-06
**Version:** 0.5.0 → 1.0.0
**Status:** Approved

## Overview

This document describes the complete design for transforming tmlcs-tui from a functional TUI framework (v0.5.0) into a production-ready library (v1.0.0). The roadmap consists of 7 sequential phases with 28+ individual improvements across architecture, widgets, input handling, developer experience, testing, and production readiness.

## Dependency Graph

```
FASE 1 → FASE 2 → FASE 3 → FASE 4 → FASE 5 → FASE 6 → FASE 7
                ↓
           FASE 3b (parallel with FASE 3)
```

---

## FASE 1: Architecture Foundations

### 1.1 Opaque Types

**Problem:** All structs are transparent. Users access `mgr->workspace_count`, `win->focused`, `ti->buffer` directly. Any internal change breaks ABI.

**Solution:** Public headers expose only forward declarations. Full struct definitions move to private headers not installed with the library.

```c
// include/core/types.h (public)
typedef struct TuiManager TuiManager;
typedef struct TuiWindow TuiWindow;
typedef struct TuiTab TuiTab;
typedef struct TuiWorkspace TuiWorkspace;

// src/core/types_private.h (NOT installed)
struct TuiManager {
    struct notcurses* nc;
    struct ncplane* _stdplane;
    TuiWorkspace** _workspaces;
    int _workspace_count;
    // ... all fields with _ prefix for internal ones
};
```

**Getters/Setters:** Only fields that users legitimately need to read get getters. Setters are provided only where mutation is part of the public API.

| Struct | Getter | Setter |
|--------|--------|--------|
| TuiManager | `is_running()`, `get_workspace_count()`, `get_active_workspace_index()` | `set_running()` |
| TuiWindow | `get_id()`, `is_focused()`, `get_plane()`, `get_user_data()` | `set_user_data()`, `set_render_cb()`, `set_on_destroy()` |
| TuiTab | `get_name()`, `get_window_count()`, `get_active_window_index()` | — |
| TuiWorkspace | `get_name()`, `get_tab_count()`, `get_active_tab_index()` | — |

**Files:**
- NEW: `include/core/error.h`, `src/core/error.c`, `include/core/types_private.h`
- MODIFY: `types.h`, `manager.h`, `window.h`, `workspace.h`, `tab.h`, all widget headers, all test files

### 1.2 Error System

**Problem:** Functions return NULL with no way to know why. Error messages are in Spanish.

**Solution:** `TuiError` enum with thread-local error tracking.

```c
typedef enum {
    TUI_OK = 0,
    TUI_ERROR_NULL_ARGUMENT,
    TUI_ERROR_OUT_OF_MEMORY,
    TUI_ERROR_PLANE_CREATE_FAILED,
    TUI_ERROR_REPARENT_FAILED,
    TUI_ERROR_WIDGET_LIMIT_REACHED,
    TUI_ERROR_INVALID_INDEX,
    TUI_ERROR_INVALID_THEME,
    TUI_ERROR_FILE_NOT_FOUND,
} TuiError;

TuiError tui_last_error(void);
const char* tui_last_error_msg(void);
void tui_clear_error(void);
```

Internal macro for setting errors:
```c
#define TUI_CHECK(ptr, error_code, msg_fmt, ...) \
    do { if (!(ptr)) { _tui_set_error(error_code, msg_fmt, ##__VA_ARGS__); return NULL; } } while(0)
```

### 1.3 English Standardization

All code comments, log messages, and user-facing error messages standardized to English. Demo app content (window titles, tab names) remains as application content (optional language).

**Files affected:** `src/main.c`, `src/core/manager/*.c`, `src/core/tab.c`, `src/core/workspace.c`, `src/core/window.c`, `examples/demo.c`

---

## FASE 2: Event System (Widget VTable)

### 2.1 Widget Interface

Replaces the giant switch statement in `input.c` with polymorphic dispatch.

```c
struct TuiWidgetIface {
    bool (*handle_key)(void* widget, uint32_t key, const struct ncinput* ni);
    bool (*handle_mouse)(void* widget, uint32_t key, const struct ncinput* ni);
    void (*render)(void* widget);
    void (*on_focus)(void* widget);       /* optional */
    void (*on_blur)(void* widget);        /* optional */
    void (*on_resize)(void* widget, int h, int w); /* optional */
    bool (*is_focusable)(void* widget);   /* optional */
    bool (*needs_periodic_render)(void* widget); /* optional */
};
```

### 2.2 Auto-Registration

Each widget type registers its interface on first instance creation:
```c
static int s_button_type_id = -1;
void tui_button_register_type(void) {
    if (s_button_type_id < 0)
        s_button_type_id = tui_widget_register(&s_button_iface);
}
```

### 2.3 Focus Traversal

Tab/Shift+Tab cycles focusable widgets within the focused window. New field `focused_widget_index` in TuiWindow.

**Before:** Only input.c knows about widget types (violates Open/Closed Principle).
**After:** New widgets require zero changes to input.c.

**Files:**
- NEW: `include/core/widget.h`, `src/core/widget.c`
- MODIFY: `input.c` (eliminate switch), `window.c` (auto-detect type), `types.h` (add _type_id, focused_widget_index), all 8 widget files (add VTable), tests

---

## FASE 3: Layout Engine

### 3.1 Box Layout

Simple flexbox-style layout without constraint solver complexity.

```c
TuiLayout* tui_layout_create(TuiLayoutDirection direction);
void tui_layout_set_spacing(TuiLayout* layout, int spacing);
void tui_layout_set_padding(TuiLayout* layout, int top, int right, int bottom, int left);
void tui_layout_add_widget(TuiLayout* layout, void* widget, struct ncplane* plane,
                           TuiSizeConstraint size, float size_value, TuiAlignment alignment);
void tui_layout_add_layout(TuiLayout* layout, TuiLayout* child, ...);
void tui_layout_compute(TuiLayout* layout, int available_height, int available_width);
void tui_layout_render(TuiLayout* layout);
void tui_window_attach_layout(TuiWindow* win, TuiLayout* layout);
```

**Size constraints:** FIXED, FILL, WEIGHT(n), PERCENT(n), AUTO
**Alignment:** START, CENTER, END, STRETCH

### 3.2 Size Negotiation

Each widget implements `preferred_size(max_h, max_w, &out_h, &out_w)`:
- Label: 1 row, text width
- Text input: 1 row, fills width
- Progress: 1 row, min 10 chars

### 3.3 Auto-Resize

On NCKEY_RESIZE, all attached layouts are recomputed automatically.

**Backward compatible:** Without layout, works exactly as before (manual ncplane_move_yx).

**Files:**
- NEW: `include/core/layout.h`, `src/core/layout.c`, `tests/test_layout.c`
- MODIFY: `types.h` (attached_layout field), `input.c` (NCKEY_RESIZE), `render.c`, all widgets (preferred_size callback)

---

## FASE 3b: Unicode & Input

### 3b.1 Full UTF-8

Replace ASCII-only buffer with UTF-8:
```c
char  buffer[TEXT_INPUT_MAX_LEN * 4];  /* UTF-8: up to 4 bytes per codepoint */
uint32_t codepoints[TEXT_INPUT_MAX_LEN]; /* Decoded for cursor navigation */
int len_bytes, len_codepoints, cursor_cp, scroll_offset;
```

Cursor navigation moves by codepoint, not byte. Rendering uses `ncstrwidth()` for visual column width.

### 3b.2 Text Selection

- Shift+Left/Right: select character
- Shift+Ctrl+Left/Right: select word
- Ctrl+A: select all
- Ctrl+C: copy selection (or all if no selection)
- Ctrl+X: cut selection (or all if no selection)
- Ctrl+V: paste from clipboard (replaces selection if active)
- Render selection with inverted colors

### 3b.3 Bracketed Paste

Enable `notcurses_set_paste_mode(nc, true)`. Paste received as complete string via `ni->paste` and `ni->str`.

### 3b.4 Delete Key

- Backspace: delete before cursor
- Delete: delete after cursor
- Ctrl+Backspace: delete word before
- Ctrl+Delete: delete word after

**Files:**
- NEW: `include/core/utf8.h`, `src/core/utf8.c`, `tests/test_utf8.c`, `tests/test_textarea.c`
- MODIFY: `text_input.c`, `textarea.c`, `input.c`, clipboard

---

## FASE 4: New Widgets

9 new widgets following the VTable pattern from FASE 2, implementing preferred_size from FASE 3, and supporting disabled/read-only state.

### Widget Inventory

| Widget | File | Complexity | Key Features |
|--------|------|------------|--------------|
| Slider | slider.h/c | Medium | Drag, keyboard, step, range, on_change callback |
| Dropdown | dropdown.h/c | High | Popup list, keyboard nav, auto-close |
| Radio Group | radio_group.h/c | Medium | Mutual exclusion, horizontal/vertical layout |
| Table | table.h/c | Very High | Columns, rows, sorting, scroll, row selection |
| Tree | tree.h/c | Very High | Expand/collapse, hierarchical, keyboard nav |
| Tab Container | tab_container.h/c | High | Tabs as widget, content area, on_change callback |
| Spinner | spinner.h/c | Low | Multiple styles, start/stop, periodic render |
| Dialog | dialog.h/c | High | Modal overlay, blocking/async, button callbacks |
| File Picker | file_picker.h/c | High | Directory navigation, filters, open/save modes |

### Existing Widget Improvements

- **List:** `tui_list_on_select()` callback
- **Progress:** `tui_progress_set_format()`, customizable fill/empty chars
- **All widgets:** `tui_widget_set_enabled()`, `tui_widget_set_read_only()`

**Implementation order:** Slider → Radio Group → Dropdown → Tab Container → Spinner → Dialog → Table → Tree → File Picker

**Files:** 18 new files (9 headers + 9 implementations + 9 test files), modifications to progress.h/c, list.h, widget.h (VTable additions)

---

## FASE 5: Main Loop & Developer Experience

### 5.1 Integrated Main Loop

```c
int tui_manager_run(TuiManager* mgr, const TuiLoopConfig* config);
void tui_manager_quit(TuiManager* mgr);
```

Configurable callbacks: on_frame, after_render, on_unhandled_key, on_exit_request, target_fps.

### 5.2 Customizable Keybindings

```c
typedef enum { ACTION_QUIT, ACTION_NEW_TAB, ACTION_NEXT_TAB, ... } TuiAction;

void tui_keymap_bind(TuiManager* mgr, uint32_t key, bool ctrl, bool alt, bool shift, TuiAction action);
void tui_keymap_bind_custom(TuiManager* mgr, uint32_t key, ..., void (*cb)(...), void* userdata);
void tui_keymap_reset_defaults(TuiManager* mgr);
```

### 5.3 Builder API

Option structs for each widget:
```c
TuiButton* btn = tui_button_create_ext(parent, &(TuiButtonOpts){
    .y = 2, .x = 4, .label = "OK",
    .on_click = my_callback, .userdata = &state,
});
```

### 5.4 Dynamic Widget Registry

Replace fixed 16-element arrays with dynamic allocation (same pattern as TuiTab/TuiWorkspace).

### 5.5 Documentation

All public functions: @brief, @param (with NULL policy), @return, @see, @since, @code example.

**Files:**
- NEW: `include/core/loop.h`, `src/core/loop.c`, `include/core/keymap.h`, `src/core/keymap.c`
- MODIFY: `manager.h`, `input.c`, all widget headers (Opts structs), `types.h` (dynamic registry), `window.c`, all examples

---

## FASE 6: Testing & CI/CD

### 6.1 Fuzz Testing

AFL++ targets for text_input, textarea, list. Seed corpus with edge cases. Nightly CI with 60-minute timeout. Invariants checked: null-terminated buffer, cursor in bounds, valid UTF-8, no crashes.

### 6.2 Property-Based Tests

Generate random input sequences and verify invariants:
- After any operation sequence, buffer is null-terminated
- Cursor always within [0, len_codepoints]
- UTF-8 roundtrip: insert chars → buffer matches expected
- Insert + backspace = original state

### 6.3 Performance Regression

Benchmark output tracked via github-action-benchmark. CI fails if >150% slower than baseline.

### 6.4 Thread Sanitizer

TSAN job for detecting data races (especially in logger with pthread_mutex).

### 6.5 Complete CI Pipeline

10 jobs: build-test (matrix 4x), static-analysis, format-check, asan-ubsan, tsan, valgrind, coverage, fuzz (nightly), benchmark (on push), arm64-cross-build, install-test.

### 6.6 New Test Files

25+ new test files covering: all 9 new widgets, layout engine, UTF-8, error system, keymap, VTable dispatch, textarea, dialog, property tests, fuzz targets.

**Files:** 20+ new test files, 3 fuzz targets, 3 new CI workflows, Makefile additions

---

## FASE 7: Polish & Production

### 7.1 LICENSE
MIT license file (currently only mentioned in header comments).

### 7.2 Shared Library Versioning
```
libtmlcs_tui.so → libtmlcs_tui.so.0 → libtmlcs_tui.so.0.5.0
```
With proper soname in linker flags.

### 7.3 CMake Support
`CMakeLists.txt` coexisting with Makefile. Supports static/shared libs, tests, install.

### 7.4 SECURITY.md
Security policy with vulnerability reporting process. Documents security boundaries (text input, file picker, logger, clipboard).

### 7.5 .editorconfig
Enforces 4-space indent, LF endings, UTF-8, trailing newline removal for .c/.h files.

### 7.6 Debug Tools
```c
void tui_debug_dump_planes(TuiManager* mgr);
void tui_debug_dump_widgets(const TuiWindow* win);
void tui_debug_dump_dirty_flags(const TuiManager* mgr);
void tui_debug_enable_timing(bool enable);
int tui_debug_get_last_render_time_us(void);
```
Enabled with `-DTUI_DEBUG` compile flag.

### 7.7 Basic Animations
Easing functions (linear, in-out, in, out) for window position animation. Used by dialog open/close, menu slide-in.

### 7.8 Accessibility
- Consistent focus ring using `ncplane_perimeter_double()`
- `ACCESSIBILITY_HIGH_CONTRAST` env var auto-detection
- Screen reader presence detection (AT-SPI/DBUS)
- Auto-apply highcontrast theme when detected

**Files:**
- NEW: `LICENSE`, `SECURITY.md`, `.editorconfig`, `CMakeLists.txt`, `include/core/debug.h`, `src/core/debug.c`, `include/core/anim.h`, `src/core/anim.c`
- MODIFY: `Makefile` (so versioning, new targets), `render.c` (focus ring, timing), `theme_loader.c` (auto-detect)

---

## Total Impact

| Metric | Count |
|--------|-------|
| New files | ~65 |
| Modified files | ~35 |
| New lines of code | ~8,000-12,000 |
| New tests | ~25 files, ~300 tests |
| New widgets | 9 |
| New public APIs | ~80 functions |

## Success Criteria for v1.0

- [ ] All tests pass (including fuzz and property tests)
- [ ] Zero crashes in 24h continuous fuzzing
- [ ] Benchmark within 150% of baseline
- [ ] ASAN/UBSAN/TSAN/Valgrind clean
- [ ] Code coverage > 85%
- [ ] Complete Doxygen without warnings
- [ ] `make install` functional with pkg-config
- [ ] CMake build functional
- [ ] 17 total widgets (8 existing + 9 new)
- [ ] Full Unicode support (emoji, CJK, accents)
- [ ] Integrated main loop functional
- [ ] Customizable keybindings
- [ ] Layout engine with nesting
- [ ] LICENSE file present
- [ ] SECURITY.md present
- [ ] CI with 10+ jobs
