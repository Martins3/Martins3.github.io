## 支持 SR-IOV 的 NVMe 设备概述（2025 年 11 月数据）

> [!NOTE]
> 参考神奇海螺的意见，有待验证

NVMe（Non-Volatile Memory Express）协议从 NVMe 1.3 规范开始引入 SR-IOV（Single Root I/O Virtualization）支持，这允许一个物理 NVMe 控制器（Physical Function, PF）虚拟化为多个虚拟控制器（Virtual Functions, VFs），每个 VF 可以独立分配给虚拟机（VM），实现近原生性能的 I/O 直通。主要用于数据中心、云虚拟化和高性能计算（HPC）场景，能显著降低虚拟化开销（hypervisor 干预）。

**注意事项**：
- SR-IOV 支持依赖 NVMe 控制器硬件实现，不是所有 NVMe SSD 都支持（消费级如 Samsung 970/980 系列通常不支持，只有企业级/数据中心级）。
- 需要主机 BIOS/UEFI 启用 SR-IOV 和 IOMMU（Intel VT-d 或 AMD-Vi），以及操作系统（如 Linux kernel 4.15+、VMware ESXi 7.0+）支持。
- 配置通常用 `nvme-cli` 工具（如 `nvme virt-mgmt` 创建 VF、`nvme create-ns` 创建命名空间并 attach 到 VF）。
- Windows 仅支持网络设备的 SR-IOV，存储（NVMe）不支持原生 SR-IOV（需第三方工具或 Hyper-V 变通）。
- 截至 2025 年，支持的设备不多，主要集中在企业级 U.2/E3 形式因子。

#### 支持 SR-IOV 的主流 NVMe SSD/控制器列表

基于 2025 年市场数据和规范，以下是已确认支持 SR-IOV 的 NVMe 设备（聚焦控制器和典型 SSD 型号）。这些设备符合 NVMe 1.3+ 规范的 Virtualization Enhancements（章节 8.5）。

| 厂商/品牌            | 控制器型号                        | 典型 SSD 型号                    | 关键规格（接口/最大 VF 数）                                      | 价格范围（USD，单卡，企业批量） | 备注                                                                                  |
|----------------------|-----------------------------------|----------------------------------|------------------------------------------------------------------|---------------------------------|---------------------------------------------------------------------------------------|
| **Samsung**          | S4LN089X01 (或类似)               | PM1725a/b, PM1733, PM1735        | PCIe Gen4 x4, NVMe 1.4, 最多 64 VF，支持多命名空间（NS）共享     | $1,500–$4,000 (3.2TB–15.36TB)   | 最成熟支持，常用于 Falcon 5208 等多主机解决方案；需 `nvme-cli` 配置 VF 和 NS attach。 |
| **Silicon Motion**   | SM2264, SM2267, SM2267XT          | 汽车/企业级 PCIe 4.0 SSD         | PCIe Gen4 x4, NVMe 1.4, 最多 8 VF，内置 SR-IOV 降低 30% CPU 开销 | $800–$2,000 (1TB–8TB)           | 针对软件定义车辆（SDV）和边缘计算；SM2264 支持 8 通道闪存。                           |
| **Smart IOPS**       | truRandom 控制器                  | Data Engine 系列 (DE1000/DE2000) | PCIe Gen4 x8, NVMe 1.3D, 支持 SR-IOV + ZNS/OC，3M 读 IOPS        | $2,000–$5,000 (4TB–30TB)        | 高端企业级，强调持续性能和数据整形；唯一支持 800K 写 IOPS 的 SR-IOV SSD。             |
| **Intel (Solidigm)** | P3600 系列控制器                  | DC P3600, P4610                  | PCIe Gen3 x4, NVMe 1.3, 支持 SR-IOV（虚拟化兼容）                | $1,000–$3,000 (1.6TB–7.68TB)    | 早期数据中心驱动，支持 SR-IOV 用于性能隔离测试；QEMU/KVM 兼容好。                     |
| **KIOXIA**           | BiCS FLASH 控制器                 | CD8P/CD8R 系列                   | PCIe Gen4 x4, NVMe 1.4, SR-IOV 用于数据中心扩展                  | $1,200–$3,500 (1.92TB–15.36TB)  | 强调多租户（multi-tenancy）和基础设施整合；与 VMware ESXi 集成。                      |
| **Microchip**        | 企业级 PCIe 控制器 (前 Microsemi) | 高级织构 PCIe SSD                | PCIe Gen4, NVMe 1.4, 多 SR-IOV 实例 attach 到 NS                 | $1,500–$4,000 (企业定制)        | 用于云 SSD，支持多主机共享；需 PCIe 开关。                                            |

#### 其他支持或新兴设备
- **Broadcom/VMware 生态**：ESXi 7.0+ 支持 NVMe SR-IOV（非特定硬件），常与 Samsung PM1735 搭配测试。
- **新兴/汽车级**：Silicon Motion 的汽车控制器（2024–2025 新品）支持 SR-IOV，用于 ECU（电子控制单元）共享存储。
- **QEMU 模拟**：Linux QEMU 6.0+ 有 SR-IOV NVMe 补丁（hw/nvme）
- **不推荐/不成熟**：消费级如 WD Black SN850X 或 Samsung 990 PRO 不支持 SR-IOV（无企业控制器）。

#### 如何验证和启用（Linux 示例）
1. **检查支持**：`lspci -vvv | grep -i sriov`（看 SR-IOV Capabilities）。
2. **启用 VF**：`echo 4 > /sys/class/nvme/nvme0/device/sriov_numvfs`（创建 4 个 VF）。
3. **配置命名空间**：用 `nvme-cli`：
   - `nvme delete-ns /dev/nvme0n1 -n 1`（删除默认 NS）。
   - `nvme create-ns /dev/nvme0 -s 1000G`（创建 1TB NS）。
   - `nvme attach-ns /dev/nvme0 -n 1 -c 1`（attach 到 VF 控制器 ID 1）。
   - `nvme virt-mgmt /dev/nvme0 -c 0x0001 -r0 -n2 -a8`（分配资源）。
4. **VF 直通**：在 KVM/QEMU 中用 PCI passthrough 将 VF 分配给 VM。

#### 价格与可用性（2025 年 11 月）
- **入门级**：Samsung PM1733 (~$1,500/3.2TB)，易购（Amazon/企业渠道）。
- **高端**：Smart IOPS DE2000 (~$4,000+），需 OEM（如 Dell/HP 服务器集成）。
- 趋势：价格稳定，但 Gen5 NVMe（2025 新品）可能加 $500–1,000 支持 SR-IOV。批量采购降 20–30%。

如果需要特定型号的详细配置脚本或 QEMU 模拟指南，请提供更多细节！


## 每一个 vm 是否可以观测到所有的容量
**答案：观测到的容量 **不相同**，每个 VM 看到的都是 **分割后的子集容量**。**

这是 NVMe SR-IOV 的核心设计：**物理盘总容量 = Σ(所有 VF 的命名空间容量)**

1. 为什么不是每个 VF 都看到完整物理容量？

| 方案 | 每个 VM 看到的容量 | 物理盘总容量利用率 | 实际可行性 |
|------|-------------------|-------------------|------------|
| **每个 VF 看到完整容量** | 4TB（物理也是 4TB） | 400%（4 VM × 4TB） | ❌ 不可能，违反物理定律 |
| **每个 VF 看到分割容量** | 1TB（4 VF，每个 1TB） | 100% | ✅ NVMe SR-IOV 标准做法 |
| **每个 VF 共享整个容量** | 4TB（但有隔离） | 100% | ❌ 目前无硬件支持 |

2. NVMe SR-IOV 的真实工作机制

```
物理 NVMe SSD (4TB总容量)
    |
    +-- PF (Physical Function)  ← 管理所有命名空间
    |       |
    |       +-- Namespace 1 (1TB) → VF 0
    |       +-- Namespace 2 (1TB) → VF 1
    |       +-- Namespace 3 (1TB) → VF 2
    |       +-- Namespace 4 (1TB) → VF 3
    |
    +-- VF 0 → VM 0 看到 /dev/nvme0n1 [1TB]
    +-- VF 1 → VM 1 看到 /dev/nvme0n1 [1TB]
    +-- VF 2 → VM 2 看到 /dev/nvme0n1 [1TB]
    +-- VF 3 → VM 3 看到 /dev/nvme0n1 [1TB]
```

3. 实际配置示例（Samsung PM1733，4TB 盘）

- 在宿主机创建命名空间并分配给 VF

```bash
# 查看物理盘
nvme list
# /dev/nvme0 4.0 TB Samsung PM1733

# 删除默认命名空间（如果有）
nvme delete-ns /dev/nvme0 -n 1

# 创建 4 个 1TB 命名空间
nvme create-ns /dev/nvme0 -s 1000G  # NS 1: 1TB
nvme create-ns /dev/nvme0 -s 1000G  # NS 2: 1TB
nvme create-ns /dev/nvme0 -s 1000G  # NS 3: 1TB
nvme create-ns /dev/nvme0 -s 1000G  # NS 4: 1TB

# 启用 4 个 VF
echo 4 > /sys/class/nvme/nvme0/device/sriov_numvfs

# 把命名空间 attach 到对应 VF
nvme attach-ns /dev/nvme0 -n 1 -c 0  # NS 1 → VF 0 (控制器 ID 0)
nvme attach-ns /dev/nvme0 -n 2 -c 1  # NS 2 → VF 1 (控制器 ID 1)
nvme attach-ns /dev/nvme0 -n 3 -c 2  # NS 3 → VF 2
nvme attach-ns /dev/nvme0 -n 4 -c 3  # NS 4 → VF 3
```

- 每个 VM 看到的真实情况

**VM 0（直通 VF 0）**：
```bash
lspci -nn | grep NVMe
# 05:00.0 Non-Volatile memory controller [0108]: Samsung Electronics Co Ltd PM1733 NVMe SSD [144d:a82b]

lsblk
# nvme0n1    1T  nvme0n1p1  500G  (VM 0 只能看到 1TB)
```

**VM 1（直通 VF 1）**：
```bash
lsblk
# nvme0n1    1T  nvme0n1p1  500G  (VM 1 也只能看到 1TB，完全隔离)
```

- 验证总容量利用率

**宿主机查看**：
```bash
# PF 看到所有命名空间总和 = 物理容量
nvme list-ns /dev/nvme0
# [ 1 ]:0x1   1.00 TB
# [ 2 ]:0x2   1.00 TB
# [ 3 ]:0x3   1.00 TB
# [ 4 ]:0x4   1.00 TB
# 总计：4TB ✓
```

**动态调整示例**：
```bash
# 把 NS 1 从 VF 0 迁移到 VF 1
nvme detach-ns /dev/nvme0 -n 1 -c 0
nvme attach-ns /dev/nvme0 -n 1 -c 1
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
