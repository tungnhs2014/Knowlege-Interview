obj-m += dma_mapping_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(CURDIR)

.PHONY: all modules clean

all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
