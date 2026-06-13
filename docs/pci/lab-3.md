## question

For the driver developer, the PCI family offers an attractive advantage: a system of automatic device
configuration. Unlike drivers for the older ISA generation, PCI drivers need not implement complex probing
logic. During boot, the BIOS-type boot firmware (or the kernel itself if so configured) walks the PCI bus and
assigns resources such as interrupt levels and I/O base addresses. **The device driver gleans this assignment by
peeking at a memory region called the PCI configuration space.** PCI devices possess 256 bytes of configuration
memory. The top 64 bytes of the configuration space is standardized and holds registers that contain details
such as the status, interrupt line, and I/O base addresses.
> bios 对于设备进行探测，然后将 configuration 放到 configuration region.
> 然后让设备驱动读取。

- [ ] 这个读去过程具体在哪里 ? probe 吗 ？
- [ ] pcie 如何支持一个新插入的设备 ？

To operate on a memory region such as the frame buffer on the above PCI video card, follow these steps:
1.  pci_resource_start
2.  request_mem_region


- [ ] [^1] 解释 request_mem_region 只是预留空间的作用, 但是这个预留又不能修改 pcie root hub 对于地址的解析，或者说，这完全是软件上的操作，软件申请了，向该地方写数值，最后还是靠 pcie 的解析，所以，request_mem_region 的根本作用引入检查 ？
  - [ ] 如果是引入检查，那么，从 pci configuration region 中间直接读去不香吗 ?

- [ ] host bridge 是 ?

- https://stackoverflow.com/questions/7682422/what-does-request-mem-region-actually-do-and-when-it-is-needed
- https://stackoverflow.com/questions/18854931/how-does-the-os-detect-hardware

## pci=realloc 的确会有问题的哦
```txt
[   37.010271] NVRM: This PCI I/O region assigned to your NVIDIA device is invalid:
               NVRM: BAR1 is 0M @ 0x0 (PCI:0000:4f:00.0)
[   37.010350] nvidia: probe of 0000:4f:00.0 failed with error -1
[   37.010449] NVRM: This PCI I/O region assigned to your NVIDIA device is invalid:
               NVRM: BAR1 is 0M @ 0x0 (PCI:0000:52:00.0)
[   37.010538] nvidia: probe of 0000:52:00.0 failed with error -1
[   37.010658] NVRM: This PCI I/O region assigned to your NVIDIA device is invalid:
               NVRM: BAR1 is 0M @ 0x0 (PCI:0000:56:00.0)
[   37.010736] nvidia: probe of 0000:56:00.0 failed with error -1
[   37.010853] NVRM: This PCI I/O region assigned to your NVIDIA device is invalid:
               NVRM: BAR1 is 0M @ 0x0 (PCI:0000:57:00.0)
[   37.010936] nvidia: probe of 0000:57:00.0 failed with error -1
[   37.011009] NVRM: The NVIDIA probe routine failed for 4 device(s).
```

### pci-realloc 的作用是什么
`pci=realloc` 是 Linux 内核启动参数，用于**强制内核在启动阶段重新分配（reallocate）PCI 设备的 I/O 与内存资源**。它主要用于解决 BIOS/固件对 PCI 资源分配不正确或不完整的问题。

**`pci=realloc` 告诉内核：不要完全信任固件给 PCI 设备分配的 BAR 资源，启动时由内核重新计算并分配。**

背景：为什么需要它

在 x86/ARM 服务器、虚拟化环境中，PCI 设备的资源分配流程大致是：

1. **BIOS / UEFI 固件**
   * 枚举 PCI 设备
   * 给每个设备的 BAR（MMIO / I/O）分配地址
2. **Linux 内核**
   * 默认尽量“沿用”固件给的分配结果
   * 只在必要时做最小调整

但现实中常见问题包括：

* BIOS 分配的 BAR **重叠**
* 分配的 MMIO 空间 **不够大**
* 高端 PCIe 设备（如 GPU / NIC）需要 **64-bit BAR**，但 BIOS 没给
* SR-IOV / VFIO / 直通设备，BIOS 分配策略不合理
* 热插拔（hotplug）空间未预留


`pci=realloc` 的具体作用

启用后，内核在 PCI 枚举阶段会：
1. **忽略或部分忽略 BIOS 的 BAR 分配**
2. 重新扫描整个 PCI 拓扑
3. 按内核的资源管理策略：
   * 重新计算 BAR 大小
   * 重新分配 I/O port、32-bit MMIO、64-bit MMIO
4. 尽量保证：
   * BAR 不重叠
   * 大 BAR 放在 64-bit 地址空间
   * 给热插拔 / SR-IOV 留足空间

常见的使用场景:
1. VFIO / PCI Passthrough
2. SR-IOV
3. 大 BAR 设备（Resizable BAR / GPU）
4. 某些服务器 / 工控主板


与相关参数的区别

| 参数                | 作用                   | 说明                     |
| ------------------- | ------------------     | ---------------          |
| `pci=realloc`       | 强制重新分配 PCI 资源  | 最常用                   |
| `pci=nocrs`         | 忽略 ACPI _CRS         | 当 ACPI 报告的资源有问题 |
| `pci=assign-busses` | 重新分配 bus number    | 多 PCI bridge 场景       |
| `pci=hpmemsize=`    | 指定 hotplug MMIO 空间 | 手工预留                 |
| `pci=use_crs`       | 强制使用 ACPI CRS      | 与 `nocrs` 相反          |

1. 潜在风险与注意事项
	* **可能改变设备 BAR 地址**
	  * 对依赖“固定物理地址”的老驱动可能有影响
	* 在某些 **buggy BIOS** 上可能导致：
	  * 某些设备消失
	  * legacy I/O 设备异常
	* 不建议在**一切正常的系统上盲目开启**
2. `pci=realloc` 是 **PCI 资源分配兜底方案** 本质是 内核接管 PCI BAR 分配权
在 **VFIO / SR-IOV / 大 BAR / BIOS 不靠谱** 的场景中非常有用
不是性能优化参数，而是 **稳定性 / 兼容性参数**

(不过，现在知道了重要的事情，那就是操作的确会使用固件的信息)

(难道 mmio 不能超过 4G 吗?)

简单看了一下 pci=realloc 的影响在 drivers/pci/setup-bus.c 中

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
