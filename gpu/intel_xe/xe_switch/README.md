## 验证命令

```bash
# 快速验证当前驱动
lspci -k -s 00:02.0 | grep "Kernel driver"
# 详细验证
readlink /sys/bus/pci/devices/0000:00:02.0/driver
cat /sys/bus/pci/devices/0000:00:02.0/uevent | grep DRIVER
```

- i915 驱动: `~/data/kernel/default/drivers/gpu/drm/i915/`
- xe 驱动: `~/data/kernel/default/drivers/gpu/drm/xe/`

## 持久化配置

当前切换仅在本次运行有效，重启后将恢复 i915。如需永久切换：

### 方法 1: modprobe 配置
```bash
# 创建配置
sudo tee /etc/modprobe.d/xe-force-probe.conf << 'EOF'
# Force Xe driver for Raptor Lake-S
options xe force_probe=a780

# Blacklist i915
blacklist i915
EOF

# 重新生成 initramfs
sudo dracut --force
```

### 方法 2: 内核启动参数
```bash
# 在 GRUB 配置中添加：
modprobe.blacklist=i915 xe.force_probe=a780
```

agent --resume=188c27a8-2ed1-49c7-b66d-4fa8b2695325

## 参考文档
- [Xe Driver - kernel.org](https://www.kernel.org/doc/html/latest/gpu/driver-xe.html)
- [Intel Graphics - Arch Wiki](https://wiki.archlinux.org/title/Intel_graphics)

## i915 存在这个错误

```txt
[154467.605170] perf: interrupt took too long (7879 > 7876), lowering kernel.perf_event_max_sample_rate to 25000
[175107.467418] wlo1: AP 78:57:73:4d:51:72 changed bandwidth in beacon, new used config is 5300.000 MHz, width 1 (5300.000/0 MHz)
[175113.304193] wlo1: AP 78:57:73:4d:51:72 changed bandwidth in beacon, new used config is 5300.000 MHz, width 2 (5310.000/0 MHz)
[183982.755241] Console: switching to colour dummy device 80x25
[183982.799462] i915 0000:00:02.0: [drm] *ERROR* audio power refcount 1 after unbind
[183983.018237] ------------[ cut here ]------------
[183983.018250] i915 0000:00:02.0: [drm] i915 raw-wakerefs=1 wakelocks=1 on cleanup
[183983.018330] WARNING: CPU: 10 PID: 152419 at drivers/gpu/drm/i915/intel_runtime_pm.c:445 intel_runtime_pm_driver_release+0x79/0x90 [i915]
[183983.018611] Modules linked in: vhost_net vhost vhost_iotlb tap xt_mark veth nf_conntrack_netlink xt_nat xt_conntrack xt_MASQUERADE bridge stp llc xt_set ip_set nft_chain_nat xt_addrtype nft_compat rpcrdma rdma_cm iw_cm ib_cm ib_core binfmt_misc tun overlay nf_tables nfnetlink_cttimeout openvswitch nsh nf_conncount nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 psample qrtr bnep vfat fat snd_sof_pci_intel_tgl snd_sof_pci_intel_cnl snd_sof_intel_hda_generic soundwire_intel snd_sof_intel_hda_sdw_bpt snd_sof_intel_hda_common iwlmvm snd_soc_hdac_hda snd_sof_intel_hda_mlink snd_sof_intel_hda soundwire_cadence intel_rapl_msr snd_sof_pci intel_rapl_common snd_sof_xtensa_dsp intel_uncore_frequency snd_sof intel_uncore_frequency_common intel_tcc_cooling snd_hda_codec_intelhdmi x86_pkg_temp_thermal intel_powerclamp snd_sof_utils mac80211 snd_soc_acpi_intel_match coretemp snd_soc_acpi_intel_sdca_quirks soundwire_generic_allocation snd_soc_acpi crc8 snd_hda_codec_alc662 soundwire_bus snd_hda_codec_realtek_lib kvm_intel
[183983.018693]  snd_hda_codec_generic snd_soc_sdca snd_hda_codec_nvhdmi snd_soc_avs libarc4 snd_hda_codec_hdmi snd_soc_hda_codec snd_hda_intel snd_hda_ext_core kvm snd_hda_codec snd_soc_core snd_hda_core iwlwifi btusb snd_intel_dspcfg snd_compress snd_intel_sdw_acpi ac97_bus btmtk snd_hwdep snd_pcm_dmaengine btrtl snd_seq btbcm irqbypass btintel rapl snd_seq_device iTCO_wdt eeepc_wmi spi_nor asus_wmi mei_pxp mei_hdcp intel_cstate intel_pmc_bxt snd_pcm r8169 ee1004 iTCO_vendor_support sparse_keymap cfg80211 bluetooth intel_uncore igc mtd mei_me snd_timer platform_profile i2c_i801 spi_intel_pci realtek wmi_bmof mei i2c_smbus spi_intel snd idma64 rfkill intel_pmc_core soundcore pmt_telemetry pmt_discovery pmt_class acpi_pad acpi_tad intel_pmc_ssram_telemetry nfsd auth_rpcgss nfs_acl lockd grace nfs_localio sunrpc fuse loop dm_multipath nfnetlink zram lz4hc_compress lz4_compress xfs xe drm_suballoc_helper drm_gpusvm_helper i915 nouveau mxm_wmi drm_ttm_helper gpu_sched drm_gpuvm drm_exec drm_buddy i2c_algo_bit nvme ttm
[183983.018815]  nvme_core drm_display_helper polyval_clmulni nvme_keyring ghash_clmulni_intel cec nvme_auth hkdf intel_vsec video wmi intel_oc_wdt pinctrl_alderlake scsi_dh_rdac scsi_dh_emc scsi_dh_alua i2c_dev
[183983.018864] CPU: 10 UID: 0 PID: 152419 Comm: bash Kdump: loaded Not tainted 6.18.9-100.fc42.x86_64 #1 PREEMPT(lazy)
[183983.018874] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
[183983.018880] RIP: 0010:intel_runtime_pm_driver_release+0x79/0x90 [i915]
[183983.019123] Code: 85 db 75 03 48 8b 1f 44 89 44 24 04 e8 70 fa 5c c1 44 8b 44 24 04 89 e9 48 89 da 48 89 c6 48 c7 c7 80 b8 f8 c0 e8 17 13 9b c0 <0f> 0b 48 83 c4 08 5b 5d c3 cc cc cc cc 66 2e 0f 1f 84 00 00 00 00
[183983.019133] RSP: 0018:ffffc900205d7ab8 EFLAGS: 00010246
[183983.019140] RAX: 0000000000000000 RBX: ffff88810403cf40 RCX: 0000000000000027
[183983.019146] RDX: ffff889ffed1cfc8 RSI: 0000000000000001 RDI: ffff889ffed1cfc0
[183983.019151] RBP: 0000000000000001 R08: 0000000000000000 R09: ffffc900205d7960
[183983.019155] R10: ffffffff8393c2c8 R11: 00000000ffffdfff R12: ffffc900205d7b30
[183983.019161] R13: ffff8881041610c8 R14: 0000000000000202 R15: 0000000000000000
[183983.019166] FS:  00007f2c40050740(0000) GS:ffff88a07a526000(0000) knlGS:0000000000000000
[183983.019172] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[183983.019177] CR2: 000000c0023a3000 CR3: 0000000350bf4006 CR4: 0000000000f72ef0
[183983.019183] PKRU: 55555554
[183983.019187] Call Trace:
[183983.019193]  <TASK>
[183983.019199]  i915_driver_release+0x6b/0x80 [i915]
[183983.019392]  devm_drm_dev_init_release+0x4d/0x80
[183983.019403]  release_nodes+0x38/0xb0
[183983.019413]  devres_release_all+0x94/0x100
[183983.019421]  device_unbind_cleanup+0xe/0x80
[183983.019432]  device_release_driver_internal+0x1c3/0x200
[183983.019439]  unbind_store+0xa4/0xb0
[183983.019448]  kernfs_fop_write_iter+0x14d/0x200
[183983.019457]  vfs_write+0x25d/0x480
[183983.019471]  ksys_write+0x73/0xf0
[183983.019479]  do_syscall_64+0x7e/0x7f0
[183983.019487]  ? mod_memcg_lruvec_state+0xe7/0x2e0
[183983.019496]  ? charge_memcg+0x48/0x80
[183983.019501]  ? blk_cgroup_congested+0x65/0x70
[183983.019512]  ? __lruvec_stat_mod_folio+0x85/0xd0
[183983.019520]  ? __folio_mod_stat+0x2d/0x90
[183983.019529]  ? set_ptes.isra.0+0x36/0x80
[183983.019535]  ? do_anonymous_page+0x100/0x520
[183983.019544]  ? __handle_mm_fault+0x551/0x6a0
[183983.019552]  ? filp_flush+0x5b/0x80
[183983.019563]  ? count_memcg_events+0xd6/0x220
[183983.019571]  ? handle_mm_fault+0x248/0x360
[183983.019580]  ? do_user_addr_fault+0x21a/0x690
[183983.019590]  ? irqentry_exit_to_user_mode+0x2c/0x1c0
[183983.019597]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[183983.019605] RIP: 0033:0x7f2c400c173e
[183983.019649] Code: 4d 89 d8 e8 d4 bc 00 00 4c 8b 5d f8 41 8b 93 08 03 00 00 59 5e 48 83 f8 fc 74 11 c9 c3 0f 1f 80 00 00 00 00 48 8b 45 10 0f 05 <c9> c3 83 e2 39 83 fa 08 75 e7 e8 13 ff ff ff 0f 1f 00 f3 0f 1e fa
[183983.019657] RSP: 002b:00007ffc1663f5d0 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[183983.019665] RAX: ffffffffffffffda RBX: 000000000000000d RCX: 00007f2c400c173e
[183983.019670] RDX: 000000000000000d RSI: 000055d1f7016ab0 RDI: 0000000000000001
[183983.019675] RBP: 00007ffc1663f5e0 R08: 0000000000000000 R09: 0000000000000000
[183983.019680] R10: 0000000000000000 R11: 0000000000000202 R12: 000000000000000d
[183983.019685] R13: 000055d1f7016ab0 R14: 00007f2c4023b5c0 R15: 0000000000000000
[183983.019692]  </TASK>
[183983.019696] ---[ end trace 0000000000000000 ]---
[183989.512652] xe 0000:00:02.0: Your graphics device a780 is not officially supported
                by xe driver in this kernel version. To force Xe probe,
                use xe.force_probe='a780' and i915.force_probe='!a780'
                module parameters or CONFIG_DRM_XE_FORCE_PROBE='a780' and
                CONFIG_DRM_I915_FORCE_PROBE='!a780' configuration options.
[183998.240145] Setting dangerous option force_probe - tainting kernel
[183998.240533] xe 0000:00:02.0: vgaarb: deactivate vga console
[183998.240658] xe 0000:00:02.0: [drm] Running in SR-IOV PF mode
[183998.240662] xe 0000:00:02.0: [drm] Found alderlake_s/raptorlake_s (device ID a780) integrated display version 12.00 stepping D0
[183998.248918] xe 0000:00:02.0: [drm] Finished loading DMC firmware i915/adls_dmc_ver2_01.bin (v2.1)
[183998.253342] xe 0000:00:02.0: [drm] Tile0: GT0: Using GuC firmware from i915/tgl_guc_70.bin version 70.49.4
[183998.261442] xe 0000:00:02.0: [drm] Tile0: GT0: Using HuC firmware from i915/tgl_huc.bin version 7.9.3
[183998.344172] xe 0000:00:02.0: [drm] Tile0: GT0: vcs1 fused off
[183998.344176] xe 0000:00:02.0: [drm] Tile0: GT0: vcs3 fused off
[183998.344177] xe 0000:00:02.0: [drm] Tile0: GT0: vcs4 fused off
[183998.344178] xe 0000:00:02.0: [drm] Tile0: GT0: vcs5 fused off
[183998.344179] xe 0000:00:02.0: [drm] Tile0: GT0: vcs6 fused off
[183998.344180] xe 0000:00:02.0: [drm] Tile0: GT0: vcs7 fused off
[183998.344181] xe 0000:00:02.0: [drm] Tile0: GT0: vecs1 fused off
[183998.344181] xe 0000:00:02.0: [drm] Tile0: GT0: vecs2 fused off
[183998.344182] xe 0000:00:02.0: [drm] Tile0: GT0: vecs3 fused off
[183998.351112] mei_hdcp 0000:00:16.0-b638ab7e-94e2-4ea2-a552-d1c54b627f04: bound 0000:00:02.0 (ops i915_hdcp_ops [xe])
[183998.351633] xe 0000:00:02.0: [drm] Registered 4 planes with drm panic
[183998.351635] [drm] Initialized xe 1.1.0 for 0000:00:02.0 on minor 0
[183998.352146] ACPI: video: Video Device [GFX0] (multi-head: yes  rom: no  post: no)
[183998.352323] input: Video Bus as /devices/LNXSYSTM:00/LNXSYBUS:00/PNP0A08:00/LNXVIDEO:00/input/input20
[183998.352451] snd_hda_intel 0000:00:1f.3: bound 0000:00:02.0 (ops intel_audio_component_bind_ops [xe])
[183998.387421] xe 0000:00:02.0: [drm] Allocated fbdev into stolen
[183998.391231] fbcon: xedrmfb (fb0) is primary device
[183998.421733] xe 0000:00:02.0: [drm] Reducing the compressed framebuffer size. This may lead to less power savings than a non-reduced-size. Try to increase stolen memory size if available in BIOS.
[183998.455215] Console: switching to colour frame buffer device 480x135
[183998.482876] xe 0000:00:02.0: [drm] fb0: xedrmfb frame buffer device
```

## 虚拟机中探测 xe 存在这个错误

当时的内核版本比较低
```txt
[ 9244.749333] Setting dangerous option force_probe - tainting kernel
[ 9244.749842] xe 0000:00:07.0: vgaarb: deactivate vga console
[ 9244.751034] xe 0000:00:07.0: [drm] Found alderlake_s/raptorlake_s (device ID a780) integrated display version 12.00 stepping D0
[ 9244.801194] xe 0000:00:07.0: [drm] *ERROR* GT0: Force wake domain 0 failed to ack wake (-ETIMEDOUT) reg[0x130044] = 0x0
[ 9244.801215] ------------[ cut here ]------------
[ 9244.801217] xe 0000:00:07.0: [drm] GT0: Forcewake domain 0x1 failed to acknowledge awake request
[ 9244.801287] WARNING: CPU: 28 PID: 21035 at drivers/gpu/drm/xe/xe_force_wake.c:205 xe_force_wake_get+0x28a/0x2c0 [xe]
[ 9244.801371] Modules linked in: xe(+) drm_gpuvm gpu_sched drm_ttm_helper drm_suballoc_helper rpcsec_gss_krb5 auth_rpcgss nfsv4 dns_resolver nfs lockd grace nfs_localio netfs overlay uinput drm_exec rfkill sunrpc nf_conntrack_netbios_ns nf_conntrack_broadcast nft_fib_inet nft_fib_ipv4 nft_fib_ipv6 nft_fib nft_reject_inet nf_reject_ipv4 nf_reject_ipv6 nft_reject nft_ct nft_chain_nat nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 nf_tables qrtr vfat fat intel_rapl_msr intel_rapl_common intel_uncore_frequency_common intel_pmc_core pmt_telemetry pmt_discovery pmt_class intel_pmc_ssram_telemetry intel_vsec kvm_intel kvm irqbypass rapl i915 drm_buddy ttm i2c_algo_bit drm_display_helper cec video pcspkr virtio_input virtio_balloon i2c_piix4 wmi i2c_smbus joydev loop nfnetlink vsock_loopback vmw_vsock_virtio_transport_common vmw_vsock_vmci_transport vsock zram lz4hc_compress vmw_vmci lz4_compress virtio_net virtio_gpu net_failover polyval_clmulni failover ghash_clmulni_intel virtio_dma_buf virtio_scsi ata_generic pata_acpi
[ 9244.801412]  serio_raw i2c_dev qemu_fw_cfg virtiofs fuse [last unloaded: drm_gpuvm]
[ 9244.801419] CPU: 28 UID: 0 PID: 21035 Comm: modprobe Tainted: G     U              6.17.1-300.fc43.x86_64 #1 PREEMPT(lazy)
[ 9244.801422] Tainted: [U]=USER
[ 9244.801422] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS 0.0.0 02/06/2015
[ 9244.801424] RIP: 0010:xe_force_wake_get+0x28a/0x2c0 [xe]
[ 9244.801474] Code: 85 ed 75 03 4c 8b 2f 89 0c 24 e8 e1 a3 d9 d6 8b 0c 24 41 89 d9 4d 89 f8 4c 89 ea 48 89 c6 48 c7 c7 e0 8e 76 c1 e8 96 50 1a d6 <0f> 0b e9 68 ff ff ff 48 8b 74 24 20 48 8b 7c 24 08 e8 00 23 36 d7
[ 9244.801475] RSP: 0018:ffffcf398c31b970 EFLAGS: 00010246
[ 9244.801476] RAX: 0000000000000000 RBX: 0000000000000001 RCX: 0000000000000027
[ 9244.801477] RDX: ffff8cd1b7c1cf88 RSI: 0000000000000001 RDI: ffff8cd1b7c1cf80
[ 9244.801478] RBP: 0000000000000000 R08: 0000000000000000 R09: ffffcf398c31b818
[ 9244.801478] R10: ffffffff99b399e8 R11: 00000000ffffdfff R12: ffff8cd0bb8f8098
[ 9244.801479] R13: ffff8cd0826e6e60 R14: ffff8cd0bb8f80ac R15: ffffffffc175ae8a
[ 9244.801479] FS:  00007f5ce5ccb740(0000) GS:ffff8cd21d247000(0000) knlGS:0000000000000000
[ 9244.801480] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[ 9244.801480] CR2: 00007ffe55bcbf58 CR3: 0000000111bbe006 CR4: 0000000000f72ef0
[ 9244.801484] PKRU: 55555554
[ 9244.801485] Call Trace:
[ 9244.801487]  <TASK>
[ 9244.801491]  xe_gt_init_early+0xc2/0x120 [xe]
[ 9244.801543]  xe_device_probe+0x11b/0x7c0 [xe]
[ 9244.801594]  xe_pci_probe+0x535/0x6f0 [xe]
[ 9244.801665]  local_pci_probe+0x3f/0x90
[ 9244.801678]  pci_call_probe+0x5b/0x190
[ 9244.801679]  ? kernfs_create_link+0x61/0xb0
[ 9244.801686]  pci_device_probe+0x95/0x140
[ 9244.801687]  really_probe+0xdb/0x340
[ 9244.801696]  ? pm_runtime_barrier+0x55/0x90
[ 9244.801699]  __driver_probe_device+0x78/0x140
[ 9244.801701]  driver_probe_device+0x1f/0xa0
[ 9244.801703]  ? __pfx___driver_attach+0x10/0x10
[ 9244.801704]  __driver_attach+0xcb/0x1e0
[ 9244.801706]  bus_for_each_dev+0x82/0xd0
[ 9244.801708]  bus_add_driver+0x12f/0x210
[ 9244.801710]  ? __pfx_xe_register_pci_driver+0x10/0x10 [xe]
[ 9244.801770]  ? __pfx_xe_init+0x10/0x10 [xe]
[ 9244.801828]  driver_register+0x75/0xe0
[ 9244.801829]  xe_init+0x4f/0x80 [xe]
[ 9244.801878]  do_one_initcall+0x58/0x300
[ 9244.801888]  do_init_module+0x84/0x280
[ 9244.801894]  init_module_from_file+0x8a/0xe0
[ 9244.801897]  idempotent_init_module+0x114/0x310
[ 9244.801899]  __x64_sys_finit_module+0x6d/0xd0
[ 9244.801900]  ? _raw_spin_unlock+0xe/0x30
[ 9244.801907]  do_syscall_64+0x7e/0x250
[ 9244.801913]  ? do_sys_openat2+0xa2/0xe0
[ 9244.801918]  ? __x64_sys_openat+0x61/0xa0
[ 9244.801919]  ? do_syscall_64+0xb6/0x250
[ 9244.801920]  ? irqentry_exit_to_user_mode+0x2c/0x1c0
[ 9244.801922]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[ 9244.801926] RIP: 0033:0x7f5ce550038d
[ 9244.801961] Code: ff c3 66 2e 0f 1f 84 00 00 00 00 00 90 f3 0f 1e fa 48 89 f8 48 89 f7 48 89 d6 48 89 ca 4d 89 c2 4d 89 c8 4c 8b 4c 24 08 0f 05 <48> 3d 01 f0 ff ff 73 01 c3 48 8b 0d 43 5a 0f 00 f7 d8 64 89 01 48
[ 9244.801962] RSP: 002b:00007ffe55bcf008 EFLAGS: 00000246 ORIG_RAX: 0000000000000139
[ 9244.801963] RAX: ffffffffffffffda RBX: 000055f04b63ac70 RCX: 00007f5ce550038d
[ 9244.801963] RDX: 0000000000000004 RSI: 000055f04b63e900 RDI: 0000000000000007
[ 9244.801964] RBP: 00007ffe55bcf0a0 R08: 0000000000000000 R09: 0000000000000000
[ 9244.801964] R10: 0000000000000000 R11: 0000000000000246 R12: 000055f04b63e900
[ 9244.801965] R13: 0000000000040000 R14: 000055f04b63ad70 R15: 0000000000000010
[ 9244.801966]  </TASK>
[ 9244.801966] ---[ end trace 0000000000000000 ]---
[ 9244.801968] xe 0000:00:07.0: probe with driver xe failed with error -110
```

更新内核之后，结果有其他的问题:

6.19.11-200.fc43.x86_64

也许需要重启一下机器才可以的吧
```txt
[    7.244752] xe 0000:00:06.0: [drm] Tile0: GT0: Engine reset: engine_class=rcs, logical_mask: 0x1, guc_id=3
[    7.244825] xe 0000:00:06.0: [drm] Tile0: GT0: Timedout job: seqno=4294967169, lrc_seqno=4294967169, guc_id=3, flags=0x0 in systemd-logind [1121]
[    7.244868] xe 0000:00:06.0: [drm] Xe device coredump has been created
[    7.244869] xe 0000:00:06.0: [drm] Check your /sys/class/drm/card0/device/devcoredump/data
```
