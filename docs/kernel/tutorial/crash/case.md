# 实战材料
记录各种不了了之的问题，有兴趣可以调试下。

## irqblance 在 kunpeng 上会触发 bug

-  在 6.16.3 到 16.4 之间的修复了一个有趣的 bug :
```txt
.rw-r--r--@  34M root 27 Aug 10:51   vmlinuz-6.16.3
.rw-r--r--@  34M root  8 Sep 16:59   vmlinuz-6.16.4
```

周末的 bonus 吧

```txt
[46472.051367][ T6263] CPU: 13 UID: 0 PID: 6263 Comm: irqbalance Kdump: loaded Tainted: G            E       6.16.0 #7 NONE
[46472.062606][ T6263] Tainted: [E]=UNSIGNED_MODULE
[46472.067368][ T6263] Hardware name: Yunke China KunTai R722/BC82AMDDRA, BIOS 1.89 05/20/2022
[46472.076017][ T6263] pstate: 804000c9 (Nzcv daIF +PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[46472.083978][ T6263] pc : string+0x54/0x140
[46472.088233][ T6263] lr : vsnprintf+0x1ec/0x5f0
[46472.092828][ T6263] sp : ffff8000b084bae0
[46472.096985][ T6263] x29: ffff8000b084bae0 x28: ffff00208919c44e x27: ffffa276655198f4
[46472.105105][ T6263] x26: 0000000000000005 x25: 0000000000000bb4 x24: 00000000ffffffd8
[46472.113223][ T6263] x23: ffff8000b084bc40 x22: 0000000000000004 x21: ffff00208919d000
[46472.121327][ T6263] x20: ffffa276655198f4 x19: 0000000000000405 x18: 00000000ffffffff
[46472.129438][ T6263] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000004
[46472.137548][ T6263] x14: 0000000000000001 x13: ffff00208919d000 x12: 2032303a32353130
[46472.145659][ T6263] x11: 495349482d32762d x10: 0000000000000020 x9 : ffff8000b084bc40
[46472.153767][ T6263] x8 : 0000000000000020 x7 : 00000000ffffffff x6 : ffff00208919d000
[46472.161875][ T6263] x5 : 0000000000000000 x4 : 00000000ffffffff x3 : ffffffffffff0a00
[46472.169983][ T6263] x2 : ffff80008048bc98 x1 : 0000000000000000 x0 : ffff00208919c44e
[46472.178090][ T6263] Call trace:
[46472.181371][ T6263]  string+0x54/0x140 (P)
[46472.185604][ T6263]  vsnprintf+0x1ec/0x5f0
[46472.189834][ T6263]  seq_printf+0xc4/0xe8
[46472.193975][ T6263]  show_interrupts+0x2a0/0x438
[46472.198719][ T6263]  seq_read_iter+0x120/0x478
[46472.203284][ T6263]  proc_reg_read_iter+0x8c/0xe8
[46472.208111][ T6263]  vfs_read+0x270/0x320
[46472.212245][ T6263]  ksys_read+0x74/0x118
[46472.216378][ T6263]  __arm64_sys_read+0x24/0x40
[46472.221029][ T6263]  invoke_syscall+0x50/0x120
[46472.225591][ T6263]  el0_svc_common.constprop.0+0x48/0xf0
[46472.231099][ T6263]  do_el0_svc+0x24/0x38
[46472.235222][ T6263]  el0_svc+0x34/0x120
[46472.239167][ T6263]  el0t_64_sync_handler+0x10c/0x138
[46472.244320][ T6263]  el0t_64_sync+0x190/0x198
[46472.248780][ T6263] Code: 39000001 91000400 eb0700bf 54000560 (38656841)
[46472.255658][ T6263] SMP: stopping secondary CPUs
```

看上去还没有修复:
```txt
[35093.095082][ T6308] Unable to handle kernel paging request at virtual address ffff800080433c98
[35093.104109][ T6308] Mem abort info:
[35093.107761][ T6308]   ESR = 0x0000000096000007
[35093.112356][ T6308]   EC = 0x25: DABT (current EL), IL = 32 bits
[35093.118488][ T6308]   SET = 0, FnV = 0
[35093.122379][ T6308]   EA = 0, S1PTW = 0
[35093.126342][ T6308]   FSC = 0x07: level 3 translation fault
[35093.132044][ T6308] Data abort info:
[35093.135761][ T6308]   ISV = 0, ISS = 0x00000007, ISS2 = 0x00000000
[35093.142066][ T6308]   CM = 0, WnR = 0, TnD = 0, TagAccess = 0
[35093.147948][ T6308]   GCS = 0, Overlay = 0, DirtyBit = 0, Xs = 0
[35093.154115][ T6308] swapper pgtable: 4k pages, 48-bit VAs, pgdp=00002027e39ec000
[35093.161781][ T6308] [ffff800080433c98] pgd=10000020802d8403, p4d=10000020802d8403, pud=10000020802d9403, pmd=1000002080339403, pte=0000000000000000
[35093.175248][ T6308] Internal error: Oops: 0000000096000007 [#1]  SMP
[35093.181759][ T6308] Modules linked in: martins3(OE) vhost_net(E) vhost(E) vhost_iotlb(E) tap(E) tun(E) veth(E) xt_conntrack(E) xt_MASQUERADE(E) nf_conntrack_netlink(E) nfnetlink(E) iptable_nat(E) nf_nat(E) nf_conntrack(E) nf_defrag_ipv6(E) nf_defrag_ipv4(E) xt_addrtype(E) iptable_filter(E) ip_tables(E) br_netfilter(E) overlay(E) rpcrdma(E) rdma_cm(E) iw_cm(E) ib_cm(E) bridge(E) stp(E) llc(E) rfkill(E) vfat(E) fat(E) ipmi_ssif(E) hibmc_drm(E) drm_client_lib(E) drm_vram_helper(E) drm_ttm_helper(E) arm_smmuv3_pmu(E) ttm(E) acpi_ipmi(E) drm_display_helper(E) mlx5_ib(E) ipmi_si(E) drm_kms_helper(E) ipmi_devintf(E) ipmi_msghandler(E) hisi_uncore_hha_pmu(E) hisi_uncore_l3c_pmu(E) hisi_uncore_ddrc_pmu(E) hisi_uncore_pmu(E) hns_roce_hw_v2(E) cppc_cpufreq(E) ib_uverbs(E) ib_core(E) fuse(E) drm(E) nfsd(E) auth_rpcgss(E) nfs_acl(E) lockd(E) grace(E) sunrpc(E) ext4(E) crc16(E) mbcache(E) jbd2(E) hisi_sas_v3_hw(E) hisi_sas_main(E) realtek(E) hclge(E) ghash_ce(E) hclge_common(E) sha1_ce(E) sbsa_gwdt(E) nvme(E) libsas(E) ahci(E) mlx5_core(E)
[35093.181858][ T6308]  megaraid_sas(E) scsi_transport_sas(E) libahci(E) hns3(E) nvme_core(E) mlxfw(E) hnae3(E) nfit(E) libata(E) i2c_designware_platform(E) libnvdimm(E) i2c_designware_core(E) dm_mirror(E) dm_region_hash(E) dm_log(E) dm_multipath(E) dm_mod(E) aes_neon_bs(E) aes_neon_blk(E) aes_ce_blk(E) aes_ce_cipher(E) [last unloaded: martins3(OE)]
[35093.306394][ T6308] CPU: 50 UID: 0 PID: 6308 Comm: irqbalance Kdump: loaded Tainted: G           OE       6.16.0 #6 NONE
[35093.317974][ T6308] Tainted: [O]=OOT_MODULE, [E]=UNSIGNED_MODULE
[35093.324248][ T6308] Hardware name: Yunke China KunTai R722/BC82AMDDRA, BIOS 1.89 05/20/2022
[35093.333054][ T6308] pstate: 804000c9 (Nzcv daIF +PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[35093.341196][ T6308] pc : string+0x54/0x140
[35093.345576][ T6308] lr : vsnprintf+0x1ec/0x5f0
[35093.350270][ T6308] sp : ffff80009963bae0
[35093.354496][ T6308] x29: ffff80009963bae0 x28: ffff20202e9cd44e x27: ffffc122aa9398f4
[35093.362729][ T6308] x26: 0000000000000005 x25: 0000000000000bb4 x24: 00000000ffffffd8
[35093.370976][ T6308] x23: ffff80009963bc40 x22: 0000000000000004 x21: ffff20202e9ce000
[35093.379220][ T6308] x20: ffffc122aa9398f4 x19: 0000000000000405 x18: 00000000ffffffff
[35093.387461][ T6308] x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000004
[35093.395701][ T6308] x14: 0000000000000001 x13: ffff20202e9ce000 x12: 2020202020203020
[35093.403927][ T6308] x11: 2020202020202020 x10: 0000000000000020 x9 : ffff80009963bc40
[35093.412144][ T6308] x8 : 0000000000000020 x7 : 00000000ffffffff x6 : ffff20202e9ce000
[35093.420360][ T6308] x5 : 0000000000000000 x4 : 00000000ffffffff x3 : ffffffffffff0a00
[35093.428579][ T6308] x2 : ffff800080433c98 x1 : 0000000000000000 x0 : ffff20202e9cd44e
[35093.436814][ T6308] Call trace:
[35093.440191][ T6308]  string+0x54/0x140 (P)
[35093.444472][ T6308]  vsnprintf+0x1ec/0x5f0
[35093.448759][ T6308]  seq_printf+0xc4/0xe8
[35093.452966][ T6308]  show_interrupts+0x2a0/0x438
[35093.457782][ T6308]  seq_read_iter+0x120/0x478
[35093.462412][ T6308]  proc_reg_read_iter+0x8c/0xe8
[35093.467299][ T6308]  vfs_read+0x270/0x320
[35093.471495][ T6308]  ksys_read+0x74/0x118
[35093.475676][ T6308]  __arm64_sys_read+0x24/0x40
[35093.480402][ T6308]  invoke_syscall+0x50/0x120
[35093.485037][ T6308]  el0_svc_common.constprop.0+0x48/0xf0
[35093.490618][ T6308]  do_el0_svc+0x24/0x38
[35093.494806][ T6308]  el0_svc+0x34/0x120
[35093.498808][ T6308]  el0t_64_sync_handler+0x10c/0x138
[35093.504030][ T6308]  el0t_64_sync+0x190/0x198
[35093.508559][ T6308] Code: 39000001 91000400 eb0700bf 54000560 (38656841)
[35093.515511][ T6308] SM
```

从 irqbalance 的角度来思考一下，先 disable 掉，看看还有多久才可以触发?
如果不会复现，那么 irqbalance 。

感觉像是什么 rcu 之类的，最后遍历到了一个空指针上去了。

结合 yusur 网卡的问题，这个问题可能是具体的驱动问题了，但是我已经没有精力调试这个问题了。

## 终于我的 ASUS 主板上 nvme 故障了
```txt
[3061045.152954] nvme nvme0: I/O tag 489 (d1e9) opcode 0x1 (I/O Cmd) QID 3 timeout, aborting req_op:WRITE(1) size:524288
[3061045.282440] nvme nvme0: I/O tag 490 (01ea) opcode 0x1 (I/O Cmd) QID 3 timeout, aborting req_op:WRITE(1) size:524288
[3061045.410209] nvme nvme0: I/O tag 491 (f1eb) opcode 0x1 (I/O Cmd) QID 3 timeout, aborting req_op:WRITE(1) size:524288
[3061046.070871] nvme nvme0: Abort status: 0x0
[3061046.928938] nvme nvme0: I/O tag 129 (a081) opcode 0x1 (I/O Cmd) QID 5 timeout, aborting req_op:WRITE(1) size:32768
[3061048.832576] nvme nvme0: Abort status: 0x0
[3061049.841408] nvme nvme0: Abort status: 0x0
[3061055.910139] nvme nvme0: Abort status: 0x0
```

## 我的虚拟机开机就卡死了

```txt
[    3.006773][  T605] EXT4-fs (vda2): mounted filesystem with ordered data mode. Opts: (null)
[    3.339646][  T400] systemd-journald[400]: Received SIGTERM from PID 1 (systemd).
[    3.371295][    T1] printk: systemd: 18 output lines suppressed due to ratelimiting
[    3.430757][    T1] Kernel panic - not syncing: Attempted to kill init! exitcode=0x00007f00
[    3.436022][    T1] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[    3.438967][    T1] Call Trace:
[    3.439942][    T1]  dump_stack+0x57/0x6e
[    3.440923][    T1]  panic+0x10e/0x2f6
[    3.441745][    T1]  do_exit.cold+0x14/0xaf
[    3.442580][    T1]  do_group_exit+0x33/0xa0
[    3.443376][    T1]  __x64_sys_exit_group+0x14/0x20
[    3.444161][    T1]  do_syscall_64+0x40/0x80
[    3.444817][    T1]  entry_SYSCALL_64_after_hwframe+0x67/0xcc
[    3.445572][    T1] RIP: 0033:0x7f16cffb0e76
[    3.446123][    T1] Code: 83 c8 ff c3 89 fa 41 b8 e7 00 00 00 be 3c 00 00 00 eb 10 90 89 d7 89 f0 0f 05 48 3d 00 f0 ff ff 77 22 f4 89 d7 44 89 c0 0f 05 <48> 3d 00 f0 ff ff 76 e2 f7 d8 89 05 7a e2 00 00 eb d8 0f 1f 84 00
[    3.447981][    T1] RSP: 002b:00007ffd5d632908 EFLAGS: 00000202 ORIG_RAX: 00000000000000e7
[    3.448676][    T1] RAX: ffffffffffffffda RBX: 00007f16cffb92d8 RCX: 00007f16cffb0e76
[    3.449295][    T1] RDX: 000000000000007f RSI: 000000000000003c RDI: 000000000000007f
[    3.449849][    T1] RBP: 00007ffd5d633980 R08: 00000000000000e7 R09: 0000000000000000
[    3.450375][    T1] R10: ffffffffffffffff R11: 0000000000000202 R12: 0000000000000026
[    3.450851][    T1] R13: 00007f16cefe3ba0 R14: 0000000000000000 R15: 00007f16cf29faf0
[    3.451593][    T1] Kernel Offset: 0x29c00000 from 0xffffffff81000000 (relocation range: 0xffffffff80000000-0xffffffffbfffffff)
[    3.452233][    T1] ---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x00007f00 ]---
```

## 这种 Warning 需要在乎吗?

```txt
[ 7382.261370] sfc 0000:38:00.2 (unnamed net_device) (uninitialized): Solarflare NIC detected: device 1924:1b03 subsys 1924:8028
[ 7382.261374] sfc 0000:38:00.2: enabling device (0000 -> 0002)
[ 7382.265470] sfc 0000:38:00.2 (unnamed net_device) (uninitialized): WARNING: Insufficient MSI-X vectors available (8 < 32).
[ 7382.277992] sfc 0000:38:00.2 (unnamed net_device) (uninitialized): WARNING: Performance may be reduced.
[ 7382.289384] sfc 0000:38:00.2 (unnamed net_device) (uninitialized): Could not allocate enough VIs to satisfy RSS requirements. Performance may be impaired.
[ 7382.352895] sfc 0000:38:00.2 p2p1_0: renamed from eth0
[ 7677.279414] sfc 0000:38:00.0 ens2f0np0: VF 1 has been reset to reconfigure MAC
[ 7677.279660] vfio-pci 0000:38:00.3: vfio_pci: Relaying device request to user (#0)
[ 7678.373409] unlink ovsbr-mgqaqnpu3 as upper device of vnet1
[ 7678.373414] unlink port-mgt as upper device of vnet1
[ 7678.373422] device vnet1 left promiscuous mode
[ 7678.435912] sfc 0000:38:00.3 (unnamed net_device) (uninitialized): Solarflare NIC detected: device 1924:1b03 subsys 1924:8028
[ 7678.435917] sfc 0000:38:00.3: enabling device (0000 -> 0002)
[ 7678.440026] sfc 0000:38:00.3 (unnamed net_device) (uninitialized): WARNING: Insufficient MSI-X vectors available (8 < 32).
[ 7678.452556] sfc 0000:38:00.3 (unnamed net_device) (uninitialized): WARNING: Performance may be reduced.
[ 7678.463957] sfc 0000:38:00.3 (unnamed net_device) (uninitialized): Could not allocate enough VIs to satisfy RSS requirements. Performance may be impaired.
```

## 错误忽然变的寻常起来了

```txt
[426428.781771] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!
[551014.355307] NOHZ tick-stop error: Non-RCU local softirq work is pending, handler #08!!!
```

## 6.18 内核找到这个警告了
```txt
[   13.678727] XFS (vda2): Ending recovery (logdev: internal)
[   15.034245]
[   15.034336] ============================================
[   15.034537] WARNING: possible recursive locking detected
[   15.034738] 6.18.1-martins3-00001-g344d23e3a12a #34 Not tainted
[   15.034999] --------------------------------------------
[   15.035198] abrt-dump-journ/911 is trying to acquire lock:
[   15.035410] ffff88800df80118 (&xfs_dir_ilock_class){++++}-{4:4}, at: xfs_icwalk_ag+0x488/0x980 [xfs]
[   15.035915]
but task is already holding lock:
[   15.036101] ffff8880079ea418 (&xfs_dir_ilock_class){++++}-{4:4}, at: xfs_ilock_data_map_shared+0x1f/0x30 [xfs]
[   15.036596]
other info that might help us debug this:
[   15.036809]  Possible unsafe locking scenario:

[   15.036998]        CPU0
[   15.037088]        ----
[   15.037179]   lock(&xfs_dir_ilock_class);
[   15.037331]   lock(&xfs_dir_ilock_class);
[   15.037481]
 *** DEADLOCK ***

[   15.037639]  May be due to missing lock nesting notation

[   15.037859] 2 locks held by abrt-dump-journ/911:
[   15.038033]  #0: ffff8880079ea418 (&xfs_dir_ilock_class){++++}-{4:4}, at: xfs_ilock_data_map_shared+0x1f/0x30 [xfs]
[   15.038540]  #1: ffff888007c7c0e0 (&type->s_umount_key#48){++++}-{4:4}, at: super_cache_scan+0x3b/0x1d0
[   15.041967]
stack backtrace:
[   15.042104] CPU: 4 UID: 0 PID: 911 Comm: abrt-dump-journ Not tainted 6.18.1-martins3-00001-g344d23e3a12a #34 PREEMPT(full)
[   15.042105] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[   15.042107] Call Trace:
[   15.042108]  <TASK>
[   15.042108]  dump_stack_lvl+0x6f/0xb0
[   15.042114]  print_deadlock_bug.cold+0xbd/0xca
[   15.042118]  __lock_acquire+0x124a/0x2200
[   15.042122]  lock_acquire+0xbe/0x2e0
[   15.042124]  ? xfs_icwalk_ag+0x488/0x980 [xfs]
[   15.042216]  down_write_nested+0x42/0xd0
[   15.042217]  ? xfs_icwalk_ag+0x488/0x980 [xfs]
[   15.042307]  xfs_icwalk_ag+0x488/0x980 [xfs]
[   15.042394]  ? xfs_icwalk_ag+0x69/0x980 [xfs]
[   15.042481]  ? xa_find+0x70/0x200
[   15.042487]  xfs_icwalk+0x3e/0x70 [xfs]
[   15.042575]  xfs_reclaim_inodes_nr+0x98/0xd0 [xfs]
[   15.042664]  super_cache_scan+0x179/0x1d0
[   15.042666]  do_shrink_slab+0x16c/0x690
[   15.042672]  shrink_slab+0x495/0x8b0
[   15.042673]  ? shrink_slab+0x2d6/0x8b0
[   15.042675]  shrink_node+0x43b/0x1390
[   15.042677]  ? mark_held_locks+0x40/0x70
[   15.042678]  ? do_try_to_free_pages+0xb3/0x560
[   15.042679]  do_try_to_free_pages+0xb3/0x560
[   15.042680]  try_to_free_pages+0xf3/0x290
[   15.042682]  __alloc_pages_slowpath.constprop.0+0x39c/0xeb0
[   15.042688]  __alloc_frozen_pages_noprof+0x33a/0x380
[   15.042690]  alloc_pages_mpol+0x48/0x100
[   15.042693]  folio_alloc_noprof+0x5b/0xa0
[   15.042694]  xfs_buf_alloc+0x3b3/0x7f0 [xfs]
[   15.042783]  xfs_buf_get_map+0x63d/0x12d0 [xfs]
[   15.042871]  ? xfs_buf_readahead_map+0x42/0x1e0 [xfs]
[   15.042955]  xfs_buf_readahead_map+0x42/0x1e0 [xfs]
[   15.043043]  xfs_da_reada_buf+0xb3/0xc0 [xfs]
[   15.043121]  xfs_dir_open+0x7c/0x90 [xfs]
[   15.043209]  ? __pfx_xfs_dir_open+0x10/0x10 [xfs]
[   15.043297]  do_dentry_open+0x14c/0x440
[   15.043300]  vfs_open+0x34/0xf0
[   15.043301]  path_openat+0x825/0xb20
[   15.043305]  do_filp_open+0xd7/0x190
[   15.043308]  ? alloc_fd+0x128/0x200
[   15.043312]  do_sys_openat2+0x8a/0xe0
[   15.043313]  __x64_sys_openat+0x54/0xa0
[   15.043315]  do_syscall_64+0x74/0xfa0
[   15.043317]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[   15.043318] RIP: 0033:0x7ff51a189526
[   15.043320] Code: 4c 89 55 c8 41 89 f2 41 83 e2 40 75 37 89 f2 f7 d2 81 e2 00 00 41 00 74 2b 89 f2 bf 9c ff ff ff 48 89 c6 b8 01 01 00 00 0f 05 <48> 3d 00 f0 ff ff 77 32 48 8b 55 c8 64 48 2b 14 25 28 00 00 00 75
[   15.043320] RSP: 002b:00007ffe8fc21c10 EFLAGS: 00000206 ORIG_RAX: 0000000000000101
[   15.043322] RAX: ffffffffffffffda RBX: 00007ffe8fc21cc0 RCX: 00007ff51a189526
[   15.043322] RDX: 0000000000090800 RSI: 000055aaf55af630 RDI: 00000000ffffff9c
[   15.043323] RBP: 00007ffe8fc21c60 R08: 0000000000000036 R09: 0000000000000000
[   15.043323] R10: 0000000000000000 R11: 0000000000000206 R12: 00000000fffffff7
[   15.043325] R13: 000055aaf558f710 R14: 000055aaf5569920 R15: 000055aaf55caed0
[   15.043326]  </TASK>
[ 1957.028807] hrtimer: interrupt took 40430 ns
+ set +x
```

## 给 19.60 切换内核之后，然后就无法启动了
```txt
[   10.790250] iommu: Adding device 0000:bc:00.0 to group 19
[   10.799018] iommu: Adding device 0000:b4:01.0 to group 20
[   10.815924] rtc-efi rtc-efi: setting system clock to 2025-12-11 09:43:46 UTC (1765446226)
[   10.832069] SDEI NMI watchdog: SDEI Watchdog registered successfully
[   10.842486] md: Waiting for all devices to be available before autodetect
[   10.852065] md: If you don't use raid, use raid=noautodetect
[   10.860749] md: Autodetecting RAID arrays.
[   10.867564] md: autorun ...
[   10.873051] md: ... autorun DONE.
[   10.879011] List of all partitions:
[   10.885025] No filesystem could mount root, tried:
[   10.885026]
[   10.896308] Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0)
[   10.922862] Hardware name: Yunke China KunTai R722/BC82AMDDRA, BIOS 1.89 05/20/2022
[   10.935727] Call trace:
[   10.940716]  dump_backtrace+0x0/0x1a0
[   10.946843]  show_stack+0x24/0x30
[   10.952536]  dump_stack+0xa4/0xe8
[   10.958160]  panic+0x134/0x320
[   10.963512]  mount_block_root+0x298/0x360
[   10.969790]  mount_root+0x88/0x98
[   10.975391]  prepare_namespace+0x144/0x1a8
[   10.988255]  kernel_init+0x18/0x118
[   10.993831]  ret_from_fork+0x10/0x18
[   10.999451] SMP: stopping secondary CPUs
[   11.005355] Kernel Offset: disabled
[   11.010781] CPU features: 0x12,aa200a38
[   11.016571] Memory Limit: none
[   11.021598] ---[ end Kernel panic - not syncing: VFS: Unable to mount root fs on unknown-block(0,0) ]---
```

## Synchronous Exception at 0x0000000025F7BF9C
其实这个问题和 arm 没什么关系了，只是 arm 下容易出现这个问题而已:

### kunpeng 服务器 通过 bmc 来安装 Fedora 43，进入到 grub ，当一开始选择安装，然后就触发错误

```txt
GRUB version 2.12


/----------------------------------------------------------------------------\||||||||||||||||||||||||||\----------------------------------------------------------------------------/     Use the ^ and v keys to select which entry is highlighted.

      Press enter to boot the selected OS, `e' to edit the commands

      before booting or `c' for a command-line.
 Install Fedora 43                                                          *Test this media & install Fedora 43                                         Troubleshooting -->
   The highlighted entry will be executed automatically in 60s.                   The highlighted entry will be executed automatically in 59s.                   The highlighted entry will be executed automatically in 58s.                   The highlighted entry will be executed automatically in 57s.                   The highlighted entry will be executed automatically in 56s.                   The highlighted entry will be executed automatically in 55s.                   The highlighted entry will be executed automatically in 54s.                   The highlighted entry will be executed automatically in 53s.                   The highlighted entry will be executed automatically in 52s.                   The highlighted entry will be executed automatically in 51s.                   The highlighted entry will be executed automatically in 50s.                   The highlighted entry will be executed automatically in 49s.                   The highlighted entry will be executed automatically in 48s.                   The highlighted entry will be executed automatically in 47s.                   The highlighted entry will be executed automatically in 46s.                   The highlighted entry will be executed automatically in 45s.                   The highlighted entry will be executed automatically in 44s.                   The highlighted entry will be executed automatically in 43s.                   The highlighted entry will be executed automatically in 42s.                   The highlighted entry will be executed automatically in 41s.                   The highlighted entry will be executed automatically in 40s.                   The highlighted entry will be executed automatically in 39s.                   The highlighted entry will be executed automatically in 38s.                   The highlighted entry will be executed automatically in 37s.                   The highlighted entry will be executed automatically in 36s.                   The highlighted entry will be executed automatically in 35s.                   The highlighted entry will be executed automatically in 34s.                   The highlighted entry will be executed automatically in 33s.                   The highlighted entry will be executed automatically in 32s.                   The highlighted entry will be executed automatically in 31s.                   The highlighted entry will be executed automatically in 30s.                   The highlighted entry will be executed automatically in 29s.                   The highlighted entry will be executed automatically in 28s.                   The highlighted entry will be executed automatically in 27s.                   The highlighted entry will be executed automatically in 26s.                   The highlighted entry will be executed automatically in 25s.                   The highlighted entry will be executed automatically in 24s.                   The highlighted entry will be executed automatically in 23s.                   The highlighted entry will be executed automatically in 22s.                   The highlighted entry will be executed automatically in 21s.                   The highlighted entry will be executed automatically in 20s.                   The highlighted entry will be executed automatically in 19s.                   The highlighted entry will be executed automatically in 18s.                   The highlighted entry will be executed automatically in 17s.                   The highlighted entry will be executed automatically in 16s.                   The highlighted entry will be executed automatically in 15s.                   The highlighted entry will be executed automatically in 14s.                   The highlighted entry will be executed automatically in 13s.                   The highlighted entry will be executed automatically in 12s.                   The highlighted entry will be executed automatically in 11s.                   The highlighted entry will be executed automatically in 10s.                                                                                                                                                                               Test this media & install Fedora 43                                        *Install Fedora 43

Synchronous Exception at 0x0000000025F7BF9C


Synchronous Exception at 0x0000000025F7BF9C
PC 0x000025F7BF9C
PC 0x000025F7BF08
PC 0x000025F803FC
PC 0x000025F7F6A4
PC 0x000025F8B298
PC 0x205FFFD94CF4
PC 0x205FFFF3F2B8
PC 0x205FFFF3EA90
PC 0x205FFFF3F464
PC 0x205FFFF3EA90
PC 0x205FFFF3F9A0
PC 0x205FFFF3EDAC
PC 0x205FFFF3EE50
PC 0x205FFFF30A8C
PC 0x205FFFF31D6C
PC 0x205FFFF31DD0
PC 0x205FFFF2ED14
PC 0x205FFFF2EDE8
PC 0x205FFFF2F048
PC 0x205FFFF2F1E4
PC 0x205FFFF2F234
PC 0x000025F8C838
PC 0x000025F8D3A0
PC 0x000025F8D778
PC 0x00003F0CA488 (0x00003F0BE000+0x0000C488) [ 1] DxeCore.dll
PC 0x00002F73981C (0x00002F72E000+0x0000B81C) [ 2] BdsDxe.dll
PC 0x00002F73A8CC (0x00002F72E000+0x0000C8CC) [ 2] BdsDxe.dll
PC 0x00002F7413EC (0x00002F72E000+0x000133EC) [ 2] BdsDxe.dll
PC 0x00003F0CECB8 (0x00003F0BE000+0x00010CB8) [ 3] DxeCore.dll
[ 1] /usr1/output/TaiShan2281C/RELEASE_GCC5/AARCH64/MdeModulePkg/Core/Dxe/DxeMain/DEBUG/DxeCore.dll
[ 2] /usr1/output/TaiShan2281C/RELEASE_GCC5/AARCH64/vendor/ByoModulePkg/ByoModulePkg/BdsDxe/BdsDxe/DEBUG/BdsDxe.dll
[ 3] /usr1/output/TaiShan2281C/RELEASE_GCC5/AARCH64/MdeModulePkg/Core/Dxe/DxeMain/DEBUG/DxeCore.dll

  X0 0xAFAFAFAFAFAFAFAF   X1 0x0000000000000063   X2 0x0000000000000000   X3 0x0000000025F99080
  X4 0x0000205FFFB14780   X5 0x0000000000000020   X6 0x0000000000000064   X7 0x000000000000002D
  X8 0x000000002ED23968   X9 0x000000000000001F  X10 0x0000002E00000006  X11 0x0000080600000001
 X12 0x0000000000001490  X13 0x0098989800989898  X14 0x000000003F0BD3C0  X15 0x000000003F0D906C
 X16 0x0000000025F8AB9C  X17 0x0000000000001348  X18 0x0000000000000000  X19 0x0000000000000000
 X20 0x000000000000000D  X21 0x000000002F739000  X22 0x000000002F739000  X23 0x0000000000000000
 X24 0x0000000000000000  X25 0x000000002F739000  X26 0x0000000000000001  X27 0x0000000000000000
 X28 0x0000000000000000   FP 0x000000003F0BCFC0   LR 0x0000000025F7BF08

  V0 0x0000000000000000 0000000000000000   V1 0x000000000000DC84 000000000000DCD2
  V2 0x6D65747379730031 6168732D336C7373   V3 0x0000000000000000 0000000000000000
  V4 0x0000000000000000 0000000000000000   V5 0x4010040140100401 4010040140100401
  V6 0x0000000000000000 0000000000000000   V7 0x0000000000000000 0000000000000000
  V8 0x0000000000000000 0000000000000000   V9 0x0000000000000000 0000000000000000
 V10 0x0000000000000000 0000000000000000  V11 0x0000000000000000 0000000000000000
 V12 0x0000000000000000 0000000000000000  V13 0x0000000000000000 0000000000000000
 V14 0x0000000000000000 0000000000000000  V15 0x0000000000000000 0000000000000000
 V16 0x0000000000000000 0000000000000000  V17 0x0000000000000000 0000000000000000
 V18 0x0000000000000000 0000000000000000  V19 0x0000000000000000 0000000000000000
 V20 0x0000000000000000 0000000000000000  V21 0x0000000000000000 0000000000000000
 V22 0x0000000000000000 0000000000000000  V23 0x0000000000000000 0000000000000000
 V24 0x0000000000000000 0000000000000000  V25 0x0000000000000000 0000000000000000
 V26 0x0002000068929B00 0000000373717368  V27 0x00000004000200C0 0011000300000000
 V28 0x000000000017A800 0000000000000002  V29 0x0000000000000002 0000000017200000
 V30 0x000000003F0BCFE0 000000003F0BCFE0  V31 0xFFFFFF80FFFFFFE0 000000003F0BCFC0

  SP 0x000000003F0BCFC0  ELR 0x0000000025F7BF9C  SPSR 0x20000209  FPSR 0x00000000
 ESR 0x96000004          FAR 0xAFAFAFAFAFAFAFC7

 ESR : EC 0x25  IL 0x1  ISS 0x00000004

Data abort: Translation fault, zeroth level

Stack dump:
  000003F0BCEC0: 0000000000000000 0000000000000000 0000000000000000 0000000000000000
  000003F0BCEE0: 0000000000000000 0000000000000000 0000000000000000 0000000000000000
  000003F0BCF00: 0000000000000000 0000000000000000 0000000000000000 0000000000000000
  000003F0BCF20: 0000000000000000 0000000000000000 0000000373717368 0002000068929B00
  000003F0BCF40: 0011000300000000 00000004000200C0 0000000000000002 000000000017A800
  000003F0BCF60: 0000000017200000 0000000000000002 000000003F0BCFE0 000000003F0BCFE0
  000003F0BCF80: 000000003F0BCFC0 FFFFFF80FFFFFFE0 000000002F968628 0000000020000308
  000003F0BCFA0: 0000000000000000 0000000096000004 AFAFAFAFAFAFAFC7 0000205FFFFFF060
> 000003F0BCFC0: 000000003F0BD000 0000000025F803FC 0000205FFFB1A360 0000205FFFB14780
  000003F0BCFE0: 000000003F0BD000 AFAFAFAFAFAFAFAF 00000000FFB1A360 0000205FFFFFF060
  000003F0BD000: 000000003F0BD050 0000000025F7F6A4 0000000B00000000 0000205FFFB14780
  000003F0BD020: 000000003F0BD050 0000205FFE000000 0000000000000000 0000205FFFB1A360
  000003F0BD040: 0000205FFFB14780 0000000025F9CE18 000000003F0BD080 0000000025F8B298
  000003F0BD060: 0000205FFFB1A160 0000205FFFB14780 0000000000000064 0000205FFFB1A3E0
  000003F0BD080: 000000003F0BD0D0 0000205FFFD94CF4 000000033F0BD0B0 0000205FFFB1A160
  000003F0BD0A0: 000000003F0BD0D0 0000205FFFB1A160 00000000
```


## 热插内存，结果 oom 了
6.18.3-martins3-00001-gd99e6e036338-dirty

```txt
               total        used        free      shared  buff/cache   available
Mem:           2.3Gi       956Mi       219Mi        10Mi       1.2Gi       1.4Gi
Swap:          2.3Gi        20Ki       2.3Gi
```
当时还有很多 page cache 吧

配置虚拟机 3G 内存，这应该是一个 bug 吧

```txt
[  394.608211] kworker/u128:5: vmemmap alloc failure: order:9, mode:0x4cc0(GFP_KERNEL|__GFP_RETRY_MAYFAIL), nodemask=(null),cpuset=/,
mems_allowed=0
[  394.608786] CPU: 4 UID: 0 PID: 744 Comm: kworker/u128:5 Not tainted 6.18.3-martins3-00001-gd99e6e036338-dirty #37 PREEMPT(full)
[  394.608788] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[  394.608791] Workqueue: kacpi_hotplug acpi_hotplug_work_fn
[  394.608800] Call Trace:
[  394.608803]  <TASK>
[  394.608804]  dump_stack_lvl+0x6f/0xb0
[  394.608811]  warn_alloc+0x162/0x190
[  394.608822]  vmemmap_alloc_block+0xd2/0xe0
[  394.608826]  vmemmap_populate_hugepages+0xdd/0x190
[  394.608828]  vmemmap_populate+0x3e/0x70
[  394.608833]  __populate_section_memmap+0x90/0x300
[  394.608835]  sparse_add_section+0x14c/0x310
[  394.608839]  __add_pages+0x9d/0x130
[  394.608843]  add_pages+0x28/0x80
[  394.608844]  add_memory_resource+0x115/0x350
[  394.608848]  __add_memory+0x3c/0x80
[  394.608850]  acpi_memory_device_add+0x1a2/0x300
[  394.608856]  acpi_bus_attach+0x1e2/0x2f0
[  394.608859]  ? __pfx_acpi_dev_for_one_check+0x10/0x10
[  394.608861]  device_for_each_child+0x76/0xc0
[  394.608867]  acpi_dev_for_each_child+0x3b/0x60
[  394.608868]  ? __pfx_acpi_bus_attach+0x10/0x10
[  394.608870]  acpi_bus_attach+0x94/0x2f0
[  394.608873]  acpi_bus_scan+0x67/0x1e0
[  394.608876]  acpi_scan_rescan_bus+0x45/0x50
[  394.608877]  acpi_device_hotplug+0x366/0x440
[  394.608879]  acpi_hotplug_work_fn+0x1e/0x30
[  394.608880]  process_one_work+0x1f8/0x580
[  394.608885]  worker_thread+0x1ce/0x3c0
[  394.608887]  ? __pfx_worker_thread+0x10/0x10
[  394.608888]  kthread+0x10f/0x230
[  394.608890]  ? __pfx_kthread+0x10/0x10
[  394.608891]  ? __pfx_kthread+0x10/0x10
[  394.608893]  ret_from_fork+0x21e/0x280
[  394.608896]  ? __pfx_kthread+0x10/0x10
[  394.608897]  ret_from_fork_asm+0x1a/0x30
[  394.608902]  </TASK>
[  394.608903] Mem-Info:
[  394.612835] active_anon:1379 inactive_anon:75083 isolated_anon:0
                active_file:63627 inactive_file:279531 isolated_file:0
                unevictable:1000 dirty:111 writeback:0
                slab_reclaimable:22472 slab_unreclaimable:33138
                mapped:67499 shmem:2703 pagetables:3155
                sec_pagetables:8 bounce:0
                kernel_misc_reclaimable:0
                free:72919 free_pcp:250 free_cma:0
[  394.615101] Node 0 active_anon:5516kB inactive_anon:300332kB active_file:254508kB inactive_file:1118124kB unevictable:4000kB isola
ted(anon):0kB isolated(file):0kB mapped:269996kB dirty:444kB writeback:0kB shmem:10812kB kernel_stack:7760kB pagetables:12620kB sec_p
agetables:32kB all_unreclaimable? no Balloon:0kB
[  394.617314] Node 0 DMA free:11264kB boost:0kB min:36kB low:48kB high:60kB reserved_highatomic:0KB free_highatomic:0KB active_anon:
0kB inactive_anon:0kB active_file:0kB inactive_file:0kB unevictable:0kB writepending:0kB zspages:0kB present:15992kB managed:15360kB
mlocked:0kB bounce:0kB free_pcp:0kB local_pcp:0kB free_cma:0kB
[  394.619825] lowmem_reserve[]: 0 2375 2375 2375
[  394.620541] Node 0 DMA32 free:280412kB boost:0kB min:6140kB low:8512kB high:10884kB reserved_highatomic:0KB free_highatomic:0KB ac
tive_anon:5516kB inactive_anon:300332kB active_file:254492kB inactive_file:1117948kB unevictable:4000kB writepending:684kB zspages:20
kB present:3129172kB managed:2432528kB mlocked:0kB bounce:0kB free_pcp:1000kB local_pcp:0kB free_cma:0kB
[  394.623024] lowmem_reserve[]: 0 0 0 0
[  394.623588] Node 0 DMA: 0*4kB 0*8kB 0*16kB 0*32kB 0*64kB 0*128kB 0*256kB 0*512kB 1*1024kB (U) 1*2048kB (M) 2*4096kB (M) = 11264kB
[  394.624527] Node 0 DMA32: 5336*4kB (UME) 3113*8kB (UME) 1728*16kB (UME) 1138*32kB (UME) 589*64kB (UME) 307*128kB (UME) 118*256kB (
UME) 67*512kB (UM) 28*1024kB (M) 0*2048kB 0*4096kB = 280488kB
[  394.625838] Node 0 hugepages_total=0 hugepages_free=0 hugepages_surp=0 hugepages_size=1048576kB
[  394.626568] Node 0 hugepages_total=0 hugepages_free=0 hugepages_surp=0 hugepages_size=2048kB
[  394.627332] 345835 total pagecache pages
[  394.627909] 0 pages in swap cache
[  394.628489] Free swap  = 2447336kB
[  394.629086] Total swap = 2447356kB
[  394.629713] 786291 pages RAM
[  394.630419] 0 pages HighMem/MovableOnly
[  394.631114] 174319 pages reserved
```

## 6.18.3-martins3-00001-gd99e6e036338 虚拟机中，似乎内存不足的时候就会触发

```txt
[Sat Jan 10 01:01:05 2026] ============================================
[Sat Jan 10 01:01:05 2026] WARNING: possible recursive locking detected
[Sat Jan 10 01:01:05 2026] 6.18.3-martins3-00001-gd99e6e036338-dirty #37 Not tainted
[Sat Jan 10 01:01:05 2026] --------------------------------------------
[Sat Jan 10 01:01:05 2026] updatedb/12224 is trying to acquire lock:
[Sat Jan 10 01:01:05 2026] ffff8880241cf118 (&xfs_dir_ilock_class){++++}-{4:4}, at: xfs_icwalk_ag+0x488/0x980 [xfs]
[Sat Jan 10 01:01:05 2026]
                           but task is already holding lock:
[Sat Jan 10 01:01:05 2026] ffff88802668d518 (&xfs_dir_ilock_class){++++}-{4:4}, at: xfs_ilock_data_map_shared+0x1f/0x30 [xfs]
[Sat Jan 10 01:01:05 2026]
                           other info that might help us debug this:
[Sat Jan 10 01:01:05 2026]  Possible unsafe locking scenario:

[Sat Jan 10 01:01:05 2026]        CPU0
[Sat Jan 10 01:01:05 2026]        ----
[Sat Jan 10 01:01:05 2026]   lock(&xfs_dir_ilock_class);
[Sat Jan 10 01:01:05 2026]   lock(&xfs_dir_ilock_class);
[Sat Jan 10 01:01:05 2026]
                            *** DEADLOCK ***

[Sat Jan 10 01:01:05 2026]  May be due to missing lock nesting notation

[Sat Jan 10 01:01:05 2026] 2 locks held by updatedb/12224:
[Sat Jan 10 01:01:05 2026]  #0: ffff88802668d518 (&xfs_dir_ilock_class){++++}-{4:4}, at: xfs_ilock_data_map_shared+0x1f/0x30 [xfs]
[Sat Jan 10 01:01:05 2026]  #1: ffff888009f0c0e0 (&type->s_umount_key#50){++++}-{4:4}, at: super_cache_scan+0x3b/0x1d0
[Sat Jan 10 01:01:05 2026]
                           stack backtrace:
[Sat Jan 10 01:01:05 2026] CPU: 9 UID: 0 PID: 12224 Comm: updatedb Not tainted 6.18.3-martins3-00001-gd99e6e036338-dirty #37 PREEMPT(full)
[Sat Jan 10 01:01:05 2026] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[Sat Jan 10 01:01:05 2026] Call Trace:
[Sat Jan 10 01:01:05 2026]  <TASK>
[Sat Jan 10 01:01:05 2026]  dump_stack_lvl+0x6f/0xb0
[Sat Jan 10 01:01:05 2026]  print_deadlock_bug.cold+0xbd/0xca
[Sat Jan 10 01:01:05 2026]  __lock_acquire+0x124a/0x2200
[Sat Jan 10 01:01:05 2026]  lock_acquire+0xbe/0x2e0
[Sat Jan 10 01:01:05 2026]  ? xfs_icwalk_ag+0x488/0x980 [xfs]
[Sat Jan 10 01:01:05 2026]  down_write_nested+0x42/0xd0
[Sat Jan 10 01:01:05 2026]  ? xfs_icwalk_ag+0x488/0x980 [xfs]
[Sat Jan 10 01:01:05 2026]  xfs_icwalk_ag+0x488/0x980 [xfs]
[Sat Jan 10 01:01:05 2026]  ? xfs_icwalk_ag+0x69/0x980 [xfs]
[Sat Jan 10 01:01:05 2026]  ? xa_find+0x70/0x200
[Sat Jan 10 01:01:05 2026]  xfs_icwalk+0x3e/0x70 [xfs]
[Sat Jan 10 01:01:05 2026]  xfs_reclaim_inodes_nr+0x98/0xd0 [xfs]
[Sat Jan 10 01:01:05 2026]  super_cache_scan+0x179/0x1d0
[Sat Jan 10 01:01:05 2026]  do_shrink_slab+0x16c/0x690
[Sat Jan 10 01:01:05 2026]  shrink_slab+0x495/0x8b0
[Sat Jan 10 01:01:05 2026]  ? shrink_slab+0x2d6/0x8b0
[Sat Jan 10 01:01:05 2026]  shrink_node+0x43b/0x1390
[Sat Jan 10 01:01:05 2026]  ? __lock_acquire+0x475/0x2200
[Sat Jan 10 01:01:05 2026]  ? do_try_to_free_pages+0xb3/0x560
[Sat Jan 10 01:01:05 2026]  do_try_to_free_pages+0xb3/0x560
[Sat Jan 10 01:01:05 2026]  try_to_free_pages+0xf3/0x290
[Sat Jan 10 01:01:05 2026]  __alloc_pages_slowpath.constprop.0+0x39c/0xeb0
[Sat Jan 10 01:01:05 2026]  __alloc_frozen_pages_noprof+0x33a/0x380
[Sat Jan 10 01:01:05 2026]  alloc_pages_mpol+0x48/0x100
[Sat Jan 10 01:01:05 2026]  folio_alloc_noprof+0x5b/0xa0
[Sat Jan 10 01:01:05 2026]  xfs_buf_alloc+0x3b3/0x7f0 [xfs]
[Sat Jan 10 01:01:05 2026]  xfs_buf_get_map+0x63d/0x12d0 [xfs]
[Sat Jan 10 01:01:05 2026]  ? xfs_buf_readahead_map+0x42/0x1e0 [xfs]
[Sat Jan 10 01:01:05 2026]  xfs_buf_readahead_map+0x42/0x1e0 [xfs]
[Sat Jan 10 01:01:05 2026]  xfs_da_reada_buf+0xb3/0xc0 [xfs]
[Sat Jan 10 01:01:05 2026]  xfs_dir_open+0x7c/0x90 [xfs]
[Sat Jan 10 01:01:05 2026]  ? __pfx_xfs_dir_open+0x10/0x10 [xfs]
[Sat Jan 10 01:01:05 2026]  do_dentry_open+0x14c/0x440
[Sat Jan 10 01:01:05 2026]  vfs_open+0x34/0xf0
[Sat Jan 10 01:01:05 2026]  path_openat+0x825/0xb20
[Sat Jan 10 01:01:05 2026]  do_filp_open+0xd7/0x190
[Sat Jan 10 01:01:05 2026]  ? alloc_fd+0x128/0x200
[Sat Jan 10 01:01:05 2026]  do_sys_openat2+0x8a/0xe0
[Sat Jan 10 01:01:05 2026]  __x64_sys_openat+0x54/0xa0
[Sat Jan 10 01:01:05 2026]  do_syscall_64+0x74/0xfa0
[Sat Jan 10 01:01:05 2026]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[Sat Jan 10 01:01:05 2026] RIP: 0033:0x7f726001277e
[Sat Jan 10 01:01:05 2026] Code: 4d 89 d8 e8 d4 bc 00 00 4c 8b 5d f8 41 8b 93 08 03 00 00 59 5e 48 83 f8 fc 74 11 c9 c3 0f 1f 80 00 00 00 00 48 8b 45 10 0f 05 <c9> c3 83 e2 39 83 fa 08 75 e7 e8 13 ff ff ff 0f 1f 00 f3 0f 1e fa
[Sat Jan 10 01:01:05 2026] RSP: 002b:00007ffe045984d0 EFLAGS: 00000202 ORIG_RAX: 0000000000000101
[Sat Jan 10 01:01:05 2026] RAX: ffffffffffffffda RBX: 0000000000000000 RCX: 00007f726001277e
[Sat Jan 10 01:01:05 2026] RDX: 0000000000010000 RSI: 000055c2b7a77230 RDI: 00000000000004c7
[Sat Jan 10 01:01:05 2026] RBP: 00007ffe045984e0 R08: 0000000000000000 R09: 0000000000000000
[Sat Jan 10 01:01:05 2026] R10: 0000000000000000 R11: 0000000000000202 R12: 0000000000000000
[Sat Jan 10 01:01:05 2026] R13: 00007ffe04598960 R14: 000055c2b7a77220 R15: 0000000000000000
[Sat Jan 10 01:01:05 2026]  </TASK>
```

## 类似这种在 kernel schduler 或者 cgroup 的位置出现错误的，都不是 guest kernel 自身的问题
```txt
[339073.057117]  [<ffffffff9d0eab04>] update_curr+0x164/0x1f0
[339073.057574]  [<ffffffff9d0eaf28>] dequeue_entity+0x28/0x5d0
[339073.058027]  [<ffffffff9d0706fe>] ? kvm_sched_clock_read+0x1e/0x30
[339073.058490]  [<ffffffff9d0eb523>] dequeue_task_fair+0x53/0x660
[339073.058956]  [<ffffffff9d0e4345>] ? sched_clock_cpu+0x85/0xc0
[339073.059427]  [<ffffffff9d0dd006>] deactivate_task+0x46/0xe0
[339073.059902]  [<ffffffff9d7b7b85>] __schedule+0x585/0x680
[339073.060384]  [<ffffffff9d7b7ca9>] schedule+0x29/0x70
[339073.060862]  [<ffffffff9d7b7012>] schedule_hrtimeout_range_clock+0xb2/0x160
[339073.061338]  [<ffffffff9d0cf340>] ? hrtimer_get_res+0x50/0x50
[339073.061848]  [<ffffffff9d7b7006>] ? schedule_hrtimeout_range_clock+0xa6/0x160
[339073.062349]  [<ffffffff9d7b70d3>] schedule_hrtimeout_range+0x13/0x20
[339073.062865]  [<ffffffff9d2acf7e>] ep_poll+0x24e/0x370
[339073.063384]  [<ffffffff9d27d738>] ? __fget_light+0x28/0x140
[339073.063902]  [<ffffffff9d0e1170>] ? wake_up_state+0x20/0x20
[339073.064420]  [<ffffffff9d2ae45d>] SyS_epoll_wait+0xdd/0x110
[339073.064947]  [<ffffffff9d146966>] ? __audit_syscall_exit+0x1f6/0x2b0
[339073.065482]  [<ffffffff9d7c539a>] system_call_fastpath+0x25/0x2a
```

## 嵌套虚拟化中有不可忽视的因素是来自于 spinlock

2026-01-20 : 这是当时的记录，我无法确认当时的环境配置
```txt
+   95.41%     0.01%  qemu-system-x86  libc.so.6                                [.] __GI___ioctl
+   92.27%     0.00%  qemu-system-x86  [kernel.kallsyms]                        [k] entry_SYSCALL_64_after_hwframe
+   92.27%     0.01%  qemu-system-x86  [kernel.kallsyms]                        [k] do_syscall_64
+   90.88%     0.01%  qemu-system-x86  [kernel.kallsyms]                        [k] __x64_sys_ioctl
+   90.81%     0.01%  qemu-system-x86  [kernel.kallsyms]                        [k] kvm_vcpu_ioctl
+   90.79%     0.53%  qemu-system-x86  [kernel.kallsyms]                        [k] kvm_arch_vcpu_ioctl_run
+   88.93%     1.80%  qemu-system-x86  [kernel.kallsyms]                        [k] vcpu_enter_guest.constprop.0
+   43.89%     0.37%  qemu-system-x86  [kernel.kallsyms]                        [k] vmx_handle_exit
+   21.13%     0.23%  qemu-system-x86  [kernel.kallsyms]                        [k] nested_vmx_run
+   18.09%     0.87%  qemu-system-x86  [kernel.kallsyms]                        [k] nested_vmx_enter_non_root_mode
+   15.85%     1.78%  qemu-system-x86  [kernel.kallsyms]                        [k] vmx_vcpu_run
-   14.85%    14.75%  qemu-system-x86  [kernel.kallsyms]                        [k] native_queued_spin_lock_slowpath
     14.74% __GI___ioctl
        entry_SYSCALL_64_after_hwframe
        do_syscall_64
        __x64_sys_ioctl
        kvm_vcpu_ioctl
      - kvm_arch_vcpu_ioctl_run
         - 14.74% vcpu_enter_guest.constprop.0
            - 14.69% vmx_handle_exit
               - 9.59% kvm_mmu_page_fault
                  - 9.57% kvm_mmu_do_page_fault
                     - 9.54% ept_page_fault
                        - queued_write_lock_slowpath
                             native_queued_spin_lock_slowpath
               - 5.03% nested_vmx_run
                  - 5.03% nested_vmx_enter_non_root_mode
                       nested_get_vmcs12_pages
                       __kvm_vcpu_map
                       hva_to_pfn
                       pin_user_pages_unlocked
                     - __gup_longterm_locked
                        - 5.01% __get_user_pages
                             follow_page_pte
                             __pte_offset_map_lock
                           - _raw_spin_lock
                                native_queued_spin_lock_slowpath
```
## CPPC 错误
CPPC（Collaborative Processor Performance Control）是 ACPI 规范中用于 CPU 调频/电源管理的一种机制。
PCC（Platform Communication Channel）是 CPU 与主板固件（BIOS/BMC）通信的通道。

PPC 之后异常之后，多个设备超时
```txt
Mar 10 17:31:31 ACPI CPPC: PCC check channel failed for ss: 0. ret=-110
Mar 10 17:31:33 ACPI CPPC: PCC check channel failed for ss: 0. ret=-110
Mar 10 17:31:35 ACPI CPPC: PCC check channel failed for ss: 0. ret=-110
Mar 10 17:31:37 ACPI CPPC: PCC check channel failed for ss: 0. ret=-110
Mar 10 17:31:39 ACPI CPPC: PCC check channel failed for ss: 0. ret=-110
Mar 10 17:31:41 ACPI CPPC: PCC check channel failed for ss: 0. ret=-110
Mar 10 17:31:42 ------------[ cut here ]------------
Mar 10 17:31:42 NETDEV WATCHDOG: p2p2 (igb): transmit queue 5 timed out
Mar 10 17:31:42 WARNING: CPU: 18 PID: 0 at net/sched/sch_generic.c:461 dev_watchdog+0x247/0x250
Mar 10 17:31:43 RIP: 0010:dev_watchdog+0x247/0x250
Mar 10 17:31:43 Code: e0 eb 8e 4c 8b 3c 24 c6 05 fa e5 c7 00 01 4c 89 ff e8 5d 3b fc ff 89 d9 4c 89 fe 48 c7 c7 c8 c7 74 9d 48 89 c2 e
Mar 10 17:31:43 RSP: 0018:ffff88c6bf483e80 EFLAGS: 00010282
Mar 10 17:31:43 RAX: 0000000000000000 RBX: 0000000000000005 RCX: 0000000000000000
Mar 10 17:31:43 RDX: ffff88c6bf49fb40 RSI: ffff88c6bf497058 RDI: ffff88c6bf497058
Mar 10 17:31:43 RBP: 0000000000000012 R08: 000000000000120d R09: 0000000000000000
Mar 10 17:31:43 R10: 0000000000000002 R11: 00000000000000f0 R12: ffff88a6c353445c
Mar 10 17:31:43 R13: 0000000000000008 R14: ffff88a6c3534480 R15: ffff88a6c3534000
Mar 10 17:31:43 FS:  0000000000000000(0000) GS:ffff88c6bf480000(0000) knlGS:0000000000000000
Mar 10 17:31:43 CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
Mar 10 17:31:43 CR2: 00007f05f2ff9ff8 CR3: 0000002a17a0a005 CR4: 0000000000760ee0
Mar 10 17:31:43 PKRU: 55555554
Mar 10 17:31:43 Call Trace:
Mar 10 17:31:43 <IRQ>
Mar 10 17:31:43 ? pfifo_fast_enqueue+0x110/0x110
Mar 10 17:31:43 call_timer_fn+0x2b/0x130
Mar 10 17:31:43 run_timer_softirq+0x1c7/0x3e0
Mar 10 17:31:43 ? __hrtimer_init+0xb0/0xb0
Mar 10 17:31:43 ? hrtimer_wakeup+0x1e/0x30
```

BMC 中有:
```txt
CPU 2 machine check error detected.
An OEM diagnostic event occurred.
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
