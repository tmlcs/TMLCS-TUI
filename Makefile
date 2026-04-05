CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude -g
LDFLAGS = -lnotcurses -lnotcurses-core -pthread

# Test framework
TEST_SRC = tests/test_logger.c
TEST_OBJ = $(BUILD_DIR)/test_logger.o
TEST_TARGET = $(BUILD_DIR)/test_runner

SRC_DIR = src
EXAMPLES_DIR = examples
INC_DIR = include
BUILD_DIR = build

# Encuentra todos los .c recursivamente usando bash find
SRCS := $(shell find $(SRC_DIR) -name '*.c')
EXAMPLE_SRCS := $(shell find $(EXAMPLES_DIR) -name '*.c')
ALL_SRCS := $(SRCS) $(EXAMPLE_SRCS)

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
EXAMPLE_OBJS := $(patsubst $(EXAMPLES_DIR)/%.c, $(BUILD_DIR)/examples/%.o, $(EXAMPLE_SRCS))
ALL_OBJS := $(OBJS) $(EXAMPLE_OBJS)

TARGET = mi_tui

.PHONY: all clean run test asan ubsan coverage tidy valgrind

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/examples/%.o: $(EXAMPLES_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

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
TAB_WS_CORE_SRCS = src/core/logger.c src/core/tab.c src/core/workspace.c src/core/window.c src/core/manager/state.c src/core/manager/utils.c

$(BUILD_DIR)/test_tab_runner: tests/test_tab.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_tab.c $(TAB_WS_CORE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

$(BUILD_DIR)/test_workspace_runner: tests/test_workspace.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ tests/test_workspace.c $(TAB_WS_CORE_SRCS) tests/nc_stubs.c -Iinclude -g -lpthread

# Logger tests don't need stubs (pure C, no notcurses)
$(BUILD_DIR)/test_logger_runner: tests/test_logger.c $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/test_logger_extended_runner: tests/test_logger_extended.c $(filter-out $(BUILD_DIR)/main.o $(BUILD_DIR)/examples/demo.o,$(OBJS))
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

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
asan: clean all test

ubsan: CFLAGS += -fsanitize=undefined -fno-omit-frame-pointer -g -O1
ubsan: LDFLAGS += -fsanitize=undefined
ubsan: clean all test

# ============================================
# Code coverage
# ============================================
COV_CFLAGS = --coverage -fprofile-arcs -ftest-coverage
COV_LDFLAGS = --coverage

.PHONY: coverage

coverage: CFLAGS += $(COV_CFLAGS)
coverage: LDFLAGS += $(COV_LDFLAGS)
coverage: clean all test
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

valgrind: all test
	@echo "Running valgrind on test_runner..."
	@mkdir -p build
	valgrind $(VALGRIND_OPTS) ./build/test_runner 2>build/valgrind_report.txt || \
		{ echo "Valgrind detected memory errors!"; cat build/valgrind_report.txt; exit 1; }
	@echo "Valgrind report: build/valgrind_report.txt"
