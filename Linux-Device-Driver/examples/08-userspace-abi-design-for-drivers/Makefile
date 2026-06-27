obj-m += abidemo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2

.PHONY: all modules user clean

all: modules user

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

user: abidemo_test

abidemo_test: abidemo_test.c abidemo_uapi.h
	$(CC) $(CFLAGS) -o $@ abidemo_test.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	$(RM) abidemo_test
