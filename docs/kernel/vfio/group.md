## 看看这个 ACS override 是什么鬼?
https://superuser.com/questions/1350451/isolate-single-device-into-separate-iommu-group-for-pci-passthrough
 - https://vfio.blogspot.com/2014/08/iommu-groups-inside-and-out.html
 - https://queuecumber.gitlab.io/linux-acs-override/
 - https://wiki.archlinux.org/title/PCI_passthrough_via_OVMF#Bypassing_the_IOMMU_groups_(ACS_override_patch)

## 看看内核是如何确定加入方法的

- iommu_init_device
  -	group = ops->device_group(dev);

viommu 和 intel 都是如此
```c
static struct iommu_group *viommu_device_group(struct device *dev)
{
	if (dev_is_pci(dev)
		return pci_device_group(dev);
	else
		return generic_device_group(dev);
}
```

amd 这样的:
```c
static struct iommu_group *amd_iommu_device_group(struct device *dev)
{
	if (dev_is_pci(dev))
		return pci_device_group(dev);

	return acpihid_device_group(dev);
}
```

aarch64 的: arm_smmu_device_group

- pci_device_group
  - get_pci_function_alias_group
    - get_pci_alias_group

这三个设备是由于分别几个 function
```txt
00:1f.0 ISA bridge: Intel Corporation 82801IB (ICH9) LPC Interface Controller (rev 02)
00:1f.2 SATA controller: Intel Corporation 82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode] (rev 02)
00:1f.3 SMBus: Intel Corporation 82801I (ICH9 Family) SMBus Controller (rev 02)
```

使用 alpine.sh 中的 nvme sriov ，
```txt
[   95.376646] pci 0000:01:00.1: [1b36:0010] type 00 class 0x010802 PCIe Endpoint
[   95.376877] pci 0000:01:00.1: enabling Extended Tags
[   95.377270] pci 0000:01:00.1: Adding to iommu group 16
[   95.377976] nvme nvme1: pci function 0000:01:00.1
[   95.378138] nvme 0000:01:00.1: enabling device (0000 -> 0002)
[   95.381532] nvme nvme1: Device not ready; aborting initialisation, CSTS=0x2
```
产生新的 group 的过程是:
```txt
01:00.0 Non-Volatile memory controller: Red Hat, Inc. QEMU NVM Express Controller (rev 02)
01:00.1 Non-Volatile memory controller: Red Hat, Inc. QEMU NVM Express Controller (rev 02)
```

探测过程也是在 iommu_probe_device 中
```txt
@[
    iommu_group_alloc+5
    pci_device_group+330
    __iommu_probe_device+290
    iommu_probe_device+36
    iommu_bus_notifier+43
    notifier_call_chain+90
    blocking_notifier_call_chain+63
    bus_notify+52
    device_add+1591
    pci_device_add+577
    pci_iov_add_virtfn+446
    sriov_enable+502
    pci_sriov_configure_simple+56
    sriov_numvfs_store+191
    kernfs_fop_write_iter+289
    vfs_write+673
    ksys_write+110
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1
```
其区别在于:
pci_acs_enabled(pdev, REQ_ACS_FLAGS) -> pci_dev_specific_acs_enabled

## 使用 alpine.sh 模拟的时候，观察到
qemu 模拟的，两个 nvme 在一个
```txt
IOMMU Group 9:
        00:0a.0 PCI bridge [0604]: Red Hat, Inc. QEMU PCI-PCI bridge [1b36:0001]
        03:01.0 Non-Volatile memory controller [0108]: Red Hat, Inc. QEMU NVM Express Controller [1b36:0010] (rev 02)
        03:02.0 Non-Volatile memory controller [0108]: Red Hat, Inc. QEMU NVM Express Controller [1b36:0010] (rev 02)
```
但是 sriov 可以每一个都切分一个:
```txt
IOMMU Group 16:
        01:00.0 Ethernet controller [0200]: Intel Corporation 82576 Gigabit Network Connection [8086:10c9] (rev 01)
IOMMU Group 17:
        01:10.0 Ethernet controller [0200]: Intel Corporation 82576 Virtual Function [8086:10ca] (rev 01)
IOMMU Group 18:
        01:10.2 Ethernet controller [0200]: Intel Corporation 82576 Virtual Function [8086:10ca] (rev 01)
IOMMU Group 19:
        01:10.4 Ethernet controller [0200]: Intel Corporation 82576 Virtual Function [8086:10ca] (rev 01)
```

## 在 qemu 中，在 pci bridge 下的 nvme 无法被直通
用这个查找
```sh
find /sys -name "*iommu*"
```
根本找不到

## iommu 的 domain 的概念深入理解下

### struct iommu_group

l1 虚拟机中没有配置 iommu ，但是可以观测到:
```txt
/sys/kernel/iommu_groups/0/devices:
 82a31eeb-094b-4464-9a0e-69c416f76cd4

/sys/kernel/iommu_groups/1/devices:
 bd52f358-1920-48ed-9f85-920b58859259
```

```txt
kernel/iommu_groups/1🔒 🌳
🧀  tree
.
├── devices
│   └── bd52f358-1920-48ed-9f85-920b58859259 -> ../../../../devices/virtual/mdpy/mdpy/bd52f358-1920-48ed-9f85-920b58859259
├── name
├── reserved_regions
└── type
```

检查其中的内容:
```txt
🧀  cat type
unknown
kernel/iommu_groups/1🔒 🌳
🧀  cat name
vfio-noiommu
```

### 不同的 iommu domain type 的差别说明是什么?
<!-- a3cb451a-abdd-466f-bb16-b0220aeba999 -->

一共就这三个 domain 吗?

原来这三个 demain 都是可以选的
```txt
choice
	prompt "IOMMU default domain type"
	depends on IOMMU_API
	default IOMMU_DEFAULT_DMA_LAZY if X86 || S390
	default IOMMU_DEFAULT_DMA_STRICT
	help
	  Choose the type of IOMMU domain used to manage DMA API usage by
	  device drivers. The options here typically represent different
	  levels of tradeoff between robustness/security and performance,
	  depending on the IOMMU driver. Not all IOMMUs support all options.
	  This choice can be overridden at boot via the command line, and for
	  some devices also at runtime via sysfs.

	  If unsure, keep the default.
```

hygon 的机器上:
```txt
[]$ find . -name type -exec cat {} +
identity
identity
identity
identity
identity
identity
identity
identity
identity
identity
identity
identity
identity
identity
identity
identity
```

13900K 上测试结果，iommu=pt
```txt
[root@nixos:/sys/kernel/iommu_groups]# for i in /sys/kernel/iommu_groups/*;do cat $i/type ;done
DMA
DMA
DMA
DMA
DMA
identity
identity
identity
DMA
identity
identity
DMA
identity
identity
identity
identity
identity
identity
DMA
```

```txt
🧀  find . -name type -exec cat {} +

DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA-FQ
DMA
DMA-FQ
```

cd /sys/kernel/iommu_groups && for i in ./*;do cat $i/type ;done


```txt
[root@nixos:/sys/kernel/iommu_groups]#cd /sys/kernel/iommu_groups && for i in ./*;do cat $i/type &&  ls $i/devices ;done
DMA
0000:00:00.0
DMA
0000:00:01.0
DMA
0000:00:1c.0
DMA
0000:00:1c.2
DMA
0000:00:1d.0
identity
0000:00:1f.0  0000:00:1f.3  0000:00:1f.4  0000:00:1f.5
identity
0000:01:00.0  0000:01:00.1
identity
0000:02:00.0
DMA
0000:03:00.0
identity
0000:05:00.0
identity
0000:06:00.0
DMA
0000:00:06.0
identity
0000:00:0a.0
identity
0000:00:14.0  0000:00:14.2
identity
0000:00:14.3
identity
0000:00:15.0  0000:00:15.1  0000:00:15.2
identity
0000:00:16.0
identity
0000:00:17.0
DMA
0000:00:1a.0
```

```txt
🧀  lspci
00:00.0 Host bridge: Intel Corporation Device a700 (rev 01)
00:01.0 PCI bridge: Intel Corporation Device a70d (rev 01)
00:06.0 PCI bridge: Intel Corporation Device a74d (rev 01)
00:0a.0 Signal processing controller: Intel Corporation Device a77d (rev 01)
00:14.0 USB controller: Intel Corporation Alder Lake-S PCH USB 3.2 Gen 2x2 XHCI Controller (rev 11)
00:14.2 RAM memory: Intel Corporation Alder Lake-S PCH Shared SRAM (rev 11)
00:14.3 Network controller: Intel Corporation Alder Lake-S PCH CNVi WiFi (rev 11)
00:15.0 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #0 (rev 11)
00:15.1 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #1 (rev 11)
00:15.2 Serial bus controller: Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #2 (rev 11)
00:16.0 Communication controller: Intel Corporation Alder Lake-S PCH HECI Controller #1 (rev 11)
00:17.0 SATA controller: Intel Corporation Alder Lake-S PCH SATA Controller [AHCI Mode] (rev 11)
00:1a.0 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #25 (rev 11)
00:1c.0 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #1 (rev 11)
00:1c.2 PCI bridge: Intel Corporation Device 7aba (rev 11)
00:1d.0 PCI bridge: Intel Corporation Device 7ab6 (rev 11)
00:1f.0 ISA bridge: Intel Corporation Device 7a86 (rev 11)
00:1f.3 Audio device: Intel Corporation Alder Lake-S HD Audio Controller (rev 11)
00:1f.4 SMBus: Intel Corporation Alder Lake-S PCH SMBus Controller (rev 11)
00:1f.5 Serial bus controller: Intel Corporation Alder Lake-S PCH SPI Controller (rev 11)
01:00.0 VGA compatible controller: NVIDIA Corporation GP106 [GeForce GTX 1060 3GB] (rev a1)
01:00.1 Audio device: NVIDIA Corporation GP106 High Definition Audio Controller (rev a1)
02:00.0 Non-Volatile memory controller: Yangtze Memory Technologies Co.,Ltd ZHITAI TiPro7000 (rev 01)
03:00.0 Non-Volatile memory controller: Yangtze Memory Technologies Co.,Ltd Device 0071 (rev 01)
05:00.0 Ethernet controller: Intel Corporation Ethernet Controller I225-V (rev 03)
06:00.0 Non-Volatile memory controller: MAXIO Technology (Hangzhou) Ltd. Device 1602 (rev 01)
```
没有找到那些可以修改，那些不能修改的原因。

intel 上是可以修改 type 的 ，AMD 上所有的尝试都失败了

```txt
[    4.566167] Call Trace:
[    4.566170]  dump_stack+0x66/0x8b
[    4.566173]  __iommu_domain_alloc+0x44/0x80
[    4.566175]  iommu_group_get_for_dev+0xd8/0x1c0
[    4.566178]  ? iommu_device_link+0x76/0xc0
[    4.566181]  amd_iommu_add_device+0x137/0x5f0
[    4.566185]  add_iommu_group+0x2b/0x50
[    4.566187]  ? iommu_group_unshare_domain+0x40/0x40
[    4.566190]  bus_for_each_dev+0x76/0xc0
[    4.566193]  ? down_write+0xe/0x40
[    4.566196]  bus_set_iommu+0xbf/0x100
[    4.566198]  amd_iommu_init_api+0x112/0x132
[    4.566201]  iommu_go_to_state+0x740/0x933
[    4.566203]  ? e820__memblock_setup+0x60/0x60
[    4.566206]  ? set_debug_rodata+0x2f/0x2f
[    4.566208]  amd_iommu_init+0x11/0x9f
[    4.566211]  ? e820__memblock_setup+0x60/0x60
[    4.566213]  pci_iommu_init+0x16/0x3f
[    4.566216]  do_one_initcall+0x46/0x1d1
[    4.566219]  kernel_init_freeable+0x1c3/0x278
[    4.566222]  ? rest_init+0xb0/0xb0
[    4.566225]  kernel_init+0xa/0x110
[    4.566228]  ret_from_fork+0x35/0x40
```

在函数 iommu_group_get_for_dev 中

那么最后 iommu=pt 的实现在什么地方?
```c
static int __init iommu_set_def_domain_type(char *str)
{
	bool pt;
	int ret;

	ret = kstrtobool(str, &pt);
	if (ret)
		return ret;

	iommu_def_domain_type = pt ? IOMMU_DOMAIN_IDENTITY : IOMMU_DOMAIN_DMA;
	return 0;
}
early_param("iommu.passthrough", iommu_set_def_domain_type);
```

而 iommu_setup 中设置的是，最后影响的地方是这里:
```c
int __init amd_iommu_init_dma_ops(void)
{
	swiotlb        = (iommu_pass_through || sme_me_mask) ? 1 : 0;
	iommu_detected = 1;

	/*
	 * In case we don't initialize SWIOTLB (actually the common case
	 * when AMD IOMMU is enabled and SME is not active), make sure there
	 * are global dma_ops set as a fall-back for devices not handled by
	 * this driver (for example non-PCI devices). When SME is active,
	 * make sure that swiotlb variable remains set so the global dma_ops
	 * continue to be SWIOTLB.
	 */
	if (!swiotlb){
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		dma_ops = &dma_direct_ops;
	}

	if (amd_iommu_unmap_flush)
		pr_info("AMD-Vi: IO/TLB flush on unmap enabled\n");
	else
		pr_info("AMD-Vi: Lazy IO/TLB flushing enabled\n");

	return 0;

}
```

### 那么 domain 是什么意思
通过 docs/kernel/vfio/group.md ，已经很清楚，group 和 container 的含义了，
那么 domain 是什么意思

`vfio_iommu_type1_group_iommu_domain` 中的 domain 是个什么含义

其实不是一个东西了，但是

```c
void amd_iommu_init_identity_domain(void)
{
	struct iommu_domain *domain = &identity_domain.domain;

	domain->type = IOMMU_DOMAIN_IDENTITY;
	domain->ops = &identity_domain_ops;
	domain->owner = &amd_iommu_ops;

	identity_domain.id = pdom_id_alloc();

	protection_domain_init(&identity_domain);
}
```

为什么在 13900k 上，即便是直通了设备，依旧看到所有的 iommu group 的输出都是 identity
```sh
for i in /sys/kernel/iommu_groups/*;do cat $i/type ;done
```

### 将一个 device attach 到 domain 到底是什么含义?

例如，在 __iommu_attach_device 中，传递的就是 device :
```c
static int __iommu_attach_device(struct iommu_domain *domain,
				 struct device *dev)
{
	int ret;

	if (unlikely(domain->ops->attach_dev == NULL))
		return -ENODEV;

	ret = domain->ops->attach_dev(domain, dev);
	if (ret)
		return ret;
	dev->iommu->attach_deferred = 0;
	trace_attach_device_to_domain(dev);
	return 0;
}
```

例如在 intel_pasid_get_entry 中
将 intel_pasid_get_table

```c
static inline void *dev_iommu_priv_get(struct device *dev)
{
	if (dev->iommu)
		return dev->iommu->priv;
	else
		return NULL;
}
```

看来 device 初始化的时候，就添加了很多属性，参考这些属性

这些的关键都是 device 中的 iommu 参数。


### domain 就是这样就被终结了?
<!-- d505b33f-7aaf-47da-b4b5-7b1a9c38f8c8 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

| 概念 | 说明 |
|------|------|
| **Domain** | IOMMU 中的隔离单元，每个 domain 拥有独立的页表。**不同 domain 间的设备无法互相访问内存** |
| **DMA API** | 驱动通过 `dma_map_*()` 申请设备可访问的物理地址，IOMMU 会为这些地址建立映射 |

一个 domain = 一套页表 = 一个隔离边界。设备加入哪个 domain，决定了它能访问哪些内存。

三种 Domain 类型对比

| 类型 | 页表行为 | 隔离粒度 | 安全性 | 性能 | 适用场景 |
|------|----------|----------|--------|------|----------|
| **DMA_STRICT** | 每次 `dma_unmap()` **立即清除**映射 | 每次 DMA 操作后回收权限 | ★★★ 最高 | 较低（频繁更新页表） | 高安全场景（如虚拟化、不可信设备） |
| **DMA_LAZY** | `dma_unmap()` **延迟清除**，定期批量回收 | 按需回收，容忍短暂残留映射 | ★★ 中等 | 较高（减少 IOTLB flush） | 通用场景（默认平衡点） |
| **IDENTITY** | 绕过 IOMMU，设备直通物理地址 | 无隔离 | ★ 无 | 最高（零转换开销） | 可信设备或性能敏感场景（需显式指定） |

> **注意**：`IDENTITY` 通常**不作为默认选项**，需通过 `iommu.passthrough=1` 单独启用。

Strict vs Lazy 的本质区别

```c
// DMA_STRICT 行为
dma_map_single(dev, buf, size, DMA_TO_DEVICE);
    → IOMMU 页表插入映射 [VA=0x1000 → PA=0xdeadbeef]

dma_unmap_single(dev, dma_addr, size, DMA_TO_DEVICE);
    → IOMMU 页表**立即删除**映射
    → 设备此后访问 0x1000 会触发 **DMA fault**

// DMA_LAZY 行为
dma_unmap_single(...);
    → 仅标记映射为"待回收"，**不立即删除**
    → 页表项保留直到：
    //   a) 下次映射时空间不足
    //   b) 定时器触发批量回收
    //   c) 显式调用 iommu_flush()
```

**安全影响**：
- **Lazy 残留风险**：设备在 `unmap` 后、映射实际删除前的窗口期内，仍可访问已释放的内存（可能被内核重用），存在**信息泄露或破坏风险**。
- **Strict 保障**：映射立即失效，设备访问会触发 fault 并被 IOMMU 拦截。

虚拟化宿主机全局设为 `strict`，但为可信的 NVMe SSD 设备单独设为 `lazy` 以提升吞吐。

| **最高安全**（如云主机隔离、不可信设备） | `DMA_STRICT` + 内核参数 `iommu.strict=1` |
| **性能优先**（如高性能 NVMe、GPU 直通） | `DMA_LAZY` + 确保设备驱动正确使用 DMA API |

> **关键提醒**：Lazy 模式**依赖驱动正确调用 `dma_unmap_*()`**。若驱动存在 bug（如漏调用 unmap），lazy 会放大安全风险；strict 则能通过 fault 早期暴露问题。

cat /sys/bus/pci/devices/*/iommu_group/type


### 是这样的吗?
<!-- ef5aab60-1d42-4bec-bea9-6a70ce8480d0 -->


| 配置项 | 作用层次 | 决策问题 | 是否经过 IOMMU |
|--------|----------|----------|----------------|
| **`iommu=pt`** | **设备接入层** | 设备是否参与 IOMMU 地址转换？ | ❌ 绕过 IOMMU，直通物理地址 |
| **`DMA_STRICT`/`LAZY`** | **映射管理层** | 若使用 IOMMU，映射何时回收？ | ✅ 经过 IOMMU，管理页表生命周期 |

> ✅ **关键结论**：`iommu=pt` 的设备**根本不进入 IOMMU domain 体系**，因此 `DMA_STRICT/LAZY` 对其**完全不生效**。

二、执行路径对比

```text
设备发起 DMA 请求
│
├─ 情况 A: iommu=pt (passthrough)
│   → IOMMU 硬件被绕过
│   → 设备直接使用物理地址（identity mapping）
│   → 无 domain 概念，无页表，无隔离
│   → ⚠️ 高性能但无 DMA 保护（可能访问任意物理内存）
│
└─ 情况 B: IOMMU enabled (默认)
    → 请求进入 IOMMU
    → 分配到某个 domain（通常为 default DMA domain）
    → 根据 domain type 决定映射策略：
        ├─ DMA_STRICT: unmap 时立即删除页表项
        └─ DMA_LAZY : unmap 时延迟回收（批量优化）
```

iommu=pt` 的实现细节

| 项目 | 说明 |
|------|------|
| **实际效果** | 并非"完全禁用 IOMMU"，而是为设备建立 **1:1 identity mapping**（VA = PA），或通过硬件特性（如 Intel VT-d 的 "PassThrough" 位）绕过地址转换 |
| **与 identity domain 区别** | `iommu=pt` 是**启动时全局策略**；identity domain 是运行时可动态分配的 domain 类型（通过 `iommu_identity_mapping`） |
| **粒度** | 传统 `iommu=pt` 是全局的；现代内核支持 **per-device passthrough**（依赖 PCIe ACS 特性） |
| **典型用途** | GPU 直通（避免 VFIO 性能开销）、高性能 NVMe（绕过 IOTLB flush） |

四、配置组合的实际效果

| 内核参数组合 | IOMMU 是否启用 | Domain 类型生效？ | 安全性 | 性能 |
|--------------|----------------|-------------------|--------|------|
| `iommu=pt` | ❌ 设备绕过 | ❌ 不适用 | 低（无隔离） | 高 |
| `iommu=on` + `iommu.strict=1` | ✅ 启用 | ✅ STRICT | 高 | 中 |
| `iommu=on` + `iommu.strict=0` | ✅ 启用 | ✅ LAZY | 中 | 高 |
| 无参数（默认） | ✅ 启用 | ✅ 取决于 Kconfig 默认 | 中 | 中 |

> 💡 **验证当前状态**：
> ```bash
> # 查看 IOMMU 是否全局启用
> dmesg | grep -i "IOMMU.*enabled"
>
> # 查看设备是否处于 passthrough
> cat /sys/bus/pci/devices/0000:01:00.0/iommu_group/name  # passthrough 设备可能无此文件
>
> # 查看 strict 模式开关
> cat /sys/module/iommu/parameters/strict
> ```


五、典型场景对比

| 场景 | 推荐配置 | 原因 |
|------|----------|------|
| **虚拟化宿主机（VFIO）** | `iommu=on` + `iommu.strict=1` | 严格隔离防止恶意设备逃逸 |
| **GPU 直通给虚拟机** | `iommu=pt`（仅针对该 GPU） | 避免 IOTLB 开销，提升图形性能 |
| **高性能 NVMe（可信设备）** | `iommu=on` + `iommu.strict=0` | lazy 模式减少映射开销，保留基础隔离 |
| **调试 DMA 驱动** | `iommu=on` + `iommu.strict=1` | 严格模式可快速暴露 `dma_unmap` 漏洞 |

六、常见误解澄清

| 误解 | 正确理解 |
|------|----------|
| "`iommu=pt` 就是 identity domain" | ❌ `pt` 是绕过机制，identity domain 仍是 IOMMU 管理的 domain 类型 |
| "`iommu=pt` 会禁用所有 IOMMU 功能" | ❌ 仅影响标记为 passthrough 的设备，其他设备仍受 IOMMU 保护 |
| "LAZY 模式 = passthrough" | ❌ LAZY 仍经过 IOMMU，只是映射回收策略不同 |

七、架构差异（x86 vs ARM）

| 架构 | `iommu=pt` 支持 | 备注 |
|------|-----------------|------|
| **x86 (Intel VT-d/AMD-Vi)** | ✅ 完善 | 通过 `DMAR` 表的 `P` 位控制 |
| **ARM (SMMU)** | 部分支持 | 依赖具体 SoC 实现，通常用 `passthrough` 属性在 DTS 中指定 |

- **`iommu=pt`** 决定设备**是否进入 IOMMU 体系**（二选一：绕过 or 参与）
- **`DMA_STRICT/LAZY`** 决定设备**参与 IOMMU 后的映射管理策略**（细化控制）
- 两者是**父子关系**：只有当设备未被 `pt` 绕过时，domain type 配置才生效

作为内核工程师，在涉及设备直通或安全隔离的场景中，需同时审视这两个维度：先决定是否需要 IOMMU 保护（`pt` vs `on`），再决定保护粒度（`strict` vs `lazy`）。

## pci_acs_enabled
看看这个函数的定义

## ats 和 acs

https://liujunming.top/2019/11/24/Introduction-to-PCIe-Access-Control-Services/

## PCI ACS

pci_device_group

```txt
	/*
	 * Continue upstream from the point of minimum IOMMU granularity
	 * due to aliases to the point where devices are protected from
	 * peer-to-peer DMA by PCI ACS.  Again, if we find an existing
	 * group, use it.
	 */
```


## 经典问题，有看到了 attach_device_to_domain 和 add_device_to_group

所以是什么区别呢?
```txt
 tracepoint -w -s
sudo perf stat -e iommu:*
^C
 Performance counter stats for 'system wide':

                 0      iommu:io_page_fault
              8674      iommu:unmap
             12641      iommu:map
                 0      iommu:attach_device_to_domain
                 0      iommu:remove_device_from_group
                 0      iommu:add_device_to_group

      12.426687319 seconds time elapsed
```

## [ ] 原来，iommu_groups 是可以动态创建的

通过 mtty 可以发现
```txt
/sys/kernel/iommu_groups/21/devices:
 83b8f4f2-509f-382f-3c1e-e6bfe0fa1001

🧀  ls /sys/kernel/iommu_groups/
 0   1   2   3   4   5   6   7   8   9   10   11   12   13   14   15   16   17   18   19   20   21
```

将 mdev device 删除之后:
```txt
🧀  ls /sys/kernel/iommu_groups/
 0   1   2   3   4   5   6   7   8   9   10   11   12   13   14   15   16   17   18   19   20
```

在虚拟机中测试更加直接:
```txt
cd /sys/devices/virtual/mtty/mtty/mdev_supported_types/mtty-1
uuidgen | sudo tee create
```

即便是虚拟机中，完全没有 iommu 支持，结果可以发现 iommu_groups
```txt
🧀  ls -la /sys/kernel/iommu_groups/0/devices
lrwxrwxrwx - root 15 Mar 11:35 4d29d626-f797-42b7-8dd9-08bc30ee719b -> ../../../../devices/virtual/mtty/mtty/4d29d626-f797-42b7-8dd9-08bc30ee719b
ls -la /sys/class/iommu
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
