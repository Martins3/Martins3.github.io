# 年轻人的第一台游戏本

## 一些错误的记录
```txt
[ 3799.664310] nvim[12643]: segfault at 5f0e0001 ip 000000005f0e0001 sp 00007fffaa2e7928 error 14 in c.so[7f408ac6b000+3000] likely on CPU 12 (core 6, socket 0)
[ 3799.664320] Code: Unable to access opcode bytes at 0x5f0dffd7.
```

## 这个应该真的是内核的 bug 了


在 Linux 项目中使用了一下 `EXPORT_SYMBOL`，然后系统 hang 住，屏幕还有，但是结果如下:

```txt
6月 04 15:40:28 nixos kernel: efi: Remove mem57: MMIO range=[0xff000000-0xffffffff] (16MB) from e820 map
...skipping...
6月 04 19:21:56 nixos kernel: RSP: 002b:00007f3cad1b80e0 EFLAGS: 00000246 ORIG_RAX: 00000000000000ca
6月 04 19:21:56 nixos kernel: RAX: fffffffffffffe00 RBX: 0000000000000000 RCX: 00007f3d4e04ca36
6月 04 19:21:56 nixos kernel: RDX: 0000000000000000 RSI: 0000000000000189 RDI: 00007f3cad1b8318
6月 04 19:21:56 nixos kernel: RBP: 0000000000000000 R08: 0000000000000000 R09: 00000000ffffffff
6月 04 19:21:56 nixos kernel: R10: 0000000000000000 R11: 0000000000000246 R12: 00007f3cad1b82c8
6月 04 19:21:56 nixos kernel: R13: 0000000000000000 R14: 0000000000000000 R15: 00007f3cad1b8318
6月 04 19:21:56 nixos kernel:  </TASK>
6月 04 19:21:56 nixos kernel: _swap_info_get: Bad swap file entry 4003ffffa095a63f
6月 04 19:21:56 nixos kernel: BUG: Bad page map in process ThreadPoolForeg  pte:800000bed4b380ba pmd:55ec77067
6月 04 19:21:56 nixos kernel: addr:000006dc01c46000 vm_flags:00100073 anon_vma:ffff9b938cae25b0 mapping:0000000000000000 index:6dc01c46
6月 04 19:21:56 nixos kernel: file:(null) fault:0x0 mmap:0x0 read_folio:0x0
6月 04 19:21:56 nixos kernel: CPU: 16 PID: 200876 Comm: ThreadPoolForeg Kdump: loaded Tainted: P    B      O       6.3.5 #1-NixOS
6月 04 19:21:56 nixos kernel: Hardware name: LENOVO 82WM/LNVNB161216, BIOS LPCN41WW 05/24/2023
6月 04 19:21:56 nixos kernel: Call Trace:
6月 04 19:21:56 nixos kernel:  <TASK>
6月 04 19:21:56 nixos kernel:  dump_stack_lvl+0x47/0x60
6月 04 19:21:56 nixos kernel:  print_bad_pte+0x1ad/0x270
6月 04 19:21:56 nixos kernel:  unmap_page_range+0xe7f/0x1190
6月 04 19:21:56 nixos kernel:  unmap_vmas+0xf8/0x190
6月 04 19:21:56 nixos kernel:  exit_mmap+0xdb/0x2f0
6月 04 19:21:56 nixos kernel:  __mmput+0x3e/0x130
6月 04 19:21:56 nixos kernel:  do_exit+0x2e7/0xac0
6月 04 19:21:56 nixos kernel:  ? futex_unqueue+0x3c/0x60
6月 04 19:21:56 nixos kernel:  do_group_exit+0x31/0x80
6月 04 19:21:56 nixos kernel:  get_signal+0x988/0x9c0
6月 04 19:21:56 nixos kernel:  arch_do_signal_or_restart+0x3e/0x270
6月 04 19:21:56 nixos kernel:  exit_to_user_mode_prepare+0x19f/0x200
6月 04 19:21:56 nixos kernel:  syscall_exit_to_user_mode+0x1b/0x40
6月 04 19:21:56 nixos kernel:  do_syscall_64+0x4a/0x90
6月 04 19:21:56 nixos kernel:  entry_SYSCALL_64_after_hwframe+0x72/0xdc
6月 04 19:21:56 nixos kernel: RIP: 0033:0x7f3d4e04ca36
6月 04 19:21:56 nixos kernel: Code: Unable to access opcode bytes at 0x7f3d4e04ca0c.
6月 04 19:21:56 nixos kernel: RSP: 002b:00007f3cad1b80e0 EFLAGS: 00000246 ORIG_RAX: 00000000000000ca
6月 04 19:21:56 nixos kernel: RAX: fffffffffffffe00 RBX: 0000000000000000 RCX: 00007f3d4e04ca36
6月 04 19:21:56 nixos kernel: RDX: 0000000000000000 RSI: 0000000000000189 RDI: 00007f3cad1b8318
6月 04 19:21:56 nixos kernel: RBP: 0000000000000000 R08: 0000000000000000 R09: 00000000ffffffff
6月 04 19:21:56 nixos kernel: R10: 0000000000000000 R11: 0000000000000246 R12: 00007f3cad1b82c8
6月 04 19:21:56 nixos kernel: R13: 0000000000000000 R14: 0000000000000000 R15: 00007f3cad1b8318
6月 04 19:21:56 nixos kernel:  </TASK>
6月 04 19:21:56 nixos kernel: _swap_info_get: Bad swap file entry 4003fffff0f3cb3f
6月 04 19:21:56 nixos kernel: BUG: Bad page map in process ThreadPoolForeg  pte:8000001e1869801a pmd:55ec77067
6月 04 19:21:56 nixos kernel: addr:000006dc01c47000 vm_flags:00100073 anon_vma:ffff9b938cae25b0 mapping:0000000000000000 index:6dc01c47
6月 04 19:21:56 nixos kernel: file:(null) fault:0x0 mmap:0x0 read_folio:0x0
6月 04 19:21:56 nixos kernel: CPU: 16 PID: 200876 Comm: ThreadPoolForeg Kdump: loaded Tainted: P    B      O       6.3.5 #1-NixOS
6月 04 19:21:56 nixos kernel: Hardware name: LENOVO 82WM/LNVNB161216, BIOS LPCN41WW 05/24/2023
6月 04 19:21:56 nixos kernel: Call Trace:
6月 04 19:21:56 nixos kernel:  <TASK>
6月 04 19:21:56 nixos kernel:  dump_stack_lvl+0x47/0x60
6月 04 19:21:56 nixos kernel:  print_bad_pte+0x1ad/0x270
6月 04 19:21:56 nixos kernel:  unmap_page_range+0xe7f/0x1190
6月 04 19:21:56 nixos kernel:  unmap_vmas+0xf8/0x190
6月 04 19:21:56 nixos kernel:  exit_mmap+0xdb/0x2f0
6月 04 19:21:56 nixos kernel:  __mmput+0x3e/0x130
6月 04 19:21:56 nixos kernel:  do_exit+0x2e7/0xac0
6月 04 19:21:56 nixos kernel:  ? futex_unqueue+0x3c/0x60
6月 04 19:21:56 nixos kernel:  do_group_exit+0x31/0x80
6月 04 19:21:56 nixos kernel:  get_signal+0x988/0x9c0
6月 04 19:21:56 nixos kernel:  arch_do_signal_or_restart+0x3e/0x270
6月 04 19:21:56 nixos kernel:  exit_to_user_mode_prepare+0x19f/0x200
6月 04 19:21:56 nixos kernel:  syscall_exit_to_user_mode+0x1b/0x40
6月 04 19:21:56 nixos kernel:  do_syscall_64+0x4a/0x90
6月 04 19:21:56 nixos kernel:  entry_SYSCALL_64_after_hwframe+0x72/0xdc
6月 04 19:21:56 nixos kernel: RIP: 0033:0x7f3d4e04ca36
6月 04 19:21:56 nixos kernel: Code: Unable to access opcode bytes at 0x7f3d4e04ca0c.
6月 04 19:21:56 nixos kernel: RSP: 002b:00007f3cad1b80e0 EFLAGS: 00000246 ORIG_RAX: 00000000000000ca
6月 04 19:21:56 nixos kernel: RAX: fffffffffffffe00 RBX: 0000000000000000 RCX: 00007f3d4e04ca36
6月 04 19:21:56 nixos kernel: RDX: 0000000000000000 RSI: 0000000000000189 RDI: 00007f3cad1b8318
6月 04 19:21:56 nixos kernel: RBP: 0000000000000000 R08: 0000000000000000 R09: 00000000ffffffff
6月 04 19:21:56 nixos kernel: R10: 0000000000000000 R11: 0000000000000246 R12: 00007f3cad1b82c8
6月 04 19:21:56 nixos kernel: R13: 0000000000000000 R14: 0000000000000000 R15: 00007f3cad1b8318
6月 04 19:21:56 nixos kernel:  </TASK>
6月 04 19:21:56 nixos systemd[1]: Started Journal Service.
6月 04 19:21:56 nixos systemd-journald[205396]: File /var/log/journal/c4d5dffd3fce45ff9046f3017148dd83/user-1000.journal corrupted or uncleanly shut down, renaming and replacing.
6月 04 19:21:56 nixos kernel: BUG: Bad rss-counter state mm:0000000042c1f37c type:MM_ANONPAGES val:8
6月 04 19:21:56 nixos kernel: BUG: Bad rss-counter state mm:0000000042c1f37c type:MM_SWAPENTS val:-8
6月 04 19:30:33 nixos kernel: kvm: SMP vm created on host with unstable TSC; guest TSC will not be reliable
```

## 好家伙，有一个 rcu stall 的 bug

当时的行为: 打开 bilibili ，发现长时间没有响应，然后
```txt

6月 04 20:23:15 nixos kernel: ------------[ cut here ]------------
6月 04 20:23:15 nixos kernel: list_del corruption. prev->next should be ffffba95d4ccfe20, but was ffffba9cd4ccfe08. (prev=ffffa09c42343808)
6月 04 20:23:15 nixos kernel: WARNING: CPU: 14 PID: 8619 at lib/list_debug.c:59 __list_del_entry_valid+0xb4/0xe0
6月 04 20:23:15 nixos kernel: Modules linked in: tcp_diag inet_diag xt_mark nft_chain_nat xt_MASQUERADE nf_conntrack_netlink xfrm_user xfrm_algo xt_addrtype overlay qrtr rfcomm snd_seq_dummy snd_hrtimer snd_seq snd_seq_device ccm cmac algif_hash algif_skcipher af_alg af_packet bnep mousedev joydev hid_multitouch snd_sof_amd_rembrandt snd_sof_amd_renoir mt7921e snd_sof_amd_acp snd_sof_pci mt7921_common snd_sof_xtensa_dsp mt76_connac_lib snd_sof snd_hda_codec_realtek mt76 snd_hda_codec_generic snd_sof_utils ledtrig_audio snd_hda_codec_hdmi snd_soc_core mac80211 snd_hda_intel snd_intel_dspcfg snd_intel_sdw_acpi uvcvideo snd_compress snd_hda_codec ac97_bus ip6_tables snd_pcm_dmaengine nls_iso8859_1 snd_pci_ps btusb videobuf2_vmalloc nls_cp437 uvc edac_mce_amd snd_rpl_pci_acp6x btrtl videobuf2_memops snd_acp_pci snd_hda_core btbcm edac_core vfat snd_pci_acp6x intel_rapl_msr btintel intel_rapl_common snd_hwdep snd_pci_acp5x videobuf2_v4l2 btmtk fat r8169 crc32_pclmul snd_pcm hid_generic polyval_clmulni polyval_generic cfg80211 xt_conntrack
6月 04 20:23:15 nixos kernel:  bluetooth gf128mul snd_rn_pci_acp3x videodev ghash_clmulni_intel i2c_hid_acpi snd_acp_config sha512_ssse3 wdat_wdt realtek snd_timer ip6t_rpfilter sp5100_tco sha512_generic snd_soc_acpi ucsi_acpi mdio_devres aesni_intel typec_ucsi ideapad_laptop videobuf2_common usbhid i2c_hid crypto_simd ecdh_generic sparse_keymap wmi_bmof cryptd ecc libaes hid mc ipt_rpfilter rapl typec watchdog libphy snd k10temp platform_profile i2c_piix4 snd_pci_acp3x libarc4 soundcore roles rfkill battery xt_pkttype tpm_crb xt_LOG tpm_tis nf_log_syslog tpm_tis_core xt_tcpudp tiny_power_button nft_compat acpi_cpufreq i2c_designware_platform evdev i2c_designware_core ac input_leds button led_class mac_hid serio_raw nf_tables sch_fq_codel nfnetlink nvidia_drm(PO) drm_kms_helper syscopyarea sysfillrect sysimgblt nvidia_modeset(PO) video wmi nvidia_uvm(PO) nvidia(PO) ctr loop xt_nat br_netfilter veth tap macvlan bridge stp llc openvswitch nsh nf_conncount nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 libcrc32c tun kvm_amd ccp kvm drm
6月 04 20:23:15 nixos kernel:  fuse deflate backlight efi_pstore i2c_core configfs zstd zram zsmalloc efivarfs tpm rng_core dmi_sysfs ip_tables x_tables autofs4 ext4 crc32c_generic crc16 mbcache jbd2 xhci_pci xhci_pci_renesas xhci_hcd nvme atkbd libps2 usbcore vivaldi_fmap nvme_core crc32c_intel t10_pi crc64_rocksoft crc64 crc_t10dif usb_common crct10dif_generic crct10dif_pclmul i8042 crct10dif_common rtc_cmos serio dm_mod dax vfio_pci vfio_pci_core irqbypass vfio_iommu_type1 vfio iommufd
6月 04 20:23:15 nixos kernel: CPU: 14 PID: 8619 Comm: Compositor Kdump: loaded Tainted: P           O       6.3.5 #1-NixOS
6月 04 20:23:15 nixos kernel: Hardware name: LENOVO 82WM/LNVNB161216, BIOS LPCN41WW 05/24/2023
6月 04 20:23:15 nixos kernel: RIP: 0010:__list_del_entry_valid+0xb4/0xe0
6月 04 20:23:15 nixos kernel: Code: eb d3 48 89 fe 48 89 ca 48 c7 c7 58 32 be b4 e8 22 0c b3 ff 0f 0b eb bd 48 89 fe 48 89 c2 48 c7 c7 90 32 be b4 e8 0c 0c b3 ff <0f> 0b eb a7 48 89 d1 48 c7 c7 d8 32 be b4 48 89 f2 48 89 c6 e8 f3
6月 04 20:23:15 nixos kernel: RSP: 0018:ffffba95d4ccfd50 EFLAGS: 00010286
6月 04 20:23:15 nixos kernel: RAX: 0000000000000000 RBX: ffffba95d4ccfe08 RCX: 0000000000000027
6月 04 20:23:15 nixos kernel: RDX: ffffa0ab2d9a14c8 RSI: 0000000000000001 RDI: ffffa0ab2d9a14c0
6月 04 20:23:15 nixos kernel: RBP: ffffba95d4ccfe20 R08: 0000000000000000 R09: ffffba95d4ccfbf8
6月 04 20:23:15 nixos kernel: R10: 0000000000000003 R11: ffffffffb5338868 R12: 0000000000000000
6月 04 20:23:15 nixos kernel: R13: 0000000000000000 R14: 00007f11875f9570 R15: 00000000ffffffff
6月 04 20:23:15 nixos kernel: FS:  00007f11875fa6c0(0000) GS:ffffa0ab2d980000(0000) knlGS:0000000000000000
6月 04 20:23:15 nixos kernel: CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
6月 04 20:23:15 nixos kernel: CR2: 000000c00083f000 CR3: 00000002899c4000 CR4: 0000000000750ee0
6月 04 20:23:15 nixos kernel: DR0: ffffffff8122eb60 DR1: ffffffff83558bb0 DR2: 0000000000000003
6月 04 20:23:15 nixos kernel: DR3: 00007ffce1f0c430 DR6: 00000000ffff0ff0 DR7: 0000000000000400
6月 04 20:23:15 nixos kernel: PKRU: 55555558
6月 04 20:23:15 nixos kernel: Call Trace:
6月 04 20:23:15 nixos kernel:  <TASK>
6月 04 20:23:15 nixos kernel:  ? __list_del_entry_valid+0xb4/0xe0
6月 04 20:23:15 nixos kernel:  ? __warn+0x81/0x130
6月 04 20:23:15 nixos kernel:  ? __list_del_entry_valid+0xb4/0xe0
6月 04 20:23:15 nixos kernel:  ? report_bug+0x171/0x1a0
6月 04 20:23:15 nixos kernel:  ? handle_bug+0x41/0x70
6月 04 20:23:15 nixos kernel:  ? exc_invalid_op+0x17/0x70
6月 04 20:23:15 nixos kernel:  ? asm_exc_invalid_op+0x1a/0x20
6月 04 20:23:15 nixos kernel:  ? __list_del_entry_valid+0xb4/0xe0
6月 04 20:23:15 nixos kernel:  plist_del+0x63/0xc0
6月 04 20:23:15 nixos kernel:  __futex_unqueue+0x29/0x40
6月 04 20:23:15 nixos kernel:  futex_unqueue+0x2d/0x60
6月 04 20:23:15 nixos kernel:  futex_wait+0x193/0x270
6月 04 20:23:15 nixos kernel:  ? __pfx_hrtimer_wakeup+0x10/0x10
6月 04 20:23:15 nixos kernel:  do_futex+0x10a/0x1b0
6月 04 20:23:15 nixos kernel:  __x64_sys_futex+0x92/0x1d0
6月 04 20:23:15 nixos kernel:  do_syscall_64+0x3b/0x90
6月 04 20:23:15 nixos kernel:  entry_SYSCALL_64_after_hwframe+0x72/0xdc
6月 04 20:23:15 nixos kernel: RIP: 0033:0x7f1199c75a36
6月 04 20:23:15 nixos kernel: Code: 74 24 08 e8 cc f8 ff ff 4c 8b 54 24 18 8b 74 24 08 89 ea 89 c3 48 8b 7c 24 10 41 b9 ff ff ff ff 45 31 c0 b8 ca 00 00 00 0f 05 <89> df 48 89 44 24 08 e8 1e f9 ff ff 48 8b 44 24 08 e9 6a ff ff ff
6月 04 20:23:15 nixos kernel: RSP: 002b:00007f11875f9300 EFLAGS: 00000246 ORIG_RAX: 00000000000000ca
6月 04 20:23:15 nixos kernel: RAX: ffffffffffffffda RBX: 0000000000000000 RCX: 00007f1199c75a36
6月 04 20:23:15 nixos kernel: RDX: 0000000000000000 RSI: 0000000000000089 RDI: 00007f11875f9570
6月 04 20:23:15 nixos kernel: RBP: 0000000000000000 R08: 0000000000000000 R09: 00000000ffffffff
6月 04 20:23:15 nixos kernel: R10: 00007f11875f9410 R11: 0000000000000246 R12: 0000000000000000
6月 04 20:23:15 nixos kernel: R13: 00007f11875f9520 R14: 00007f11875f9570 R15: 0000000000000000
6月 04 20:23:15 nixos kernel:  </TASK>
6月 04 20:23:15 nixos kernel: ---[ end trace 0000000000000000 ]---
6月 04 20:23:15 nixos kernel: BUG: unable to handle page fault for address: ffffba9cd4ccfdf0
6月 04 20:23:15 nixos kernel: #PF: supervisor read access in kernel mode
6月 04 20:23:15 nixos kernel: #PF: error_code(0x0000) - not-present page
6月 04 20:23:15 nixos kernel: PGD 100000067 P4D 100000067 PUD 0
6月 04 20:23:15 nixos kernel: Oops: 0000 [#1] PREEMPT SMP NOPTI
6月 04 20:23:15 nixos kernel: CPU: 14 PID: 8619 Comm: Compositor Kdump: loaded Tainted: P        W  O       6.3.5 #1-NixOS
6月 04 20:23:15 nixos kernel: Hardware name: LENOVO 82WM/LNVNB161216, BIOS LPCN41WW 05/24/2023
6月 04 20:23:15 nixos kernel: RIP: 0010:plist_add+0x66/0x100
6月 04 20:23:15 nixos kernel: Code: 06 48 39 c6 74 69 48 8b 1e 8b 4d 00 31 f6 48 83 eb 18 48 89 da eb 13 48 8b 42 08 48 89 d6 48 83 e8 08 48 39 d8 74 11 48 89 c2 <3b> 0a 7d e9 4c 8d 62 18 48 89 d3 48 89 f2 48 85 d2 74 04 3b 0a 74
6月 04 20:23:15 nixos kernel: RSP: 0018:ffffba95d4ccfd38 EFLAGS: 00010286
6月 04 20:23:15 nixos kernel: RAX: ffffba9cd4ccfe08 RBX: ffffba9cd4ccfdf0 RCX: 0000000000000064
6月 04 20:23:15 nixos kernel: RDX: ffffba9cd4ccfdf0 RSI: 0000000000000000 RDI: ffffba95d4ccfe08
6月 04 20:23:15 nixos kernel: RBP: ffffba95d4ccfe08 R08: ffffba95d4ccfdb8 R09: 00000000875f9410
6月 04 20:23:15 nixos kernel: R10: 0000000000000000 R11: 0000000000000000 R12: ffffa09c42343808
6月 04 20:23:15 nixos kernel: R13: ffffba95d4ccfe20 R14: ffffba95d4ccfe10 R15: 00000000ffffffff
6月 04 20:23:15 nixos kernel: FS:  00007f11875fa6c0(0000) GS:ffffa0ab2d980000(0000) knlGS:0000000000000000
6月 04 20:23:15 nixos kernel: CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
6月 04 20:23:15 nixos kernel: CR2: ffffba9cd4ccfdf0 CR3: 00000002899c4000 CR4: 0000000000750ee0
6月 04 20:23:15 nixos kernel: DR0: ffffffff8122eb60 DR1: ffffffff83558bb0 DR2: 0000000000000003
6月 04 20:23:15 nixos kernel: DR3: 00007ffce1f0c430 DR6: 00000000ffff0ff0 DR7: 0000000000000400
6月 04 20:23:15 nixos kernel: PKRU: 55555558
6月 04 20:23:15 nixos kernel: Call Trace:
6月 04 20:23:15 nixos kernel:  <TASK>
6月 04 20:23:15 nixos kernel:  ? __die+0x23/0x70
6月 04 20:23:15 nixos kernel:  ? page_fault_oops+0x17d/0x4b0
6月 04 20:23:15 nixos kernel:  ? exc_page_fault+0xb2/0x150
6月 04 20:23:15 nixos kernel:  ? asm_exc_page_fault+0x26/0x30
6月 04 20:23:15 nixos kernel:  ? plist_add+0x66/0x100
6月 04 20:23:15 nixos kernel:  __futex_queue+0x47/0x60
6月 04 20:23:15 nixos kernel:  futex_wait_queue+0x32/0x90
6月 04 20:23:15 nixos kernel:  futex_wait+0x189/0x270
6月 04 20:23:15 nixos kernel:  ? __pfx_hrtimer_wakeup+0x10/0x10
6月 04 20:23:15 nixos kernel:  do_futex+0x10a/0x1b0
6月 04 20:23:15 nixos kernel:  __x64_sys_futex+0x92/0x1d0
6月 04 20:23:15 nixos kernel:  do_syscall_64+0x3b/0x90
6月 04 20:23:15 nixos kernel:  entry_SYSCALL_64_after_hwframe+0x72/0xdc
6月 04 20:23:15 nixos kernel: RIP: 0033:0x7f1199c75a36
6月 04 20:23:15 nixos kernel: Code: 74 24 08 e8 cc f8 ff ff 4c 8b 54 24 18 8b 74 24 08 89 ea 89 c3 48 8b 7c 24 10 41 b9 ff ff ff ff 45 31 c0 b8 ca 00 00 00 0f 05 <89> df 48 89 44 24 08 e8 1e f9 ff ff 48 8b 44 24 08 e9 6a ff ff ff
6月 04 20:23:15 nixos kernel: RSP: 002b:00007f11875f9300 EFLAGS: 00000246 ORIG_RAX: 00000000000000ca
6月 04 20:23:15 nixos kernel: RAX: ffffffffffffffda RBX: 0000000000000000 RCX: 00007f1199c75a36
6月 04 20:23:15 nixos kernel: RDX: 0000000000000000 RSI: 0000000000000089 RDI: 00007f11875f9570
6月 04 20:23:15 nixos kernel: RBP: 0000000000000000 R08: 0000000000000000 R09: 00000000ffffffff
6月 04 20:23:15 nixos kernel: R10: 00007f11875f9410 R11: 0000000000000246 R12: 0000000000000000
6月 04 20:23:15 nixos kernel: R13: 00007f11875f9520 R14: 00007f11875f9570 R15: 0000000000000000
6月 04 20:23:15 nixos kernel:  </TASK>
6月 04 20:23:15 nixos kernel: Modules linked in: tcp_diag inet_diag xt_mark nft_chain_nat xt_MASQUERADE nf_conntrack_netlink xfrm_user xfrm_algo xt_addrtype overlay qrtr rfcomm snd_seq_dummy snd_hrtimer snd_seq snd_seq_device ccm cmac algif_hash algif_skcipher af_alg af_packet bnep mousedev joydev hid_multitouch snd_sof_amd_rembrandt snd_sof_amd_renoir mt7921e snd_sof_amd_acp snd_sof_pci mt7921_common snd_sof_xtensa_dsp mt76_connac_lib snd_sof snd_hda_codec_realtek mt76 snd_hda_codec_generic snd_sof_utils ledtrig_audio snd_hda_codec_hdmi snd_soc_core mac80211 snd_hda_intel snd_intel_dspcfg snd_intel_sdw_acpi uvcvideo snd_compress snd_hda_codec ac97_bus ip6_tables snd_pcm_dmaengine nls_iso8859_1 snd_pci_ps btusb videobuf2_vmalloc nls_cp437 uvc edac_mce_amd snd_rpl_pci_acp6x btrtl videobuf2_memops snd_acp_pci snd_hda_core btbcm edac_core vfat snd_pci_acp6x intel_rapl_msr btintel intel_rapl_common snd_hwdep snd_pci_acp5x videobuf2_v4l2 btmtk fat r8169 crc32_pclmul snd_pcm hid_generic polyval_clmulni polyval_generic cfg80211 xt_conntrack
6月 04 20:23:15 nixos kernel:  bluetooth gf128mul snd_rn_pci_acp3x videodev ghash_clmulni_intel i2c_hid_acpi snd_acp_config sha512_ssse3 wdat_wdt realtek snd_timer ip6t_rpfilter sp5100_tco sha512_generic snd_soc_acpi ucsi_acpi mdio_devres aesni_intel typec_ucsi ideapad_laptop videobuf2_common usbhid i2c_hid crypto_simd ecdh_generic sparse_keymap wmi_bmof cryptd ecc libaes hid mc ipt_rpfilter rapl typec watchdog libphy snd k10temp platform_profile i2c_piix4 snd_pci_acp3x libarc4 soundcore roles rfkill battery xt_pkttype tpm_crb xt_LOG tpm_tis nf_log_syslog tpm_tis_core xt_tcpudp tiny_power_button nft_compat acpi_cpufreq i2c_designware_platform evdev i2c_designware_core ac input_leds button led_class mac_hid serio_raw nf_tables sch_fq_codel nfnetlink nvidia_drm(PO) drm_kms_helper syscopyarea sysfillrect sysimgblt nvidia_modeset(PO) video wmi nvidia_uvm(PO) nvidia(PO) ctr loop xt_nat br_netfilter veth tap macvlan bridge stp llc openvswitch nsh nf_conncount nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 libcrc32c tun kvm_amd ccp kvm drm
6月 04 20:23:15 nixos kernel:  fuse deflate backlight efi_pstore i2c_core configfs zstd zram zsmalloc efivarfs tpm rng_core dmi_sysfs ip_tables x_tables autofs4 ext4 crc32c_generic crc16 mbcache jbd2 xhci_pci xhci_pci_renesas xhci_hcd nvme atkbd libps2 usbcore vivaldi_fmap nvme_core crc32c_intel t10_pi crc64_rocksoft crc64 crc_t10dif usb_common crct10dif_generic crct10dif_pclmul i8042 crct10dif_common rtc_cmos serio dm_mod dax vfio_pci vfio_pci_core irqbypass vfio_iommu_type1 vfio iommufd
6月 04 20:23:15 nixos kernel: CR2: ffffba9cd4ccfdf0
6月 04 20:23:15 nixos kernel: ---[ end trace 0000000000000000 ]---
6月 04 20:23:15 nixos kernel: pstore: backend (efi_pstore) writing error (-5)
6月 04 20:23:15 nixos kernel: RIP: 0010:plist_add+0x66/0x100
6月 04 20:23:15 nixos kernel: Code: 06 48 39 c6 74 69 48 8b 1e 8b 4d 00 31 f6 48 83 eb 18 48 89 da eb 13 48 8b 42 08 48 89 d6 48 83 e8 08 48 39 d8 74 11 48 89 c2 <3b> 0a 7d e9 4c 8d 62 18 48 89 d3 48 89 f2 48 85 d2 74 04 3b 0a 74
6月 04 20:23:15 nixos kernel: RSP: 0018:ffffba95d4ccfd38 EFLAGS: 00010286
6月 04 20:23:15 nixos kernel: RAX: ffffba9cd4ccfe08 RBX: ffffba9cd4ccfdf0 RCX: 0000000000000064
6月 04 20:23:15 nixos kernel: RDX: ffffba9cd4ccfdf0 RSI: 0000000000000000 RDI: ffffba95d4ccfe08
6月 04 20:23:15 nixos kernel: RBP: ffffba95d4ccfe08 R08: ffffba95d4ccfdb8 R09: 00000000875f9410
6月 04 20:23:15 nixos kernel: R10: 0000000000000000 R11: 0000000000000000 R12: ffffa09c42343808
6月 04 20:23:15 nixos kernel: R13: ffffba95d4ccfe20 R14: ffffba95d4ccfe10 R15: 00000000ffffffff
6月 04 20:23:15 nixos kernel: FS:  00007f11875fa6c0(0000) GS:ffffa0ab2d980000(0000) knlGS:0000000000000000
6月 04 20:23:15 nixos kernel: CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
6月 04 20:23:15 nixos kernel: CR2: ffffba9cd4ccfdf0 CR3: 00000002899c4000 CR4: 0000000000750ee0
6月 04 20:23:15 nixos kernel: DR0: ffffffff8122eb60 DR1: ffffffff83558bb0 DR2: 0000000000000003
6月 04 20:23:15 nixos kernel: DR3: 00007ffce1f0c430 DR6: 00000000ffff0ff0 DR7: 0000000000000400
6月 04 20:23:15 nixos kernel: PKRU: 55555558
6月 04 20:23:15 nixos kernel: note: Compositor[8619] exited with irqs disabled
6月 04 20:23:15 nixos kernel: note: Compositor[8619] exited with preempt_count 1
6月 04 20:24:03 nixos kernel: rcu: INFO: rcu_preempt self-detected stall on CPU
6月 04 20:24:03 nixos kernel: rcu:         0-....: (21000 ticks this GP) idle=330c/1/0x4000000000000000 softirq=36945/36947 fqs=5219
6月 04 20:24:03 nixos kernel: rcu:         (t=21001 jiffies g=172469 q=100582 ncpus=32)
6月 04 20:24:03 nixos kernel: CPU: 0 PID: 19841 Comm: msedge Kdump: loaded Tainted: P      D W  O       6.3.5 #1-NixOS
6月 04 20:24:03 nixos kernel: Hardware name: LENOVO 82WM/LNVNB161216, BIOS LPCN41WW 05/24/2023
6月 04 20:24:03 nixos kernel: RIP: 0010:native_queued_spin_lock_slowpath+0x6e/0x2a0
6月 04 20:24:03 nixos kernel: Code: 77 77 f0 0f ba 2b 08 0f 92 c2 8b 03 0f b6 d2 c1 e2 08 30 e4 09 d0 3d ff 00 00 00 77 53 85 c0 74 10 0f b6 03 84 c0 74 09 f3 90 <0f> b6 03 84 c0 75 f7 b8 01 00 00 00 66 89 03 5b 5d 41 5c 41 5d c3
6月 04 20:24:03 nixos kernel: RSP: 0018:ffffba95d5107d20 EFLAGS: 00000202
6月 04 20:24:03 nixos kernel: RAX: 0000000000000001 RBX: ffffa09c42343804 RCX: 0000000066b9ea0e
6月 04 20:24:03 nixos kernel: RDX: 0000000000000000 RSI: 0000000000000001 RDI: ffffa09c42343804
6月 04 20:24:03 nixos kernel: RBP: ffffba95d5107e08 R08: ffffba95d5107db8 R09: 0000000000000000
6月 04 20:24:03 nixos kernel: R10: 0000000000000002 R11: 0000000000000000 R12: ffffba95d5107e08
6月 04 20:24:03 nixos kernel: R13: 0000000000000000 R14: 0000000000000000 R15: ffffba95d5107e40
6月 04 20:24:03 nixos kernel: FS:  00007f8e5b0f1040(0000) GS:ffffa0ab2d600000(0000) knlGS:0000000000000000
6月 04 20:24:03 nixos kernel: CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
6月 04 20:24:03 nixos kernel: CR2: 00000c9005b0f080 CR3: 0000000312b74000 CR4: 0000000000750ef0
6月 04 20:24:03 nixos kernel: DR0: ffffffff8122eb60 DR1: ffffffff83558bb0 DR2: 0000000000000003
6月 04 20:24:03 nixos kernel: DR3: 00007ffce1f0c430 DR6: 00000000ffff0ff0 DR7: 0000000000000400
6月 04 20:24:03 nixos kernel: PKRU: 55555558
6月 04 20:24:03 nixos kernel: Call Trace:
6月 04 20:24:03 nixos kernel:  <IRQ>
6月 04 20:24:03 nixos kernel:  ? rcu_dump_cpu_stacks+0xc4/0x100
6月 04 20:24:03 nixos kernel:  ? rcu_sched_clock_irq+0x4f2/0x1150
6月 04 20:24:03 nixos kernel:  ? sched_slice+0x87/0x140
6月 04 20:24:03 nixos kernel:  ? trigger_load_balance+0x72/0x350
6月 04 20:24:03 nixos kernel:  ? update_process_times+0x74/0xb0
6月 04 20:24:03 nixos kernel:  ? tick_sched_handle+0x22/0x60
6月 04 20:24:03 nixos kernel:  ? tick_sched_timer+0x67/0x80
6月 04 20:24:03 nixos kernel:  ? __pfx_tick_sched_timer+0x10/0x10
6月 04 20:24:03 nixos kernel:  ? __hrtimer_run_queues+0x10f/0x2b0
6月 04 20:24:03 nixos kernel:  ? hrtimer_interrupt+0xf8/0x230
6月 04 20:24:03 nixos kernel:  ? __sysvec_apic_timer_interrupt+0x5e/0x130
6月 04 20:24:03 nixos kernel:  ? sysvec_apic_timer_interrupt+0x6d/0x90
6月 04 20:24:03 nixos kernel:  </IRQ>
6月 04 20:24:03 nixos kernel:  <TASK>
6月 04 20:24:03 nixos kernel:  ? asm_sysvec_apic_timer_interrupt+0x1a/0x20
6月 04 20:24:03 nixos kernel:  ? native_queued_spin_lock_slowpath+0x6e/0x2a0
6月 04 20:24:03 nixos kernel:  _raw_spin_lock+0x29/0x30
6月 04 20:24:03 nixos kernel:  futex_q_lock+0x2a/0x40
6月 04 20:24:03 nixos kernel:  futex_wait_setup+0x68/0xe0
6月 04 20:24:03 nixos kernel:  futex_wait+0x16d/0x270
6月 04 20:24:03 nixos kernel:  do_futex+0x10a/0x1b0
6月 04 20:24:03 nixos kernel:  __x64_sys_futex+0x92/0x1d0
6月 04 20:24:03 nixos kernel:  do_syscall_64+0x3b/0x90
6月 04 20:24:03 nixos kernel:  entry_SYSCALL_64_after_hwframe+0x72/0xdc
6月 04 20:24:03 nixos kernel: RIP: 0033:0x7f8e5c13ba36
6月 04 20:24:03 nixos kernel: Code: 74 24 08 e8 cc f8 ff ff 4c 8b 54 24 18 8b 74 24 08 89 ea 89 c3 48 8b 7c 24 10 41 b9 ff ff ff ff 45 31 c0 b8 ca 00 00 00 0f 05 <89> df 48 89 44 24 08 e8 1e f9 ff ff 48 8b 44 24 08 e9 6a ff ff ff
6月 04 20:24:03 nixos kernel: RSP: 002b:00007ffff441a500 EFLAGS: 00000246 ORIG_RAX: 00000000000000ca
6月 04 20:24:03 nixos kernel: RAX: ffffffffffffffda RBX: 0000000000000000 RCX: 00007f8e5c13ba36
6月 04 20:24:03 nixos kernel: RDX: 0000000000000000 RSI: 0000000000000189 RDI: 00007ffff441a738
6月 04 20:24:03 nixos kernel: RBP: 0000000000000000 R08: 0000000000000000 R09: 00000000ffffffff
6月 04 20:24:03 nixos kernel: R10: 0000000000000000 R11: 0000000000000246 R12: 00007ffff441a6e8
6月 04 20:24:03 nixos kernel: R13: 0000000000000000 R14: 0000000000000000 R15: 00007ffff441a738
6月 04 20:24:03 nixos kernel:  </TASK>
```
