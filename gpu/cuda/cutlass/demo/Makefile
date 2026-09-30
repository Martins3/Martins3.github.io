CUDA_HOME ?= /usr/local/cuda-13.1
CUDA_DRIVER_LIBDIR ?= /usr/lib64
NVCC ?= $(CUDA_HOME)/bin/nvcc
CC      := /usr/bin/gcc
HOST_CXX := /usr/bin/g++
HOST_BINUTILS_DIR := /usr/bin

MODE ?= source
SM ?= 120

LOCAL_CUTLASS_ROOT ?= $(HOME)/data/cutlass
LOCAL_CUTLASS_BUILD ?= $(LOCAL_CUTLASS_ROOT)/build-cmake-sm120-cuda12.8-tools

CUTLASS_PREFIX ?= /usr/local
CUTLASS_ROOT ?= $(LOCAL_CUTLASS_ROOT)
CUTLASS_BUILD ?= $(LOCAL_CUTLASS_BUILD)
CUTLASS_LIBDIR ?=

BUILD_DIR := build
DEMO ?= sgemm_demo
SOURCES := $(wildcard *.cu)
DEMOS := $(basename $(SOURCES))
OUTPUTS := $(addprefix $(BUILD_DIR)/,$(addsuffix .out,$(DEMOS)))
RUN_ARGS ?= 128 128 128

# 固定使用系统工具链，避免 Nix glibc 头文件与 CUDA 头文件混用。
NVCCFLAGS ?= -std=c++17 -O2 -arch=sm_$(SM) --expt-relaxed-constexpr -ccbin $(HOST_CXX) -Xcompiler -B$(HOST_BINUTILS_DIR)
LDFLAGS ?=
LDLIBS ?= -lcudart
CUDA_DRIVER_LINK ?=

ifeq ($(MODE),local)
  CUTLASS_INC := -I$(LOCAL_CUTLASS_ROOT)/include -I$(LOCAL_CUTLASS_ROOT)/tools/util/include
  CUTLASS_LIBRARY_DIR := $(LOCAL_CUTLASS_BUILD)/tools/library
  CUTLASS_LINK := -L$(CUTLASS_LIBRARY_DIR) -Xlinker -rpath -Xlinker $(CUTLASS_LIBRARY_DIR) -lcutlass
else ifeq ($(MODE),system)
  CUTLASS_INC := -I$(CUTLASS_PREFIX)/include
  CUTLASS_LIBRARY_DIR := $(or $(CUTLASS_LIBDIR),$(CUTLASS_PREFIX)/lib64)
  CUTLASS_LINK := -L$(CUTLASS_LIBRARY_DIR) -Xlinker -rpath -Xlinker $(CUTLASS_LIBRARY_DIR) -lcutlass
else ifeq ($(MODE),source)
  CUTLASS_INC := -I$(CUTLASS_ROOT)/include -I$(CUTLASS_ROOT)/tools/util/include
  CUTLASS_LIBRARY_DIR :=
  CUTLASS_LINK :=
  CUDA_DRIVER_PRELOAD ?= $(CUDA_DRIVER_LIBDIR)/libcuda.so.1
else
  $(error MODE must be local, system, or source)
endif

CUDA_DRIVER_PRELOAD ?=
CUTLASS_LIBRARY_PATH_PART := $(if $(CUTLASS_LIBRARY_DIR),:$(CUTLASS_LIBRARY_DIR),)

.PHONY: all run clean print-config check

all: $(OUTPUTS)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.out: %.cu | $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) $(CUTLASS_INC) $< -o $@ $(LDFLAGS) $(CUTLASS_LINK) $(CUDA_DRIVER_LINK) $(LDLIBS)

run: $(BUILD_DIR)/$(DEMO).out
	LD_PRELOAD="$(CUDA_DRIVER_PRELOAD)" LD_LIBRARY_PATH="$(CUDA_HOME)/lib64$(CUTLASS_LIBRARY_PATH_PART):$${LD_LIBRARY_PATH:-}" ./$(BUILD_DIR)/$(DEMO).out $(RUN_ARGS)

check: run

print-config:
	@printf 'MODE=%s\n' '$(MODE)'
	@printf 'DEMO=%s\n' '$(DEMO)'
	@printf 'DEMOS=%s\n' '$(DEMOS)'
	@printf 'OUTPUTS=%s\n' '$(OUTPUTS)'
	@printf 'RUN_ARGS=%s\n' '$(RUN_ARGS)'
	@printf 'NVCC=%s\n' '$(NVCC)'
	@printf 'HOST_CXX=%s\n' '$(HOST_CXX)'
	@printf 'HOST_BINUTILS_DIR=%s\n' '$(HOST_BINUTILS_DIR)'
	@printf 'CUDA_HOME=%s\n' '$(CUDA_HOME)'
	@printf 'CUDA_DRIVER_LIBDIR=%s\n' '$(CUDA_DRIVER_LIBDIR)'
	@printf 'SM=%s\n' '$(SM)'
	@printf 'CUTLASS_INC=%s\n' '$(CUTLASS_INC)'
	@printf 'CUTLASS_LIBRARY_DIR=%s\n' '$(CUTLASS_LIBRARY_DIR)'
	@printf 'CUTLASS_LINK=%s\n' '$(CUTLASS_LINK)'
	@printf 'CUDA_DRIVER_LINK=%s\n' '$(CUDA_DRIVER_LINK)'
	@printf 'CUDA_DRIVER_PRELOAD=%s\n' '$(CUDA_DRIVER_PRELOAD)'

clean:
	rm -rf $(BUILD_DIR)
