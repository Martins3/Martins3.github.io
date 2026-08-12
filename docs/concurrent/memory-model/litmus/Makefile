# Makefile for litmus tests
#
# 用法:
#   make all      用 herd7 检查 C/LKMM 测试，并编译全部测试
#   make check    用 herd7 + LKMM/x86 TSO 检查全部测试
#   make graph    用 herd7 为全部测试生成执行图
#   make compile  用 litmus7 生成并编译全部测试
#   make run      展示 LKMM 结果并运行全部真机测试
#   make clean    清理生成文件
#
# 常用变量覆盖示例:
#   make run LITMUS7_EXTRA="-s 100000 -r 10"
#   make check KERNEL_TREE=/path/to/linux

SHELL       := /usr/bin/bash
.SHELLFLAGS := -E -e -u -o pipefail -c

HERD7          ?= herd7
LITMUS7        ?= litmus7
HERD_LIBDIR    ?= /home/martins3/.local/share/herdtools7/herd
LITMUS_LIBDIR  ?= /home/martins3/.local/share/herdtools7/litmus
KERNEL_TREE    ?= /home/martins3/data/kernel/linux-drm
LKMM_PATH      ?= $(KERNEL_TREE)/tools/memory-model
KERNEL_TOOLS_INCLUDE ?= $(KERNEL_TREE)/tools/include
KERNEL_TOOLS_ARCH_INCLUDE ?= $(KERNEL_TREE)/tools/arch/x86/include
MACH           ?= x86_64
BUILD          := build
HERD_BUILD     := $(BUILD)/herd
LKMM_HERD_BUILD := $(HERD_BUILD)/lkmm
X86_HERD_BUILD := $(HERD_BUILD)/x86
HERD_GRAPH_BUILD := $(HERD_BUILD)/graphs
LKMM_HERD_GRAPH_BUILD := $(HERD_GRAPH_BUILD)/lkmm
X86_HERD_GRAPH_BUILD := $(HERD_GRAPH_BUILD)/x86
LITMUS_BUILD   := $(BUILD)/litmus
C_COMPAT_H     := linux-tools-compat.h
C_TOOL_HEADERS := $(KERNEL_TOOLS_INCLUDE)/linux/compiler.h \
	$(KERNEL_TOOLS_ARCH_INCLUDE)/asm/barrier.h

LKMM_HERD7_OPTS := -set-libdir "$(HERD_LIBDIR)" -I "$(LKMM_PATH)" \
	-conf "$(LKMM_PATH)/linux-kernel.cfg" $(HERD7_EXTRA) $(LKMM_HERD7_EXTRA)
X86_HERD7_OPTS := -set-libdir "$(HERD_LIBDIR)" -model x86tso.cat \
	$(HERD7_EXTRA) $(X86_HERD7_EXTRA)
HERD_GRAPH_SHOW ?= all
HERD_GRAPH_NSHOW ?= 1
HERD_GRAPH_OPTS := -show "$(HERD_GRAPH_SHOW)" -nshow "$(HERD_GRAPH_NSHOW)" \
	$(HERD_GRAPH_EXTRA)
LITMUS7_OPTS := -set-libdir "$(LITMUS_LIBDIR)" -mach "$(MACH)" $(LITMUS7_EXTRA)
C_CCOPTS     := -O2 -I"$(KERNEL_TOOLS_ARCH_INCLUDE)" -I"$(KERNEL_TOOLS_INCLUDE)" \
	-include "$(CURDIR)/$(C_COMPAT_H)"
C_LITMUS7_OPTS := $(LITMUS7_OPTS) -ccopts '$(C_CCOPTS)'

C_TESTS   := $(shell grep -l '^C ' *.litmus 2>/dev/null | sed 's/\.litmus$$//')
X86_TESTS := $(shell grep -l '^X86' *.litmus 2>/dev/null | sed 's/\.litmus$$//')
LITMUS_TESTS := $(C_TESTS) $(X86_TESTS)

LKMM_HERD_RESULTS := $(C_TESTS:%=$(LKMM_HERD_BUILD)/%.out)
X86_HERD_RESULTS := $(X86_TESTS:%=$(X86_HERD_BUILD)/%.out)
HERD_RESULTS := $(LKMM_HERD_RESULTS) $(X86_HERD_RESULTS)
LKMM_HERD_GRAPH_STAMPS := $(C_TESTS:%=$(LKMM_HERD_GRAPH_BUILD)/%/stamp)
X86_HERD_GRAPH_STAMPS := $(X86_TESTS:%=$(X86_HERD_GRAPH_BUILD)/%/stamp)
HERD_GRAPH_STAMPS := $(LKMM_HERD_GRAPH_STAMPS) $(X86_HERD_GRAPH_STAMPS)
C_STAMPS     := $(C_TESTS:%=$(LITMUS_BUILD)/%/stamp)
X86_STAMPS   := $(X86_TESTS:%=$(LITMUS_BUILD)/%/stamp)
LITMUS_STAMPS := $(C_STAMPS) $(X86_STAMPS)
LKMM_HERD_REPORTS := $(C_TESTS:%=report-lkmm-%)
X86_HERD_REPORTS := $(X86_TESTS:%=report-x86-%)
HERD_REPORTS := $(LKMM_HERD_REPORTS) $(X86_HERD_REPORTS)
HARDWARE_RUNS := $(LITMUS_TESTS:%=hardware-%)

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:
.PHONY: all check graph compile run clean FORCE $(HERD_REPORTS) $(HARDWARE_RUNS)

all: check compile

check: $(HERD_RESULTS)

graph: $(HERD_GRAPH_STAMPS)

compile: $(LITMUS_STAMPS)

FORCE:

$(BUILD)/.gitignore:
	@mkdir -p "$(BUILD)"
	@printf '%s\n' '*' > "$@"

$(LKMM_HERD_RESULTS): $(LKMM_HERD_BUILD)/%.out: %.litmus FORCE | $(BUILD)/.gitignore
	@mkdir -p "$(LKMM_HERD_BUILD)"
	$(HERD7) $(LKMM_HERD7_OPTS) "$<" > "$@"

$(X86_HERD_RESULTS): $(X86_HERD_BUILD)/%.out: %.litmus FORCE | $(BUILD)/.gitignore
	@mkdir -p "$(X86_HERD_BUILD)"
	$(HERD7) $(X86_HERD7_OPTS) "$<" > "$@"

$(LKMM_HERD_GRAPH_STAMPS): $(LKMM_HERD_GRAPH_BUILD)/%/stamp: %.litmus FORCE | $(BUILD)/.gitignore
	@rm -rf "$(@D)"
	@mkdir -p "$(@D)"
	$(HERD7) $(LKMM_HERD7_OPTS) $(HERD_GRAPH_OPTS) -o "$(@D)" "$<" > "$(@D)/summary.out"
	@touch "$@"

$(X86_HERD_GRAPH_STAMPS): $(X86_HERD_GRAPH_BUILD)/%/stamp: %.litmus FORCE | $(BUILD)/.gitignore
	@rm -rf "$(@D)"
	@mkdir -p "$(@D)"
	$(HERD7) $(X86_HERD7_OPTS) $(HERD_GRAPH_OPTS) -o "$(@D)" "$<" > "$(@D)/summary.out"
	@touch "$@"

$(C_STAMPS): LITMUS7_TEST_OPTS := $(C_LITMUS7_OPTS)
$(X86_STAMPS): LITMUS7_TEST_OPTS := $(LITMUS7_OPTS)
$(C_STAMPS): $(C_COMPAT_H) $(C_TOOL_HEADERS)

$(LITMUS_BUILD)/%/stamp: %.litmus FORCE | $(BUILD)/.gitignore
	@mkdir -p "$(LITMUS_BUILD)"
	@rm -rf "$(LITMUS_BUILD)/$*"
	@mkdir -p "$(LITMUS_BUILD)/$*"
	$(LITMUS7) $(LITMUS7_TEST_OPTS) -o "$(LITMUS_BUILD)/$*" "$<"
	$(MAKE) -C "$(LITMUS_BUILD)/$*"
	@touch "$@"

run: all $(HERD_REPORTS) $(HARDWARE_RUNS)

$(LKMM_HERD_REPORTS): report-lkmm-%: $(LKMM_HERD_BUILD)/%.out
	@echo "==== model LKMM: $* ===="
	@cat "$<"

$(X86_HERD_REPORTS): report-x86-%: $(X86_HERD_BUILD)/%.out
	@echo "==== model x86 TSO: $* ===="
	@cat "$<"

$(HARDWARE_RUNS): hardware-%: $(LITMUS_BUILD)/%/stamp
	@echo "==== hardware: $* ===="
	@cd "$(CURDIR)/$(LITMUS_BUILD)/$*" && sh run.sh \
		| grep -v '^Revision\|^Command line\|^Parameters\|^#define\|^/\*\|^GCC\|^LITMUSOPTS\|^[A-Z][a-z][a-z] [A-Z][a-z][a-z]'

clean:
	@rm -rf "$(BUILD)"
	@echo "cleaned"
