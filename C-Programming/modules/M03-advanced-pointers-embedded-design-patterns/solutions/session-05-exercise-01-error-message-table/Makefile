CC ?= cc
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

TARGET := error_message_table
STRICT_CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror

CFLAGS += $(STRICT_CFLAGS)

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) $(TARGET)
