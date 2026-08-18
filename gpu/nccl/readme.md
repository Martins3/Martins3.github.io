## nccl
<!-- 25da7738-5372-48ed-bc50-6404718e6ca9 -->

1. 分析了 rdma 和 GPU 直接结合
sudo modprobe nvidia_peermem

但是 kernel-open/nvidia-peermem/nvidia-peermem.c 居然这么简单

2. rdma 和 NCCL 的关系


一文讲清 NCCL 集合通信原理与优化 - Tim在路上的文章 - 知乎
https://zhuanlan.zhihu.com/p/720502061

https://zhuanlan.zhihu.com/p/1932137763840458794


- 官方文档：https://docs.nvidia.com/deeplearning/sdk/nccl-developer-guide/
- 官方下载：https://developer.nvidia.com/nccl
- 测试套件：https://github.com/nvidia/nccl-tests


也许从这里入手?
https://docs.nvidia.com/deeplearning/nccl/user-guide/docs/examples.html#example-1-single-process-single-thread-multiple-devices

> [!NOTE]
> 参考神奇海螺的意见，有待验证


  - 跨节点通信时，NCCL 可以使用 IB/RoCE 传输，也就是走 RDMA 网络；官方环境变量
    NCCL_IB_DISABLE 的说明就是“禁止 NCCL 使用 IB/RoCE，改为回退到 IP sockets”，这
    反过来说明默认是会尝试用它的。
  - 如果拓扑允许，NCCL 还会启用 GPU Direct RDMA。官方对 NCCL_NET_GDR_LEVEL/
    NCCL_NET_GDR_READ 的说明表明，NCCL 可以让 NIC 直接和 GPU 内存传输，而不是总经
    过主机内存。
  - 但在单机内，NCCL 往往优先用 P2P/NVLink/PCIe/SHM，不一定涉及 RDMA。

  所以一句话总结：
  NCCL 在多机、且网络是 InfiniBand 或 RoCE 时，会使用 RDMA；如果 GPU/NIC 拓扑合 适，还可能使用 GPUDirect RDMA。

参考：

  - NCCL NCCL_IB_DISABLE / NCCL_NET_GDR_LEVEL / NCCL_NET_GDR_READ 文档:

  https://docs.nvidia.com/deeplearning/nccl/archives/nccl_2297/user-guide/docs/env.html
  - NVIDIA GPUDirect RDMA 文档:
    https://docs.nvidia.com/networking/display/gpudirectrdmav18

  如果你愿意，我可以再继续给你一版：
  怎么判断你的训练任务当前到底有没有走 RDMA。

## GPU direct rdma
<!-- 2df1477b-d3a1-4c03-8430-af04c0856c7d -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

nvidia_peermem 是 NCCL 启用 GPUDirect RDMA 功能所依赖的内核模块。

2. NCCL 为什么需要它

NCCL 在进行跨节点通信（多机训练）时：
 场景                               是否加载 nvidia_peermem   效果
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 NIC 和 GPU 在同一 PCIe Switch 下   ✅ 已加载                 NIC 直接读写 GPU 内存，性能最佳
 NIC 和 GPU 在同一 PCIe Switch 下   ❌ 未加载                 数据需经 CPU 中转，性能减半
 普通 IB/RoCE 通信                  不需要                    通过主机内存中转，可以工作但性能较低

3. 工作原理
	- GPUDirect RDMA 允许网卡（NIC）直接访问 GPU 内存，绕过 CPU
	- nvidia_peermem 提供内核级别的内存映射机制，使 RDMA 网卡能够注册和访问 GPU 内存
	- NCCL 在初始化时会检测该模块，如果存在则启用 GPUDirect RDMA 优化
4. 替代方案
	较新的 Linux 内核支持 DMA-BUF 特性，配合开源 NVIDIA GPU 驱动时，NCCL 可以自动检测并启用 DMA-BUF，此时不需要 nvidia_peermem 模块。


### gpu direct memory 的基本路线

> [!NOTE]
> 参考神奇海螺的意见，有待验证

更准确的链路是：

userspace dmabuf fd
-> ibv_reg_dmabuf_mr()
-> uverbs UVERBS_METHOD_REG_DMABUF_MR
-> RDMA driver reg_user_mr_dmabuf()
-> dma_buf_map_attachment()
-> sg_table / sg_dma_address[]
-> NIC MR metadata (lkey + MPT/MTT/KLM/PAS)
-> post receive / RDMA write WQE
-> NIC 用 lkey+addr 查自己的地址翻译表
-> 得到 DMA bus address
-> 发 PCIe Memory Read/Write TLP
-> 命中 GPU FB aperture / BAR
-> GPU memory controller 转到对应 VRAM offset

核心不是 “WQE 里直接塞 sg_dma_address”。
WQE 里通常只有 lkey + buffer address，真正的 sg_dma_address[] 在注册 MR 时已经被写进 NIC 的 MTT/PAS 之类硬件表了。

可以按这几个阶段理解。

1. 用户态把 GPU buffer 变成 dma-buf fd

这一步通常来自 GPU 驱动导出：

- amdgpu 可以把 BO 导出成 dma-buf
- 这个 fd 代表“这块 GPU buffer”

这时用户态拿到的是句柄，不是总线地址。

2. ibv_reg_dmabuf_mr() 把这个 fd 注册成 RDMA MR

用户态调用 ibv_reg_dmabuf_mr() 后，内核走到：

- drivers/infiniband/core/uverbs_std_types_mr.c:186
- include/rdma/ib_verbs.h:2534

也就是 reg_user_mr_dmabuf()。

这一步的结果不是“立刻 DMA”，而是先建立一个 MR。

3. RDMA 驱动向 dma-buf exporter 要 DMA 地址列表

这里会做：

- dma_buf_attach()
- dma_buf_map_attachment()

RDMA core / 驱动拿到一个 sg_table，里面最重要的是：

- sg_dma_address(sg)
- sg_dma_len(sg)

对于 AMDGPU VRAM，这些地址来自：

- drivers/gpu/drm/amd/amdgpu/amdgpu_vram_mgr.c:714
- drivers/gpu/drm/amd/amdgpu/amdgpu_vram_mgr.c:718
- drivers/gpu/drm/amd/amdgpu/amdgpu_vram_mgr.c:725

也就是 cursor.start + adev->gmc.aper_base 再 dma_map_resource()。

所以这里得到的本质是：
“这块 GPU buffer 对这个 NIC 来说可 DMA 的总线地址列表”。

4. RDMA 驱动把这些 DMA 地址装进 NIC 的 MR 硬件表

这是最容易漏掉的一步。

以 mlx5 为例，驱动会根据 sg_dma_address[] 去填硬件的页表/地址表：

- MPT
- MTT
- PAS
- KLM

相关路径在：

- drivers/infiniband/hw/mlx5/mr.c:1627
- drivers/infiniband/hw/mlx5/umr.c:974

注册完成后，会给用户态返回一个 lkey/rkey。

所以此时 NIC 已经“知道”：
“如果有人拿着这个 lkey 访问地址区间 X，对应该去哪些 DMA bus address”。

5. 真正发包/收包时，WQE 只引用这个 MR

例如用户态 post 一个 receive buffer，WQE 里通常是：

- addr：用户看到的 buffer 虚拟地址 / IOVA
- lkey

NIC 硬件收到 WQE 后，不是直接拿 addr 当 PCIe 地址。
它会：

1. 用 lkey 找到对应 MR
2. 用 WQE 里的地址偏移做 MR 内部地址翻译
3. 查 NIC 自己的 MTT/PAS
4. 得到真正的 DMA bus address

然后才发 PCIe TLP。

6. PCIe TLP 命中 GPU BAR / aperture

这时 NIC 发出去的是 PCIe Memory Read/Write。

如果目标地址落在 GPU 的 FB aperture / BAR 里：

- PCIe fabric 把请求路由到 GPU
- GPU 的 BAR 命中
- GPU 内存控制器把这个 BAR 内偏移转换成 VRAM 内偏移
- 最终读写到显存

这里的 GPU BAR -> VRAM，不是说“BAR 就等于整个显存”，而是：

- BAR 暴露了一个可从 PCIe 访问的 aperture
- 这块 buffer 必须能落在这个 aperture 覆盖范围内
- 所以 large BAR / visible VRAM 很关键

相关代码：

- drivers/gpu/drm/amd/amdgpu/amdgpu_gmc.h:217
- drivers/gpu/drm/amd/amdgpu/gmc_v11_0.c:712
- drivers/gpu/drm/amd/amdgpu/amdgpu_device.c:7039

你这条链里最该补的一句

不是：

sg_dma_address -> NIC WQE

而是：

sg_dma_address -> NIC MR page table
WQE -> lkey + addr
NIC HW -> 查 MR page table -> DMA address

这才是 RDMA 的标准工作方式。

再压缩成一句话

dma_buf_map_attachment() 解决“GPU buffer 对 NIC 的 DMA 地址是什么”；
ibv_reg_dmabuf_mr() 解决“把这些 DMA 地址预装进 NIC 的 MR 翻译表”；
真正收发时，NIC 用 lkey+addr 查表后，才发 PCIe TLP 去 GPU BAR，再落到 VRAM。

## 看看具体发送的过程

> [!NOTE]
> 参考神奇海螺的意见，有待验证


以 mlx5 为例，真正“NIC 发送 VRAM”要分成两段看：

第一段是注册期，把 dmabuf/VRAM 变成 NIC 能理解的 mkey + PAS/MTT。
第二段是发送期，填一个普通 send/RDMA WQE，然后敲 doorbell。发送期并不会把 GPU 的 PCIe 地址直接塞进 WQE。

源码链路在内核里是这样的。

1. dmabuf MR 注册后，mlx5 先把 VRAM 的 DMA 地址装进自己的 mkey 页表。入口在 drivers/infiniband/hw/mlx5/mr.c:1687，
   mlx5_ib_init_dmabuf_mr() 继续到 drivers/infiniband/hw/mlx5/odp.c:941 和 drivers/infiniband/hw/mlx5/odp.c:847。这里会先
   ib_umem_dmabuf_map_pages()，拿到 dma-buf 导出的 sg_table。
2. 真正把这些地址写进 NIC 翻译表的是 drivers/infiniband/hw/mlx5/umr.c:687。在这个循环里，mlx5 对每个 DMA block 取
   rdma_block_iter_dma_address(&biter)，然后写成：
    - MTT 模式：cur_mtt->ptag = dma_addr | MLX5_IB_MTT_PRESENT，见 drivers/infiniband/hw/mlx5/umr.c:777
    - KSM/data-direct 模式：cur_ksm->va = dma_addr，见 drivers/infiniband/hw/mlx5/umr.c:763
3. 这些 dma_addr 最初来自 umem 的 DMA blocks。普通 umem 转 PAS 的通用函数在 drivers/infiniband/hw/mlx5/mem.c:40，本质是把
   rdma_block_iter_dma_address() 写进 PAS。对 AMDGPU VRAM，这个 DMA 地址前面我们已经看过，是由 amdgpu 用 aper_base + offset 再
   dma_map_resource() 得到的。
4. 把 PAS/MTT 真正下发给 NIC 的动作，不是走 FW 命令队列，而是走一个专门的 UMR QP 发 UMR WQE。代码在 drivers/infiniband/hw/mlx5/
   umr.c:237。mlx5r_umr_post_send() 会：
    - mlx5r_begin_wqe(...)
    - mlx5r_memcpy_send_wqe(..., wqe, wqe_size) 把 UMR WQE 拷进去
    - mlx5r_finish_wqe(...)
    - mlx5r_ring_db(...)

   所以“注册 VRAM 给 NIC”本身就是一次特殊 send。
5. 到真正发数据时，路径在 drivers/infiniband/hw/mlx5/wr.c:1051。mlx5_ib_post_send() 先把 verbs opcode 映射成 mlx5 opcode，映射表
   在 drivers/infiniband/hw/mlx5/wr.c:12。
   对 IB_WR_RDMA_WRITE/READ，会先填远端地址段 raddr/rkey，见 drivers/infiniband/hw/mlx5/wr.c:787 和 drivers/infiniband/hw/mlx5/
   wr.c:939。
6. 本地数据源怎么填？在 drivers/infiniband/hw/mlx5/wr.c:111 和 drivers/infiniband/hw/mlx5/wr.c:1176。set_data_ptr_seg() 写入的
   是：
    - byte_count
    - lkey
    - addr

   这里的 addr 是 MR 内的本地地址/IOVA，lkey 指向前面已经注册好的 mkey。它不是 VRAM 的 PCIe bus address。
   NIC 发送时靠 lkey + addr 去查自己前面装好的 MTT/PAS，翻译出真正的 VRAM DMA 地址，然后发 PCIe 读请求把数据从 GPU 显存拉出来。
7. “启动命令”就是敲 doorbell，在 drivers/infiniband/hw/mlx5/wr.c:1025。mlx5r_ring_db() 做三件事：
    - wmb()，确保 WQE 已经写好
    - 更新 doorbell record：qp->db.db[MLX5_SND_DBR] = ...
    - 再 wmb()
    - mlx5_write64((__be32 *)ctrl, bf->bfreg->map + bf->offset) 往 UAR/BlueFlame MMIO 写 doorbell

所以整条发送链路可以压成一句：

dmabuf/VRAM -> 注册成 mkey(PAS/MTT 里是 VRAM 的 DMA 地址)
-> post_send WQE 里只填 lkey + addr (+ remote_addr/rkey)
-> ring doorbell
-> NIC 按 mkey 查表，把 addr 翻译成 VRAM DMA 地址
-> 发 PCIe read，从 GPU VRAM 取数据发到网络

补一个边界：这棵内核树里能看到的是内核态 mlx5 的等价实现。如果你想看用户态 ibv_post_send() 在 libmlx5 里如何直接写 SQ ring 和
UAR，那份源码在 rdma-core/libmlx5，本机只有库，没有对应源码树。内核里的 drivers/infiniband/hw/mlx5/wr.c:1051 已经足够说明 WQE 布
局和 doorbell 机制。

如果你要，我下一条可以继续只画一条最具体的路径：IB_WR_RDMA_WRITE 从 GPU VRAM 发出去时，本地 addr/lkey、远端 raddr/rkey、mkey/
PAS、doorbell` 各自对应哪个硬件结构。


ASK : 为什么 GPU 不用 linux 通用的 p2pdma 机制:

• 难点在于：GPU 显存对内核来说并不是“普通可 DMA 的一段内存”。通用 p2pdma 更适合处理“PCI 设备公开出来的一段相对稳定、标准化的 P2P 内存”；而 NVIDIA GPU 这
  条路径里，很多关键能力都掌握在专有驱动手里。

  主要有这几层原因。

  - GPU 显存的导出接口不是内核通用接口
    这个项目必须通过 nv-p2p.h 和 nvidia_p2p_get_pages() / nvidia_p2p_dma_map_pages() 来拿 GPU 页表和 DMA 地址，而不是直接用通用内核页管理。src/nvfs-
    mmap.h:30 src/nvfs-core.c:1316 src/nvfs-core.c:399
  - GPU 内存生命周期很特殊
    用户态可能随时释放 GPU buffer，所以内核必须注册回调，在 GPU 内存失效时撤销映射、终止 I/O。你在这个仓库里能看到专门的 free callback 和复杂状态机，这不
    是通用 p2pdma 能轻松抽象掉的。src/nvfs-core.c:291 src/nvfs-mmap.h:69
  - DMA 地址是“按 peer 设备”生成的，不是全局固定的
    同一段 GPU 内存，对不同 NVMe/NIC 看到的 DMA 地址可能不同，所以这里要按 PCI peer 缓存 dma_mapping。src/nvfs-mmap.h:129 src/nvfs-core.c:487
  - PCIe 拓扑限制很强
    能不能直达取决于 GPU 和存储/NIC 之间的 PCIe 路径、ACS、带宽、桥位置，很多机器拓扑上就不成立。这个仓库专门有一套 PCI 距离和 bridge 检查逻辑。src/nvfs-
    pci.c:90 src/nvfs-pci.c:229 src/nvfs-proc.c:117
  - IOMMU、虚拟化、安全模型不好统一
    通用内核框架要给所有设备、所有平台定义一致语义；但 GPU P2P 经常受 IOMMU、虚拟机、平台固件影响。这个仓库里甚至会根据驱动版本和是否 hypervisor 来切换不
    同 P2P API。src/nvfs-core.c:2507
  - 块层/文件系统驱动还得配合
    不是 GPU 这边能导出就结束了，NVMe、RDMA、分布式文件系统驱动还得注册这套 DMA ops。这里用了动态符号探测和注册，就是因为没法假设所有后端天然兼容。src/
    nvfs-dma.c:61 src/nvfs-mod.c:40

  所以本质上，p2pdma 想要的是“标准、稳定、可泛化”的设备内存模型；而 GPU 直达实际面对的是“专有驱动控制的页表导出 + 动态生命周期 + 拓扑依赖 + 后端驱动协
  作”。这就是它很难直接收敛成一个纯通用 Linux API 的原因。


## 你会魔改 nccl 吗?
https://www.zhihu.com/question/598730156/answer/3574596878

## 两个 3060 ，所以是如何互联的?
https://www.reddit.com/r/LocalLLaMA/comments/1tokpoc/400_qwen_3627b_setup_dual_rtx_3060_3050_ts/
