BPFTOOL ?= bpftool
CLANG ?= clang
LLC ?= llc

ARCH ?= $(shell uname -m | sed 's/x86_64/x86/' \
			 | sed 's/arm.*/arm/' \
			 | sed 's/aarch64/arm64/' \
			 | sed 's/ppc64le/powerpc/' \
			 | sed 's/mips.*/mips/' \
			 | sed 's/riscv64/riscv/' \
			 | sed 's/loongarch64/loongarch/')
VMLINUX_H := vmlinux.h

CFLAGS := -g -O2 -Wall
LDFLAGS := -lbpf -lelf

.PHONY: all clean

all: ext4_trace

vmlinux.h:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

ext4_trace.skel.h: ext4_trace.bpf.c vmlinux.h
	$(CLANG) -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) -I. -c ext4_trace.bpf.c -o ext4_trace.bpf.o
	$(BPFTOOL) gen skeleton ext4_trace.bpf.o > ext4_trace.skel.h

ext4_trace: ext4_trace.c ext4_trace.skel.h
	$(CC) $(CFLAGS) -I. ext4_trace.c /usr/lib64/libelf.a /usr/lib64/libzstd.a  /usr/lib64/libbpf.a /usr/lib64/libelf.a /usr/lib64/libzstd.a /usr/lib64/libz.a -static  -o ext4_trace  -lz

clean:
	rm -f *.o *.skel.h vmlinux.h ext4_trace

run: all
	sudo ./ext4_trace
