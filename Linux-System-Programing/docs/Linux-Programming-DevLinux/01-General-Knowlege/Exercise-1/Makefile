# Build static library

# Compiler and tools
CC = gcc
AR = ar
CFLAGS = -I$(INC_DIR)

# Directory structure
CUR_DIR := .
INC_DIR := $(CUR_DIR)/inc
SRC_DIR := $(CUR_DIR)/src
OBJ_DIR := $(CUR_DIR)/obj
BIN_DIR := $(CUR_DIR)/bin
STATIC_LIB_DIR := $(CUR_DIR)/lib/static_lib

# Find all .c files in src/
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

# Convert .c filenames to .o for obj/
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Define output file names
STATIC_LIB = $(STATIC_LIB_DIR)/libcalc.a
EXEC = $(BIN_DIR)/exam

# Ensure necessary directories exist before compiling
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR) $(STATIC_LIB_DIR))

# Build target
all: $(STATIC_LIB) $(EXEC)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h
	$(CC) $(CFLAGS) -c $< -o $@

# Create the static library (.a) from object files
$(STATIC_LIB): $(OBJ_FILES)
	$(AR) rcs $@ $^

# Link the executable file with the static library
$(EXEC): $(OBJ_DIR)/main.o $(STATIC_LIB)
	$(CC) $^ -L$(STATIC_LIB_DIR) -lcalc -o $@

# Clean all compiled files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(CUR_DIR)/lib

.PHONY: all clean
