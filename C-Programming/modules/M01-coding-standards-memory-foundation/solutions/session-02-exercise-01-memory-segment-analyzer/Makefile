CC ?= gcc
CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror
TARGET := memory_segment_analyzer

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET)

.PHONY: all clean
