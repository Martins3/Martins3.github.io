
```txt
[1479461.677434] NMI watchdog: Watchdog detected hard LOCKUP on cpu 27
[1479461.677435] Modules linked in: bonding iscsi_tcp libiscsi_tcp libiscsi scsi_transport_iscsi nvme_tcp(OE) nvme_rdma(OE) nvme_fabrics(OE) nft_fib_inet nft_fib_ipv4 nft_fib_ipv6 nft_fib nft_reject_inet nf_reject_ipv4 nf_reject_ipv6 nft_reject nft_ct nft_chain_nat nf_tables ebtable_nat ebtable_broute ip6table_nat ip6t
able_mangle ip6table_raw ip6table_security iptable_nat nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 libcrc32c iptable_mangle iptable_raw iptable_security rfkill vfio_pci vfio_virqfd vfio_iommu_type1 vfio cuse ip_set nfnetlink ebtable_filter ebtables ip6table_filter ip6_tables iptable_filter ip_tables rdma_ucm(OE)
rdma_cm(OE) iw_cm(OE) ib_ipoib(OE) ib_cm(OE) ib_umad(OE) sunrpc amd64_edac_mod edac_mce_amd kvm_amd ccp kvm irqbypass ipmi_si rapl ipmi_devintf ipmi_msghandler pcspkr ast drm_vram_helper drm_ttm_helper i2c_algo_bit joydev ttm drm_kms_helper i2c_piix4 k10temp syscopyarea sg sysfillrect sysimgblt fb_sys_fops cec drm fuse
 ext4 mbcache jbd2 mlx5_ib(OE)
[1479461.677459]  ib_uverbs(OE) ib_core(OE) sd_mod mlx5_core(OE) mlxfw(OE) tls auxiliary(OE) psample ahci mlxdevm(OE) crct10dif_pclmul crc32_pclmul crc32c_intel ghash_clmulni_intel pci_hyperv_intf megaraid_sas libahci nvme(OE) nvme_core(OE) libata mlx_compat(OE) ngbe t10_pi pinctrl_amd dm_mirror dm_region_hash dm_log d
m_mod
[1479461.677468] CPU: 27 PID: 4050068 Comm: iou-wrk-4042119 Kdump: loaded Tainted: G        W  OE     5.10.0-136.108.0.188.oe2203sp1.x86_64 #1
[1479461.677468] Hardware name: Inspur CS5280H/CS5280H, BIOS 3.3.27 2021-10-16
[1479461.677469] RIP: 0010:__sbq_wake_up+0x6d/0xd0
[1479461.677470] Code: d8 45 31 e4 5b 44 89 e0 5d 41 5c 41 5d c3 cc cc cc cc 8b 55 2c 39 c2 74 03 89 45 2c 48 85 db 74 e0 b8 ff ff ff ff f0 0f c1 03 <83> e8 01 41 bc 01 00 00 00 78 cf 48 8b 43 10 74 15 48 39 c8 5b 5d
[1479461.677471] RSP: 0018:ffffbd80cd204f48 EFLAGS: 00000093
[1479461.677471] RAX: 00000000f10ee11b RBX: ffff9a6b4f8e1d80 RCX: ffff9a6b4f8e1d90
[1479461.677472] RDX: 0000000000000006 RSI: 00000000f10ee162 RDI: ffff9a6b4f8e1c00
[1479461.677472] RBP: ffff9a6acea25220 R08: ffff9aa32ca55200 R09: ffffffffacefb100
[1479461.677473] R10: ffff9aa266ebfd00 R11: 0000000000001000 R12: 000000000000001b
[1479461.677473] R13: ffff9aa28c2d52f8 R14: 00000000ffffffff R15: ffffdd60ab9d2900
[1479461.677474] FS:  00007f77f1dc8b00(0000) GS:ffff9a8a1f780000(0000) knlGS:0000000000000000
[1479461.677475] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[1479461.677475] CR2: 00007f77e851a2c8 CR3: 0000000197cbe000 CR4: 00000000003506e0
[1479461.677475] Call Trace:
[1479461.677476]  <IRQ>
[1479461.677476]  sbitmap_queue_clear+0x3b/0x70
[1479461.677476]  __blk_mq_free_request+0xb3/0x150
[1479461.677477]  flush_smp_call_function_queue+0xce/0x170
[1479461.677477]  __sysvec_call_function_single+0x2c/0xa0
[1479461.677478]  asm_call_irq_on_stack+0xf/0x20
[1479461.677478]  </IRQ>
[1479461.677478]  sysvec_call_function_single+0x72/0x80
[1479461.677479]  asm_sysvec_call_function_single+0x12/0x20
[1479461.677479] RIP: 0010:nvme_rdma_post_send+0x7d/0xb0 [nvme_rdma]
[1479461.677480] Code: 48 89 54 24 20 89 4c 24 28 48 89 44 24 2c 4d 85 c0 74 0b 48 8d 44 24 10 4c 89 c6 49 89 00 48 8b 7b 30 48 8d 54 24 08 48 8b 07 <48> 8b 40 30 ff d0 0f 1f 00 85 c0 0f 85 15 39 00 00 48 8b 4c 24 38
[1479461.677481] RSP: 0018:ffffbd80fb76f9a8 EFLAGS: 00000282
[1479461.677481] RAX: ffff9aa266d28000 RBX: ffff9aa2831f0260 RCX: 0000000000000001
[1479461.677482] RDX: ffffbd80fb76f9b0 RSI: ffff9a6b8487d0f8 RDI: ffff9aa2942ab000
[1479461.677482] RBP: ffff9aa2831f0260 R08: ffff9a6b8487d0f8 R09: ffff9a6acd6c00b8
[1479461.677483] R10: 0000000000000001 R11: 0000000000000040 R12: ffff9aa266d28000
[1479461.677483] R13: ffff9aa27056f000 R14: ffff9aa2834e6a40 R15: ffff9a6b8487cf40
[1479461.677484]  nvme_rdma_queue_rq+0x1e0/0x310 [nvme_rdma]
[1479461.677484]  __blk_mq_issue_directly+0x6c/0x120
[1479461.677485]  blk_mq_try_issue_list_directly+0xc9/0x220
[1479461.677485]  blk_mq_sched_insert_requests+0x9a/0xe0
[1479461.677485]  blk_mq_flush_plug_list+0x100/0x1a0
[1479461.677486]  blk_finish_plug+0x3a/0x60
[1479461.677486]  __blkdev_direct_IO+0x3d4/0x420
[1479461.677486]  ? io_setup_async_msg.part.0+0xb0/0xb0
[1479461.677487]  generic_file_read_iter+0x8f/0x150
[1479461.677487]  blkdev_read_iter+0x44/0x60
[1479461.677487]  io_read+0xd8/0x480
[1479461.677488]  ? newidle_balance+0x234/0x2e0
[1479461.677488]  ? put_prev_entity+0x32/0x610
[1479461.677489]  ? finish_task_switch+0x7c/0x290
[1479461.677489]  ? lock_timer_base+0x61/0x80
[1479461.677490]  io_issue_sqe+0x7f2/0xde0
[1479461.677490]  io_wq_submit_work+0x6a/0xc0
[1479461.677490]  io_worker_handle_work+0x16e/0x2c0
[1479461.677491]  io_wqe_worker+0x1df/0x250
[1479461.677491]  ? finish_task_switch+0x7c/0x290
[1479461.677491]  ? io_worker_handle_work+0x2c0/0x2c0
[1479461.677492]  ret_from_fork+0x1f/0x30
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
