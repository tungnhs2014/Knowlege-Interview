CC ?= cc
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

TARGET := ui_menu_dispatcher
STRICT_CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror

CFLAGS += $(STRICT_CFLAGS)

.PHONY: all clean inspect

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) main.c -o $@ $(LDFLAGS) $(LDLIBS)

inspect: $(TARGET)
	@if command -v readelf >/dev/null 2>&1; then readelf -S $(TARGET); readelf -s $(TARGET); else printf '%s\n' 'readelf: TOOL UNAVAILABLE'; fi
	@if command -v objdump >/dev/null 2>&1; then objdump -h $(TARGET); objdump -t $(TARGET); else printf '%s\n' 'objdump: TOOL UNAVAILABLE'; fi
	@if command -v nm >/dev/null 2>&1; then nm -S $(TARGET); else printf '%s\n' 'nm: TOOL UNAVAILABLE'; fi

clean:
	$(RM) $(TARGET)
