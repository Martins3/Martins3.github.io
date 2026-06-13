# Virtio-pmem
这个东西应该已经死掉了吧

## 读读文档
## [QEMU: virtio pmem](https://qemu.readthedocs.io/en/latest/system/devices/virtio-pmem.html)

- Virtio pmem allows to bypass the guest page cache and directly use host page cache. This reduces guest memory footprint as the host can make efficient memory reclaim decisions under memory pressure.

我是真的没有想到，居然还有这种取舍的操作。

- https://zhuanlan.zhihu.com/p/68858852

## 到底什么是 zone device
- Device memory (pmem, HMM, etc...) hotplug support

- [ ] kvm_is_zone_device_pfn

## 尝试使用一下
https://www.snia.org/sites/default/files/PM-Summit/2017/presentations/Coughlan_Tom_PM_in_Linux.pdf

- mkfs.ext4 /dev/pmem0
- mkdir pmem
- sudo mount /dev/pmem0 pmem

## [ ] 借此分析一下: DAX

- `O_DIRECT` 和 DAX 的差别
- 是不是，直接读写 blockdev 是绕过了 fs，但是没有绕过 page cache?


```txt
mkfs -t xfs -d su=1g,sw=1 /dev/pmem0
mount -t xfs -o dax /dev/pmem0 /mnt
```
看来这也是无法支持的，实在是失望至极。

## 简单的代码分析

## async_pmem_flush

1. mount
```txt
#0  async_pmem_flush (nd_region=0xffff888100d55800, bio=0xffffc900010cbc08) at drivers/nvdimm/nd_virtio.c:107
#1  0xffffffff8197e8d1 in nvdimm_flush (nd_region=nd_region@entry=0xffff888100d55800, bio=bio@entry=0xffffc900010cbc08) at drivers/nvdimm/region_devs.c:1092
#2  0xffffffff81985fe6 in pmem_submit_bio (bio=0xffffc900010cbc08) at drivers/nvdimm/pmem.c:212
#3  0xffffffff81606164 in __submit_bio (bio=0xffffc900010cbc08) at block/blk-core.c:597
#4  0xffffffff81606610 in __submit_bio_noacct (bio=<optimized out>) at block/blk-core.c:640
#5  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:691
#6  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:678
#7  0xffffffff8160002e in submit_bio_wait (bio=bio@entry=0xffffc900010cbc08) at block/bio.c:1328
#8  0xffffffff81608ae1 in blkdev_issue_flush (bdev=<optimized out>) at block/blk-flush.c:464
#9  0xffffffff8145bfee in jbd2_journal_recover (journal=journal@entry=0xffff888105ccc800) at fs/jbd2/recovery.c:328
#10 0xffffffff8146322c in jbd2_journal_load (journal=0xffff888105ccc800) at fs/jbd2/journal.c:2084
#11 jbd2_journal_load (journal=journal@entry=0xffff888105ccc800) at fs/jbd2/journal.c:2051
#12 0xffffffff81449f3e in ext4_load_journal (journal_devnum=0, es=<optimized out>, sb=0xffff888105ccd000) at fs/ext4/super.c:5805
#13 __ext4_fill_super (sb=0xffff888105ccd000, fc=0xffff8881417a8b40) at fs/ext4/super.c:5075
#14 ext4_fill_super (sb=0xffff888105ccd000, fc=0xffff8881417a8b40) at fs/ext4/super.c:5517
#15 0xffffffff8134e07b in get_tree_bdev (fc=0xffff8881417a8b40, fill_super=0xffffffff81446a70 <ext4_fill_super>) at fs/super.c:1323
#16 0xffffffff8134ccb0 in vfs_get_tree (fc=fc@entry=0xffff8881417a8b40) at fs/super.c:1530
#17 0xffffffff81376123 in do_new_mount (data=0x0 <fixed_percpu_data>, name=0xffff888108d32830 "/dev/pmem0", mnt_flags=32, sb_flags=<optimized out>, fstype=0x20 <fixed_percpu_data+32> <error: Cannot access memory at address 0x20>, path=0xffffc900010cbef8) at fs/namespace.c:3040
#18 path_mount (dev_name=dev_name@entry=0xffff888108d32830 "/dev/pmem0", path=path@entry=0xffffc900010cbef8, type_page=type_page@entry=0xffff888106cdcae0 "ext4", flags=<optimized out>, flags@entry=3236757504, data_page=data_page@entry=0x0 <fixed_percpu_data>) at fs/namespace.c:3370
#19 0xffffffff813769c2 in do_mount (data_page=0x0 <fixed_percpu_data>, flags=3236757504, type_page=0xffff888106cdcae0 "ext4", dir_name=0x5583c1bec4c0 "/home/martins3/pmem", dev_name=0xffff888108d32830 "/dev/pmem0") at fs/namespace.c:3383
#20 __do_sys_mount (data=<optimized out>, flags=3236757504, type=<optimized out>, dir_name=0x5583c1bec4c0 "/home/martins3/pmem", dev_name=<optimized out>) at fs/namespace.c:3591
#21 __se_sys_mount (data=<optimized out>, flags=3236757504, type=<optimized out>, dir_name=94024379581632, dev_name=<optimized out>) at fs/namespace.c:3568
#22 __x64_sys_mount (regs=<optimized out>) at fs/namespace.c:3568
#23 0xffffffff81ef7d1b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900010cbf58) at arch/x86/entry/common.c:50
#24 do_syscall_64 (regs=0xffffc900010cbf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#25 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

2. journal
```txt
#0  async_pmem_flush (nd_region=0xffff888100d55800, bio=0xffff8881417c9300) at drivers/nvdimm/nd_virtio.c:108
#1  async_pmem_flush (nd_region=0xffff888100d55800, bio=0xffff8881417c9300) at drivers/nvdimm/nd_virtio.c:101
#2  0xffffffff8197e8d1 in nvdimm_flush (nd_region=nd_region@entry=0xffff888100d55800, bio=bio@entry=0xffff8881417c9300) at drivers/nvdimm/region_devs.c:1092
#3  0xffffffff81985fe6 in pmem_submit_bio (bio=0xffff8881417c9300) at drivers/nvdimm/pmem.c:212
#4  0xffffffff81606164 in __submit_bio (bio=0xffff8881417c9300) at block/blk-core.c:597
#5  0xffffffff81606610 in __submit_bio_noacct (bio=<optimized out>) at block/blk-core.c:640
#6  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:691
#7  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:678
#8  0xffffffff81390605 in submit_bh_wbc (opf=<optimized out>, opf@entry=395265, bh=bh@entry=0xffff8881408ba270, wbc=wbc@entry=0x0 <fixed_percpu_data>) at fs/buffer.c:2719
#9  0xffffffff81390637 in submit_bh (opf=opf@entry=395265, bh=bh@entry=0xffff8881408ba270) at fs/buffer.c:2725
#10 0xffffffff814594c5 in journal_submit_commit_record (journal=journal@entry=0xffff888105ccc800, commit_transaction=commit_transaction@entry=0xffff888145cb9000, cbh=cbh@entry=0xffffc90001093db8, crc32_sum=crc32_sum@entry=4294967295) at fs/jbd2/commit.c:158
#11 0xffffffff8145aa69 in journal_submit_commit_record (crc32_sum=<optimized out>, cbh=<optimized out>, commit_transaction=0xffff888145cb9000, journal=0xffff888105ccc800) at include/linux/jbd2.h:1687
#12 jbd2_journal_commit_transaction (journal=journal@entry=0xffff888105ccc800) at fs/jbd2/commit.c:916
#13 0xffffffff81460c4a in kjournald2 (arg=0xffff888105ccc800) at fs/jbd2/journal.c:210
#14 0xffffffff81129c73 in kthread (_create=0xffff88810eda25c0) at kernel/kthread.c:376
#15 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

- 接受 host 的中断:
```txt
#0  virtio_pmem_host_ack (vq=0xffff8881419f6000) at drivers/nvdimm/nd_virtio.c:14
#1  0xffffffff81722f85 in vring_interrupt (irq=11, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2462
#2  vring_interrupt (irq=irq@entry=11, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2437
#3  0xffffffff817268ef in vp_vring_interrupt (irq=11, opaque=0xffff8880053ed800) at drivers/virtio/virtio_pci_common.c:68
#4  0xffffffff811658d1 in __handle_irq_event_percpu (desc=desc@entry=0xffff888003920e00) at kernel/irq/handle.c:158
#5  0xffffffff81165a7f in handle_irq_event_percpu (desc=0xffff888003920e00) at kernel/irq/handle.c:193
#6  handle_irq_event (desc=desc@entry=0xffff888003920e00) at kernel/irq/handle.c:210
#7  0xffffffff81169c1b in handle_fasteoi_irq (desc=0xffff888003920e00) at kernel/irq/chip.c:714
#8  0xffffffff810b9ad4 in generic_handle_irq_desc (desc=0xffff888003920e00) at include/linux/irqdesc.h:158
#9  handle_irq (regs=<optimized out>, desc=0xffff888003920e00) at arch/x86/kernel/irq.c:231
#10 __common_interrupt (regs=<optimized out>, vector=34) at arch/x86/kernel/irq.c:250
#11 0xffffffff81ef9533 in common_interrupt (regs=0xffffc900000bbe38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc90000101018
```

## 写操作可以轻松的触发 write_pmem

测试命令:
```sh
dd if=/dev/zero of=pmem count=1000 bs=1M
```

```txt
#0  write_pmem (len=4096, off=0, page=0xffffea00040a1140, pmem_addr=0xffffc90089000000) at drivers/nvdimm/pmem.c:131
#1  pmem_do_write (pmem=pmem@entry=0xffff8880080fe228, page=0xffffea00040a1140, page_off=0, sector=sector@entry=2392064, len=len@entry=4096) at drivers/nvdimm/pmem.c:195
#2  0xffffffff81985ea0 in pmem_submit_bio (bio=0xffff8881007ebe00) at drivers/nvdimm/pmem.c:219
#3  0xffffffff81606164 in __submit_bio (bio=0xffff8881007ebe00) at block/blk-core.c:597
#4  0xffffffff81606610 in __submit_bio_noacct (bio=<optimized out>) at block/blk-core.c:640
#5  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:691
#6  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:678
#7  0xffffffff8142b3cb in ext4_io_submit (io=0xffffc90000133ab0) at fs/ext4/page-io.c:378
#8  io_submit_add_bh (inode=<optimized out>, bh=0xffff88816cc965b0, page=0xffffea000416a400, io=0xffffc90000133ab0) at fs/ext4/page-io.c:420
#9  ext4_bio_write_page (io=io@entry=0xffffc90000133ab0, page=page@entry=0xffffea000416a400, len=<optimized out>, keep_towrite=keep_towrite@entry=false) at fs/ext4/page-io.c:546
#10 0xffffffff814070b9 in mpage_submit_page (mpd=mpd@entry=0xffffc90000133a70, page=page@entry=0xffffea000416a400) at fs/ext4/inode.c:2114
#11 0xffffffff8140d402 in mpage_map_and_submit_buffers (mpd=0xffffc90000133a70) at fs/ext4/inode.c:2359
#12 mpage_map_and_submit_extent (give_up_on_write=<synthetic pointer>, mpd=0xffffc90000133a70, handle=0xffff888104195578) at fs/ext4/inode.c:2498
#13 ext4_writepages (mapping=0xffff888148771068, wbc=<optimized out>) at fs/ext4/inode.c:2827
#14 0xffffffff81289a8a in do_writepages (mapping=mapping@entry=0xffff888148771068, wbc=wbc@entry=0xffffc90000133ca0) at mm/page-writeback.c:2468
#15 0xffffffff813831fc in __writeback_single_inode (inode=inode@entry=0xffff888148770ef0, wbc=wbc@entry=0xffffc90000133ca0) at fs/fs-writeback.c:1587
#16 0xffffffff81383b64 in writeback_sb_inodes (sb=sb@entry=0xffff88814dbfb000, wb=wb@entry=0xffff8881006c1060, work=work@entry=0xffffc90000133e30) at fs/fs-writeback.c:1865
#17 0xffffffff81383e47 in __writeback_inodes_wb (wb=wb@entry=0xffff8881006c1060, work=work@entry=0xffffc90000133e30) at fs/fs-writeback.c:1936
#18 0xffffffff813840b2 in wb_writeback (wb=wb@entry=0xffff8881006c1060, work=work@entry=0xffffc90000133e30) at fs/fs-writeback.c:2041
#19 0xffffffff81385315 in wb_check_background_flush (wb=0xffff8881006c1060) at fs/fs-writeback.c:2107
#20 wb_do_writeback (wb=0xffff8881006c1060) at fs/fs-writeback.c:2195
#21 wb_workfn (work=0xffff8881006c11e8) at fs/fs-writeback.c:2222
#22 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff88810062e000, work=0xffff8881006c11e8) at kernel/workqueue.c:2289
#23 0xffffffff811232c8 in worker_thread (__worker=0xffff88810062e000) at kernel/workqueue.c:2436
#24 0xffffffff81129c73 in kthread (_create=0xffff88810062f000) at kernel/kthread.c:376
#25 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#26 0x0000000000000000 in ?? ()
```

简单看一下  write_pmem ，就是简单的拷贝而已

## 为什么总是出现这个警告

```txt
[   20.304600] ------------[ cut here ]------------
[   20.304799] WARNING: CPU: 0 PID: 1038 at block/blk-core.c:750 submit_bio_noacct+0x24d/0x570
[   20.305084] Modules linked in:
[   20.305189] CPU: 0 PID: 1038 Comm: mkfs.xfs Not tainted 6.5.0-rc6-00002-g17475aef267e #492
[   20.305468] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[   20.305687] RIP: 0010:submit_bio_noacct+0x24d/0x570
[   20.305848] Code: 9e fe ff ff 49 8b 44 24 18 8b 88 d4 01 00 00 85 c9 0f 85 8b fe ff ff e9 f9 fe ff ff 40 0f b6 c5 83 f8 01 74 30 83 f8 0d 74 2b <0f> 0b b8 0a 00 00 00 e9 e4
 fe ff ff 48 89 df e8 ff 74 02 00 84 c0
[   20.306454] RSP: 0018:ffffc900012ffcb0 EFLAGS: 00010297
[   20.306627] RAX: 0000000000000000 RBX: ffff8880071e0b00 RCX: ffff8880071e0b00
[   20.306857] RDX: 0000000000000000 RSI: 0000000000000000 RDI: ffff8880071e0b00
[   20.307094] RBP: 0000000000040000 R08: ffffc900012ffbe0 R09: 0000000000000000
[   20.307323] R10: ffffc900012ffe60 R11: 0000000000000001 R12: ffff88800448d900
[   20.307550] R13: ffff8880058ecaf8 R14: ffff88800448d900 R15: 0000000000000000
[   20.307778] FS:  00007fd8033ba980(0000) GS:ffff88803d600000(0000) knlGS:0000000000000000
[   20.308036] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[   20.308220] CR2: 000055a859ab8000 CR3: 000000000013e000 CR4: 0000000000750ef0
[   20.308450] PKRU: 55555554
[   20.308541] Call Trace:
[   20.308627]  <TASK>
[   20.308698]  ? submit_bio_noacct+0x24d/0x570
[   20.308838]  ? __warn+0x81/0x130
[   20.308949]  ? submit_bio_noacct+0x24d/0x570
[   20.309090]  ? report_bug+0x171/0x1a0
[   20.309215]  ? handle_bug+0x3c/0x70
[   20.309336]  ? exc_invalid_op+0x17/0x70
[   20.309462]  ? asm_exc_invalid_op+0x1a/0x20
[   20.309603]  ? submit_bio_noacct+0x24d/0x570
[   20.309743]  async_pmem_flush+0x76/0x90
[   20.309875]  nvdimm_flush+0x17/0x40
[   20.309995]  pmem_submit_bio+0x42b/0x460
[   20.310124]  ? __pfx_pmem_submit_bio+0x10/0x10
[   20.310272]  __submit_bio+0x84/0x110
[   20.310390]  submit_bio_noacct_nocheck+0x159/0x370
[   20.310546]  submit_bio_wait+0x5a/0xb0
[   20.310670]  blkdev_issue_flush+0x4d/0x80
[   20.310803]  ? __pfx_submit_bio_wait_endio+0x10/0x10
[   20.310969]  blkdev_fsync+0x46/0x60
[   20.311088]  __x64_sys_fsync+0x3b/0x70
[   20.311216]  do_syscall_64+0x3b/0x90
[   20.311337]  entry_SYSCALL_64_after_hwframe+0x73/0xdd
[   20.311501] RIP: 0033:0x7fd8034c92b7
[   20.311619] Code: 64 89 01 48 83 c8 ff c3 66 2e 0f 1f 84 00 00 00 00 00 90 f3 0f 1e fa 64 8b 04 25 18 00 00 00 85 c0 75 10 b8 4a 00 00 00 0f 05 <48> 3d 00 f0 ff ff 77 41 c3
 48 83 ec 18 89 7c 24 0c e8 f3 42 f8 ff
[   20.312209] RSP: 002b:00007ffe8196d4c8 EFLAGS: 00000246 ORIG_RAX: 000000000000004a
[   20.312456] RAX: ffffffffffffffda RBX: 000055a859a8bdc0 RCX: 00007fd8034c92b7
[   20.312705] RDX: 000055a85978f2a0 RSI: 0000000000000103 RDI: 0000000000000004
[   20.312953] RBP: 0000000000000004 R08: 00007fd8035b75a0 R09: 0000000000000000
[   20.313202] R10: 00007ffe8196d440 R11: 0000000000000246 R12: 000055a859a8f370
[   20.313434] R13: 00007ffe8196d690 R14: 0000000000000002 R15: 000055a859a8bdc0
[   20.313666]  </TASK>
[   20.313741] ---[ end trace 0000000000000000 ]---
```

大致的执行流程为:

```txt
#0  async_pmem_flush (nd_region=0xffff888006c99c00, bio=0xffffc900013a7e60)
    at drivers/nvdimm/nd_virtio.c:102
#1  0xffffffff81a03187 in nvdimm_flush (nd_region=nd_region@entry=0xffff888006c99c00,
    bio=bio@entry=0xffffc900013a7e60) at drivers/nvdimm/region_devs.c:1136
#2  0xffffffff81a0d2fb in pmem_submit_bio (bio=0xffffc900013a7e60) at drivers/nvdimm/pmem.c:212
#3  0xffffffff817b5234 in __submit_bio (bio=0xffffc900013a7e60) at block/blk-core.c:602
#4  __submit_bio (bio=0xffffc900013a7e60) at block/blk-core.c:592
#5  0xffffffff817b56d9 in __submit_bio_noacct (bio=0xffffc900013a7e60) at block/blk-core.c:645
#6  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:708
#7  0xffffffff817b5a28 in submit_bio_noacct (bio=<optimized out>) at block/blk-core.c:806
#8  0xffffffff817adc7a in submit_bio_wait (bio=bio@entry=0xffffc900013a7e60) at block/bio.c:1381
#9  0xffffffff817b816d in blkdev_issue_flush (bdev=bdev@entry=0xffff88800448d900) at block/blk-flush.c:476
#10 0xffffffff817ac436 in blkdev_fsync (filp=<optimized out>, start=0, end=9223372036854775807, datasync=<optimized out>) at block/fops.c:467
#11 0xffffffff8148a79b in vfs_fsync (datasync=0, file=0xffff8880088ed600) at fs/sync.c:202
#12 do_fsync (datasync=0, fd=<optimized out>) at fs/sync.c:212
#13 __do_sys_fsync (fd=<optimized out>) at fs/sync.c:220
#14 __se_sys_fsync (fd=<optimized out>) at fs/sync.c:218
#15 __x64_sys_fsync (regs=<optimized out>) at fs/sync.c:218
#16 0xffffffff82195d9b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900013a7f58)
    at arch/x86/entry/common.c:50
#17 do_syscall_64 (regs=0xffffc900013a7f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#18 0xffffffff822000ef in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:12
```

看来也是最近的 fix 而已
```txt
🧀  kernel_version b4a6bb3a67aa0c37b2b6cd47efc326eb455de674
v6.3-rc1~145^2~9
```

很快啊，就在这里被 fix 掉了: https://lore.kernel.org/all/20230713135413.2946622-1-houtao@huaweicloud.com/

### 原因分析
blkdev_issue_flush 创建第一个 bio，在 async_pmem_flush 创建 child bio 的 flag 会有所不同。

```txt
$ p /x bio->bi_opf
$4 = 0x40801
```

## pmdk
https://github.com/pmem/pmdk

## pmem
DAX 设置 : 到时候在分析吧!
1. https://www.intel.co.uk/content/www/uk/en/it-management/cloud-analytic-hub/pmem-next-generation-storage.html
2. https://nvdimm.wiki.kernel.org/
3. https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/storage_administration_guide/ch-persistent-memory-nvdimms

目前观察到的 generic_file_read_iter 和 file_operations::mmap 的内容对于 DAX 区分对待的，但是内容远远不该如此，不仅仅可以越过 page cache 机制，而且 page reclaim 全部可以跳过。


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
