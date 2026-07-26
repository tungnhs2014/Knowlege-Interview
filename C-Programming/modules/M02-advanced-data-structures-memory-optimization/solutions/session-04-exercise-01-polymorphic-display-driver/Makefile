CC ?= cc
CPPFLAGS ?=
CFLAGS ?=

TARGET := polymorphic_display_driver
SOURCES := main.c console_display.c dummy_display.c
BASE_CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) i_display.h console_display.h dummy_display.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BASE_CFLAGS) $(SOURCES) -o $@

clean:
	$(RM) $(TARGET)
