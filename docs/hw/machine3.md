## 记录下 windows 真正可用的还需要的东西
1. 额外的一个显示器
2. 尝试用这个解决 windows 的剪切板问题
  1. https://github.com/quackduck/uniclip
3. windows 静态情况存在 20% 的 CPU ，不知道为什么?
4. 是不是传递参数需要将 nv 的声音部分也传递进去吗? 还是说这是自动的?

- [virtio-gpu and qemu graphics in 2021](https://www.kraxel.org/blog/2021/05/virtio-gpu-qemu-graphics-update/)
- [The three(ish) levels of QEMU VM graphics](https://czak.pl/2020/04/09/three-levels-of-qemu-graphics.html)

https://wiki.gentoo.org/wiki/GPU_passthrough_with_libvirt_qemu_kvm

## usb 直通
- https://unix.stackexchange.com/questions/452934/can-i-pass-through-a-usb-port-via-qemu-command-line

```txt
/:  Bus 02.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/9p, 20000M/x2
    |__ Port 8: Dev 2, If 0, Class=Hub, Driver=hub/4p, 5000M
/:  Bus 01.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/16p, 480M
    |__ Port 1: Dev 21, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 1: Dev 21, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 1: Dev 21, If 0, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 2: Dev 3, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 2: Dev 3, If 0, Class=Vendor Specific Class, Driver=, 12M
    |__ Port 7: Dev 15, If 2, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 0, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 3, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 7: Dev 15, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 9: Dev 4, If 0, Class=Hub, Driver=hub/4p, 480M
        |__ Port 4: Dev 20, If 0, Class=Human Interface Device, Driver=usbhid, 12M
        |__ Port 4: Dev 20, If 1, Class=Human Interface Device, Driver=usbhid, 12M
    |__ Port 11: Dev 5, If 0, Class=Human Interface Device, Driver=usbhid, 1.5M
    |__ Port 14: Dev 7, If 0, Class=Wireless, Driver=btusb, 12M
    |__ Port 14: Dev 7, If 1, Class=Wireless, Driver=btusb, 12M
```

```txt
Bus 002 Device 002: ID 174c:3074 ASMedia Technology Inc. ASM1074 SuperSpeed hub
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 001 Device 020: ID 0c45:7638 Microdia AKKO 3084BT
Bus 001 Device 004: ID 174c:2074 ASMedia Technology Inc. ASM1074 High-Speed hub
Bus 001 Device 015: ID 2717:003b Xiaomi Inc. MI Wireless Mouse
Bus 001 Device 003: ID 0b05:19af ASUSTek Computer, Inc. AURA LED Controller
Bus 001 Device 007: ID 8087:0026 Intel Corp. AX201 Bluetooth
Bus 001 Device 005: ID 17ef:6019 Lenovo M-U0025-O Mouse
Bus 001 Device 021: ID 2f68:0082 Hoksi Technology DURGOD Taurus K320
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
```

## 可能是这个原因吗?
```txt
[    0.550542] Unstable clock detected, switching default tracing clock to "global"
```

## 禁用 iommu

网卡无法启动了。
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
[    2.780523] Bluetooth: BNEP (Ethernet Emulation) ver 1.3
[    2.780527] Bluetooth: BNEP socket layer initialized
[    2.781084] Bluetooth: MGMT ver 1.22
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

## [ ] 请问 iommu=off 和 amd_iommu=off 有啥区别?

只是关掉 amd_iommu 的时候:
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

应该是缺少 iommu 导致的
```txt
[  613.669612] vfio-pci: probe of 0000:07:00.0 failed with error -22
```

## [ ] 为什么要强调是通过 cmdline 设置的

```txt
[    0.498658] iommu: Default domain type: Passthrough (set via kernel command line)
```

## [ ] 当 amd_iommu=off 的时候，采用

设置为
```txt
[    0.495931] iommu: Default domain type: Translated
[    0.495931] iommu: DMA domain TLB invalidation policy: lazy mode
```

## [ ] iommu=pt 对于 swiotlb 的影响是什么?
