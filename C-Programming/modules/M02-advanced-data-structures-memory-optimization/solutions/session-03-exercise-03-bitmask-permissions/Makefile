CC ?= cc
CPPFLAGS ?=
CFLAGS ?=

TARGET := bitmask_permissions
SHORT_ENUMS_TARGET := bitmask_permissions_short_enums
BASE_CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror
SHORT_ENUMS_CFLAGS := $(BASE_CFLAGS) -fshort-enums

.PHONY: all clean short-enums

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BASE_CFLAGS) $< -o $@

short-enums: main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SHORT_ENUMS_CFLAGS) $< -o $(SHORT_ENUMS_TARGET)

clean:
	$(RM) $(TARGET) $(SHORT_ENUMS_TARGET)
