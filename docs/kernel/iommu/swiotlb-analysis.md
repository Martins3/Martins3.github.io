# SWIOTLB 常见疑问整理（结合最新上游代码）

## 1. SWIOTLB 到底是什么？

SWIOTLB（software I/O TLB）是一个**软件 bounce buffer** 机制。

当设备因为以下原因无法直接访问某段物理内存时，内核会把数据先复制到一块预留的、设备能够访问的连续低地址内存中，再把这块内存的地址交给设备做 DMA：

- 物理地址不在设备可见范围内。
- 系统启用了内存加密（AMD SME/SEV、ARM Realm 等），设备只能看到未加密内存。
- IOMMU 以 identity/passthrough 模式工作，没有地址转换。
- IOMMU 已启用翻译模式，但设备是 untrusted 且 DMA 缓冲区未按 IOMMU 页面对齐。

### 1.1 与 IOMMU 的核心区别

| 机制 | 做法 | 对设备地址的作用 |
|------|------|-----------------|
| IOMMU（翻译模式） | 在 IOMMU 页表中把 IOVA 映射到物理页 | 设备拿到的是 IOVA，可大于 `dma_mask`；IOMMU 负责翻译和保护 |
| SWIOTLB | 把原数据复制到预留的 bounce buffer，再给设备物理地址 | 设备直接访问 bounce buffer；靠复制保证数据一致性 |

也就是说：**IOMMU 是地址翻译 + 隔离；SWIOTLB 是数据复制 + 兜底。**



---

## 3. 为什么开了 IOMMU 还是看到 `swiotlb_map_*`？

这是原文档里反复出现的疑问。常见原因有三类。

### 3.1 `iommu=pt` / 默认 passthrough

`iommu=pt`（或 `iommu.passthrough=1`）只是把 IOMMU 配成 **identity（1:1）映射**，
并没有给设备提供 IOVA 翻译。此时 DMA 路径退化成 direct mapping：

```c
// kernel/dma/mapping.c
dma_map_direct(dev, ops)        /* true when use_dma_iommu(dev) == false */
  -> dma_direct_map_phys()
```

如果设备 `dma_mask` 较小（如 32 位），物理内存又超过 4GB，就会触发 swiotlb。
原文档中 Intel 机器在 `iommu=pt intel_iommu=on` 下出现 `swiotlb_map_sg_attrs`，Hygon 机器默认也是同样原因。

### 3.2 IOMMU 翻译模式下设备 untrusted 或缓冲区未对齐

见 2.2。IOMMU 页表通常按 4KB/64KB 等粒度管理，如果驱动提交的缓冲区起始地址或长度没有对齐到该粒度，内核会先把它 bounce 到对齐的 bounce buffer，再用 IOMMU 映射这个 bounce buffer。

### 3.3 根本没有 IOMMU

如果 BIOS/UEFI 没有暴露 IOMMU，或者命令行 `iommu=off`，`use_dma_iommu(dev)` 为 false，直接走 direct mapping，同样会因为 `dma_mask` 限制触发 swiotlb。

---

## 4. SWIOTLB 的“保护”体现在哪里？

原文档提到 `dev_is_untrusted()` 与 `dev_use_swiotlb()`，疑惑 swiotlb 和 IOMMU 的保护关系。这里要区分两层含义：

1. **防止设备看到不该看的数据**
   - 在 IOMMU 翻译模式下，真正隔离由 IOMMU 完成：设备只能访问分配给它的 IOVA。
   - 对于 untrusted 设备，如果直接映射一个未对齐的 buffer，IOMMU 必须映射更多页面，padding 里可能包含内核数据。swiotlb 先把数据复制到独立的 bounce buffer，并把 padding 清零，再让 IOMMU 映射这个干净的 bounce buffer。
   - 因此：**swiotlb 不是替代 IOMMU，而是配合 IOMMU 处理未对齐 / untrusted 的情况。**

2. **限制设备只能访问 bounce buffer**
   - 在 direct mapping 场景下，设备收到的 DMA 地址是 bounce buffer 的物理地址，而不是原缓冲区的地址。设备在一次 DMA 中只能读到/写到这块 bounce buffer。
   - 但它并不像 IOMMU 那样提供长期的、细粒度的地址空间隔离。

所以：**swiotlb 提供的是“受限的、临时的数据隔离 + 地址可访问性兜底”，不是完整的 IOMMU 保护。**

---

## 5. 为什么 AMD 要“Enable swiotlb in all cases”？

原文档引用的 commit `121660bba631 ("iommu/amd: Enable swiotlb in all cases")` 的背景是：


在较新的内核里，这个逻辑已经被泛化到 `drivers/iommu/dma-iommu.c` 的 `dev_use_swiotlb()`：

- `dev_is_untrusted(dev)` 为真时，就会启用 swiotlb。
- 不再由 AMD 驱动单独维护一个 `swiotlb` 变量。

因此，**不是“AMD 特殊”，而是“任何 untrusted 设备在 IOMMU 翻译模式下做未对齐 DMA 时都会 bounce”。**

---

## 6. 为什么飞腾（Phytium）默认走 `swiotlb_map_sg_attrs`？

原文档里飞腾的 `/sys/kernel/iommu_groups/*/type` 全部是 `identity`，而 Apple/ Kunpeng 是 `DMA`（翻译）。根本原因是：

- 飞腾 SMMU 驱动通过 `phytium_smmu_def_domain_type()` 把默认 domain 设成了 identity/passthrough。
- identity domain 不做 IOVA 翻译，DMA 路径等同于 direct mapping。
- 当设备 `dma_mask` 不够大（如 32 位）或内存加密时，就会进入 swiotlb。

在最新上游 ARM64 代码里，`arch_setup_dma_ops()` 已经大幅简化，不再显式设置 `arm64_swiotlb_dma_ops`，而是统一走 `dma-direct` + IOMMU 框架。默认 domain 类型由 `iommu_def_domain_type` 决定：

```c
// drivers/iommu/iommu.c
static int __init iommu_subsys_init(void)
{
    ...
    if (IS_ENABLED(CONFIG_IOMMU_DEFAULT_PASSTHROUGH))
        iommu_set_default_passthrough(false);
    else
        iommu_set_default_translated(false);
    ...
}
```

如果内核/命令行把默认 domain 设为 passthrough，行为就和原文档里飞腾的情况一样。

---

## 7. 初始化、内存占用与日志

### 7.1 启动时预留内存

```c
// kernel/dma/swiotlb.c
#define IO_TLB_SHIFT 11              /* 每个 slot 2KB */
#define IO_TLB_SIZE (1 << IO_TLB_SHIFT)
#define IO_TLB_SEGSIZE 128           /* 每段 128 个 slot = 256KB */
#define IO_TLB_DEFAULT_SIZE (64UL<<20)
```

默认大小 64MB，按 2KB slot 切分，并按 CPU 数划分 area（`default_nareas` 默认取 `num_possible_cpus()`），每个 area 有独立自旋锁以减少并发冲突。

初始化入口：

- x86：`arch/x86/kernel/pci-dma.c:pci_iommu_alloc()` -> `swiotlb_init(x86_swiotlb_enable, x86_swiotlb_flags)`。
- arm64：`arch/arm64/mm/init.c:arch_mm_preinit()` -> `swiotlb_init(swiotlb, flags)`。

### 7.2 那条启动日志

```txt
[    0.365146] software IO TLB: mapped [mem 0x00000000a9000000-0x00000000ad000000] (64MB)
```

来自 `kernel/dma/swiotlb.c:swiotlb_print_info()`：

```c
void swiotlb_print_info(void)
{
    ...
    pr_info("mapped [mem %pa-%pa] (%luMB)\n", &mem->start, &mem->end,
           (mem->nslabs << IO_TLB_SHIFT) >> 20);
}
```

`area num 128` 那条日志来自 `swiotlb_adjust_nareas()`。

### 7.3 运行期观察

```sh
# 1. 是否分配成功
dmesg | grep "software IO TLB"

# 2. debugfs 统计
cat /sys/kernel/debug/swiotlb/io_tlb_used
cat /sys/kernel/debug/swiotlb/io_tlb_nslabs
cat /sys/kernel/debug/swiotlb/io_tlb_used_hiwater

# 3. 查看设备 dma mask
for dev in /sys/bus/pci/devices/*; do
    [ -f "$dev/dma_mask_bits" ] && \
        echo "$(basename $dev): $(cat $dev/dma_mask_bits) bits"
done

# 4. ftrace 看 swiotlb 路径
sudo perf ftrace -G swiotlb_map -- sleep 1
```

---

## 8. Dynamic SWIOTLB（LWN 940973）

从 6.6 左右开始，上游引入了 `CONFIG_SWIOTLB_DYNAMIC`，目标是解决“64MB 固定池在嵌入式系统太大、在全量 bounce 场景又太小”的问题。

### 8.1 动态增长

当默认池找不到 slot 时：

```c
// kernel/dma/swiotlb.c:swiotlb_find_slots()
if (!mem->can_grow)
    return -1;

schedule_work(&mem->dyn_alloc);      /* 异步扩容默认池 */

/* 同时立即分配一个 transient pool 应急 */
pool = swiotlb_alloc_pool(dev, nslabs, nslabs, 1, phys_limit, GFP_NOWAIT);
...
pool->transient = true;
list_add_rcu(&pool->node, &dev->dma_io_tlb_pools);
```

- `dyn_alloc` work 会在进程上下文分配新的 pool 并挂到全局 `mem->pools`。
- 如果当前请求非常紧急（如在硬中断上下文），会立刻分配一个 **transient pool** 挂到 `dev->dma_io_tlb_pools`，unmap 后立即释放。

### 8.2 数据结构变化

```c
// include/linux/swiotlb.h
struct io_tlb_mem {
    struct io_tlb_pool defpool;
    unsigned long nslabs;
    ...
#ifdef CONFIG_SWIOTLB_DYNAMIC
    bool can_grow;
    u64 phys_limit;
    spinlock_t lock;
    struct list_head pools;          /* 全局动态池 */
    struct work_struct dyn_alloc;
#endif
    ...
};
```

每个 `struct device` 也新增了：

```c
struct io_tlb_mem *dma_io_tlb_mem;
#ifdef CONFIG_SWIOTLB_DYNAMIC
struct list_head dma_io_tlb_pools;   /* 该设备的 transient pools */
spinlock_t dma_io_tlb_lock;
bool dma_uses_io_tlb;
#endif
```

相关代码在 `kernel/dma/swiotlb.c:559-813` 与 `include/linux/swiotlb.h:70-126`。

更多背景可参考 LWN 文章：[More dynamic IOTLB](https://lwn.net/Articles/940973/)。

---

## 9. 为什么“不就是多拷贝一次”这个想法不完整？

原文档里有一句：“无非就是多拷贝了一次，岂不是需要 swiotlb 提前分配一个 buffer 过去？”

关键点：

- **确实是多拷贝一次**，所以 swiotlb 性能差。
- **必须提前分配**：`swiotlb_init_remap()` 在启动早期用 `memblock_alloc_low()` 预留连续物理内存；运行中由 `swiotlb_find_slots()` 从预留池里切 slot。如果池耗尽且没有 `CONFIG_SWIOTLB_DYNAMIC`，映射会失败。
- **不是简单 mmap**：bounce buffer 的地址需要满足设备的 `dma_mask`，并且映射/释放需要跟踪原始地址以便 unmap 时拷回，因此每个 slot 都有 `orig_addr`、`alloc_size`、`pad_slots` 等元数据（`struct io_tlb_slot`）。

所以 swiotlb 的代价 = 预留内存 + CPU 复制 + 额外的同步（cache flush），是最后的兜底手段。

---

## 10. 小结

| 问题 | 答案 |
|------|------|
| SWIOTLB 是干什么的？ | 软件 bounce buffer，把数据复制到设备可访问的低地址连续内存。 |
| 什么时候触发？ | `dma_mask` 不足、IOMMU passthrough/无 IOMMU、强制 bounce（内存加密/swiotlb=force）、untrusted 设备未对齐 DMA、kmalloc 不对齐。 |
| 开了 IOMMU 为什么还有 swiotlb？ | `iommu=pt` 没有翻译；翻译模式下 untrusted/未对齐也会 bounce。 |
| SWIOTLB 能替代 IOMMU 保护吗？ | 不能。它只能限制设备看到 bounce buffer，并配合 IOMMU 清零 padding；完整隔离需要 IOMMU 翻译模式。 |
| AMD 为什么 always swiotlb？ | 为 untrusted 设备的未对齐 DMA 保留 bounce 能力；现代代码已泛化为 `dev_use_swiotlb()`。 |
| 飞腾为什么默认 swiotlb？ | SMMU 默认 identity domain，没有 IOVA 翻译，设备 mask 小则 bounce。 |
| 64MB 日志是哪来的？ | `swiotlb_print_info()`，默认 `IO_TLB_DEFAULT_SIZE`。 |
| 最新代码有什么变化？ | `dma_direct_map_page` 改为 `dma_direct_map_phys`；引入 `CONFIG_SWIOTLB_DYNAMIC`；`dev_use_swiotlb` 集中到 `dma-iommu.c`；新增 `CONFIG_DMA_BOUNCE_UNALIGNED_KMALLOC`。 |

---

## 11. 主要参考文件

- `kernel/dma/swiotlb.c` — 核心实现
- `kernel/dma/direct.h` — direct mapping 路径，包括 `dma_direct_map_phys()`
- `kernel/dma/mapping.c` — `dma_map_page_attrs()`、`dma_map_sg_attrs()` 分发
- `drivers/iommu/dma-iommu.c` — IOMMU 模式下的 swiotlb 触发逻辑
- `include/linux/swiotlb.h` — 数据结构、`is_swiotlb_force_bounce()`
- `include/linux/dma-map-ops.h` — `dma_kmalloc_needs_bounce()`
- `arch/x86/kernel/pci-dma.c` — x86 初始化
- `arch/arm64/mm/init.c` — arm64 初始化
- `drivers/iommu/iommu.c` — 默认 domain 类型选择

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
