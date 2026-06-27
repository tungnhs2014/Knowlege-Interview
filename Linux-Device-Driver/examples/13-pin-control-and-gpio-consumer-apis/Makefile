obj-m += pinctrl_gpio_consumer_demo.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(CURDIR)

.PHONY: all modules clean

all: modules

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f pinctrl-gpio-consumer-demo.dtb pinctrl-gpio-consumer-demo.decompiled.dts
