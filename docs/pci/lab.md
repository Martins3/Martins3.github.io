## pcie bridge 和 pcie port 到底是什么鬼
<!-- 7217ab46-739c-467b-b683-ac56a266fe4d -->

所以，只有这里的东西清楚才可以继续搞 qemu 的 pcie 配置，
或者两者是相辅相成的。

哦，还有 pcie switch

物理机上执行:
```txt
🧀  lspci -t
-[0000:00]-+-00.0
           +-01.0-[01]--+-00.0
           |            \-00.1
           +-06.0-[02]----00.0
           +-0a.0
           +-14.0
           +-14.2
           +-14.3
           +-15.0
           +-15.1
           +-15.2
           +-16.0
           +-17.0
           +-1a.0-[03]----00.0
           +-1c.0-[04]--
           +-1c.1-[05]----00.0
           +-1c.2-[06]----00.0
           +-1d.0-[07]----00.0
           +-1f.0
           +-1f.3
           +-1f.4
           \-1f.5
```

```txt
🤒  lspci | grep bridge
00:00.0 Host bridge: Intel Corporation Device a700 (rev 01)
00:01.0 PCI bridge: Intel Corporation Raptor Lake PCI Express 5.0 Graphics Port (PEG010) (rev 01)
00:06.0 PCI bridge: Intel Corporation Raptor Lake PCIe 4.0 Graphics Port (rev 01)
00:1a.0 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #25 (rev 11)
00:1c.0 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #1 (rev 11)
00:1c.1 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #2 (rev 11)
00:1c.2 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #3 (rev 11)
00:1d.0 PCI bridge: Intel Corporation Device 7ab6 (rev 11)
00:1f.0 ISA bridge: Intel Corporation Device 7a86 (rev 11)
```

尝试将一个 pcie bridge 直通过去，果然失败了
```txt
[85256.511474] vfio-pci 0000:00:1c.0: probe with driver vfio-pci failed with error -22
[85259.359787] vfio-pci 0000:00:1c.0: probe with driver vfio-pci failed with error -22
[85408.394136] vfio-pci 0000:00:1c.0: probe with driver vfio-pci failed with error -22
[85679.315118] vfio-pci 0000:00:1c.0: probe with driver vfio-pci failed with error -22
[85696.858979] vfio-pci 0000:00:1c.0: probe with driver vfio-pci failed with error -22
[85785.305796] vfio-pci 0000:00:1c.0: probe with driver vfio-pci failed with error -22
[85927.328291] vfio-pci 0000:00:1c.0: probe with driver vfio-pci failed with error -22
```

## 有趣的 topo

```txt
🧀  lspci -t -v -PP
-+-[0000:00]-+-08.0-[01]--
 |           +-10.0-[02]----00.0  Huawei Technologies Co., Ltd. iBMA Virtual Network Adapter
 |           \-11.0-[03]----00.0  Huawei Technologies Co., Ltd. Hi171x Series [iBMC Intelligent Management system chip w/VGA support]
 +-[0000:74]-+-01.0-[76]--
 |           +-02.0  Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA
 |           +-03.0  Huawei Technologies Co., Ltd. HiSilicon AHCI HBA
 |           \-04.0  Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA
 +-[0000:7a]-+-00.0  Huawei Technologies Co., Ltd. HiSilicon USB 1.1 Host Controller
 |           +-01.0  Huawei Technologies Co., Ltd. HiSilicon USB 2.0 2-port Host Controller
 |           \-02.0  Huawei Technologies Co., Ltd. HiSilicon USB 3.0 Host Controller
 +-[0000:7b]---00.0  Huawei Technologies Co., Ltd. HiSilicon Embedded DMA Engine
 +-[0000:7c]---00.0-[7d]--+-00.0  Huawei Technologies Co., Ltd. HNS GE/10GE/25GE RDMA Network Controller
 |                        +-00.1  Huawei Technologies Co., Ltd. HNS GE/10GE/25GE Network Controller
 |                        +-00.2  Huawei Technologies Co., Ltd. HNS GE/10GE/25GE RDMA Network Controller
 |                        \-00.3  Huawei Technologies Co., Ltd. HNS GE/10GE/25GE Network Controller
 +-[0000:80]-+-00.0-[81]--
 |           +-04.0-[82]--+-00.0  Mellanox Technologies MT27800 Family [ConnectX-5]
 |           |            +-00.1  Mellanox Technologies MT27800 Family [ConnectX-5]
 |           |            +-01.2  Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
 |           |            +-01.3  Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
 |           |            +-01.4  Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
 |           |            +-01.5  Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
 |           |            +-01.6  Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
 |           |            +-01.7  Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
 |           |            +-02.0  Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
 |           |            \-02.1  Mellanox Technologies MT27800 Family [ConnectX-5 Virtual Function]
 |           +-08.0-[83]----00.0  Intel Corporation NVMe Datacenter SSD [3DNAND, Beta Rock Controller]
 |           +-0a.0-[84]----00.0  Intel Corporation NVMe Datacenter SSD [3DNAND, Beta Rock Controller]
 |           +-0c.0-[85]----00.0  Beijing Memblaze Technology Co. Ltd. PBlaze6 6530/6531/6541/6630
 |           +-0e.0-[86]----00.0  Beijing Memblaze Technology Co. Ltd. PBlaze6 6530/6531/6541/6630
 |           \-10.0-[87]----00.0  Broadcom / LSI MegaRAID Tri-Mode SAS3408
 +-[0000:b4]-+-01.0-[b6]--
 |           +-02.0  Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA
 |           +-03.0  Huawei Technologies Co., Ltd. HiSilicon AHCI HBA
 |           \-04.0  Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA
 +-[0000:ba]-+-00.0  Huawei Technologies Co., Ltd. HiSilicon USB 1.1 Host Controller
 |           +-01.0  Huawei Technologies Co., Ltd. HiSilicon USB 2.0 2-port Host Controller
 |           \-02.0  Huawei Technologies Co., Ltd. HiSilicon USB 3.0 Host Controller
 +-[0000:bb]---00.0  Huawei Technologies Co., Ltd. HiSilicon Embedded DMA Engine
 \-[0000:bc]---00.0-[bd]--
```

- 0000:80:00.0 是 pcie bridge
- 每一个 pcie 设备都是在单独 iommu group 的


华硕机器上:
```txt
🧀  lspci -t -PP -v
-[0000:00]-+-00.0  Intel Corporation Device a700
           +-01.0-[01]--+-00.0  NVIDIA Corporation GP106 [GeForce GTX 1060 3GB]
           |            \-00.1  NVIDIA Corporation GP106 High Definition Audio Controller
           +-02.0  Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770]
           +-06.0-[02]----00.0  Yangtze Memory Technologies Co.,Ltd ZHITAI TiPro7000
           +-0a.0  Intel Corporation Raptor Lake Crashlog and Telemetry
           +-14.0  Intel Corporation Alder Lake-S PCH USB 3.2 Gen 2x2 XHCI Controller
           +-14.2  Intel Corporation Alder Lake-S PCH Shared SRAM
           +-14.3  Intel Corporation Alder Lake-S PCH CNVi WiFi
           +-15.0  Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #0
           +-15.1  Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #1
           +-15.2  Intel Corporation Alder Lake-S PCH Serial IO I2C Controller #2
           +-16.0  Intel Corporation Alder Lake-S PCH HECI Controller #1
           +-17.0  Intel Corporation Alder Lake-S PCH SATA Controller [AHCI Mode]
           +-1a.0-[03]----00.0  Yangtze Memory Technologies Co.,Ltd ZHITAI TiPlus7100
           +-1c.0-[04]--
           +-1c.1-[05]----00.0  Realtek Semiconductor Co., Ltd. RTL8125 2.5GbE Controller
           +-1c.2-[06]----00.0  Intel Corporation Ethernet Controller I225-V
           +-1d.0-[07]----00.0  MAXIO Technology (Hangzhou) Ltd. NVMe SSD Controller MAP1602 (DRAM-less)
           +-1f.0  Intel Corporation Device 7a86
           +-1f.3  Intel Corporation Alder Lake-S HD Audio Controller
           +-1f.4  Intel Corporation Alder Lake-S PCH SMBus Controller
           \-1f.5  Intel Corporation Alder Lake-S PCH SPI Controller
```
```txt
00:00.0 Host bridge: Intel Corporation Device a700 (rev 01)
00:01.0 PCI bridge: Intel Corporation Raptor Lake PCI Express 5.0 Graphics Port (PEG010) (rev 01)
00:02.0 VGA compatible controller: Intel Corporation Raptor Lake-S GT1 [UHD Graphics 770] (rev 04)
00:06.0 PCI bridge: Intel Corporation Raptor Lake PCIe 4.0 Graphics Port (rev 01)
00:0a.0 Signal processing controller: Intel Corporation Raptor Lake Crashlog and Telemetry (rev 01)
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
00:1c.1 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #2 (rev 11)
00:1c.2 PCI bridge: Intel Corporation Alder Lake-S PCH PCI Express Root Port #3 (rev 11)
00:1d.0 PCI bridge: Intel Corporation Device 7ab6 (rev 11)
00:1f.0 ISA bridge: Intel Corporation Device 7a86 (rev 11)
00:1f.3 Audio device: Intel Corporation Alder Lake-S HD Audio Controller (rev 11)
00:1f.4 SMBus: Intel Corporation Alder Lake-S PCH SMBus Controller (rev 11)
00:1f.5 Serial bus controller: Intel Corporation Alder Lake-S PCH SPI Controller (rev 11)
01:00.0 VGA compatible controller: NVIDIA Corporation GP106 [GeForce GTX 1060 3GB] (rev a1)
01:00.1 Audio device: NVIDIA Corporation GP106 High Definition Audio Controller (rev a1)
02:00.0 Non-Volatile memory controller: Yangtze Memory Technologies Co.,Ltd ZHITAI TiPro7000 (rev 01)
03:00.0 Non-Volatile memory controller: Yangtze Memory Technologies Co.,Ltd ZHITAI TiPlus7100 (rev 01)
05:00.0 Ethernet controller: Realtek Semiconductor Co., Ltd. RTL8125 2.5GbE Controller (rev 05)
06:00.0 Ethernet controller: Intel Corporation Ethernet Controller I225-V (rev 03)
07:00.0 Non-Volatile memory controller: MAXIO Technology (Hangzhou) Ltd. NVMe SSD Controller MAP1602 (DRAM-less) (rev 01)
```

这么一看，所有的 pci 设备都是需要插入到 pcie bridge 上的。

但是虚拟机中不是这样的

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
