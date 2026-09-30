# SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
OUTPUT := .output
CLANG ?= clang
NIXOS ?= $(shell which nix)
ifeq ($(strip $(NIXOS)),)
# 需要安装 yum install libbpf-static
# 不过，为什么需要才用静态库，这是个迷!
LIBBPF_OBJ := /usr/lib64/libbpf.a
else
LIBBPF_PREFIX := $(shell nix-build -E 'with import <nixpkgs> {}; lib.getLib libbpf' --no-out-link)
LIBELF_PREFIX := $(shell nix-build -E 'with import <nixpkgs> {}; lib.getLib elfutils' --no-out-link)
ZLIB_PREFIX := $(shell nix-build -E 'with import <nixpkgs> {}; lib.getLib zlib' --no-out-link)
LIBBPF_OBJ := $(LIBBPF_PREFIX)/lib/libbpf.a
LIBBPF_INCLUDE ?= $(LIBBPF_PREFIX)/include
NIX_LDFLAGS := -L$(LIBELF_PREFIX)/lib -L$(ZLIB_PREFIX)/lib
endif
BPFTOOL_OUTPUT ?= $(abspath $(OUTPUT)/bpftool)
BPFTOOL ?= bpftool
ARCH ?= $(shell uname -m | sed 's/x86_64/x86/' \
			 | sed 's/arm.*/arm/' \
			 | sed 's/aarch64/arm64/' \
			 | sed 's/ppc64le/powerpc/' \
			 | sed 's/mips.*/mips/' \
			 | sed 's/riscv64/riscv/' \
			 | sed 's/loongarch64/loongarch/')

VMLINUX = $(OUTPUT)/vmlinux.h

INCLUDES := -I$(OUTPUT) $(if $(LIBBPF_INCLUDE),-I$(LIBBPF_INCLUDE))
CFLAGS := -g -Wall
BPF_CFLAGS := -g -O2 -Wall -Werror -fms-extensions \
	      -Wno-microsoft-anon-tag
ALL_LDFLAGS := $(LDFLAGS) $(NIX_LDFLAGS) $(EXTRA_LDFLAGS)

APPS = tc minimal mapwriter cg ds task_iter bootstrap test_map_in_map \
	map_communication arena ext4_trace readahead
BARE_BPF_OBJ = core single iter_uds iter_slub bpf_cubic

# Get Clang's default includes on this system. We'll explicitly add these dirs
# to the includes list when compiling with `-target bpf` because otherwise some
# architecture-specific dirs will be "missing" on some architectures/distros -
# headers such as asm/types.h, asm/byteorder.h, asm/socket.h, asm/sockios.h,
# sys/cdefs.h etc. might be missing.
#
# Use '-idirafter': Don't interfere with include mechanics except where the
# build would have failed anyways.
CLANG_BPF_SYS_INCLUDES ?= $(shell $(CLANG) -v -E - </dev/null 2>&1 \
	| sed -n '/<...> search starts here:/,/End of search list./{ s| \(/.*\)|-idirafter \1|p }')

ifeq ($(V),1)
	Q =
	msg =
else
	Q = @
	msg = @printf '  %-8s %s%s\n'					\
		      "$(1)"						\
		      "$(patsubst $(abspath $(OUTPUT))/%,%,$(2))"	\
		      "$(if $(3), $(3))";
	MAKEFLAGS += --no-print-directory
endif

.PHONY: all
all: $(APPS) $(BARE_BPF_OBJ)

.PHONY: clean
clean:
	$(call msg,CLEAN)
	$(Q)rm -rf $(OUTPUT)
	$(Q)rm -f $(addsuffix .out,$(APPS))

$(OUTPUT) $(OUTPUT)/libbpf $(BPFTOOL_OUTPUT):
	$(call msg,MKDIR,$@)
	$(Q)mkdir -p $@

$(BARE_BPF_OBJ): %: $(OUTPUT)/%.bpf.o
	$(Q)echo "$<"

# Build BPF code
$(OUTPUT)/%.bpf.o: %.bpf.c $(LIBBPF_OBJ) $(wildcard %.h) $(VMLINUX) | $(OUTPUT)
	$(call msg,BPF,$@)
	$(Q)$(CLANG) $(BPF_CFLAGS) -target bpf -D__TARGET_ARCH_$(ARCH)       \
		     $(INCLUDES) $(CLANG_BPF_SYS_INCLUDES)                    \
		     -c $(filter %.c,$^) -o $(patsubst %.bpf.o,%.tmp.bpf.o,$@)
	$(Q)$(BPFTOOL) gen object $@ $(patsubst %.bpf.o,%.tmp.bpf.o,$@)

$(OUTPUT)/arena.bpf.o: bpf_arena_common.h

$(OUTPUT)/ext4_trace.bpf.o $(OUTPUT)/ext4_trace.o: ext4.h

$(VMLINUX):  | $(OUTPUT)
	mkdir -p $(OUTPUT)
	bpftool btf dump file /sys/kernel/btf/vmlinux format c > $(OUTPUT)/vmlinux.h

# Generate BPF skeletons
$(OUTPUT)/%.skel.h: $(OUTPUT)/%.bpf.o | $(OUTPUT)
	$(call msg,GEN-SKEL,$@)
	$(Q)$(BPFTOOL) gen skeleton $< > $@

# Build user-space code
$(patsubst %,$(OUTPUT)/%.o,$(APPS)): %.o: %.skel.h

$(OUTPUT)/%.o: %.c $(wildcard %.h) | $(OUTPUT)
	$(call msg,CC,$@)
	$(Q)$(CC) $(CFLAGS) $(INCLUDES) -c $(filter %.c,$^) -o $@

# Build application binary
$(APPS): %: $(OUTPUT)/%.o $(LIBBPF_OBJ) | $(OUTPUT)
	$(call msg,BINARY,$@)
	$(Q)$(CC) $(CFLAGS) $^ $(ALL_LDFLAGS) -lelf -lz -o $@.out

# delete failed targets
.DELETE_ON_ERROR:

# keep intermediate (.skel.h, .bpf.o, etc) targets
.SECONDARY:
