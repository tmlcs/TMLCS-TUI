# TMLCS-TUI

> Terminal User Interface framework with Workspace → Tab → Window architecture, built on [notcurses](https://github.com/dankamongmen/notcurses).

[![CI](https://github.com/tmlcs/tmlcs-tui/actions/workflows/ci.yml/badge.svg)](https://github.com/tmlcs/tmlcs-tui/actions/workflows/ci.yml)

## Features

- **Workspace management**: Multiple workspaces with independent tab sets
- **Tab system**: Switch between tabs, each containing multiple windows
- **Window management**: Draggable, resizable windows with z-ordering
- **8 Widgets**: Label, Button, Checkbox, Progress, List, Text Input, TextArea, Context Menu
- **Widget mouse support**: Click to activate buttons, toggle checkboxes, select list items
- **Cursor positioning**: Click in text input to position cursor
- **Thread-safe logger**: Ring buffer logger with file output
- **Dracula theme**: Centralized color constants for consistent theming
- **Mouse support**: Click, drag, resize via mouse
- **Keyboard shortcuts**: Full keyboard navigation and management

## Dependencies

- GCC or Clang
- [notcurses](https://github.com/dankamongmen/notcurses) ≥ 3.0
- POSIX threads (`libpthread`)

### Ubuntu/Debian

```bash
sudo apt install build-essential libnotcurses-dev
```

## Build

```bash
make clean && make
```

## Run

```bash
./mi_tui
```

## Keybindings

| Key | Action |
|-----|--------|
| `q` / `Q` | Quit application |
| `Alt+1..9` | Switch workspace |
| `←` / `→` | Navigate tabs |
| `Tab` / `Shift+Tab` | Cycle window focus (forward/reverse) |
| `x` | Close focused window |
| `w` | Close active tab |
| `d` | Close active workspace |
| `Ctrl+T` | New tab |
| `Ctrl+N` | New window |
| `Ctrl+Shift+Arrows` | Resize focused window |
| `F1` / `?` | Toggle help overlay |
| `Enter` | Submit text input |
| `Backspace` | Delete character |
| `ESC` | Cancel drag/resize/help |

## Mouse Controls

| Action | How |
|--------|-----|
| Switch tab | Click on tab name |
| Focus window | Click anywhere on window |
| Drag window | Click and drag title bar |
| Resize window | Click and drag bottom-right grip (`⌟`) |
| Close window | Click `[X]` button |
| Click button | Click on button activates callback |
| Toggle checkbox | Click on checkbox toggles state |
| Select list item | Click on item selects it |
| Context menu item | Click on item activates and closes |
| Position cursor | Click in text input sets cursor |
| Cycle tabs | Scroll wheel up/down |

## Architecture

```
include/
├── core/
│   ├── types.h           # Core structs + widget type enum
│   ├── theme.h           # Color/layout constants (Dracula theme)
│   ├── logger.h          # Thread-safe ring buffer logger
│   ├── manager.h         # Manager public API
│   ├── workspace.h       # Workspace API
│   ├── tab.h             # Tab API
│   ├── window.h          # Window API + widget registry
│   ├── clipboard.h       # Clipboard (OSC 52)
│   ├── help.h            # Help overlay
│   └── theme_loader.h    # Theme loading
└── widget/
    ├── text_input.h      # Text input widget
    ├── textarea.h        # Multi-line text area
    ├── button.h          # Clickable button
    ├── checkbox.h        # Toggle checkbox
    ├── list.h            # Scrollable list
    ├── label.h           # Text label
    ├── progress.h        # Progress bar
    └── context_menu.h    # Context/popup menu

src/
├── core/
│   ├── manager/
│   │   ├── state.c       # Manager lifecycle, workspace mgmt
│   │   ├── render.c      # Full UI rendering
│   │   ├── input.c       # Mouse + keyboard event routing
│   │   └── utils.c       # Helper functions
│   ├── workspace.c       # Workspace lifecycle
│   ├── tab.c             # Tab lifecycle
│   ├── window.c          # Window lifecycle + widget registry
│   ├── logger.c          # Ring buffer logger
│   ├── clipboard.c       # Clipboard (OSC 52)
│   ├── help.c            # Help overlay
│   └── theme_loader.c    # Theme loading
├── widget/
│   ├── text_input.c      # Text input widget
│   ├── textarea.c        # Multi-line text area
│   ├── button.c          # Clickable button
│   ├── checkbox.c        # Toggle checkbox
│   ├── list.c            # Scrollable list
│   ├── label.c           # Text label
│   ├── progress.c        # Progress bar
│   └── context_menu.c    # Context/popup menu
└── main.c                # Entry point (delegates to examples/demo.c)

examples/
├── demo.c                # Full demo application
├── hello_world.c         # Minimal hello world
├── tabs_demo.c           # Tabs-focused demo
└── text_input_demo.c     # Text input demo

tests/
├── nc_stubs.c            # Notcurses mock/stub infrastructure
├── test_logger.c         # Logger tests
├── test_logger_extended.c
├── test_tab.c            # Tab CRUD tests
├── test_workspace.c      # Workspace tests
├── test_window.c         # Window lifecycle tests
├── test_manager_*.c      # Manager state/input/render/utils tests
├── test_integration.c    # Full cross-module lifecycle tests
├── test_text_input.c     # Text input + mouse tests
└── test_widgets.c        # Widget tests (6 widgets)
```

## Testing

```bash
make test          # Run unit tests
make asan          # Build with AddressSanitizer + UBSan and run tests
make valgrind      # Run Valgrind memory check on tests
make coverage      # Generate gcov/lcov coverage report
make tidy          # Run clang-tidy static analysis
```

## CI/CD

This project uses GitHub Actions with a 6-job pipeline:

| Job | Purpose |
|-----|---------|
| Build & Test | GCC + Clang × Ubuntu 22.04 + 24.04 |
| Static Analysis | clang-tidy with project `.clang-tidy` config |
| ASAN + UBSan | Memory safety and undefined behavior detection |
| Code Coverage | gcov/lcov coverage report |
| Valgrind | Memory leak detection |
| Clang Build | Cross-compiler compatibility check |

## Quality Score

| Category | Score |
|----------|-------|
| Build System | 9/10 |
| Memory Safety | 10/10 |
| Buffer Safety | 10/10 |
| Code Cleanliness | 9/10 |
| Modularity | 8/10 |
| Testing | 9/10 |
| Documentation | 8/10 |
| API Design | 8/10 |
| **Overall** | **8.9/10** |

## Documentation

- **[API Reference](docs/API.md)** — Complete function reference for all 120+ public APIs
- **[Doxygen](docs/api/)** — Auto-generated API docs (`make docs`)
- **[Changelog](CHANGELOG.md)** — Version history with all changes

## License

MIT
