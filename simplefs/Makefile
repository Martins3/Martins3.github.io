KDIR ?= /home/martins3/data/kernel/linux-drm

obj-m += simplefs.o
simplefs-m := simplefs_fs.o \
	simplefs_super.o \
	simplefs_inode.o \
	simplefs_file.o \
	simplefs_dir.o \
	simplefs_extent.o \
	simplefs_trace.o \
	simplefs_metabuf.o \
	simplefs_debug.o \
	simplefs_journal.o \
	simplefs_xattr.o \
	simplefs_acl.o

CFLAGS_simplefs_trace.o := -I$(src)

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
