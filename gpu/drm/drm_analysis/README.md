# DRM 子系统深度分析

## 概述

本目录包含对 Linux DRM (Direct Rendering Manager) 子系统的深入分析，包括：

1. **GEM** (Graphics Execution Manager) - GPU 显存管理
2. **KMS** (Kernel Mode Setting) - 内核显示模式设置
3. **TTM** (Translation Table Maps) - 通用 GPU 内存管理器

## 三个核心问题

### 问题 1: GEM 是什么？

GEM (Graphics Execution Manager) 是 DRM 的**高层显存管理 API**。

**职责**:
- 提供用户空间接口（handle-based）
- 管理显存对象生命周期（引用计数）
- 支持显存共享（PRIME/DMA-BUF）
- **不关心物理内存位置**

**核心概念**: `struct drm_gem_object`

### 问题 2: KMS 是什么？

KMS (Kernel Mode Setting) 是内核显示子系统。

**职责**:
- 配置显示器参数（分辨率、刷新率）
- 管理显示缓冲区（framebuffer）
- 处理显示控制器（CRTC）和编码器（Encoder）

**核心概念**: `struct drm_crtc`, `struct drm_connector`, `struct drm_framebuffer`

### 问题 3: TTM 是什么？

TTM (Translation Table Maps) 是**底层物理内存管理器**。

**职责**:
- 管理多种内存类型（VRAM、系统内存、AGP）
- 处理内存迁移（GPU ↔ CPU）
- 实现内存驱逐（VRAM 满时换出）
- **不直接暴露给用户空间**

**核心概念**: `struct ttm_buffer_object`, `struct ttm_resource`

### GEM vs TTM 的关系

**一句话**: GEM 是"前台"（面向用户），TTM 是"后台"（面向硬件）

```
用户空间
    ↓ ioctl (handle)
GEM (高层 API)
    ↓
驱动 (如 Xe)
    ├── GEM 部分: handle 管理
    └── TTM 部分: 物理内存管理
        ↓
    GPU 硬件
```

详细解释见: `cat GEM_vs_TTM.md`

## 快速开始

```bash
# 进入环境并运行所有实验
nix-shell --run "make test"

# 或分步执行
cd gpu_demo/drm_analysis
nix-shell
make all
make test
```

## 实验验证 GEM vs TTM 关系

### 方法 1: 内核符号分析

```bash
# 分析 /proc/kallsyms 和模块依赖
./trace_gem_ttm.sh

# 查看实验结果
 cat EXPERIMENTS.md
```

### 方法 2: bpftrace 实时追踪 (需要 root) - 推荐

```bash
# 运行带实际 GPU 负载的 trace (密码: a)
echo "a" | sudo -S ./trace_with_workload.sh

# 查看完整分析 (含原始 trace 数据)
cat TRACE_RESULTS.md
```

### 关键发现 (实际验证)

**Trace 数据**: 分配 64MB 显存时捕获的内核调用

| 层级 | 函数 | 调用次数 | 结论 |
|------|------|----------|------|
| DRM | ioctl | 148 | 系统调用入口 |
| GEM | drm_gem_mmap | 13 | 用户空间映射 |
| TTM | ttm_resource_alloc | 592 | **物理内存分配** |

**验证结论**:
- TTM 负责实际物理内存分配 (592 次分页分配)
- GEM 提供用户接口 (mmap 映射)
- 调用链: DRM ioctl → TTM 分配 → GEM mmap

详细分析: `TRACE_RESULTS.md` (含原始数据)
原理说明: `GEM_vs_TTM.md`

## SR-IOV VF 环境说明

**重要**: 在 SR-IOV VF (虚拟功能) 模式下：
- `DRM_IOCTL_MODE_CREATE_DUMB` 被驱动层面限制
- 这是 Intel Xe 驱动对 VF 的安全策略
- 与权限无关，sudo 无法绕过

**替代方案**: 如需实际功能测试，请使用 Vulkan DMA-BUF：
```bash
../intel_xe/vulkan_dmabuf_test.out  # [OK] 工作正常
```
## 下一步做什么
- gpu_demo/drm_analysis/trace_memory_migrate.bt : ai 写的，有趣的想法，也就是显存也有类似效果?
