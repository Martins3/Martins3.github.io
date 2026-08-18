尝试将 nvidia 的显卡从

echo 0000:00:01.0 | sudo tee /sys/bus/pci/devices/0000:00:01.0/driver/unbind

```txt
Sep 09 01:57:57 nixos kernel: ------------[ cut here ]------------
Sep 09 01:57:57 nixos kernel: WARNING: CPU: 2 PID: 142967 at drivers/net/wireless/intel/iwlwifi/mvm/sta.c:2828 iwl_mvm_sta_rx_agg+0x86d/0x8d0 [iwlmvm]
Sep 09 01:57:57 nixos kernel: CPU: 2 PID: 142967 Comm: kworker/u64:2 Tainted: P        W  O       6.1.50 #1-NixOS
Sep 09 01:57:57 nixos kernel: Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
Sep 09 01:57:57 nixos kernel: Workqueue: phy0 ieee80211_iface_work [mac80211]
Sep 09 01:57:57 nixos kernel: RIP: 0010:iwl_mvm_sta_rx_agg+0x86d/0x8d0 [iwlmvm]
Sep 09 01:57:57 nixos kernel: Code: fd ff ff 0f 0b 41 b9 ea ff ff ff e9 17 fb ff ff 0f 0b 41 b9 ea ff ff ff e9 0a fb ff ff 8b 44 24 2c 89 44 24 34 e9 60 fc ff ff <0f> 0b e9 ba fe ff ff 48 8b 3f 48 c7 c6 a8 4d 52 c5 e8 c5 a9 e0 ff
Sep 09 01:57:57 nixos kernel: RSP: 0018:ffffc02d01eafbb8 EFLAGS: 00010286
Sep 09 01:57:57 nixos kernel: RAX: ffff9ff8e5340000 RBX: ffff9ff930e9e880 RCX: 0000000000000003
Sep 09 01:57:57 nixos kernel: RDX: 0000000000000000 RSI: 0000000000000012 RDI: ffffc02d01eafb68
Sep 09 01:57:57 nixos kernel: RBP: ffff9ff8448d2068 R08: ffffffffc551ee00 R09: 0000000000000000
Sep 09 01:57:57 nixos kernel: R10: ffff9ff830935010 R11: 0000000000000001 R12: 0000000000000012
Sep 09 01:57:57 nixos kernel: R13: ffff9ff930e80000 R14: 0000000000000003 R15: ffff9fff59f84a98
Sep 09 01:57:57 nixos kernel: FS:  0000000000000000(0000) GS:ffffa0073f080000(0000) knlGS:0000000000000000
Sep 09 01:57:57 nixos kernel: CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
Sep 09 01:57:57 nixos kernel: CR2: 00007fe739bb5004 CR3: 00000001d4410000 CR4: 0000000000752ee0
Sep 09 01:57:57 nixos kernel: PKRU: 55555554
Sep 09 01:57:57 nixos kernel: Call Trace:
Sep 09 01:57:57 nixos kernel:  <TASK>
Sep 09 01:57:57 nixos kernel:  ? __warn+0x7d/0xc0
Sep 09 01:57:57 nixos kernel:  ? iwl_mvm_sta_rx_agg+0x86d/0x8d0 [iwlmvm]
Sep 09 01:57:57 nixos kernel:  ? report_bug+0xe6/0x170
Sep 09 01:57:57 nixos kernel:  ? handle_bug+0x41/0x70
Sep 09 01:57:57 nixos kernel:  ? exc_invalid_op+0x13/0x60
Sep 09 01:57:57 nixos kernel:  ? asm_exc_invalid_op+0x16/0x20
Sep 09 01:57:57 nixos kernel:  ? iwl_mvm_sta_rx_agg+0x86d/0x8d0 [iwlmvm]
Sep 09 01:57:57 nixos kernel:  ? try_to_grab_pending+0xdf/0x170
Sep 09 01:57:57 nixos kernel:  iwl_mvm_mac_ampdu_action+0x1fe/0x400 [iwlmvm]
Sep 09 01:57:57 nixos kernel:  ? __kmem_cache_alloc_node+0x153/0x290
Sep 09 01:57:57 nixos kernel:  drv_ampdu_action+0x75/0x170 [mac80211]
Sep 09 01:57:57 nixos kernel:  ___ieee80211_start_rx_ba_session+0x1b2/0x830 [mac80211]
Sep 09 01:57:57 nixos kernel:  ? update_load_avg+0x7e/0x780
Sep 09 01:57:57 nixos kernel:  ieee80211_process_addba_request+0xb9/0x1a0 [mac80211]
Sep 09 01:57:57 nixos kernel:  ieee80211_iface_work+0x3e0/0x410 [mac80211]
Sep 09 01:57:57 nixos kernel:  process_one_work+0x1c4/0x380
Sep 09 01:57:57 nixos kernel:  worker_thread+0x4d/0x380
Sep 09 01:57:57 nixos kernel:  ? _raw_spin_lock_irqsave+0x23/0x50
Sep 09 01:57:57 nixos kernel:  ? rescuer_thread+0x3a0/0x3a0
Sep 09 01:57:57 nixos kernel:  kthread+0xe6/0x110
Sep 09 01:57:57 nixos kernel:  ? kthread_complete_and_exit+0x20/0x20
Sep 09 01:57:57 nixos kernel:  ret_from_fork+0x1f/0x30
Sep 09 01:57:57 nixos kernel:  </TASK>
Sep 09 01:57:57 nixos kernel: ---[ end trace 0000000000000000 ]---
Sep 09 04:01:16 nixos kernel: hrtimer: interrupt took 11911 ns
Sep 09 12:24:27 nixos kernel: NVRM: GPU at PCI:0000:01:00: GPU-9ba0f26a-a046-9909-7b2a-17a000456830
Sep 09 12:24:27 nixos kernel: NVRM: Xid (PCI:0000:01:00): 16, pid='<unknown>', name=<unknown>, Head 00000003 Count 0023070b
Sep 09 12:24:31 nixos kernel: NVRM: Xid (PCI:0000:01:00): 8, pid=4270, name=wezterm-gui, Channel 00000028
Sep 09 12:24:31 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=3911, name=X, Channel ID 00000018 intr 00008000
Sep 09 12:24:31 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=3911, name=X, Channel ID 00000018 intr 00008000
Sep 09 12:24:31 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=3911, name=X, Channel ID 00000018 intr 00008000
Sep 09 12:24:31 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=3911, name=X, Channel ID 00000018 intr 00008000
Sep 09 12:24:31 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=3911, name=X, Channel ID 00000018 intr 00008000
Sep 09 12:24:31 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=3911, name=X, Channel ID 00000018 intr 00008000
Sep 09 12:24:31 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=969, name=(udev-worker), Channel ID 00000001 intr 00008000
Sep 09 12:24:34 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=3911, name=X, Channel ID 0000001a intr 00008000
Sep 09 12:24:34 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=969, name=(udev-worker), Channel ID 00000001 intr 00008000
Sep 09 12:24:34 nixos kernel: snd_hda_intel 0000:01:00.1: spurious response 0x80000031:0x0, last cmd=0x570700
Sep 09 12:24:37 nixos kernel: nvidia-modeset: WARNING: GPU:0: Lost display notification (0:0x00000000); continuing.
Sep 09 12:24:37 nixos kernel: snd_hda_intel 0000:01:00.1: spurious response 0x80000051:0x0, last cmd=0x570700
Sep 09 12:24:39 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=969, name=(udev-worker), Channel ID 00000001 intr 00008000
Sep 09 12:24:39 nixos kernel: NVRM: Xid (PCI:0000:01:00): 32, pid=969, name=(udev-worker), Channel ID 00000001 intr 00008000
```
