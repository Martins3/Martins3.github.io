CUDA_PATH ?= /usr/local/cuda-13.1
NVCC = $(CUDA_PATH)/bin/nvcc
# Blackwell consumer (GB20x) is sm_120. Override for another card, e.g. SMS=89.
SMS ?= 120
GENCODE = $(foreach sm,$(SMS),-gencode arch=compute_$(sm),code=sm_$(sm))
# Keep the system compiler/linker together even inside this repository's Nix shell.
NVCCFLAGS ?= -ccbin /usr/bin/g++ -Xcompiler -B/usr/bin/ -O3 -g -std=c++20 \
	-lineinfo $(GENCODE) -Xcompiler -Wall,-Wextra
LDLIBS = -lnvidia-ml -lcuda \
	-L/usr/lib64 -L$(CUDA_PATH)/lib64 -L$(CUDA_PATH)/targets/x86_64-linux/lib

SRCS = bench.cu grp_device.cu grp_compute.cu grp_memory.cu grp_exec.cu \
	grp_launch.cu grp_pcie.cu
HDRS = bench.h

.PHONY: all smoke clean
all: gpubench.out

gpubench.out: $(SRCS) $(HDRS)
	$(NVCC) $(NVCCFLAGS) $(SRCS) -o $@ $(LDLIBS)

smoke: gpubench.out
	./gpubench.out --quick --samples 1

clean:
	rm -f gpubench.out
