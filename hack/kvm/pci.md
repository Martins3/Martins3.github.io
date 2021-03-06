# pci

<!-- vim-markdown-toc GitLab -->

- [关键blog](#关键blog)
- [关键blog2](#关键blog2)
- [Loyenwang](#loyenwang)
- [PCH](#pch)
- [overview](#overview)
- [ldd3](#ldd3)
  - [pci_device_id](#pci_device_id)
  - [pci_dev](#pci_dev)
  - [bus_type](#bus_type)
  - [pci_bus](#pci_bus)
  - [pci_host_bridge](#pci_host_bridge)
  - [device_driver](#device_driver)
- [pci bar](#pci-bar)
- [hotplug](#hotplug)
- [proc and sys](#proc-and-sys)
  - [lspci](#lspci)
- [configuration space](#configuration-space)
- [probe](#probe)
- [irq](#irq)
- [question](#question)

<!-- vim-markdown-toc -->

**When Comming back : Loyenwang has analyze PCI, I don't think we will have any question about it**

- [ ] iommu group

- [ ] 内核文档 : [^3]
- [ ] https://wiki.osdev.org/PCI_Express / https://wiki.osdev.org/PCI

- [ ] https://www.kernel.org/doc/Documentation/PCI/pci.txt
- [ ] https://sites.google.com/site/pinczakko/pinczakko-s-guide-to-award-bios-reverse-engineering
- [ ] https://www.kernel.org/doc/html/latest/PCI/sysfs-pci.html?highlight=proc%20irq

## 关键blog
-  https://resources.infosecinstitute.com/topic/system-address-map-initialization-in-x86x64-architecture-part-1-pci-based-systems/

Platform firmware execution can be summarized as follows:
1. CPU 从 reset vector 开始，对于 x86 这个地址是 4GB - 16 byte, 而这个地址一定在 ROM 芯片中间
2. CPU operating mode initialization : 初始化 CPU 为 real mode 还是 flat protected mode
3. CPU microcode update : 使用 ROM 芯片的更新 CPU 的状态
4. Preparation for memory initialization:
   - CPU-specific initialization : 对于 CPU 的一些特殊初始化，比如 cache-as-RAM (CAR), 将 cache 作为内存使用，进而执行初始化内存的代码
   - Chipset initialization. 初始化 chipset 的寄存器，particularly the chipset base address register (BAR).
   - Main memory (RAM) initialization. 初始化内存控制器，
5. Post memory initialization. 
> 下次从这里开始。**WHEN COMMING BACK** 讲解的很清楚啊!

## 关键blog2
- https://resources.infosecinstitute.com/topic/system-address-map-initialization-x86x64-architecture-part-2-pci-express-based-systems/

## Loyenwang
> TODO

[【原创】Linux PCI驱动框架分析（一）](https://www.cnblogs.com/LoyenWang/p/14165852.html)
[【原创】Linux PCI驱动框架分析（二）](https://www.cnblogs.com/LoyenWang/p/14209318.html)


## PCH
[what has happened to north bridge in modern](https://www.reddit.com/r/hardware/comments/bt4xff/what_has_happened_to_north_bridge_in_modern/) :
> partly integrated into the cpu, partly combined with southbridge—> now just called chipset, aka Platform Control Hub (PCH).
> 
> If I'm not mistaken, they were integrated into CPUs. This allows for faster communication between the CPU and other components, because it's closer and on the same silicon die as the CPU.

[Platform Controller Hub](https://en.wikipedia.org/wiki/Platform_Controller_Hub)
> The Platform Controller Hub (PCH) is a family of Intel's single-chip chipsets, first introduced in 2009. It is the successor to the Intel Hub Architecture, which used two chips - a northbridge and southbridge instead, and first appeared in the Intel 5 Series.
>
> As such, I/O functions are reassigned between this new central hub and the CPU compared to the previous architecture: some northbridge functions, the memory controller and PCI-e lanes, were integrated into the CPU while the PCH took over the remaining functions in addition to the traditional roles of the southbridge. AMD has its equivalent for the PCH, known simply as a chipset, no longer using the previous term Fusion controller hub since the release of the Zen architecture in 2017.[1]

[What is the difference between CPU and Chipset?](https://stackoverflow.com/questions/18978503/what-is-the-difference-between-cpu-and-chipset)
> In a mobile phone, combination of Chipset and CPU is called a SoC (System on Chip) which integrates all the components on a single chip. 

> ## Chipset
> - is a set of (chips) electronic components in an integrated circuit known as a "Data Flow Management System" that manages the data flow between the processor, memory and peripherals.
> - usually designed to work with a specific family of microprocessors.
> - historically, chips (for keyboard controller, memory controller, ...) were scattered arround the motherboard. With time, engineers reduced the number of chips to the same job and condenced them to only a few chips or what is now called a __chipset__.
> - recently, the north bridge were built inside the CPU to maximize performance (1 jump instead of 2 from south bridge + performance sensitive devices talks directly to the CPU without the latency addded by north bridge) and the south bridge is called __Platform Controller Hub__.
> 
> copyright: github/cpu-internals


## overview
- [x] https://unix.stackexchange.com/questions/83390/what-are-pci-quirks

[^2]
> Generally speaking, most modern OSes (Windows and Linux) will re-scan the detected hardware as part of the boot sequence. Trusting the BIOS to detect everything and have it setup properly has proven to be unreliable.

1. os 会重新对于所有的设备进行一次扫描，bios 的扫描其实不可靠的
2. ACPI Enumeration - The OS can rely on the BIOS to describe these devices in ASL code.


| File                                                        blank   | comment | code | explanation                     |
|---------------------------------------------------------------------|---------|------|---------------------------------|
| pci.c                                                       950     | 1955    | 3667 |                                 |
| probe.c                                                     543     | 631     | 2137 |                                 |
| controller/pci-hyperv.c                                     454     | 911     | 2133 |                                 |
| setup-bus.c                                                 333     | 373     | 1536 |                                 |
| pci-sysfs.c                                                 244     | 150     | 1170 |                                 |
| pci-driver.c                                                283     | 331     | 1045 | pcie 本身作为 bus driver 的一种 |
| msi.c                                                       258     | 276     | 1045 |                                 |
| pci-acpi.c                                                  205     | 175     | 989  |                                 |
| pcie/aer.c                                                  214     | 229     | 961  |                                 |
| pcie/aspm.c                                                 191     | 245     | 941  |                                 |
| xen-pcifront.c                                              199     | 49      | 934  |                                 |
| iov.c                                                       189     | 177     | 725  |                                 |
| p2pdma.c                                                    164     | 240     | 601  |                                 |
| pci.h                                                        97     | 64      | 532  |                                 |
| vpd.c                                                       101     | 85      | 467  |                                 |
| pcie/aer_inject.c                                            67     | 18      | 460  |                                 |
| slot.c                                                            0 | 0       | 379  |                                 |


## ldd3
sudo insmod pci_skel.ko 之后
```c
/sys/bus/pci/drivers/pci_skel
/sys/module/pci_skel
/sys/module/pci_skel/drivers/pci:pci_skel
```
- [ ] pci_enable_device

- [x] pci_driver 到底指的是控制 pci 设备的软件, 因为在 /sys/bus/pci/drivers/pci_skel 的同级目录下，还可以找到其他各种驱动
  - 而 pci_device 用于抽象对应的设备

### pci_device_id
1. 用于 struct pci_driver 中间, 例如ldd3的例子
2. 被用于告知用户空间特定的设备驱动支持什么设备
```c
static struct pci_driver pci_driver = {
	.name = "pci_skel",
	.id_table = ids,
	.probe = probe,
	.remove = remove,
};
```



### pci_dev

### bus_type
```c
 /* A bus is a channel between the processor and one or more devices. For the
 * purposes of the device model, all devices are connected via a bus, even if
 * it is an internal, virtual, "platform" bus. Buses can plug into each other.
 * A USB controller is usually a PCI device, for example. The device model
 * represents the actual connections between buses and the devices they control.
 * A bus is represented by the bus_type structure. It contains the name, the
 * default attributes, the bus' methods, PM operations, and the driver core's
 * private data.
 */
```


### pci_bus
```c
struct pci_bus {
	struct list_head node;		/* Node in list of buses */
	struct pci_bus	*parent;	/* Parent bus this bridge is on */
	struct list_head children;	/* List of child buses */
	struct list_head devices;	/* List of devices on this bus */
	struct pci_dev	*self;		/* Bridge device as seen by parent */
	struct list_head slots;		/* List of slots on this bus;
					   protected by pci_slot_mutex */
	struct resource *resource[PCI_BRIDGE_RESOURCE_NUM];
	struct list_head resources;	/* Address space routed to this bus */
	struct resource busn_res;	/* Bus numbers routed to this bus */

	struct pci_ops	*ops;		/* Configuration access functions */
	struct msi_controller *msi;	/* MSI controller */
	void		*sysdata;	/* Hook for sys-specific extension */
	struct proc_dir_entry *procdir;	/* Directory entry in /proc/bus/pci */

	unsigned char	number;		/* Bus number */
	unsigned char	primary;	/* Number of primary bridge */
	unsigned char	max_bus_speed;	/* enum pci_bus_speed */
	unsigned char	cur_bus_speed;	/* enum pci_bus_speed */
#ifdef CONFIG_PCI_DOMAINS_GENERIC
	int		domain_nr;
#endif

	char		name[48];

	unsigned short	bridge_ctl;	/* Manage NO_ISA/FBB/et al behaviors */
	pci_bus_flags_t bus_flags;	/* Inherited by child buses */
	struct device		*bridge;
	struct device		dev;
	struct bin_attribute	*legacy_io;	/* Legacy I/O for this bus */
	struct bin_attribute	*legacy_mem;	/* Legacy mem */
	unsigned int		is_added:1;
};
```
pci_read_config_byte 最终利用 pci_bus::ops:read 实现

pci_bus_set_ops :

### pci_host_bridge
- [ ] pci_register_host_bridge
- [ ] pci_alloc_child_bus

### device_driver
```c
/**
 * __pci_register_driver - register a new pci driver
 * @drv: the driver structure to register
 * @owner: owner module of drv
 * @mod_name: module name string
 *
 * Adds the driver structure to the list of registered drivers.
 * Returns a negative value on error, otherwise 0.
 * If no error occurred, the driver remains registered even if
 * no device was claimed during registration.
 */
int __pci_register_driver(struct pci_driver *drv, struct module *owner,
			  const char *mod_name)
{
	/* initialize common driver fields */
	drv->driver.name = drv->name;
	drv->driver.bus = &pci_bus_type;
	drv->driver.owner = owner;
	drv->driver.mod_name = mod_name;
	drv->driver.groups = drv->groups;

	spin_lock_init(&drv->dynids.lock);
	INIT_LIST_HEAD(&drv->dynids.list);

	/* register with core */
	return driver_register(&drv->driver);
}
EXPORT_SYMBOL(__pci_register_driver);
```
1. pci_driver 包含一个 device_driver
- [ ]  `drv->driver.bus = &pci_bus_type;` : 猜测，在 linux 的涉及中间，所有的 device 都是通过这个设备理解的内核
  - [ ] 但是为什么各种 char / block 设备驱动从来没有在乎过 device_driver, 比如 sbull



## pci bar
pci_read_bases

## hotplug
https://github.com/intel/nemu/wiki/ACPI-PCI-discovery-hotplug



## proc and sys
存在的所有 interface :
- /sys/devices/pci0000:00
- /sys/bus/pci
  - devices : 指向 /sys/devices/pci0000:00 的软连接
  - driver : 各种 device 的驱动, 其中包含的文件是同一个模式产生的, bind, unbind 之类的, 这个是 bus.c:bus_add_driver() 产生的 *TODO*
  - slots : 没有任何东西 *TODO*
  - drivers_autoprobe : 下面的几个都是*处理 driver 和 device* 绑定的东西吧？
  - drivers_probe
  - rescan
  - resource_alignment
  - uevent
- /proc/bus/pci : /proc 下面的信息应该都是放在 linux/drivers/pci/proc.c 中间了
  - devices : https://stackoverflow.com/questions/2790637/how-to-interpret-the-contents-of-proc-bus-pci-devices : 是对应的，参考 ldd3 中文版 p303，其中
  - 00 01 02 03: 之类的, 分别对应 bus:dev:function 的 bus 为 00 的 ，在目录记录 dev:functions *TODO* 具体信息暂时看不到.

### lspci
- [ ] lspci 是如何利用 /sys 实现的
    - https://unix.stackexchange.com/questions/121357/is-there-any-substitute-for-lspci

- [ ] host bridge 和 pci bridge ？

```
-> lspci -vmm # 的部分结果
Slot:   01:00.0
Class:  3D controller
Vendor: NVIDIA Corporation
Device: GP108M [GeForce MX150]
SVendor:        Xiaomi
SDevice:        Mi Notebook Pro [GeForce MX150]
Rev:    a1
```

```
➜  lspci
00:00.0 Host bridge: Intel Corporation Xeon E3-1200 v6/7th Gen Core Processor Host Bridge/DRAM Registers (rev 08)
00:02.0 VGA compatible controller: Intel Corporation UHD Graphics 620 (rev 07)
00:08.0 System peripheral: Intel Corporation Xeon E3-1200 v5/v6 / E3-1500 v5 / 6th/7th/8th Gen Core Processor Gaussian Mixture Model
00:14.0 USB controller: Intel Corporation Sunrise Point-LP USB 3.0 xHCI Controller (rev 21)
00:14.2 Signal processing controller: Intel Corporation Sunrise Point-LP Thermal subsystem (rev 21)
00:15.0 Signal processing controller: Intel Corporation Sunrise Point-LP Serial IO I2C Controller #0 (rev 21)
00:15.1 Signal processing controller: Intel Corporation Sunrise Point-LP Serial IO I2C Controller #1 (rev 21)
00:16.0 Communication controller: Intel Corporation Sunrise Point-LP CSME HECI #1 (rev 21)
00:1c.0 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #1 (rev f1)
00:1c.7 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #8 (rev f1)
00:1d.0 PCI bridge: Intel Corporation Sunrise Point-LP PCI Express Root Port #9 (rev f1)
00:1f.0 ISA bridge: Intel Corporation Sunrise Point LPC Controller/eSPI Controller (rev 21)
00:1f.2 Memory controller: Intel Corporation Sunrise Point-LP PMC (rev 21)
00:1f.3 Audio device: Intel Corporation Sunrise Point-LP HD Audio (rev 21)
00:1f.4 SMBus: Intel Corporation Sunrise Point-LP SMBus (rev 21)
01:00.0 3D controller: NVIDIA Corporation GP108M [GeForce MX150] (rev a1)
02:00.0 Network controller: Intel Corporation Wireless 8265 / 8275 (rev 78)
03:00.0 Non-Volatile memory controller: Samsung Electronics Co Ltd NVMe SSD Controller SM961/PM961
```
- 前面都是在同一个 bus 上，最后三个设备分别一个 bus

在 /sys/devices/pci0000:00 中间，找不到后面三个设备，但是
```
/sys/devices/pci0000:00/0000:00:1d.0/0000:03:00.0/nvme
/sys/devices/pci0000:00/0000:00:1c.7/0000:02:00.0/net
/sys/devices/pci0000:00/0000:00:1c.0/0000:01:00.0/drm
```
- [ ] 三个设备分别花在到三个不同的 PCI bridge 上，不会这么巧吧，除非是动态创建的

## configuration space
![loading](https://en.wikipedia.org/wiki/File:Pci-config-space.svg)

- [ ] 每一个设备都有一个 configuration space, 靠什么知道这些 configuration space 在哪里 ?
  - 在内核中间，这些都是靠 pci_ops 实现的，但是这个变量的赋值有点绕，感觉从 kvmtool 入手其实更好
```
➜  ldd3 git:(master) ✗ lspci -x -s 00:02.00
00:02.0 VGA compatible controller: Intel Corporation UHD Graphics 620 (rev 07)
00: 86 80 17 59 07 04 10 00 07 00 00 03 10 00 00 00
10: 04 00 00 b2 00 00 00 00 0c 00 00 c0 00 00 00 00
20: 01 40 00 00 00 00 00 00 00 00 00 00 72 1d 01 17
30: 00 00 00 00 40 00 00 00 00 00 00 00 ff 01 00 00
```

## probe
```c
struct bus_type pci_bus_type = {
	.name		= "pci",
	.match		= pci_bus_match,
	.uevent		= pci_uevent,
	.probe		= pci_device_probe,
	.remove		= pci_device_remove,
	.shutdown	= pci_device_shutdown,
	.dev_groups	= pci_dev_groups,
	.bus_groups	= pci_bus_groups,
	.drv_groups	= pci_drv_groups,
	.pm		= PCI_PM_OPS_PTR,
	.num_vf		= pci_bus_num_vf,
	.dma_configure	= pci_dma_configure,
};
```
- [ ]  pci_bus 的 probe 和 pci_driver 的 probe 是什么关系

> PCI drivers “discover” PCI devices in a system via pci_register_driver(). [^3]

从 pci_register_driver 出发，其实可以很容易的 trace 到 pci_bus_type::probe 调用位置, linux/drivers/base/dd.c:really_probe 中间，而具体的 pci_driver::probe 又是在 pci_driver.c 中间被 pci_bus_type::probe 调用的.

> Once the driver knows about a PCI device and takes ownership, the driver generally needs to perform the following initialization: [^3]
>
> Enable the device
> Request MMIO/IOP resources
> Set the DMA mask size (for both coherent and streaming DMA)
> Allocate and initialize shared control data (pci_allocate_coherent())
> Access device configuration space (if needed)
> Register IRQ handler (request_irq())
> Initialize non-PCI (i.e. LAN/SCSI/etc parts of the chip)
> Enable DMA/processing engines

下面使用 nvme 作为例子来分析一一解释上面的话:
- queue_request_irq : 如果不考虑 msi 之类的，调用 request_threaded_irq 的参数 irq 即使 `dev->irq` *TODO* 所以 desc 的分配是什么什么时候
- nvme_create_io_queues => nvme_alloc_queue => dma_alloc_coherent
- [ ] 好吧，感觉是这个道理，但是代码已经不是这么回事了


## irq
- **写 irq** pci_device_probe : 注册到 pci_bus_type 上, 调用 pci_assign_irq 获取 irq, 并且写入到 configuration space 中间

- [ ] pci_scan_child_bus : *读 irq*
  - [ ] pci_scan_child_bus_extend
      - [ ] pci_scan_bridge_extend : If it's a bridge, configure it and scan the bus behind it.
      - [ ] pci_scan_slot : 用于辅助 https://www.linuxjournal.com/content/jailhouse 的情况
      - [ ] pci_scan_single_device : 如果调用 pci_get_slot 找到了 dev 那么马上又 pci_dev_put, 因为 scan 是为了找到不存在的 device
        - pci_get_slot : 首先在 slot 中间利用 devfn 查找, devfn 在语境中又称为 slot
        - [ ] pci_scan_device 
          - [ ] pci_alloc_dev : malloc 一个 struct pci_dev
          - [ ] pci_setup_device : pci_setup_device - Fill in class and map information of a device
            - [ ] pci_read_irq : 正如 ldd3 中间所说，只有需要中断的 PCI_INTERRUPT_PIN 的才会给中断
pci_scan_child_bus 的调用者很多:
- pci_scan_root_bus_bridge
- pci_scan_root_bus
- pci_scan_bus
- pci_rescan_bus_bridge_resize
跟踪其中一个上去，最后到达了
```c
static struct platform_driver gen_pci_driver = {
	.driver = {
		.name = "pci-host-generic",
		.of_match_table = gen_pci_of_match,
	},
	.probe = pci_host_common_probe,
	.remove = pci_host_common_remove,
};
```
也就是 irq_dev 其实是 configuration space 以及所挂在 bus 的抽象。

- [ ] 上面的分析还是让人疑惑，既然 irq 是 bus_type::probe 中间写入的，那么写入的是 linux irq 还是 hardware irq
  - [ ] 设想是，一个设备挂到 pci 上，pci 说，当你发起中断，我可以保证 configuration space 中间记录的 irq 即使 cpu 的 idt[irq] 相应的


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


[^1]: https://stackoverflow.com/questions/7682422/what-does-request-mem-region-actually-do-and-when-it-is-needed
[^2]: https://stackoverflow.com/questions/18854931/how-does-the-os-detect-hardware
[^3]: https://www.kernel.org/doc/html/latest/PCI/index.html
