## bus

```txt
	echo 1 | sudo tee /sys/bus/pci/rescan
```

```txt
🧀  ls /sys/bus/pci
 devices   drivers   drivers_autoprobe   drivers_probe   rescan   resource_alignment   slots   uevent
```

## 理解下 sysfs 的几个接口
drivers/pci/pci-driver.c 对应 /sys/bus/pci/drivers/ 的输出结果

这个没有复杂的映射，似乎要简单很多
```txt
igc
├── bind
├── module -> ../../../../module/igc
├── new_id
├── remove_id
├── uevent
└── unbind
```

## drivers/pci/pci-sysfs.c

https://unix.stackexchange.com/questions/121357/is-there-any-substitute-for-lspci

对应的位置:
/sys/devices/pci0000:00/0000:00:1c.1/0000:05:00.0
```txt
 ari_enabled
 broken_parity_status
 class
 config
 consistent_dma_mask_bits
 current_link_speed
 current_link_width
 d3cold_allowed
 device
 dma_mask_bits
 driver -> ../../../../bus/pci/drivers/r8169
 driver_override
 enable
 firmware_node -> ../../../LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/device:1b/device:1c
 iommu -> ../../../virtual/iommu/dmar0
 iommu_group -> ../../../../kernel/iommu_groups/18
 irq
 link
 local_cpulist
 local_cpus
 max_link_speed
 max_link_width
 mdio_bus
 modalias
 msi_bus
 msi_irqs
 net
 numa_node
 power
 power_state
 remove
 rescan
 reset
 reset_method
 resource
 resource0
 resource2
 resource4
 revision
 subsystem -> ../../../../bus/pci
 subsystem_device
 subsystem_vendor
 uevent
 vendor
 vpd
```
### resource
resource 文件使用的是 resource_show ，展示每一个 resource 的基本情况。

```c
static struct attribute *resource_resize_attrs[] = {
	&dev_attr_resource0_resize.attr,
	&dev_attr_resource1_resize.attr,
	&dev_attr_resource2_resize.attr,
	&dev_attr_resource3_resize.attr,
	&dev_attr_resource4_resize.attr,
	&dev_attr_resource5_resize.attr,
	NULL,
};
```

resource4 resource2 resource0 取决于 PCI 的实际情况，先定义所有的
具体取决于 pci_create_attr
而且还可以 pci_mmap_resource

### msi
1. msi_bus : 是否支持 msi
2. irq : 其实只是对于 msi 有意义
3. msi_irqs : 该目录的实现在 msi_sysfs_populate_desc 中

用 docs/kernel/sysfs-pci.sh 非常奇怪

为什么这个 nvme 设备有那么多中断?
```txt
01:01.0 Non-Volatile memory controller: Red Hat, Inc. QEMU NVM Express Controller (rev 02)
irq : 10
msi_bus : 1
msi_irqs : [65] 25 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 8 5 86 87 88 89 90
```

一般来说，msix 的数量和 hardware queue 数量 + 1 就是了

```sh
		arg_nvme+=" -device nvme,drive=nvme2,max_ioqpairs=14,serial=foo2,id=nvme2 "
		arg_nvme+=" -drive file=${vm_dir}/img/$2,format=qcow2,if=none,id=nvme2"
```
```txt
00:0b.0 Non-Volatile memory controller: Red Hat, Inc. QEMU NVM Express Controller (rev 02)
irq : 11
msi_bus : 1
msi_irqs : [15] 37 39 40 41 42 43 44 45 46 47 48 49 50 51 52
resource  resource0
```

## PCIe 的 reset 需要使用 driver 的行为吧

还是直接操作 PCI config 就可以了

## new_id
/sys/bus/pci/drivers/nvme/new_id

drivers/pci/pci-driver.c 中 new_id_store

这个 1234 1337 是我们自己构建的模拟 GPU :
```sh
echo 1234 1337 | sudo tee /sys/bus/pci/drivers/nvme/new_id
```
甚至可以 echo 任意的东西

但是奇怪的是 nvme 会报错，这是 dmesg 中的
```txt
[ 5488.169178] nvme nvme2: pci function 0000:00:0d.0
[ 5488.172183] nvme nvme2: Device not ready; aborting initialisation, CSTS=0x0
```

这个是 console 中的:
```txt
[ 8538.209958] nvme nvme2: pci function 0000:00:0d.0
reading idx 28 = 0
reading idx 0 = 0
reading idx 4 = 0
reading idx 60 = 0
reading idx 8 = 0
writing addr 20 [reg 5], size 4 = 0
reading idx 28 = 0
writing addr 36 [reg 9], size 4 = 2031647
writing addr 40 [reg 10], size 4 = 1373315072
writing addr 44 [reg 11], size 4 = 0
writing addr 48 [reg 12], size 4 = 1183395840
writing addr 52 [reg 13], size 4 = 0
reading idx 0 = 0
reading idx 4 = 0
writing addr 20 [reg 5], size 4 = 4587520
reading idx 0 = 0
reading idx 4 = 0
writing addr 20 [reg 5], size 4 = 4587521
reading idx 28 = 0
[ 8538.213209] nvme nvme2: Device not ready; aborting initialisation, CSTS=0x0
```




有趣的，原来每一个 bus 类型都是有一个:
```txt
🧀  rg new_id_store
drivers/hv/vmbus_drv.c
686:static ssize_t new_id_store(struct device_driver *driver, const char *buf,

drivers/usb/serial/bus.c
123:static ssize_t new_id_store(struct device_driver *driver,

drivers/hid/hid-core.c
1947:static ssize_t new_id_store(struct device_driver *drv, const char *buf,

drivers/dax/bus.c
110:static ssize_t new_id_store(struct device_driver *drv, const char *buf,

drivers/usb/core/driver.c
138:static ssize_t new_id_store(struct device_driver *driver,

drivers/pcmcia/ds.c
98:new_id_store(struct device_driver *driver, const char *buf, size_t count)

drivers/pci/pci-driver.c
98:static ssize_t new_id_store(struct device_driver *driver, const char *buf,
```
甚至都不认识 pcmcia

为什么 dax 还有?
drivers/dax/bus.c

### pci

pci 要看
```c
	/* Only accept driver_data values that match an existing id_table
	   entry */
```

- 问题 1 : nvme 是有 nvme_id_table 的，为什么还是可以随便 echo ?
```c
static struct pci_driver nvme_driver = {
	.name		= "nvme",
	.id_table	= nvme_id_table,
	.probe		= nvme_probe,
	.remove		= nvme_remove,
	.shutdown	= nvme_shutdown,
	.driver		= {
		.probe_type	= PROBE_PREFER_ASYNCHRONOUS,
#ifdef CONFIG_PM_SLEEP
		.pm		= &nvme_dev_pm_ops,
#endif
	},
	.sriov_configure = pci_sriov_configure_simple,
	.err_handler	= &nvme_err_handler,
};
```

- 问题 2 : 如果一个 driver 有了 id_table ，那么 echo 的意义是什么，反正又不可以添加新的 id ?

在仔细看看 driver data 的含义吧

## 原来 /sys/ 下可以直接设置设备的所在的 numa 的位置啊

numa_node_show
numa_node_store

## 工具
- `lscpi t` shows the bus device tree
- `lspci -x` show device configuration
- `lshw`
- `lspci -x -s 00:02.00` : 查看一个 PCI 设备的配置空间

实现原理参考 [Accessing PCI device resources through sysfs](https://www.kernel.org/doc/html/latest/PCI/sysfs-pci.html)

## /sys/bus/pci/drivers/ 这个目录也是不错的

## proc and sys
存在的所有 interface :
- /sys/devices/pci0000:00

- /sys/bus/pci
  - /devices : 指向 /sys/devices/pci0000:00 的软连接
  - /driver : 具体的驱动通过操作 bind, unbind 来实现驱动绑定
  - /slots
  - drivers_autoprobe
  - drivers_probe
  - rescan
  - resource_alignment
  - uevent


- /proc/bus/pci : /proc 下面的信息应该都是放在 linux/drivers/pci/proc.c 中间了
  - devices : https://stackoverflow.com/questions/2790637/how-to-interpret-the-contents-of-proc-bus-pci-devices : 是对应的，参考 ldd3 中文版 p303，其中
  - 00 01 02 03: 之类的, 分别对应 bus:dev:function 的 bus 为 00 的 ，在目录记录 dev:functions *TODO* 具体信息暂时看不到.

## /sys/bus/pci/slots 目录如何理解?
<!-- f1b9792a-f19f-4e26-8145-69942a7b7d34 -->

应该和和 pci 热插有关，在服务器上测试，结果如下:

```txt
🧀  cat */address
0000:82:00
0000:83:00
0000:ca:00
0000:27:00
0000:a8:00
0000:17:00
0000:81:00
0000:5a:00
0000:38:00
0000:16:00
0000:99:00
0000:b8:00
0000:98:00
0000:5b:00
0000:5c:00
0000:c8:00
0000:c9:00
0000:49:00
0000:d8:00
```
这些地址没有对应设备

但是华硕 /sys/bus/pci/slots 的这个目录就是空的。

## 回答这个问题，然后放到 sysfs 介绍中
https://askubuntu.com/questions/654820/how-to-find-pci-address-of-an-ethernet-interface

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
