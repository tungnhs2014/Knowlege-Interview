CC ?= cc
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

TARGET := timer_callback_registration
SOURCES := main.c timer.c
STRICT_CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror

CFLAGS += $(STRICT_CFLAGS)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) timer.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) $(TARGET)
