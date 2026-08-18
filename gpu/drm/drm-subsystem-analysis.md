# DRM 子系统（Direct Rendering Manager）完整分析
<!-- aa10ba5d-5874-41a4-bd6f-84c5dab95b17 -->

(感觉基本是对的，就是缺少对于其中的关键子系统的理解
例如，GEM KMS 和 TTM 内存都是什么意思
)

## 🎯 DRM 是什么？

### 一句话定义
> **DRM（Direct Rendering Manager）是 Linux 内核的图形子系统，负责统一管理 GPU、显示输出、图形渲染和内存分配。**

### 历史背景
```
1999年: DRM 诞生（最初为 Direct Rendering Infrastructure）
   ↓
2008年: KMS（Kernel Mode Setting）加入
   ↓
2010年: GEM（Graphics Execution Manager）引入
   ↓
2012年: TTM 内存管理成熟
   ↓
现在: 现代 DRM 支持 3D、视频、显示、计算
```

### 与 fbdev 的关系
```
早期: fbdev（Framebuffer）是唯一的显示接口
   ↓
中期: DRM 和 fbdev 并存
   ↓
现在: DRM 包含 fbdev 兼容层，fbdev 逐渐淘汰
```

---

## 🏗️ DRM 架构概览

```
┌─────────────────────────────────────────────────────────────┐
│                      用户态                                  │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  OpenGL  │ │  Vulkan  │ │  EGL/GLES│ │   GBM    │       │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘       │
├───────┼────────────┼────────────┼────────────┼─────────────┤
│       ↓            ↓            ↓            ↓             │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐       │
│  │  Mesa    │ │  Mesa    │ │  Mesa    │ │  libdrm  │       │
│  │  iris    │ │  RADV    │ │  lima    │ │          │       │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘       │
├───────┼────────────┼────────────┼────────────┼─────────────┤
│       ↓            ↓            ↓            ↓             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                    libdrm 接口层                     │   │
│  │         (drmIoctl, drmCommandWriteRead, ...)        │   │
│  └────────────────────────┬────────────────────────────┘   │
├───────────────────────────┼────────────────────────────────┤
│                      内核态                                │
│  ┌────────────────────────┼────────────────────────────┐   │
│  │                    DRM 核心层                        │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │   │
│  │  │   KMS    │ │   GEM    │ │   TTM    │ │  Prime  │ │   │
│  │  │ (显示)   │ │ (内存)   │ │ (内存)   │ │(DMA-BUF)│ │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────┘ │   │
│  └────────────────────────┬────────────────────────────┘   │
│                           ↓                                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                  GPU 设备驱动                        │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌─────────┐ │   │
│  │  │  i915    │ │  amdgpu  │ │  nouveau │ │  virtio │ │   │
│  │  │ (Intel)  │ │  (AMD)   │ │ (NVIDIA) │ │  -gpu   │ │   │
│  │  └──────────┘ └──────────┘ └──────────┘ └─────────┘ │   │
│  └────────────────────────┬────────────────────────────┘   │
│                           ↓                                 │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                   GPU 硬件                           │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

---

## 📋 DRM 主要功能模块

### 1. KMS（Kernel Mode Setting）- 显示管理

**功能**: 管理显示输出、分辨率、刷新率

**核心组件**:
```
KMS
├── CRTC (Controller)
│   └── 显示控制器，对应一个显示输出流
│
├── Plane
│   └── 图层，可以是 Primary（主图层）、Cursor（光标）、Overlay（叠加）
│
├── Encoder
│   └── 编码器，将信号转换为 HDMI/DP/VGA 等格式
│
├── Connector
│   └── 物理接口，对应实际的显示器接口（HDMI-1、DP-1 等）
│
└── FB (Framebuffer)
    └── 显存缓冲区，存储像素数据
```

**关键操作**:
- **Mode Setting**: 设置分辨率、刷新率
- **Page Flip**: 切换显示缓冲区（VSync）
- **Atomic Commit**: 原子提交，同时更新多个显示参数

**代码位置**: `drivers/gpu/drm/drm_crtc.c`, `drm_plane.c`, `drm_encoder.c`, `drm_connector.c`

---

### 2. GEM（Graphics Execution Manager）- 内存管理

**功能**: 管理 GPU 内存对象（Buffer Object）

**核心概念**:
```
GEM Object (Buffer Object)
├── 数据结构: struct drm_gem_object
├── 包含: 内存大小、引用计数、handle
└── 操作:
    ├── create: 创建内存对象
    ├── open: 获取引用
    ├── close: 释放引用
    └── mmap: 映射到用户态
```

**关键操作**:
- **创建 BO**: `drm_gem_create_ioctl()`
- **映射 BO**: `drm_gem_mmap()`
- **导出 BO**: `drm_gem_prime_export()` (DMA-BUF)

**代码位置**: `drivers/gpu/drm/drm_gem.c`

---

### 3. TTM（Translation Table Maps）- 显存管理

**功能**: 管理 GPU 显存和系统内存之间的迁移

**核心概念**:
```
TTM Pool
├── VRAM (Video RAM)
│   └── GPU 专用显存
│
├── System Memory
│   └── 系统内存（可作为显存使用）
│
└── Migration
    └── 在 VRAM 和系统内存之间移动数据
```

**适用场景**:
- 显存不足时，将数据换出到系统内存
- 支持大内存场景（超过物理显存）

**代码位置**: `drivers/gpu/drm/ttm/`

**对比 GEM vs TTM**:
| 特性 | GEM | TTM |
|------|-----|-----|
| **设计目标** | Buffer Object 管理 | 显存迁移 |
| **适用驱动** | i915, nouveau | amdgpu, radeon |
| **功能** | 简单内存分配 | 复杂的内存池管理 |
| **换出支持** | 有限 | 完整 |

---

### 4. Prime - 跨设备内存共享

**功能**: 在不同 GPU 之间共享内存（DMA-BUF）

**使用场景**:
```
集成 GPU (iGPU) + 独立 GPU (dGPU)
├── 应用在 iGPU 渲染
├── 通过 Prime 导出缓冲区
├── dGPU 导入缓冲区
└── dGPU 显示到屏幕
```

**关键操作**:
- **Export**: `drm_gem_prime_export()` - 导出为 DMA-BUF fd
- **Import**: `drm_gem_prime_import()` - 从 DMA-BUF fd 导入

**代码位置**: `drivers/gpu/drm/drm_prime.c`

---

### 5. Render 节点 - 安全渲染

**功能**: 支持无显示权限的 GPU 计算

**设备节点**:
```
/dev/dri/card0      - 主节点（可控制显示）
/dev/dri/renderD128 - 渲染节点（仅计算/渲染）
```

**作用**:
- 容器/沙箱环境可以访问 GPU 进行计算
- 不需要显示权限（防止篡改屏幕）

---

### 6. Syncobj - GPU 同步

**功能**: 跨引擎、跨设备的 GPU 同步机制

**类型**:
- **Binary Syncobj**: 二值信号量（有信号/无信号）
- **Timeline Syncobj**: 时间线信号量（支持多阶段）

**使用场景**:
- 等待渲染完成再提交显示
- 跨 GPU 同步
- 与 Vulkan fence 对应

**代码位置**: `drivers/gpu/drm/drm_syncobj.c`

---

### 7. Scheduler - GPU 任务调度

**功能**: 调度 GPU 渲染/计算任务

**设计**:
```
DRM Scheduler
├── 队列管理
│   └── 按优先级/时间片排队
│
├── 依赖处理
│   └── 处理任务之间的依赖关系
│
└── 硬件提交
    └── 将任务提交到 GPU 硬件队列
```

**代码位置**: `drivers/gpu/drm/scheduler/`

---

## 🔌 用户态接口

### 1. DRM 设备节点

```bash
# 查看 DRM 设备
ls -la /dev/dri/
card0       # 主节点
renderD128  # 渲染节点
```

### 2. 核心 ioctl

| ioctl | 功能 | 说明 |
|-------|------|------|
| `DRM_IOCTL_VERSION` | 获取驱动版本 | 查询驱动信息 |
| `DRM_IOCTL_GET_CAP` | 获取能力 | 查询支持的特性 |
| `DRM_IOCTL_SET_MASTER` | 设置 Master | 获取显示控制权 |
| `DRM_IOCTL_DROP_MASTER` | 释放 Master | 释放显示控制权 |
| `DRM_IOCTL_MODE_GETRESOURCES` | 获取显示资源 | 查询 CRTCs/Encoders/Connectors |
| `DRM_IOCTL_MODE_GETCONNECTOR` | 获取连接器信息 | 查询显示器信息 |
| `DRM_IOCTL_MODE_GETMODE` | 获取显示模式 | 查询支持的分辨率 |
| `DRM_IOCTL_MODE_SETCRTC` | 设置 CRTC | 配置显示输出 |
| `DRM_IOCTL_MODE_PAGE_FLIP` | 页面翻转 | 切换显示缓冲区 |
| `DRM_IOCTL_GEM_OPEN` | 打开 GEM 对象 | 获取 GEM 引用 |
| `DRM_IOCTL_GEM_CLOSE` | 关闭 GEM 对象 | 释放 GEM 引用 |
| `DRM_IOCTL_PRIME_HANDLE_TO_FD` | GEM 转 DMA-BUF | 导出内存 |
| `DRM_IOCTL_PRIME_FD_TO_HANDLE` | DMA-BUF 转 GEM | 导入内存 |

### 3. libdrm 库

**功能**: 用户态封装，简化 DRM 操作

```c
// 打开 DRM 设备
drm_fd = open("/dev/dri/card0", O_RDWR);

// 获取资源
drmModeResPtr res = drmModeGetResources(drm_fd);

// 获取连接器信息
drmModeConnectorPtr conn = drmModeGetConnector(drm_fd, connector_id);

// 设置 CRTC
drmModeSetCrtc(drm_fd, crtc_id, fb_id, x, y, &connector_id, 1, mode);

// 页面翻转
drmModePageFlip(drm_fd, crtc_id, fb_id, DRM_MODE_PAGE_FLIP_EVENT, data);
```

---

## 📊 DRM 功能矩阵

| 功能 | 子系统 | 主要用途 | 现代支持 |
|------|--------|----------|----------|
| **显示管理** | KMS | 分辨率、刷新率、多显示器 | ✅ 完整 |
| **内存管理** | GEM/TTM | GPU 内存分配 | ✅ 完整 |
| **跨设备共享** | Prime | GPU 间内存共享 | ✅ 完整 |
| **渲染计算** | Render | 3D/计算任务 | ✅ 完整 |
| **同步机制** | Syncobj | GPU 同步 | ✅ 完整 |
| **任务调度** | Scheduler | GPU 调度 | ✅ 完整 |
| **视频编解码** | Video | 硬件编解码 | 🔄 发展中 |
| **显示压缩** | DSC | Display Stream Compression | 🔄 发展中 |

---

## 🔄 DRM 工作流程

### 初始化流程
```
1. 用户态打开 /dev/dri/card0
   ↓
2. 查询驱动能力 (GET_CAP)
   ↓
3. 获取显示资源 (MODE_GETRESOURCES)
   ↓
4. 枚举连接器 (MODE_GETCONNECTOR)
   ↓
5. 创建 Framebuffer
   ↓
6. 设置 CRTC (MODE_SETCRTC)
   ↓
7. 显示就绪
```

### 渲染流程
```
1. 创建 GEM Object (分配 GPU 内存)
   ↓
2. 映射 GEM (mmap) 或 GPU 直接写入
   ↓
3. 提交渲染命令
   ↓
4. 等待渲染完成 (fence/syncobj)
   ↓
5. 将结果作为 Framebuffer 显示
```

### 多缓冲（Page Flip）流程
```
┌─────────┐    ┌─────────┐    ┌─────────┐
│ Buffer 0│ →  │ Buffer 1│ →  │ Buffer 0│
│ (显示)  │    │ (渲染)  │    │ (渲染)  │
└─────────┘    └─────────┘    └─────────┘
      ↑              ↓              ↑
      └──────────────┴──────────────┘
              Page Flip
```

---

## 💡 DRM 设计优势

### 1. **统一架构**
- 所有 GPU 驱动使用统一接口
- 用户态代码可跨 GPU 移植

### 2. **安全性**
- 内核管理 GPU 内存
- 用户态无法直接访问硬件寄存器

### 3. **灵活性**
- 支持多种 GPU（集成/独立）
- 支持多显示器、热插拔

### 4. **性能**
- 直接渲染（Direct Rendering）
- 零拷贝内存共享（Prime）

---

## 📚 相关代码路径

```
Linux Kernel
├── drivers/gpu/drm/
│   ├── drm_crtc.c          # KMS 核心
│   ├── drm_plane.c         # Plane 管理
│   ├── drm_encoder.c       # Encoder 管理
│   ├── drm_connector.c     # Connector 管理
│   ├── drm_framebuffer.c   # Framebuffer 管理
│   ├── drm_gem.c           # GEM 核心
│   ├── drm_prime.c         # Prime/DMA-BUF
│   ├── drm_syncobj.c       # 同步对象
│   ├── drm_ioctl.c         # ioctl 处理
│   ├── ttm/                # TTM 内存管理
│   ├── scheduler/          # GPU 调度器
│   ├── i915/               # Intel 驱动
│   ├── amdgpu/             # AMD 驱动
│   ├── nouveau/            # NVIDIA 开源驱动
│   ├── virtio/             # VirtIO GPU
│   └── ...
│
└── include/uapi/drm/
    ├── drm.h               # DRM UAPI 头文件
    ├── drm_mode.h          # KMS 头文件
    └── ...

Mesa (用户态)
├── src/gallium/
│   ├── drivers/iris/       # Intel Gallium 驱动
│   ├── drivers/radeonsi/   # AMD Gallium 驱动
│   └── ...
└── src/intel/              # Intel 特定代码

libdrm (用户态库)
├── xf86drm.c               # DRM 操作封装
├── xf86drmMode.c           # KMS 操作封装
└── ...
```
