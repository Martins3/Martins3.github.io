# GPU Demo 测试结果

测试环境：NVIDIA GeForce RTX 4060 Laptop GPU (8GB)
测试时间：2026-03-02

## 测试结果

### 1. gpu_3d_load.exe ✅ 成功

```powershell
# 命令
./gpu_3d_load.exe 10

# nvidia-smi 观测
Before:  GPU 12%
During:  GPU 100%
After:   GPU idle
```

**效果**：GPU 利用率从 12% 上升到 100%

### 2. gpu_copy_load.exe ✅ 成功

```powershell
# 命令（下载模式）
./gpu_copy_load.exe 3 1

# 结果
Total data transferred: 26.00 GB
Average bandwidth: 8.64 GB/s
GPU utilization: 100%
```

**效果**：传输带宽 6-8 GB/s，GPU 利用率 100%

### 3. gpu_dedicated_vram.exe ✅ 成功

```powershell
# 命令
./gpu_dedicated_vram.exe 1024 5

# nvidia-smi memory.used
Before:  1802 MiB
During:  2912 MiB (+1110 MiB)
After:   1844 MiB
```

**效果**：显存占用增加约 1GB，释放后恢复

### 4. gpu_shared_vram.exe ⚠️ 部分成功

```powershell
# 命令
./gpu_shared_vram.exe 512 5

# nvidia-smi memory.used
Before:  1809 MiB
During:  1892 MiB (+83 MiB)
After:   1851 MiB
```

**说明**：STAGING 缓冲区位于系统内存，不会完全反映在 GPU 显存统计中

### 5. gpu_decode_load.exe ❌ 失败

```powershell
# nvidia-smi utilization.decoder
During:  0%
```

**原因**：程序使用 Video Processor 而非真正的 Video Decoder

## 结论

| 功能 | 状态 | 说明 |
|-----|------|------|
| 3D 引擎负载 | ✅ | 工作正常 |
| Copy 引擎负载 | ✅ | 工作正常 |
| 专用显存占用 | ✅ | 工作正常 |
| 共享显存占用 | ⚠️ | 受限（系统内存机制） |
| 视频解码 | ❌ | 需要重写，使用真实解码 |

## Windows 任务管理器观察

打开任务管理器 -> 性能 -> GPU，运行程序时可以看到：
- **3D**：随 gpu_3d_load.exe 上升
- **Copy**：随 gpu_copy_load.exe 上升（部分驱动版本）
- **Dedicated GPU memory**：随 gpu_dedicated_vram.exe 上升
