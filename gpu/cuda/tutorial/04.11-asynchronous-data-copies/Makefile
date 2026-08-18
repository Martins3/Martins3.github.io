CUDA_HOME ?= /usr/local/cuda
NVCC  := $(CUDA_HOME)/bin/nvcc
ARCH  := sm_120
STD   := c++17
OPT   := -O2
HOST_CXX ?= /usr/bin/g++-14

COMMON_FLAGS := -std=$(STD) $(OPT) -ccbin $(HOST_CXX) -arch=$(ARCH)

.PHONY: all clean ldgsts tma stas

all: ldgsts

ldgsts: chapter_demo.out

tma: tma_bulk_demo_sm90.out

stas: stas_demo_sm90.out

chapter_demo.out: chapter_demo.cu
	$(NVCC) $(COMMON_FLAGS) -o $@ $<

tma_bulk_demo_sm90.out: tma_bulk_demo_sm90.cu
	$(NVCC) $(COMMON_FLAGS) -o $@ $<

stas_demo_sm90.out: stas_demo_sm90.cu
	$(NVCC) $(COMMON_FLAGS) -o $@ $<

clean:
	rm -f chapter_demo.out tma_bulk_demo_sm90.out stas_demo_sm90.out
