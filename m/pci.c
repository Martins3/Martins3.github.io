#include "internal.h"
#include <linux/pci.h>
#include <asm/topology.h> // 提供 pcibus_to_node

/*
 * [151825.322165] [0000:00]
[151825.323215] Child PCI Bus: 0000:01
[151825.325382] Child PCI Bus: 0000:02
[151825.327550] Child PCI Bus: 0000:03
[151825.329710] [0000:7b]
[151825.330750] [0000:7a]
[151825.331792] [0000:78]
[151825.332832] [0000:7c]
[151825.333872] Child PCI Bus: 0000:7d
[151825.336038] [0000:74]
[151825.337083] Child PCI Bus: 0000:76
[151825.339253] [0000:80]
[151825.340302] Child PCI Bus: 0000:81
[151825.342476] Child PCI Bus: 0000:82
[151825.344648] Child PCI Bus: 0000:83
[151825.346819] Child PCI Bus: 0000:84
[151825.348992] Child PCI Bus: 0000:85
[151825.351163] Child PCI Bus: 0000:86
[151825.353333] Child PCI Bus: 0000:87
[151825.355503] [0000:bb]
[151825.356547] [0000:ba]
[151825.357594] [0000:b8]
[151825.358640] [0000:bc]
[151825.359685] Child PCI Bus: 0000:bd
[151825.361856] [0000:b4]
[151825.362903] Child PCI Bus: 0000:b6

机器上执行 lspci -t 的结果:
1. 显然，和内核中输出的完全对应
2. 有 bus 下面并没有接入一个 pci device ，但是我们依旧可以看到，这些看似是 pci bus ，其实是 pci bridge 的。


-+-[0000:00]-+-08.0-[01]--
 |           +-10.0-[02]----00.0
 |           \-11.0-[03]----00.0
 +-[0000:74]-+-01.0-[76]--
 |           +-02.0
 |           +-03.0
 |           \-04.0
 +-[0000:7a]-+-00.0
 |           +-01.0
 |           \-02.0
 +-[0000:7b]---00.0
 +-[0000:7c]---00.0-[7d]--+-00.0
 |                        +-00.1
 |                        +-00.2
 |                        \-00.3
 +-[0000:80]-+-00.0-[81]--
 |           +-04.0-[82]--+-00.0
 |           |            \-00.1
 |           +-08.0-[83]----00.0
 |           +-0a.0-[84]----00.0
 |           +-0c.0-[85]----00.0
 |           +-0e.0-[86]----00.0
 |           \-10.0-[87]----00.0
 +-[0000:b4]-+-01.0-[b6]--
 |           +-02.0
 |           +-03.0
 |           \-04.0
 +-[0000:ba]-+-00.0
 |           +-01.0
 |           \-02.0
 +-[0000:bb]---00.0
 \-[0000:bc]---00.0-[bd]--


00:08.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
00:10.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
00:11.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
02:00.0 Signal processing controller [1180]: Huawei Technologies Co., Ltd. iBMA Virtual Network Adapter [19e5:1710] (rev 01)
03:00.0 VGA compatible controller [0300]: Huawei Technologies Co., Ltd. Hi171x Series [iBMC Intelligent Management system chip w/VGA support] [19e5:1711] (rev 01)
74:01.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCI-PCI Bridge [19e5:a121] (rev 20)
74:02.0 Serial Attached SCSI controller [0107]: Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA [19e5:a230] (rev 21)
74:03.0 SATA controller [0106]: Huawei Technologies Co., Ltd. HiSilicon AHCI HBA [19e5:a235] (rev 21)
74:04.0 Serial Attached SCSI controller [0107]: Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA [19e5:a230] (rev 21)
7a:00.0 USB controller [0c03]: Huawei Technologies Co., Ltd. HiSilicon USB 1.1 Host Controller [19e5:a23b] (rev 21)
7a:01.0 USB controller [0c03]: Huawei Technologies Co., Ltd. HiSilicon USB 2.0 2-port Host Controller [19e5:a239] (rev 21)
7a:02.0 USB controller [0c03]: Huawei Technologies Co., Ltd. HiSilicon USB 3.0 Host Controller [19e5:a238] (rev 21)
7b:00.0 System peripheral [0880]: Huawei Technologies Co., Ltd. HiSilicon Embedded DMA Engine [19e5:a122] (rev 21)
7c:00.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCI-PCI Bridge [19e5:a121] (rev 20)
7d:00.0 Ethernet controller [0200]: Huawei Technologies Co., Ltd. HNS GE/10GE/25GE RDMA Network Controller [19e5:a222] (rev 21)
7d:00.1 Ethernet controller [0200]: Huawei Technologies Co., Ltd. HNS GE/10GE/25GE Network Controller [19e5:a221] (rev 21)
7d:00.2 Ethernet controller [0200]: Huawei Technologies Co., Ltd. HNS GE/10GE/25GE RDMA Network Controller [19e5:a222] (rev 21)
7d:00.3 Ethernet controller [0200]: Huawei Technologies Co., Ltd. HNS GE/10GE/25GE Network Controller [19e5:a221] (rev 21)
80:00.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
80:04.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
80:08.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
80:0a.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
80:0c.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
80:0e.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
80:10.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCIe Root Port with Gen4 [19e5:a120] (rev 21)
82:00.0 Ethernet controller [0200]: Mellanox Technologies MT27800 Family [ConnectX-5] [15b3:1017]
82:00.1 Ethernet controller [0200]: Mellanox Technologies MT27800 Family [ConnectX-5] [15b3:1017]
83:00.0 Non-Volatile memory controller [0108]: Intel Corporation NVMe Datacenter SSD [3DNAND, Beta Rock Controller] [8086:0a54]
84:00.0 Non-Volatile memory controller [0108]: Intel Corporation NVMe Datacenter SSD [3DNAND, Beta Rock Controller] [8086:0a54]
85:00.0 Non-Volatile memory controller [0108]: Beijing Memblaze Technology Co. Ltd. PBlaze6 6530/6531/6541/6630 [1c5f:000e] (rev 01)
86:00.0 Non-Volatile memory controller [0108]: Beijing Memblaze Technology Co. Ltd. PBlaze6 6530/6531/6541/6630 [1c5f:000e] (rev 01)
87:00.0 RAID bus controller [0104]: Broadcom / LSI MegaRAID Tri-Mode SAS3408 [1000:0017] (rev 01)
b4:01.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCI-PCI Bridge [19e5:a121] (rev 20)
b4:02.0 Serial Attached SCSI controller [0107]: Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA [19e5:a230] (rev 21)
b4:03.0 SATA controller [0106]: Huawei Technologies Co., Ltd. HiSilicon AHCI HBA [19e5:a235] (rev 21)
b4:04.0 Serial Attached SCSI controller [0107]: Huawei Technologies Co., Ltd. HiSilicon SAS 3.0 HBA [19e5:a230] (rev 21)
ba:00.0 USB controller [0c03]: Huawei Technologies Co., Ltd. HiSilicon USB 1.1 Host Controller [19e5:a23b] (rev 21)
ba:01.0 USB controller [0c03]: Huawei Technologies Co., Ltd. HiSilicon USB 2.0 2-port Host Controller [19e5:a239] (rev 21)
ba:02.0 USB controller [0c03]: Huawei Technologies Co., Ltd. HiSilicon USB 3.0 Host Controller [19e5:a238] (rev 21)
bb:00.0 System peripheral [0880]: Huawei Technologies Co., Ltd. HiSilicon Embedded DMA Engine [19e5:a122] (rev 21)
bc:00.0 PCI bridge [0604]: Huawei Technologies Co., Ltd. HiSilicon PCI-PCI Bridge [19e5:a121] (rev 20)

 */
static void iter_bus(void)
{
	// XXX 原来可以通过这个方法来遍历
	struct pci_bus *bus;
	list_for_each_entry(bus, &pci_root_buses, node) {
		struct pci_bus *child;

		// TODO bus->name 反而完全是空的，通过 dev_name 可以获取正确的内容
		pr_info("[%s] %d\n", dev_name(&bus->dev), pcibus_to_node(bus));
		list_for_each_entry(child, &bus->children, node) {
			pr_info("Child PCI Bus: %s %d\n", dev_name(&child->dev),
				pcibus_to_node(child));
		}
	}
}

// XXX 太神奇了，欧耶，可以直接遍历系统中的 pci device
static void iter_pci_dev(void)
{
	struct pci_dev *dev = NULL;
	for_each_pci_dev(dev) {
		struct pci_bus *bus = dev->bus;
		pr_info("Device %04x:%04x on bus %s\n", dev->vendor,
			dev->device, dev_name(&bus->dev));
	}
}

// 	pci_walk_bus(pdev->bus, vfio_pci_walk_wrapper, &walk);
int test_pci(long action)
{
	switch (action) {
	case 0:
		iter_bus();
		break;
	case 1:
		iter_pci_dev();
		break;
	}
	return 0;
}
