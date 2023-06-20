什么参数都不加
```txt
[    0.458729] iommu: Default domain type: Translated
[    0.459021] iommu: DMA domain TLB invalidation policy: lazy mode
```

加上 virtio 之后:
```txt
[    1.799559] iommu: Default domain type: Translated
[    1.803487] iommu: DMA domain TLB invalidation policy: lazy mode
[    2.554608] pci 0000:00:00.0: Adding to iommu group 0
[    2.554817] pci 0000:00:01.0: Adding to iommu group 1
[    2.555020] pci 0000:00:02.0: Adding to iommu group 2
[    2.555223] pci 0000:00:03.0: Adding to iommu group 3
[    2.555421] pci 0000:00:04.0: Adding to iommu group 4
[    2.555620] pci 0000:00:05.0: Adding to iommu group 5
[    2.555825] pci 0000:00:06.0: Adding to iommu group 6
[    2.556026] pci 0000:00:07.0: Adding to iommu group 7
[    2.556230] pci 0000:00:08.0: Adding to iommu group 8
[    2.556431] pci 0000:00:09.0: Adding to iommu group 9
[    2.556639] pci 0000:00:1f.0: Adding to iommu group 10
[    2.556844] pci 0000:00:1f.2: Adding to iommu group 10
[    2.557049] pci 0000:00:1f.3: Adding to iommu group 10
[    2.557252] pci 0000:01:01.0: Adding to iommu group 3
```

## [ ] vIOMMU 和 virtio iommu 是一个东西吗
- QEMU 的解释 : https://wiki.qemu.org/Features/VT-d


https://www.usenix.org/legacy/event/atc11/tech/final_files/Amit.pdf


https://www.youtube.com/watch?v=KlBgB4br1HM

https://www.youtube.com/watch?v=7aZAsanbKwI

## 不添加任何参数
根本不会调用到 iommu_iova_to_phys 中

## 透传 host iommu 进去

这是含有:
```txt
#0  iommu_iova_to_phys (domain=0xffff8880059bcb90, iova=4289724416) at drivers/iommu/iommu.c:2280
#1  0xffffffff819583b2 in iommu_dma_unmap_page (dev=0xffff8880056e80c8, dma_handle=4289724416, size=131072, dir=DMA_FROM_DEVICE, attrs=0) at drivers/iommu/dma-iommu.c:1045
#2  0xffffffff81a253c5 in nvme_pci_unmap_rq (req=0xffff8880116b0000) at drivers/nvme/host/pci.c:975
#3  nvme_complete_batch (fn=<optimized out>, iob=0xffffc900001d4df0, iob@entry=0xffffc900001d4dc8) at drivers/nvme/host/nvme.h:732
#4  nvme_pci_complete_batch (iob=iob@entry=0xffffc900001d4df0) at drivers/nvme/host/pci.c:986
#5  0xffffffff81a263e2 in nvme_irq (irq=<optimized out>, data=<optimized out>) at drivers/nvme/host/pci.c:1087
```

```txt
$3 = (phys_addr_t (*)(struct iommu_domain *,
    dma_addr_t)) 0xffffffff8193ca60 <intel_iommu_iova_to_phys>
```


## 真正的模拟 virtio iommu

arg_machine+=" -device virtio-iommu-pci"

```txt
$ p domain->ops->iova_to_phys
$1 = (phys_addr_t (*)(struct iommu_domain *, dma_addr_t)) 0xffffffff8195be70 <viommu_iova_to_phys>
```

不知道为什么，使用这种方法，启动特别慢。
```txt
[    2.306892] virtio-pci 0000:00:0b.0: Adding to iommu group 8
[    2.307239] iommu: Failed to allocate default IOMMU domain of type 11 for group (null) - Falling back to IOMMU_DOMAIN_DMA
[    2.316790] virtio_blk virtio6: 31/0/0 default/read/poll queues
[    2.322378] virtio_blk virtio6: [vdc] 209715200 512-byte logical blocks (107 GB/100 GiB)
[    2.325449] PM:   Magic number: 11:459:734
[    2.325730] hwmon hwmon1: hash matches
[    2.326050] printk: console [netcon0] enabled
[    2.326327] netconsole: network logging started
[    2.326649] cfg80211: Loading compiled-in X.509 certificates for regulatory database
[    2.329037] modprobe (294) used greatest stack depth: 13376 bytes left
[    2.330604] Loaded X.509 cert 'sforshee: 00b28ddf47aef9cea7'
[    2.331038] platform regulatory.0: Direct firmware load for regulatory.db failed with error -2
[    2.331570] cfg80211: failed to load regulatory.db
[    2.332000] ALSA device list:
[    2.332185]   No soundcards found.
```
在这里会卡很久。

中间存在内核中
```txt
qemu-system-x86_64: virtio_iommu_translate no mapping for 0xa86b100 for sid=32
```

## 虚拟机中测试中断 remapping ?


## 如何实现 int remapping 吗？


## https://michael2012z.medium.com/virtio-iommu-789369049443
