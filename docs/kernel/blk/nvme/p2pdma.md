# NVMe 与 PCI P2PDMA

notice : 2026-07-06 大部分是 codex 结合 7.1.2 源码分析的，目前没发现任何问题

本文解释三个容易混在一起的问题：

1. 为什么 NVMe 驱动需要显式支持 PCI P2PDMA；
2. Controller Memory Buffer（CMB）在其中解决了什么问题；
3. Linux NVMe target 如何让 RNIC 和后端 NVMe SSD 通过 CMB 搬运数据。

先给出结论：

- NVMe 协议并不天然等于 P2PDMA。Linux NVMe PCI 驱动必须特殊识别 P2P page、检查 PCIe 拓扑并为 PRP/SGL 填入正确的 PCI bus address，才能安全接收这类 block request。
- CMB 是 NVMe 控制器通过 PCI BAR 暴露的一块内存。在经典 NVMe-oF target P2P 路径中，它是 RNIC 和后端 SSD 都能访问的中转 buffer。
- CMB 不是所有 NVMe P2PDMA 的硬性要求。只要存在另一块同时可被两个 client 访问的 P2P memory，例如 RNIC 自己发布的 BAR memory，也能承担相同角色。
- “CMB 存放 SQE”和“CMB 存放 I/O payload”是两种独立用途，不能混为一谈。

## 为什么需要一块 CMB

SSD 的 NAND/namespace 不是 PCIe 总线上可由 RNIC 直接寻址的一段内存。NVMe SSD 的正常工作模型仍是：host 提交一个 NVMe command，并通过 PRP/SGL 告诉控制器从哪个 buffer 读数据或把数据写到哪个 buffer。

因此，RNIC 与 NVMe SSD 之间需要一块同时满足以下条件的 buffer：

- 它具有 PCIe bus address，能成为 DMA 目标或源；
- RNIC 可以访问它；
- 后端 NVMe controller 也可以访问它；
- Linux 能管理它的分配、生命周期和 PCIe 拓扑兼容性。

CMB 正好是 NVMe controller 在 BAR 中暴露的 controller-local memory，因此适合作为这块 rendezvous/staging buffer。它不是持久化介质，也不是 page cache；一次请求完成后就可以回收复用。

不过，P2PDMA 的抽象并不要求 provider 必须是 NVMe CMB。角色可以概括为：

| 角色 | NVMe/RDMA target 中的实现 |
| --- | --- |
| provider | 通常是发布 CMB 的 NVMe PCI device |
| client 1 | RNIC，它对 CMB 执行 RDMA DMA |
| client 2 | 后端 NVMe controller，它以 CMB page 作为 I/O buffer |
| orchestrator | `nvmet`，选择 provider、分配 buffer 并把同一组 page 交给两端 |

如果 RNIC 或其他 PCIe 设备能够发布合适的 P2P memory，也可以替代 CMB。所谓“需要 CMB”，准确含义是当前经典部署需要一个共同可达的 P2P memory provider，而 CMB 是已有内核实现最自然的 provider。

## CMB 有两种不同用途

NVMe `CMBSZ` 寄存器通过不同 capability bit 描述 CMB 可以保存什么：

- `SQS`：Submission Queue entries；
- `CQS`：Completion Queue entries；
- `LISTS`：PRP/SGL lists；
- `RDS`：Read Data；
- `WDS`：Write Data。

Linux NVMe PCI 驱动中的两条路径彼此独立。

### 用 CMB 保存 SQE

当 `use_cmb_sqes=Y` 且 CMB 声明 `SQS` 时，`nvme_alloc_sq_cmds()` 尝试从 CMB 分配 I/O submission queue。这样 controller 不必再从 system RAM DMA fetch SQE。

这只是 queue placement 优化，不代表 I/O payload 使用了 P2PDMA。空间不足或不适用时驱动会回退到 `dma_alloc_coherent()` 分配的 host memory。

### 把 CMB 发布成 payload P2P memory

`nvme_map_cmb()` 读取 `CMBLOC/CMBSZ`，定位 CMB 所在 BAR、offset 和 size，然后调用：

```c
pci_p2pdma_add_resource(pdev, bar, size, offset);
```

只有 CMB 同时具有 `WDS` 和 `RDS`，驱动才调用：

```c
pci_p2pmem_publish(pdev, true);
```

原因是通用 I/O buffer 必须既可能被 device 读，也可能被 device 写。只有 `SQS` 的小 CMB 可以放 SQE，却不能作为 nvmet payload buffer。

## 为什么 NVMe 驱动必须特殊支持 P2PDMA

P2P memory 虽然可通过 `ZONE_DEVICE` 建立 `struct page`，其本质仍是 PCI BAR/MMIO，而不是普通 RAM。普通 block/DMA 路径不能直接假设：

- page 可以被 CPU `memcpy`；
- `page_to_phys()` 得到的地址就是 client 应使用的 DMA address；
- 任意两个 PCIe endpoint 之间都能路由 TLP；
- IOMMU、ACS 和 host bridge 会像 system RAM DMA 一样工作；
- provider 被移除时其他设备已经停止访问其 BAR。

所以支持必须从 block queue 一直贯穿到具体的 DMA driver，而不是仅在通用 block 层打开一个开关。

### 1. 向 block layer 声明能力

NVMe core 创建 namespace queue 时，通过 transport 的 `supports_pci_p2pdma()` 判断能力，并设置：

```c
lim.features |= BLK_FEAT_PCI_P2PDMA;
```

有这个 feature 后，block layer 才允许把 P2P pages 放入该 queue 的 bio/request。block layer 还会阻止不兼容的 P2P range 被错误合并；nvmet 对 P2P bio 设置 `REQ_NOMERGE`。

这里的含义是“driver 知道如何处理 P2P buffer”，不是“这个 NVMe device 与机器中所有其他 PCIe device 都可以 P2P”。每次实际组合仍需要拓扑检查。

### 2. 为 NVMe command 构造正确的 PRP/SGL

`nvme_map_data()` 不能把 P2P page 当普通 RAM 快速映射。当前实现通过 block DMA iterator 识别三种结果：

- `PCI_P2PDMA_MAP_BUS_ADDR`：使用 provider 对 client 可见的 PCI bus address；
- `PCI_P2PDMA_MAP_THRU_HOST_BRIDGE`：平台允许 TLP 经过 host bridge，按 MMIO DMA 处理；
- `PCI_P2PDMA_MAP_NOT_SUPPORTED`：拓扑或平台不支持，请求失败。

映射成功后，NVMe PCI 驱动把地址填入 command 的 PRP 或 SGL descriptor。controller 随后直接对 peer BAR memory 发起 DMA。unmap 路径也必须记住原来的 P2P map type，不能套用普通 RAM 的 unmap 假设。

NVMe 很适合实现这一能力，是因为它的数据面本来就通过 PRP/SGL 描述 DMA buffer，并且正常 I/O 不要求 CPU 读取 payload。但它并非唯一能支持 P2PDMA 的 block driver；其他驱动若实现同样的 page、DMA 和生命周期规则，也可以声明 `BLK_FEAT_PCI_P2PDMA`。

## Linux NVMe/RDMA target 的工作流程

### 1. NVMe PCI 驱动注册 provider

设备 probe/reset 过程中，`nvme_map_cmb()`：

1. 读取 `CMBLOC`、`CMBSZ`；
2. 计算 BAR、offset、size；
3. 检查 BAR 范围以及 `memremap_compat_align()` 对齐；
4. 对 NVMe 1.4+ controller，通过 `CMBMSC` 告诉 controller host 侧的 CMB address mapping 并启用 decode；
5. 用 `pci_p2pdma_add_resource()` 注册 CMB；
6. 若同时支持 RDS/WDS，用 `pci_p2pmem_publish()` 发布为候选 provider。

PCI P2PDMA 层为这段 BAR memory 建立 `MEMORY_DEVICE_PCI_P2PDMA` page，并用 genpool 管理分配。

### 2. 管理员为 namespace 启用 p2pmem

nvmet 的 configfs namespace 属性是：

```txt
/sys/kernel/config/nvmet/subsystems/<subsys>/namespaces/<nsid>/p2pmem
```

namespace 必须先处于 disabled 状态。写入值的含义是：

```bash
# 自动选择同时兼容 RNIC 和后端 block device 的 provider
echo 1 > p2pmem

# 固定使用指定 PCI device 发布的 provider
echo 0000:5e:00.0 > p2pmem

# 禁用
echo 0 > p2pmem
```

启用 namespace 时，`nvmet_p2pmem_ns_enable()` 首先要求：

- namespace 后端是 block device，而不是 file namespace；
- 后端 queue 声明了 `BLK_FEAT_PCI_P2PDMA`；
- 指定 provider 与后端兼容，或至少能找到一个可用 provider。

RDMA queue 建立时，`nvmet-rdma` 只有在 RNIC 的 DMA API 支持 PCI P2P 时才设置 `req->p2p_client`。随后 nvmet 将 RNIC 和后端 NVMe device 一起交给 `pci_p2pmem_find_many()`，选择距离较近且两边都可达的 provider，并记录 namespace 到 provider 的映射。

因此，当前这条 nvmet P2P 数据路径针对 RDMA transport；TCP transport 没有一个能够对 CMB 发起 PCI P2P DMA 的 RNIC client，不能仅通过打开 `p2pmem` 获得同样效果。

### 3. 为每个 request 分配 CMB pages

`nvmet_req_alloc_sgls()` 根据 namespace 映射找到 provider，然后通过：

```c
pci_p2pmem_alloc_sgl()
```

从 CMB 分配 payload scatterlist。分配失败时当前代码会回退到普通 `sgl_alloc()` host pages，因此启用了配置不等于每个请求都必然命中 P2P buffer。

### 4. 两个 client 使用同一组 page

以 host 向 namespace 写入为例：

1. `nvmet-rdma` 把 CMB scatterlist 映射给 RNIC；
2. RNIC 将远端数据直接 DMA 到 CMB；
3. nvmet 不复制 payload，而是把同一 scatterlist 加入后端 bio；
4. block layer 把 bio 交给后端 NVMe queue；
5. `nvme-pci` 识别 P2P page，为后端 controller 构造指向 CMB 的 PRP/SGL；
6. 后端 controller 从 CMB DMA read 并写入介质；
7. request 完成后，nvmet 通过 `pci_p2pmem_free_sgl()` 回收 CMB 空间。

读请求则是后端 controller 先把数据 DMA write 到 CMB，RNIC 再从 CMB DMA read 到远端。

```txt
                       nvmet（orchestrator）
                   选择 provider / 分配 SGL
                              |
             +----------------+----------------+
             |                                 |
             v                                 v
        RNIC（client） <---- CMB ----> backend NVMe（client）
                              ^
                              |
                    NVMe PCI（provider）
```

## 拓扑为什么是硬约束

PCIe 规范能明确保证的是同一 PCIe hierarchy 内的路由。若 TLP 到达 root port/host bridge，是否能 hairpin 到另一个 endpoint 取决于 CPU SoC、host bridge、IOMMU 和 ACS 配置，不能靠 endpoint capability 推断。

Linux 使用 `pci_p2pdma_distance()` / `pci_p2pmem_find_many()` 检查 provider 到所有 client 的路径。常见的可靠部署是 RNIC 与 NVMe 位于同一 root port 下，必要时经过同一个 PCIe switch。跨 root port 只有在架构原生支持或 host bridge 位于内核允许范围时才会通过检查。

所以以下条件缺一不可：

- kernel 开启 `CONFIG_PCI_P2PDMA`（它依赖 64-bit 和 `ZONE_DEVICE`）；
- NVMe controller 确实有足够大的 CMB；
- CMB 具有 RDS/WDS，能被发布为 data provider；
- RNIC 的 RDMA driver 支持 PCI P2P DMA；
- 后端 block queue 支持 `BLK_FEAT_PCI_P2PDMA`；
- provider、RNIC、后端 NVMe 的 PCIe 拓扑兼容；
- CMB BAR range 满足 P2PDMA/`devm_memremap_pages()` 的对齐要求。

“设备规格写着支持 CMB”仍可能不可用。例如 512 KiB CMB 可能不满足平台通常为 2 MiB 的 memory hotplug 对齐要求，`nvme_map_cmb()` 会跳过注册。

## 如何观察和排查

### CMB 是否存在以及支持什么

```bash
cat /sys/class/nvme/nvme0/cmb
cat /sys/class/nvme/nvme0/cmbloc
cat /sys/class/nvme/nvme0/cmbsz
cat /sys/module/nvme/parameters/use_cmb_sqes
```

`cmbsz` 需要按 `include/linux/nvme.h` 中的 `NVME_CMBSZ_*` 位解析。看到 sysfs 文件只说明驱动识别了 CMB；还要确认 RDS/WDS、注册成功和拓扑兼容。

### 内核和 PCIe 拓扑

```bash
grep CONFIG_PCI_P2PDMA /boot/config-$(uname -r)
lspci -tv
lspci -vv -s <RNIC-BDF>
lspci -vv -s <NVMe-BDF>
dmesg | grep -Ei 'p2p|peer-to-peer|CMB'
```

nvmet 成功建立映射时会打印类似：

```txt
using p2pmem on 0000:5e:00.0 for nsid 1
```

常见失败信息包括后端 driver 不支持 P2PDMA、找不到同时兼容两个 client 的 provider、指定设备没有已发布的 P2P memory。

### 不能只用吞吐量证明 P2P 生效

启用后请求可能因 CMB 空间不足回退到普通 pages。验证时至少应结合：

- configfs `p2pmem` 当前值；
- nvmet 的 provider 选择日志；
- PCIe topology；
- system memory bandwidth/uncore counter 对比；
- 必要时在 `nvmet_req_alloc_p2pmem_sgls()`、`nvme_map_data()` 或 P2PDMA trace point/kprobe 上统计实际请求。

## 关键代码位置

| 代码 | 作用 |
| --- | --- |
| `Documentation/driver-api/pci/p2pdma.rst` | P2PDMA 的 provider/client/orchestrator 模型与拓扑规则 |
| `drivers/pci/p2pdma.c` | provider 注册、拓扑检查、P2P memory 分配 |
| `drivers/nvme/host/pci.c:nvme_map_cmb()` | CMB 探测、注册和发布 |
| `drivers/nvme/host/pci.c:nvme_alloc_sq_cmds()` | 将 I/O SQ 放入 CMB；与 payload P2P 是独立用途 |
| `drivers/nvme/host/pci.c:nvme_map_data()` | 将 P2P request 映射成 NVMe PRP/SGL |
| `drivers/nvme/host/core.c:nvme_alloc_ns()` | 为支持的 namespace queue 设置 `BLK_FEAT_PCI_P2PDMA` |
| `drivers/nvme/target/configfs.c:nvmet_ns_p2pmem_store()` | `p2pmem` 配置入口 |
| `drivers/nvme/target/core.c:nvmet_p2pmem_ns_enable()` | 检查 namespace 与 provider |
| `drivers/nvme/target/core.c:nvmet_req_alloc_sgls()` | 每个请求优先从 P2P provider 分配 SGL |
| `drivers/nvme/target/rdma.c:nvmet_rdma_alloc_rsp()` | 将支持 P2P 的 RNIC 标记为 client |
| `drivers/nvme/target/io-cmd-bdev.c:nvmet_bdev_execute_rw()` | 把 P2P pages 原样加入后端 bio |
| `block/blk-mq-dma.c` | block request 的普通 RAM/P2P/MMIO DMA iterator |

## 历史线索

2018 年合入的两组改动奠定了当前模型：

- `0f238ff5cc92 (nvme-pci: Use PCI p2pmem subsystem to manage the CMB)`：把 CMB 注册到 PCI P2PDMA 子系统，并在具有 RDS/WDS 时发布；
- `e0596ab2900d (nvme-pci: Add support for P2P memory in requests)`：让 NVMe request 接受 P2P pages，并向 block layer 声明能力。

后续实现持续收紧映射、DMA API 和对齐约束。例如：

- `56cf7ef0d490 (nvme-pci: skip CMB blocks incompatible with PCI P2P DMA)`：跳过不满足 memory hotplug/P2PDMA 对齐要求的 CMB；
- `23528aa3320a (nvme: enable PCI P2PDMA support for RDMA transport)`：让 NVMe/RDMA host transport 也能按 RDMA device 能力声明 P2PDMA。

因此 P2PDMA 不是简单地“把 DMA 地址换成另一个设备的 BAR 地址”，而是一条由 memory model、block layer、DMA API、NVMe PRP/SGL、RDMA 和 PCIe topology 共同约束的数据路径。

## 参考

- `Documentation/driver-api/pci/p2pdma.rst`
- `Documentation/mm/memory-model.rst`
- `drivers/pci/p2pdma.c`
- `drivers/nvme/host/pci.c`
- `drivers/nvme/target/core.c`
- `drivers/nvme/target/rdma.c`

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
