ifneq ($(KERNELRELEASE),)
obj-m += peach.o
peach-objs := peach_intel.o vmexit_handler.o
else
KERNEL_SOURCE ?= $(HOME)/data/kernel/default
.PHONY: all clean
all:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) modules
	$(CC) -O2 -Wall -Wextra -Werror -o peach-user.out peach-user.c
clean:
	$(MAKE) -C $(KERNEL_SOURCE) M=$(CURDIR) clean
endif
