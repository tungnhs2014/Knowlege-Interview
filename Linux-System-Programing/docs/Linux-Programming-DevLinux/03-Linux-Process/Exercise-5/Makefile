# Makefile for Exercise 5: Creating Zombie and Orphan Processes

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Werror -g

# Directory structure
CUR_DIR := .
SRC_DIR := $(CUR_DIR)/src
OBJ_DIR := $(CUR_DIR)/obj
BIN_DIR := $(CUR_DIR)/bin

# Phony targets (not associated with files)
.PHONY: all clean directories

# Source files
ZOMBIE_SRC := $(SRC_DIR)/zombie.c
ORPHAN_SRC := $(SRC_DIR)/orphan.c

# Object files
ZOMBIE_OBJ := $(OBJ_DIR)/zombie.o
ORPHAN_OBJ := $(OBJ_DIR)/orphan.o

# Output executables
ZOMBIE_EXEC := $(BIN_DIR)/zombie
ORPHAN_EXEC := $(BIN_DIR)/orphan

# Default target builds both executables
all: directories $(ZOMBIE_EXEC) $(ORPHAN_EXEC)

# Create required directories
directories:
	mkdir -p $(SRC_DIR) $(OBJ_DIR) $(BIN_DIR)

# Compile zombie.c to object file in obj directory
$(ZOMBIE_OBJ): $(ZOMBIE_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile orphan.c to object file in obj directory
$(ORPHAN_OBJ): $(ORPHAN_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Link the zombie executable from its object file
$(ZOMBIE_EXEC): $(ZOMBIE_OBJ)
	$(CC) $(CFLAGS) $< -o $@

# Link the orphan executable from its object file
$(ORPHAN_EXEC): $(ORPHAN_OBJ)
	$(CC) $(CFLAGS) $< -o $@

# Clean up
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)