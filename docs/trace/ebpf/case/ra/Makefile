# Makefile
BPFTOOL ?= bpftool
CLANG ?= clang
ARCH ?= $(shell uname -m | sed 's/x86_64/x86/' \
			 | sed 's/arm.*/arm/' \
			 | sed 's/aarch64/arm64/' \
			 | sed 's/ppc64le/powerpc/' \
			 | sed 's/mips.*/mips/' \
			 | sed 's/riscv64/riscv/' \
			 | sed 's/loongarch64/loongarch/')

CFLAGS := -g -O2 -Wall
LDFLAGS := -lbpf -lelf
# LDFLAGS := -static /usr/lib64/libelf.a /usr/lib64/libzstd.a  /usr/lib64/libbpf.a /usr/lib64/libelf.a /usr/lib64/libzstd.a /usr/lib64/libz.a

.PHONY: all clean

all: readahead

vmlinux.h:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

readahead.skel.h: readahead.bpf.c vmlinux.h
	$(CLANG) -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) -I. -c readahead.bpf.c -o readahead.bpf.o
	$(BPFTOOL) gen skeleton readahead.bpf.o > readahead.skel.h

readahead: readahead.c readahead.skel.h
	$(CC) $(CFLAGS) -I. readahead.c $(LDFLAGS) -o readahead

clean:
	rm -f *.o *.skel.h vmlinux.h readahead

run: all
	sudo ./readahead
