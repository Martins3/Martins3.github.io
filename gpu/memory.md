# 显卡内存分析

## 系统概述

当前系统为物理机（非虚拟机），配备双显卡配置：
- **Intel UHD Graphics 770** (核显) - 使用中
- **NVIDIA GeForce GTX 1060 3GB** (独显) - 被 VFIO 直通

---

## 一、硬件配置详情

### 1.1 Intel UHD Graphics 770 (Raptor Lake-S GT1)

| 属性 | 值 |
|------|-----|
| 设备ID | `8086:A780` |
| PCI插槽 | `0000:00:02.0` |
| 驱动 | `i915` |
| 状态 | 活跃，作为显示输出 |

**PCI BAR 空间分配：**

```
BAR0: 0x6012000000-0x6012ffffff (16MB, 64-bit non-prefetchable)
BAR2: 0x4000000000-0x400fffffff (256MB, 64-bit prefetchable)  <-- 显存MMIO
BAR4: 0x5000-0x503f (64字节, I/O端口)
BAR6: 0x000c0000-0x000dffff (128KB, Expansion ROM, virtual/disabled)
BAR8: 0x4010000000-0x4016ffffff (112MB, 64-bit non-prefetchable)
BAR10: 0x4020000000-0x40ffffffff (224MB, 64-bit prefetchable)
```

**关键观察：**
- 总 BAR 空间约 608MB
- 256MB 的 prefetchable 区域是主要的 GPU 显存映射区
- 使用 VT-d 进行图形访问隔离 (`VT-d active for gfx access`)

### 1.2 NVIDIA GeForce GTX 1060 3GB

| 属性 | 值 |
|------|-----|
| 设备ID | GP106 |
| PCI插槽 | `0000:01:00.0` |
| 驱动 | `vfio-pci` (当前被直通) |
| 理论显存 | 3GB GDDR5 |

**PCI BAR 空间分配：**

```
BAR0: 0x81000000-0x81ffffff (16MB, 32-bit non-prefetchable)
BAR1: 0x6000000000-0x600fffffff (256MB, 64-bit prefetchable)  <-- 显存MMIO
BAR3: 0x6010000000-0x6011ffffff (32MB, 64-bit prefetchable)
BAR5: 0x4000-0x407f (128字节, I/O端口)
BAR7: 0x82000000-0x8207ffff (512KB, Expansion ROM, disabled)
```

**关键观察：**
- 总 BAR 空间约 304MB
- 288MB 的 prefetchable 区域用于显存访问
- 当前被 `vfio-pci` 绑定，用于虚拟机 GPU 直通
- 物理机上 `nouveau` 驱动加载但未使用

---

## 二、显卡内存架构分析

### 2.1 Intel 核显内存架构

Intel 核显（UHD Graphics 770）采用**统一内存架构 (UMA)**：

```
┌─────────────────────────────────────────┐
│           系统主内存 (DDR5)              │
│  ┌─────────────────────────────────┐    │
│  │     动态视频内存 (DVMT)          │    │
│  │  ┌─────────┐    ┌─────────┐     │    │
│  │  │  显存   │←──→│  系统   │     │    │
│  │  │  区域   │    │  内存   │     │    │
│  │  └─────────┘    └─────────┘     │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

**特点：**
1. **无专用显存**：与系统内存共享
2. **动态分配**：根据需求动态从系统内存中分配
3. **零拷贝优势**：CPU 和 GPU 之间数据传输无需拷贝

**Intel 显存分配机制：**
- **DVMT (Dynamic Video Memory Technology)**：动态视频内存技术
- 预分配 (Pre-allocated)：BIOS 设置中可配置固定显存
- 动态分配 (Dynamic)：运行时根据需要从系统内存中分配

### 2.2 NVIDIA 独显内存架构

NVIDIA GTX 1060 采用**独立显存架构**：

```
┌─────────────────┐      PCIe 3.0 x16      ┌─────────────────┐
│                 │ ←────────────────────→ │  GPU + GDDR5    │
│   CPU/系统内存   │                        │  3GB 专用显存    │
│                 │                        │                 │
└─────────────────┘                        └─────────────────┘
```

**特点：**
1. **专用显存**：3GB GDDR5，独立于系统内存
2. **高带宽**：GDDR5 提供比系统内存更高的带宽 (~192 GB/s)
3. **数据拷贝**：CPU-GPU 数据传输需要通过 PCIe

---

## 三、PCIe BAR 空间与显存映射

### 3.1 BAR 空间类型解析

| BAR 类型 | 特性 | 用途 |
|---------|------|------|
| **32-bit non-prefetchable** | 严格的读写顺序 | 控制寄存器、配置空间 |
| **64-bit prefetchable** | 允许预取、合并写 | 大容量显存映射 |
| **I/O Space** | 传统 I/O 端口 | 遗留设备兼容 |

### 3.2 显存地址映射

```
系统物理地址空间 (64-bit)
├─ 0x00000000_00000000 ─┬─ 传统内存 (DRAM)
│                       │
├─ 0x00000040_00000000 ─┼─ Intel GPU BAR2 (256MB prefetchable)
│                       │
├─ 0x00000060_10000000 ─┼─ NVIDIA GPU BAR1 (256MB prefetchable)
│                       │
├─ 0x00000060_12000000 ─┼─ Intel GPU BAR0 (16MB)
│                       │
├─ 0x00000060_20000000 ─┼─ Intel GPU BAR8/BAR10 (336MB)
│                       │
└─ ...                  └─ 其他设备 / 预留
```

**关键概念：**
- **PCIe 显存窗口**：通过 BAR 暴露给 CPU 的显存可见区域
- **实际显存容量**：可能大于 BAR 窗口（通过页表切换访问）
- **GART/GTT**：图形地址重映射表，用于管理 GPU 内存页

---

## 四、内核驱动与显存管理

### 4.1 当前驱动状态

```
┌────────────────────────────────────────┐
│  Intel i915                            │
│  ├── 活跃使用中                         │
│  ├── 管理 frame buffer (fb0)           │
│  ├── 使用 GuC/HuC 固件                 │
│  └── VT-d 保护激活                      │
└────────────────────────────────────────┘

┌────────────────────────────────────────┐
│  NVIDIA                                │
│  ├── nouveau (开源驱动) - 已加载但未使用 │
│  ├── vfio-pci - 当前绑定，用于直通       │
│  └── GPU 被隔离给虚拟机                 │
└────────────────────────────────────────┘
```

### 4.2 VFIO 直通配置

NVIDIA 显卡当前配置为 VFIO 直通：

```bash
# 查看绑定状态
cat /sys/bus/pci/devices/0000:01:00.0/uevent
# DRIVER=vfio-pci
```

这意味着：
1. 物理机无法直接使用该 GPU
2. 虚拟机可通过 VFIO 获得原生 GPU 性能
3. 显存完全由虚拟机内的驱动管理

---

## 五、显存使用观察方法

### 5.1 Intel 核显显存监控 - 实际测试

#### GPU 频率状态

```bash
$ cat /sys/class/drm/card2/gt_cur_freq_mhz
700

$ cat /sys/class/drm/card2/gt_max_freq_mhz
1650

$ cat /sys/class/drm/card2/gt_min_freq_mhz
300

$ cat /sys/class/drm/card2/gt_boost_freq_mhz
1650

$ cat /sys/class/drm/card2/gt_act_freq_mhz
0
```

**测试结果分析：**
- 当前运行频率：**700 MHz** (基础工作频率)
- 最大睿频：**1650 MHz**
- 最小频率：**300 MHz**
- 实际活动频率：**0 MHz** (当前 GPU 处于空闲状态)
- 睿频上限：**1650 MHz** (RP0)

**频率状态说明：**
| 参数 | 值 | 说明 |
|------|-----|------|
| RP0 | 1650 MHz | 最高睿频 |
| RP1 | 700 MHz | 基础频率 |
| RPn | 300 MHz | 最低节能频率 |

#### GPU 引擎信息

```bash
$ ls /sys/class/drm/card2/engine/
bcs0  rcs0  vcs0  vcs1  vecs0

$ for engine in /sys/class/drm/card2/engine/*; do echo "$(basename $engine): $(cat $engine/name)"; done
bcs0: bcs0
rcs0: rcs0
vcs0: vcs0
vcs1: vcs1
vecs0: vecs0
```

**引擎功能说明：**
| 引擎 | 全称 | 功能 |
|------|------|------|
| rcs0 | Render Command Streamer | 3D/计算渲染引擎 |
| bcs0 | Blitter Command Streamer | 2D/位图拷贝引擎 |
| vcs0 | Video Command Streamer 0 | 视频编解码引擎 #0 |
| vcs1 | Video Command Streamer 1 | 视频编解码引擎 #1 |
| vecs0 | Video Enhancement CS | 视频增强处理引擎 |

#### 功耗与节能状态

```bash
$ cat /sys/class/drm/card2/power/rc6_enable
1

$ cat /sys/class/drm/card2/power/rc6_residency_ms
3052946

$ cat /sys/class/drm/card2/power/runtime_status
unsupported
```

**测试结果分析：**
- **RC6 节能状态**：已启用 (`rc6_enable = 1`)
- **RC6 驻留时间**：3,052,946 ms (约 50 分钟处于低功耗状态)
- 运行时电源管理：不支持 (`unsupported`)

RC6 是 Intel GPU 的渲染待机状态，当 GPU 空闲时自动进入低功耗模式。

#### GT (Graphics Technology) 核心状态

```bash
$ cat /sys/class/drm/card2/gt/gt0/rps_cur_freq_mhz
700

$ cat /sys/class/drm/card2/gt/gt0/rps_act_freq_mhz
700

$ cat /sys/class/drm/card2/gt/gt0/rps_max_freq_mhz
1650

$ cat /sys/class/drm/card2/gt/gt0/rps_min_freq_mhz
300

$ cat /sys/class/drm/card2/gt/gt0/id
0

$ cat /sys/class/drm/card2/gt/gt0/throttle_reason_status
0
```

**RPS (Render Performance States) 说明：**
- 当前请求频率：700 MHz
- 实际运行频率：700 MHz
- 无节流限制 (`throttle_reason_status = 0`)

### 5.2 debugfs 深入分析 (root 权限)

**注意**：以下测试需要 root 权限访问 `/sys/kernel/debug/dri/0000:00:02.0/`

#### i915 GEM 对象统计

```bash
$ sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_gem_objects

8 shrinkable [0 free] objects, 34127872 bytes
system: total:0x0000001f41692000 bytes
stolen-system: total:0x0000000004000000 bytes
```

**测试结果分析：**

| 指标 | 值 | 说明 |
|------|-----|------|
| shrinkable objects | 8 | 可回收的 GEM 对象数量 |
| free objects | 0 | 空闲对象数量 |
| GEM 对象总大小 | 34,127,872 bytes (~32.5 MB) | 当前 GPU 分配的内存 |
| system memory | 0x1f41692000 bytes (~127 GB) | 系统内存总量 |
| stolen memory | 0x04000000 bytes (64 MB) | BIOS 预分配的显存 |

**关键概念解释：**

- **GEM (Graphics Execution Manager)**：Linux DRM 子系统的内存管理器
- **shrinkable objects**：可以被回收的内存对象（如缓存的纹理、缓冲区）
- **stolen memory**：BIOS 从系统内存中预分配给 GPU 的专用区域，用于帧缓冲等

#### i915 帧缓冲详情

```bash
$ sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_gem_framebuffer

fbcon size: 3840 x 2160, depth 24, 32 bpp, modifier 0x0, refcount 3,
obj 00000000bbd182e5: 32400KiB 01 00 UC (ggtt offset: 00c43000, size: 01fa4000, pages: 4K, normal)
(pinned x 1) (fb)
```

**帧缓冲分析：**

| 参数 | 值 | 说明 |
|------|-----|------|
| 分辨率 | 3840 x 2160 | 4K UHD 显示器 |
| 色深 | 24-bit | 真彩色 |
| BPP | 32 | 每像素 32 位 (ARGB) |
| 内存占用 | 32,400 KiB (~31.6 MB) | 帧缓冲区大小 |
| GGTT 偏移 | 0x00c43000 | 全局图形转换表偏移 |
| 状态 | pinned | 固定内存（不可交换） |

**计算验证：**
```
3840 x 2160 x 4 bytes = 33,177,600 bytes ≈ 32.4 MB
```

#### GPU 引擎详细状态

```bash
$ sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_engine_info
```

**关键状态摘要：**

| 引擎 | 状态 | 运行时间 | 强制唤醒 | 备注 |
|------|------|---------|---------|------|
| rcs0 (3D) | Idle | 0ms | 0 | 空闲 |
| bcs0 (2D) | Idle | 0ms | 0 | 空闲 |
| vcs0 (视频) | Idle | 0ms | 0 | 空闲 |
| vcs1 (视频) | Idle | 0ms | 0 | 空闲 |
| vecs0 (增强) | Idle | 0ms | 0 | 空闲 |

**引擎寄存器状态：**
- **RING_MODE**: `0x00000200 [idle]` - 所有引擎处于空闲模式
- **RING_HEAD/TAIL**: 0x00000000 - 命令队列空
- **Reset count**: 0 - 无重置发生
- **IRQ**: disabled - 中断已禁用（空闲状态）

#### GPU 频率详细信息

```bash
$ sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_frequency_info

PM MASK=0x80000000
RPSTAT1: 0x0001507a
RPNSWREQ: 700MHz
Lowest (RPN) frequency: 300MHz
Nominal (RP1) frequency: 700MHz
Max non-overclocked (RP0) frequency: 1650MHz
Current freq: 700 MHz
Actual freq: 700 MHz
Min freq: 300 MHz
Boost freq: 1650 MHz
Max freq: 1650 MHz
efficient (RPe) frequency: 700 MHz
```

**频率状态表：**

| 频率类型 | 值 | 描述 |
|---------|-----|------|
| RP0 | 1650 MHz | 最大非超频频率 |
| RP1 | 700 MHz | 标称频率 |
| RPN | 300 MHz | 最低频率 |
| RPe | 700 MHz | 高效频率点 |
| 当前 | 700 MHz | 当前运行频率 |
| Boost | 1650 MHz | 睿频上限 |

#### GPU 硬件能力

```bash
$ sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_capabilities
```

**关键硬件规格：**

| 特性 | 值 | 说明 |
|------|-----|------|
| Graphics Version | 12 | Gen12 (Alder Lake) |
| Platform | ALDERLAKE_S | Alder Lake 桌面平台 |
| Stepping | D0 | 步进版本 |
| EU 总数 | 32 | 执行单元数量 |
| Slice 数 | 1 | GPU 切片数量 |
| Subslice 数 | 2 | 每切片子切片数 |
| EU 每 Subslice | 16 | 子切片内 EU 数量 |
| PPGTT 大小 | 48-bit | 进程页表大小 |
| DMA Mask | 39-bit | DMA 寻址能力 |

**支持的功能标志：**

| 功能 | 状态 | 说明 |
|------|------|------|
| has_llc | yes | 支持 Last Level Cache |
| has_rc6 | yes | 支持 RC6 电源管理 |
| has_rps | yes | 支持渲染性能状态 |
| has_gt_uc | yes | 支持 GuC 微控制器 |
| has_3d_pipeline | yes | 支持 3D 渲染管线 |
| has_reset_engine | yes | 支持引擎级重置 |
| iommu | enabled | IOMMU 已启用 |

**驱动参数：**
```
i915.enable_guc=3          # GuC 完全启用
i915.reset=3               # 重置模式
i915.request_timeout_ms=20000  # 请求超时 20 秒
```

#### GPU 信息概览

```bash
$ sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_gpu_info | head -30

Kernel: 6.18.9-100.fc42.x86_64
Platform: ALDERLAKE_S
PCI ID: 0xa780
PCI Subsystem: 1043:8694
IOMMU enabled: 1
GT awake: no
Suspend count: 0
Reset count: 0
```

**系统信息：**
- **内核版本**: 6.18.9-100.fc42.x86_64 (Fedora 42)
- **运行时间**: 87,897 秒 (~24.4 小时)
- **捕获时间**: 10 毫秒前
- **GT 状态**: 休眠 (awake: no)
- **挂起次数**: 0
- **重置次数**: 0

### 5.3 通用显存信息获取 - 实际测试

#### PCI 资源配置详情

```bash
$ lspci -v -s 00:02.0
00:02.0 VGA compatible controller: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] (rev 04) (prog-if 00 [VGA controller])
	DeviceName: Onboard - Video
	Subsystem: ASUSTeK Computer Inc. Device 8694
	Flags: bus master, fast devsel, latency 0, IRQ 197, IOMMU group 0
	Memory at 6012000000 (64-bit, non-prefetchable) [size=16M]
	Memory at 4000000000 (64-bit, prefetchable) [size=256M]
	I/O ports at 5000 [size=64]
	Expansion ROM at 000c0000 [virtual] [disabled] [size=128K]
	Kernel driver in use: i915
	Kernel modules: i915, xe
```

#### BAR 空间详情

```bash
$ cat /sys/bus/pci/devices/0000:00:02.0/resource
0x0000006012000000 0x0000006012ffffff 0x0000000000140204
0x0000000000000000 0x0000000000000000 0x0000000000000000
0x0000004000000000 0x000000400fffffff 0x000000000014220c
0x0000000000000000 0x0000000000000000 0x0000000000000000
0x0000000000005000 0x000000000000503f 0x0000000000040101
...
```

**BAR 资源文件格式说明：**
每行包含三个十六进制值：`起始地址 结束地址 标志`

| BAR | 起始地址 | 结束地址 | 大小 | 类型 |
|-----|---------|---------|------|------|
| BAR0 | 0x6012000000 | 0x6012ffffff | 16MB | 64-bit non-prefetchable |
| BAR2 | 0x4000000000 | 0x400fffffff | 256MB | 64-bit prefetchable |
| BAR4 | 0x5000 | 0x503f | 64B | I/O ports |

#### DMA 能力

```bash
$ cat /sys/class/drm/card2/device/consistent_dma_mask_bits
39

$ cat /sys/class/drm/card2/device/dma_mask_bits
39
```

**测试结果分析：**
- DMA 掩码：**39-bit**
- 可寻址空间：2^39 = 512GB
- 这是 Intel 核显在 64 位系统上的标准 DMA 能力

#### 显示连接状态

```bash
$ for conn in /sys/class/drm/card2/card2-*; do
    echo "$(basename $conn): status=$(cat $conn/status), enabled=$(cat $conn/enabled)"
done

card2-DP-4: status=disconnected, enabled=disabled
card2-HDMI-A-2: status=disconnected, enabled=disabled
card2-HDMI-A-3: status=connected, enabled=enabled
```

**测试结果：**
- HDMI-A-3 端口已连接并启用 (当前正在使用)
- DP-4 和 HDMI-A-2 未连接

#### 驱动模块信息

```bash
$ lsmod | grep -E "i915|drm|ttm"
i915                 5353472  2
drm_buddy              32768  2 xe,i915
ttm                   135168  4 drm_ttm_helper,xe,i915,nouveau
drm_display_helper    331776  3 xe,i915,nouveau
```

**关键模块说明：**
| 模块 | 大小 | 用途 |
|------|------|------|
| i915 | 5.3MB | Intel GPU 主驱动 |
| ttm | 135KB | 内存管理 (Translation Table Maps) |
| drm_buddy | 32KB | DRM 内存分配器 |
| drm_display_helper | 331KB | 显示辅助功能 |

### 5.4 显存监控限制说明

在实际测试中发现以下限制：

1. **i915_gem_objects 需要 root 权限**：
   ```bash
   $ cat /sys/kernel/debug/dri/*/i915_gem_objects
   需要 root 权限或 debugfs 未挂载
   ```

2. **debugfs DRI 目录访问受限**：
   - 即使 debugfs 已挂载，DRI 子目录可能需要 CAP_SYS_ADMIN 权限
   - 某些内核配置可能禁用了 i915 的 debugfs 接口

3. **替代监控方法**：
   - 使用 `intel_gpu_top` 工具 (需安装 intel-gpu-tools)
   - 使用 `perf` 或 `ftrace` 跟踪 GPU 事件
   - 通过 `sysfs` 接口获取频率、功耗等基本信息

### 5.5 显存使用估算（基于实测数据）

根据实际测试数据，当前系统显存使用情况如下：

#### 当前显存分配实测

```
总 GEM 对象大小: 34,127,872 bytes (~32.5 MB)
├── 帧缓冲区 (fbcon): 32,400 KiB (~31.6 MB) - 4K@32bpp
├── 其他 GEM 对象: ~1 MB
└── 驱动开销: 少量

Stolen Memory (BIOS 预分配): 64 MB
```

#### 帧缓冲区计算验证

**理论计算：**
```
分辨率: 3840 x 2160 (4K UHD)
色深: 32 bpp (4 bytes)
单帧大小: 3840 x 2160 x 4 = 33,177,600 bytes ≈ 31.6 MB
双缓冲: 31.6 MB x 2 = 63.3 MB (理论最大值)
```

**实际测量：**
```
实际帧缓冲: 32,400 KiB ≈ 31.6 MB
状态: pinned (固定内存)
说明: 当前使用单缓冲或压缩/优化存储
```

#### 完整显存使用模型

```
Intel UHD 770 显存使用:
├─ Stolen Memory (BIOS 预分配): 64 MB
│  └─ 用于帧缓冲、固件、启动画面等
├─ 运行时动态分配 (GEM Objects): ~32.5 MB
│  └─ 帧缓冲区: ~31.6 MB (4K 显示)
│  └─ 纹理/缓冲区: ~1 MB
└─ 潜在分配 (按需): 可达系统内存的 50%

总系统内存: ~127 GB
GPU 可寻址空间: 512 GB (39-bit DMA)
```

#### 与理论值的对比

| 项目 | 理论估算 | 实测值 | 差异原因 |
|------|---------|--------|---------|
| 4K 帧缓冲 | 63 MB (双缓冲) | 31.6 MB | 单缓冲或压缩 |
| GEM 对象 | 视应用而定 | 32.5 MB | 桌面空闲状态 |
| Stolen Memory | BIOS 设置 | 64 MB | 典型默认值 |

**实际观察建议：**
- 使用 `sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_gem_objects` 查看 GEM 对象
- 使用 `sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_gem_framebuffer` 查看帧缓冲详情
- 监控 `/sys/class/drm/card2/gt/gt0/rc6_residency_ms` 了解 GPU 活跃程度
- 使用 `intel_gpu_top` 查看实时显存使用 (需安装 intel-gpu-tools)

---

## 六、分析与结论

### 6.1 显存配置总结

| 显卡 | 架构 | 显存类型 | 显存大小 | 当前用途 |
|------|------|---------|---------|---------|
| Intel UHD 770 | UMA | 共享系统内存 | 动态分配 (~32MB-1GB+) | 主机显示输出 (4K) |
| NVIDIA GTX 1060 | 独立 | GDDR5 | 3GB 专用 | 虚拟机直通 |

### 6.2 关键发现

1. **双显卡分工明确**：核显负责主机显示，独显直通给虚拟机
2. **Intel 核显无专用显存**：依赖系统内存，但具有零拷贝优势
3. **NVIDIA 显存被隔离**：通过 VFIO 直通，物理机无法访问
4. **BAR 空间映射**：64-bit prefetchable BAR 是 GPU 显存的主要访问窗口
5. **当前 GPU 频率状态**：
   - 基础频率 700 MHz，最大睿频 1650 MHz
   - 当前处于低负载状态 (700 MHz)
   - RC6 节能模式已启用，累计节能时间 >50 分钟
6. **多引擎架构**：包含 3D 渲染、2D 位图、双视频编解码、视频增强共 5 个引擎
7. **DMA 能力**：39-bit 寻址空间 (512GB)，满足大内存系统需求
8. **实测显存使用**：当前 GEM 对象约 32.5MB，主要为 4K 帧缓冲 (31.6MB)
9. **硬件规格**：Gen12 架构，32 EU，1 Slice，支持 GuC/HuC 固件

### 6.3 优化建议

1. **BIOS 设置**：可调整 DVMT Pre-Allocated 大小优化核显性能
2. **VFIO 配置**：如需在物理机使用独显，需解绑 vfio-pci 并加载 nvidia 驱动
3. **内存分配**：对于大显存需求的应用，考虑在虚拟机内使用直通 GPU

---

## 七、测试验证总结

### 7.1 已完成的测试项

| 测试项 | 命令/方法 | 结果 | 备注 |
|--------|----------|------|------|
| GPU 频率查询 | `cat /sys/class/drm/card2/gt_*_freq_mhz` | 成功 | 当前 700MHz，最大 1650MHz |
| GPU 引擎枚举 | `ls /sys/class/drm/card2/engine/` | 成功 | 5 个引擎 (rcs0, bcs0, vcs0, vcs1, vecs0) |
| 功耗状态查询 | `cat power/rc6_*` | 成功 | RC6 已启用，节能 >50 分钟 |
| GT 核心状态 | `cat gt/gt0/rps_*` | 成功 | RPS 频率管理正常 |
| PCI 资源配置 | `lspci -v -s 00:02.0` | 成功 | 确认 BAR 空间分配 |
| BAR 空间解析 | `cat resource` | 成功 | 3 个有效 BAR |
| DMA 能力查询 | `cat dma_mask_bits` | 成功 | 39-bit 寻址 (512GB) |
| 显示连接状态 | `cat card2-*/status` | 成功 | HDMI-A-3 已连接 |
| 驱动模块信息 | `lsmod \| grep i915` | 成功 | i915 驱动已加载 |
| i915_gem_objects | `cat /sys/kernel/debug/dri/*/i915_gem_objects` | 成功 | 8 个对象，约 32MB |
| i915_framebuffer | `cat /sys/kernel/debug/dri/*/i915_gem_framebuffer` | 成功 | 4K 帧缓冲，约 32MB |
| i915_engine_info | `cat /sys/kernel/debug/dri/*/i915_engine_info` | 成功 | 5 个引擎详细状态 |
| i915_frequency_info | `cat /sys/kernel/debug/dri/*/i915_frequency_info` | 成功 | 完整频率状态 |
| i915_capabilities | `cat /sys/kernel/debug/dri/*/i915_capabilities` | 成功 | GPU 硬件能力 |

### 7.2 测试环境信息

- **测试时间**：2026-03-03
- **内核版本**：6.18.9-100.fc42.x86_64 (Fedora 42)
- **DRM 版本**：1.1.0 20060810
- **i915 驱动版本**：1.6.0
- **GPU 设备**：Intel UHD Graphics 770 (0x8086:0xA780)

### 7.3 监控工具建议

如需更详细的显存监控，建议安装以下工具：

```bash
# intel-gpu-tools 提供详细的 GPU 监控
sudo apt install intel-gpu-tools  # Debian/Ubuntu
sudo dnf install intel-gpu-tools  # Fedora

# 使用 intel_gpu_top 查看实时显存使用
sudo intel_gpu_top

# 使用 radeontop 监控 AMD GPU
sudo apt install radeontop

# 使用 nvidia-smi 监控 NVIDIA GPU (需要专有驱动)
nvidia-smi
```


---

## 八、GPU 指令集 (ISA) 分析指南

### 8.1 指令集获取方式对比

| GPU | 文档公开程度 | 获取方式 | 可分析性 |
|-----|-------------|---------|---------|
| **Intel UHD 770** | 低 | 内核驱动源码 + 固件逆向 | 困难 |
| **NVIDIA GTX 1060** | 中等 | PTX 公开文档 + SASS 反编译 | 中等 |

### 8.2 Intel GPU 指令集 (Gen12/Xe)

**现状**: Intel **没有公开** 像 CPU 那样的完整 GPU ISA 文档 PDF。

#### 可用的分析途径

**1. 开源驱动代码分析**

```bash
# 查看 GPU 相关定义
ls ~/data/kernel/linux/drivers/gpu/drm/i915/gt/

# 关键文件
- gen8_engine_cs.c      # 命令流提交
- gen8_renderstate.c    # 渲染状态
- intel_lrc.c           # 逻辑环上下文
```

**2. 固件分析**

Intel GPU 微码固件位于 `/lib/firmware/i915/`:

```bash
$ ls /lib/firmware/i915/ | grep -E "guc|huc|dmc"

adls_dmc_ver2_01.bin.xz   # Display Microcode
adlp_guc_70.bin.xz        # Graphics Microcode (GuC)
tgl_huc.bin.xz            # HEVC/H.265 Microcode (HuC)
```

**注意**: 这些固件是 **二进制格式**，需要逆向工程才能分析指令集。

**3. Intel Graphics Assembler (IGA)**

Intel 提供了汇编器工具 `iga64`，但主要用于内部开发，不公开完整 ISA 文档。

### 8.3 NVIDIA GPU 指令集 (GP106/Pascal)

**现状**: NVIDIA **公开** PTX ISA，但 **不公开** 机器码 SASS ISA。

#### PTX ISA 文档（公开）

PTX 是 NVIDIA 的并行线程执行中间语言：

```bash
# 下载地址
https://docs.nvidia.com/cuda/parallel-thread-execution/

# 或 CUDA Toolkit 中的 PDF
/usr/local/cuda/doc/ptx_isa_X.X.pdf
```

**PTX 特点**:
- 是虚拟指令集，不是真实机器码
- 安装时 JIT 编译为实际 GPU 机器码 (SASS)
- 所有 CUDA 程序最终都转换为 PTX

#### 获取真实机器码 SASS

**方法 1**: 使用 `nvcc` 反编译

```bash
# 编译时保留 SASS 汇编
nvcc -cubin -arch=sm_61 kernel.cu        # 生成 cubin
cuobjdump -sass kernel.cubin             # 反汇编为 SASS

# 或使用 ptxas 查看
ptxas -v -arch=sm_61 kernel.ptx
```

**方法 2**: 使用 `nvdisasm`

```bash
# 从二进制提取 SASS
nvdisasm kernel.cubin > kernel.sass
```

#### envytools 项目（开源逆向）

```bash
# 克隆 envytools（包含 nouveau 驱动文档）
git clone https://github.com/envytools/envytools.git

# 包含的文档
- docs/hw/gpu.rst          # GPU 架构概述
- docs/hw/pascal/index.rst # Pascal 架构详情
```

**envytools 包含的信息**:
- 寄存器定义
- 部分指令编码
- GPU 内存映射

### 8.4 当前系统分析建议

#### Intel UHD 770 (Gen12)

```bash
# 1. 查看 GPU 使用的固件版本
sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_dmc_info

# 2. 分析内核提交的命令流
sudo cat /sys/kernel/debug/dri/0000:00:02.0/i915_engine_info

# 3. 阅读 i915 驱动源码了解指令提交机制
# 路径: drivers/gpu/drm/i915/gt/gen8_engine_cs.c
```

#### NVIDIA GTX 1060 (GP106)

**注意**: 当前该 GPU 被 VFIO 直通，物理机无法直接访问。

```bash
# 1. 查看 PTX ISA 文档（需安装 CUDA）
cat /usr/local/cuda/doc/ptx_isa_*.pdf 2>/dev/null || echo "CUDA not installed"

# 2. 查看 nouveau 文档
# https://envytools.readthedocs.io/

# 3. 查看当前绑定状态
lspci -v -s 01:00.0 | grep "Kernel driver"
# 输出: Kernel driver in use: vfio-pci
```

### 8.5 指令集分析结论

| 分析维度 | Intel Gen12 | NVIDIA Pascal |
|---------|-------------|---------------|
| **公开文档** | 无完整 ISA | PTX ISA 完整公开 |
| **机器码文档** | 无 | SASS 不公开 |
| **分析工具** | iga64 (内部) | cuobjdump, nvdisasm |
| **开源资料** | i915 驱动源码 | envytools |
| **固件格式** | 二进制闭源 | 二进制闭源 |
| **推荐方法** | 驱动源码分析 | PTX + SASS 反编译 |

---

## intel_gpu_top 来检查 GPU 的使用量
<!-- 3e8d4349-1e1c-4fda-9afa-344fa9d0ee3a -->
sudo intel_gpu_top


## 九、参考资源

- [Intel Graphics Driver - i915](https://www.kernel.org/doc/html/latest/gpu/i915.html)
- [VFIO - Virtual Function I/O](https://www.kernel.org/doc/Documentation/vfio.txt)
- [PCIe BAR 空间详解](https://wiki.osdev.org/PCI)
- [Intel GPU 频率管理](https://www.kernel.org/doc/html/latest/gpu/i915.html#frequency-management)
- [RC6 电源管理](https://01.org/linuxgraphics/documentation/power-management)
- [NVIDIA PTX ISA 文档](https://docs.nvidia.com/cuda/parallel-thread-execution/)
- [envytools - NVIDIA 开源文档](https://envytools.readthedocs.io/)
