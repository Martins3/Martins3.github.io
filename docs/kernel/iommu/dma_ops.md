## dev->dma_ops

## drgn 查看发现 dev->dma_ops 总是 NULL

```sh
cd /home/martins3/data/vn/docs/kernel/tutorial/drgn/scripts
sudo drgn -c /proc/kcore ./pci_dma_ops.py
sudo drgn -c /proc/kcore ./pci_dma_ops.py --verbose 0000:01:00.0
```

示例:

```text
0000:00:00.0  DMA_OPS=NULL  source=dev.dma_ops is NULL
  active=type=4,ops=__compound_literal.1

0000:01:00.0  DMA_OPS=NULL  source=dev.dma_ops is NULL
  active=type=1,ops=intel_ss_paging_domain_ops
```

这里 `DMA_OPS=NULL` 只说明没有 per-device `dma_map_ops` override。是否走 IOMMU DMA 需要结合 `dev->dma_iommu` 和 active domain 判断。

## 6.12 的切换

关键提交:

```text
2024-09-19 11:12 +0200 Linus Torvalds    M─┐ Merge tag 'dma-mapping-6.12-2024-09-19' of git://git.infradead.org/users/hch/dma-mapping
2024-09-12 16:28 +0200 Christoph Hellwig │ o dma-mapping: reflow dma_supported
2024-09-12 09:19 +0200 Leon Romanovsky   │ o dma-mapping: reliably inform about DMA support for IOMMU
2024-09-10 07:47 +0300 Sean Anderson     │ o dma-mapping: add tracing for dma-mapping API calls
2024-09-05 14:29 +0300 Leon Romanovsky   │ o dma-mapping: use IOMMU DMA calls for common alloc/free page calls
2024-09-04 07:08 +0300 Chen Yu           │ o dma-direct: optimize page freeing when it is not addressable
2024-09-04 07:08 +0300 Christoph Hellwig │ o dma-mapping: clearly mark DMA ops as an architecture feature
2024-09-04 07:08 +0300 Christoph Hellwig │ o vdpa_sim: don't select DMA_OPS
2024-09-03 10:25 +0300 Baruch Siach      │ o arm64: mm: keep low RAM dma zone
2024-08-29 07:22 +0300 Christoph Hellwig │ o dma-mapping: don't return errors from dma_set_max_seg_size
2024-08-29 07:22 +0300 Christoph Hellwig │ o dma-mapping: don't return errors from dma_set_seg_boundary
2024-08-29 07:22 +0300 Christoph Hellwig │ o dma-mapping: don't return errors from dma_set_min_align_mask
2024-08-29 07:22 +0300 Christoph Hellwig │ o scsi: check that busses support the DMA API before setting dma parameters
2024-08-29 07:21 +0300 Baruch Siach      │ o arm64: mm: fix DMA zone when dma-ranges is missing
2024-08-22 06:18 +0200 Leon Romanovsky   │ o dma-mapping: direct calls for dma-iommu
2024-08-22 06:18 +0200 Leon Romanovsky   │ o dma-mapping: call ->unmap_page and ->unmap_sg unconditionally
2024-08-22 06:18 +0200 Catalin Marinas   │ o arm64: support DMA zone above 4GB
2024-08-22 06:18 +0200 Catalin Marinas   │ o dma-mapping: replace zone_dma_bits by zone_dma_limit
2024-08-22 06:15 +0200 Yosry Ahmed       │ o dma-mapping: use bit masking to check VM_DMA_COHERENT
```

现在 DMA API 分发变成显式三路:

```c
if (dma_map_direct(dev, ops))
	addr = dma_direct_map_page(dev, page, offset, size, dir, attrs);
else if (use_dma_iommu(dev))
	addr = iommu_dma_map_page(dev, page, offset, size, dir, attrs);
else
	addr = ops->map_page(dev, page, offset, size, dir, attrs);
```

### 关键源码

`get_dma_ops()` 仍然先看 `dev->dma_ops`，但它现在只代表 arch/special override:

```c
static inline const struct dma_map_ops *get_dma_ops(struct device *dev)
{
	if (dev->dma_ops)
		return dev->dma_ops;
	return get_arch_dma_ops();
}
```

x86 上 `get_arch_dma_ops()` 返回全局 `dma_ops`。普通 bare metal 场景下这个指针通常也是 NULL，只有 Xen/GART 等特殊后端会设置:

```c
extern const struct dma_map_ops *dma_ops;

static inline const struct dma_map_ops *get_arch_dma_ops(void)
{
	return dma_ops;
}
```

`ops == NULL` 是 direct DMA 的正常快速路径:

```c
static bool dma_go_direct(struct device *dev, dma_addr_t mask,
		const struct dma_map_ops *ops)
{
	if (use_dma_iommu(dev))
		return false;

	if (likely(!ops))
		return true;
	...
}
```

IOMMU DMA 由 `dev->dma_iommu` 控制:

```c
static inline bool use_dma_iommu(struct device *dev)
{
	return dev->dma_iommu;
}
```

`iommu_setup_dma_ops()` 现在设置的是 `dev->dma_iommu`，不是 `dev->dma_ops`:

```c
void iommu_setup_dma_ops(struct device *dev, struct iommu_domain *domain)
{
	dev->dma_iommu = iommu_is_dma_domain(domain);
	if (dev->dma_iommu && iommu_dma_init_domain(domain, dev))
		goto out_err;

	return;
out_err:
	dev->dma_iommu = false;
}
```

## 为什么切换

- `dma_ops` 回归 architecture feature，不再作为普通 driver/backend override 机制。
- generic IOMMU DMA 是 DMA API 公共层的一等 backend，不需要再包装成 `iommu_dma_ops` 表。
- 配置更干净，常见配置不再因为 IOMMU DMA 依赖 `DMA_OPS`。

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
