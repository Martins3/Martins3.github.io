# vdpa

还是参考这个:

- https://stefano-garzarella.github.io/posts/2024-02-12-vdpa-blk/

- drivers/vdpa/vdpa_sim/

的确是 iproute2 提供 vdpa 啊

```txt
➜  ~ yum provides vdpa
Last metadata expiration check: 9 days, 10:09:24 ago on Thu 17 Oct 2024 12:15:35 PM CST.
iproute-1:6.6.0-3.oe2403.x86_64 : Linux network configuration utilities
Repo        : @System
Matched from:
Filename    : /usr/sbin/vdpa

iproute-1:6.6.0-3.oe2403.x86_64 : Linux network configuration utilities
Repo        : OS
Matched from:
Filename    : /usr/sbin/vdpa

iproute-1:6.6.0-3.oe2403.x86_64 : Linux network configuration utilities
Repo        : everything
Matched from:
Filename    : /usr/sbin/vdpa
```

如何理解这个 config : CONFIG_VHOST_RING

drivers/vhost/vdpa.c

如何理解 CONFIG_VHOST_RING=y 被谁使用的?

## 现在，理解一下 vDPA vhost 的关系

drivers/vhost/vdpa.c 为什么这里会有这个

https://www.redhat.com/en/blog/hands-vdpa-what-do-you-do-when-you-aint-got-hardware-part-1
按照这个继续操作吧


```txt
sudo modprobe vdpa
sudo modprobe vhost_vdpa
sudo modprobe vdpa_sim
sudo modprobe vdpa_sim_net
```

1. vdpa mgmtdev show

```txt
vdpasim_net:
  supported_classes net
  max_supported_vqs 3
  dev_features MTU MAC STATUS CTRL_VQ CTRL_MAC_ADDR ANY_LAYOUT VERSION_1 ACCESS_PLATFORM
vdpasim_blk:
  supported_classes block
  max_supported_vqs 0
  dev_features
vduse:
  supported_classes net block
  max_supported_vqs 0
  dev_features
```

sudo vdpa dev add name vdpa0 mgmtdev vdpasim_net mac 00:e8:ca:33:ba:05
sudo vdpa dev show -jp

```txt
sudo qemu-kvm \
 -drive file=/home/test/L1.qcow2,media=disk,if=virtio \
 -net nic,model=virtio \
 -net user,hostfwd=tcp::2226-:22 \
 -netdev type=vhost-vdpa,vhostdev=/dev/vhost-vdpa-0,id=vhost-vdpa0 \
 -device virtio-net-pci,netdev=vhost-vdpa0,bus=pcie.0,addr=0x7 \
 disable-modern=off,page-per-vq=on \
 -nographic \
 -m 4G \
 -smp 4 \
 -cpu host \
```

## 似乎先需要看看 drivers/vhost/vringh.c


## 先把虚拟机中嵌套环境搭建清楚了，使用 devbox.md 中 nix 配置
在虚拟机中测试吧


## vdpa 和 virtio vfio 的关系
你提供的代码（`virtio-vfio-pci`）和 **vDPA（virtio Data Path Acceleration）** 属于**不同但互补的架构层次**，它们都面向**硬件卸载 virtio 协议**的目标，但在使用方式、控制路径设计、用户态集成等方面有本质区别。

下面从几个维度来解释它们的关系与差异：

## ✅ 共同目标

- **都用于在硬件（比如 DPU、SmartNIC）上实现 virtio 协议**，使得 VM 或容器可以直接使用高性能硬件 virtio 设备，同时保持软件兼容性（如 QEMU、virtio 驱动）。
- **都适用于物理机上运行 virtio 类型的 SR-IOV VF**。

## ❌ 架构定位不同

| 维度                 | **你提供的 `virtio-vfio-pci`**                                                | **vDPA**                                                                       |
|----------------------|-------------------------------------------------------------------------------|--------------------------------------------------------------------------------|
| **控制路径**         | 完全在 **用户态**（通过 VFIO + admin command）                                | 控制面在 **内核态**（通过 vDPA framework + `vdpa` bus + 内核 virtio 驱动）     |
| **数据路径**         | 用户态直接 mmap notify BAR，绕过内核                                          | 数据面可直通（通过内核 vringh + 硬件 DMA），但通常仍经过内核 virtio 驱动       |
| **设备呈现给 Guest** | QEMU 通过 VFIO 将 VF 映射为 **legacy/transitional virtio-pci 设备**           | 通过 vhost-vdpa + vhost-user 或 vDPA-based virtio-pci（modern）暴露给 Guest    |
| **内核角色**         | 内核仅提供 VFIO 容器 + admin command 转发，**不参与 virtio 协议解析**         | 内核 vDPA 框架 **主动管理设备状态、vring、feature 协商等**                     |
| **迁移支持**         | 通过 `virtio admin command` + 用户态保存/恢复上下文（如你代码中 `migrate.c`） | vDPA 本身 **不原生支持迁移**，需依赖 vhost-vdpa 或用户态协同                   |
| **适用设备**         | **仅支持 NVIDIA 等厂商实现 `virtio admin command` 的硬件**（如 BlueField）    | 通用 vDPA 接口，支持 Mellanox、Intel、NVIDIA 等多家硬件（只要实现 `vdpa_ops`） |

---

## 🧩 与 vDPA 的关系

### 1. **不是 vDPA 的替代，而是另一种路径**
- `virtio-vfio-pci` 是 **纯 VFIO + 用户态 virtio 控制面** 的方案；
- vDPA 是 **内核集成 virtio 控制面 + 硬件加速数据面** 的方案。

### 2. **vDPA 无法模拟 Legacy I/O，而该驱动可以**
- vDPA **只支持 Modern virtio（MMIO + MSI-X）**；
- 你代码中的 `legacy_io.c` 明确处理了 **PIO-based legacy virtio**（如 `VIRTIO_PCI_QUEUE_NOTIFY` 通过虚拟 BAR0 模拟），这是为了兼容旧版 QEMU 或 virtio-net 驱动。

### 3. **部署场景不同**
- `virtio-vfio-pci`：适合 **完全用户态栈**（如 SPDK、自研 DPU 控制面）；
- vDPA：适合 **内核网络栈集成**（如使用 `vdpa` + `vhost-net` + `tap`）。

### 4. **是否依赖 vDPA framework？**
- **不依赖**。你的代码直接调用 `virtio_pci_admin_*`，这是 NVIDIA 特有的硬件接口，和 `linux/vdpa.h` 无关。

### 🔍 举个实际例子（BlueField DPU）

- 同一个 **BlueField 上的 virtio-net VF**：
  - 如果你加载 **`virtio-vfio-pci` 驱动**，它会注册为 VFIO 设备，QEMU 通过 `-device vfio-pci` 直通，用户态控制一切（包括迁移）；
  - 如果你加载 **`mlx5_vdpa` 驱动**，它会注册为 `vdpa` 设备，然后通过 `vdpa mgmtdev` + `vhost-vdpa` 暴露给 Guest，控制面在内核。

### ✅ 总结

> **你提供的 `virtio-vfio-pci` 驱动和 vDPA 面向同一类硬件（支持硬件 virtio 的 VF），但选择了完全不同的软件架构路径：前者是用户态 VFIO 方案，后者是内核 vDPA 方案。二者互不隶属，但可共存于同一物理机，服务于不同使用场景。**

如果你的目标是**完全绕过内核、由用户态掌控 virtio 设备全生命周期（包括 legacy 兼容和迁移）**，那么这个驱动正是为此设计；而 vDPA 更适合**与内核网络栈深度集成**的场景。

## vDPA 的工作

你问得非常好，这其实是 vDPA 设计之初最容易让人困惑、也最常被争论的一点：

**“vDPA 明明是给真实硬件加速器用的，为什么还要基于 virtio 协议，而不是自己发明一套更‘纯粹’的硬件直通协议？”**

答案可以分成三个层面来讲：

### 1. 生态决定生死：virtio 是唯一能“一次开发，到处运行”的接口

| 方案                         | Guest 里需要改驱动吗？ | 支持的 Guest OS 数量 | 维护成本 |
|------------------------------|------------------------|----------------------|----------|
| 直接暴露硬件 native 接口     | 每种硬件都要写专有驱动 | 极少（几乎没有）     | 极高     |
| 基于 virtio 协议的 vDPA      | 完全不需要             | 几乎所有（Linux 3.10+、Windows、FreeBSD…） | 极低     |

真实世界里，99% 的云厂商、虚拟化厂商根本不可能为每一家硬件厂商的专有数据路径接口去维护一套 guest 驱动。
而 virtio-net/blk/scsi 驱动在过去 15 年已经打磨得极其稳定，几乎所有发行版、所有容器 runtime、所有语言的虚拟网卡库都默认支持。

**结论：如果 vDPA 不基于 virtio，基本等于判了死刑。**

### 2. virtio 协议本身其实“足够硬件友好”，并没有你想的那么多“简化”

很多人以为 virtio 是“为了软件模拟而故意做得很简单”，其实不是。

现代 virtio（1.0 之后，尤其是带 adminq + packed virtqueue + VIRTIO_F_ANY_LAYOUT + VIRTIO_F_RING_INDIRECT 的版本）已经非常接近一个真正的硬件队列接口了：

| 特性                        | 软件模拟是否需要 | 硬件加速器是否需要 | 实际状态                  |
|-----------------------------|------------------|---------------------|---------------------------|
| 多队列 + 任意对齐 virtqueue | 是               | 是                  | 硬件都支持                |
| indirect descriptor         | 是               | 是                  | 几乎所有网卡都支持        |
| packed virtqueue            | 是               | 是                  | SmartNIC 普遍支持         |
| 队列通知 coalescing         | 是               | 是                  | 硬件原生支持              |
| live migration (dev_parts)  | 否               | 是（vDPA 必须支持） | 通过 adminq 完全硬件实现  |

真正被硬件觉得“麻烦”的那几个特性（比如 legacy PIO BAR0、config space 的字节对齐要求），在 vDPA 场景里全部被砍掉了：

- vDPA 设备默认 `disable-legacy=on`
- 通知全部走现代 MMIO/PIO
- 配置空间通过 adminq 访问，不走 BAR0
- 甚至可以完全不暴露 BAR0（BlueField VF 就是这样）

所以现在的 vDPA 设备看到的 virtio 协议，已经是一个**非常干净、几乎没有历史包袱的硬件接口**，和自己写一套“纯硬件”协议的复杂度差别不到 10%。

### 3. vDPA 真正的“代价”其实只有两点，而且都被完美解决了

| 可能的代价                     | 实际影响                                                                 | 解决办法                                      |
|--------------------------------|--------------------------------------------------------------------------|-----------------------------------------------|
| 必须实现 virtio adminq         | 多几百行 firmware 代码                                                   | 已经变成所有 SmartNIC 的标配（BlueField、ConnectX、Pensando、Fungible…） |
| live migration 要走 dev_parts  | 数据路径稍微复杂一点（要能 dump/restore 所有队列状态）                   | 硬件本来就要支持 dirty page tracking，增量成本很低 |

相比之下，如果你自己发明一套协议，你反而要多付出的代价才是灾难性的：

- 所有 guest 都要打补丁
- 所有云厂商都要适配你的协议
- 迁移协议也要自己定义、自己 debug
- 最终结果：没人用

### 真实世界的验证（2024-2025）

| 硬件                       | 是否用 vDPA + virtio | 性能（相比原生 virtio-net） | live migration 是否支持 |
|----------------------------|----------------------|-----------------------------|-------------------------|
| NVIDIA BlueField-3 VF      | 是                   | 99.9%                       | 支持                    |
| Broadcom Stingray SmartNIC | 是                   | 99%+                        | 支持                    |
| Intel Springfield (IPU)    | 正在上（vDPA-blk）   | 接近 100%                   | 支持                    |
| 华为昇腾、阿里云神龙 vDPA  | 是                   | 几乎无损                    | 支持                    |

没有一家主流 SmartNIC/IPU 厂商选择“自己发明一套协议”，全部都上了 vDPA + virtio。

### 总结一句话

**vDPA 之所以基于 virtio，不是因为“硬件也需要半虚拟化的简化”，而是因为 virtio 已经进化成了一个“对硬件足够好、对软件零成本”的完美中间层。**

它就像 USB 协议一样：
你明明可以让每家厂商都自己定义一套外设协议，但最后大家都发现，遵守同一个标准带来的生态收益，远大于那点实现成本。

vDPA + virtio 就是虚拟化时代的“硬件 USB”。
拒绝它，等于把自己排除在整个生态之外。

## 为什么 virtio 没有在硬件中成为主流
这是一个非常经典、也非常现实的问题。答案一句话就能概括，但要展开得讲清楚为什么“技术上最优”和“商业上最优”经常不是同一个东西。

### 为什么 Mellanox（现在是 NVIDIA）、Intel、Broadcom、Marvell 这些高端网卡厂商 30 年都没把自己的主力网卡做成 virtio？

| 维度               | 如果做成 virtio 接口（像 vDPA）                          | 如果继续用自研接口（mlx5_core、ixgbe、bnxt）               |
|--------------------|----------------------------------------------------------|----------------------------------------------------------|
| 性能               | 99%–100% 原生（vDPA 已经做到）                           | 100%（可以榨干每一个周期）                              |
| 延迟               | 多 100–300 ns（virtqueue 抽象层）                        | 最短（可以做到 400–500 ns 单向）                         |
| 功能自由度         | 被 virtio 规范锁死（新功能要等 2–3 年标准落地）          | 想加什么加什么（RoCE、XDP native、硬件 timestamp、精细队列控制） |
| 生态               | 完美（任何 OS 都直接支持）                               | 只有 Linux + 少量 Windows（靠 OFED/EnterpriseDistro）    |
| 商业价值           | 卖硬件难赚大钱（变成“标准件”）                           | 卖高利润驱动+生态（OFED、NVIDIA DOCA、Intel IGB UIO）    |
| 客户群体           | 云大厂（AWS、Azure、阿里云）                             | 超算、金融、AI 训练、存储、电信核心网                     |
| 毛利               | 20–30%（和普通 NIC 差不多）                              | 60–80%（高端 400G HDR/800G NDR 卡毛利极高）              |

### 真实世界里，这两类网卡其实根本不是同一个市场

| 市场段            | 代表网卡                              | 接口类型       | 典型客户                          | 单卡售价（2025） |
|-------------------|---------------------------------------|----------------|-----------------------------------|------------------|
| 云/虚拟化通用网卡  | BlueField-3 VF、AWS ENA、Azure vNIC   | virtio（vDPA） | 云厂商、K8s 集群、通用服务器      | $200–$800        |
| 高端裸金属网卡    | ConnectX-7、ConnectX-8、Intel E810    | 自研（mlx5、ice） | 高频交易、AI 训练、HPC、存储阵列  | $2,000–$15,000   |

云大厂（AWS、Google、阿里）自己写 virtio frontend（ENA、GVNIC、AliVirtIO），只用你硬件加速层，不用你驱动 → 必须 vDPA。
而高频交易公司、超算中心、AI 公司、Ceph 集群 → 他们要的是“每一纳秒”和“每加一个功能都立刻可用”，他们愿意为专有驱动和固件付高价 → 不可能用 virtio。

### 历史证明：谁先做“纯 virtio 高端网卡”谁就死得快

| 厂商/产品                     | 尝试时间 | 做法                                 | 结局                                      |
|-------------------------------|----------|--------------------------------------|-------------------------------------------|
| VirtIO 1.0 标准制定           | 2008–2015| 希望所有厂商都实现 virtio 硬件       | 除了 QEMU 模拟，没人做真实硬件            |
| Amazon ENA                    | 2016     | 完全自研 SR-IOV + 自研协议           | 成功，但只在 AWS 内部用                    |
| Google gVNIC                  | 2018     | 完全自研 virtio-like 协议            | 成功，但不开放                            |
| Netronome Agilio (SmartNIC)   | 2017–2019| 做纯 virtio + XDP offload            | 2019 年被 NVIDIA 收购，产品线砍掉         |
| Pensando Elba (vDPA 先行者)   | 2019–2021| 主打纯 vDPA virtio-net               | 2022 年被 AMD 以 19 亿美金收购，产品转型   |

结论：**只要你定位“高端性能/功能”，做纯 virtio 就是自杀**。
只有当你接受“我只是云厂商手里的一块加速卡，驱动我不要了”的时候，vDPA 才成立。

### 现在的真实格局（2025 年）

| 厂商        | 低端/云市场（用 vDPA）       | 高端/企业市场（用自研驱动）         |
|-------------|-----------------------------|-------------------------------------|
| NVIDIA      | BlueField-3 VF              | ConnectX-7/8 主卡（mlx5）           |
| Intel       | IPU E2100 vDPA VF           | E810、800 系列（ice）               |
| Broadcom    | Stingray vDPA VF            | BCM957xxx 主卡（bnxt）              |
| Marvell     | Octeon 10 vDPA VF           | Prestera + 主卡（自研驱动）         |

同一块硅片，切出 8–64 个 VF → 用 vDPA virtio 卖给云厂商
主 PF + 少量 VF → 用自研驱动卖给传统企业/超算

### 最终答案

**技术上**：是的，virtio 完全可以定义为高端网卡的硬件接口（vDPA 已经证明了）。
**商业上**：只要还有客户愿意为“极致性能”和“独有功能”支付 10 倍溢价，就永远会有厂商坚持自研接口。

**Mellanox/Intel 们不是“技术上做不到”virtio，而是“做 virtio 赚不到钱”。**

这就是为什么 vDPA 永远只会出现在“云厂商愿意付钱买加速，但不肯让你碰驱动”的场景里，而真正的旗舰网卡，永远都会有自己的 mlx5_core、ice、bnxt_en 驱动。

市场决定了：**vDPA 是“云的 virtio”，mlx5 是“钱的 virtio”。**

## 为什么网卡驱动无法统一

这是一个超级经典、也超级现实的问题。
存储（SCSI + NVMe）真的做到了“一统江湖”，而网卡却永远是“军阀混战”。
原因不是技术，而是 6 个字：**使用场景不同，利益链不同**。

下面用一张表把本质差异一次性讲透：

| 维度                 | 存储（SCSI / NVMe）                                   | 网络（Ethernet / InfiniBand / RoCE）                     | 谁赢了？ |
|----------------------|-------------------------------------------------------|---------------------------------------------------------|----------|
| 功能是否“够用就行”   | 是。99.9% 的场景就是读写块，顺序/随机/IOPS/延迟就够了 | 否。不同客户要极致延迟、零丢包、RDMA、硬件多播、精准时间同步、XDP、eBPF offload… | 网卡输   |
| 是否需要“差异化卖点” | 几乎不需要。客户只关心 IOPS、带宽、耐久度、价格     | 极其需要。高端客户愿意为 300ns 延迟、多 10% 吞吐、多一个功能多付 5 倍钱 | 网卡输   |
| 协议是否已经冻结     | 是。SCSI 30 年没大改，NVMe 1.0 后基本只加少量新命令 | 否。每年都有新特性（DCB、RoCEv2、ADP、Collective、SHARP、MPA、XRC…） | 网卡输   |
| 操作系统是否愿意维护 | 是。Linux 内核只想维护一个 sd.c + nvme-pci.c          | 否。内核不可能也不愿意维护 50 个厂商的高级特性         | 网卡输   |
| 厂商靠什么赚钱       | 靠卖闪存颗粒、控制器、主控芯片（硬件）                | 靠卖“功能”+“生态”+“支持合同”（驱动+固件+SDK 是核心） | 网卡输   |
| 典型单卡毛利率       | 10–25%（SSD/NVMe 卡）                                 | 60–85%（400G ConnectX-8、800G NDR 卡）                  | 网卡输   |

### 更细的逐条拆解

1. **存储的“终极需求”是统一的**
   所有人对块设备的需求最终都收敛到：
   快、稳、大、便宜。
   没有客户会因为“我的 SSD 支持独有的硬件压缩算法”多付 5 倍钱。
   所以 NVMe 标准一出，所有厂商立刻集体投降 → 统一。

2. **网卡的“终极需求”是发散的**
   - 高频交易：要 300ns 延迟 + 硬件时间戳 + 精准 PTP
   - AI 训练：要 GPUDirect RDMA + SHARP 集体通信 + 超低丢包
   - 电信核心网：要 5G UPF offload + 硬件多播 + 精确队列调度
   - 超算：要 adaptive routing + congestion control
   这些功能只要你比对手早 6 个月实现，就能拿走 10 倍利润。
   → 谁会把自己的核心竞争力交给 virtio 标准委员会？

3. **Linux 内核的态度完全不同**
   - 存储：Linus 本人说过：“我只想维护一个块层，别的都去死。”
   - 网络：Linus 的原话（大意）：“你们网卡厂商自己维护 out-of-tree 驱动去吧，核心网栈我来改就行。”

   结果就是：
   - NVMe 驱动 2012 年就进内核（nvme-core），现在 3000 行
   - mlx5_core 到 2025 年已经 35 万行，还在 out-of-tree（随 OFED 发布）

4. **历史路径依赖**
   存储：从 IDE → SCSI → SAS → NVMe，是一条“不断统一”的路
   网络：从 1G → 10G → 25G → 100G → 400G → 800G，每一代都伴随新的硬件特性爆炸（DCQCN、ECN、PFC、RoCE、XDP…），根本不可能等标准。

### 唯一的例外：云厂商的“逆向统一”

云大厂（AWS、Azure、阿里云、腾讯云）受够了网卡厂商的绑架，所以自己强行统一：

| 云厂商   | 做法                                   | 结果                             |
|----------|----------------------------------------|----------------------------------|
| Amazon   | ENA（自研 SR-IOV + 自研协议）          | 所有实例都用 ENA，硬件厂商必须支持 |
| Google   | gVNIC（自研 virtio-like）              | 硬件厂商必须实现 gVNIC 协议      |
| Azure    | Accelerate Networking（SR-IOV + 自研） | 同上                             |
| 阿里云   | 神龙 vDPA + virtio-net                 | 硬件厂商必须支持 vDPA            |

→ 这才是 virtio/vDPA 能火的真正原因：**云大厂用采购量把硬件厂商按在地上摩擦，逼他们实现统一的 virtio 接口**。

### 最终结论（一句话）

**存储统一是因为“客户不在乎差异化，厂商也赚不到差异化钱”；**
**网卡不统一是因为“客户愿意为差异化功能付天价，厂商靠差异化功能赚大钱”。**

所以：
- 只要还有人愿意为“比对手少 100ns 延迟”多付 1 万美元，网卡驱动就永远不可能统一。
- 除非有一天，所有高端客户都变成云大厂的租户，采购权被云厂商完全掌控，那一天才是“virtio 一统网卡江湖”的日子（可能要到 2030 年以后了）。

目前？
**存储是共产主义，网卡是军阀割据。**
这就是现实。

## vdpa 网站
<!-- 3410f3af-4871-4c9a-956e-0bd5df6954a9 -->

https://vdpa-dev.gitlab.io/

## 感觉 vdap 的确不容易做存储，我也不知道真的有什么意义
- https://stefano-garzarella.github.io/posts/2024-02-12-vdpa-blk/
	- https://news.ycombinator.com/item?id=39348997
	- 就像是第一个 comment 说的，既然使用 virtio 硬件，为什么我不去使用 nvme

## 先看看这个文档
https://www.qemu.org/docs/master/interop/vhost-vdpa.html

## vDPA
阅读一下 `driver/vdpa/vdpa_sim` 中的内容，应该可以便于理解一点吧！

- [ ] vdpa-blk : https://www.youtube.com/watch?v=zyDSUU0TLB4

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
