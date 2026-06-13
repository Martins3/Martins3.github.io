# virtio-iommu
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
```txt
 -device intel-iommu,intremap=on,caching-mode=on \
```

https://gist.github.com/mcastelino/08f6e49f2faba295eb690a3a8ee44c70

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

似乎这个是不需要的:
```txt
,iommu_platform=true,disable-legacy=on
```
真的有这个参数吗?
## gdb 中分析下


dma_map_page_attrs
```txt
$ p dev->dma_ops
$1 = (const struct dma_map_ops *) 0xffffffff824c1960 <iommu_dma_ops>
```

## 增加一个 virtio-iommu ，为什么有这么大的开机延迟?
```txt
[    0.545222] sd 3:0:0:10: Attached scsi generic sg3 type 0
[    0.545257] sd 3:0:0:10: Power-on or device reset occurred
[    0.546038] sd 3:0:0:10: [sdd] 3774873600 512-byte logical blocks: (1.93 TB/1.76 TiB)
[    0.546848] sd 3:0:0:10: [sdd] Write Protect is off
[    0.547385] sd 3:0:0:10: [sdd] Write cache: enabled, read cache: enabled, doesn't support DPO or FUA
[    0.679263]  sdd: sdd1 sdd2 sdd3
[    0.679646] sd 3:0:0:10: [sdd] Attached SCSI disk
[    0.933552] input: ImExPS/2 Generic Explorer Mouse as /devices/platform/i8042/serio1/input/input3
[    4.217152] virtio-pci 0000:00:04.0: Adding to iommu group 5
[    4.218889] virtio-pci 0000:00:05.0: Adding to iommu group 6
[    4.219447] ACPI: \_SB_.LNKA: Enabled at IRQ 10
[    4.221282] virtio-pci 0000:00:06.0: Adding to iommu group 7
[    4.223271] virtio-pci 0000:00:07.0: Adding to iommu group 8
[    4.224752] virtio-pci 0000:00:09.0: Adding to iommu group 9
[    4.226577] virtio-pci 0000:00:0c.0: Adding to iommu group 10
[    4.246345] virtio-pci 0000:00:0d.0: Adding to iommu group 11
[    4.251415] virtio-pci 0000:00:0e.0: Adding to iommu group 12
[    4.256408] virtio-pci 0000:00:0f.0: Adding to iommu group 13
[    4.257354] PM:   Magic number: 4:70:675
[    4.257594] misc mpt2ctl: hash matches
```

## 原来 iommu domain 的分配是在 pci 探测的时候
```txt
[    3.024665] CPU: 0 UID: 0 PID: 69 Comm: kworker/u258:2 Not tainted 6.12.1 #43
[    3.025136] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[    3.025136] Workqueue: events_unbound deferred_probe_work_func
[    3.025136] Call Trace:
[    3.025136]  <TASK>
[    3.025136]  dump_stack_lvl+0x86/0xc0
[    3.025136]  viommu_domain_alloc+0x11/0x90 [virtio_iommu]
[    3.025136]  __iommu_domain_alloc+0x74/0x140
[    3.025136]  iommu_setup_default_domain+0x2d0/0x540
[    3.028089]  __iommu_probe_device+0x388/0x4a0
[    3.028089]  iommu_probe_device+0x24/0x70
[    3.028089]  acpi_dma_configure_id+0x9e/0xc0
[    3.028089]  pci_dma_configure+0x71/0xc0
[    3.028089]  really_probe+0x9d/0x440
[    3.028089]  __driver_probe_device+0x7c/0x140
[    3.028089]  driver_probe_device+0x1e/0x190
[    3.028089]  __device_attach_driver+0x11e/0x1a0
[    3.028089]  ? __pfx___device_attach_driver+0x10/0x10
[    3.028089]  bus_for_each_drv+0x113/0x170
[    3.028089]  __device_attach+0xc7/0x190
[    3.028089]  bus_probe_device+0x9e/0x120
[    3.028089]  deferred_probe_work_func+0x87/0xd0
[    3.028089]  process_scheduled_works+0x1bc/0x3f0
[    3.028089]  worker_thread+0x2c8/0x370
[    3.028089]  ? __pfx_worker_thread+0x10/0x10
[    3.028089]  kthread+0xf8/0x120
[    3.028089]  ? __pfx_kthread+0x10/0x10
[    3.028089]  ret_from_fork+0x37/0x50
[    3.028089]  ? __pfx_kthread+0x10/0x10
[    3.028089]  ret_from_fork_asm+0x1a/0x30
[    3.028089]  </TASK>
[    3.045092] CPU: 0 UID: 0 PID: 69 Comm: kworker/u258:2 Not tainted 6.12.1 #43
[    3.045689] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[    3.046085] Workqueue: events_unbound deferred_probe_work_func
[    3.046085] Call Trace:
[    3.046085]  <TASK>
[    3.046085]  dump_stack_lvl+0x86/0xc0
[    3.046085]  viommu_attach_dev+0x6a/0x580 [virtio_iommu]
[    3.046085]  ? iommu_create_device_direct_mappings+0x230/0x310
[    3.046085]  __iommu_device_set_domain+0x6e/0x1b0
[    3.046085]  iommu_setup_default_domain+0x434/0x540
[    3.046085]  __iommu_probe_device+0x388/0x4a0
[    3.046085]  iommu_probe_device+0x24/0x70
[    3.046085]  acpi_dma_configure_id+0x9e/0xc0
[    3.046085]  pci_dma_configure+0x71/0xc0
[    3.046085]  really_probe+0x9d/0x440
[    3.046085]  __driver_probe_device+0x7c/0x140
[    3.046085]  driver_probe_device+0x1e/0x190
[    3.046085]  __device_attach_driver+0x11e/0x1a0
[    3.046085]  bus_for_each_drv+0x113/0x170
[    3.046085]  __device_attach+0xc7/0x190
[    3.046085]  bus_probe_device+0x9e/0x120
[    3.046085]  deferred_probe_work_func+0x87/0xd0
[    3.046085]  process_scheduled_works+0x1bc/0x3f0
[    3.046085]  worker_thread+0x2c8/0x370
[    3.046085]  ? __pfx_worker_thread+0x10/0x10
[    3.046085]  kthread+0xf8/0x120
[    3.046085]  ? __pfx_kthread+0x10/0x10
[    3.046085]  ret_from_fork+0x37/0x50
[    3.046085]  ? __pfx_kthread+0x10/0x10
[    3.046085]  ret_from_fork_asm+0x1a/0x30
[    3.046085]  </TASK>
```

启动过程中也会调用这个:
```sh
sudo bpftrace -e "kprobe:viommu_device_group { @[kstack] = count(); }"
```

## mdpy
为什么 mdpy 完全不会有这个? 他甚至不需要 vfio_pci 啊，那么他的 page 是如何映射上的?

```sh
sudo bpftrace -e "kprobe:viommu_map_pages { @[kstack] = count(); }"
```

```txt
@[
    viommu_map_pages+5
    __iommu_map+366
    iommu_map+129
    vfio_pin_map_dma+338
    vfio_iommu_type1_ioctl+3690
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1857
```

mdpy_mmap

### 使用 iova_stress 的也可以

```txt
sudo bpftrace -e "kprobe:viommu_attach_dev { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    viommu_attach_dev+5
    __iommu_device_set_domain+110
    __iommu_group_set_domain_internal+110
    __iommu_take_dma_ownership+420
    iommu_group_claim_dma_owner+64
    vfio_container_attach_group+140
    vfio_group_fops_unl_ioctl+1183
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    viommu_attach_dev+5
    __iommu_device_set_domain+110
    __iommu_group_set_domain_internal+110
    iommu_attach_group+125
    vfio_iommu_type1_attach_group+405
    vfio_fops_unl_ioctl+589
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

原来这样就是一个完整的流程了:
```txt
@[
    viommu_domain_free+5
    vfio_iommu_type1_detach_group+631
    vfio_group_detach_container+82
    vfio_group_fops_release+67
    __fput+133
    task_work_run+137
    do_exit+745
    do_group_exit+120
    get_signal+1707
    arch_do_signal_or_restart+142
    syscall_exit_to_user_mode+85
    do_syscall_64+250
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

所以，大致的逻辑就是有的设备必需在一个 domain 下，有的设备可以配置在一个 domain 下。

## 如何 attach

kernel 侧发送 viommu_attach_dev 发送命令
`VIRTIO_IOMMU_T_ATTACH` 给 qemu ，在 qemu 中使用 virtio_iommu_attach 就可以了。

### 继续推进一步，就很容易可以看到才可以实现多个 iommu_group 共享一个 domain 了

## 64k 也是有这个问题，用 4.19 内核
```txt
[drm] features: -context_init
virtio-pci 0000:00:0c.0: Adding to iommu group 0
virtio-pci 0000:00:0c.0: granule 0x10000 larger than system page size 0x1000
[drm] number of scanouts: 1
------------[ cut here ]------------
[drm] number of cap sets: 0
WARNING: CPU: 7 PID: 82 at drivers/iommu/iommu.c:2979 iommu_setup_default_domain+0x3c4/0x564
[drm] Initialized virtio_gpu 0.1.0 for 0000:00:08.0 on minor 0
Modules linked in: virtio_gpu(+) virtio_dma_buf drm_shmem_helper drm_kms_helper virtio_console drm i2c_core drm_panel_orientation_quirks 9pnet_virtio virtio_balloon virtio_net net_failover failover virtio_blk virtio_iommu vfat fat nls_iso8859_1 nls_cp437 dm_mod bridge stp llc rpcsec_gss_krb5 af_packet auth_rpcgss oid_registry openvswitch libcrc32c crc32c_generic nsh tun 9p 9pnet netfs nfsv4 dns_resolver nfs lockd grace sunrpc configs efivarfs virtio_pci virtio_pci_legacy_dev virtio_pci_modern_dev virtio virtio_ring autofs4
CPU: 7 UID: 0 PID: 82 Comm: kworker/u56:0 Tainted: G        W          6.13.1 #17
Tainted: [W]=WARN
Hardware name: QEMU KVM Virtual Machine, BIOS 0.0.0 02/06/2015
Workqueue: events_unbound deferred_probe_work_func
pstate: 60400005 (nZCv daif +PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : iommu_setup_default_domain+0x3c4/0x564
lr : iommu_setup_default_domain+0x3bc/0x564
sp : ffff800081c6ba60
x29: ffff800081c6ba60 x28: ffff0000c112f600 x27: ffff0000c1351098
x26: ffff8000807e7000 x25: ffff800080976a60 x24: ffff800080e2afdc
x23: 0000000000000000 x22: ffff0000c2737100 x21: ffff0000c2737000
x20: ffff0000c2737048 x19: 00000000ffffffed x18: ffffffffffffffff
x17: 65676170206d6574 x16: 737973206e616874 x15: 2072656772616c20
x14: 3030303031783020 x13: 3030303178302065 x12: 7a69732065676170
x11: 206d657473797320 x10: 6e61687420726567 x9 : 656c756e61726720
x8 : 3a302e63303a3030 x7 : 3a30303030206963 x6 : 702d6f6974726976
x5 : ffff0000fff65908 x4 : 0000000000000000 x3 : 0000000000000000
x2 : 0000000000000000 x1 : 0000000000000000 x0 : 00000000ffffffed
Call trace:
 iommu_setup_default_domain+0x3c4/0x564 (P)
 __iommu_probe_device+0x2f8/0x3a0
 iommu_probe_device+0x34/0x78
 acpi_dma_configure_id+0x8c/0x100
 pci_dma_configure+0xec/0xf4
 really_probe+0x60/0x298
 __driver_probe_device+0x78/0x12c
 driver_probe_device+0x3c/0x15c
 __device_attach_driver+0xb8/0x134
 bus_for_each_drv+0x88/0xe8
 __device_attach+0xa0/0x190
 device_initial_probe+0x14/0x20
 bus_probe_device+0xac/0xb0
 deferred_probe_work_func+0x88/0xc0
 process_one_work+0x144/0x38c
 worker_thread+0x27c/0x458
 kthread+0xe0/0xe4
 ret_from_fork+0x10/0x20
---[ end trace 0000000000000000 ]---
virtio-pci 0000:00:0c.0: enabling device (0000 -> 0003)
virtio-pci 0000:00:0d.0: Adding to iommu group 0
virtio-pci 0000:00:0d.0: granule 0x10000 larger than system page size 0x1000
------------[ cut here ]------------
WARNING: CPU: 7 PID: 82 at drivers/iommu/iommu.c:2979 iommu_setup_default_domain+0x3c4/0x564
Modules linked in: virtio_gpu virtio_dma_buf drm_shmem_helper drm_kms_helper virtio_console drm i2c_core drm_panel_orientation_quirks 9pnet_virtio virtio_balloon virtio_net net_failover failover virtio_blk virtio_iommu vfat fat nls_iso8859_1 nls_cp437 dm_mod bridge stp llc rpcsec_gss_krb5 af_packet auth_rpcgss oid_registry openvswitch libcrc32c crc32c_generic nsh tun 9p 9pnet netfs nfsv4 dns_resolver nfs lockd grace sunrpc configs efivarfs virtio_pci virtio_pci_legacy_dev virtio_pci_modern_dev virtio virtio_ring autofs4
CPU: 7 UID: 0 PID: 82 Comm: kworker/u56:0 Tainted: G        W          6.13.1 #17
Tainted: [W]=WARN
Hardware name: QEMU KVM Virtual Machine, BIOS 0.0.0 02/06/2015
Workqueue: events_unbound deferred_probe_work_func
pstate: 60400005 (nZCv daif +PAN -UAO -TCO -DIT -SSBS BTYPE=--)
pc : iommu_setup_default_domain+0x3c4/0x564
lr : iommu_setup_default_domain+0x3bc/0x564
sp : ffff800081c6ba60
x29: ffff800081c6ba60 x28: ffff0000c112f600 x27: ffff0000c1351098
x26: ffff8000807e7000 x25: ffff800080976a60 x24: ffff800080e2afdc
x23: 0000000000000000 x22: ffff0000c2733500 x21: ffff0000c2733400
x20: ffff0000c2733448 x19: 00000000ffffffed x18: ffffffffffffffff
x17: 65676170206d6574 x16: 737973206e616874 x15: 0773077907730720
x14: 076e076107680774 x13: ffff800080e40b08 x12: 0000000000000a65
x11: 0000000000000377 x10: ffff800080ef0b08 x9 : ffff800080e40b08
x8 : 00000000ffffdfff x7 : ffff800080ef0b08 x6 : 80000000ffffe000
x5 : ffff0000fff65908 x4 : 0000000000000000 x3 : 0000000000000000
x2 : 0000000000000000 x1 : 0000000000000000 x0 : 00000000ffffffed
Call trace:
 iommu_setup_default_domain+0x3c4/0x564 (P)
 __iommu_probe_device+0x2f8/0x3a0
 iommu_probe_device+0x34/0x78
 acpi_dma_configure_id+0x8c/0x100
 pci_dma_configure+0xec/0xf4
 really_probe+0x60/0x298
 __driver_probe_device+0x78/0x12c
 driver_probe_device+0x3c/0x15c
 __device_attach_driver+0xb8/0x134
 bus_for_each_drv+0x88/0xe8
 __device_attach+0xa0/0x190
 device_initial_probe+0x14/0x20
 bus_probe_device+0xac/0xb0
 deferred_probe_work_func+0x88/0xc0
 process_one_work+0x144/0x38c
 worker_thread+0x27c/0x458
 kthread+0xe0/0xe4
 ret_from_fork+0x10/0x20
---[ end trace 0000000000000000 ]---
virtio-pci 0000:00:0d.0: enabling device (0000 -> 0002)
random: crng init done

```

```txt
sudo bpftrace -e "kprobe:viommu_attach_dev { @[kstack] = count(); }"
sudo bpftrace -e "kprobe:viommu_domain_alloc { @[kstack] = count(); }"
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
