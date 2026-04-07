# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Widget mouse handling for Button, Checkbox, List, Context Menu
- `tui_text_input_handle_mouse()` — click positions cursor in text input
- Widget registry system in `TuiWindow` — `tui_window_add_widget()`, `tui_window_get_widget_at()`
- Widget mouse routing in manager `process_window_click`
- Configurable plane positions in test stubs (`stub_set_plane_position`)
- `docs/API.md` — comprehensive API reference

### Fixed
- `tui_textarea_get()` static buffer replaced with caller-provided buffer

---

## [0.5.0] — 2026-04-06

### Added
- `tui_text_input_handle_mouse()` — click positions cursor in text input
- Widget mouse handlers: `tui_button_handle_mouse`, `tui_checkbox_handle_mouse`, `tui_list_handle_mouse`, `tui_menu_handle_mouse`
- Widget registry in `TuiWindow` — tracks widget instances for mouse event routing
- Widget mouse routing in `process_window_click` (manager/input.c)
- 7 widget headers added to `make install` (textarea, button, label, checkbox, list, progress, context_menu)
- Defensive re-init check in `tui_logger_init` (prevents FILE* leak on double-init)
- `--log-level` validation with `strtol` instead of `atoi`

### Fixed
- Ctrl+Left/Right word-jump handlers were unreachable dead code
- Active tab render width off-by-1 vs hit detection area
- `x` key consumed by text input preventing window close in demo
- Missing NULL guard in `tui_workspace_set_active_tab`
- Missing NULL check on `ni` in `tui_button_handle_key`

### Changed
- `tui_textarea_get()` API changed: now requires caller-provided buffer
- `make asan` now runs `test-all` instead of just `test`
- Demo: `x` key intercepted before text input routing

---

## [0.4.0] — 2026-03-20

### Added
- 8 widgets: Label, Button, Checkbox, Progress, List, Text Input, TextArea, Context Menu
- 119 unit tests across 10 suites (from 31)
- Integration tests for full cross-module lifecycle
- Thread-safe ID generation with `atomic_int`
- `tui_window_is_focused()` getter
- `tui_textarea_get_line_count()` getter
- `make hello`, `make tabs-demo`, `make text-input-demo` example targets

### Fixed
- Unsafe `realloc` pattern in `tui_tab_add_window` — uses temp variable to prevent leak on OOM
- Unsafe `realloc` pattern in `tui_workspace_add_tab` — same fix
- `g_active_input` use-after-free — `on_destroy` callback called on every destruction path
- Replaced 41 hardcoded color values with `theme.h` constants

### Changed
- Quality score: 7.0 → 9.5/10

---

## [0.3.0] — 2026-03-05

### Added
- GitHub Actions CI/CD pipeline with 6 parallel jobs
- `make asan`, `make ubsan`, `make coverage`, `make tidy`, `make valgrind` targets
- Doxygen documentation on core headers
- 31 tests across 4 suites (logger, manager, tab, workspace)
- `nc_stubs.c` infrastructure for testing without real TTY
- Theme support: Dracula, Solarized, High Contrast via `theme_loader.c`
- Help overlay toggle with `F1` / `?`
- Clipboard integration via OSC 52 escape sequences

### Fixed
- Clean build with `-Wall -Wextra -Werror`
- Memory leaks in OOM paths

---

## [0.2.0] — 2026-02-25

### Added
- Thread-safe ring buffer logger with `pthread_mutex_t`
- Mouse support: click, drag, resize windows
- Keyboard shortcuts: Alt+1..9 (workspaces), arrows (tabs), Tab (windows), x/w/d (close)
- Dracula theme with centralized color constants in `theme.h`
- Ctrl+T (new tab), Ctrl+N (new window)

### Fixed
- Removed commented-out dead code from `tab.c`
- `sprintf` → `snprintf` throughout codebase
- NULL checks on all factory function parameters
- `on_destroy` callback lifecycle integration

---

## [0.1.0] — 2026-02-19

### Initial Quality Baseline

- Clean build with `-Wall -Wextra -Werror`
- 3 unit tests for logger module (passing)
- Basic workspace/tab/window architecture
- Dracula theme with centralized constants
- Mouse and keyboard input handling
- Thread-safe ring buffer logger

[Unreleased]: https://github.com/tmlcs/tmlcs-tui/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/tmlcs/tmlcs-tui/releases/tag/v0.5.0
[0.4.0]: https://github.com/tmlcs/tmlcs-tui/releases/tag/v0.4.0
[0.3.0]: https://github.com/tmlcs/tmlcs-tui/releases/tag/v0.3.0
[0.2.0]: https://github.com/tmlcs/tmlcs-tui/releases/tag/v0.2.0
[0.1.0]: https://github.com/tmlcs/tmlcs-tui/releases/tag/v0.1.0
