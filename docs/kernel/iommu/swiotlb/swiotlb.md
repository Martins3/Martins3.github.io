## swiotlb

## swiotlb 的基本工作原理
<!-- 719adb3e-2383-476d-ba24-760070d7f059 -->

- iommu 的流程 : 通过 map ，会获取到 iova，将 iova 填充到命令中，然后发送给设备。
	由于设备是经过 iommu 访问，通过 iova 可以访问数据

- 而 dma_direct_map_page 映射一个页面，会直接将数据拷贝新的位置
在中断中，驱动会调用 dma_unmap_sg_attrs 这会调用到 swiotlb 的 callback ，这会把数据拷贝


实现源码: kernel/dma/swiotlb.c

经典例子:

 - scsi_queue_rq
   - scsi_dispatch_cmd
     - ata_scsi_queuecmd
       - __ata_scsi_queuecmd
         - ata_scsi_translate
           - ata_qc_issue
             - ata_sg_setup
               - dma_map_sg_attrs
                 - __dma_map_sg_attrs
                   - dma_direct_map_sg
                     - dma_direct_map_page
                       - swiotlb_map

```txt
@[
    dma_unmap_page_attrs+1
    igb_clean_tx_irq+218
    igb_poll+56
    __napi_poll+40
    net_rx_action+606
    handle_softirqs+224
    irq_exit_rcu+158
    common_interrupt+130
    asm_common_interrupt+34
    cpuidle_enter_state+194
    cpuidle_enter+41
    cpuidle_idle_call+245
    do_idle+123
    cpu_startup_entry+38
    start_secondary+277
    secondary_startup_64_no_verify+404
]: 10
```


```
dma_map_page_attrs()              [kernel/dma/mapping.c]
  -> dma_map_phys()
       -> dma_direct_map_phys()   [kernel/dma/direct.h]
            -> 如果 force_bounce / dma_capable 失败 / kmalloc 不对齐
               -> swiotlb_map()   [kernel/dma/swiotlb.c]
                    -> swiotlb_tbl_map_single()
                         -> swiotlb_find_slots()   分配 IO TLB slot
                         -> swiotlb_bounce(DMA_TO_DEVICE)  把原数据拷进 bounce buffer
       -> 返回 bounce buffer 的 dma_addr
```

unmap 时走 `dma_direct_unmap_phys()` -> `swiotlb_tbl_unmap_single()`：

- 如果是 `DMA_FROM_DEVICE` / `DMA_BIDIRECTIONAL`，先 `swiotlb_bounce(DMA_FROM_DEVICE)` 把数据拷回原内存。
- 释放 slot。

`swiotlb_bounce()` 在 `kernel/dma/swiotlb.c:858` 实现，本质上就是根据方向做 `memcpy`（支持 highmem）。

## 1. 设备寻址受限
设备的 `dma_mask`


这是是拯救者笔记本中，当关闭 iommu 的时候
```txt
@[
    swiotlb_tbl_map_single+5
    swiotlb_map+115
    dma_map_page_attrs+268
    page_pool_dma_map+48
    __page_pool_alloc_pages_slow+307
    page_pool_alloc_frag+332
    mt76_dma_rx_fill.isra.0+306
    mt7921_wpdma_reset+408
    mt7921_wpdma_reinit_cond+115
    mt7921e_mcu_drv_pmctrl+33
    mt7921_mcu_drv_pmctrl+56
    mt7921_pm_wake_work+48
    process_one_work+453
    worker_thread+81
    kthread+219
    ret_from_fork+41
]: 42368
```

```txt
@[
    is_swiotlb_active+5 // 跟踪这里，返回值全部是 1
    dma_map_page_attrs+506
    page_pool_dma_map+48
    __page_pool_alloc_pages_slow+301
    page_pool_alloc_frag+324
    mt76_dma_rx_fill.isra.0+303
    mt792x_wpdma_reset+420
    mt792x_wpdma_reinit_cond+125
    mt792xe_mcu_drv_pmctrl+33
    mt792x_mcu_drv_pmctrl+56
    mt792x_pm_wake_work+44
    process_one_work+394
    worker_thread+652
    kthread+244
    ret_from_fork+49
    ret_from_fork_asm+27
```

可以知道路径在这里:
- dma_map_page_attrs
  - dma_direct_map_page
    - swiotlb_map

最新代码里有四条主要路径会进入 swiotlb。

## 2. IOMMU 翻译模式下的 bounce

即使 IOMMU 已启用，当设备是 **untrusted** 或 DMA 缓冲区未按 IOMMU 粒度对齐时，也会先 bounce：

```c
// drivers/iommu/dma-iommu.c
static bool dev_use_swiotlb(struct device *dev, size_t size,
                            enum dma_data_direction dir)
{
    return IS_ENABLED(CONFIG_SWIOTLB) &&
        (dev_is_untrusted(dev) ||
         dma_kmalloc_needs_bounce(dev, size, dir));
}
```

```c
// drivers/iommu/dma-iommu.c:iommu_dma_map_phys()
if (dev_use_swiotlb(dev, size, dir) &&
    iova_unaligned(iovad, phys, size)) {
    ...
    phys = iommu_dma_map_swiotlb(dev, phys, size, dir, attrs);
    ...
}
```
具体分两种情况：

### 2.1 dev_is_untrusted(dev) + 非 page 对齐页面

只针对标记为 untrusted 的 PCI 设备。IOMMU 通常按页映射；
如果 DMA 缓冲区没有页对齐，映射首尾页时可能顺带让设备访问缓冲区之外的内核数 据。

因此先 bounce 到一个隔离缓冲区，并清零首尾 padding，避免不可信设备越界读取或破坏相邻数据。
源码也明确做了 padding 清零

```c
static phys_addr_t iommu_dma_map_swiotlb(struct device *dev, phys_addr_t phys,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	struct iova_domain *iovad = &domain->iova_cookie->iovad;

	if (!is_swiotlb_active(dev)) {
		dev_warn_once(dev, "DMA bounce buffers are inactive, unable to map unaligned transaction.\n");
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	trace_swiotlb_bounced(dev, phys, size);

	phys = swiotlb_tbl_map_single(dev, phys, size, iova_mask(iovad), dir,
			attrs);

	/*
	 * Untrusted devices should not see padding areas with random leftover
	 * kernel data, so zero the pre- and post-padding.
	 * swiotlb_tbl_map_single() has initialized the bounce buffer proper to
	 * the contents of the original memory buffer.
	 */
	if (phys != (phys_addr_t)DMA_MAPPING_ERROR && dev_is_untrusted(dev)) {
		size_t start, virt = (size_t)phys_to_virt(phys);

		/* Pre-padding */
		start = iova_align_down(iovad, virt);
		memset((void *)start, 0, virt - start);

		/* Post-padding */
		start = virt + size;
		memset((void *)start, 0, iova_align(iovad, start) - start);
	}

	return phys;
}
```

### 2.2 uncoherent 访问

dma_kmalloc_needs_bounce(dev, size, dir)

这是为了解决非一致性 DMA 设备访问小型 kmalloc() 对象时的 cache line 共享问题。

它等价于：

!dma_kmalloc_safe(dev, dir) &&
!dma_kmalloc_size_aligned(size)

一般在下面条件同时成立时返回 true：

- 启用了 CONFIG_DMA_BOUNCE_UNALIGNED_KMALLOC
- 设备不是 DMA coherent
- 方向不是 DMA_TO_DEVICE，例如 DMA_FROM_DEVICE
- 小型 kmalloc() 对象没有达到 DMA cache-line 安全对齐

例如两个小对象共用一条 cache line：

```txt
cache line:
+----------------+----------------+
| DMA buffer     | another object |
+----------------+----------------+
```

设备向 DMA buffer 写数据后，CPU 为 DMA_FROM_DEVICE 执行 cache invalidation，
可能顺带丢弃相邻对象的脏数据。因此通过 SWIOTLB 中间缓冲区隔离它们。

不过，这个函数返回 true 不代表最终一定 bounce。
实际映射时还会检查物理缓冲区首尾是否相对 IOMMU 页粒度未对齐：

if (dev_use_swiotlb(dev, size, dir) &&
    iova_unaligned(iovad, phys, size))
        iommu_dma_map_swiotlb(...);

## 3. 强制 bounce：内存加密 / `swiotlb=force`

x86 检测到 guest memory encryption 时：

```c
// arch/x86/kernel/pci-dma.c
if (cc_platform_has(CC_ATTR_GUEST_MEM_ENCRYPT)) {
    x86_swiotlb_enable = true;
    x86_swiotlb_flags |= SWIOTLB_FORCE;
}
```

ARM64 Realm world 同理：

```c
// arch/arm64/mm/init.c
if (is_realm_world()) {
    swiotlb = true;
    flags |= SWIOTLB_FORCE;
}
```

`SWIOTLB_FORCE` 会让 `io_tlb_default_mem.force_bounce = true`，
于是 `is_swiotlb_force_bounce(dev)` 永远为真，所有 DMA 都走 bounce。

启动参数也可以强制：

```txt
swiotlb=<size>[,<areas>][,force|noforce]
```

## 经典案例分析

```txt
commit 121660bba631104154b7c15e88f208c48c8c3297
Author: Mario Limonciello <mario.limonciello@amd.com>
Date:   Tue Apr 5 04:47:22 2022

    iommu/amd: Enable swiotlb in all cases

    Previously the AMD IOMMU would only enable SWIOTLB in certain
    circumstances:
     * IOMMU in passthrough mode
     * SME enabled

    This logic however doesn't work when an untrusted device is plugged in
    that doesn't do page aligned DMA transactions.  The expectation is
    that a bounce buffer is used for those transactions.

    This fails like this:

    swiotlb buffer is full (sz: 4096 bytes), total 0 (slots), used 0 (slots)

    That happens because the bounce buffers have been allocated, followed by
    freed during startup but the bounce buffering code expects that all IOMMUs
    have left it enabled.

    Remove the criteria to set up bounce buffers on AMD systems to ensure
    they're always available for supporting untrusted devices.

    Fixes: 82612d66d51d ("iommu: Allow the dma-iommu api to use bounce buffers")
    Suggested-by: Christoph Hellwig <hch@infradead.org>
    Signed-off-by: Mario Limonciello <mario.limonciello@amd.com>
    Reviewed-by: Robin Murphy <robin.murphy@arm.com>
    Reviewed-by: Christoph Hellwig <hch@lst.de>
    Link: https://lore.kernel.org/r/20220404204723.9767-2-mario.limonciello@amd.com
    Signed-off-by: Joerg Roedel <jroedel@suse.de>
```

## edu 的实验

启动 qemu 的参数:
```txt
	-device edu,dma_mask=0xffffffff
```

然后虚拟机中测试
```sh
cd /home/martins3/data/vn/docs/kernel/iommu/swiotlb/
sudo sh -c ': > /sys/kernel/tracing/trace'
sudo sh -c 'echo 1 > /sys/kernel/tracing/events/swiotlb/enable'
sudo insmod ./swiotlb_edu.ko
sudo dmesg | grep swiotlb_edu
sudo cat /sys/kernel/tracing/trace
sudo grep . /sys/kernel/debug/swiotlb/*
sudo rmmod swiotlb_edu
sudo sh -c 'echo 0 > /sys/kernel/tracing/events/swiotlb/swiotlb_bounced/enable'
```

然后就可以观察到
```txt
# tracer: nop
#
# entries-in-buffer/entries-written: 2/2   #P:8
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
          insmod-1871    [002] .....   285.655295: swiotlb_bounced: dev_name: 0000:00:0a.0 dma_mask=ffffffff dev_addr=17c407000 size=256 NORMAL
          insmod-1871    [002] .....   285.754860: swiotlb_bounced: dev_name: 0000:00:0a.0 dma_mask=ffffffff dev_addr=17c407000 size=256 NORMAL
```

```txt
@[
        swiotlb_map+5
        dma_map_phys+564
        __SCT__tp_func_ovs_dp_upcall+1826
        local_pci_probe+64
        pci_call_probe+96
        pci_device_probe+154
        really_probe+212
        __driver_probe_device+137
        driver_probe_device+36
        __driver_attach+216
        bus_for_each_dev+145
        bus_add_driver+293
        driver_register+121
        do_one_initcall+114
        do_init_module+106
        init_module_from_file+236
        idempotent_init_module+306
        __x64_sys_finit_module+119
        do_syscall_64+171
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

## 优化

从 6.6 左右开始，上游引入了 `CONFIG_SWIOTLB_DYNAMIC`，目标是解决“64MB 固定池在嵌入式系统太大、在全量 bounce 场景又太小”的问题。

https://lwn.net/Articles/940973/

## 疑问?

### [x] 为什么还是存在
那么，为什么获取
既然有 iommu ，那么还需要是 untrusted ?

> 以前 AMD IOMMU 只在 passthrough 模式或 SME 启用时才启用 swiotlb。
但当插入一个 untrusted 设备且该设备进行非页对齐 DMA 时，bounce buffer 可能根本不存在，
导致 `swiotlb buffer is full` 错误。

### edu 既然在 iommu 的情况下测试的?
为什么还是有 swiotlb 的使用啊

### kmalloc coherent 访问如何理解?

## 参考文档
- http://xillybus.com/tutorials/iommu-swiotlb-linux


## 原来如此哦

实测结果

```txt
  swiotlb_edu 0000:00:0a.0: DMA_TO_DEVICE original phys=0x0000000104b0c000 mapped dma=0x0000000096e00000 (SWIOTLB bounced)
  swiotlb_edu 0000:00:0a.0: DMA_FROM_DEVICE original phys=0x0000000104b0c000 mapped dma=0x0000000096e00800 (SWIOTLB bounced)
  swiotlb_edu 0000:00:0a.0: SWIOTLB DMA round-trip passed (256 bytes)
```

ftrace：

```txt
  swiotlb_bounced: dev_name: 0000:00:0a.0 dma_mask=ffffffff dev_addr=104b0c000 size=256 NORMAL
  swiotlb_bounced: dev_name: 0000:00:0a.0 dma_mask=ffffffff dev_addr=104b0c000 size=256 NORMAL
```

为什么会 bounce

yyds-fs 的启动配置是：

```txt
  -device virtio-iommu-pci
  -device edu,dma_mask=0xffffffff
  append ... iommu.passthrough=1
```

iommu.passthrough=1 让所有设备都走 identity/passthrough domain，DMA 地址等于物理地址。EDU 设备只有 32-bit DMA mask，而驱动故意分配到
0x104b0c000（超过 4 GiB），dma_direct_map_phys 里 dma_capable() 失败，只能走 swiotlb_map 把数据 bounce 到 0x96e00000 这种低地址缓冲区。

这和 swiotlb.md 第二节讲的 “IOMMU 翻译模式下因 untrusted/unaligned 而 bounce” 不是同一个场景——那个场景会走 iommu_dma_map_phys。EDU 实验实际演
示的是 IOMMU 不透传翻译时，设备 DMA mask 受限导致的 bounce。

如果你想看 IOMMU 翻译模式下的对比，需要把 yyds-fs 的 opt/cmdline 里的 iommu.passthrough=1 去掉再重启；那种情况下 IOMMU 会把高物理地址映射到
32-bit IOVA，通常就不会再触发 swiotlb（除非再命中 untrusted/unaligned 分支）。

## swiotlb=force iommu.passthrough=1 如果打开，所有都会过 swiotlb

如果 swiotlb=force iommu.passthrough=0 还是会走 iommu 的

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
