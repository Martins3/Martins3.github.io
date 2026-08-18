# CUTLASS 在本机上的实际验证记录

本文档记录 2026-05-17 在这台机器上重新搭建 CUTLASS 环境的实际结果。
旧记录是 GTX 1060 / `sm_61`，当前机器已经换成 RTX 5060 Ti / `sm_120`。

CUTLASS 源码本地路径：

```bash
~/data/cutlass
```

## 当前硬件和工具链

GPU：

```text
NVIDIA GeForce RTX 5060 Ti, compute capability 12.0, driver 580.159.03, 16311 MiB
```

本机可用 CUDA Toolkit：

```text
/usr/local/cuda-12.8/bin/nvcc: release 12.8, V12.8.93
/usr/local/cuda-13.1/bin/nvcc: release 13.1, V13.1.115
```

当前验证通过的组合：

- CUDA Toolkit: `/usr/local/cuda-12.8`
- Host compiler: `/usr/bin/g++-14`
- CUTLASS arch: `120`
- Build dir: `build-cmake-sm120-cuda12.8-tools`
- CUTLASS examples/tests/library/profiler/tools: 全部打开

## 配置环境

不要直接依赖默认 PATH。当前 shell 默认找不到 `nvcc`，需要显式指定 CUDA
Toolkit。

```bash
cd ~/data/cutlass

export CUDA_HOME=/usr/local/cuda-12.8
export CUDACXX=/usr/local/cuda-12.8/bin/nvcc
export PATH=/usr/local/cuda-12.8/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/cuda-12.8/lib64:${LD_LIBRARY_PATH:-}
```

检查：

```bash
nvcc --version
nvidia-smi --query-gpu=name,compute_cap,driver_version,memory.total --format=csv,noheader
```

## 用官方 CMake 流程编译并运行 `00_basic_gemm`

配置命令：

```bash
cmake -S . -B build-cmake-sm120-cuda12.8-tools -G Ninja \
  -DCMAKE_CXX_COMPILER=/usr/bin/g++-14 \
  -DCMAKE_CUDA_HOST_COMPILER=/usr/bin/g++-14 \
  -DCMAKE_CUDA_COMPILER=/usr/local/cuda-12.8/bin/nvcc \
  -DCUTLASS_NVCC_ARCHS=120 \
  -DCUTLASS_ENABLE_TESTS=ON \
  -DCUTLASS_ENABLE_LIBRARY=OFF \
  -DCUTLASS_ENABLE_PROFILER=ON \
  -DCUTLASS_ENABLE_TOOLS=ON \
  -DCUTLASS_ENABLE_EXAMPLES=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

本次配置结果：

```text
-- CUTLASS 4.4.2
-- The CUDA compiler identification is NVIDIA 12.8.93 with host compiler GNU 14.3.1
-- Found CUDAToolkit: /usr/local/cuda-12.8/targets/x86_64-linux/include (found version "12.8.93")
-- CUDA Compilation Architectures: 120
-- CUTLASS: Applying -mcmodel=medium for large library support (Linux x86_64, GNU)
-- Completed generation of library instances. See /home/martins3/data/cutlass/build-cmake-sm120-cuda12.8-tools/tools/library/library_instance_generation.log for more information.
-- Configuring done
-- Generating done
-- Build files have been written to: /home/martins3/data/cutlass/build-cmake-sm120-cuda12.8-tools
```

注意：

- 这次是全开配置：tests、library、profiler、tools、examples 都是 `ON`
- CMake 输出里 `cuBLAS Disabled.`，所以后面的 profiler 结果里 `cuBLAS: Not run`
- `CUTLASS_ENABLE_LIBRARY=ON` 会生成并编译 profiler operation registry，构建量很大

构建：

```bash
cmake --build build-cmake-sm120-cuda12.8-tools --target 00_basic_gemm -j8
```
或者直接到 build-cmake-sm120-cuda12.8-tools 目录下
```bash
ninja
```


运行：

```bash
./build-cmake-sm120-cuda12.8-tools/examples/00_basic_gemm/00_basic_gemm
```

实际结果：

```text
Passed.
```

ctest 验证：

```bash
cd ~/data/cutlass/build-cmake-sm120-cuda12.8-tools
ctest -R test_examples_00_basic_gemm --output-on-failure
```

实际结果：

```text
Test project /home/martins3/data/cutlass/build-cmake-sm120-cuda12.8-tools
    Start 1: ctest_examples_00_basic_gemm
1/1 Test #1: ctest_examples_00_basic_gemm .....   Passed    0.26 sec

100% tests passed, 0 tests failed out of 1
```

## 构建并运行 `cutlass_profiler`

构建 profiler：

```bash
cmake --build build-cmake-sm120-cuda12.8-tools --target cutlass_profiler -j8
```

这一步实际会编译 `tools/library/generated/...` 下的大量 generated operation
对象。本次 Ninja 显示大约 1479 个步骤，最后完成：

```text
[1477/1479] Linking CXX shared library tools/library/libcutlass.so
[1478/1479] Linking CXX executable tools/profiler/cutlass_profiler
```

运行指定 sgemm case：

```bash
cd ~/data/cutlass/build-cmake-sm120-cuda12.8-tools

CUDA_HOME=/usr/local/cuda-12.8 \
PATH=/usr/local/cuda-12.8/bin:$PATH \
LD_LIBRARY_PATH=/usr/local/cuda-12.8/lib64:${LD_LIBRARY_PATH:-} \
./tools/profiler/cutlass_profiler --kernels=sgemm --m=4352 --n=4096 --k=4096
```

实际结果：8 个 CUTLASS sgemm operation 全部 `Passed`。

本次最好结果：

```text
Operation: cutlass_simt_sgemm_128x128_8x2_tt_align1
Runtime: 8.30216 ms
Math: 17593.6 GFLOP/s
Verification: ON
Disposition: Passed
```

完整 CSV 结果摘要：

```text
cutlass_simt_sgemm_128x128_8x2_nn_align1: 8.30633 ms, 17584.7 GFLOP/s, passed
cutlass_simt_sgemm_128x128_8x2_nt_align1: 9.74709 ms, 14985.5 GFLOP/s, passed
cutlass_simt_sgemm_128x128_8x2_tn_align1: 9.23558 ms, 15815.4 GFLOP/s, passed
cutlass_simt_sgemm_128x128_8x2_tt_align1: 8.30216 ms, 17593.6 GFLOP/s, passed
cutlass_simt_sgemm_256x128_8x5_nn_align1: 9.02869 ms, 16177.8 GFLOP/s, passed
cutlass_simt_sgemm_256x128_8x5_nt_align1: 8.63726 ms, 16911.0 GFLOP/s, passed
cutlass_simt_sgemm_256x128_8x5_tn_align1: 9.82907 ms, 14860.5 GFLOP/s, passed
cutlass_simt_sgemm_256x128_8x5_tt_align1: 9.10873 ms, 16035.7 GFLOP/s, passed
```

## 我实际踩到的问题

### 1. 当前默认 shell 找不到 `nvcc`

现象：

```text
zsh:1: command not found: nvcc
```

原因是 CUDA Toolkit 没有放进默认 PATH。直接使用上面的 `export CUDA_HOME`
和 `PATH` 配置即可。

### 2. 为什么全开配置会编译大量 `sm50/sm75/sm80` 文件

`CUTLASS_NVCC_ARCHS=120` 控制的是 nvcc 面向哪个 GPU 架构生成代码；它不等于
只生成 `sm120` 的 profiler operation catalog。

`CUTLASS_ENABLE_LIBRARY=ON` 会让 CUTLASS 生成 profiler 的 operation
registry，文件位于：

```text
tools/library/generated/
```

这个 registry 按 CUTLASS 的 operation family 展开，包括不同历史架构族、数据
类型和算子类型，例如：

```text
generated/gemm/50/sgemm/
generated/gemm/80/sgemm/
generated/conv2d/80/
generated/rank_k/80/
generated/trmm/80/
```

所以构建 `cutlass_profiler` 时会看到很多文件名带 `sm50/sm75/sm80`。这些名字
表示 CUTLASS profiler catalog 里的 operation 类别，不表示本次没有使用
`CUTLASS_NVCC_ARCHS=120`。

### 3. CUDA 13.1 当前在 CMake CUDA compiler probe 阶段失败

我试过用 `/usr/local/cuda-13.1/bin/nvcc` 配合 `/usr/bin/g++-14`：

```bash
export CUDA_HOME=/usr/local/cuda-13.1
export CUDACXX=/usr/local/cuda-13.1/bin/nvcc
```

CMake 在编译 `CMakeCUDACompilerId.cu` 时失败，关键错误是：

```text
/usr/include/bits/mathcalls.h(206): error: exception specification is incompatible with that of previous function "rsqrt"
/usr/include/bits/mathcalls.h(206): error: exception specification is incompatible with that of previous function "rsqrtf"
```

所以这台机器上，当前可复现的 CUTLASS C++ 示例环境先固定为 CUDA 12.8。
