CC ?= gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror
TARGET := stack_depth_monitor

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET)

.PHONY: all clean
