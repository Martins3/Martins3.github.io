# 参考 : https://stackoverflow.com/questions/22368264/compiling-out-of-tree-kernel-module-against-any-kernel-source-tree-on-the-filesy
# 但是是 M=${PWD} 而不是 SOURCES=${PWD}
#
# if KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.

ifneq (${KERNELRELEASE},)
# $(info call this in kernel environment)

# https://stackoverflow.com/questions/56429389/expand-macros-of-a-single-file-when-compiling-linux-kernel
# TODO 这个居然只能配合 gcc 使用，难道整个内核中都没有 save-temps 的需求吗?
CFLAGS_access-once.o := -save-temps=obj
# CFLAGS_barrier-dis.o := -save-temps=obj
# CFLAGS_misc.o    := -save-temps=obj
# CFLAGS_misc.o    := -DDEBUG

CFLAGS_tracepoint.o := -I$(src)

# 好像没什么影响
GCOV_PROFILE := y

ccflags-y += -I$(PWD)

OBJECTS := concurrent/access_once.o
martins3-m := main.o $(OBJECTS)
# $(info $(martins3-m))

obj-m += martins3.o
# obj-m += virtio/
# obj-m += tty0tty/
# obj-m += mini/
# 让这个只能在 Intel 上构建
# obj-m += peach/

# 依赖 kernel 的 config，看看如何优雅的解决
# obj-m += gpu/

# obj-m += rust_minimal.o

# Otherwise we were called directly from the command line.
# Invoke the kernel build system.
else


KERNEL_SOURCE ?= $(shell realpath ~/data/kernel/linux-drm)

# 注意，Makefile 对于空格敏感
ifeq ($(NIXOS),1)
# nixos 的 kernel-devel 比较特殊
PATH_1 := $(shell nix-build -E '(import <nixpkgs> {}).linuxPackages_latest.kernel.dev' --no-out-link)
PATH_2 := lib/modules/$(shell uname -r)/build
KERNEL_SOURCE := $(PATH_1)/$(PATH_2)
# 在 /run/current-system/kernel-modules/lib/modules/6.11.0/kernel/ 下不是 kernel-devel ，而是对应 kernel-modules 这个包
endif

ifeq ($(NORMAL),1)
KERNEL_SOURCE := /lib/modules/$(shell uname -r)/build
# KERNEL_SOURCE := /lib/modules/martins3-5.10.x86_64/build
endif

# 例如 VERSION=6.6.0-114.0.0.el7.v48.aarch64 make
ifneq ($(strip $(VERSION)),)
KERNEL_SOURCE := /lib/modules/$(VERSION)/build
endif

PWD := $(shell pwd)

# 这是一个相当强的技巧，我不知道为什么恰好可以运行。
# 当 config.h 或 config.sh 修改时，自动重新生成 OBJECTS ，这是会修改当前的 Makefile 的
# 然后被修改的 OBJECTS 立刻生效，然后构建的 makefile 就是对的
.config.mk: config.h config.sh
	@echo "检测到配置变化，正在更新 OBJECTS..."
	@./config.sh
	@touch $@

default: .config.mk
	@echo "构建内核模块"
	@${MAKE} -C ${KERNEL_SOURCE} M=${PWD} modules
	@./gen_compile_commands.py

clean:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} clean

rs:
	${MAKE} -C ${KERNEL_SOURCE} M=${PWD} rust-analyzer
endif
