# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- `make asan` target for AddressSanitizer + UBSan builds
- `make coverage` target for gcov/lcov coverage reports
- `make tidy` target for clang-tidy static analysis
- `make valgrind` target for memory leak detection
- GitHub Actions CI/CD pipeline with 6 parallel jobs
- `GRIP_AREA_START_X` and `TAB_LABEL_PADDING` constants to `theme.h`
- NULL validation for factory function parameters (`name`, `ws`, `manager`)
- `on_destroy` callback invocation in `tui_window_destroy` (fixes use-after-free)

### Fixed
- **CRITICAL**: Unsafe `realloc` pattern in `tui_tab_add_window` — now uses temp variable to prevent memory leak on OOM
- **CRITICAL**: Unsafe `realloc` pattern in `tui_workspace_add_tab` — same fix
- **CRITICAL**: `g_active_input` use-after-free — `on_destroy` callback now called on every destruction path
- Replaced 41 hardcoded color values with `theme.h` constants across `render.c`, `demo.c`, `text_input.c`
- Eliminated magic numbers in `input.c` and `render.c` — all use `theme.h` constants
- Removed duplicate `TEXT_INPUT_MAX_LEN` macro from `theme.h`
- Removed dead macros `WS_HIDDEN_X` (alias kept) and `WINDOW_HIDDEN_X` from `theme.h`

### Changed
- `tui_tab_add_window` and `tui_workspace_add_tab` now return early on realloc failure (no crash)
- `tui_window_destroy` now calls `on_destroy` callback before destroying the plane
- All color literals replaced with semantic constants (`THEME_BG_WINDOW`, `THEME_FG_DEFAULT`, etc.)

---

## [0.1.0] — 2026-02-19

### Initial Quality Baseline

- Clean build with `-Wall -Wextra -Werror`
- 3 unit tests for logger module (passing)
- Basic workspace/tab/window architecture
- Dracula theme with centralized constants
- Mouse and keyboard input handling
- Thread-safe ring buffer logger

[Unreleased]: https://github.com/tmlcs/tmlcs-tui/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/tmlcs/tmlcs-tui/releases/tag/v0.1.0
