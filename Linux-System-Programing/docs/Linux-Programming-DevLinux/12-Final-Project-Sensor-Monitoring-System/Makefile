.PHONY: all clean

CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -O2

TARGET := sensor_gateway
SRC := src/main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
