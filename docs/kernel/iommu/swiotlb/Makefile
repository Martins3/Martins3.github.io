KDIR ?= /home/martins3/data/kernel/linux-drm

obj-m += swiotlb_edu.o

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
