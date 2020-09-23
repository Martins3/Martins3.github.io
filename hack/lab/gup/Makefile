obj-m += sample.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	clang user.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
