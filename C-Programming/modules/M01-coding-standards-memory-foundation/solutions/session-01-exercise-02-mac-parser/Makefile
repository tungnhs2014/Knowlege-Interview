CC ?= cc
CPPFLAGS ?=
CFLAGS ?=

BASE_CFLAGS := -std=c99 -Wall -Wextra -Wpedantic -Werror
TARGET := mac_parser

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BASE_CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET)

.PHONY: all clean
