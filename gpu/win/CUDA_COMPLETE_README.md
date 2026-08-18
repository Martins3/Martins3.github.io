# GPU Load Generator - 完整 CUDA 版本

## 概述

基于 **CUDA Driver API** 的完整 GPU 测试套件，直接操作 NVIDIA GPU 硬件。

## 版本对比

### DirectX 11 vs CUDA Driver API

| 测试项 | DirectX 11 | CUDA Driver API | 优势 |
|-------|-----------|-----------------|------|
| **3D/Compute** | 渲染三角形 | PTX 内核计算 | 直接控制计算核心 |
| **Copy** | ~2.4 GB/s | **~12.5 GB/s** | 直接 PCIe 访问 |
| **VRAM** | D3D11_USAGE_DEFAULT | cudaMalloc | 精确内存控制 |
| **Decode** | Video Processor | NVDEC (via FFmpeg) | 专用解码硬件 |

## CUDA 版本程序

### 1. cuda_3d_load.exe - 计算负载

使用 PTX 内核进行大规模并行计算。

```cmd
cuda_3d_load.exe [duration_sec] [intensity]

# 示例：运行 30 秒，每元素 1000 次迭代
cuda_3d_load.exe 30 1000
```

**原理**:
- 使用内联 PTX 代码编译 CUDA 内核
- 执行大量 sin/cos/sqrt 数学运算
- 模拟 3D 像素着色器的计算特征

### 2. cuda_driver_gpu_load.exe - Copy 引擎

底层数据传输测试。

```cmd
cuda_driver_gpu_load.exe copy [duration] [mode]

# mode: 0=Upload, 1=Download, 2=Bidirectional
cuda_driver_gpu_load.exe copy 30 1
```

**性能**:
- Upload: ~12.4 GB/s
- Download: ~12.3 GB/s
- 比 D3D11 版本快 **5 倍**

### 3. cuda_vram_load.exe - 显存分配

直接 GPU 内存分配。

```cmd
cuda_vram_load.exe [size_mb] [duration] [type]

# type: 0=Dedicated, 1=Managed, 2=Mixed
cuda_vram_load.exe 4096 0 0  # 4GB 专用显存
cuda_vram_load.exe 2048 0 1  # 2GB 统一内存
```

**内存类型**:
- **Dedicated**: `cuMemAlloc` - 独占 GPU 显存
- **Managed**: `cuMemAllocManaged` - CPU/GPU 共享页

### 4. cuda_decode_load.exe - NVDEC 解码

硬件视频解码测试。

```cmd
cuda_decode_load.exe info              # 显示解码器信息
cuda_decode_load.exe test 30 4         # 4 实例解码 30 秒
```

**原理**:
- 调用 FFmpeg + h264_cuvid (NVDEC)
- NVDEC 是独立于 CUDA 的专用硬件
- 实时监控 GPU/Decoder 利用率

## 技术实现

### CUDA Driver API 直接调用

无需 CUDA Toolkit，直接加载 `nvcuda.dll`：

```c
// 动态加载 CUDA Driver
HMODULE cuda = LoadLibraryA("nvcuda.dll");
cuInit_t cuInit = (cuInit_t)GetProcAddress(cuda, "cuInit");
cuMemAlloc_t cuMemAlloc = ...

// 直接使用
cuInit(0);
cuMemAlloc(&ptr, size);
```

### PTX 内核（3D 计算）

使用内联汇编风格的 PTX 代码：

```asm
.entry compute_kernel(
    .param .u64 data,
    .param .u64 n,
    .param .u32 iterations
){
    // CUDA 并行线程计算
    sin.approx.f32 %f3, %f2;
    cos.approx.f32 %f4, %f2;
    // ...
}
```

## 性能基准

### 测试环境
- GPU: NVIDIA GeForce RTX 4060 Laptop
- VRAM: 8GB
- PCIe: 4.0 x8

### 实测结果

| 程序 | 资源 | 性能 | 说明 |
|-----|------|------|------|
| cuda_3d_load | 计算核心 | 高 | 36K 并行线程 |
| cuda_driver_gpu_load | Copy 引擎 | **12.4 GB/s** | 接近 PCIe 极限 |
| cuda_vram_load | 显存 | - | 精确分配 |
| cuda_decode_load | NVDEC | - | 硬件解码 |

## 编译

```cmd
# 使用 MSVC 编译器
cl /O2 cuda_3d_load.c /out:cuda_3d_load.exe
cl /O2 cuda_vram_load.c /out:cuda_vram_load.exe
cl /O2 cuda_decode_load.c /out:cuda_decode_load.exe
cl /O2 cuda_driver_gpu_load.c /out:cuda_driver_gpu_load.exe
```

**要求**: Windows + NVIDIA GPU 驱动

## 与 llama.cpp 的关系

llama.cpp 使用类似的底层方法：
- `ggml-cuda`: CUDA Runtime/Backend 封装
- `cudaMalloc/cudaMemcpy`: 底层内存操作
- 直接 kernel 调用实现自定义算子

本套件使用更底层的 **Driver API**，展示了：
1. 不依赖 CUDA Toolkit 的调用方式
2. PTX 内核直接编程
3. 动态加载和运行时兼容

## 文件列表

| 文件 | 说明 |
|-----|------|
| cuda_3d_load.c | 计算负载 (PTX 内核) |
| cuda_vram_load.c | 显存分配 |
| cuda_decode_load.c | NVDEC 解码 |
| cuda_driver_gpu_load.c | Copy 引擎 |
| *.exe | 编译后的可执行文件 |
| CUDA_COMPLETE_README.md | 本文档 |

## 总结

CUDA 版本优势：
1. **5 倍性能提升** (Copy 带宽)
2. **直接硬件访问** (无 Runtime 开销)
3. **精确控制** (内存类型、计算参数)
4. **零依赖** (只需 nvcuda.dll)
