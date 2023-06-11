# Linux Kernel 如何管理 PCIe 设备

假设已经阅读过 [【原创】Linux PCI 驱动框架分析（二）](https://www.cnblogs.com/LoyenWang/p/14209318.html)

在 driver/pci 主要文件的内容:

| File                    | blank | comment | code | explanation            |
|-------------------------|-------|---------|------|------------------------|
| pci.c                   | 950   | 1955    | 3667 | capability list 的封装                        |
| probe.c                 | 543   | 631     | 2137 |                        |
| setup-bus.c             | 333   | 373     | 1536 |                        |
| pci-sysfs.c             | 244   | 150     | 1170 |                        |
| pci-driver.c            | 283   | 331     | 1045 |                        |
| msi.c                   | 258   | 276     | 1045 |                        |
| pci-acpi.c              | 205   | 175     | 989  |                        |
| pcie/aer.c              | 214   | 229     | 961  |                        |
| pcie/aspm.c             | 191   | 245     | 941  |                        |
| iov.c                   | 189   | 177     | 725  |                        |
| p2pdma.c                | 164   | 240     | 601  |                        |
| pci.h                   | 97    | 64      | 532  |                        |
| vpd.c                   | 101   | 85      | 467  |                        |
| pcie/aer_inject.c       | 67    | 18      | 460  |                        |
| slot.c                  | 0     | 0       | 379  |                        |
| access.c                |       |         |      | 访问配置空间的函数封装 |
| ecam.c                  |       |         |      | ecam 的支持            |

其实，内核中处理 PCIe 的代码并不多发。

## 关键结构体
- `pci_dev` : 将配置空间的信息会保存一些到里面
- `pci_bus` : 有点像是在描述 pci bridge 啊，从 `pci_scan_bridge_extend` 看，虽然 bridge 会形成一个 pci bus，但是这个 bus 的属性是通过 `pci_bus` 描述的
- `pci_driver` : 包含一个 `device_driver`
- `pci_host_bridge`
- [ ] `pci_slot`

## scan 的过程

- `pci_scan_child_bus`
  - `pci_scan_child_bus_extend`
      - `pci_scan_bridge_extend` : If it's a bridge, configure it and scan the bus behind it.
      - `pci_scan_slot`
      -  `pci_scan_single_device`
        - `pci_get_slot`
        - `pci_scan_device`
          - `pci_alloc_dev` : 分配一个 `struct pci_dev`
          - `pci_setup_device` : `pci_setup_device` - Fill in class and map information of a device

## probe 的过程

使用 e1000 的为例:
```c
static struct pci_driver e1000_driver = {
	.name     = e1000_driver_name,
	.id_table = e1000_pci_tbl,
	.probe    = e1000_probe,
	.remove   = e1000_remove,
	.driver = {
		.pm = &e1000_pm_ops,
	},
	.shutdown = e1000_shutdown,
	.err_handler = &e1000_err_handler
};
```

```txt
#0  e1000_probe (pdev=0xffff888003699000, ent=0xffffffff822e2f40 <e1000_pci_tbl+640>) at drivers/net/ethernet/intel/e1000e/netdev.c:7408
#1  0xffffffff814abf6d in local_pci_probe (_ddi=_ddi@entry=0xffffc90000013d80) at drivers/pci/pci-driver.c:323
#2  0xffffffff814ad343 in pci_call_probe (id=<optimized out>, dev=0xffff888003699000, drv=<optimized out>) at drivers/pci/pci-driver.c:391
#3  __pci_device_probe (pci_dev=0xffff888003699000, drv=<optimized out>) at drivers/pci/pci-driver.c:416
#4  pci_device_probe (dev=0xffff8880036990c8) at drivers/pci/pci-driver.c:459
#5  0xffffffff81722350 in call_driver_probe (drv=0xffffffff829f3fd8 <e1000_driver+120>, drv=0xffffffff829f3fd8 <e1000_driver+120>, dev=0xffff8880036990c8) at drivers/base/dd.c:542
#6  really_probe (drv=0xffffffff829f3fd8 <e1000_driver+120>, dev=0xffff8880036990c8) at drivers/base/dd.c:621
#7  really_probe (dev=0xffff8880036990c8, drv=0xffffffff829f3fd8 <e1000_driver+120>) at drivers/base/dd.c:566
#8  0xffffffff817224fd in __driver_probe_device (drv=drv@entry=0xffffffff829f3fd8 <e1000_driver+120>, dev=dev@entry=0xffff8880036990c8) at drivers/base/dd.c:752
#9  0xffffffff81722579 in driver_probe_device (drv=drv@entry=0xffffffff829f3fd8 <e1000_driver+120>, dev=dev@entry=0xffff8880036990c8) at drivers/base/dd.c:782
#10 0xffffffff8172288a in __driver_attach (data=0xffffffff829f3fd8 <e1000_driver+120>, dev=0xffff8880036990c8) at drivers/base/dd.c:1141
#11 __driver_attach (dev=0xffff8880036990c8, data=0xffffffff829f3fd8 <e1000_driver+120>) at drivers/base/dd.c:1093
#12 0xffffffff817204a3 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff829f3fd8 <e1000_driver+120>, fn=fn@entry=0xffffffff81722830 <__driver_attach>) at drivers/base/bus.c:301
#13 0xffffffff81721d05 in driver_attach (drv=drv@entry=0xffffffff829f3fd8 <e1000_driver+120>) at drivers/base/dd.c:1158
#14 0xffffffff81721662 in bus_add_driver (drv=drv@entry=0xffffffff829f3fd8 <e1000_driver+120>) at drivers/base/bus.c:618
#15 0xffffffff817230f7 in driver_register (drv=0xffffffff829f3fd8 <e1000_driver+120>) at drivers/base/driver.c:171
#16 0xffffffff81000dbf in do_one_initcall (fn=0xffffffff8300519c <e1000_init_module>) at init/main.c:1298
#17 0xffffffff82fc5405 in do_initcall_level (command_line=0xffff888003546040 "root", level=6) at ./include/linux/compiler.h:234
#18 do_initcalls () at init/main.c:1387
#19 do_basic_setup () at init/main.c:1406
#20 kernel_init_freeable () at init/main.c:1613
#21 0xffffffff81c9e851 in kernel_init (unused=<optimized out>) at init/main.c:1502
#22 0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#23 0x0000000000000000 in ?? ()
```

- PCI drivers “discover” PCI devices in a system via `pci_register_driver`(). [^3]
- `pci_bus::probe`和 `pci_driver::probe` 的关系
- `pci_driver::probe` 中完成的工作

> Once the driver knows about a PCI device and takes ownership, the driver generally needs to perform the following initialization: [^3]
> - Enable the device
> - Request MMIO/IOP resources
> - Set the DMA mask size (for both coherent and streaming DMA)
> - Allocate and initialize shared control data (`pci_allocate_coherent()`)
> - Access device configuration space (if needed)
> - Register IRQ handler (`request_irq()`)
> - Initialize non-PCI (i.e. LAN/SCSI/etc parts of the chip)
> - Enable DMA/processing engines

## [ ] pci 的 remove 和 unbind 的行为差别是什么?

[^3]: https://www.kernel.org/doc/html/latest/PCI/index.html
