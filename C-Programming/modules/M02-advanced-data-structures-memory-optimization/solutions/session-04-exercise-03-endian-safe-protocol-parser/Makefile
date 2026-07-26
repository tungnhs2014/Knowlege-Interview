CC ?= cc
CPPFLAGS ?=
CFLAGS ?=

TARGET := endian_safe_protocol_parser
BASE_CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BASE_CFLAGS) $< -o $@

clean:
	$(RM) $(TARGET)
