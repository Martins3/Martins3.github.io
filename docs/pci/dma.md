## 内核中关于 dma 的几个目录做什么的
<!-- a5d7b3c3-34a3-4dc1-9090-1c86905e7d93 -->

- kernel/dma/
	- swiotlb.c : 老朋友了
	- dummy.c
	- debug.c : 看下面小结的分析
	- direct.c : 这就是当没有 iommu 支持的时候，那么就走到这里
- drivers/dma-buf/
- drivers/dma/
- kernel/bpf/dmabuf_iter.c : 就是遍历 dma buf 的，就不用说什么的


在 qemu 中 hw/dma/ 存在 dma 相关的模拟，其实勉强是可以和内核中 drivers/dma/ 中对应的:
- drivers/dma/pl330.c 用于 ARM Cortex-A SoC 和 AMBA 总线
- drivers/dma/xilinx/ : 也许我们的真的应该找一个 FPGA 卡，编写程序之后，插到物理机中测试一下
- drivers/dma/stm32/ : 估计是其中一个很简单的控制器了
此外，drivers/dma/ 的 idxd 是 intel 的 dsa 加速器

## 关于 kernel/dma/debug.c 的作用
<!-- b0b60030-18bd-4d91-b565-2361761235bb -->

1. 默认情况下，debugfs 下
```txt
root@localhost:/sys/kernel/debug# ls dma*
dma_buf:
	bufinfo

dmaengine:
	summary

dma_pools:
	pool_size_dma  pool_size_dma32  pool_size_kernel
```

2. 如果打开 CONFIG_DMA_API_DEBUG=y 可以观察到的结果:
```txt
root@localhost:/sys/kernel/debug# ls dma*
dma-api:
	all_errors  driver_filter  error_count       nr_total_entries  num_free_entries
	disabled    dump           min_free_entries  num_errors
```
也就是新添加了一个 `dma-api` 的目录:

M1 上观察到的:
```txt
 sudo cat /sys/kernel/debug/dmaengine/summary
	dma0 (238200000.dma-controller): number of channels: 24
	 dma0chan0    | in-use
	 dma0chan3    | in-use
	 dma0chan4    | in-use
	 dma0chan11   | in-use

	dma1 (24a980000.dma-controller): number of channels: 16
```

13900k 的结果:
```txt
root@linux:/sys/kernel/debug/dmaengine# ls
	0000:00:15.0  0000:00:15.1  0000:00:15.2  summary
root@linux:/sys/kernel/debug/dmaengine# cat summary
	dma0 (0000:00:15.0): number of channels: 2
	dma1 (0000:00:15.1): number of channels: 2
	dma2 (0000:00:15.2): number of channels: 2
```

```txt
root@linux:/sys/kernel/debug/dmaengine# lspci -s 0000:00:15.0
	00:15.0 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #0 (rev 11)
root@linux:/sys/kernel/debug/dmaengine# lspci -s 0000:00:15.1
	00:15.1 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #1 (rev 11)
root@linux:/sys/kernel/debug/dmaengine# lspci -s 0000:00:15.2
	00:15.2 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #2 (rev 11)
```

dma-api 中 dump 的内容为:
```txt
virtio-pci 0000:00:05.0 single idx 12752 P=0x000000010e3a0010 D=10e3a0010 L=7ff0 cln=0x000000000438e800 DMA_FROM_DEVICE dma map error checked
virtio-pci 0000:00:05.0 single idx 13276 P=0x000000010e7b8010 D=10e7b8010 L=7ff0 cln=0x000000000439ee00 DMA_FROM_DEVICE dma map error checked
virtio-pci 0000:00:0a.0 scatter-gather idx 13288 P=0x00000001167d0000 D=1167d0000 L=30000 cln=0x000000000459f400 DMA_BIDIRECTIONAL dma map error check not applicable
virtio-pci 0000:00:0a.0 scatter-gather idx 14336 P=0x0000000117000000 D=117000000 L=62000 cln=0x00000000045c0000 DMA_BIDIRECTIONAL dma map error check not applicable
```

所以，就是这样，专门用来调试 DMA API 使用的:
```txt
config DMA_API_DEBUG
	bool "Enable debugging of DMA-API usage"
	select NEED_DMA_MAP_STATE
	help
	  Enable this option to debug the use of the DMA API by device drivers.
	  With this option you will be able to detect common bugs in device
	  drivers like double-freeing of DMA mappings or freeing mappings that
	  were never allocated.

	  This option causes a performance degradation.  Use only if you want to
	  debug device drivers and dma interactions.

	  If unsure, say N.
```



## docs/kernel/iommu/dma.md 关联的看看

## dma 的一致性问题
- wbinvd : 这个做啥的 ?


pcie cxl 区别体现在什么地方?

## 理解 cxl 中的一致性 ，cache 一致性，dma 的一致性，

## dma
- dma_alloc_coherent 如何理解?

## dma 和想象不同的点在于，似乎没有那么多地方使用

### 为什么 nvme 只有这个地方才需要?

```c
static inline bool nvme_poll_cq(struct nvme_queue *nvmeq,
			        struct io_comp_batch *iob)
{
	bool found = false;

	while (nvme_cqe_pending(nvmeq)) {
		found = true;
		/*
		 * load-load control dependency between phase and the rest of
		 * the cqe requires a full read memory barrier
		 */
		dma_rmb();
		nvme_handle_cqe(nvmeq, iob, nvmeq->cq_head);
		nvme_update_cq_head(nvmeq);
	}

	if (found)
		nvme_ring_cq_doorbell(nvmeq);
	return found;
}
```

而且还一开始没有，后面才有的这个东西。
commit b69e2ef24b7b ("nvme-pci: dma read memory barrier for completions")


### virtio

include/linux/virtio_ring.h 中，非常合理，和硬件打交道的地方:
```c
/*
 * Barriers in virtio are tricky.  Non-SMP virtio guests can't assume
 * they're not on an SMP host system, so they need to assume real
 * barriers.  Non-SMP virtio hosts could skip the barriers, but does
 * anyone care?
 *
 * For virtio_pci on SMP, we don't need to order with respect to MMIO
 * accesses through relaxed memory I/O windows, so virt_mb() et al are
 * sufficient.
 *
 * For using virtio to talk to real devices (eg. other heterogeneous
 * CPUs) we do need real barriers.  In theory, we could be using both
 * kinds of virtio, so it's a runtime decision, and the branch is
 * actually quite cheap.
 */

static inline void virtio_mb(bool weak_barriers)
{
	if (weak_barriers)
		virt_mb();
	else
		mb();
}

static inline void virtio_rmb(bool weak_barriers)
{
	if (weak_barriers)
		virt_rmb();
	else
		dma_rmb();
}

static inline void virtio_wmb(bool weak_barriers)
{
	if (weak_barriers)
		virt_wmb();
	else
		dma_wmb();
}

#define virtio_store_mb(weak_barriers, p, v) \
do { \
	if (weak_barriers) { \
		virt_store_mb(*p, v); \
	} else { \
		WRITE_ONCE(*p, v); \
		mb(); \
	} \
} while (0) \
```


### 其他的

这个设备太不熟悉了，
```diff
commit f81781835f0adfae8d701545386030d223efcd6f
Author: Catherine Sullivan <csully@google.com>
Date:   Mon May 17 14:08:14 2021 -0700

    gve: Upgrade memory barrier in poll routine

    As currently written, if the driver checks for more work (via
    gve_tx_poll or gve_rx_poll) before the device posts work and the
    irq doorbell is not unmasked
    (via iowrite32be(GVE_IRQ_ACK | GVE_IRQ_EVENT, ...)) before the device
    attempts to raise an interrupt, an interrupt is lost and this could
    potentially lead to the traffic being completely halted. For
    example, if a tx queue has already been stopped, the driver won't get
    the chance to complete work and egress will be halted.

    We need a full memory barrier in the poll
    routine to ensure that the irq doorbell is unmasked before the driver
    checks for more work.

    Fixes: f5cedc84a30d ("gve: Add transmit and receive support")
    Signed-off-by: Catherine Sullivan <csully@google.com>
    Signed-off-by: David Awogbemila <awogbemila@google.com>
    Acked-by: Willem de Brujin <willemb@google.com>
    Signed-off-by: David S. Miller <davem@davemloft.net>

diff --git a/drivers/net/ethernet/google/gve/gve_main.c b/drivers/net/ethernet/google/gve/gve_main.c
index 21a5d058dab4..bbc423e93122 100644
--- a/drivers/net/ethernet/google/gve/gve_main.c
+++ b/drivers/net/ethernet/google/gve/gve_main.c
@@ -180,7 +180,7 @@ static int gve_napi_poll(struct napi_struct *napi, int budget)
 	/* Double check we have no extra work.
 	 * Ensure unmask synchronizes with checking for work.
 	 */
-	dma_rmb();
+	mb();
 	if (block->tx)
 		reschedule |= gve_tx_poll(block, -1);
 	if (block->rx)
```

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
