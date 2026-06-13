# GCC Inline Assembly Tutorial - 主 Makefile
# ===========================================
# 支持 x86_64 和 aarch64 架构

# 检测当前架构
ARCH := $(shell uname -m)

# 架构映射
ifeq ($(ARCH),x86_64)
    ARCH_DIR = x86_64
endif
ifeq ($(ARCH),aarch64)
    ARCH_DIR = aarch64
endif
ifeq ($(ARCH),arm64)
    ARCH_DIR = aarch64
endif

# 如果架构不支持，报错
ifndef ARCH_DIR
    $(error Unsupported architecture: $(ARCH). Only x86_64 and aarch64 are supported)
endif

# 编译器设置
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -pthread
CFLAGS_DEBUG = -Wall -Wextra -g -O0 -std=c11 -pthread -DDEBUG

# 架构特定标志
ifeq ($(ARCH_DIR),x86_64)
    CFLAGS += -march=native
endif
ifeq ($(ARCH_DIR),aarch64)
    CFLAGS += -march=armv8-a
endif

# 源文件目录
SRC_DIR = $(ARCH_DIR)

# 所有源文件
SRCS = $(wildcard $(SRC_DIR)/*.c)

# 可执行文件 (.out 后缀便于 git ignore)
BINS = $(SRCS:.c=.out)

# 默认目标
.PHONY: all clean test debug asm help info

all: $(BINS)

# 编译规则
$(SRC_DIR)/%.out: $(SRC_DIR)/%.c
	@echo "CC  [$(ARCH)] $<"
	@$(CC) $(CFLAGS) -o $@ $<

# 测试目标
test: all
	@echo ""
	@echo "========== Running Tests on $(ARCH) =========="
	@echo ""
	@for bin in $(BINS); do \
		echo "Running $$bin..."; \
		./$$bin || exit 1; \
		echo ""; \
	done
	@echo "=========================================="
	@echo "All tests passed successfully on $(ARCH)!"
	@echo "=========================================="

# 调试版本
debug: CFLAGS = $(CFLAGS_DEBUG)
debug: all

# 生成汇编代码
asm: $(SRCS)
	@for src in $(SRCS); do \
		echo "Generating assembly for $$src..."; \
		$(CC) $(CFLAGS) -S -o $${src%.c}.s $$src; \
	done

# 清理
clean:
	@echo "Cleaning up $(ARCH_DIR)..."
	@rm -f $(SRC_DIR)/*.out
	@rm -f $(SRC_DIR)/*.s
	@echo "Done."

# 显示架构信息
info:
	@echo "Current architecture: $(ARCH)"
	@echo "Architecture directory: $(ARCH_DIR)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Source files:"
	@for src in $(SRCS); do echo "  $$src"; done

# 帮助信息
help:
	@echo "GCC Inline Assembly Tutorial - Makefile"
	@echo "========================================"
	@echo ""
	@echo "Current architecture: $(ARCH)"
	@echo "Architecture directory: $(ARCH_DIR)"
	@echo ""
	@echo "Available targets:"
	@echo "  make all       - Build all examples for current architecture"
	@echo "  make test      - Build and run all tests"
	@echo "  make debug     - Build with debug symbols"
	@echo "  make asm       - Generate assembly listings"
	@echo "  make clean     - Remove all generated files"
	@echo "  make info      - Show architecture and build information"
	@echo "  make help      - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make all && ./$(ARCH_DIR)/01_hello_asm.out"
	@echo "  make test"
	@echo "  make debug && gdb ./$(ARCH_DIR)/01_hello_asm.out"
