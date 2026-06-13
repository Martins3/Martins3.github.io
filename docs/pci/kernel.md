# Linux Kernel 如何管理 PCIe 设备

| File              | blank | comment | code | explanation            |
|-------------------|-------|---------|------|------------------------|
| pci.c             | 950   | 1955    | 3667 | capability list 的封装 |
| probe.c           | 543   | 631     | 2137 |                        |
| setup-bus.c       | 333   | 373     | 1536 |                        |
| pci-sysfs.c       | 244   | 150     | 1170 |                        |
| pci-driver.c      | 283   | 331     | 1045 |                        |
| msi.c             | 258   | 276     | 1045 |                        |
| pci-acpi.c        | 205   | 175     | 989  |                        |
| pcie/aer.c        | 214   | 229     | 961  |                        |
| pcie/aspm.c       | 191   | 245     | 941  |                        |
| iov.c             | 189   | 177     | 725  |                        |
| p2pdma.c          | 164   | 240     | 601  |                        |
| pci.h             | 97    | 64      | 532  |                        |
| vpd.c             | 101   | 85      | 467  |                        |
| pcie/aer_inject.c | 67    | 18      | 460  |                        |
| slot.c            | 0     | 0       | 379  |                        |
| access.c          |       |         |      | 访问配置空间的函数封装 |
| ecam.c            |       |         |      | ecam 的支持            |

其实，内核中处理 PCIe 的代码并不多。

## 关键结构体
- `pci_dev` : 将配置空间的信息会保存一些到里面
- `pci_bus` : 有点像是在描述 pci bridge 啊，从 `pci_scan_bridge_extend` 看，虽然 bridge 会形成一个 pci bus，但是这个 bus 的属性是通过 `pci_bus` 描述的
- `pci_driver` : 包含一个 `device_driver`
- `pci_host_bridge`
- `pci_slot`

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

- ret_from_fork
  - kernel_init
    - kernel_init_freeable
      - do_basic_setup
        - do_initcalls
          - do_initcall_level
            - do_one_initcall
              - driver_register
                - bus_add_driver
                  - driver_attach
                    - bus_for_each_dev
                      - __driver_attach
                        - __driver_attach
                          - driver_probe_device
                            - __driver_probe_device
                              - really_probe
                                - really_probe
                                  - call_driver_probe
                                    - pci_device_probe
                                      - __pci_device_probe
                                        - pci_call_probe
                                          - local_pci_probe
                                            - e1000_probe

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

## 两个小教程可以玩玩
- drivers/pci/pci-stub.c
- drivers/pci/pci-pf-stub.c

[^3]: https://www.kernel.org/doc/html/latest/PCI/index.html

You will also learn a bit about how PCI handles some nifty kernel features such as
probing and power management.

For an in-depth discussion of PCI, such as device
driver design, PCI bus features, and implementation details, refer to Linux Device
Drivers and *Understanding the Linux Kernel*, as well as PCI specifications.
> 什么，*Understanding the Linux Kernel* 中间 讲过 pci

`pci_device_id`
Device identifier. This is not a local ID used by Linux, but an ID defined accordingly to the PCI standard.

`pci_dev`
Each PCI device is assigned a pci_dev instance, just as network devices are
assigned net_device instances. This is the structure used by the kernel to refer to
a PCI device.

`pci_driver`
Defines the interface between the PCI layer and the device drivers. This structure consists mostly of function pointers. All PCI devices use it.

PCI devices are uniquely identified by a combination of parameters, including vendor, model, etc.

Each device driver registers with the kernel a vector of pci_device_id instances that
lists the IDs of the devices it can handle.

PCI device drivers register and unregister with the kernel with pci_register_driver
and pci_unregister_driver, respectively.

**One of the great advantages of PCI is its elegant support for probing to find the IRQ
and other resources each device needs.**

> The Big Picture 的这一节可以细品

想法: pci 是基础架构，pci 的三层 (事务，路由，物理)内核并不会参与，
但是，哪一个设备 和 对应的驱动关联 (nvme)
资源的分配
需要 pci driver 处理，一旦这些搞定之后，pci driver 就可以退居幕后了

## PCIe 配置空间中是存在

https://stackoverflow.com/questions/72993292/how-to-get-irq-pins-in-pci

## 看看 kernel 中的
```txt
static const struct pci_device_id
```

## 本来以为 reset 是 pcie 的接口，但是发现 virtio 设备都没有这个

```txt
➜  ~ find /sys -name reset
/sys/devices/pci0000:00/0000:00:0c.0/reset
/sys/devices/pci0000:00/0000:00:0b.0/reset
/sys/devices/pci0000:00/0000:00:0a.0/reset
/sys/devices/virtual/block/zram0/reset
/sys/module/i915/parameters/reset
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
