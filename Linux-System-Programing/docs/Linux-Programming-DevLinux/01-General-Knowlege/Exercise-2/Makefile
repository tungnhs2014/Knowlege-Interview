# Build shared library

# Compiler and tools
CC = gcc
CFLAGS = -fPIC -I$(INC_DIR)

# Directory structure
CUR_DIR := .
INC_DIR := $(CUR_DIR)/inc
SRC_DIR := $(CUR_DIR)/src
OBJ_DIR := $(CUR_DIR)/obj
BIN_DIR := $(CUR_DIR)/bin
SHARED_LIB_DIR := $(CUR_DIR)/lib/shared_lib

# Find all .c files in src/
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

# Convert .c filenames to .o for obj/
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Define output file names
SHARED_LIB = $(SHARED_LIB_DIR)/libcalc.so
EXEC = $(BIN_DIR)/exam

# Ensure necessary directories exist before compiling
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR) $(SHARED_LIB_DIR))

# Build target
all: $(SHARED_LIB) $(EXEC)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h
	$(CC) $(CFLAGS) -c $< -o $@

# Create the shared library (.so) from object files
$(SHARED_LIB): $(OBJ_FILES)
	$(CC) -shared $^ -o $@ 

# Link the executable file with the shared library
$(EXEC): $(OBJ_DIR)/main.o $(SHARED_LIB)
	$(CC) $^ -L$(SHARED_LIB_DIR) -lcalc -o $@

# Clean all compiled files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(CUR_DIR)/lib

.PHONY: all clean
