# swiotlb
<!-- 6fefcfb6-7d78-444f-a62c-86cf3e0b0f59 -->

## swiotlb 的基本工作原理
<!-- 719adb3e-2383-476d-ba24-760070d7f059 -->

- iommu 的流程 : 通过 map ，会获取到 iova，将 iova 填充到命令中，然后发送给设备。
	由于设备是经过 iommu 访问，通过 iova 可以访问数据

- 而 dma_direct_map_page 映射一个页面，会直接将数据拷贝新的位置
在中断中，驱动会调用 dma_unmap_sg_attrs 这会调用到 swiotlb 的 callback ，这会把数据拷贝


实现源码: kernel/dma/swiotlb.c

经典例子:

 - scsi_queue_rq
   - scsi_dispatch_cmd
     - ata_scsi_queuecmd
       - __ata_scsi_queuecmd
         - ata_scsi_translate
           - ata_qc_issue
             - ata_sg_setup
               - dma_map_sg_attrs
                 - __dma_map_sg_attrs
                   - dma_direct_map_sg
                     - dma_direct_map_page
                       - swiotlb_map

```txt
@[
    dma_unmap_page_attrs+1
    igb_clean_tx_irq+218
    igb_poll+56
    __napi_poll+40
    net_rx_action+606
    handle_softirqs+224
    irq_exit_rcu+158
    common_interrupt+130
    asm_common_interrupt+34
    cpuidle_enter_state+194
    cpuidle_enter+41
    cpuidle_idle_call+245
    do_idle+123
    cpu_startup_entry+38
    start_secondary+277
    secondary_startup_64_no_verify+404
]: 10
```


```
dma_map_page_attrs()              [kernel/dma/mapping.c]
  -> dma_map_phys()
       -> dma_direct_map_phys()   [kernel/dma/direct.h]
            -> 如果 force_bounce / dma_capable 失败 / kmalloc 不对齐
               -> swiotlb_map()   [kernel/dma/swiotlb.c]
                    -> swiotlb_tbl_map_single()
                         -> swiotlb_find_slots()   分配 IO TLB slot
                         -> swiotlb_bounce(DMA_TO_DEVICE)  把原数据拷进 bounce buffer
       -> 返回 bounce buffer 的 dma_addr
```

unmap 时走 `dma_direct_unmap_phys()` -> `swiotlb_tbl_unmap_single()`：

- 如果是 `DMA_FROM_DEVICE` / `DMA_BIDIRECTIONAL`，先 `swiotlb_bounce(DMA_FROM_DEVICE)` 把数据拷回原内存。
- 释放 slot。

`swiotlb_bounce()` 在 `kernel/dma/swiotlb.c:858` 实现，本质上就是根据方向做 `memcpy`（支持 highmem）。

## 1. 设备寻址受限
设备的 `dma_mask`


这是是拯救者笔记本中，当关闭 iommu 的时候
```txt
@[
    swiotlb_tbl_map_single+5
    swiotlb_map+115
    dma_map_page_attrs+268
    page_pool_dma_map+48
    __page_pool_alloc_pages_slow+307
    page_pool_alloc_frag+332
    mt76_dma_rx_fill.isra.0+306
    mt7921_wpdma_reset+408
    mt7921_wpdma_reinit_cond+115
    mt7921e_mcu_drv_pmctrl+33
    mt7921_mcu_drv_pmctrl+56
    mt7921_pm_wake_work+48
    process_one_work+453
    worker_thread+81
    kthread+219
    ret_from_fork+41
]: 42368
```

```txt
@[
    is_swiotlb_active+5 // 跟踪这里，返回值全部是 1
    dma_map_page_attrs+506
    page_pool_dma_map+48
    __page_pool_alloc_pages_slow+301
    page_pool_alloc_frag+324
    mt76_dma_rx_fill.isra.0+303
    mt792x_wpdma_reset+420
    mt792x_wpdma_reinit_cond+125
    mt792xe_mcu_drv_pmctrl+33
    mt792x_mcu_drv_pmctrl+56
    mt792x_pm_wake_work+44
    process_one_work+394
    worker_thread+652
    kthread+244
    ret_from_fork+49
    ret_from_fork_asm+27
```

可以知道路径在这里:
- dma_map_page_attrs
  - dma_direct_map_page
    - swiotlb_map

最新代码里有四条主要路径会进入 swiotlb。

## 2. IOMMU 翻译模式下的 bounce

即使 IOMMU 已启用，当设备是 **untrusted** 或 DMA 缓冲区未按 IOMMU 粒度对齐时，也会先 bounce：

```c
// drivers/iommu/dma-iommu.c
static bool dev_use_swiotlb(struct device *dev, size_t size,
                            enum dma_data_direction dir)
{
    return IS_ENABLED(CONFIG_SWIOTLB) &&
        (dev_is_untrusted(dev) ||
         dma_kmalloc_needs_bounce(dev, size, dir));
}
```

```c
// drivers/iommu/dma-iommu.c:iommu_dma_map_phys()
if (dev_use_swiotlb(dev, size, dir) &&
    iova_unaligned(iovad, phys, size)) {
    ...
    phys = iommu_dma_map_swiotlb(dev, phys, size, dir, attrs);
    ...
}
```
具体分两种情况：

### 2.1 dev_is_untrusted(dev) + 非 page 对齐页面

只针对标记为 untrusted 的 PCI 设备。IOMMU 通常按页映射；
如果 DMA 缓冲区没有页对齐，映射首尾页时可能顺带让设备访问缓冲区之外的内核数 据。

因此先 bounce 到一个隔离缓冲区，并清零首尾 padding，避免不可信设备越界读取或破坏相邻数据。
源码也明确做了 padding 清零

```c
static phys_addr_t iommu_dma_map_swiotlb(struct device *dev, phys_addr_t phys,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	struct iova_domain *iovad = &domain->iova_cookie->iovad;

	if (!is_swiotlb_active(dev)) {
		dev_warn_once(dev, "DMA bounce buffers are inactive, unable to map unaligned transaction.\n");
		return (phys_addr_t)DMA_MAPPING_ERROR;
	}

	trace_swiotlb_bounced(dev, phys, size);

	phys = swiotlb_tbl_map_single(dev, phys, size, iova_mask(iovad), dir,
			attrs);

	/*
	 * Untrusted devices should not see padding areas with random leftover
	 * kernel data, so zero the pre- and post-padding.
	 * swiotlb_tbl_map_single() has initialized the bounce buffer proper to
	 * the contents of the original memory buffer.
	 */
	if (phys != (phys_addr_t)DMA_MAPPING_ERROR && dev_is_untrusted(dev)) {
		size_t start, virt = (size_t)phys_to_virt(phys);

		/* Pre-padding */
		start = iova_align_down(iovad, virt);
		memset((void *)start, 0, virt - start);

		/* Post-padding */
		start = virt + size;
		memset((void *)start, 0, iova_align(iovad, start) - start);
	}

	return phys;
}
```

### 2.2 uncoherent 访问

dma_kmalloc_needs_bounce(dev, size, dir)

这是为了解决非一致性 DMA 设备访问小型 kmalloc() 对象时的 cache line 共享问题。

它等价于：

!dma_kmalloc_safe(dev, dir) &&
!dma_kmalloc_size_aligned(size)

一般在下面条件同时成立时返回 true：

- 启用了 CONFIG_DMA_BOUNCE_UNALIGNED_KMALLOC
- 设备不是 DMA coherent
- 方向不是 DMA_TO_DEVICE，例如 DMA_FROM_DEVICE
- 小型 kmalloc() 对象没有达到 DMA cache-line 安全对齐

例如两个小对象共用一条 cache line：

```txt
cache line:
+----------------+----------------+
| DMA buffer     | another object |
+----------------+----------------+
```

设备向 DMA buffer 写数据后，CPU 为 DMA_FROM_DEVICE 执行 cache invalidation，
可能顺带丢弃相邻对象的脏数据。因此通过 SWIOTLB 中间缓冲区隔离它们。

不过，这个函数返回 true 不代表最终一定 bounce。
实际映射时还会检查物理缓冲区首尾是否相对 IOMMU 页粒度未对齐：

if (dev_use_swiotlb(dev, size, dir) &&
    iova_unaligned(iovad, phys, size))
        iommu_dma_map_swiotlb(...);

## 3. 强制 bounce：内存加密 / `swiotlb=force`

x86 检测到 guest memory encryption 时：

```c
// arch/x86/kernel/pci-dma.c
if (cc_platform_has(CC_ATTR_GUEST_MEM_ENCRYPT)) {
    x86_swiotlb_enable = true;
    x86_swiotlb_flags |= SWIOTLB_FORCE;
}
```

ARM64 Realm world 同理：

```c
// arch/arm64/mm/init.c
if (is_realm_world()) {
    swiotlb = true;
    flags |= SWIOTLB_FORCE;
}
```

`SWIOTLB_FORCE` 会让 `io_tlb_default_mem.force_bounce = true`，
于是 `is_swiotlb_force_bounce(dev)` 永远为真，所有 DMA 都走 bounce。

启动参数也可以强制：

```txt
swiotlb=<size>[,<areas>][,force|noforce]
```

## 经典案例分析

### iommu=off 后，32bit 设备无法工作

AMD 机器上有一个 dma_mask 为 32bit 的网卡:

```txt
[    2.517363] ------------[ cut here ]------------
[    2.517364] mt7921e 0000:04:00.0: DMA addr 0x0000000117eaf000+4096 overflow (mask ffffffff, bus limit 0).
[    2.517367] WARNING: CPU: 19 PID: 1054 at kernel/dma/direct.h:103 dma_map_page_attrs+0x242/0x280
[    2.517371] Modules linked in: ip6_tables xt_conntrack ip6t_rpfilter ipt_rpfilter xt_pkttype xt_LOG nf_log_syslog xt_tcpudp snd_sof_amd_rembrandt mt7921e(+) snd_sof_amd_renoir snd_sof_amd_acp snd_hda_codec_realtek mt7921_common snd_sof_pci snd_sof_xtensa_dsp mt76_connac_lib snd_hda_codec_generic mousedev joydev hid_multitouch nft_compat ledtrig_audio snd_hda_codec_hdmi snd_sof mt76 snd_hda_intel snd_sof_utils snd_intel_dspcfg snd_soc_core snd_intel_sdw_acpi uvcvideo mac80211 snd_hda_codec snd_compress videobuf2_vmalloc ac97_bus uvc videobuf2_memops snd_pcm_dmaengine videobuf2_v4l2 snd_pci_ps snd_rpl_pci_acp6x snd_hda_core snd_acp_pci videodev nf_tables snd_pci_acp6x edac_mce_amd snd_hwdep intel_rapl_msr snd_pci_acp5x edac_core nls_iso8859_1 snd_rn_pci_acp3x snd_pcm intel_rapl_common snd_acp_config nls_cp437 crc32_pclmul videobuf2_common snd_soc_acpi vfat polyval_clmulni sch_fq_codel nfnetlink polyval_generic fat cfg80211 mc hid_generic snd_timer snd_pci_acp3x gf128mul ghash_clmulni_intel btusb snd i2c_hid_acpi r8169
[    2.517389]  sha512_ssse3 sha512_generic sp5100_tco btrtl wdat_wdt ucsi_acpi aesni_intel btbcm typec_ucsi btintel realtek crypto_simd i2c_hid mdio_devres ideapad_laptop wmi_bmof cryptd btmtk rapl typec watchdog libphy k10temp sparse_keymap i2c_piix4 soundcore libarc4 nvidia_drm(PO) roles battery platform_profile bluetooth drm_kms_helper evdev tpm_crb input_leds tiny_power_button led_class tpm_tis tpm_tis_core syscopyarea mac_hid sysfillrect sysimgblt i2c_designware_platform acpi_cpufreq i2c_designware_core ac button usbhid serio_raw ecdh_generic nvidia_modeset(PO) hid rfkill ecc video libaes wmi nvidia_uvm(PO) nvidia(PO) ctr loop xt_nat br_netfilter veth tap macvlan bridge stp llc openvswitch nsh nf_conncount nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 libcrc32c tun kvm_amd ccp kvm drm fuse deflate backlight efi_pstore i2c_core configfs efivarfs tpm rng_core dmi_sysfs ip_tables x_tables autofs4 ext4 crc32c_generic crc16 mbcache jbd2 xhci_pci xhci_pci_renesas xhci_hcd atkbd nvme libps2 vivaldi_fmap usbcore
[    2.517415]  nvme_core t10_pi crc32c_intel crc64_rocksoft crc64 crc_t10dif usb_common crct10dif_generic crct10dif_pclmul i8042 crct10dif_common rtc_cmos serio dm_mod dax vfio_pci vfio_pci_core irqbypass vfio_iommu_type1 vfio iommufd
[    2.517420] CPU: 19 PID: 1054 Comm: (udev-worker) Tainted: P           O       6.3.5 #1-NixOS
[    2.517422] Hardware name: LENOVO 82WM/LNVNB161216, BIOS LPCN41WW 05/24/2023
[    2.517422] RIP: 0010:dma_map_page_attrs+0x242/0x280
[    2.517424] Code: 8b 5d 00 48 89 ef e8 bd 68 63 00 4d 89 e9 4d 89 e0 48 89 da 41 56 48 89 c6 48 c7 c7 40 05 5c a5 48 8d 4c 24 10 e8 9e 8e f4 ff <0f> 0b 58 e9 7b ff ff ff 48 c7 44 24 08 ff ff ff ff 4d 85 c0 0f 84
[    2.517425] RSP: 0018:ffff998b43b3ba08 EFLAGS: 00010282
[    2.517426] RAX: 0000000000000000 RBX: ffff8b6784b5cae0 RCX: 0000000000000027
[    2.517426] RDX: ffff8b766dae14c8 RSI: 0000000000000001 RDI: ffff8b766dae14c0
[    2.517427] RBP: ffff8b6784c070c8 R08: 0000000000000000 R09: ffff998b43b3b8b0
[    2.517427] R10: 0000000000000003 R11: ffffffffa5d38868 R12: 0000000000001000
[    2.517428] R13: 00000000ffffffff R14: 0000000000000000 R15: ffff8b67888f3908
[    2.517428] FS:  00007f14a4b49c40(0000) GS:ffff8b766dac0000(0000) knlGS:0000000000000000
[    2.517429] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[    2.517429] CR2: 00007ff0bce47dfc CR3: 0000000106cf6000 CR4: 0000000000750ee0
[    2.517430] PKRU: 55555554
[    2.517430] Call Trace:
[    2.517431]  <TASK>
[    2.517431]  ? dma_map_page_attrs+0x242/0x280
[    2.517433]  ? __warn+0x81/0x130
[    2.517436]  ? dma_map_page_attrs+0x242/0x280
[    2.517437]  ? report_bug+0x171/0x1a0
[    2.517439]  ? handle_bug+0x41/0x70
[    2.517441]  ? exc_invalid_op+0x17/0x70
[    2.517442]  ? asm_exc_invalid_op+0x1a/0x20
[    2.517445]  ? dma_map_page_attrs+0x242/0x280
[    2.517446]  ? dma_map_page_attrs+0x242/0x280
[    2.517447]  page_pool_dma_map+0x30/0x70
[    2.517449]  __page_pool_alloc_pages_slow+0x133/0x3d0
[    2.517451]  ? sched_clock_cpu+0xf2/0x190
[    2.517453]  page_pool_alloc_frag+0x14c/0x1d0
[    2.517455]  mt76_dma_rx_fill.isra.0+0x132/0x390 [mt76]
[    2.517460]  ? __pfx_mt7921_poll_rx+0x10/0x10 [mt7921e]
[    2.517463]  ? napi_kthread_create+0x48/0x90
[    2.517465]  ? __pfx_mt7921_poll_rx+0x10/0x10 [mt7921e]
[    2.517467]  mt76_dma_init+0x110/0x140 [mt76]
[    2.517471]  ? __pfx_mt7921_rr+0x10/0x10 [mt7921e]
[    2.517473]  mt7921_dma_init+0x18b/0x1f0 [mt7921e]
[    2.517475]  mt7921_pci_probe+0x387/0x430 [mt7921e]
[    2.517477]  local_pci_probe+0x3f/0x90
[    2.517480]  pci_device_probe+0xc3/0x240
[    2.517482]  ? sysfs_do_create_link_sd+0x6e/0xe0
[    2.517484]  really_probe+0x19f/0x400
[    2.517487]  ? __pfx___driver_attach+0x10/0x10
[    2.517488]  __driver_probe_device+0x78/0x160
[    2.517489]  driver_probe_device+0x1f/0x90
[    2.517490]  __driver_attach+0xd2/0x1c0
[    2.517491]  bus_for_each_dev+0x85/0xd0
[    2.517493]  bus_add_driver+0x116/0x220
[    2.517494]  driver_register+0x59/0x100
[    2.517495]  ? __pfx_init_module+0x10/0x10 [mt7921e]
[    2.517497]  do_one_initcall+0x5a/0x240
[    2.517500]  do_init_module+0x4a/0x200
[    2.517501]  __do_sys_init_module+0x17f/0x1b0
[    2.517503]  do_syscall_64+0x3b/0x90
[    2.517505]  entry_SYSCALL_64_after_hwframe+0x72/0xdc
[    2.517507] RIP: 0033:0x7f14a4722b5e
[    2.517531] Code: 48 8b 0d bd e2 0c 00 f7 d8 64 89 01 48 83 c8 ff c3 66 2e 0f 1f 84 00 00 00 00 00 90 f3 0f 1e fa 49 89 ca b8 af 00 00 00 0f 05 <48> 3d 01 f0 ff ff 73 01 c3 48 8b 0d 8a e2 0c 00 f7 d8 64 89 01 48
[    2.517531] RSP: 002b:00007ffda4e74928 EFLAGS: 00000246 ORIG_RAX: 00000000000000af
[    2.517532] RAX: ffffffffffffffda RBX: 00005616c16d0060 RCX: 00007f14a4722b5e
[    2.517532] RDX: 00007f14a4cecb0d RSI: 0000000000019bc8 RDI: 00005616c1ee5290
[    2.517533] RBP: 00007f14a4cecb0d R08: 0000000000000007 R09: 00005616c16ccd20
[    2.517533] R10: 0000000000000005 R11: 0000000000000246 R12: 00005616c1ee5290
[    2.517533] R13: 0000000000000000 R14: 00005616c16bcfe0 R15: 0000000000000000
[    2.517534]  </TASK>
[    2.517534] ---[ end trace 0000000000000000 ]---
[    2.517778] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.592359] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.667874] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.739891] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.782399] NET: Registered PF_ALG protocol family
[    2.812364] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.887399] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    2.962403] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    3.037498] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    3.112444] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    3.152283] Generic FE-GE Realtek PHY r8169-0-700:00: attached PHY driver (mii_bus:phy_addr=r8169-0-700:00, irq=MAC)
[    3.187426] mt7921e 0000:04:00.0: Failed to get patch semaphore
[    3.262368] mt7921e 0000:04:00.0: hardware init failed
[    3.308670] r8169 0000:07:00.0 enp7s0: No native access to PCI extended config space, falling back to CSI
[    3.330433] memfd_create() without MFD_EXEC nor MFD_NOEXEC_SEAL, pid=1582 'systemd'
[    3.331709] r8169 0000:07:00.0 enp7s0: Link is Down
[    3.463048] systemd-journald[944]: /var/log/journal/c4d5dffd3fce45ff9046f3017148dd83/user-1000.journal: Monotonic clock jumped backwards relative to last journal entry, rotating.
[    3.562042] ACPI Warning: \_SB.NPCF._DSM: Argument #4 type mismatch - Found [Buffer], ACPI requires [Package] (20221020/nsarguments-61)
[    3.562087] ACPI Warning: \_SB.PCI0.GPP0.PEGP._DSM: Argument #4 type mismatch - Found [Buffer], ACPI requires [Package] (20221020/nsarguments-61)
```
这需要说到 iommu=off 和 amd_iommu=off 的区别:



- iommu=off：x86 通用参数，禁用内核能使用的所有 IOMMU，包括 AMD-Vi、Intel VT-d、老式 AMD GART IOMMU 等。
- amd_iommu=off：AMD IOMMU 驱动专用参数，只设置 amd_iommu_disabled=true，禁止初始化 AMD-Vi；


其中的关键区别在于 iommu=off 会阻止常规条件下自动初始化 SWIOTLB ，所以


只是关掉 amd_iommu 的时候，swiotlb 还是可以正常工作的:
```txt
[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_nslabs
32768

[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_used
2801

[root@nixos:/sys/kernel/debug/swiotlb]# cat /proc/cmdline
initrd=\efi\nixos\i7ijxg4186dp43vzkavh7gb1vq8dp916-initrd-linux-6.3.5-initrd.efi init=/nix/store/17yknrp3kdnzar0mxymvyca8ppjcgcd8-nixos-system-nixos-23.05
.563.70f7275b32f/init transparent_hugepage=always intel_iommu=on amd_iommu=off ftrace=function_graph ftrace_filter=iommu_setup_dma_ops fsck.mode=force fsc
k.repair=yes loglevel=4
```

这个设备也是可以正常工作的


### AMD 为所有的场景都打开 swiotlb
```txt
commit 121660bba631104154b7c15e88f208c48c8c3297
Author: Mario Limonciello <mario.limonciello@amd.com>
Date:   Tue Apr 5 04:47:22 2022

    iommu/amd: Enable swiotlb in all cases

    Previously the AMD IOMMU would only enable SWIOTLB in certain
    circumstances:
     * IOMMU in passthrough mode
     * SME enabled

    This logic however doesn't work when an untrusted device is plugged in
    that doesn't do page aligned DMA transactions.  The expectation is
    that a bounce buffer is used for those transactions.

    This fails like this:

    swiotlb buffer is full (sz: 4096 bytes), total 0 (slots), used 0 (slots)

    That happens because the bounce buffers have been allocated, followed by
    freed during startup but the bounce buffering code expects that all IOMMUs
    have left it enabled.

    Remove the criteria to set up bounce buffers on AMD systems to ensure
    they're always available for supporting untrusted devices.

    Fixes: 82612d66d51d ("iommu: Allow the dma-iommu api to use bounce buffers")
    Suggested-by: Christoph Hellwig <hch@infradead.org>
    Signed-off-by: Mario Limonciello <mario.limonciello@amd.com>
    Reviewed-by: Robin Murphy <robin.murphy@arm.com>
    Reviewed-by: Christoph Hellwig <hch@lst.de>
    Link: https://lore.kernel.org/r/20220404204723.9767-2-mario.limonciello@amd.com
    Signed-off-by: Joerg Roedel <jroedel@suse.de>
```

我之前很疑惑， 既然有 iommu ，那么还需要是 untrusted ?

因为 iommu 只能处理对齐页面。

以前 AMD IOMMU 只在 passthrough 模式或 SME 启用时才启用 swiotlb。
但当插入一个 untrusted 设备且该设备进行非页对齐 DMA 时，bounce buffer 可能根本不存在，
导致 `swiotlb buffer is full` 错误。

## edu 的实验

启动 qemu 的参数:

```txt
  -device virtio-iommu-pci
  -device edu,dma_mask=0xffffffff
  append ... iommu.passthrough=1
```

然后虚拟机中测试
```sh
cd /home/martins3/data/vn/docs/kernel/iommu/swiotlb/
sudo sh -c ': > /sys/kernel/tracing/trace'
sudo sh -c 'echo 1 > /sys/kernel/tracing/events/swiotlb/enable'
sudo insmod ./swiotlb_edu.ko
sudo dmesg | grep swiotlb_edu
sudo cat /sys/kernel/tracing/trace
sudo grep . /sys/kernel/debug/swiotlb/*
sudo rmmod swiotlb_edu
sudo sh -c 'echo 0 > /sys/kernel/tracing/events/swiotlb/swiotlb_bounced/enable'
```

然后就可以观察到
```txt
# tracer: nop
#
# entries-in-buffer/entries-written: 2/2   #P:8
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
          insmod-1871    [002] .....   285.655295: swiotlb_bounced: dev_name: 0000:00:0a.0 dma_mask=ffffffff dev_addr=17c407000 size=256 NORMAL
          insmod-1871    [002] .....   285.754860: swiotlb_bounced: dev_name: 0000:00:0a.0 dma_mask=ffffffff dev_addr=17c407000 size=256 NORMAL
```

```txt
@[
        swiotlb_map+5
        dma_map_phys+564
        __SCT__tp_func_ovs_dp_upcall+1826
        local_pci_probe+64
        pci_call_probe+96
        pci_device_probe+154
        really_probe+212
        __driver_probe_device+137
        driver_probe_device+36
        __driver_attach+216
        bus_for_each_dev+145
        bus_add_driver+293
        driver_register+121
        do_one_initcall+114
        do_init_module+106
        init_module_from_file+236
        idempotent_init_module+306
        __x64_sys_finit_module+119
        do_syscall_64+171
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

在内核中日志中，可以观察到

```txt
  swiotlb_edu 0000:00:0a.0: DMA_TO_DEVICE original phys=0x0000000104b0c000 mapped dma=0x0000000096e00000 (SWIOTLB bounced)
  swiotlb_edu 0000:00:0a.0: DMA_FROM_DEVICE original phys=0x0000000104b0c000 mapped dma=0x0000000096e00800 (SWIOTLB bounced)
  swiotlb_edu 0000:00:0a.0: SWIOTLB DMA round-trip passed (256 bytes)
```

ftrace：

```txt
  swiotlb_bounced: dev_name: 0000:00:0a.0 dma_mask=ffffffff dev_addr=104b0c000 size=256 NORMAL
  swiotlb_bounced: dev_name: 0000:00:0a.0 dma_mask=ffffffff dev_addr=104b0c000 size=256 NORMAL
```

### 为什么会 bounce

iommu.passthrough=1 让所有设备都走 identity/passthrough domain，DMA 地址等于物理地址。
EDU 设备只有 32-bit DMA mask，而驱动故意分配到 0x104b0c000（超过 4 GiB），
dma_direct_map_phys 里 dma_capable() 失败，只能走 swiotlb_map 把数据 bounce 到 0x96e00000 这种低地址缓冲区。

这就很好的解释了为什么我之前观察到一个现象:

**iommu=pt 导致可能使用上 swiotlb**

```txt
[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_nslabs
32768

[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_used
2877

[root@nixos:/sys/kernel/debug/swiotlb]# cat /proc/cmdline
initrd=\efi\nixos\zkf5v40bsl6lzvgbi9a4mnf5wm55aqh6-initrd-linux-6.3.5-initrd.efi init=/nix/store/np8xhzj01vrhld37vhcd0zlgan1v9kwc-nixos-system-nixos-23.05.563.70f7275b32f/init
 transparent_hugepage=always intel_iommu=on iommu=pt fsck.mode=force fsck.repair=yes loglevel=4
```

## 优化

从 6.6 左右开始，上游引入了 `CONFIG_SWIOTLB_DYNAMIC`，目标是解决“64MB 固定池在嵌入式系统太大、在全量 bounce 场景又太小”的问题。

https://lwn.net/Articles/940973/

## 其他
关于 swiotlb 参数:

- swiotlb=force iommu.passthrough=1 如果打开，所有的 DMA 都是走
	如果 swiotlb=force iommu.passthrough=0 还是会走 iommu 的

## 参考文档
- http://xillybus.com/tutorials/iommu-swiotlb-linux

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
