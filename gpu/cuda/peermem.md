  这个模块不是自己“实现 RDMA”，而是做两件事：

  - 向旧的 Mellanox PeerDirect 接口注册成一个 peer_memory_client
  - 把 RDMA 栈请求的“用户虚拟地址”转换成 NVIDIA 的 GPU P2P 页表和 DMA 地址

  核心链路就：

  RDMA/IB core -> nvidia-peermem.ko -> nvidia.ko 的 nvidia_p2p_* API -> GPU 显存

  对应代码：

  - PeerDirect 接口定义在 kernel-open/nvidia-peermem/peer_mem.h:47
  - nvidia-peermem 的实现主文件是 kernel-open/nvidia-peermem/nvidia-peermem.c:1
  - 底层 NVIDIA P2P API 在 kernel-open/nvidia/nv-p2p.h:193 和 kernel-open/nvidia/nv-p2p.c:650

  模块初始化

  它启动时会向 IB 栈注册一个 peer memory client：

  - 旧 client: nv_mem_client_ex
  - persistent/non-callback client: nv_mem_client_nc

  注册调用在：

  - kernel-open/nvidia-peermem/nvidia-peermem.c:571
  - kernel-open/nvidia-peermem/nvidia-peermem.c:602

  也就是说，RDMA core 后面看到某段内存时，会回调这个模块的：

  - acquire
  - get_pages
  - dma_map
  - dma_unmap
  - put_pages
  - release

  这些函数指针挂在：

  - kernel-open/nvidia-peermem/nvidia-peermem.c:492
  - kernel-open/nvidia-peermem/nvidia-peermem.c:535

  真正的数据路径

  1. acquire: 判断这段地址是不是 NVIDIA GPU memory

  nv_mem_acquire() 先把 RDMA 传进来的地址按 GPU 页大小对齐，记录到 nv_mem_context 里，见 kernel-open/nvidia-peermem/nvidia-
  peermem.c:199。

  然后它调用：

  - nvidia_p2p_get_pages(...)
  - 紧接着又 nvidia_p2p_put_pages(...)

  见 kernel-open/nvidia-peermem/nvidia-peermem.c:217 和 kernel-open/nvidia-peermem/nvidia-peermem.c:223

  这一段本质上是在“试探”：
  这块用户地址能不能被 NVIDIA 驱动识别成可做 P2P 的 GPU memory。如果可以，就返回 1 means mine。

  2. get_pages: 真正 pin 住 GPU 页

  后面 RDMA 真要建立 MR 时，会调 nv_mem_get_pages()，见 kernel-open/nvidia-peermem/nvidia-peermem.c:453。

  这里再次调用：

  - nvidia_p2p_get_pages(...)

  这次不是探测，而是真正拿回一张 nvidia_p2p_page_table，保存在 nv_mem_context->page_table 里，见 kernel-open/nvidia-peermem/
  nvidia-peermem.c:469。

  这张表描述的是：
  “这段 GPU 虚拟地址背后，对应哪些 GPU-resident 页”。

  3. dma_map: 给 RDMA 设备生成 DMA 地址

  拿到 GPU 页表后，RDMA core 会调 nv_dma_map()，见 kernel-open/nvidia-peermem/nvidia-peermem.c:298。

  这里最关键的一步是：

  - nvidia_p2p_dma_map_pages(pdev, page_table, &dma_mapping)

  见 kernel-open/nvidia-peermem/nvidia-peermem.c:324

  这一步的含义是：

  - 以 RDMA NIC 对应的 pci_dev 为目标设备
  - 让 nvidia.ko 把 GPU 页映射成这个 NIC 可直接 DMA 的总线地址
  - 返回 dma_mapping->dma_addresses[]

  随后 nvidia-peermem 把这些 DMA 地址塞进 sg_table，见 kernel-open/nvidia-peermem/nvidia-peermem.c:345。

  到这一步，RDMA HCA 拿到的已经不是“CPU 页”，而是“可以直接 DMA 到 GPU memory 的地址列表”。

  这就是 GPUDirect RDMA 的关键：

  NIC 直接 DMA 到 GPU 显存，不经过 CPU 拷贝。

  4. RDMA 使用这些 DMA 地址做传输

  这部分不在 nvidia-peermem 里，而在 IB/RDMA 栈和 NIC 驱动里。
  对 nvidia-peermem 来说，它的职责到 sg_table 填好基本就完成了。

  失效与回收

  这部分是这个模块里最关键、也最容易出错的地方。

  当 GPU 那段 memory 被释放、回收、失效时，NVIDIA 驱动会触发 free callback。
  这里注册进去的 callback 是 nv_get_p2p_free_callback()，见 kernel-open/nvidia-peermem/nvidia-peermem.c:123。

  这个 callback 做三件事：

  1. 调 mem_invalidate_callback(reg_handle, core_context) 通知 IB core
     见 kernel-open/nvidia-peermem/nvidia-peermem.c:162
  2. 释放 DMA mapping
     nvidia_p2p_free_dma_mapping(...)
     见 kernel-open/nvidia-peermem/nvidia-peermem.c:168
  3. 释放 GPU page table
     nvidia_p2p_free_page_table(...)
     见 kernel-open/nvidia-peermem/nvidia-peermem.c:172

  也就是说，失效路径是：

  GPU memory 失效 -> nvidia.ko 回调 nvidia-peermem -> nvidia-peermem 通知 RDMA core revoke -> 释放 DMA 映射和页表

  这保证 RDMA 不会继续 DMA 到一段已经无效的 GPU 显存。

  为什么会有 acquire/get_pages/dma_map 这么绕的一套

  因为旧 PeerDirect 模型要求这个模块去伪装成一种“特殊内存提供者”：

  - acquire: 先认领地址范围
  - get_pages: 提供页
  - dma_map: 提供给特定 DMA 设备可用的总线地址

  而 NVIDIA 自己并不把 GPU memory 暴露成普通 Linux struct page 给 GUP 走，所以必须通过 nvidia_p2p_* 私有 API 转一层。

  为什么它依赖 nvidia.ko

  因为真正知道：

  - GPU 虚拟地址怎么找到底层显存页
  - 哪些页能 pin
  - 怎么针对某个 PCIe peer 设备做 DMA map

  这些能力都在 nvidia.ko 的 nv-p2p.c 里，不在 nvidia-peermem 自己这里。

  所以 nvidia-peermem 更准确地说是：

  “把旧 RDMA PeerDirect API 翻译成 NVIDIA GPU P2P API 的胶水层”。

  一个重要限制

  这个模块走的是旧 Mellanox peer_memory_client 模型，不是主线现在更常见的 dma-buf 路线。
  所以你前面看到的那个宏问题，本质上就是：

  - 这套代码要旧 PeerDirect 符号
  - 你的内核更偏向新 ib_umem_dmabuf_* 路线

  一句话总结

  nvidia-peermem 实现 GPUDirect RDMA 的方式是：

  - 认领用户态传入的 GPU 地址
  - 用 nvidia.ko 的 nvidia_p2p_get_pages() 把 GPU 显存 pin 成 P2P 页表
  - 用 nvidia_p2p_dma_map_pages() 生成面向 RDMA NIC 的 DMA 地址
  - 把这些地址交给 RDMA 栈
  - 在 GPU memory 失效时通过 invalidate callback 撤销并回收

  如果你愿意，我下一步可以继续这条链画成“函数调用时序图”，从 ib_register_peer_memory_client() 一直到
  nvidia_p2p_dma_map_pages()。

## gemini fs
https://mp.weixin.qq.com/s/pbeeogrsK1bKA8FfvAWl8A
