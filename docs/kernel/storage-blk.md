## blk_mq_freeze_queue

```txt
#0  blk_mq_freeze_queue (q=q@entry=0xffff888108fb8000) at block/blk-mq.c:177
#1  0xffffffff81775ad3 in blkcg_activate_policy (disk=disk@entry=0xffff888108fb1400, pol=pol@entry=0xffffffff8322cfc0 <ioprio_policy>) at block/blk-cgroup.c:1470
#2  0xffffffff8177c685 in blk_ioprio_init (disk=disk@entry=0xffff888108fb1400) at block/blk-ioprio.c:212
#3  0xffffffff817765c9 in blkcg_init_disk (disk=disk@entry=0xffff888108fb1400) at block/blk-cgroup.c:1387
#4  0xffffffff8176d676 in __alloc_disk_node (q=q@entry=0xffff888108fb8000, node_id=<optimized out>, lkclass=lkclass@entry=0xffffffff83f3c3f4 <part_shift>) at block/genhd.c:1400
#5  0xffffffff8176677c in __blk_mq_alloc_disk (set=set@entry=0xffff888105c7b108, queuedata=queuedata@entry=0xffff888105c7b000, lkclass=lkclass@entry=0xffffffff83f3c3f4 <part_shift>) at block/blk-mq.c:4092
#6  0xffffffff81b8ab9c in loop_add (i=i@entry=0) at drivers/block/loop.c:2007
#7  0xffffffff83a8c6cc in loop_init () at drivers/block/loop.c:2248
#8  0xffffffff81001a55 in do_one_initcall (fn=0xffffffff83a8c610 <loop_init>) at init/main.c:1246
#9  0xffffffff83a26700 in do_initcall_level (command_line=0xffff888103b45100 "root", level=6) at init/main.c:1319
#10 do_initcalls () at init/main.c:1335
#11 do_basic_setup () at init/main.c:1354
#12 kernel_init_freeable () at init/main.c:1571
#13 0xffffffff822b8d4a in kernel_init (unused=<optimized out>) at init/main.c:1462
#14 0xffffffff81002939 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

- blk_mq_freeze_queue : io 会堵在什么地方?

```txt
@[
    blk_mq_dispatch_rq_list+5
    __blk_mq_sched_dispatch_requests+301
    blk_mq_sched_dispatch_requests+57
    __blk_mq_run_hw_queue+115
    process_one_work+482
    worker_thread+84
    kthread+218
    ret_from_fork+41
]: 57
@[
    blk_mq_dispatch_rq_list+5
    __blk_mq_sched_dispatch_requests+171
    blk_mq_sched_dispatch_requests+57
    __blk_mq_run_hw_queue+115
    blk_mq_run_hw_queues+105
    blk_mq_requeue_work+340
    process_one_work+482
    worker_thread+84
    kthread+218
    ret_from_fork+41
]: 92
```


```txt
#0  nvme_alloc_admin_tag_set (ctrl=ctrl@entry=0xffff8881062d01f0, set=set@entry=0xffff8881062d00b0, ops=ops@entry=0xffffffff82717fe0 <nvme_mq_admin_ops>, cmd_size=cmd_size@entry=184) at drivers/nvme/host/core.c:4906
#1  0xffffffff81c1a752 in nvme_probe (pdev=0xffff888104089000, id=<optimized out>) at drivers/nvme/host/pci.c:3004
#2  0xffffffff81820ff4 in local_pci_probe (_ddi=_ddi@entry=0xffffc900011efdc0) at drivers/pci/pci-driver.c:324
#3  0xffffffff81822ac3 in pci_call_probe (id=<optimized out>, dev=0xffff888104089000, drv=<optimized out>) at drivers/pci/pci-driver.c:392
```

- nvme_alloc_admin_tag_set
  - blk_mq_init_queue
    - blk_mq_init_queue_data
      - blk_mq_init_allocated_queue 来分配的
        - INIT_WORK(&q->timeout_work, blk_mq_timeout_work);
        - INIT_DELAYED_WORK(&q->requeue_work, blk_mq_requeue_work);

## 分析下，到底是谁在启动这些 queue

blk_mq_kick_requeue_list 总是如下的路径:
```txt
#0  blk_mq_kick_requeue_list (q=0xffff888107c29bc0) at block/blk-mq.c:1487
#1  0xffffffff81756790 in blk_kick_flush (flags=<optimized out>, fq=0xffff8881093ac4e0, q=<optimized out>) at block/blk-flush.c:350
#2  0xffffffff81756d3f in blk_insert_flush (rq=rq@entry=0xffff888109786900) at block/blk-flush.c:448
#3  0xffffffff817638ab in blk_mq_submit_bio (bio=<optimized out>) at block/blk-mq.c:2974
#4  0xffffffff81753d7e in __submit_bio_noacct_mq (bio=0xffff88811240ae00) at block/blk-core.c:673
#5  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:702
#6  0xffffffff81753fc7 in submit_bio_noacct (bio=<optimized out>) at block/blk-core.c:801
#7  0xffffffff8147d11e in submit_bh_wbc (opf=<optimized out>, bh=bh@entry=0xffff88811d96d5b0, wbc=wbc@entry=0x0 <fixed_percpu_data>) at fs/buffer.c:2777
#8  0xffffffff8147d150 in submit_bh (opf=<optimized out>, bh=bh@entry=0xffff88811d96d5b0) at fs/buffer.c:2782
#9  0xffffffff8156d3de in journal_submit_commit_record (journal=journal@entry=0xffff88810612a000, commit_transaction=commit_transaction@entry=0xffff88812f42a100, cbh=cbh@entry
=0xffffc9000178fdc0, crc32_sum=crc32_sum@entry=4294967295) at fs/jbd2/commit.c:156
```

## blk_mq_quiesce_queue

## 太复杂了，看看基本流程吧
使用这个作为基本流程:
```c
static const struct blk_mq_ops virtio_mq_ops = {
	.queue_rq	= virtio_queue_rq,
	.queue_rqs	= virtio_queue_rqs,
	.commit_rqs	= virtio_commit_rqs,
	.complete	= virtblk_request_done,
	.map_queues	= virtblk_map_queues,
	.poll		= virtblk_poll,
};
```

```txt
#0  virtio_queue_rq (hctx=0xffff888103b92000, bd=0xffffc9000151bd00) at drivers/block/virtio_blk.c:461
#1  0xffffffff817626c1 in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff888103b92000, list=list@entry=0xffffc9000151bd78, nr_budgets=nr_budgets@entry=0) at block/blk-mq.c:2035
#2  0xffffffff81768ad7 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff888103b92000) at block/blk-mq-sched.c:301
#3  0xffffffff81769089 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff888103b92000) at block/blk-mq-sched.c:333
#4  0xffffffff8175f5a4 in blk_mq_run_hw_queue (hctx=0xffff888103b92000, async=async@entry=false) at block/blk-mq.c:2249
#5  0xffffffff8175f6a9 in blk_mq_run_hw_queues (q=q@entry=0xffff888107c29bc0, async=async@entry=false) at block/blk-mq.c:2298
#6  0xffffffff81761531 in blk_mq_requeue_work (work=0xffff888107c29dd8) at block/blk-mq.c:1462
#7  0xffffffff81165469 in process_one_work (worker=worker@entry=0xffff8881061b20c0, work=0xffff888107c29dd8) at kernel/workqueue.c:2405
#8  0xffffffff81165a71 in worker_thread (__worker=0xffff8881061b20c0) at kernel/workqueue.c:2552
#9  0xffffffff8116ea42 in kthread (_create=0xffff888106176000) at kernel/kthread.c:379
#10 0xffffffff81002939 in ret_from_fork () at arch/x86/entry/entry_64.S:308
#11 0x0000000000000000 in ?? ()
```

似乎这才是常用路径，一次性提交大量的 request:
```txt
#0  virtio_queue_rqs (rqlist=0xffffc9000160fcc0) at drivers/block/virtio_blk.c:536
#1  0xffffffff8176312f in __blk_mq_flush_plug_list (q=<optimized out>, q=<optimized out>, plug=0xffffc9000160fcc0) at block/blk-mq.c:2704
#2  __blk_mq_flush_plug_list (plug=0xffffc9000160fcc0, q=0xffff888107c29bc0) at block/blk-mq.c:2699
#3  blk_mq_flush_plug_list (plug=plug@entry=0xffffc9000160fcc0, from_schedule=from_schedule@entry=false) at block/blk-mq.c:2770
#4  0xffffffff817547ea in __blk_flush_plug (plug=0xffffc9000160fcc0, plug@entry=0xffffc9000160fc08, from_schedule=from_schedule@entry=false) at block/blk-core.c:1144
#5  0xffffffff81754a29 in blk_finish_plug (plug=0xffffc9000160fc08) at block/blk-core.c:1168
#6  blk_finish_plug (plug=plug@entry=0xffffc9000160fcc0) at block/blk-core.c:1165
#7  0xffffffff81511660 in ext4_do_writepages (mpd=mpd@entry=0xffffc9000160fd28) at fs/ext4/inode.c:2754
#8  0xffffffff81511b1e in ext4_writepages (mapping=<optimized out>, wbc=<optimized out>) at fs/ext4/inode.c:2792
#9  0xffffffff813451f0 in do_writepages (mapping=mapping@entry=0xffff888110dbb040, wbc=wbc@entry=0xffffc9000160fe40) at mm/page-writeback.c:2551
#10 0xffffffff8133608a in filemap_fdatawrite_wbc (wbc=0xffffc9000160fe40, mapping=0xffff888110dbb040) at mm/filemap.c:390
#11 filemap_fdatawrite_wbc (mapping=0xffff888110dbb040, wbc=0xffffc9000160fe40) at mm/filemap.c:380
#12 0xffffffff81339dc8 in __filemap_fdatawrite_range (mapping=mapping@entry=0xffff888110dbb040, start=start@entry=0, end=end@entry=9223372036854775807, sync_mode=sync_mode@ent ry=1) at mm/filemap.c:423
#13 0xffffffff8133a59c in file_write_and_wait_range (file=file@entry=0xffff888107d83600, lstart=lstart@entry=0, lend=lend@entry=9223372036854775807) at mm/filemap.c:781
#14 0xffffffff814ffc40 in ext4_sync_file (file=0xffff888107d83600, start=0, end=9223372036854775807, datasync=0) at fs/ext4/fsync.c:151
#15 0xffffffff81474357 in vfs_fsync (datasync=0, file=0xffff888107d83600) at fs/sync.c:202
```
