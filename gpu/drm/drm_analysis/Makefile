# DRM 子系统分析 - 一键编译和测试

CC = gcc
CXX = g++
CFLAGS = -Wall -O2
CXXFLAGS = -Wall -O2 -std=c++11

# 检测 libdrm 路径
LIBDRM_CFLAGS := $(shell pkg-config --cflags libdrm 2>/dev/null || echo "-I/usr/include/libdrm")
LIBDRM_LIBS := $(shell pkg-config --libs libdrm 2>/dev/null || echo "-ldrm")

# 所有目标
GEM_TARGETS = 01_gem/gem_alloc.out 01_gem/gem_mmap.out
KMS_TARGETS = 02_kms/kms_info.out 02_kms/kms_setmode.out
TTM_TARGETS = 03_ttm/ttm_evict.out

ALL_TARGETS = $(GEM_TARGETS) $(KMS_TARGETS) $(TTM_TARGETS)

.PHONY: all clean help test gem_test kms_test ttm_test

# 默认目标
all: $(ALL_TARGETS)

help:
	@echo "DRM 子系统分析 - GEM / KMS / TTM"
	@echo ""
	@echo "编译目标:"
	@echo "  make all        - 编译所有实验"
	@echo "  make gem        - 编译 GEM 实验"
	@echo "  make kms        - 编译 KMS 实验"
	@echo "  make ttm        - 编译 TTM 实验"
	@echo ""
	@echo "运行实验:"
	@echo "  make test       - 运行所有实验"
	@echo "  make gem_test   - 运行 GEM 实验"
	@echo "  make kms_test   - 运行 KMS 实验"
	@echo "  make ttm_test   - 运行 TTM 实验"
	@echo ""
	@echo "清理:"
	@echo "  make clean      - 删除所有编译产物"
	@echo ""
	@echo "文档:"
	@echo "  cat README.md              - 总体介绍"
	@echo "  cat TRACE_RESULTS.md       - **实际 Trace 结果分析 (含原始数据)**"
	@echo "  cat GEM_vs_TTM.md          - GEM vs TTM 详细对比"
	@echo "  cat EXPERIMENTS.md         - 实验结果分析"
	@echo "  cat BPFTRACE.md            - bpftrace 使用指南"
	@echo "  cat 01_gem/README.md       - GEM 详解"
	@echo "  cat 02_kms/README.md       - KMS 详解"
	@echo "  cat 03_ttm/README.md       - TTM 详解"
	@echo ""
	@echo "实验脚本:"
	@echo "  ./trace_with_workload.sh   - **实际 GPU 负载 + bpftrace (推荐)**"
	@echo "  ./trace_gem_ttm.sh         - 内核符号分析"
	@echo ""
	@echo "注意: SR-IOV VF 模式下 DRM_IOCTL_MODE_CREATE_DUMB 被限制"
	@echo "      推荐使用 ../intel_xe/vulkan_dmabuf_test.out 验证功能"

# GEM 实验
gem: $(GEM_TARGETS)

01_gem/gem_alloc.out: 01_gem/gem_alloc.c
	@echo "编译 GEM 分配实验..."
	$(CC) $(CFLAGS) $(LIBDRM_CFLAGS) -o $@ $< $(LIBDRM_LIBS)

01_gem/gem_mmap.out: 01_gem/gem_mmap.c
	@echo "编译 GEM mmap 实验..."
	$(CC) $(CFLAGS) $(LIBDRM_CFLAGS) -o $@ $< $(LIBDRM_LIBS)

# KMS 实验
kms: $(KMS_TARGETS)

02_kms/kms_info.out: 02_kms/kms_info.c
	@echo "编译 KMS 信息实验..."
	$(CC) $(CFLAGS) $(LIBDRM_CFLAGS) -o $@ $< $(LIBDRM_LIBS)

02_kms/kms_setmode.out: 02_kms/kms_setmode.c
	@echo "编译 KMS 设置模式实验..."
	$(CC) $(CFLAGS) $(LIBDRM_CFLAGS) -o $@ $< $(LIBDRM_LIBS)

# TTM 实验
ttm: $(TTM_TARGETS)

03_ttm/ttm_evict.out: 03_ttm/ttm_evict.c
	@echo "编译 TTM 驱逐实验..."
	$(CC) $(CFLAGS) $(LIBDRM_CFLAGS) -o $@ $< $(LIBDRM_LIBS)

# 运行测试
test: gem_test kms_test ttm_test

gem_test: $(GEM_TARGETS)
	@echo ""
	@echo "======================================"
	@echo "GEM 实验"
	@echo "======================================"
	@echo ""
	@echo "[实验 1] GEM 分配测试 (renderD128)"
	@./01_gem/gem_alloc.out /dev/dri/renderD128 2>&1 | head -30 || echo "测试完成"
	@echo ""
	@echo "[实验 2] GEM mmap 测试 (renderD128)"
	@./01_gem/gem_mmap.out /dev/dri/renderD128 2>&1 | head -30 || echo "测试完成"

kms_test: $(KMS_TARGETS)
	@echo ""
	@echo "======================================"
	@echo "KMS 实验"
	@echo "======================================"
	@echo ""
	@echo "[实验 1] KMS 信息查询 (card0)"
	@./02_kms/kms_info.out /dev/dri/card0 2>&1 | head -40 || echo "测试完成"

ttm_test: $(TTM_TARGETS)
	@echo ""
	@echo "======================================"
	@echo "TTM 实验"
	@echo "======================================"
	@echo ""
	@echo "[实验 1] TTM 内存压力测试 (renderD128)"
	@./03_ttm/ttm_evict.out /dev/dri/renderD128 2>&1 | head -40 || echo "测试完成"

# 清理
clean:
	@echo "清理编译产物..."
	rm -f $(ALL_TARGETS)
	@echo "完成"
