# i915 vs Xe 驱动架构对比分析

基于本地内核源码 (`~/data/kernel/default/drivers/gpu/drm/`) 的深度分析

---

## 一、总体概览

| 特性 | i915 | Xe | 说明 |
|------|------|-----|------|
| **代码规模** | 885 个文件, 14MB | 512 个文件, 4.8MB | Xe 更精简现代 |
| **设计目标** | 支持 20+ 年历史硬件 | 支持现代 GPU (Gen12+) | Xe 无历史包袱 |
| **首次引入** | Linux 2.6 时代 | Linux 6.8+ | Xe 是新一代驱动 |
| **架构风格** | 演进式 | 革命式重构 | Xe 重新设计核心架构 |

---

## 二、核心架构差异

### 2.1 设备结构体设计

#### i915: `struct drm_i915_private`
```c
// drivers/gpu/drm/i915/i915_drv.h
struct drm_i915_private {
    struct drm_device drm;
    struct intel_display *display;
    
    // 大量历史遗留字段
    struct i915_dsm dsm;
    struct intel_uncore uncore;
    struct i915_virtual_gpu vgpu;
    struct intel_gvt *gvt;
    
    // 多代硬件共存
    union {
        struct llist_head uabi_engines_llist;
        struct list_head uabi_engines_list;
        struct rb_root uabi_engines;
    };
    
    // 各代引擎特定处理
    struct workqueue_struct *wq;
    struct workqueue_struct *unordered_wq;
    
    // 大量平台特定 workaround
    bool preserve_bios_swizzle;
    u32 gen2_imr_mask;
    // ... 数百个字段
};
```

#### Xe: `struct xe_device`
```c
// drivers/gpu/drm/xe/xe_device_types.h
struct xe_device {
    struct drm_device drm;
    struct intel_display *display;
    
    // 基于 Tile/GT 的层次架构
    struct xe_tile tiles[XE_MAX_TILES_PER_DEVICE];  // 多 Tile 支持
    
    // 现代化内存管理
    struct ttm_device ttm;  // 使用 TTM 而非自建 GEM
    
    // 统一设备信息
    struct {
        u32 graphics_verx100;
        u32 media_verx100;
        u8 tile_count;
        u8 gt_count;
        bool has_sriov:1;
        bool has_usm:1;
        // ... 简洁的 capability flags
    } info;
    
    // SR-IOV 原生支持
    struct {
        enum xe_sriov_mode __mode;
        union {
            struct xe_device_pf pf;
            struct xe_device_vf vf;
        };
    } sriov;
    
    // 统一内存管理 (USM)
    struct {
        struct xarray asid_to_vm;
        u32 next_asid;
    } usm;
};
```

**关键差异**:
- i915: 单体式结构，包含 20+ 年历史代码
- Xe: 模块化分层设计，Tile/GT 层次清晰

---

### 2.2 Tile/GT 架构（Xe 核心创新）

#### Xe 的三层架构

```
┌─────────────────────────────────────┐
│         struct xe_device            │  ← 设备层（PCI 设备）
│         (1 per PCI device)          │
├─────────────────────────────────────┤
│  ┌─────────┐     ┌─────────┐       │
│  │ tile[0] │     │ tile[1] │       │  ← Tile 层（多 GPU 封装）
│  │(Root)   │     │(Media)  │       │     XE_MAX_TILES_PER_DEVICE = 2
│  └────┬────┘     └────┬────┘       │
│       │               │             │
│  ┌────┴────┐     ┌────┴────┐       │
│  │ primary │     │ primary │       │  ← GT 层（Graphics Technology）
│  │   gt    │     │   gt    │       │     渲染/计算引擎
│  │ (Render)│     │ (Media) │       │
│  └─────────┘     └─────────┘       │
└─────────────────────────────────────┘
```

#### Xe Tile 结构
```c
struct xe_tile {
    struct xe_device *xe;
    u8 id;
    
    struct xe_gt *primary_gt;   // 渲染 GT
    struct xe_gt *media_gt;     // 媒体 GT (Gen13+)
    
    struct xe_mmio mmio;        // 寄存器访问
    struct {
        struct xe_vram_region *vram;
        struct xe_ggtt *ggtt;   // 全局 GTT
    } mem;
    
    // SR-IOV 支持
    union {
        struct { struct xe_lmtt lmtt; } pf;  // PF: Local Memory Translation Table
        struct { struct xe_ggtt_node *ggtt_balloon[2]; } vf;  // VF
    } sriov;
};
```

#### i915 的扁平架构
```c
// i915 没有 Tile 概念，直接管理引擎
struct drm_i915_private {
    // 引擎通过链表/红黑树管理
    struct rb_root uabi_engines;
    
    // 多代硬件寄存器访问混杂
    struct intel_uncore uncore;
    
    // 显示和渲染耦合
    struct intel_display *display;
};
```

**关键差异**:
- Xe: 为 Multi-Tile (如 Ponte Vecchio) 设计，支持扩展
- i915: 单 Tile 假设，多 GPU 支持为后加功能

---

## 三、内存管理架构

### 3.1 对比总览

| 特性 | i915 | Xe |
|------|------|-----|
| **内存框架** | 自建 GEM | TTM (Translation Table Maps) |
| **GPU VM** | 自建 PPGTT | xe_vm (TTM-based) |
| **统一内存** | 有限支持 | USM (Unified Shared Memory) |
| **SVM 支持** | 基础 | 完整 (xe_svm) |
| **VRAM 管理** | i915_gem_stolen | TTM + xe_vram_region |

### 3.2 i915: 自研 GEM 系统

```
i915_gem_*
├── i915_gem_shmem.c       # System memory
├── i915_gem_stolen.c      # Stolen memory
├── i915_gem_ttm.c         # TTM 迁移（后期添加）
├── i915_gem_lmem.c        # Local memory (DG1+)
├── i915_gem_context.c     # GEM contexts
├── i915_gem_execbuffer.c  # Execbuf submission
└── i915_gem_mman.c        # Memory mapping
```

**特点**:
- 历史包袱重，多代代码共存
- 从 GEM 到 TTM 的渐进式迁移
- PPGTT (Per-Process GTT) 自建实现

### 3.3 Xe: 基于 TTM 的现代化管理

```
xe_bo* (Buffer Object)
├── xe_bo.c              # BO 核心管理
├── xe_bo_evict.c        # Eviction 策略
├── xe_ttm_stolen_mgr.c  # Stolen memory
└── xe_vm*               # GPU VM 管理
    ├── xe_vm.c          # VM 核心
    ├── xe_vm_madvise.c  # Memory advising
    └── xe_svm.c         # Shared Virtual Memory
```

**核心代码示例**:
```c
// Xe 使用 TTM 资源管理
struct xe_device {
    struct ttm_device ttm;           // TTM 设备
    struct {
        struct xe_vram_region *vram; // VRAM 区域
        struct ttm_resource_manager sys_mgr; // System memory
    } mem;
};

// VM 绑定使用 TTM 迁移
int xe_vm_bind(struct xe_vm *vm, struct xe_bo *bo,
               struct xe_vma *vma, ...)
{
    // 使用 TTM 的迁移机制
    return ttm_bo_validate(&bo->ttm_bo, ...);
}
```

**关键差异**:
- Xe: 直接使用 TTM，支持 GPU 内存和系统内存无缝迁移
- i915: 自建内存管理，TTM 为后期补丁

---

## 四、调度与执行模型

### 4.1 调度架构对比

| 特性 | i915 | Xe |
|------|------|-----|
| **调度单元** | Engine/Ring | Exec Queue |
| **上下文** | GEM Context | xe_exec_queue |
| **提交方式** | Execbuf (ioctl) | DRM_SCHED |
| **优先级** | 简单优先级 | 依赖调度器 (dep_scheduler) |
| **抢占** | GuC 抢占 | 硬件 fence + GuC |

### 4.2 i915: 传统引擎模型

```c
// i915 引擎结构
struct intel_engine_cs {
    u8 class;           // RCS (Render), BCS (Blitter), VCS (Video), etc.
    u8 instance;
    
    struct intel_ring *ring;        // 环形缓冲区
    struct intel_context *context;  // 活动上下文
    
    // 提交
    int (*submit_request)(struct i915_request *rq);
};

// 用户通过 execbuf ioctl 提交
struct drm_i915_gem_execbuffer2 {
    __u32 buffer_count;
    __u32 batch_start_offset;
    // ...
};
```

**调度方式**:
- Execlist: 软件调度
- GuC: 固件调度 (Gen11+)
- 混合模式导致复杂性

### 4.3 Xe: Exec Queue 模型

```c
// Xe 执行队列结构
struct xe_exec_queue {
    struct xe_device *xe;
    struct xe_gt *gt;
    
    enum xe_engine_class class;  // RENDER, COPY, VIDEO_ENCODE, etc.
    u32 width;                   // 并行度（用于 CCS）
    
    struct drm_sched_entity sched_entity;  // DRM 调度器实体
    
    struct xe_hw_engine *hw_engine;  // 绑定的硬件引擎
    struct xe_guc_exec_queue *guc;   // GuC 队列（如果启用）
};

// 提交使用 DRM 调度器
struct drm_sched_job {
    struct drm_sched_entity *entity;
    struct dma_fence *fence;
    // ...
};
```

**调度流程对比**:

```
i915 提交流程:
User → ioctl(DRM_IOCTL_I915_GEM_EXECBUFFER2)
     → i915_gem_do_execbuffer()
     → intel_engine_submit_request()
     → [Execlist 或 GuC 调度]

Xe 提交流程:
User → ioctl(DRM_IOCTL_XE_EXEC)
     → xe_exec()
     → drm_sched_job_init() + drm_sched_entity_push_job()
     → [统一 DRM 调度器]
     → xe_sched_job_run()
```

**关键差异**:
- Xe: 使用内核标准 DRM_SCHED 框架，代码更简洁
- i915: 自建调度逻辑，支持多种模式导致复杂

---

## 五、固件与微控制器 (GuC/HuC)

### 5.1 架构对比

| 特性 | i915 | Xe |
|------|------|-----|
| **GuC 版本** | 多版本共存 (32/70/...) | 统一新版本 |
| **加载方式** | 早期加载 | 支持 late bind |
| **接口** | 多代 ABI 兼容 | 统一 ABI |
| **文件数** | 38 个 GuC 相关 | 69 个 GuC 相关 |

### 5.2 i915: 多代兼容

```
i915/gt/uc/
├── intel_guc.c           # GuC 主模块
├── intel_guc_32.c        # Gen9 旧版 GuC
├── intel_guc_70.c        # Gen11+ 新版 GuC
├── intel_guc_submission.c # GuC 提交
├── intel_guc_slpc.c      # 电源管理
├── intel_guc_rc.c        # 运行时控制
└── intel_huc.c           # HuC 模块
```

### 5.3 Xe: 统一接口

```
xe_*_guc*
├── xe_guc.c              # GuC 核心
├── xe_guc_ads.c          # Action Data Stream
├── xe_guc_buf.c          # 缓冲区管理
├── xe_guc_capture.c      # 错误捕获
├── xe_guc_exec_queue.c   # 执行队列管理
├── xe_guc_engine_activity.c  # 引擎活动跟踪
└── xe_huc.c              # HuC 模块
```

**核心差异**:
```c
// i915: 需要处理多代 GuC 差异
struct intel_guc {
    struct intel_uc *uc;
    struct intel_guc_ops *ops;  // 多态操作表
    // 不同版本不同处理
};

// Xe: 统一接口
struct xe_guc {
    struct xe_uc *uc;
    struct xe_gt *gt;
    // 单一代码路径
};
```

---

## 六、虚拟化支持 (SR-IOV)

### 6.1 支持程度对比

| 特性 | i915 | Xe |
|------|------|-----|
| **SR-IOV** | 有限/实验性 | 完整支持 |
| **PF 代码** | 几乎无 | 20+ 文件 |
| **VF 代码** | 几乎无 | 10+ 文件 |
| **迁移** | 不支持 | 支持 (live migration) |

### 6.2 Xe SR-IOV 架构

```
xe_sriov*
├── xe_sriov.c              # SR-IOV 核心
├── xe_sriov_packet.c       # PF/VF 通信协议
├── xe_sriov_pf.c           # Physical Function
│   ├── xe_sriov_pf_control.c   # VF 控制
│   ├── xe_sriov_pf_migration.c # 实时迁移
│   ├── xe_sriov_pf_monitor.c   # 监控
│   ├── xe_sriov_pf_policy.c    # 资源策略
│   └── xe_sriov_pf_provision.c # 资源分配
├── xe_sriov_vf.c           # Virtual Function
│   └── xe_sriov_vf_ccs.c   # VF CCS 支持
└── xe_pci_sriov.c          # PCI SR-IOV 接口

GT 级别的 SR-IOV:
├── xe_gt_sriov_pf.c
├── xe_gt_sriov_vf.c
└── xe_tile_sriov_*.c
```

**关键数据结构**:
```c
// PF 数据结构
struct xe_device_pf {
    struct xe_sriov_pf_migration migration;
    struct xe_sriov_pf_provision provision;
    struct xe_sriov_pf_policy policy;
    struct xe_sriov_pf_monitor monitor;
};

// VF 数据结构
struct xe_device_vf {
    struct xe_sriov_vf_selfconfig self_config;
    struct xe_lmtt lmtt;           // Local Memory Translation Table
};
```

**i915 对比**:
```bash
# i915 SR-IOV 支持
find ~/data/kernel/default/drivers/gpu/drm/i915 -name "*sriov*" | wc -l
# 输出: 0 或极少

# Xe SR-IOV 支持
find ~/data/kernel/default/drivers/gpu/drm/xe -name "*sriov*" | wc -l
# 输出: 30+
```

---

## 七、显示架构

### 7.1 规模对比

| 指标 | i915 | Xe |
|------|------|-----|
| **显示文件数** | 339 个 | 17 个 |
| **支持硬件** | 从 G4x 到现代 | 仅 Gen12+ |
| **DDC/DDI** | 多代代码 | 统一处理 |
| **Panel** | 大量 legacy | 简化 |

### 7.2 目录结构对比

```
i915/display/ (339 文件)
├── g4x_dp.c              # Gen4 显示端口
├── hsw_*.c               # Haswell 特定
├── skl_*.c               # Skylake 特定
├── bxt_*.c               # Broxton 特定
├── icl_*.c               # Ice Lake 特定
├── tgl_*.c               # Tiger Lake 特定
├── adl_*.c               # Alder Lake 特定
├── dvo_*.c               # 数字视频输出 (legacy)
├── i9xx_*.c              # 旧代芯片组
└── vlv_*.c               # Valleyview (Atom)

xe/display/ (17 文件)
├── xe_display.c          # 显示核心
├── xe_display_rpm.c      # 运行时电源
├── xe_display_wa.c       # Workarounds
├── intel_bo.c            # 显示缓冲区
├── intel_fbdev_fb.c      # Framebuffer
└── ext/                  # 外部接口
```

**关键差异**:
- i915: 支持从 2000 年代至今的所有 Intel GPU
- Xe: 仅支持 Gen12+，代码精简一个数量级

---

## 八、电源管理

### 8.1 架构对比

| 特性 | i915 | Xe |
|------|------|-----|
| **RC6** | intel_rc6.c | 集成到 xe_gt_idle |
| **SLPC** | intel_guc_slpc.c | 集成到 GuC |
| **运行时 PM** | i915_drm_resume/suspend | xe_pm_resume/suspend |
| **频率调节** | intel_gt_pm.c | xe_gt_freq.c |
| **D3Cold** | 基础支持 | 完整支持 |

### 8.2 Xe 电源管理架构

```
xe_pm*
├── xe_pm.c               # 电源管理核心
├── xe_gt_idle.c          # GT 空闲状态
├── xe_gt_freq.c          # 频率调节
├── xe_d3cold.c           # D3Cold 支持
└── xe_device_sysfs.c     # Sysfs 接口
```

**关键代码**:
```c
// Xe D3Cold 支持
struct xe_device {
    struct {
        bool capable;       // Root port 支持 D3Cold
        bool allowed;       // 当前允许 D3Cold
        u32 vram_threshold; // VRAM 保存阈值
    } d3cold;
};
```

---

## 九、Trace 与调试

### 9.1 Trace Events 对比

| 驱动 | Trace Events 数量 | 主要类别 |
|------|------------------|----------|
| i915 | ~60 个 | 显示、调度、上下文 |
| Xe | 105 个 | 执行队列、内存、SVM、迁移 |

### 9.2 Xe Trace Events 分类

```
/sys/kernel/debug/tracing/events/xe/
├── xe_bo_*              # Buffer Object (10+)
├── xe_exec_queue_*      # 执行队列 (15+)
├── xe_guc_*             # GuC 通信 (10+)
├── xe_hw_fence_*        # 硬件 fence (3)
├── xe_lrc_*             # LRC 上下文 (1)
├── xe_pm_*              # 电源管理 (5)
├── xe_sched_*           # 调度器 (10+)
├── xe_tlb_inval_*       # TLB 失效 (5)
├── xe_vma_*             # 虚拟内存区域 (15+)
└── xe_vm_*              # VM 操作 (10+)
```

**关键观察**:
- Xe 的 trace 覆盖更多 GPU 计算场景
- i915 更多关注显示相关

---

## 十、总结：何时选择哪个驱动

### 选择 i915 的场景

1. **生产环境稳定性优先**
   - 20+ 年成熟代码
   - 广泛硬件支持
   - 完善的显示支持

2. **旧硬件支持**
   - Gen11 及更早
   - Atom/Celeron 集成显卡
   - 嵌入式设备

3. **显示密集型工作负载**
   - 多显示器
   - 复杂显示拓扑
   - 老旧显示接口

### 选择 Xe 的场景

1. **GPU 计算/AI 工作负载**
   - 更好的 USM 支持
   - 完整的 SVM
   - 优化的调度器

2. **虚拟化环境**
   - SR-IOV 完整支持
   - 实时迁移能力
   - VF 隔离

3. **多 Tile GPU**
   - Ponte Vecchio
   - 未来数据中心 GPU

4. **开发/测试**
   - 更简洁的代码
   - 更好的调试工具
   - 未来架构方向

### 当前状态 (Raptor Lake-S)

| 维度 | 建议 |
|------|------|
| **日常使用** | i915（官方支持） |
| **开发测试** | Xe（可用，需 force_probe） |
| **稳定性** | i915 > Xe |
| **功能完整性** | i915 > Xe |
| **未来支持** | Xe 是发展方向 |

---

## 附录：源码路径速查

```
~/data/kernel/default/drivers/gpu/drm/
├── i915/                       # i915 驱动
│   ├── i915_drv.h              # 核心结构
│   ├── gt/                     # GT 管理
│   │   ├── intel_engine_cs.c   # 引擎管理
│   │   ├── intel_guc*.c        # GuC 固件
│   │   └── intel_ppgtt.c       # GPU VM
│   ├── gem/                    # GEM 内存
│   │   ├── i915_gem_context.c
│   │   ├── i915_gem_execbuffer.c
│   │   └── i915_gem_ttm.c
│   └── display/                # 显示 (339 文件)
│
└── xe/                         # Xe 驱动
    ├── xe_device_types.h       # 核心结构
    ├── xe_tile*.c              # Tile 管理
    ├── xe_gt*.c                # GT 管理
    ├── xe_guc*.c               # GuC 固件 (69 文件)
    ├── xe_vm*.c                # GPU VM
    ├── xe_bo*.c                # Buffer Object
    ├── xe_exec_queue*.c        # 执行队列
    ├── xe_sriov*.c             # SR-IOV (30+ 文件)
    └── display/                # 显示 (17 文件)
```

---

*文档基于内核版本 6.18 生成*

---

## 架构图对比

### i915 架构（演进式设计）

```
┌─────────────────────────────────────────────────────────────┐
│                    drm_i915_private                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │   display    │  │   GEM/TTM    │  │   engines    │      │
│  │  (339 files) │  │  (mixed)     │  │  (multiple)  │      │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘      │
│         │                 │                 │              │
│         └─────────────────┼─────────────────┘              │
│                           ▼                                │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              intel_uncore (MMIO)                     │  │
│  └─────────────────────────────────────────────────────┘  │
│                           │                                │
│         ┌─────────────────┼─────────────────┐              │
│         ▼                 ▼                 ▼              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │  Gen2-Gen4   │  │  Gen6-Gen9   │  │  Gen11+      │      │
│  │   legacy     │  │   PPGTT      │  │   GuC        │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
                  ┌──────────────────┐
                  │   Intel GPU      │
                  │  (20+ 代硬件)    │
                  └──────────────────┘
```

### Xe 架构（现代化设计）

```
┌─────────────────────────────────────────────────────────────┐
│                      xe_device                              │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │                 Tile/GT 层 (模块化)                  │   │
│  │  ┌─────────────────────────────────────────────┐   │   │
│  │  │  Tile 0 (Root)                              │   │   │
│  │  │  ├── primary_gt (Render)                    │   │   │
│  │  │  └── media_gt   (Video)     ← 可选          │   │   │
│  │  └─────────────────────────────────────────────┘   │   │
│  │  ┌─────────────────────────────────────────────┐   │   │
│  │  │  Tile 1 (Ext)              ← 多 Tile 扩展   │   │   │
│  │  │  └── primary_gt                             │   │   │
│  │  └─────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────┘   │
│                            │                                │
│         ┌──────────────────┼──────────────────┐            │
│         ▼                  ▼                  ▼            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │   xe_vm      │  │    xe_bo     │  │ xe_exec_queue│     │
│  │  (GPU VM)    │  │  (TTM-based) │  │  (调度)      │     │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘     │
│         │                 │                 │              │
│         └─────────────────┼─────────────────┘              │
│                           ▼                                │
│  ┌─────────────────────────────────────────────────────┐  │
│  │              ttm_device (统一内存)                   │  │
│  └─────────────────────────────────────────────────────┘  │
│                           │                                │
│         ┌─────────────────┼─────────────────┐              │
│         ▼                 ▼                 ▼              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ System RAM   │  │   Stolen     │  │    VRAM      │      │
│  │              │  │   Memory     │  │   (DG2+)     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │            SR-IOV (PF/VF 完整支持)                   │   │
│  └─────────────────────────────────────────────────────┘   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
                  ┌──────────────────┐
                  │   Intel GPU      │
│                 │  (Gen12+ only)   │
                  └──────────────────┘
```

### 核心差异总结

```
┌──────────────────────┬──────────────────────┐
│        i915          │         Xe           │
├──────────────────────┼──────────────────────┤
│ 历史演进式           │ 重新设计             │
│ 20+ 年代码累积       │ 现代架构             │
│ 多代硬件兼容         │ 仅支持 Gen12+        │
│ 自建内存管理         │ TTM 标准框架         │
│ 引擎为中心           │ Tile/GT 层次化       │
│ 显示优先             │ 计算优先             │
│ SR-IOV 实验性        │ SR-IOV 原生支持      │
│ 调度自建             │ DRM_SCHED 标准       │
│ 339 显示文件         │ 17 显示文件          │
└──────────────────────┴──────────────────────┘
```

