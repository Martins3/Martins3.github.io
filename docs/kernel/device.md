# linux 设备驱动

- [ ] 将 Linux Knerle Lab 中的内容合并进来
- [ ] 只要要求一个事情，将其中的

## TODO
1. 阅读 /lib/modules/(shell uname -a)/build 下的 makefile 中间的内容
  1. 包含的头文件有点不对 asm 下头文件似乎没有被包含下来
  2. 应该不会指向 glibc 的文件
2. 修改 Makefile　产生的文件放置到指定的文件夹中间去
6. 修改 `scull_load` 文件中间的内容
7. klogd syslogd

## usb
测试辅助模块 `dummy_hcd` 和 `g_zero`

- [ ] https://github.com/gregkh/usbutils : 用户态工具

## device model
- [ ]  [A fresh look at the kernel's device model](https://lwn.net/Articles/645810/)


The simple idea is that the "devices" of the system show up in one single tree at /sys/devices. Devices of a common type have a "subsystem" symlink pointing back to the subsystem's directory. All devices of that subsystem are listed there.
- /sys/devices is the single unified hierarchy of all devices, used to express parent/child relationships
- /sys/{bus,class} is home of the subsystem, the grouping/listing of devices of this type, used to lookup devices by subsystem + name
The distinction of bus vs. class never made much sense. For that reason, udev internally merges the view of them and only exposes "subsystem", it refuses to distinguish "bus" and "class". [^3]

为什么需要设备模型，之前遇到的的问题是什么 ? 存在如下猜测
1. 需要 hot plug ? 所以构建出来一个 bus 类型，并且使用 device 和 device deriver
2. 处理 power 相关的 ? suspend 或者 resume
2. 重复代码消除: 一个设备驱动可以处理不同设备控制器，甚至一个设备驱动可以支持多个类似的设备[^1]

设备模型 : platform bus 用于注销

设备驱动在内核软件的架构:
- a framework that allows the driver to
expose the hardware features in a
generic way.
- a bus infrastructure, part of the device
model, to detect/communicate with the
hardware.

adapter driver are device deriver too !
> 图不错，但是还是理解不了呀
> 是不是存在多种 usb 驱动，那么是不是注册多个 adapter driver ? adapter deriver 的作用是什么 ?


> lld3 上分析的全部内容 :
1. linux 设备模型的核心要素 : bus device device_driver class
2. attribute sfsfs
3. kobject kset **subsystem**
4. hotplug

> wowtech 添加的内容

> 其中的 driver 的内容在哪里 ?

很多 device 不一定就是一个硬件设备，很多时候是作为一个接口，或者是为了使用 sfsfs


- For the same device, need to use the same device driver on……
multiple CPU architectures (x86, ARM…), even though the
hardware controllers are different.
- Need for a single driver to support multiple devices of the
same kind.


#### device
device_unregister 和 device_register 的作用 ?

To work with the attributes, we have structure atribute , DEVICE_ATTR macrodefine for definition, and device_create_file and device_remove_file functions to add the attribute to the device.
> device_create_file 这种函数用于挂载 attr 到

The device_create and device_destroy functions are available for initialization / deinterlacing .

```c
/*
 * The type of device, "struct device" is embedded in. A class
 * or bus can contain devices of different types
 * like "partitions" and "disks", "mouse" and "event".
 * This identifies the device type and carries type-specific
 * information, equivalent to the kobj_type of a kobject.
 * If "name" is specified, the uevent will contain it in
 * the DEVTYPE variable.
 */
struct device_type {
  const char *name;
  const struct attribute_group **groups;
  int (*uevent)(struct device *dev, struct kobj_uevent_env *env);
  char *(*devnode)(struct device *dev, umode_t *mode,
       kuid_t *uid, kgid_t *gid);
  void (*release)(struct device *dev);

  const struct dev_pm_ops *pm; // 电源管理相关
};
```

#### driver
同样存在 driver_register 和 driver_unregister ?
To work with attributes, we have the driver_attribute structure , the macro definition of DRIVER_ATTR for definition, and the driver_create_file and driver_remove_file functions for adding the attribute to the device.


#### bus
bus_register 和 bus_unregister 的作用 ?
bus_create_file

Other possible operations on a bus are browsing the drivers or devices attached to it.
Although we can not directly access them (lists of drives and devices being stored in the private data of the driver, the subsys_private * p field ),
these can be scanned using the `bus_for_each_dev` and `bus_for_each_drv` macrodefines .

The `match` function is used when a new device or a new driver is added to the bus.
Its role is to make a comparison between the device ID and the driver ID.
The `uevent` function is called before generating a hotplug in user-space and has the role of adding environment variables.
> match 函数用于在 device 或者 driver 连接到 bus 上(device 和 device_driver 都是持有 bus_type 这个成员的，当其注册的时候，那么就是自动关联上对应的 bus )，但是 uevent 被 hotplug 触发

struct subsys_private 中各个字段的解释：

subsys、devices_kset、drivers_kset 是三个 kset，kset 是一个特殊的 kobject，用来集合相似的 kobject，它在 sysfs 中也会以目录的形式体现。其中 subsys，代表了本 bus（如/sys/bus/spi），它下面可以包含其它的 kset 或者其它的 kobject；devices_kset 和 drivers_kset 则是 bus 下面的两个 kset（如/sys/bus/spi/devices 和/sys/bus/spi/drivers），分别包括本 bus 下所有的 device 和 device_driver。

interface 是一个 list head，用于保存该 bus 下所有的 interface。有关 interface 的概念后面会详细介绍。

klist_devices 和 klist_drivers 是两个链表，分别保存了本 bus 下所有的 device 和 device_driver 的指针，以方便查找。

drivers_autoprobe，用于控制该 bus 下的 drivers 或者 device 是否自动 probe
bus 和 class 指针，分别保存上层的 bus 或者 class 指针。

wowo : 分析过 bus_register bus_add_device bus_add_driver 的具体实现

#### class
The class_register and class_unregister functions for initialization / and cleanup :

class_create_file_ns

**两套接口原来大家都是存在的呀!**

sysfs 中所有的目录以及目录下的文件都对应内核中的一个 struct kobj 实例。
bus 目录中所有的子目录都对应内核中的一个 struct bus_type 实例。
class 目录中所有的子目录对应内核中的一个 struct class 实例
devices 目录中的所有目录以及子目录都对应一个 struct device 实例

#### resource management
内容在 : drivers/base/devres.c (wowo 还分析过具体的实现，暂时没有阅读的兴趣，至少等待 KVM 和 namespace cgroup 看完再说吧) 提供一种机制，将系统中某个设备的所有资源，以链表的形式，组织起来，以便在 driver detach 的时候，自动释放。

a）power，供电。
b）clock，时钟。
c）memory，内存，在 kernel 中一般使用 kzalloc 分配。
d）GPIO，用户和 CPU 交换简单控制、状态等信息。
e）IRQ，触发中断。
f）DMA，无 CPU 参与情况下进行数据传输。
g）虚拟地址空间，一般使用 ioremap、request_region 等分配。
h）等等

```c
static inline void *devm_kzalloc(struct device *dev, size_t size, gfp_t gfp)
{
  return devm_kmalloc(dev, size, gfp | __GFP_ZERO);
}
```

## char device
- [ ] ldd3 的 chapter 3
- [ ] 检查 `pci_register_driver` 的实现，大致就是调用过程

## ldd3
`pci_register_driver`

sudo insmod pci_skel.ko 之后

```c
/sys/bus/pci/drivers/pci_skel
/sys/module/pci_skel
/sys/module/pci_skel/drivers/pci:pci_skel
```

- [ ] pci_enable_device

- [x] pci_driver 到底指的是控制 pci 设备的软件, 因为在 /sys/bus/pci/drivers/pci_skel 的同级目录下，还可以找到其他各种驱动
  - 而 pci_device 用于抽象对应的设备

## 资源(整理成为 footnote)
1. [linuxjournal : Writing a Simple USB Driver](https://www.linuxjournal.com/article/7353)
2. [linuxjournal : I2C Drivers, Part I](https://www.linuxjournal.com/article/7136)

## bus 的 match

- 现在才知道 virtio 设备同时挂到 PCI 和 virtio bus 上的


- [ ] struct bus_type 的 match 和 probe 是什么关系?
```c
static inline int driver_match_device(struct device_driver *drv,
              struct device *dev)
{
  return drv->bus->match ? drv->bus->match(dev, drv) : 1;
}
```

`device_attach` 是提供给外部的一个常用的函数，会调用 `bus->probe`，在当前的上下文就是 pci_device_probe 了。

- [ ] `pci_device_probe` 的内容是很简单, 根据设备找驱动的地方在哪里？
  - 根据参数 struct device 获取 pc_driver 和 pci_device
  - 分配 irq number
  - 回调 `pci_driver->probe`, 使用 virtio_pci_probe 为例子
      - 初始化 pci 设备
      - 回调 `virtio_driver->probe`, 使用 virtnet_probe 作为例子


# 内核中的驱动

## 到底都是谁在写这些驱动?
相当有意思的问题，这里用脚本分析一下

## 到底有那些驱动

## 到底如何管理这些驱动?
module.md 中分析吧

driver/base 中的文件的确是最核心的，负责 device driver bus class 的抽象，但是也存在两个有趣的文件:
1. memory.c : 内存条的管理
2. cpu.c : cpu 的管理

| File                                    | blank | comment | code | desc                                        |
|-----------------------------------------|-------|---------|------|---------------------------------------------|
| core.c                                  | 505   | 980     | 2294 |  |
| regmap/regmap.c                         | 516   | 502     | 2108 |                                             |
| power/domain.c                          | 572   | 566     | 1951 |                                             |
| power/main.c                            | 342   | 370     | 1424 |                                             |
| power/runtime.c                         | 286   | 531     | 1005 |                                             |
| firmware_loader/main.c                  | 224   | 244     | 1002 |                                             |
| platform.c                              | 201   | 335     | 812  |                                             |
| bus.c                                   | 159   | 241     | 799  |                                             |
| node.c                                  | 141   | 125     | 757  |                                             |
| regmap/regmap-irq.c                     | 135   | 138     | 719  |                                             |
| dd.c                                    | 176   | 363     | 695  |                                             |
| power/wakeup.c                          | 173   | 335     | 674  |                                             |
| power/qos.c                             | 145   | 217     | 620  |                                             |
| swnode.c                                | 161   | 69      | 614  |                                             |
| devres.c                                | 156   | 368     | 593  |                                             |
| memory.c                                | 133   | 155     | 592  |                                             |
| power/sysfs.c                           | 99    | 88      | 555  |                                             |
| regmap/regcache.c                       | 140   | 117     | 528  |                                             |
| cacheinfo.c                             | 101   | 40      | 526  |                                             |
| property.c                              | 123   | 542     | 511  |                                             |
| regmap/regmap-debugfs.c                 | 112   | 56      | 507  |                                             |
| cpu.c                                   | 99    | 48      | 467  |                                             |
| firmware_loader/fallback.c              | 96    | 99      | 461  |                                             |
| component.c                             | 127   | 189     | 460  |                                             |
| regmap/regcache-rbtree.c                | 86    | 34      | 434  |                                             |
| arch_topology.c                         | 95    | 50      | 406  |                                             |
| power/clock_ops.c                       | 105   | 151     | 391  |                                             |
| class.c                                 | 81    | 133     | 373  |                                             |
| devtmpfs.c                              | 67    | 32      | 366  |                                             |
| test/property-entry-test.c              | 104   | 16      | 355  |                                             |
| attribute_container.c                   | 70    | 152     | 322  |                                             |
| regmap/regmap-mmio.c                    | 61    | 6       | 312  |                                             |
| regmap/regcache-lzo.c                   | 52    | 39      | 277  |                                             |
| platform-msi.c                          | 65    | 88      | 259  |                                             |
| regmap/regmap-i2c.c                     | 57    | 10      | 240  |                                             |
| test/test_async_driver_probe.c          | 48    | 29      | 226  |                                             |
| regmap/internal.h                       | 50    | 25      | 222  |                                             |
| devcoredump.c                           | 56    | 78      | 214  |                                             |
| soc.c                                   | 44    | 27      | 200  |                                             |
| power/trace.c                           | 30    | 80      | 183  |                                             |
| power/domain_governor.c                 | 44    | 104     | 173  |                                             |
| regmap/regmap-w1.c                      | 47    | 18      | 172  |                                             |
| power/wakeirq.c                         | 51    | 129     | 169  |                                             |
| regmap/regmap-spmi.c                    | 41    | 17      | 167  |                                             |
| regmap/trace.h                          | 90    | 5       | 163  |                                             |
| power/wakeup_stats.c                    | 34    | 22      | 158  |                                             |
| power/generic_ops.c                     | 47    | 106     | 145  |                                             |
| devcon.c                                | 34    | 53      | 144  |                                             |
| isa.c                                   | 40    | 4       | 140  |                                             |
| driver.c                                | 23    | 70      | 134  |                                             |
| map.c                                   | 15    | 11      | 128  |                                             |
| transport_class.c                       | 30    | 131     | 123  |                                             |
| power/power.h                           | 38    | 7       | 118  |                                             |
| base.h                                  | 19    | 62      | 115  |                                             |
| topology.c                              | 22    | 11      | 110  |                                             |
| firmware_loader/firmware.h              | 22    | 25      | 101  |                                             |
| regmap/regmap-spi.c                     | 24    | 7       | 101  |                                             |
| power/common.c                          | 26    | 109     | 95   |                                             |
| syscore.c                               | 20    | 28      | 82   |                                             |
| power/qos-test.c                        | 19    | 18      | 80   |                                             |
| regmap/regmap-sccb.c                    | 23    | 33      | 72   |                                             |
| regmap/regmap-ac97.c                    | 13    | 5       | 71   |                                             |
| module.c                                | 15    | 8       | 70   |                                             |
| regmap/regmap-sdw.c                     | 18    | 4       | 66   |                                             |
| pinctrl.c                               | 15    | 30      | 60   |                                             |
| regmap/regcache-flat.c                  | 19    | 7       | 57   |                                             |
| regmap/regmap-slimbus.c                 | 16    | 2       | 53   |                                             |
| regmap/regmap-i3c.c                     | 10    | 2       | 48   |                                             |
| firmware_loader/fallback.h              | 10    | 18      | 41   |                                             |
| firmware_loader/fallback_table.c        | 5     | 4       | 40   |                                             |
| firmware_loader/builtin/Makefile        | 7     | 4       | 30   |                                             |
| container.c                             | 9     | 7       | 25   |                                             |
| Makefile                                | 4     | 2       | 24   |                                             |
| init.c                                  | 4     | 15      | 19   |                                             |
| regmap/Makefile                         | 1     | 2       | 16   |                                             |
| firmware.c                              | 3     | 9       | 14   |                                             |
| hypervisor.c                            | 3     | 8       | 13   |                                             |


## Makefile
driver/ 文件夹过于恐怖，但是分析其 Makefile 大多数都是可选的.

```Makefile
# SPDX-License-Identifier: GPL-2.0
#
# Makefile for the Linux kernel device drivers.
#
# 15 Sep 2000, Christoph Hellwig <hch@infradead.org>
# Rewritten to use lists instead of if-statements.
#

obj-y				+= irqchip/
obj-y				+= bus/

obj-$(CONFIG_GENERIC_PHY)	+= phy/

# GPIO must come after pinctrl as gpios may need to mux pins etc
obj-$(CONFIG_PINCTRL)		+= pinctrl/
obj-$(CONFIG_GPIOLIB)		+= gpio/
obj-y				+= pwm/

obj-y				+= pci/

obj-$(CONFIG_PARISC)		+= parisc/
obj-$(CONFIG_RAPIDIO)		+= rapidio/
obj-y				+= video/
obj-y				+= idle/

# IPMI must come before ACPI in order to provide IPMI opregion support
obj-y				+= char/ipmi/

obj-$(CONFIG_ACPI)		+= acpi/
obj-$(CONFIG_SFI)		+= sfi/
# PnP must come after ACPI since it will eventually need to check if acpi
# was used and do nothing if so
obj-$(CONFIG_PNP)		+= pnp/
obj-y				+= amba/

obj-y				+= clk/
# Many drivers will want to use DMA so this has to be made available
# really early.
obj-$(CONFIG_DMADEVICES)	+= dma/

# SOC specific infrastructure drivers.
obj-y				+= soc/

obj-$(CONFIG_VIRTIO)		+= virtio/
obj-$(CONFIG_XEN)		+= xen/

# regulators early, since some subsystems rely on them to initialize
obj-$(CONFIG_REGULATOR)		+= regulator/

# reset controllers early, since gpu drivers might rely on them to initialize
obj-$(CONFIG_RESET_CONTROLLER)	+= reset/

# tty/ comes before char/ so that the VT console is the boot-time
# default.
obj-y				+= tty/
obj-y				+= char/

# iommu/ comes before gpu as gpu are using iommu controllers
obj-$(CONFIG_IOMMU_SUPPORT)	+= iommu/

# gpu/ comes after char for AGP vs DRM startup and after iommu
obj-y				+= gpu/

obj-$(CONFIG_CONNECTOR)		+= connector/

# i810fb and intelfb depend on char/agp/
obj-$(CONFIG_FB_I810)           += video/fbdev/i810/
obj-$(CONFIG_FB_INTEL)          += video/fbdev/intelfb/

obj-$(CONFIG_PARPORT)		+= parport/
obj-$(CONFIG_NVM)		+= lightnvm/
obj-y				+= base/ block/ misc/ mfd/ nfc/
obj-$(CONFIG_LIBNVDIMM)		+= nvdimm/
obj-$(CONFIG_DAX)		+= dax/
obj-$(CONFIG_DMA_SHARED_BUFFER) += dma-buf/
obj-$(CONFIG_NUBUS)		+= nubus/
obj-y				+= macintosh/
obj-$(CONFIG_IDE)		+= ide/
obj-y				+= scsi/
obj-y				+= nvme/
obj-$(CONFIG_ATA)		+= ata/
obj-$(CONFIG_TARGET_CORE)	+= target/
obj-$(CONFIG_MTD)		+= mtd/
obj-$(CONFIG_SPI)		+= spi/
obj-$(CONFIG_SPMI)		+= spmi/
obj-$(CONFIG_HSI)		+= hsi/
obj-$(CONFIG_SLIMBUS)		+= slimbus/
obj-y				+= net/
obj-$(CONFIG_ATM)		+= atm/
obj-$(CONFIG_FUSION)		+= message/
obj-y				+= firewire/
obj-$(CONFIG_UIO)		+= uio/
obj-$(CONFIG_VFIO)		+= vfio/
obj-y				+= cdrom/
obj-y				+= auxdisplay/
obj-$(CONFIG_PCCARD)		+= pcmcia/
obj-$(CONFIG_DIO)		+= dio/
obj-$(CONFIG_SBUS)		+= sbus/
obj-$(CONFIG_ZORRO)		+= zorro/
obj-$(CONFIG_ATA_OVER_ETH)	+= block/aoe/
obj-$(CONFIG_PARIDE) 		+= block/paride/
obj-$(CONFIG_TC)		+= tc/
obj-$(CONFIG_UWB)		+= uwb/
obj-$(CONFIG_USB_PHY)		+= usb/
obj-$(CONFIG_USB)		+= usb/
obj-$(CONFIG_USB_SUPPORT)	+= usb/
obj-$(CONFIG_PCI)		+= usb/
obj-$(CONFIG_USB_GADGET)	+= usb/
obj-$(CONFIG_OF)		+= usb/
obj-$(CONFIG_SERIO)		+= input/serio/
obj-$(CONFIG_GAMEPORT)		+= input/gameport/
obj-$(CONFIG_INPUT)		+= input/
obj-$(CONFIG_RTC_LIB)		+= rtc/
obj-y				+= i2c/ media/
obj-$(CONFIG_PPS)		+= pps/
obj-y				+= ptp/
obj-$(CONFIG_W1)		+= w1/
obj-y				+= power/
obj-$(CONFIG_HWMON)		+= hwmon/
obj-$(CONFIG_THERMAL)		+= thermal/
obj-$(CONFIG_WATCHDOG)		+= watchdog/
obj-$(CONFIG_MD)		+= md/
obj-$(CONFIG_ACCESSIBILITY)	+= accessibility/
obj-$(CONFIG_ISDN)		+= isdn/
obj-$(CONFIG_EDAC)		+= edac/
obj-$(CONFIG_EISA)		+= eisa/
obj-$(CONFIG_PM_OPP)		+= opp/
obj-$(CONFIG_CPU_FREQ)		+= cpufreq/
obj-$(CONFIG_CPU_IDLE)		+= cpuidle/
obj-y				+= mmc/
obj-$(CONFIG_MEMSTICK)		+= memstick/
obj-$(CONFIG_NEW_LEDS)		+= leds/
obj-$(CONFIG_INFINIBAND)	+= infiniband/
obj-$(CONFIG_SGI_SN)		+= sn/
obj-y				+= firmware/
obj-$(CONFIG_CRYPTO)		+= crypto/
obj-$(CONFIG_SUPERH)		+= sh/
ifndef CONFIG_ARCH_USES_GETTIMEOFFSET
obj-y				+= clocksource/
endif
obj-$(CONFIG_DCA)		+= dca/
obj-$(CONFIG_HID)		+= hid/
obj-$(CONFIG_PPC_PS3)		+= ps3/
obj-$(CONFIG_OF)		+= of/
obj-$(CONFIG_SSB)		+= ssb/
obj-$(CONFIG_BCMA)		+= bcma/
obj-$(CONFIG_VHOST_RING)	+= vhost/
obj-$(CONFIG_VHOST)		+= vhost/
obj-$(CONFIG_VLYNQ)		+= vlynq/
obj-$(CONFIG_STAGING)		+= staging/
obj-y				+= platform/

obj-$(CONFIG_MAILBOX)		+= mailbox/
obj-$(CONFIG_HWSPINLOCK)	+= hwspinlock/
obj-$(CONFIG_REMOTEPROC)	+= remoteproc/
obj-$(CONFIG_RPMSG)		+= rpmsg/
obj-$(CONFIG_SOUNDWIRE)		+= soundwire/

# Virtualization drivers
obj-$(CONFIG_VIRT_DRIVERS)	+= virt/
obj-$(CONFIG_HYPERV)		+= hv/

obj-$(CONFIG_PM_DEVFREQ)	+= devfreq/
obj-$(CONFIG_EXTCON)		+= extcon/
obj-$(CONFIG_MEMORY)		+= memory/
obj-$(CONFIG_IIO)		+= iio/
obj-$(CONFIG_VME_BUS)		+= vme/
obj-$(CONFIG_IPACK_BUS)		+= ipack/
obj-$(CONFIG_NTB)		+= ntb/
obj-$(CONFIG_FMC)		+= fmc/
obj-$(CONFIG_POWERCAP)		+= powercap/
obj-$(CONFIG_MCB)		+= mcb/
obj-$(CONFIG_PERF_EVENTS)	+= perf/
obj-$(CONFIG_RAS)		+= ras/
obj-$(CONFIG_THUNDERBOLT)	+= thunderbolt/
obj-$(CONFIG_CORESIGHT)		+= hwtracing/coresight/
obj-y				+= hwtracing/intel_th/
obj-$(CONFIG_STM)		+= hwtracing/stm/
obj-$(CONFIG_ANDROID)		+= android/
obj-$(CONFIG_NVMEM)		+= nvmem/
obj-$(CONFIG_FPGA)		+= fpga/
obj-$(CONFIG_FSI)		+= fsi/
obj-$(CONFIG_TEE)		+= tee/
obj-$(CONFIG_MULTIPLEXER)	+= mux/
obj-$(CONFIG_UNISYS_VISORBUS)	+= visorbus/
obj-$(CONFIG_SIOX)		+= siox/
obj-$(CONFIG_GNSS)		+= gnss/
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
