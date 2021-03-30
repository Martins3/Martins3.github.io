QEMU_DISPLAY ?= none
ARCH ?= x86-64
ifeq ($(ARCH),x86-64)
b = b
endif

KDIR = /home/maritns3/core/linux
ZIMAGE = $(KDIR)/arch/$(ARCH)/boot/$(b)zImage

ifeq ($(ARCH),x86-64)
ZIMAGE = $(KDIR)/arch/x86_64/boot/$(b)zImage
endif

PROJECT_DIR = /home/maritns3/core/vn/hack/kvm/qemu/mini-img

YOCTO_URL = http://downloads.yoctoproject.org/releases/yocto/yocto-3.1/machines/qemu/qemu$(ARCH)/
YOCTO_IMAGE_NAME=core-image-minimal-qemu$(ARCH).ext4
YOCTO_IMAGE = $(PROJECT_DIR)/$(YOCTO_IMAGE_NAME)
#YOCTO_IMAGE = core-image-minimal-dev-qemu$(ARCH).ext4
#YOCTO_IMAGE = core-image-sato-dev-qemu$(ARCH).ext4
#YOCTO_IMAGE = core-image-sato-qemu$(ARCH).ext4
#YOCTO_IMAGE = core-image-sato-sdk-qemu$(ARCH).ext4

QEMU_OPTS = -kernel $(ZIMAGE) \
	-device virtio-serial \
	-chardev pty,id=virtiocon0 -device virtconsole,chardev=virtiocon0 \
	-serial pipe:pipe1 -serial pipe:pipe2 \
	-netdev tap,id=tap0,ifname=tap0,script=no,downscript=no -net nic,netdev=tap0,model=virtio \
	-netdev tap,id=tap1,ifname=tap1,script=no,downscript=no -net nic,netdev=tap1,model=i82559er \
	-drive file=$(YOCTO_IMAGE),if=virtio,format=raw \
	-drive file=disk1.img,if=virtio,format=raw \
	-drive file=disk2.img,if=virtio,format=raw \
	--append "root=/dev/vda loglevel=15 console=hvc0" \
	--display $(QEMU_DISPLAY) -s -m 256

boot: .modinst tap0 tap1 pipe1.in pipe1.out pipe2.in pipe2.out disk1.img disk2.img
	ARCH=$(ARCH) $(PROJECT_DIR)/qemu.sh $(QEMU_OPTS)

TEMPDIR := $(shell mktemp -u)

.modinst: $(ZIMAGE) $(YOCTO_IMAGE)
	mkdir $(TEMPDIR)
	sudo mount -t ext4 -o loop $(YOCTO_IMAGE) $(TEMPDIR)
	sudo $(MAKE) -C $(KDIR) modules_install INSTALL_MOD_PATH=$(TEMPDIR)
	sudo umount $(TEMPDIR)
	rmdir $(TEMPDIR)
	sleep 1 && touch .modinst

gdb: $(ZIMAGE)
	gdb -ex "target remote localhost:1234" $(KDIR)/vmlinux

$(YOCTO_IMAGE):
	wget $(YOCTO_URL)/$(YOCTO_IMAGE_NAME)
	sudo $(PROJECT_DIR)/prepare-img.sh $(YOCTO_IMAGE)

tap0:
	$(PROJECT_DIR)/create_net.sh $@

tap1:
	$(PROJECT_DIR)/create_net.sh $@

pipe1.in:
	mkfifo $@

pipe1.out:
	mkfifo $@

pipe2.in: pipe1.out
	ln $< $@

pipe2.out: pipe1.in
	ln $< $@

disk1.img:
	qemu-img create -f raw $@ 100M

disk2.img:
	qemu-img create -f raw $@ 100M

clean::
	-rm -f .modinst
	-rm -f disk1.img disk2.img
	-rm -f pipe1.in pipe1.out pipe2.in pipe2.out

.PHONY: boot gdb clean tap0 tap1
