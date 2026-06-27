# Build Chat Application

# Compiler and tools
CC = gcc
CFLAGS = -lpthread -Wall -g -I$(INC_DIR)

# Directory structure
CUR_DIR := .
INC_DIR := $(CUR_DIR)/inc
SRC_DIR := $(CUR_DIR)/src
OBJ_DIR := $(CUR_DIR)/obj
BIN_DIR := $(CUR_DIR)/bin

# Source and object files
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

# Ensure necessary directories exist before compiling
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Define output file names
TARGET = $(BIN_DIR)/chat_app

# Build target
all: $(TARGET)

# Rule to compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link the final executable
$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $@

# Run Valgrind to check for memory leak
valgrind-check: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET) 8000

# Clean all compiled files
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean valgrind-check