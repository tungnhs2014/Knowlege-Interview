CC ?= cc
CPPFLAGS ?=
CFLAGS ?=

BASE_CFLAGS := -std=c99 -Wall -Wextra -Wpedantic -Werror
TARGET := memory_segment_analyzer

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BASE_CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET)

.PHONY: all clean
