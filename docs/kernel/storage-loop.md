# loop device

测试:
```txt
dd if=/dev/null of=x.ext4 bs=1M seek=100
mkfs.ext4 -F x.ext4
mkdir dir
sudo mount -t ext4 -o loop x.ext4 dir
```

0xffffffff83003bdc in loop_init () at drivers/block/loop.c:2261

- loop_add

```c
static const struct blk_mq_ops loop_mq_ops = {
	.queue_rq       = loop_queue_rq,
	.complete	= lo_complete_rq,
};
```

这个在 mount 过程中，总是出现的:
```txt
#0  loop_queue_rq (hctx=0xffff8880045d0a00, bd=0xffffc90001d5bbd8) at drivers/block/loop.c:1805
#1  0xffffffff81635649 in __blk_mq_issue_directly (last=true, rq=0xffff888005570000, hctx=0xffff8880045d0a00) at block/blk-mq.c:2530
#2  __blk_mq_try_issue_directly (hctx=0xffff8880045d0a00, rq=rq@entry=0xffff888005570000, bypass_insert=bypass_insert@entry=false, last=last@entry=true) at block/blk-mq.c:2583
#3  0xffffffff81635842 in blk_mq_try_issue_directly (hctx=<optimized out>, rq=0xffff888005570000) at block/blk-mq.c:2607
#4  0xffffffff81636eac in blk_mq_submit_bio (bio=<optimized out>) at block/blk-mq.c:2933
#5  0xffffffff81629302 in __submit_bio (bio=<optimized out>) at block/blk-core.c:590
#6  0xffffffff816298e6 in __submit_bio_noacct_mq (bio=<optimized out>) at block/blk-core.c:667
#7  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:684
#8  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:673
#9  0xffffffff814639c1 in __ext4_read_bh (end_io=<optimized out>, op_flags=<optimized out>, bh=0xffff8880128ff478) at fs/ext4/super.c:174
#10 ext4_read_bh (bh=0xffff8880128ff478, op_flags=<optimized out>, end_io=<optimized out>) at fs/ext4/super.c:198
#11 0xffffffff81463ab4 in __ext4_sb_bread_gfp (block=block@entry=1, op_flags=<optimized out>, op_flags@entry=0, gfp=gfp@entry=0, sb=<optimized out>, sb=<optimized out>) at fs/ext4/super.c:235
#12 0xffffffff814691c9 in ext4_sb_bread_unmovable (block=<optimized out>, sb=0xffff88800ef21000) at fs/ext4/super.c:252
#13 ext4_load_super (silent=0, lsb=<synthetic pointer>, sb=0xffff88800ef21000) at fs/ext4/super.c:4948
#14 __ext4_fill_super (sb=0xffff88800ef21000, fc=0xffff888006761900) at fs/ext4/super.c:5056
#15 ext4_fill_super (sb=0xffff88800ef21000, fc=0xffff888006761900) at fs/ext4/super.c:5643
#16 0xffffffff81370238 in get_tree_bdev (fc=0xffff888006761900, fill_super=0xffffffff81468fe0 <ext4_fill_super>) at fs/super.c:1324
#17 0xffffffff8136ee6d in vfs_get_tree (fc=0xffff8880045d0a00, fc@entry=0xffff888006761900) at fs/super.c:1531
#18 0xffffffff81397f33 in do_new_mount (data=0x0 <fixed_percpu_data>, name=0xffff88800d6ebd40 "/dev/loop0", mnt_flags=32, sb_flags=<optimized out>, fstype=0x20 <fixed_percpu_data+32> <error: Cannot access memory at address 0x20>, path=0xffffc90001d5bef8) at fs/namespace.c:3040
#19 path_mount (dev_name=dev_name@entry=0xffff88800d6ebd40 "/dev/loop0", path=path@entry=0xffffc90001d5bef8, type_page=type_page@entry=0xffff8880058b3568 "ext4", flags=<optimized out>, flags@entry=0, data_page=data_page@entry=0x0 <fixed_percpu_data>) at fs/namespace.c:3370
#20 0xffffffff81398b82 in do_mount (data_page=0x0 <fixed_percpu_data>, flags=0, type_page=0xffff8880058b3568 "ext4", dir_name=0x55f287dae4e0 "/root/loop/dir", dev_name=0xffff88800d6ebd40 "/dev/loop0") at fs/namespace.c:3383
#21 __do_sys_mount (data=<optimized out>, flags=0, type=<optimized out>, dir_name=0x55f287dae4e0 "/root/loop/dir", dev_name=<optimized out>) at fs/namespace.c:3591
#22 __se_sys_mount (data=<optimized out>, flags=0, type=<optimized out>, dir_name=94500149716192, dev_name=<optimized out>) at fs/namespace.c:3568
#23 __x64_sys_mount (regs=<optimized out>) at fs/namespace.c:3568
#24 0xffffffff81fae018 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001d5bf58) at arch/x86/entry/common.c:50
#25 do_syscall_64 (regs=0xffffc90001d5bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#26 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## 在运行的时候出现的问题
```txt
#0  loop_queue_rq (hctx=0xffff8880045d0a00, bd=0xffffc900017afd70) at drivers/block/loop.c:1805
#1  0xffffffff81635c1c in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff8880045d0a00, list=list@entry=0xffffc900017afdc0, nr_budgets=nr_budgets@entry=0) at block/blk-mq.c:1992
#2  0xffffffff8163bd03 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff8880045d0a00) at block/blk-mq-sched.c:306
#3  0xffffffff8163bde0 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff8880045d0a00) at block/blk-mq-sched.c:339
#4  0xffffffff81632860 in __blk_mq_run_hw_queue (hctx=0xffff8880045d0a00) at block/blk-mq.c:2110
#5  0xffffffff81632cf0 in __blk_mq_delay_run_hw_queue (hctx=<optimized out>, async=<optimized out>, msecs=msecs@entry=0) at block/blk-mq.c:2186
#6  0xffffffff81632ee9 in blk_mq_run_hw_queue (hctx=<optimized out>, async=async@entry=false) at block/blk-mq.c:2234
#7  0xffffffff81633280 in blk_mq_run_hw_queues (q=q@entry=0xffff888101108000, async=async@entry=false) at block/blk-mq.c:2282
#8  0xffffffff81633dbb in blk_mq_requeue_work (work=0xffff888101108238) at block/blk-mq.c:1449
#9  0xffffffff8112bd54 in process_one_work (worker=worker@entry=0xffff88810072a180, work=0xffff888101108238) at kernel/workqueue.c:2289
#10 0xffffffff8112bf68 in worker_thread (__worker=0xffff88810072a180) at kernel/workqueue.c:2436
#11 0xffffffff81133870 in kthread (_create=0xffff88810072b200) at kernel/kthread.c:376
#12 0xffffffff81001a6f in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

```txt
#0  lo_write_bvec (ppos=0xffffc90000167e28, bvec=0xffffc90000167e30, file=<optimized out>) at drivers/block/loop.c:249
#1  lo_write_simple (rq=0xffff888005571e00, rq=0xffff888005571e00, pos=<optimized out>, lo=0xffff8880045d0200) at drivers/block/loop.c:271
#2  do_req_filebacked (rq=0xffff888005571e00, lo=0xffff8880045d0200) at drivers/block/loop.c:495
#3  loop_handle_cmd (cmd=0xffff888005571f08) at drivers/block/loop.c:1864
#4  loop_process_work (worker=0x0 <fixed_percpu_data>, cmd_list=0xffff8880045d02a8, lo=0xffff8880045d0200) at drivers/block/loop.c:1899
#5  0xffffffff8112bd54 in process_one_work (worker=worker@entry=0xffff88810072a000, work=0xffff8880045d0288) at kernel/workqueue.c:2289
#6  0xffffffff8112bf68 in worker_thread (__worker=0xffff88810072a000) at kernel/workqueue.c:2436
#7  0xffffffff81133870 in kthread (_create=0xffff88810072b000) at kernel/kthread.c:376
#8  0xffffffff81001a6f in ret_from_fork () at arch/x86/entry/entry_64.S:306
```
