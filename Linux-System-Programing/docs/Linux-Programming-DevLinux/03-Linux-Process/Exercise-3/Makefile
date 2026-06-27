# Makefile for Exercise 3: Process Signal

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g

# Directory structure
CUR_DIR := .
SRC_DIR := $(CUR_DIR)/src
OBJ_DIR := $(CUR_DIR)/obj
BIN_DIR := $(CUR_DIR)/bin

# Target executable name
TARGET = exam
EXEC = $(BIN_DIR)/$(TARGET)

# Find all source files and generate corresponding object file paths
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Phony targets (not associated with files)
.PHONY: all clean directories

# Default target
all: directories $(EXEC)

# Create required directories
directories:
	mkdir -p $(SRC_DIR) $(OBJ_DIR) $(BIN_DIR)

# Rule to build object files from source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link object files into final executable
$(EXEC): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)