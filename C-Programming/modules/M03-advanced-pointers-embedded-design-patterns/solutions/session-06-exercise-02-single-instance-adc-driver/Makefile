CC ?= cc
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

TARGET := single_instance_adc_driver
SOURCES := main.c adc.c
STRICT_CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror

CFLAGS += $(STRICT_CFLAGS)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) adc.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	$(RM) $(TARGET)
