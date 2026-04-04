CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
LDFLAGS = -lnotcurses -lnotcurses-core

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Encuentra todos los .c recursivamente usando bash find
SRCS := $(shell find $(SRC_DIR) -name '*.c')
# Genera ruta .o manteniendo la matriz sub-directorial
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

TARGET = mi_tui

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

run: all
	./$(TARGET)
