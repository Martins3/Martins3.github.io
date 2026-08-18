# GPU Load Generator - 完整版本对比

## 版本总览

| 版本 | API 层级 | 依赖 | 适用场景 |
|-----|---------|------|---------|
| **DirectX 11** | 高层 | Windows + GPU 驱动 | 通用兼容性 |
| **CUDA Driver** | 底层 | nvcuda.dll (随驱动) | 性能最大化 |

---

## 详细对比

### 1. 3D / 计算负载

| 程序 | API | 实现方式 | 特点 |
|-----|-----|---------|------|
| gpu_3d_load.exe | D3D11 | 渲染三角形 | 窗口化，VS/PS 着色器 |
| **cuda_3d_load.exe** | **CUDA** | **PTX 计算内核** | **直接计算核心，36K 线程** |

**性能**: CUDA 版本可更精确控制计算负载类型。

### 2. Copy 引擎负载

| 程序 | API | Upload | Download | 技术 |
|-----|-----|--------|----------|------|
| gpu_copy_load.exe | D3D11 | 2.43 GB/s | ~2 GB/s | UpdateSubresource |
| gpu_copy_load_real.exe | D3D11 | 1.7 GB/s | **12.07 GB/s** | CopyResource+Map |
| **cuda_driver_gpu_load.exe** | **CUDA** | **12.43 GB/s** | **12.27 GB/s** | **cuMemcpy** |

**结论**: CUDA 版本达到 **PCIe 4.0 x8 理论极限 (~16 GB/s) 的 75%**！

### 3. 显存分配

| 程序 | API | 专用显存 | 共享内存 | 精度 |
|-----|-----|---------|---------|------|
| gpu_dedicated_vram.exe | D3D11 | ✅ 精确 | ❌ | 纹理分配 |
| gpu_shared_vram.exe | D3D11 | ❌ | ⚠️ 部分 | STAGING 缓冲区 |
| gpu_shared_memory_real.exe | D3D11 | ✅ 溢出触发 | ✅ 溢出时 | 填满策略 |
| **cuda_vram_load.exe** | **CUDA** | **✅ 精确** | **✅ 统一内存** | **cuMemAlloc** |

### 4. 视频解码

| 程序 | API | 引擎 | 实现 |
|-----|-----|------|------|
| gpu_decode_load.exe | D3D11 | Video Processor | VideoProcessorBlt |
| gpu_decode_nvdec.exe | FFmpeg | NVDEC | h264_cuvid |
| **cuda_decode_load.exe** | **FFmpeg+CUDA** | **NVDEC** | **+ 实时监控** |

**注意**: NVDEC 是独立于 CUDA 的硬件单元，CUDA 用于调度和监控。

---

## 性能基准 (RTX 4060 Laptop)

| 测试项 | DX11 最佳 | CUDA 最佳 | 提升 |
|-------|----------|----------|------|
| Copy Upload | 2.43 GB/s | **12.43 GB/s** | **5.1x** |
| Copy Download | 12.07 GB/s | **12.27 GB/s** | 相当 |
| 3D/Compute | 中等 | **高** | 更精确控制 |
| VRAM 分配 | 间接 | **直接** | 更精确 |

---

## 使用建议

### 选择 DirectX 11 版本当：
- 需要跨平台兼容性
- 无需极致性能
- 系统可能没有 NVIDIA GPU

### 选择 CUDA 版本当：
- **需要最大 Copy 带宽**
- 需要精确 GPU 控制
- 进行性能基准测试
- 学习底层 GPU 编程

---

## 完整文件列表

### DirectX 11 版本
```
gpu_3d_load.exe          - 3D 引擎 (渲染)
gpu_copy_load.exe        - Copy 引擎 (基础)
gpu_copy_load_real.exe   - Copy 引擎 (优化)
gpu_dedicated_vram.exe   - 专用显存
gpu_shared_vram.exe      - 共享显存 (STAGING)
gpu_shared_memory_real.exe - 共享显存 (溢出)
gpu_decode_load.exe      - Video Processor
gpu_decode_nvdec.exe     - NVDEC (FFmpeg)
```

### CUDA 版本
```
cuda_3d_load.exe         - 计算核心 (PTX)
cuda_vram_load.exe       - 显存分配
cuda_decode_load.exe     - NVDEC + 监控
cuda_driver_gpu_load.exe - Copy 引擎 (底层)
```

---

## 技术栈对比

```
┌─────────────────────────────────────────────────────────────┐
│                    DirectX 11 架构                          │
├─────────────────────────────────────────────────────────────┤
│  Application -> D3D11 Runtime -> User Mode Driver           │
│                                     -> Kernel Mode Driver   │
│                                              -> GPU Hardware│
└─────────────────────────────────────────────────────────────┘
                           ↓ 多层抽象，性能损失

┌─────────────────────────────────────────────────────────────┐
│                   CUDA Driver API 架构                      │
├─────────────────────────────────────────────────────────────┤
│  Application -> nvcuda.dll (Driver API)                     │
│                      -> Kernel Mode Driver                  │
│                               -> GPU Hardware               │
└─────────────────────────────────────────────────────────────┘
                           ↓ 更少层次，更高性能
```

---

## 总结

- **日常使用**: DirectX 11 版本足够
- **性能测试**: 使用 CUDA 版本获取真实硬件性能
- **学习目的**: CUDA 版本展示底层 GPU 工作原理
