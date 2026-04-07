CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude -g -O2
CFLAGS += -D_FORTIFY_SOURCE=2 -fstack-protector-strong -Wformat-security
LDFLAGS = -lnotcurses -lnotcurses-core -pthread
LDFLAGS += -Wl,-z,relro,-z,now

# Library versioning
VERSION_MAJOR = 0
VERSION_MINOR = 5
VERSION_PATCH = 0
VERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

# Test framework
TEST_SRC = tests/test_logger.c
TEST_OBJ = $(BUILD_DIR)/test_logger.o
TEST_TARGET = $(BUILD_DIR)/test_runner

SRC_DIR = src
EXAMPLES_DIR = examples
INC_DIR = include
BUILD_DIR = build

# Encuentra todos los .c recursivamente usando bash find
# Demo sources — all workspace files + common + demo entry point
DEMO_SRCS = examples/demo.c examples/common.c \
            examples/demo_system.c examples/demo_console.c \
            examples/demo_desktop.c examples/demo_finance.c \
            examples/demo_browse.c examples/demo_development.c \
            examples/demo_agents.c examples/demo_log.c \
            examples/demo_files.c

DEMO_OBJS = $(BUILD_DIR)/examples/demo.o $(BUILD_DIR)/examples/common.o \
            $(BUILD_DIR)/examples/demo_system.o $(BUILD_DIR)/examples/demo_console.o \
            $(BUILD_DIR)/examples/demo_desktop.o $(BUILD_DIR)/examples/demo_finance.o \
            $(BUILD_DIR)/examples/demo_browse.o $(BUILD_DIR)/examples/demo_development.o \
            $(BUILD_DIR)/examples/demo_agents.o $(BUILD_DIR)/examples/demo_log.o \
            $(BUILD_DIR)/examples/demo_files.o

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OTHER_EXAMPLE_SRCS := $(shell find $(EXAMPLES_DIR) -name '*.c' ! -name 'demo.c' ! -name 'common.c' ! -name 'workspace_*.c')
ALL_SRCS := $(SRCS) $(OTHER_EXAMPLE_SRCS) $(DEMO_SRCS)

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
OTHER_EXAMPLE_OBJS := $(patsubst $(EXAMPLES_DIR)/%.c, $(BUILD_DIR)/examples/%.o, $(OTHER_EXAMPLE_SRCS))
ALL_OBJS := $(OBJS) $(OTHER_EXAMPLE_OBJS) $(DEMO_OBJS)

TARGET = mi_tui

# ============================================
# Library targets
# ============================================
LIB_STATIC = libtmlcs_tui.a
LIB_SHARED = libtmlcs_tui.so
LIB_SHARED_VERSIONED = libtmlcs_tui.so.$(VERSION)
LIB_SHARED_SONAME = libtmlcs_tui.so.$(VERSION_MAJOR)

PREFIX ?= /usr/local
INSTALL_DIR = $(PREFIX)/include/tmlcs-tui

.PHONY: all clean run test asan ubsan coverage tidy valgrind install uninstall lib shared docs fmt

all: $(TARGET)

lib: $(LIB_STATIC)

$(LIB_STATIC): $(OBJS)
	ar rcs $@ $^

shared: CFLAGS += -fPIC
shared: LDFLAGS += -shared
shared: $(LIB_SHARED)

$(LIB_SHARED): $(LIB_SHARED_VERSIONED)
	ln -sf $(LIB_SHARED_VERSIONED) $(LIB_SHARED_SONAME)
	ln -sf $(LIB_SHARED_VERSIONED) $(LIB_SHARED)

$(LIB_SHARED_VERSIONED): $(OBJS)
	$(CC) -shared -Wl,-soname,$(LIB_SHARED_SONAME) -o $@ $^ $(LDFLAGS)

$(TARGET): $(OBJS) $(BUILD_DIR)/main.o $(DEMO_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/examples/%.o: $(EXAMPLES_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Hello world example
$(BUILD_DIR)/examples/hello_world.o: examples/hello_world.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/examples/hello_world: $(BUILD_DIR)/examples/hello_world.o $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: hello
hello: build/examples/hello_world
	./build/examples/hello_world

# Tabs demo example
$(BUILD_DIR)/examples/tabs_demo.o: examples/tabs_demo.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/examples/tabs_demo: $(BUILD_DIR)/examples/tabs_demo.o $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: tabs-demo
tabs-demo: build/examples/tabs_demo
	./build/examples/tabs_demo

# Text input demo example
$(BUILD_DIR)/examples/text_input_demo.o: examples/text_input_demo.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/examples/text_input_demo: $(BUILD_DIR)/examples/text_input_demo.o $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: text-input-demo
text-input-demo: build/examples/text_input_demo
	./build/examples/text_input_demo

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: all
	./$(TARGET)

# ============================================
# Testing rules
# ============================================
.PHONY: test test-all

TEST_SRCS = $(wildcard tests/test_*.c)
TEST_RUNNERS = $(patsubst tests/%.c, $(BUILD_DIR)/%_runner, $(TEST_SRCS))

# Shared stubs for notcurses
NC_STUBS = $(BUILD_DIR)/nc_stubs.o

test: $(TEST_TARGET)
	@echo "Running tests..."
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJ) $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/test_logger.o: tests/test_logger.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Build nc_stubs
$(BUILD_DIR)/nc_stubs.o: tests/nc_stubs.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Generic rule: each test_*.c → *_runner (linked with notcurses for core objects)
$(BUILD_DIR)/test_%_runner: tests/test_%.c $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS)) $(NC_STUBS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Tab and workspace tests: compile core sources directly with stubs (no notcurses link)
TAB_WS_CORE_SRCS = src/core/logger.c src/core/tab.c src/core/workspace.c src/core/window.c src/core/layout.c src/core/widget.c src/core/manager/state.c src/core/manager/utils.c src/core/manager/render.c src/core/theme_loader.c src/core/help.c src/core/clipboard.c src/core/keymap.c

# Widget sources needed when manager/input.c references widget mouse handlers
WIDGET_MOUSE_SRCS = src/widget/button.c src/widget/checkbox.c src/widget/list.c src/widget/context_menu.c src/widget/textarea.c src/widget/text_input.c src/core/widget.c src/core/utf8.c

$(BUILD_DIR)/test_tab_runner: tests/test_tab.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_tab.c $(TAB_WS_CORE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_workspace_runner: tests/test_workspace.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_workspace.c $(TAB_WS_CORE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_window_runner: tests/test_window.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_window.c $(TAB_WS_CORE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Manager state tests: core sources + stubs
$(BUILD_DIR)/test_manager_state_runner: tests/test_manager_state.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_manager_state.c $(TAB_WS_CORE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Manager input tests: core sources (including manager/input.c) + stubs
MANAGER_INPUT_CORE_SRCS = src/core/logger.c src/core/tab.c src/core/workspace.c src/core/window.c src/core/layout.c src/core/manager/input.c src/core/manager/state.c src/core/manager/utils.c src/core/manager/render.c src/core/theme_loader.c src/core/help.c src/core/clipboard.c src/core/keymap.c

$(BUILD_DIR)/test_manager_input_runner: tests/test_manager_input.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_manager_input.c $(MANAGER_INPUT_CORE_SRCS) $(WIDGET_MOUSE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Manager render tests: core sources (including manager/render.c) + stubs
MANAGER_RENDER_CORE_SRCS = src/core/logger.c src/core/tab.c src/core/workspace.c src/core/window.c src/core/layout.c src/core/manager/render.c src/core/manager/input.c src/core/manager/state.c src/core/manager/utils.c src/core/theme_loader.c src/core/help.c src/core/clipboard.c src/core/keymap.c

$(BUILD_DIR)/test_manager_render_runner: tests/test_manager_render.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_manager_render.c $(MANAGER_RENDER_CORE_SRCS) $(WIDGET_MOUSE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Manager utils tests: core sources + stubs
MANAGER_UTILS_CORE_SRCS = src/core/logger.c src/core/tab.c src/core/workspace.c src/core/window.c src/core/layout.c src/core/widget.c src/core/manager/state.c src/core/manager/utils.c src/core/manager/render.c src/core/theme_loader.c src/core/help.c src/core/clipboard.c src/core/keymap.c

$(BUILD_DIR)/test_manager_utils_runner: tests/test_manager_utils.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_manager_utils.c $(MANAGER_UTILS_CORE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Integration tests: full cross-module lifecycle with widget + manager input/render
MANAGER_FULL_CORE_SRCS = src/core/logger.c src/core/tab.c src/core/workspace.c src/core/window.c src/core/layout.c src/core/manager/state.c src/core/manager/utils.c src/core/manager/input.c src/core/manager/render.c src/core/theme_loader.c src/core/help.c src/core/clipboard.c src/core/keymap.c src/core/loop.c

$(BUILD_DIR)/test_integration_runner: tests/test_integration.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_integration.c $(MANAGER_FULL_CORE_SRCS) $(WIDGET_MOUSE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Text input tests: widget + logger + stubs + utf8
$(BUILD_DIR)/test_text_input_runner: tests/test_text_input.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_text_input.c src/widget/text_input.c src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude -g -lpthread

# Widgets tests: all 15 widgets + logger + stubs + math + widget.c for VTable
WIDGET_SRCS = src/widget/label.c src/widget/button.c src/widget/progress.c src/widget/list.c src/widget/textarea.c src/widget/checkbox.c src/widget/context_menu.c src/widget/slider.c src/widget/radio_group.c src/widget/spinner.c src/widget/tab_container.c src/widget/dropdown.c src/widget/dialog.c src/widget/table.c src/widget/tree.c src/widget/file_picker.c

$(BUILD_DIR)/test_widgets_runner: tests/test_widgets.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_widgets.c $(WIDGET_SRCS) src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude -g -lpthread -lm

# Context menu tests: context_menu widget + logger + stubs + widget.c for VTable
$(BUILD_DIR)/test_context_menu_runner: tests/test_context_menu.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_context_menu.c src/widget/context_menu.c src/core/widget.c src/core/logger.c tests/nc_stubs.c -Iinclude -g -lpthread

# Layout tests: layout.c + all widget sources + logger + stubs
LAYOUT_WIDGET_SRCS = src/widget/label.c src/widget/button.c src/widget/progress.c src/widget/list.c src/widget/textarea.c src/widget/checkbox.c src/widget/context_menu.c src/widget/text_input.c src/widget/slider.c src/widget/radio_group.c src/widget/spinner.c src/widget/tab_container.c src/widget/dropdown.c src/widget/dialog.c src/widget/table.c src/widget/tree.c src/widget/file_picker.c

$(BUILD_DIR)/test_layout_runner: tests/test_layout.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_layout.c src/core/layout.c $(LAYOUT_WIDGET_SRCS) src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude -g -lpthread -lm

# Phase 4 Widget tests
NEW_WIDGET_CORE_SRCS = src/widget/slider.c src/widget/radio_group.c src/widget/spinner.c src/widget/tab_container.c src/widget/dropdown.c src/widget/dialog.c src/widget/table.c src/widget/tree.c src/widget/file_picker.c src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c

$(BUILD_DIR)/test_slider_runner: tests/test_slider.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_slider.c src/widget/slider.c src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude -g -lpthread -lm

$(BUILD_DIR)/test_radio_group_runner: tests/test_radio_group.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_radio_group.c src/widget/radio_group.c src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_spinner_runner: tests/test_spinner.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_spinner.c src/widget/spinner.c src/core/widget.c src/core/logger.c tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_tab_container_runner: tests/test_tab_container.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_tab_container.c src/widget/tab_container.c src/core/widget.c src/core/logger.c tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_dropdown_runner: tests/test_dropdown.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_dropdown.c src/widget/dropdown.c src/core/widget.c src/core/logger.c tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_dialog_runner: tests/test_dialog.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_dialog.c src/widget/dialog.c src/core/widget.c src/core/logger.c tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_table_runner: tests/test_table.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_table.c src/widget/table.c src/core/widget.c src/core/logger.c tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_tree_runner: tests/test_tree.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_tree.c src/widget/tree.c src/core/widget.c src/core/logger.c tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_file_picker_runner: tests/test_file_picker.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_file_picker.c src/widget/file_picker.c src/core/widget.c src/core/logger.c tests/nc_stubs.c -Iinclude -g -lpthread

# Property tests: same deps as text_input tests (widget + logger + stubs + utf8)
$(BUILD_DIR)/property_text_input_runner: tests/property/test_text_input_properties.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/property/test_text_input_properties.c src/widget/text_input.c src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude -g -lpthread

# UTF-8 property tests: pure C (no stubs needed)
$(BUILD_DIR)/property_utf8_runner: tests/property/test_utf8_properties.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/property/test_utf8_properties.c src/core/utf8.c -Iinclude -g -lpthread

# Keymap tests: core sources + stubs (needs manager struct for keymap)
KEYMAP_CORE_SRCS = src/core/logger.c src/core/keymap.c src/core/clipboard.c

$(BUILD_DIR)/test_keymap_runner: tests/test_keymap.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_keymap.c $(KEYMAP_CORE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Loop tests: core sources + stubs (needs manager struct for loop)
LOOP_CORE_SRCS = src/core/logger.c src/core/loop.c src/core/keymap.c src/core/manager/state.c src/core/manager/utils.c src/core/manager/input.c src/core/manager/render.c src/core/clipboard.c src/core/workspace.c src/core/tab.c src/core/window.c src/core/layout.c src/core/help.c src/core/theme_loader.c

$(BUILD_DIR)/test_loop_runner: tests/test_loop.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_loop.c $(LOOP_CORE_SRCS) $(WIDGET_MOUSE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Fuzz test runners (compiled with sanitizer flags and libFuzzer)
FUZZ_CFLAGS = -fsanitize=address,undefined,fuzzer -fno-omit-frame-pointer -g -O1
FUZZ_LDFLAGS = -fsanitize=address,undefined,fuzzer

$(BUILD_DIR)/fuzz_text_input_runner: tests/fuzz/fuzz_text_input.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(FUZZ_CFLAGS) -o $@ tests/fuzz/fuzz_text_input.c src/widget/text_input.c src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude $(FUZZ_LDFLAGS) -lpthread

$(BUILD_DIR)/fuzz_textarea_runner: tests/fuzz/fuzz_textarea.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(FUZZ_CFLAGS) -o $@ tests/fuzz/fuzz_textarea.c src/widget/textarea.c src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude $(FUZZ_LDFLAGS) -lpthread

$(BUILD_DIR)/fuzz_list_runner: tests/fuzz/fuzz_list.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(FUZZ_CFLAGS) -o $@ tests/fuzz/fuzz_list.c src/widget/list.c src/core/widget.c src/core/logger.c src/core/clipboard.c src/core/utf8.c tests/nc_stubs.c -Iinclude $(FUZZ_LDFLAGS) -lpthread

# Logger tests don't need stubs (pure C, no notcurses)
$(BUILD_DIR)/test_logger_runner: tests/test_logger.c $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/test_logger_extended_runner: tests/test_logger_extended.c $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Theme loader tests: pure C + logger (no stubs needed)
$(BUILD_DIR)/test_theme_loader_runner: tests/test_theme_loader.c src/core/theme_loader.c src/core/logger.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_theme_loader.c src/core/theme_loader.c src/core/logger.c -Iinclude -g -lpthread -lm

# Clipboard tests: pure C + logger (no stubs needed)
$(BUILD_DIR)/test_clipboard_runner: tests/test_clipboard.c src/core/clipboard.c src/core/logger.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_clipboard.c src/core/clipboard.c src/core/logger.c -Iinclude -g -lpthread

# UTF-8 tests: pure C (no stubs needed)
$(BUILD_DIR)/test_utf8_runner: tests/test_utf8.c src/core/utf8.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_utf8.c src/core/utf8.c -Iinclude -g -lpthread

# Render benchmark: uses logger + core types (no notcurses stubs needed)
$(BUILD_DIR)/benchmark_render_runner: tests/benchmark_render.c src/core/logger.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/benchmark_render.c src/core/logger.c -Iinclude -g -lpthread

benchmark: $(BUILD_DIR)/benchmark_render_runner
	@echo "--- Running Render Benchmark ---"
	@./$(BUILD_DIR)/benchmark_render_runner

check: test-all
	@:

# Run ALL tests
test-all: $(TEST_RUNNERS)
	@echo "Running all tests..."
	@FAILED=0; \
	for runner in $(TEST_RUNNERS); do \
		echo "--- Running $$runner ---"; \
		./$$runner || FAILED=$$((FAILED+1)); \
	done; \
	echo ""; \
	if [ $$FAILED -gt 0 ]; then \
		echo "FAILED: $$FAILED test suite(s)"; exit 1; \
	else \
		echo "ALL TESTS PASSED"; \
	fi

# ============================================
# Sanitizer builds
# ============================================
SAN_CFLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1
SAN_LDFLAGS = -fsanitize=address,undefined

.PHONY: asan ubsan

asan: CFLAGS += $(SAN_CFLAGS)
asan: LDFLAGS += $(SAN_LDFLAGS)
asan: clean all test-all

ubsan: CFLAGS += -fsanitize=undefined -fno-omit-frame-pointer -g -O1
ubsan: LDFLAGS += -fsanitize=undefined
ubsan: clean all test-all

# ThreadSanitizer
.PHONY: tsan
tsan: CFLAGS += -fsanitize=thread -g
tsan: LDFLAGS += -fsanitize=thread
tsan: clean test-all

# Fuzz build: compile fuzz targets with ASAN+UBSAN
.PHONY: fuzz-build
fuzz-build: CFLAGS += -fsanitize=address,undefined -g
fuzz-build: $(BUILD_DIR)/fuzz_text_input_runner $(BUILD_DIR)/fuzz_textarea_runner $(BUILD_DIR)/fuzz_list_runner

# ============================================
# Format check
# ============================================
FMT_CHECK_FILES := $(shell find src include examples tests -name '*.c' -o -name '*.h' 2>/dev/null)

.PHONY: fmt-check
fmt-check:
	find src include examples tests -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror

# ============================================
# Code coverage
# ============================================
COV_CFLAGS = --coverage -fprofile-arcs -ftest-coverage
COV_LDFLAGS = --coverage

.PHONY: coverage

coverage: CFLAGS += $(COV_CFLAGS)
coverage: LDFLAGS += $(COV_LDFLAGS)
coverage: clean all test-all
	@echo "Generating coverage report..."
	lcov --capture --directory . --output-file build/coverage.info --no-external --quiet
	lcov --remove build/coverage.info '/usr/*' '*/tests/*' --output-file build/coverage_filtered.info --quiet
	genhtml build/coverage_filtered.info --output-directory build/coverage_report --quiet
	@echo "Coverage report generated at build/coverage_report/index.html"
	@lcov --list build/coverage_filtered.info | tail -1

# ============================================
# Static analysis (clang-tidy)
# ============================================
TIDY_FILES := $(shell find src include -name '*.c' -o -name '*.h')

.PHONY: tidy

tidy:
	@echo "Running clang-tidy..."
	@FAILED=0; \
	for f in $(TIDY_FILES); do \
		echo "  $$f"; \
		clang-tidy $$f -- $(CFLAGS) 2>&1 || FAILED=$$((FAILED+1)); \
	done; \
	echo ""; \
	echo "clang-tidy complete: $$FAILED file(s) with warnings"; \
	if [ $$FAILED -gt 0 ]; then exit 1; fi

# ============================================
# Valgrind memory check
# ============================================
VALGRIND_OPTS = --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	--error-exitcode=1

.PHONY: valgrind

valgrind: all test-all
	@echo "Running valgrind on all test runners..."
	@mkdir -p build
	@FAILED=0; \
	for runner in $(TEST_RUNNERS); do \
		echo "  Valgrind: $$runner"; \
		valgrind $(VALGRIND_OPTS) ./$$runner 2>>build/valgrind_report.txt || FAILED=$$((FAILED+1)); \
	done; \
	if [ $$FAILED -gt 0 ]; then \
		echo "Valgrind detected memory errors in $$FAILED suite(s)!"; \
		cat build/valgrind_report.txt; exit 1; \
	fi
	@echo "Valgrind report: build/valgrind_report.txt"

# ============================================
# Documentation (Doxygen)
# ============================================
.PHONY: docs

docs:
	@echo "Generating Doxygen documentation..."
	@doxygen Doxyfile 2>build/doxygen_warnings.txt || echo "Doxygen not found or failed"
	@echo "Docs generated at docs/api/"

# ============================================
# Code formatting (clang-format)
# ============================================
FMT_FILES := $(shell find src include examples -name '*.c' -o -name '*.h' 2>/dev/null)

.PHONY: fmt

fmt:
	@echo "Running clang-format..."
	@clang-format -i $(FMT_FILES)
	@echo "Formatting complete."

# ============================================
# Installation
# ============================================
.PHONY: install uninstall

install: all
	@if [ "$(shell id -u)" -ne 0 ] && [ "$(PREFIX)" = "/usr/local" ]; then \
		echo "Error: Installation to $(PREFIX) requires root privileges."; \
		echo "Please run: sudo make install"; \
		exit 1; \
	fi
	@echo "Installing tmlcs-tui to $(PREFIX)..."
	@mkdir -p $(INSTALL_DIR)/core
	@mkdir -p $(INSTALL_DIR)/widget
	@mkdir -p $(PREFIX)/lib/pkgconfig
	@install -m 644 include/core/types.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/theme.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/logger.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/manager.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/workspace.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/tab.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/window.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/clipboard.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/help.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/theme_loader.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/debug.h $(INSTALL_DIR)/core/
	@install -m 644 include/core/anim.h $(INSTALL_DIR)/core/
	@install -m 644 include/tmlcs_tui.h $(INSTALL_DIR)/
	@install -m 644 include/widget/text_input.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/textarea.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/button.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/label.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/checkbox.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/list.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/progress.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/context_menu.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/slider.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/radio_group.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/spinner.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/tab_container.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/dropdown.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/dialog.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/table.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/tree.h $(INSTALL_DIR)/widget/
	@install -m 644 include/widget/file_picker.h $(INSTALL_DIR)/widget/
	@install -m 644 tmlcs-tui.pc $(PREFIX)/lib/pkgconfig/
	@install -m 755 $(TARGET) $(PREFIX)/bin/
	@echo "Installing libraries to $(PREFIX)/lib/..."
	@mkdir -p $(PREFIX)/lib
	@install -m 644 $(LIB_STATIC) $(PREFIX)/lib/
	@if [ -f "$(LIB_SHARED_VERSIONED)" ]; then \
		install -m 755 $(LIB_SHARED_VERSIONED) $(PREFIX)/lib/; \
		ln -sf $(LIB_SHARED_VERSIONED) $(PREFIX)/lib/$(LIB_SHARED_SONAME); \
		ln -sf $(LIB_SHARED_VERSIONED) $(PREFIX)/lib/$(LIB_SHARED); \
	fi
	@echo "Installation complete."
	@echo "  Headers: $(INSTALL_DIR)/"
	@echo "  Binary:  $(PREFIX)/bin/$(TARGET)"
	@echo "  Static:  $(PREFIX)/lib/$(LIB_STATIC)"
	@echo "  Shared:  $(PREFIX)/lib/$(LIB_SHARED_VERSIONED)"
	@echo "  pkg-config: $(PREFIX)/lib/pkgconfig/tmlcs-tui.pc"

uninstall:
	@echo "Uninstalling tmlcs-tui from $(PREFIX)..."
	@rm -rf $(INSTALL_DIR)
	@rm -f $(PREFIX)/lib/pkgconfig/tmlcs-tui.pc
	@rm -f $(PREFIX)/bin/$(TARGET)
	@rm -f $(PREFIX)/lib/$(LIB_STATIC)
	@rm -f $(PREFIX)/lib/$(LIB_SHARED) $(PREFIX)/lib/$(LIB_SHARED_SONAME) $(PREFIX)/lib/$(LIB_SHARED_VERSIONED)
	@echo "Uninstallation complete."
