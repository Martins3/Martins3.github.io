# Unified Makefile for chapter_summaries
# Builds all chapter_demo.cu (and special cases) from root

# CUDA_HOME ?= /usr/local/cuda
# fedora 默认就是 cuda 13.1 的，只是之前为了解决 12.8 才搞的
# 从 nvidia-smi 输出最高支持 13.2
CUDA_HOME = /usr/local/cuda-13.1
NVCC    := $(CUDA_HOME)/bin/nvcc
CC      := /usr/bin/gcc
STD     := c++17
ARCH    := sm_120
OPT     := -O2 -g
HOST_CXX ?= g++
HOST_CXX_PATH := $(shell command -v $(HOST_CXX) 2>/dev/null || echo $(HOST_CXX))
HOST_CXX_REAL := $(shell readlink -f $(HOST_CXX_PATH) 2>/dev/null || echo $(HOST_CXX_PATH))
NVCC_LINK_FLAGS ?= $(if $(findstring /nix/,$(HOST_CXX_REAL)),-Xlinker --dynamic-linker=/lib64/ld-linux-x86-64.so.2 -Xlinker -rpath=/usr/lib64:$(CUDA_HOME)/lib64,)

BASE_FLAGS    := -std=$(STD) $(OPT) $(CPPFLAGS) -ccbin $(HOST_CXX)
DEFAULT_FLAGS := $(BASE_FLAGS) -arch=$(ARCH)
DEFAULT_LINK_FLAGS := $(DEFAULT_FLAGS) $(LDFLAGS) $(NVCC_LINK_FLAGS)
# CFLAGS        := -O2 -Wall -Wextra $(CPPFLAGS) -I$(CUDA_HOME)/include
CFLAGS        := -O0 -g -G -Wextra $(CPPFLAGS) -I$(CUDA_HOME)/include
DRIVER_LIBS   := $(LDFLAGS) -L/usr/lib64 -lcuda -ldl

# Auto-discover all .cu files (exclude cache and preserved duplicate summaries)
CHAPTER_DEMO_CU := $(shell find . -name "*.cu" -not -path "./.ccls-cache/*" -not -path "*/chapter-level-summary/*" -not -path "./basic/driver_module_kernel.cu" | sort)
CHAPTER_DEMOS   := $(CHAPTER_DEMO_CU:%.cu=%.out)
BASIC_C_SRCS    := $(shell find ./basic -maxdepth 1 -name "*.c" | sort)
BASIC_C_OUTS    := $(BASIC_C_SRCS:%.c=%.out)
BASIC_PTX       := ./basic/driver_module_kernel.ptx

# Extra targets
SEP_SUCCESS := ./02.5-nvcc-the-nvidia-cuda-compiler/chapter-level-summary/separate_compilation/separate_success.out

ALL_TARGETS := $(CHAPTER_DEMOS) $(BASIC_C_OUTS) $(BASIC_PTX) $(SEP_SUCCESS)

# Special directories
NVCC25_DIR := 02.5-nvcc-the-nvidia-cuda-compiler
SEP_DIR    := $(NVCC25_DIR)/chapter-level-summary/separate_compilation

.PHONY: all clean run list help verbose keep run-success basic-run basic-test basic-bench basic-info basic-info-compare basic-driver-module basic-um basic-tools-check

all: $(ALL_TARGETS)

help:
	@echo "Unified Makefile for chapter_summaries"
	@echo ""
	@echo "Targets:"
	@echo "  make all         - build all demos"
	@echo "  make clean       - remove all built binaries"
	@echo "  make run         - run all chapter_demo.out demos"
	@echo "  make list        - list all discovered chapter_demo.cu"
	@echo "  make verbose     - nvcc verbose compile (02.5 NVCC)"
	@echo "  make keep        - keep intermediates (02.5 NVCC)"
	@echo "  make run-success - run separate_success.out (02.5 separate compilation)"
	@echo "  make basic-run   - run basic/vector_add.out"
	@echo "  make basic-test  - run basic smoke tests"
	@echo "  make basic-driver-module - run basic Driver API module-load demo"
	@echo ""

list:
	@for f in $(CHAPTER_DEMO_CU); do echo "  $$f"; done

# ---------------------------------------------------------------------------
# Generic compile rule
# ---------------------------------------------------------------------------
%.out: %.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -o $@ $<

%.out: %.c
	$(CC) $(CFLAGS) -o $@ $< $(DRIVER_LIBS)

# ---------------------------------------------------------------------------
# Special compile rules (override generic)
# ---------------------------------------------------------------------------
03.3-cuda-driver-api/chapter_demo.out: 03.3-cuda-driver-api/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -lcuda -o $@ $<

04.20-driver-entry-point-access/chapter_demo.out: 04.20-driver-entry-point-access/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -lcuda -o $@ $<

04.18-cuda-dynamic-parallelism/chapter_demo.out: 04.18-cuda-dynamic-parallelism/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -rdc=true -lcudadevrt -o $@ $<

04.08-error-log-management/chapter_demo.out: 04.08-error-log-management/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -lcuda -o $@ $<

04.17-extended-gpu-memory/chapter_demo.out: 04.17-extended-gpu-memory/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -lcuda -o $@ $<

04.19-cuda-interoperability-with-apis/chapter_demo.out: 04.19-cuda-interoperability-with-apis/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -DGL_GLEXT_PROTOTYPES -lGL -lX11 -o $@ $<

04.07-lazy-loading/chapter_demo.out: 04.07-lazy-loading/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -lcuda -o $@ $<

04.16-virtual-memory-management/chapter_demo.out: 04.16-virtual-memory-management/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -lcuda -o $@ $<

04.16-virtual-memory-management/allocation_fault_demo.out: 04.16-virtual-memory-management/allocation_fault_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -lcuda -o $@ $<

05.3-cpp-language-support/chapter_demo.out: 05.3-cpp-language-support/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) --extended-lambda -o $@ $<

05.8-cuda-cpp-execution-model/chapter_demo.out: 05.8-cuda-cpp-execution-model/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -Wno-deprecated-gpu-targets -o $@ $<

05.7-cuda-cpp-memory-model/chapter_demo.out: 05.7-cuda-cpp-memory-model/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -Xcompiler "-Wall -Wextra" --expt-relaxed-constexpr -o $@ $<

04.3-stream-ordered-memory-allocator/chapter_demo.out: 04.3-stream-ordered-memory-allocator/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -Xcompiler -Wall -o $@ $<

04.11-asynchronous-data-copies/stas_demo_sm90.out: 04.11-asynchronous-data-copies/stas_demo_sm90.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -o $@ $<

04.11-asynchronous-data-copies/tma_bulk_demo_sm90.out: 04.11-asynchronous-data-copies/tma_bulk_demo_sm90.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -o $@ $<

04.10-pipelines/cuda_pipeline_cpp_api.out: 04.10-pipelines/cuda_pipeline_cpp_api.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -o $@ $<

basic/runtime_driver_device_info.out: basic/runtime_driver_device_info.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -o $@ $< -lcuda

basic/driver_module_kernel.ptx: basic/driver_module_kernel.cu
	$(NVCC) $(DEFAULT_FLAGS) -ptx -o $@ $<

basic/driver_module_load.out: basic/driver_module_load.c basic/driver_module_kernel.ptx
	$(CC) $(CFLAGS) -o $@ $< $(DRIVER_LIBS)

# ---------------------------------------------------------------------------
# basic demo run targets
# ---------------------------------------------------------------------------
basic-run: basic/vector_add.out
	cd basic && ./vector_add.out

basic-bench: basic/memory_bandwidth.out
	cd basic && ./memory_bandwidth.out

basic-info: basic/device_info.out
	cd basic && ./device_info.out

basic-info-compare: basic/runtime_driver_device_info.out
	cd basic && ./runtime_driver_device_info.out

basic-driver-module: basic/driver_module_load.out basic/driver_module_kernel.ptx
	cd basic && ./driver_module_load.out

basic-um: basic/unified_memory.out
	cd basic && ./unified_memory.out

basic-tools-check:
	cd basic && ./validate_cuda_tools.sh

BASIC_TEST_BINS := \
	driver_probe.out \
	driver_module_load.out \
	memcpy_roundtrip.out \
	vector_add.out \
	memory_bandwidth.out \
	device_info.out \
	runtime_driver_device_info.out \
	syncwarp_demo.out

basic-test: $(addprefix basic/,$(BASIC_TEST_BINS)) basic/driver_module_kernel.ptx
	@status=0; \
	for bin in $(BASIC_TEST_BINS); do \
		echo "== basic/$${bin%.out} =="; \
		(cd basic && ./$$bin) || status=$$?; \
		echo ""; \
	done; \
	exit $$status

# ---------------------------------------------------------------------------
# 02.5 NVCC special targets
# ---------------------------------------------------------------------------
verbose: $(NVCC25_DIR)/chapter_demo.cu
	$(NVCC) $(DEFAULT_LINK_FLAGS) -v $< -o $(NVCC25_DIR)/chapter_demo_v.out 2>&1 | tee $(NVCC25_DIR)/nvcc_v_output.txt

keep: $(NVCC25_DIR)/chapter_demo.cu
	mkdir -p $(NVCC25_DIR)/nvcc_intermediates
	$(NVCC) $(DEFAULT_LINK_FLAGS) -keep --keep-dir $(NVCC25_DIR)/nvcc_intermediates $< -o $(NVCC25_DIR)/chapter_demo_keepdir.out

# ---------------------------------------------------------------------------
# separate_compilation
# ---------------------------------------------------------------------------
$(SEP_DIR)/example_no_dc.o: $(SEP_DIR)/example.cu
	$(NVCC) $(DEFAULT_FLAGS) -c $< -o $@

$(SEP_DIR)/definition_no_dc.o: $(SEP_DIR)/definition.cu
	$(NVCC) $(DEFAULT_FLAGS) -c $< -o $@

$(SEP_DIR)/separate_fail.out: $(SEP_DIR)/example_no_dc.o $(SEP_DIR)/definition_no_dc.o
	$(NVCC) $(NVCC_LINK_FLAGS) $^ -o $@ || echo "[expected error] link failed!"

$(SEP_DIR)/example_dc.o: $(SEP_DIR)/example.cu
	$(NVCC) $(DEFAULT_FLAGS) -dc $< -o $@

$(SEP_DIR)/definition_dc.o: $(SEP_DIR)/definition.cu
	$(NVCC) $(DEFAULT_FLAGS) -dc $< -o $@

$(SEP_SUCCESS): $(SEP_DIR)/example_dc.o $(SEP_DIR)/definition_dc.o
	$(NVCC) $(DEFAULT_LINK_FLAGS) $^ -o $@

run-success: $(SEP_SUCCESS)
	$(SEP_SUCCESS)

# ---------------------------------------------------------------------------
# clean
# ---------------------------------------------------------------------------
clean:
	rm -f $(CHAPTER_DEMOS)
	rm -f $(BASIC_C_OUTS) $(BASIC_PTX) basic/driver_module_kernel.out
	rm -f $(SEP_DIR)/*.o $(SEP_DIR)/*.out
	rm -f $(NVCC25_DIR)/chapter_demo_v.out $(NVCC25_DIR)/chapter_demo_keep.out $(NVCC25_DIR)/chapter_demo_keepdir.out
	rm -f $(NVCC25_DIR)/chapter_demo.cpp*.ii $(NVCC25_DIR)/chapter_demo.cudafe1.* $(NVCC25_DIR)/chapter_demo.fatbin*
	rm -f $(NVCC25_DIR)/chapter_demo.module_id $(NVCC25_DIR)/chapter_demo.o $(NVCC25_DIR)/chapter_demo.ptx $(NVCC25_DIR)/chapter_demo.sm_*.cubin
	rm -f $(NVCC25_DIR)/chapter_demo_keep_* $(NVCC25_DIR)/chapter_demo_keepdir_*
	rm -rf $(NVCC25_DIR)/nvcc_intermediates $(NVCC25_DIR)/nvcc_intermediates2 $(NVCC25_DIR)/nvcc_v_output.txt
