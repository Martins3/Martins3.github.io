# dma coherence

如何理解 : 94f3dc41-062d-40fe-a69e-49adcb89e9f3
## 先看文档
就是:
Documentation/memory-barriers.txt

## 终极挑战 : dma coherence
dma_unmap_page_attrs

经典考核内容:
```c
void dma_unmap_page_attrs(struct device *dev, dma_addr_t addr, size_t size,
		enum dma_data_direction dir, unsigned long attrs)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);

	BUG_ON(!valid_dma_direction(dir));
	if (dma_map_direct(dev, ops) ||
	    arch_dma_unmap_page_direct(dev, addr + size))
		dma_direct_unmap_page(dev, addr, size, dir, attrs);
	else if (use_dma_iommu(dev))
		iommu_dma_unmap_page(dev, addr, size, dir, attrs);
	else
		ops->unmap_page(dev, addr, size, dir, attrs);
	trace_dma_unmap_page(dev, addr, size, dir, attrs);
	debug_dma_unmap_page(dev, addr, size, dir);
}
```

### dma_direct_unmap_page 处理的

ARM 最后利用的是:

看上去 acpi_init_coherency 之后 arch_setup_dma_ops -> acpi_dma_configure -> arch_setup_dma_ops
```c
void arch_setup_dma_ops(struct device *dev, bool coherent)
{
	int cls = cache_line_size_of_cpu();

	WARN_TAINT(!coherent && cls > ARCH_DMA_MINALIGN,
		   TAINT_CPU_OUT_OF_SPEC,
		   "%s %s: ARCH_DMA_MINALIGN smaller than CTR_EL0.CWG (%d < %d)",
		   dev_driver_string(dev), dev_name(dev),
		   ARCH_DMA_MINALIGN, cls);

	dev->dma_coherent = coherent;

	xen_setup_dma_ops(dev);
}
```
之后通过判断设备的 dma_coherent 来决定是否来 flush cache 。

这合理吗? 为什么不是都放到 iommu 中间。

在 Mac 中可以观察到:

```txt
@[
    arch_sync_dma_for_cpu+0
    dma_unmap_page_attrs+432
    usb_hcd_unmap_urb_for_dma+108
    xhci_unmap_urb_for_dma+76
    __usb_hcd_giveback_urb+84
    usb_giveback_urb_bh+208
    process_one_work+384
    bh_worker+428
    workqueue_softirq_action+152
    tasklet_action+28
    handle_softirqs+316
    __do_softirq+28
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+240
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+232
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+156
    cpu_startup_entry+64
    secondary_start_kernel+216
    __secondary_switched+184
]: 25

@[
    arch_sync_dma_for_cpu+0
    dma_unmap_page_attrs+432
    brcmf_msgbuf_get_pktid+96
    brcmf_msgbuf_process_rx_complete+112
    brcmf_msgbuf_process_rx+544
    brcmf_proto_msgbuf_rx_trigger+64
    brcmf_pcie_isr_thread+292
    irq_thread_fn+52
    irq_thread+356
    kthread+244
    ret_from_fork+16
]: 23
```

### iommu 如何处理 cache coherence 的问题的 ?
没有，没有看到任何证据。

### 谁在注册这个 unmap_page ?
drivers/vdpa/vdpa_user/vduse_dev.c

### virtio 居然需要考虑
virtqueue_dma_sync_single_range_for_cpu

为什么只有 virtio-net 调用了?

# Cache Coherence 机制

- ccNUMA 中如何设计的?
- 只有一个 NUMA 节点的时候的设计。

## 和 cache coherence 的关系是什么
- [ ] 可以周的总结搞过来看看

- [ ] cache 中的三级 cache 同步的时候，需要每一个层级都同步吗?
- [ ] 可不可以将 load queue / store queue 也是作为 3 级 cache 中的一种方法

- 浅谈多核系统的缓存一致性协议与非均一缓存访问 : https://zhuanlan.zhihu.com/p/162099300

## 其实，cache coherence 到底提供的保证是什么?

## 忽然意识到: cache coherence 关于设备是两个问题: mmio 和 DMA

## 简单思考一个问题
中断的时候，需要把 write buffer 都 flush 掉吗?
似乎也不需要

一个 CPU 在执行

a = 1
b = 1

如果中断发生在该 CPU ，那就是顺序执行了，
如果中断发生在其他 CPU ，显然还是可以观察到乱序的


## Linux 内核中分析 DMA Coherency 的文档

## 核心 DMA API 文档

### `Documentation/core-api/dma-api.rst`

- Dynamic DMA mapping using the generic device
- Part I - DMA API
	- Part Ia - Using large DMA-coherent buffers
	- Part Ib - Using small DMA-coherent buffers
	- Part Ic - DMA addressing limitations ()
	- Part Id - Streaming DMA mappings
- Part II - Non-coherent DMA allocations
- Part III - Debug drivers use of the DMA API

说明
1. DMA addressing limitations：设备能直接寻址的 DMA 地址范围是有限的，驱动需要显式告诉内核设备能访问多大的地址空间，内核再据此决定能否把某块内存交给设备做 DMA。
2. large 和 small 分别为:
	- dma_alloc_coherent 最小单位是 page
	- dma_pool_alloc 可以是 64 字节这种
	- small DMA-coherent buffers 的具体实现为 dma_pool

Documentation/core-api/dma-api-howto.rst
Documentation/core-api/dma-attributes.rst
Documentation/driver-api/dma-buf.rst

## 和 user page 的沟通也许 flush cache

```txt
@[
    flush_dcache_page+0
    __wp_page_copy_user+48
    wp_page_copy+244
    do_wp_page+844
    handle_pte_fault+440
    __handle_mm_fault+448
    handle_mm_fault+176
    do_page_fault+308
    do_mem_abort+72
    el0_da+68
    el0t_64_sync_handler+180
    el0t_64_sync+404
]: 17598
```

```txt
@[
    flush_dcache_folio+0
    aio_complete_rw+268
    blkdev_bio_end_io_async+80
    bio_endio+372
    blk_update_request+436
    blk_mq_end_request+44
    nvme_end_req+116
    nvme_complete_rq+100
    apple_nvme_complete_rq+84
    blk_complete_reqs+92
    blk_done_softirq+40
    handle_softirqs+316
    __do_softirq+28
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+240
    irq_exit_rcu+24
    el0_interrupt+80
    __el0_irq_handler_common+24
    el0t_64_irq_handler+16
    el0t_64_irq+404
]: 186508
@[
    flush_dcache_folio+0
    aio_complete_rw+268
    blkdev_bio_end_io_async+80
    bio_endio+372
    blk_update_request+436
    blk_mq_end_request+44
    nvme_end_req+116
    nvme_complete_rq+100
    apple_nvme_complete_rq+84
    blk_complete_reqs+92
    blk_done_softirq+40
    handle_softirqs+316
    __do_softirq+28
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+240
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+268
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+156
    cpu_startup_entry+60
    secondary_start_kernel+216
    __secondary_switched+184
]: 295306
@[
    flush_dcache_folio+0
    aio_complete_rw+268
    blkdev_bio_end_io_async+80
    bio_endio+372
    blk_update_request+436
    blk_mq_end_request+44
    nvme_end_req+116
    nvme_complete_rq+100
    apple_nvme_complete_rq+84
    blk_complete_reqs+92
    blk_done_softirq+40
    handle_softirqs+316
    __do_softirq+28
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+240
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+268
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+156
    cpu_startup_entry+64
    secondary_start_kernel+216
    __secondary_switched+184
]: 295770
@[
    flush_dcache_folio+0
    read_events+124
    do_io_getevents+136
    __arm64_sys_io_getevents+104
    invoke_syscall+108
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 665110
```

原来这就是 page flags 中的 ARCH 专用的:
```c
/*
 * This function is called when a page has been modified by the kernel. Mark
 * it as dirty for later flushing when mapped in user space (if executable,
 * see __sync_icache_dcache).
 */
void flush_dcache_folio(struct folio *folio)
{
	if (test_bit(PG_dcache_clean, &folio->flags))
		clear_bit(PG_dcache_clean, &folio->flags);
}
EXPORT_SYMBOL(flush_dcache_folio);

void flush_dcache_page(struct page *page)
{
	flush_dcache_folio(page_folio(page));
}
EXPORT_SYMBOL(flush_dcache_page);
```

这些位置都是设置 flags 而已，检查 flags ，然后生效的位置在
`__sync_icache_dcache`
```txt
@[
    __sync_icache_dcache+0
    filemap_map_pages+456
    do_read_fault+240
    do_fault+320
    handle_pte_fault+360
    __handle_mm_fault+448
    handle_mm_fault+176
    do_page_fault+308
    do_translation_fault+84
    do_mem_abort+72
    el0_ia+104
    el0t_64_sync_handler+204
    el0t_64_sync+404
]: 27768

@[
    __sync_icache_dcache+0
    finish_fault+720
    do_read_fault+312
    do_fault+320
    handle_pte_fault+360
    __handle_mm_fault+448
    handle_mm_fault+176
    do_page_fault+428
    do_translation_fault+84
    do_mem_abort+72
    el0_ia+104
    el0t_64_sync_handler+204
    el0t_64_sync+404
]: 31
@[
    __sync_icache_dcache+0
    finish_fault+720
    do_read_fault+312
    do_fault+320
    handle_pte_fault+360
    __handle_mm_fault+448
    handle_mm_fault+176
    do_page_fault+428
    do_translation_fault+84
    do_mem_abort+72
    el0_da+68
    el0t_64_sync_handler+180
    el0t_64_sync+404
]: 114
@[
    __sync_icache_dcache+0
    change_pte_range+888
    change_pmd_range.isra.0+344
    change_protection_range+340
    change_protection+120
    mprotect_fixup+264
    do_mprotect_pkey.constprop.0+664
    __arm64_sys_mprotect+36
    invoke_syscall+108
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 669
```

通过分析，其实生效的地方只是在设置 page table 的位置，
在 arm 的 page arch/arm64/include/asm/pgtable.h 中:
```c
static inline void __set_pte_at(struct mm_struct *mm,
				unsigned long __always_unused addr,
				pte_t *ptep, pte_t pte, unsigned int nr)
{
	__sync_cache_and_tags(pte, nr);
	__check_safe_pte_update(mm, ptep, pte);
	__set_pte(ptep, pte);
}

static inline void __set_ptes(struct mm_struct *mm,
			      unsigned long __always_unused addr,
			      pte_t *ptep, pte_t pte, unsigned int nr)
{
	page_table_check_ptes_set(mm, ptep, pte, nr);
	__sync_cache_and_tags(pte, nr);

	for (;;) {
		__check_safe_pte_update(mm, ptep, pte;	}
}

		__set_pte(ptep, pte);
		if (--nr == 0)
			break;
		ptep++;
		pte = pte_advance_pfn(pte, 1);
)
```

其实就是页着色问题:
https://www.intel.com/content/www/us/en/docs/programmable/683836/current/virtual-address-aliasing.html

仔细看看: https://www.kernel.org/doc/Documentation/cachetlb.txt

### 但是仔细想想，flush_dcache_folio 真的可以把所有的 kernel 和 user 的地址都防范住吗?

1. 一个 page 如果同时在被两个 CPU 访问，一个 kernel 中，一个在 user 中，会有问题吗?
2. 而且从目前看，只是在 set pte 的时候，是不是完全不够啊?
3. 为什么 aio 都是需要调用 flush_dcache_folio 来 flush ，但是 iouring 就完全不需要了?
4. 如果这样，上下文切换的时候，岂不是必然将 cache 都清理一下?

## dma_wmb() : kimi

 ● dma_wmb() / dma_rmb() 是 CPU 与 DMA 设备之间做内存可见顺序保证 的屏障。它们只约束“普通内存访问”之间的顺序，而不像 wmb()/rmb() 那样还要管 MMIO。

   定义

   include/asm-generic/barrier.h:46-51：

   ```c
     #ifdef __dma_rmb
     #define dma_rmb()   do { kcsan_rmb(); __dma_rmb(); } while (0)
     #endif
     #ifdef __dma_wmb
     #define dma_wmb()   do { kcsan_wmb(); __dma_wmb(); } while (0)
     #endif
   ```

   具体实现在架构层：

   • ARM64：arch/arm64/include/asm/barrier.h:68-69
     ```c
       #define __dma_rmb()   dmb(oshld)   /* Outer Shareable, load */
       #define __dma_wmb()   dmb(oshst)   /* Outer Shareable, store */
     ```
     即只保证 Outer Shareable 域 的读/写顺序，对 DMA 设备来说已经足够。

   • x86：arch/x86/include/asm/barrier.h:50-51
     ```c
       #define __dma_rmb()   barrier()
       #define __dma_wmb()   barrier()
     ```
     x86 普通内存写本身强有序，所以只需要编译器屏障。

   语义：

   • dma_wmb()：保证它之前的内存写对 DMA 可见后，才执行它之后的写。
   • dma_rmb()：保证它之后的内存读能看到 DMA 已经写好的数据，不会读到旧值。

   典型使用场景

   1. CPU 填描述符/数据，再通知设备去取 —— 用 dma_wmb()

   最常见模式：

   ```c
     desc->addr = dma_handle;
     desc->len  = len;
     desc->flags = FLAG_VALID;

     dma_wmb();              /* 确保 desc 内容先落内存，再写 tail */

     ring->tail = new_tail;  /* 设备会从内存读这个 tail，然后去取描述符 */
   ```

   如果没有 dma_wmb()，CPU 可能先把 tail 写到了内存，描述符内容还在 cache/write buffer 里没刷出去，设备看到新 tail 后读到的却是旧描述符。

   Linux 里大量这种写法，例如 drivers/infiniband/hw/irdma/ctrl.c：

   ```c
     wqe->... = ...;
     dma_wmb(); /* make sure WQE is written before valid bit is set */
     wqe->valid = ...;
   ```

   2. CPU 读到设备更新的索引/标志后，再去读数据 —— 用 dma_rmb()

   ```c
     head = ring->head;      /* 设备通过 DMA 更新 head */
     dma_rmb();              /* 确保 head 读到新值后，再读描述符/数据 */

     desc = &ring->desc[head];
     process(desc);
   ```

   没有 dma_rmb()，CPU 可能先读到旧描述符，然后才看到新的 head。

   Virtio ring 里就有这个模式（include/linux/virtio_ring.h:39）：

   ```c
     virtio_rmb();  // 在 weak_barriers=false 时就是 dma_rmb()
   ```

   3. 触发 MMIO doorbell 时，注意 dma_wmb() 不够

   如果通知设备的方式是写 MMIO 寄存器（writel(tail, doorbell)），dma_wmb() 只约束内存到内存的顺序，不约束内存到 MMIO 的顺序。常见正确写法：

   ```c
     /* 准备 buffer */
     buf->len = len;
     dma_wmb();

     /* 再写 MMIO doorbell */
     wmb();
     writel(new_tail, regs + TAIL);
   ```

   实际驱动里常写成 dma_wmb(); writel(...)，是因为 writel() 本身在很多平台上已经隐含了必要的屏障；但严格意义上，如果平台 writel 是 relaxed 的，就需要额外 wmb()。

   什么时候“必须使用”

   满足下面两个条件时通常需要：

   1. CPU 和设备共享同一块内存（描述符环、DMA buffer、队列等）。
   2. 存在先后依赖：先写/读 A，再写/读 B，设备和 CPU 靠 B 判断 A 是否已经准备好。

   具体情形：

   • 描述符环：driver 写描述符 → 写 tail/valid bit。
   • 完成队列：driver 读 head → 读 CQE/数据。
   • 任何“先写数据，再写标志位让设备可见”的内存协议。
   • 任何“先读标志位，再读对应数据”的内存协议。

   什么时候可能不需要

   • 设备通过 MSI/中断 通知你，而你在中断处理函数里用 dma_sync_*_for_cpu() 同步过，且读的是 coherent/已同步的内存。但中断只保证“某件事发生了”，不保证内存顺序，关键路
     径仍建议加 dma_rmb()。
   • 使用 dma_alloc_coherent() 只保证 cache 一致性，不保证 写顺序。即使 coherent，描述符 + tail 这种先后依赖仍需要 dma_wmb() / dma_rmb()。

   一句话总结

   │ dma_wmb() 是“我写完数据了，再让设备看到标志”；dma_rmb() 是“我看到设备更新的标志了，再去读数据”。
   │ 它们保证的是 CPU 与 DMA 之间内存访问的可见顺序，和 dma_alloc_coherent 解决的是不同层面的问题。


### 为什么我会感觉到 dma_wmb() 是非常奇怪的

常规的 memory 都是为了让指令不要出现乱序的，但是 dma_wmb
似乎是让 cache 刷新下去。

似乎，有没有发现一个问题的本质，其实，memory model 就是类似
这个时候，将 pcie 设备其实是作为一个 CPU 来看的。
这里有一个前提，pcie 设备需要接入到 cache 一致性中，
理论上可以说，可以 L3 cache 中 1GB 的数据都不要落入到内存中。

如果没有 cache，那么 memory model 需要考虑吗?
	- 还是需要，store queue 和 load queue ，CPU 写入 memory 只用按照自己的意思来


所以，这个真的是有考虑的，可以仔细的考虑了
```txt
#define __dma_rmb()   dmb(oshld)   /* Outer Shareable, load */
#define __dma_wmb()   dmb(oshst)   /* Outer Shareable, store */
```

## DMA coherence 真的仅仅是 flush cache 吗?


- memory model 是定义更加精确的 flush cache 机制吗?

并不是的，例如这样的例子，CPU 都是从内存中读取结果就结束了:
```c
/* device */
desc->data = result;
desc->status = CPU_OWN;
```

```c
if (READ_ONCE(desc->status) == CPU_OWN) {
        dma_rmb();
        value = desc->data;
}
```

```txt
CPU 观察到 status == CPU_OWN
        happens-before
CPU 读取 desc->data
```

### 但是如何解释 mac 上的观察?

## 说说我的问题都是什么

1. memory model 和 cache coherency 什么关系? 他们的定义和管理范围是什么东西?

2. x86 中为什么 dma_rmb 为什么可以退化为 barrier() ，为什么不可以退化为空操作?
	- 先理解为什么需要 barrier() 吧

4. flush cache 就是在 dma coherencey 就完全够了吗?

5. virtio 为什么真的需要这个机制吗?

6. 为什么 mmio 为什么需要其他的机制?

7. 为什么写入有的地方，是自动的 cache 穿透的，也就是 mmio 的方向

### 是这样的吗?

cache coherency 的定义是这样的吗?
- 写传播：一个核的写入，最终必须被其他核看到。
- 写串行化：所有核对同一个地址的写入，必须被所有核观察到相同的顺序。

是这样的吗?
缓存一致性只解决了单地址的传播顺序，但管不了跨地址的乱序。
多核为了性能，都存在 Store Buffer 等部件。
它们会破坏跨地址的顺序，这些破坏行为是被“内存模型”所允许并定义清楚的。

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
