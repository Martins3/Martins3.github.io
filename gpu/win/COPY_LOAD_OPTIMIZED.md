# GPU Copy Load Generator - 优化版本说明

## 问题

原版 `gpu_copy_load.exe` 带宽只有 **2.43 GB/s**，Copy 引擎没有跑满。

## 原因分析

| 问题 | 说明 |
|-----|------|
| 同步等待 | `UpdateSubresource` 是同步操作，会等待 GPU 完成 |
| 操作频率低 | 每次传输后 Flush，限制了吞吐量 |
| 缓冲区大小不合适 | 256MB 太小，每次传输的开销占比大 |

## 解决方案

创建 `gpu_copy_load_real.exe`，使用以下优化：

### 1. 使用 STAGING 缓冲区 + Map/Unmap

```c
// Download 模式（GPU -> CPU）
// 步骤 1: GPU -> STAGING (CopyResource)
context->CopyResource(stagingBuffer, gpuBuffer);

// 步骤 2: Map STAGING (强制同步和传输)
context->Map(stagingBuffer, ...);
// 读取数据
context->Unmap(stagingBuffer);
```

### 2. 流水线设计

- 6 个 256MB 缓冲区循环使用
- 每个缓冲区独立操作，减少等待

### 3. 三种模式

| 模式 | 带宽 | 说明 |
|-----|------|------|
| Upload (CPU->GPU) | ~1.7 GB/s | UpdateSubresource 同步限制 |
| Download (GPU->CPU) | ~12 GB/s | **接近 PCIe 极限！** |
| Bidirectional | ~4 GB/s | 同时上下行 |

## 测试结果对比

| 程序 | 模式 | 带宽 | Copy 引擎利用率 |
|-----|------|------|----------------|
| gpu_copy_load.exe | Upload | 2.43 GB/s | 低 |
| gpu_copy_load_real.exe | Upload | 1.71 GB/s | 中 |
| gpu_copy_load_real.exe | **Download** | **12.07 GB/s** | **高** |
| gpu_copy_load_real.exe | Bidirectional | 4.22 GB/s | 中 |

**结论**: Download (GPU->CPU) 模式最能利用 Copy 引擎带宽！

## 使用方法

```bash
# Download 模式（最高带宽）
gpu_copy_load_real.exe 30 1

# Upload 模式
gpu_copy_load_real.exe 30 0

# 双向模式
gpu_copy_load_real.exe 30 2
```

## 监控 Copy 引擎

```bash
# nvidia-smi 监控
nvidia-smi dmon -s pucm

# 或查看 Copy 利用率
nvidia-smi --query-gpu=utilization.gpu,memory.used --format=csv -l 1
```

## 文件列表

| 文件 | 说明 |
|-----|------|
| gpu_copy_load.exe | 原版（带宽低） |
| gpu_copy_load_real.exe | **推荐** - 真实传输，Download 模式可达 12 GB/s |
| gpu_copy_load_max.exe | 多线程版（有 bug） |
| gpu_copy_load_optimized.exe | 单线程优化版 |
| gpu_copy_load_fast.exe | DYNAMIC 缓冲区（被驱动优化，数据不准） |

## 技术细节

### 为什么 Download 比 Upload 快？

1. **Upload**: `UpdateSubresource` 是同步 API，CPU 等待 GPU 确认
2. **Download**: `CopyResource` + `Map` 是异步流水线，GPU 批量执行

### PCIe 带宽限制

RTX 4060 Laptop GPU 使用 PCIe 4.0 x8：
- 理论峰值: ~16 GB/s
- 实测 Download: 12 GB/s (~75% 效率)
- 实测 Upload: 1.7 GB/s (~10% 效率，受同步限制)

## 建议

要最大化 Copy 引擎利用率：
1. 使用 **Download 模式** (GPU -> CPU)
2. 使用 **较大的缓冲区** (256MB+)
3. 使用 **多个缓冲区流水线**
4. 避免频繁 Flush，批量提交
