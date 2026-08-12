# iommu=pt 到底意味着什么?

## 原来 iommu pt 不等于不存在

这是预期行为。iommu=pt 下，IOMMU 不参与每一次 dma_map_*()；它只在设备绑定 domain 时被配置为硬件 passthrough。

调用关系是：

启动/设备初始化：
iommu=pt
  → IOMMU_DOMAIN_IDENTITY
  → Intel VT-d context 设置为 PASS_THROUGH

每次 DMA 映射：
dma_map_single()
  → CPU VA 转成 PA
  → dma_map_phys()
  → dma_direct_map_phys()
  → 返回设备 DMA 地址（x86 上通常等于 PA）

关键原因在这里：

dev->dma_iommu = iommu_is_dma_domain(domain);

identity domain 不是 DMA translation domain，所以 dev->dma_iommu == false：drivers/iommu/dma-iommu.c:2113。

随后 dma_map_phys() 选择 direct 路径，而不会调用 iommu_dma_map_phys()：kernel/dma/mapping.c:155。

if (dma_map_direct(dev, ops))
        addr = dma_direct_map_phys(...);
else if (use_dma_iommu(dev))
        addr = iommu_dma_map_phys(...);

dma_direct_map_phys() 最终主要执行：

dma_addr = phys_to_dma(dev, phys);

在普通 x86 PCI 系统上通常就是：

DMA address == host physical address

但这不表示 VT-d 被关闭。设备挂载 identity domain 时，Intel 驱动已经把 VT-d context 设置为 passthrough：drivers/iommu/intel/iommu.c:3824。

设备真正发起 DMA 时：

```txt
Device emits DMA address P
        ↓
VT-d 根据 Requester ID 找到 context
        ↓
context translation type = PASS_THROUGH
        ↓
不查 IOMMU 页表，输出地址仍为 P
        ↓
内存控制器
```

因此：

- 软件映射阶段：不调用 IOMMU map/unmap。
- 硬件传输阶段：DMA 请求仍经过启用的 VT-d remapping 单元，但地址不转换。
- 性能上接近没有 IOMMU。
- 该设备没有按 buffer 建立的 DMA 隔离；dma_unmap_*() 也不会撤销页表权限。
- IOMMU 仍可服务其他 translation domain、VFIO 设备及 interrupt remapping。

另外，dma_addr_t 不是 CPU 虚拟地址。dma_map_single() 的输入才是 CPU VA，它先经 virt_to_page()、page_to_phys() 转为物理地址：include/linux/dma-
mapping.h:509。返回值是设备使用的总线地址，不能作为 CPU 指针解引用。
少数平台还可能通过 dma-ranges 加偏移，或者因 DMA mask/SWIOTLB 返回 bounce buffer 地
址。

## 确定的
1. iommu=pt 和设备直通没有关系
2. iommu=pt 之后，会影响 swiotlb ，具体看
	- docs/kernel/iommu/swiotlb/swiotlb.md
3. iommu=pt 会直接跳过 iommu page table 的 map 和 umap 操作

## 有待确认

iommu=pt 之后，iommu 硬件是不是完全不工作？
	- 在硬件上，是直接绕过了 translation 的操作吗?
iommu=pt 可以实现 DMA 保护吗？
iommu=pt 会创建 page table 吗?

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
