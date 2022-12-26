## TODO
1. 希望搞清楚 bio request 和 request_queue 的关系

## submit_bio
1. 这是 fs 和 dev 进行 io 的唯一的接口吗 ?
2. submit_bio 和 generic_make_request 的关系是什么 ? 两者都是类似接口，但是 submit_bio 做出部分检查，最后调用 generic_make_request

iomap 直接来调用 submit_bio，有趣

## bio

### bio::bi_end_io

submit_bio 是异步的，但是可以修改为同步的，通过设置 bio::bi_end_io 来提醒结束，否则始终等待在
上面。
```txt
#0  io_schedule_timeout (timeout=timeout@entry=9223372036854775807) at kernel/sched/core.c:8794
#1  0xffffffff821906a3 in do_wait_for_common (state=2, timeout=9223372036854775807, action=0xffffffff8218ea70 <io_schedule_timeout>, x=0xffffc9003f94fe20) at kernel/sched/build_utility.c:3653
#2  __wait_for_common (x=x@entry=0xffffc9003f94fe20, action=0xffffffff8218ea70 <io_schedule_timeout>, timeout=timeout@entry=9223372036854775807, state=state@entry=2) at kernel/sched/build_utility.c:3674
#3  0xffffffff8219099f in wait_for_common_io (state=2, timeout=9223372036854775807, x=0xffffc9003f94fe20) at kernel/sched/build_utility.c:3691
#4  0xffffffff816ba46a in submit_bio_wait (bio=bio@entry=0xffffc9003f94fe58) at block/bio.c:1388
#5  0xffffffff816c408b in blkdev_issue_flush (bdev=<optimized out>) at block/blk-flush.c:467
#6  0xffffffff8148bf39 in ext4_sync_file (file=0xffff888145091e00, start=<optimized out>, end=<optimized out>, datasync=<optimized out>) at fs/ext4/fsync.c:177
#7  0xffffffff8140af15 in vfs_fsync_range (datasync=1, end=9223372036854775807, start=0, file=0xffff888145091e00) at fs/sync.c:188
#8  vfs_fsync (datasync=1, file=0xffff888145091e00) at fs/sync.c:202
#9  do_fsync (datasync=1, fd=<optimized out>) at fs/sync.c:212
#10 __do_sys_fdatasync (fd=<optimized out>) at fs/sync.c:225
#11 __se_sys_fdatasync (fd=<optimized out>) at fs/sync.c:223
#12 __x64_sys_fdatasync (regs=<optimized out>) at fs/sync.c:223
#13 0xffffffff82180c3f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9003f94ff58) at arch/x86/entry/common.c:50
#14 do_syscall_64 (regs=0xffffc9003f94ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#15 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

通知 bio 结束的执行流程:
```txt
#0  bio_endio (bio=bio@entry=0xffff888104430100) at block/bio.c:1577
#1  0xffffffff816cd00c in req_bio_endio (error=0 '\000', nbytes=4096, bio=0xffff888104430100, rq=0xffff8881042d1680) at block/blk-mq.c:794
#2  blk_update_request (req=req@entry=0xffff8881042d1680, error=error@entry=0 '\000', nr_bytes=4096) at block/blk-mq.c:926
#3  0xffffffff816cd349 in blk_mq_end_request (rq=0xffff8881042d1680, error=0 '\000') at block/blk-mq.c:1053
#4  0xffffffff81aa9e99 in virtblk_done (vq=0xffff8881008a7000) at drivers/block/virtio_blk.c:291
#5  0xffffffff818134f9 in vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2470
#6  vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2445
#7  0xffffffff811a3d35 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100cb7a00) at kernel/irq/handle.c:158
#8  0xffffffff811a3f13 in handle_irq_event_percpu (desc=0xffff888100cb7a00) at kernel/irq/handle.c:193
#9  handle_irq_event (desc=desc@entry=0xffff888100cb7a00) at kernel/irq/handle.c:210
#10 0xffffffff811a8bee in handle_edge_irq (desc=0xffff888100cb7a00) at kernel/irq/chip.c:819
#11 0xffffffff810ce1d8 in generic_handle_irq_desc (desc=0xffff888100cb7a00) at ./include/linux/irqdesc.h:158
#12 handle_irq (regs=<optimized out>, desc=0xffff888100cb7a00) at arch/x86/kernel/irq.c:231
#13 __common_interrupt (regs=<optimized out>, vector=35) at arch/x86/kernel/irq.c:250
#14 0xffffffff8218a897 in common_interrupt (regs=0xffffc900000a3e38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```

几乎每一个和 bio 打交道的都会对应的注册 bio::bi_end_io


## 关键参考
- [IO 子系统全流程介绍](https://zhuanlan.zhihu.com/p/545906763)
- [ ] 还有两个 lwn 的文章
