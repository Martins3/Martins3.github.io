# virito pmem

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
