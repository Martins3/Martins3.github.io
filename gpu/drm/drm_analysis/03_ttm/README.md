# TTM (Translation Table Maps)

## 什么是 TTM？

TTM 是 Linux DRM 子系统中的**通用 GPU 内存管理器**。它负责管理多种类型的内存，并处理内存之间的迁移。

### 为什么需要 TTM？

现代 GPU 系统有多种内存类型：
1. **VRAM** - 显卡上的高速显存
2. **System RAM** - 系统内存
3. **GART** - 图形地址重映射表内存
4. **Stolen Memory** - 从系统内存偷取的显存

**问题**: 这些内存有不同的特性（速度、访问方式、容量）

**TTM 解决**: 统一管理这些内存，自动在它们之间迁移数据

### TTM 核心概念

```
┌────────────────────────────────────────────────────────────────┐
│                        TTM 内存管理器                           │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │              TTM Buffer Object (BO)                      │ │
│  │  (代表一块 GPU 可访问的内存)                              │ │
│  │                                                          │ │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │ │
│  │  │ VRAM (TTM)  │ ↔  │ System (TTM)│ ↔  │ GART (TTM)  │  │ │
│  │  │  显卡显存   │    │  系统内存   │    │  映射内存   │  │ │
│  │  └─────────────┘    └─────────────┘    └─────────────┘  │ │
│  │         ↑                    ↑                    ↑      │ │
│  └─────────┼────────────────────┼────────────────────┼──────┘ │
│            │                    │                    │        │
│            └────────────────────┴────────────────────┘        │
│                         自动迁移                               │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

### 核心数据结构

```c
// TTM Buffer Object - 代表一块 GPU 内存
struct ttm_buffer_object {
    struct ttm_device *bdev;        // 所属 TTM 设备
    
    // 内存资源
    struct ttm_resource *resource;  // 当前所在的内存区域
    
    // 内存类型
    enum ttm_resource_mem_type mem_type;
    //   - TTM_PL_SYSTEM: 系统内存
    //   - TTM_PL_TT: GART/PCIe 映射内存
    //   - TTM_PL_VRAM: 显卡显存
    
    // 大小和对齐
    size_t size;
    size_t alignment;
    
    // 页面偏移（用于计算 GPU 虚拟地址）
    uint64_t offset;
    
    // 驱逐/换出相关
    struct ttm_lru_item lru;        // LRU 列表
    bool evicted;                   // 是否被驱逐
};

// 内存资源（代表具体的一块物理内存）
struct ttm_resource {
    struct ttm_device *dev;
    
    // 内存类型
    enum ttm_resource_mem_type mem_type;
    
    // 物理位置
    struct ttm_range_manager_node *mm_node;
    
    // 总线地址（用于 DMA）
    bus_addr_t bus_offset;
};

// TTM 设备（代表一个 GPU）
struct ttm_device {
    struct device *dev;
    
    // 内存管理器
    struct ttm_resource_manager man[TTM_NUM_MEM_TYPES];
    
    // LRU 列表（用于驱逐决策）
    struct list_head lru[TTM_NUM_MEM_TYPES];
    
    // 内存移动回调
    const struct ttm_device_funcs *funcs;
};
```

### 内存迁移流程

```
场景: GPU 需要访问一块在系统内存中的数据

1. 初始状态
   Buffer Object 在 System RAM
   
2. TTM 决定迁移
   - 检查访问模式
   - 决定将数据移到 VRAM
   
3. 执行迁移
   a) 在 VRAM 分配空间
   b) 使用 DMA 将数据从 System RAM 复制到 VRAM
   c) 更新 Buffer Object 的 resource 指针
   d) 释放 System RAM 中的旧空间
   
4. GPU 直接访问 VRAM
   - 高带宽
   - 低延迟
```

### TTM 的驱逐 (Eviction) 机制

当 VRAM 满时，TTM 会自动驱逐不常用的数据：

```
VRAM 已满，需要分配新缓冲区:

1. 查找 LRU 列表（Least Recently Used）
2. 选择最久未使用的 Buffer Object
3. 将该 BO 驱逐到 System RAM
   a) 在 System RAM 分配空间
   b) 复制数据
   c) 更新 BO 的内存类型
4. 释放 VRAM 空间
5. 分配新的缓冲区
```

### TTM vs GEM

| 特性 | GEM | TTM |
|------|-----|-----|
| 层级 | 高层 API | 底层实现 |
| 功能 | 显存对象管理 | 物理内存管理 |
| 内存类型 | 单一 | 多种（VRAM/System/GART）|
| 迁移 | 不支持 | 支持 |
| 驱逐 | 不支持 | 支持 |
| 驱动使用 | Intel i915 | Radeon, Nouveau, AMDGPU |

### 现代趋势: TTM + GEM 统一

Intel Xe 驱动使用了 **TTM + GEM** 的混合架构：

```
用户空间
    ↓ ioctl
GEM API (drm_gem_object)
    ↓
Intel Xe 驱动
    ↓
TTM (管理 VRAM/System 内存)
    ↓
GPU 硬件
```

这样既有 GEM 的简洁 API，又有 TTM 的强大内存管理能力。

## 实验: 观察 TTM 行为

```bash
make ttm_evict.out && ./ttm_evict.out
```

注意: TTM 是内核内部机制，用户空间只能间接观察。

## 源码参考

- `drivers/gpu/drm/ttm/ttm_bo.c` - Buffer Object 管理
- `drivers/gpu/drm/ttm/ttm_resource.c` - 内存资源管理
- `drivers/gpu/drm/ttm/ttm_tt.c` - 分页表管理
