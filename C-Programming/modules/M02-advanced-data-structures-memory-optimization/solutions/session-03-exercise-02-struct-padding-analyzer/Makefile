CC ?= cc
CPPFLAGS ?=
CFLAGS ?=

TARGET := struct_padding_analyzer
DIAGNOSTIC_TARGET := struct_padding_diagnostic
BASE_CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror
DIAGNOSTIC_CFLAGS := -std=c99 -Wall -Wextra -pedantic \
	-Waddress-of-packed-member -Wno-error=address-of-packed-member

.PHONY: all clean diagnostic

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BASE_CFLAGS) $< -o $@

diagnostic: main.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DIAGNOSTIC_CFLAGS) \
		-DPACKED_MEMBER_DIAGNOSTIC $< -o $(DIAGNOSTIC_TARGET)

clean:
	$(RM) $(TARGET) $(DIAGNOSTIC_TARGET)
