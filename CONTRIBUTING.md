# Contributing to tmlcs-tui

Thank you for your interest in contributing to the tmlcs-tui framework!

## Getting Started

### Prerequisites

- **GCC** or **Clang** (C11 support required)
- **notcurses** >= 3.0 (`libnotcurses-dev` on Ubuntu/Debian)
- **GNU Make** >= 4.0
- **clang-tidy** (for static analysis, optional)
- **lcov** (for coverage reports, optional)
- **valgrind** (for memory checking, optional)

### Building

```bash
make clean && make all       # Build the main binary
make test-all                 # Run all unit tests (189 tests)
make asan                     # Build + test with ASAN + UBSan
make valgrind                 # Run tests under Valgrind
make coverage                 # Generate coverage report
make tidy                     # Run clang-tidy
```

## Code Style

### Formatting

We use `clang-format` with the project's `.clang-format` configuration:
- 4-space indentation (no tabs)
- 100 character line limit
- Braces on same line for functions

```bash
make format   # Auto-format all source files
```

### Naming Conventions

| Entity | Convention | Example |
|--------|-----------|---------|
| Functions | `tui_<module>_<action>` | `tui_tab_create()`, `tui_window_destroy()` |
| Types | `Tui<Name>` (PascalCase) | `TuiWindow`, `TuiWidgetType` |
| Macros/Constants | `UPPER_SNAKE_CASE` | `THEME_BG_DARK`, `MAX_WINDOW_WIDGETS` |
| Globals | `g_<name>` | `g_active_input` |
| Static locals | `s_<name>` | `s_next_id` |
| Widget types | `WIDGET_<NAME>` | `WIDGET_BUTTON`, `WIDGET_LIST` |

### Header Guards

Use `#ifndef`/`#define`/`#endif` with path-based names:
```c
#ifndef CORE_TYPES_H
#define CORE_TYPES_H
// ...
#endif /* CORE_TYPES_H */
```

### Documentation

All public API functions must have Doxygen comments:
```c
/**
 * @brief Brief description of the function.
 * @param param_name Description of parameter.
 * @return Description of return value.
 */
int tui_something_do(TuiManager* mgr, const char* param_name);
```

## Adding a New Widget

1. Create `include/widget/mywidget.h` with the public API
2. Create `src/widget/mywidget.c` with the implementation
3. Add the `.c` file to `WIDGET_SRCS` in the Makefile (if needed for test builds)
4. Register the widget type in `TuiWidgetType` enum in `include/core/types.h`
5. Add mouse handler in `src/core/manager/input.c` (if applicable)
6. Write tests in `tests/test_mywidget.c`
7. Add Doxygen documentation to the header

## Pull Request Checklist

- [ ] `make clean && make all` — clean build, 0 warnings
- [ ] `make test-all` — all tests passing
- [ ] `make asan` — no memory errors under ASAN + UBSan
- [ ] `make tidy` — no clang-tidy warnings (or justified suppressions)
- [ ] New code follows naming conventions (`tui_<module>_<action>`)
- [ ] Public APIs have Doxygen comments
- [ ] CHANGELOG.md updated (if adding features)

## Running Individual Tests

```bash
./build/test_widgets_runner      # Widget tests only
./build/test_context_menu_runner # Context menu tests only
./build/benchmark_render_runner  # Performance benchmark
```

## Quality Standards

This project maintains an **8.9/10** quality score. See `docs/AUDIT_2026_04_v3.md`
for the full audit. Key principles:

- **Zero unsafe C functions** — no `strcpy`, `sprintf`, `gets`, raw `scanf`
- **Zero code debt markers** — no `TODO`, `FIXME`, `XXX`, `HACK`
- **100% NULL-safe public API** — all public functions handle NULL gracefully
- **ASAN/UBSan/Valgrind clean** — no memory errors, leaks, or UB
- **189+ tests across 15 suites** — comprehensive edge case coverage

## Communication

- Open an **Issue** for bugs, feature requests, or questions
- Open a **Pull Request** for code changes
- Reference the relevant audit finding (e.g., `PERF-01`, `TEST-01`) when applicable

## License

See the project's LICENSE file.
