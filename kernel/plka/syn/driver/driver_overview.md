# driver_overview


我想，只有 base.c 中间的文件
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
driver/ 文件夹过于恐怖，但是分析其Makefile大多数都是可选的.

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
obj-$(CONFIG_BT)		+= bluetooth/
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



