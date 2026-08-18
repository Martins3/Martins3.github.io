https://docs.nvidia.com/gpudirect-storage/getting-started/index.html

https://developer.nvidia.com/blog/gpudirect-storage/

https://github.com/NVIDIA/gds-nvidia-fs

## GPU direct fs
<!-- 06e1be29-a25d-4a47-9a76-cdec57376b76 -->

https://github.com/NVIDIA/gds-nvidia-fs

似乎的确是这样的，这样的话，也就是将 nvme 中的数据直接写入到 GPU 中了:

那么，其实我想，既然 fs 都可以可以直接写入到 fs 中的，那么块存储或者对象存储，直接写入到文件系统中，这个操作早就存在了吧


如果不用这套路径，读文件到 GPU 往往是这样：
1. 存储设备 DMA 到 CPU 内存
2. CPU 或 DMA 再把这份数据拷到 GPU 显存

用了 nvidia-fs 之后，目标变成：
1. 存储设备尽量直接 DMA 到 GPU 显存


> [!NOTE]
> 参考神奇海螺的意见，有待验证

ASK : 这个源码是做什么的?

• 这个项目本质上不是一个文件系统，而是一个 Linux 内核模块 nvidia-fs.ko，用来给 NVIDIA GPUDirect Storage 提供内核侧支持：让存储设备发起的 DMA/
  RDMA 尽量直接落到 GPU 显存，而不是先经过 CPU 内存中转。README.md:3 README.md:7

  它解决的问题
  普通 read/write 的目标通常是用户态 CPU 缓冲区。这个项目的做法是伪造出一块“看起来像用户缓冲区”的 shadow buffer，然后在块层/文件系统做 SG/DMA
  映射时，把这些页识别出来，替换成 GPU 页对应的 DMA 地址。核心数据结构是 src/nvfs-mmap.h:177，里面同时绑了：

  - shadow buffer 页和元数据
  - GPU 页表/映射信息
  - 当前一次 I/O 的上下文
  - 可选 RDMA 信息

  可以把它理解成“一个 CPU 虚拟地址窗口 + 一段 GPU 缓冲区 + 一次文件 I/O 状态”的绑定对象。src/nvfs-mmap.h:104 src/nvfs-mmap.h:136 src/nvfs-
  mmap.h:177

  整体调用链

  1. 模块加载后注册字符设备 `/dev/nvidia-fs*`，并创建 /proc/driver/nvidia-fs/* 状态接口。src/nvfs-core.c:2503 src/nvfs-proc.c:241
  2. 用户态先对这个字符设备 mmap，拿到 shadow buffer；对应逻辑在 src/nvfs-mmap.c:809。
  3. 然后通过 NVFS_IOCTL_MAP 把 cpuvaddr + gpuvaddr + end_fence 绑定起来；这里会调用 NVIDIA P2P API 把 GPU 页 pin 住并拿到页表。src/nvfs-
     core.h:226 src/nvfs-core.c:1505 src/nvfs-core.c:1241
  4. 后续 NVFS_IOCTL_READ/WRITE 会根据 cpuvaddr 找回这个 mgroup，校验 fd/inode/dev/offset，然后调用文件的 read_iter/write_iter 走 direct I/O
     路径。src/nvfs-core.c:1586 src/nvfs-core.c:1973 src/nvfs-core.c:2210
  5. 真正关键在 DMA 映射阶段：nvfs-dma.c 把 shadow buffer 页识别成 GPU 页，并通过 nvidia_p2p_dma_map_pages() 得到给某个 PCI peer 设备使用的
     DMA 地址。src/nvfs-core.c:369 src/nvfs-core.c:487 src/nvfs-dma.h:74
  6. 同步 I/O 直接返回结果；异步 I/O 通过 completion 回调更新 end-fence 页，用户态轮询它知道 I/O 完成。src/nvfs-core.c:808 src/nvfs-core.c:995

  几个核心文件各自做什么

  - src/nvfs-core.c:2210: 字符设备、ioctl、I/O 生命周期、GPU 页 pin/unpin、异步完成。
  - src/nvfs-mmap.c:809: shadow buffer 的 mmap、mgroup 哈希表、元数据页管理、状态机。
  - src/nvfs-dma.c:61: 提供给 NVMe/NFS/RDMA/分布式文件系统驱动注册的 DMA/SG 回调表。
  - src/nvfs-mod.c:40: 用 __symbol_get() 动态探测并挂接后端模块，不和某个存储驱动硬编码耦合。
  - src/nvfs-pci.c:90: 计算 GPU 和 peer 设备的 PCIe 拓扑距离/带宽优先级。
  - src/nvfs-proc.c:41: /proc/driver/nvidia-fs/ 下的版本、模块、bridge、peer affinity、distance 等观测接口。
  - src/nvfs-rdma.c:33: 为 GPFS/NFS-RDMA 这类路径保存和导出 rkey/QP/GID 等 RDMA 注册信息。

  这个项目最重要的设计点

  - 它不自己实现文件系统语义，真正的数据读写仍然调用底层文件对象的 read_iter/write_iter。src/nvfs-core.c:1042
  - 它的核心价值在于“劫持/扩展 DMA 映射层”，让块层看到的缓冲页最终映射到 GPU 显存。
  - 它通过动态注册支持多个后端：NVMe、NVMe-RDMA、Lustre、BeeGFS、NFS/RPCRDMA、GPFS 等。src/nvfs-dma.c:61
  - 它非常依赖 NVIDIA 驱动源码和 nv-p2p.h，构建时会主动去找 NVIDIA driver sources，并生成符号依赖。src/Makefile:21 src/Makefile:108


这个和 xfs 有关系吗?
• 有关系，但不是“XFS 的一部分”。

  更准确地说，这个项目是 给 GPUDirect Storage 用的内核模块，它可以和 XFS 上的 direct I/O 配合工作。README 明确写了支持 “XFS and EXT4
  filesystem in ordered mode on NVMe/NVMeOF...”。README.md:11

  关系大概是这样：

  - nvidia-fs 不实现 XFS，也不修改 XFS 源码。
  - 它走的是通用文件接口，最后调用文件对象的 read_iter/write_iter 做 direct I/O。src/nvfs-core.c:2095 src/nvfs-core.c:995
  - 真正特殊的地方在 DMA 映射层：它把本来面向 CPU 缓冲区的 DMA 映射，转成 GPU 页的 DMA 映射。

  所以从分层上看：

  - XFS 负责文件系统语义、extent、inode、direct I/O 路径。
  - nvidia-fs 负责把这条 I/O 路径里的“数据缓冲区”改造成 GPU 显存可直达。

  另外，仓库里几乎没有专门的 XFS 适配代码，rg 只在 README 里看到了 XFS，没有单独的 xfs_* 模块或 hook。这也说明它主要是依赖 通用 VFS/DIO/块层机
  制，而不是绑死在 XFS 上。反而代码里对 ext4 类问题还有通用处理，比如写入前 fallocate/flush page cache，注释里明确提到 “ext4 可能回退到
  buffered I/O”。src/nvfs-core.c:1965 src/nvfs-core.c:2031

  如果你愿意，我可以下一步专门讲一下：为什么它能和 XFS 配合，但代码里却几乎看不到 XFS 专用逻辑。


• 这里说的“硬件支持”，核心就是：设备之间必须真的能做 PCIe peer-to-peer
  DMA，也就是存储/NIC 能把数据直接 DMA 到 GPU 可导出的那块地址空间，而
  不是必须先绕回主内存。

  具体分成几层：

  - GPU 要支持 GPUDirect
    这个仓库依赖 NVIDIA 的 nv-p2p.h 和 nvidia_p2p_* 接口，不是任意 GPU
    都行。src/Makefile:23 src/nvfs-core.c:1316
  - 对端设备要能发起 DMA/RDMA
    比如 NVMe、NVMe-oF、RDMA NIC，或者相应分布式文件系统后端。README 里
    列的支持对象本质上都是这类设备/路径。README.md:11
  - PCIe 拓扑要允许 P2P
    GPU 和存储/NIC 之间的 PCIe 路径不能被桥、ACS、IOMMU/虚拟化之类机制
    挡掉。这个项目里专门有代码在算 GPU 和 peer 设备的拓扑距离、带宽和
    ACS bridge。src/nvfs-pci.c:90 src/nvfs-pci.c:229 src/nvfs-
    proc.c:117
  - 驱动要支持这条路径
    不只是硬件能连上，还得 NVIDIA 驱动和 NVMe/RDMA 侧驱动愿意注册/配合
    这套 DMA ops。src/nvfs-dma.c:61 src/nvfs-mod.c:40

  和 p2pdma 的关系是：

  - 概念上，有关系。
    它做的就是设备到设备的 peer-to-peer DMA。
  - 实现上，这个项目不是走 Linux 通用 pci_p2pdma 那套。
    它走的是 NVIDIA 自己的 GPUDirect P2P 接口，比如
    nvidia_p2p_get_pages()、nvidia_p2p_dma_map_pages()。src/nvfs-
    core.c:1316 src/nvfs-core.c:399

  所以更准确地说：

  - p2pdma 是上游 Linux 对“设备间直接 DMA”的通用框架。
  - nvidia-fs 做的事情在原理上属于同一类问题。
  - 但这个仓库实际依赖的是 NVIDIA 专有的 GPUDirect P2P 机制，不是通用
    pci_p2pdma API。

  一句话总结：有关系，但不是一回事；它更像是 NVIDIA 生态里的专用 P2P
  DMA 路径。

  如果你想，我可以下一步专门解释一下：为什么 GPU 直达这件事很难直接做成
  通用 Linux p2pdma。

## 模块构建一下

• 已经基于 /home/martins3/data/kernel/linux-build 把这个仓库编出来了，
产物是 src/nvidia-fs.ko。modinfo 显示它的 vermagic 是 7.0.0-00001-gafd597f0ba30 SMP preempt mod_unload，对应你给的内核构建树。

这次我做了两类修正：一类是让构建脚本真正支持外部 KDIR，修改在 src/Makefile 和 src/configure；另一类是补这个老版本 gds-nvidia-fs 对 7.0 内核 API 的兼
容，主要在 src/nvfs-mmap.h, src/nvfs-mmap.c, src/nvfs-dma.h, src/nvfs-dma.c。

需要注意一件事：编译时仍有 modpost 警告，nvidia_p2p_* 符号目前是未解析状态。模块已经生成，但真正 insmod 到这棵 7.0.0-00001-gafd597f0ba30 内核时，仍然需
要对应版本的 NVIDIA 内核模块在该目标内核上导出这些符号。 open-gpu-kernel-modules/kernel-open/nvidia/nv-p2p.h


gds-nvidia-fs 不是直接“自己实现设备间通信”，而是通过 nv-p2p.h 里声明的 NVIDIA P2P 接口，去调用 NVIDIA 驱动导出的符号，比如：

- nvidia_p2p_get_pages
- nvidia_p2p_get_pages_persistent
- nvidia_p2p_dma_map_pages
- nvidia_p2p_dma_unmap_pages


1. 用户态先拿到一段“shadow buffer”虚拟地址，这段地址背后是 nvidia-fs 自己分配并 vm_insert_page() 到用户 VMA 的页。
src/nvfs-mmap.c:747 到 src/nvfs-mmap.c:762
2. 然后 nvfs_map() 把这段 shadow buffer 和真正的 GPU buffer 绑定起来：先 pin 住 shadow buffer 页，再通过 NVIDIA P2P API 把 GPU 虚拟地址解析成
GPU page table / physical address。 src/nvfs-core.c:1513 到 src/nvfs-core.c:1535，src/nvfs-core.c:1316 到 src/nvfs-core.c:1346
3. 真正发起文件 I/O 时，驱动仍然走标准 read_iter / write_iter，也就是标准 VFS / block layer 路径；它提交给文件系统的用户缓冲区其实是 cpuvaddr，不是直接
   把 GPU 地址交给 VFS。src/nvfs-core.c:1707 到 src/nvfs-core.c:1733，src/nvfs-core.c:1036 到 src/nvfs-core.c:1058
4. 到块层/NVMe 层时，nvidia-fs 注册进去的 DMA hook 会识别这些“特殊页”，把它们从 shadow page 还原成对应 GPU page，再把 scatter-gather 段改写成 GPU
   physical address / GPU DMA address。src/nvfs-mod.c:86 到 src/nvfs-mod.c:103，src/nvfs-dma.c:339 到 src/nvfs-dma.c:369，src/nvfs-dma.c:447 到 src/
   nvfs-dma.c:470，src/nvfs-mmap.c:1078 到 src/nvfs-mmap.c:1090
5. 这一步最终靠 NVIDIA 的 P2P API 完成“把 GPU 页映射到存储设备 I/O 地址空间”这件事。代码里写得很直白：This will map the GPU BAR pages into device I/O
   address space. 然后调用 nvidia_p2p_dma_map_pages()。src/nvfs-core.c:364 到 src/nvfs-core.c:400

所以从实现上看，更准确的说法是：

- GPU 内存是 DMA 的源/目的地
- 存储设备的 DMA 引擎直接对 GPU 暴露出来的 P2P/BAR 页面做 DMA
- CPU 和内核模块负责建立映射、提交 I/O、改写 SG/DMA 映射
- GPU 本身不是发 NVMe 命令的那个主体

nv-p2p.h 只是这套接口的声明入口，gds-nvidia-fs 通过它调用 nvidia_p2p_get_pages* 和 nvidia_p2p_dma_map_pages*。src/nvfs-p2p.h:26 到 src/nvfs-p2p.h:35


## gpu page table 是什么东西

• 这里的 “GPU page table” 指的不是 GPU 硬件 MMU 里的原生页表格式，而是 NVIDIA P2P API 暴露给第三方驱动的一份软件描述结构。

  具体数据结构是 nvidia_p2p_page_table_t，定义在 /tmp/nvidia-580.142-headers/nv-p2p.h:145：

```txt
  typedef struct nvidia_p2p_page_table {
      uint32_t version;
      uint32_t page_size;
      struct nvidia_p2p_page **pages;
      uint32_t entries;
      uint8_t *gpu_uuid;
      uint32_t flags;
  } nvidia_p2p_page_table_t;
```

  其中每个表项是 nvidia_p2p_page_t，定义在 /tmp/nvidia-580.142-headers/nv-p2p.h:122：

```txt
  typedef struct nvidia_p2p_page {
      uint64_t physical_address;
      union nvidia_p2p_request_registers {
          ...
      } registers;
  } nvidia_p2p_page_t;
```

  对这个仓库来说，最关键的是：

  - page_table->entries: GPU 页数量
  - page_table->page_size: 页大小，这个驱动要求必须是 64KB，见 src/nvfs-core.c:1374
  - page_table->pages[i]->physical_address: 第 i 个 GPU 页对应的物理地址，驱动直接拿它来做后续 DMA 映射和连续性判断，见 src/nvfs-core.c:1348 和 src/nvfs-
    mmap.c:1085

  在 gds-nvidia-fs 里，这个表挂在 src/nvfs-mmap.h:142 上。它来自：

  - nvidia_p2p_get_pages()
  - 或 nvidia_p2p_get_pages_persistent()

  调用位置在 src/nvfs-core.c:1316 和 src/nvfs-core.c:1334。

