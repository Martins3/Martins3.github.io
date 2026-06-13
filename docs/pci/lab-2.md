## 问题
- [ ] 整理 bridge 如何模拟的
- [ ] host bridge 和 pci bridge ？

https://stackoverflow.com/questions/25908782/in-linux-is-there-a-way-to-find-out-which-pci-card-is-plugged-into-which-pci-sl

## 先把 PCI 的理念搞清楚
- [ ] CPU 的访存的基本流程是什么，解释 PCH 和 Chipset

### PCH
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

### Chipset
> - is a set of (chips) electronic components in an integrated circuit known as a "Data Flow Management System" that manages the data flow between the processor, memory and peripherals.
> - usually designed to work with a specific family of microprocessors.
> - historically, chips (for keyboard controller, memory controller, ...) were scattered arround the motherboard. With time, engineers reduced the number of chips to the same job and condenced them to only a few chips or what is now called a __chipset__.
> - recently, the north bridge were built inside the CPU to maximize performance (1 jump instead of 2 from south bridge + performance sensitive devices talks directly to the CPU without the latency addded by north bridge) and the south bridge is called __Platform Controller Hub__.
>
> copyright: github/cpu-internals


## misc
System address map initialization in x86/x64 architecture part 1: PCI-based systems 的笔记

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

- https://unix.stackexchange.com/questions/83390/what-are-pci-quirks

## pci bridge
- [ ] /sys/devices/pci0000:00 是个啥，为什么其他的设备
```plain
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

在 /sys/devices/pci0000:00 中间，找不到后面三个设备，但是
```plain
/sys/devices/pci0000:00/0000:00:1d.0/0000:03:00.0/nvme
/sys/devices/pci0000:00/0000:00:1c.7/0000:02:00.0/net
/sys/devices/pci0000:00/0000:00:1c.0/0000:01:00.0/drm
```
## 什么奇怪的 pci 设备都是什么

```txt
02:00.0 Signal processing controller: Huawei Technologies Co., Ltd. iBMA Virtual Network Adapter (rev 01)

74:03.0 SATA controller: Huawei Technologies Co., Ltd. HiSilicon AHCI HBA (rev 21)
b4:03.0 SATA controller: Huawei Technologies Co., Ltd. HiSilicon AHCI HBA (rev 21)

74:04.0 Serial Attached SCSI controller: Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA (rev 21)
b4:02.0 Serial Attached SCSI controller: Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA (rev 21)
74:02.0 Serial Attached SCSI controller: Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA (rev 21)
b4:04.0 Serial Attached SCSI controller: Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA (rev 21)

7b:00.0 System peripheral: Huawei Technologies Co., Ltd. HiSilicon Embedded DMA Engine (rev 21)
bb:00.0 System peripheral: Huawei Technologies Co., Ltd. HiSilicon Embedded DMA Engine (rev 21)
```
System peripheral 和 Signal processing controller 都没驱动，
而 SATA controller 下没有设备

Serial Attached SCSI controller 的驱动是 drivers/scsi/hisi_sas/
hisi_sas_v3_hw

## PCIe 和 dma 的关系
Most PCIe devices are DMA masters, so the driver transfers the command to the device. The device will send several write packets to transmit 4 MiB in xx max sized TLP chunks.
https://stackoverflow.com/questions/27470885/how-does-dma-work-with-pci-express-devices

## 仔细理解下
https://www.zhihu.com/question/550129239/answer/2922654174

## 啊，居然都是买 synopsis 的，不愧是 synopsis 啊
https://www.zhihu.com/question/525350571/answer/2598101335

## 手撸一下 GPU D2D 实现（PCIE 版） - Han Shen 的文章 - 知乎
https://zhuanlan.zhihu.com/p/2847929235

## PLT 是什么
- https://stackoverflow.com/questions/71706706/qemu-pcie-tlp-emulation

### 有趣的两个库
https://github.com/NetTLP/wireshark-nettlp
https://github.com/NetTLP/libtlp

## Riser Card 是什么?

https://support.huawei.com/enterprise/zh/doc/EDOC1100118957/98438122

## amd 的机器上发现 pciehp 也是可以的
```txt
IR-PCI-MSI-0000:00:01.1    0-edge      PCIe PME, aerdrv, pcie-dpc
IR-PCI-MSI-0000:00:01.2    0-edge      PCIe PME, aerdrv, pcie-dpc
IR-PCI-MSI-0000:00:01.6    0-edge      PCIe PME, aerdrv, pcie-dpc
IR-PCI-MSI-0000:00:07.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:00:08.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:20:01.1    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:20:01.2    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:20:01.3    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:20:07.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:20:08.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:40:03.3    0-edge      PCIe PME, aerdrv, pcie-dpc
IR-PCI-MSI-0000:40:07.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:40:08.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:60:03.1    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:60:03.2    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:60:07.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:60:08.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:80:01.1    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:80:01.2    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:80:01.3    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:80:01.4    0-edge      PCIe PME, aerdrv, pciehp, pcie-dpc
IR-PCI-MSI-0000:80:07.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:80:08.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:a0:01.3    0-edge      PCIe PME, aerdrv, pcie-dpc
IR-PCI-MSI-0000:a0:07.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:a0:08.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:c0:07.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:c0:08.1    0-edge      PCIe PME, aerdrv
IR-PCI-MSI-0000:e0:07.1    0-edge      PCIe PME, aerdrv
```

## pcie port vs pcie switch

setup_network_sriov ，为什么只有在 pcie-root-port 的那个 igb 设备下才可以 SRIOV

pcie-root-port 和 pcie bridge 有什么区别?

## 想不到 qemu 有这个设备
hw/misc/pci-testdev.c

## 有趣的东西
https://linux-hardware.org/?view=search


## 这个东西
https://news.ycombinator.com/item?id=46689065
- https://github.com/cakehonolulu/pciem

这个项目，有点像是用户态模拟，但是又有 qemu ，非常值得看看

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
