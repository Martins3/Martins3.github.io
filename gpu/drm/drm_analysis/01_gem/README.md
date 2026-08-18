# GEM (Graphics Execution Manager)

## 什么是 GEM？

GEM 是 Linux DRM 子系统中负责 GPU 显存管理的组件。

### 核心思想

GPU 显存是一种稀缺资源，需要：
1. **统一管理** - 内核统一管理所有显存分配
2. **安全隔离** - 不同应用之间的显存相互隔离
3. **共享机制** - 支持显存在不同上下文间共享

### 核心数据结构

```c
// 内核中的 GEM 对象
struct drm_gem_object {
    struct kref refcount;           // 引用计数
    struct file *filp;              // 文件指针（用于 mmap）
    struct drm_device *dev;         // 所属 DRM 设备
    
    // 尺寸信息
    size_t size;
    
    // 导出句柄（用户空间通过 handle 访问）
    u32 handle;
    
    // 导出 DMA-BUF
    struct dma_buf *dma_buf;
    
    // 特定驱动的私有数据
    void *driver_private;
};
```

### 工作流程

```
用户空间                        内核空间 (DRM)
    |                               |
    | 1. 创建 GEM 对象              |
    |------------------------------>|
    |   DRM_IOCTL_GEM_CREATE        |
    |                               |---> 分配显存
    |                               |     创建 drm_gem_object
    |                               |     返回 handle
    |<------------------------------|
    |   返回 handle (如: 0x1234)    |
    |                               |
    | 2. mmap GEM 对象              |
    |------------------------------>|
    |   DRM_IOCTL_GEM_MMAP          |
    |                               |---> 设置内存映射
    |<------------------------------|
    |   返回映射地址                |
    |                               |
    | 3. 访问显存                   |
    |   直接读写 mmap 区域          |
    |                               |
    | 4. 释放 GEM 对象              |
    |------------------------------>|
    |   DRM_IOCTL_GEM_CLOSE         |
    |                               |---> 释放显存
```

### 关键 IOCTL

| IOCTL | 功能 |
|-------|------|
| `DRM_IOCTL_GEM_CREATE` | 创建 GEM 对象 |
| `DRM_IOCTL_GEM_CLOSE` | 关闭 GEM 对象 |
| `DRM_IOCTL_GEM_MMAP` | 映射 GEM 对象到用户空间 |
| `DRM_IOCTL_PRIME_HANDLE_TO_FD` | 导出 DMA-BUF |
| `DRM_IOCTL_PRIME_FD_TO_HANDLE` | 导入 DMA-BUF |

## 实验 1: GEM 分配

查看 `gem_alloc.c` 了解如何创建和释放 GEM 对象。

```bash
make gem_alloc.out && ./gem_alloc.out
```

## 实验 2: GEM mmap

查看 `gem_mmap.c` 了解如何映射 GEM 对象到用户空间。

```bash
make gem_mmap.out && ./gem_mmap.out
```

## 源码参考

- `drivers/gpu/drm/drm_gem.c` - GEM 核心实现
- `drivers/gpu/drm/drm_prime.c` - DMA-BUF 导出/导入
