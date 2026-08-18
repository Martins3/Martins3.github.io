# Intel Xe GPU 测试程序 Makefile
# SR-IOV VF 模式测试

CXX = g++
CC = gcc
CXXFLAGS = -Wall -O2 -std=c++11
CFLAGS = -Wall -O2

# 使用 pkg-config 获取编译参数
VULKAN_CFLAGS := $(shell pkg-config --cflags vulkan 2>/dev/null || echo "")
VULKAN_LIBS := $(shell pkg-config --libs vulkan 2>/dev/null || echo "-lvulkan")

EGL_CFLAGS := $(shell pkg-config --cflags egl glesv2 2>/dev/null || echo "")
EGL_LIBS := $(shell pkg-config --libs egl glesv2 2>/dev/null || echo "-lEGL -lGLESv2")

OPENCL_CFLAGS := $(shell pkg-config --cflags OpenCL 2>/dev/null || echo "")
OPENCL_LIBS := $(shell pkg-config --libs OpenCL 2>/dev/null || echo "-lOpenCL")

LIBDRM_CFLAGS := $(shell pkg-config --cflags libdrm 2>/dev/null || echo "")
LIBDRM_LIBS := $(shell pkg-config --libs libdrm 2>/dev/null || echo "-ldrm")

# 可执行文件
TARGETS = vulkan_mem_test.out vulkan_compute_test.out gpu_test.out cl_test.out drm_gpu_test.out dmabuf_test.out vulkan_dmabuf_test.out

# 默认目标
.PHONY: all clean run help test

all: $(TARGETS)

help:
	@echo "Intel Xe GPU 测试程序 (SR-IOV VF 模式)"
	@echo ""
	@echo "可用目标:"
	@echo "  make              - 编译 Vulkan 显存测试 (推荐)"
	@echo "  make all          - 编译所有测试程序"
	@echo "  make run          - 运行 Vulkan 测试 (64MB)"
	@echo "  make run SIZE=128 - 运行 Vulkan 测试 (指定大小)"
	@echo "  make clean        - 清理编译产物"
	@echo "  make help         - 显示帮助"
	@echo ""
	@echo "单独编译:"
	@echo "  make vulkan_mem_test.out     - Vulkan 显存测试"
	@echo "  make vulkan_compute_test.out - Vulkan 计算框架"
	@echo "  make gpu_test.out            - EGL/GLES 测试"
	@echo "  make cl_test.out             - OpenCL 测试"
	@echo "  make drm_gpu_test.out        - DRM 测试"
	@echo "  make dmabuf_test.out         - DMA-BUF 基础测试"
	@echo "  make vulkan_dmabuf_test.out  - Vulkan DMA-BUF 导出测试"

# Vulkan 测试 (推荐)
vulkan_mem_test.out: vulkan_mem_test.cpp
	@echo "编译 Vulkan 显存测试..."
	$(CXX) $(CXXFLAGS) $(VULKAN_CFLAGS) -o $@ $< $(VULKAN_LIBS)

vulkan_compute_test.out: vulkan_compute_test.cpp
	@echo "编译 Vulkan 计算框架..."
	$(CXX) $(CXXFLAGS) $(VULKAN_CFLAGS) -o $@ $< $(VULKAN_LIBS)

# EGL/GLES 测试
gpu_test.out: gpu_test.c
	@echo "编译 EGL/GLES 测试..."
	$(CC) $(CFLAGS) $(EGL_CFLAGS) -o $@ $< $(EGL_LIBS)

# OpenCL 测试
cl_test.out: cl_test.c
	@echo "编译 OpenCL 测试..."
	$(CC) $(CFLAGS) $(OPENCL_CFLAGS) -o $@ $< $(OPENCL_LIBS) -Wno-deprecated-declarations

# DRM 测试
drm_gpu_test.out: drm_gpu_test.c
	@echo "编译 DRM 测试..."
	$(CC) $(CFLAGS) $(LIBDRM_CFLAGS) -o $@ $< $(LIBDRM_LIBS)

# DMA-BUF 测试
dmabuf_test.out: dmabuf_test.c
	@echo "编译 DMA-BUF 测试..."
	$(CC) $(CFLAGS) $(LIBDRM_CFLAGS) -o $@ $< $(LIBDRM_LIBS)

# Vulkan DMA-BUF 测试
vulkan_dmabuf_test.out: vulkan_dmabuf_test.cpp
	@echo "编译 Vulkan DMA-BUF 测试..."
	$(CXX) $(CXXFLAGS) $(VULKAN_CFLAGS) -o $@ $< $(VULKAN_LIBS)

# 编译所有
all_targets: $(TARGETS)
	@echo "所有测试程序编译完成"

# 运行测试
SIZE ?= 64
run: vulkan_mem_test.out
	@echo "运行 Vulkan 显存测试 (SIZE=$(SIZE) MB)..."
	./vulkan_mem_test.out $(SIZE)

# 测试所有程序
test: all
	@echo ""
	@echo "======================================"
	@echo "运行所有测试程序"
	@echo "======================================"
	@echo ""
	@echo "[1/5] Vulkan 显存测试"
	@./vulkan_mem_test.out 32 
	@echo ""
	@echo "[2/5] Vulkan 计算测试"
	@timeout 5 ./vulkan_compute_test.out 32 2>&1 | grep -E "(成功|失败|目标显存|发现|显存测试完成)" | head -10 || echo "测试失败或超时"
	@echo ""
	@echo "[3/5] EGL/GLES 测试"
	@timeout 3 ./gpu_test.out 32 2>&1 | grep -E "(成功|失败|目标显存)" | head -5 || echo "测试失败(需要图形环境)"
	@echo ""
	@echo "[4/5] OpenCL 测试"
	@timeout 3 ./cl_test.out 32 2>&1 | grep -E "(成功|失败|目标显存|OpenCL)" | head -5 || echo "测试失败或超时(SR-IOV VF不支持)"
	@echo ""
	@echo "[5/5] DRM 测试"
	@sudo ./drm_gpu_test.out 32

# 清理
clean:
	@echo "清理编译产物..."
	rm -f $(TARGETS)
	@echo "完成"
