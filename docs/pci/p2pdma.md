# p2pdma
<!-- f13ce3b4-7d36-470b-bc0f-3332d4389276 -->


https://lwn.net/Articles/767281/

device zone ?


【研究综述】浅谈GPU通信和PCIe P2P DMA - 函谷叨客的文章 - 知乎
https://zhuanlan.zhihu.com/p/430101220



https://www.usenix.org/conference/atc17/technical-sessions/presentation/bergman

这个也需要看看了:
Documentation/driver-api/pci/p2pdma.rst


https://developer.nvidia.com/gpudirect


https://lpc.events/event/2/contributions/136/attachments/164/379/p2pdma__why_how_what_.pdf
- https://lwn.net/Articles/1022718/

## 问问 ai

drivers/pci/p2pdma.c:1074 是内核里“PCIe peer-to-peer DMA”的基础设施。它主要解决两件事：

1. GPU 和 RNIC 这两个 PCIe 设备之间，P2P DMA 能不能走。
2. 如果能走，DMA 地址该怎么编程，是直接用 bus address，还是经 host bridge / IOMMU 走普通 DMA 映射。

“RDMA 直接发送 GPU 显存”还多了一层：GPU 显存得先被导出成 RDMA 能注册的对象。主线里主要是两条路：

- dma-buf 路径：RDMA core 在 drivers/infiniband/core/umem_dmabuf.c:116 导入，mlx5 在 drivers/infiniband/hw/mlx5/mr.c:1621 注册成
  MR。GPU 侧如 amdgpu 会先用 pci_p2pdma_distance() 看拓扑，不通就禁用 peer2peer，drivers/gpu/drm/amd/amdgpu/
  amdgpu_dma_buf.c:78。
- ODP/HMM 路径：RDMA 先在 drivers/infiniband/core/umem_odp.c:324 通过 HMM 拿到 PFN，再在 mm/hmm.c:771 里调用
  pci_p2pdma_state()；如果页是可直达的 P2P 页，就直接走 pci_p2pdma_bus_addr_map()。

关键边界是：普通 ibv_reg_mr() 那条 ib_umem_get() 路径并不等于这个。
它走 FOLL_LONGTERM，drivers/infiniband/core/umem.c:164；而
GUP 默认会拒绝 PCI_P2PDMA 页，mm/gup.c:140。所以普通用户页注册，不会自动变成“GPU 显存 P2P RDMA”。

一句话总结：p2pdma.c 是“判路和映射层”，不是“GPU 显存注册层”。RDMA 直发 GPU 显存要靠 dma-buf 或 HMM/ODP 把显存暴露出来，而
p2pdma.c 决定这条 GPU<->RNIC 的 PCIe DMA 路到底能不能成立、该怎么映射。

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
