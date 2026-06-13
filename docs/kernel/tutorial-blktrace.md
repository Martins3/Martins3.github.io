# blktrace 基本使用

多年之前，有人一脸震惊的对我说:"你做存储，连 blktrace 都不知道!"
从此，blktrace 成为我心中的梦魇。

## 如何理解 raid1_log

```c
#define raid1_log(md, fmt, args...)				\
	do { if ((md)->queue) blk_add_trace_msg((md)->queue, "raid1 " fmt, ##args); } while (0)
```


最后调用到 blktrace.c:`__blk_trace_note_message` 中

当执行:
```txt
 mdadm --grow --force --raid-devices=1000 /dev/md10 || true
```

可以得到:
```txt
➜  ~ blktrace -d /dev/md10 -o - | blkparse -i -
  9,10  10        0     0.000000000  1614 1,0  m   N md md_update_sb
  9,10  10        0     0.003608231  1614 1,0  m   N raid1 wait freeze
  9,10  10        0     0.003627965  1614 1,0  m   N md md_update_sb
  9,10  28        0     0.009725873  1546 1,0  m   N md md_update_sb
```


## 源码
https://git.kernel.org/pub/scm/linux/kernel/git/axboe/blktrace.git/about/

一个源码中包含了超级多东西。

## 综合文摘看看
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/performance_tuning_guide/ch06s03

## 读读文档
### [btt](https://man7.org/linux/man-pages/man1/btt.1.html)
- blktrace 的后期处理工具

### [blkparse](https://linux.die.net/man/1/blkparse)

### [btrecord](https://linux.die.net/man/8/btrecord) && [btreplay](https://linux.die.net/man/8/btreplay)

### [iowatcher](https://man7.org/linux/man-pages/man1/iowatcher.1.html)

### [bno_plot](https://man7.org/linux/man-pages/man1/bno_plot.1.html)

## [ ] 长久无法理解的问题，ftrace 和 blktrace 的关系是什么?


## blktrace
- https://developer.aliyun.com/article/698568

- call_bio_endio 中最后会调用到 `bio_end_io_acct`，是给 blktrace 来处理的吗?

## CONFIG_BLK_DEV_IO_TRACE
`config BLK_DEV_IO_TRACE` 已经告诉了如何使用

  echo 1 > /sys/block/vdb/vdb2/trace/enable
  echo blk > /sys/kernel/tracing/current_tracer
  cat /sys/kernel/tracing/trace_pipe

我超，实现的位置居然就是 kernel/trace/blktrace.c

## 分析 kernel/trace/blktrace.c 的实现

这个函数调用频繁，应该每次都在被调用:
```txt
#0  blk_add_trace_rq_issue (ignore=0x0 <fixed_percpu_data>, rq=0xffff88807f034440) at kernel/trace/blktrace.c:856
#1  0xffffffff81789dbc in trace_block_rq_issue (rq=0xffff88807f034440) at ./include/trace/events/block.h:227
#2  blk_mq_start_request (rq=rq@entry=0xffff88807f034440) at block/blk-mq.c:1223
#3  0xffffffff819aed84 in virtblk_prep_rq (hctx=0xffff88800d32e600, req=req@entry=0xffff88807f034440,
    vbr=vbr@entry=0xffff88807f034560, vblk=<optimized out>) at drivers/block/virtio_blk.c:424
#4  0xffffffff819afc3e in virtblk_prep_rq_batch (req=0xffff88807f034440) at drivers/block/virtio_blk.c:475
#5  virtio_queue_rqs (rqlist=0xffffc9000172fdb8) at drivers/block/virtio_blk.c:514
#6  0xffffffff8178e26a in __blk_mq_flush_plug_list (q=<optimized out>, plug=0xffffc9000172fdb8) at block/blk-mq.c:2692
#7  __blk_mq_flush_plug_list (plug=0xffffc9000172fdb8, q=0xffff88800dca9d40) at block/blk-mq.c:2687
#8  blk_mq_flush_plug_list (plug=plug@entry=0xffffc9000172fdb8, from_schedule=from_schedule@entry=false)
    at block/blk-mq.c:2767
#9  0xffffffff8177f3da in __blk_flush_plug (plug=0xffffc9000172fdb8, plug@entry=0xffffc9000172fc28,
    from_schedule=from_schedule@entry=false) at block/blk-core.c:1148
```


```c
static void blk_register_tracepoints(void)
{
	int ret;

	ret = register_trace_block_rq_insert(blk_add_trace_rq_insert, NULL);
	WARN_ON(ret);
	ret = register_trace_block_rq_issue(blk_add_trace_rq_issue, NULL);
	WARN_ON(ret);
	ret = register_trace_block_rq_merge(blk_add_trace_rq_merge, NULL);
	WARN_ON(ret);
	ret = register_trace_block_rq_requeue(blk_add_trace_rq_requeue, NULL);
	WARN_ON(ret);
	ret = register_trace_block_rq_complete(blk_add_trace_rq_complete, NULL);
	WARN_ON(ret);
	ret = register_trace_block_bio_bounce(blk_add_trace_bio_bounce, NULL);
	WARN_ON(ret);
	ret = register_trace_block_bio_complete(blk_add_trace_bio_complete, NULL);
	WARN_ON(ret);
	ret = register_trace_block_bio_backmerge(blk_add_trace_bio_backmerge, NULL);
	WARN_ON(ret);
	ret = register_trace_block_bio_frontmerge(blk_add_trace_bio_frontmerge, NULL);
	WARN_ON(ret);
	ret = register_trace_block_bio_queue(blk_add_trace_bio_queue, NULL);
	WARN_ON(ret);
	ret = register_trace_block_getrq(blk_add_trace_getrq, NULL);
	WARN_ON(ret);
	ret = register_trace_block_plug(blk_add_trace_plug, NULL);
	WARN_ON(ret);
	ret = register_trace_block_unplug(blk_add_trace_unplug, NULL);
	WARN_ON(ret);
	ret = register_trace_block_split(blk_add_trace_split, NULL);
	WARN_ON(ret);
	ret = register_trace_block_bio_remap(blk_add_trace_bio_remap, NULL);
	WARN_ON(ret);
	ret = register_trace_block_rq_remap(blk_add_trace_rq_remap, NULL);
	WARN_ON(ret);
}
```
rq 体系都会执行这个函数:
- blk_add_trace_rq

bio 体系都会执行这个函数:
- blk_add_trace_bio


当开始执行 `blktrace -d /dev/vdc -o - | blkparse -i -` 的时候，结果如下:
```txt
#0  blk_register_tracepoints () at kernel/trace/blktrace.c:1095
#1  0xffffffff8129ace2 in get_probe_ref () at kernel/trace/blktrace.c:337
#2  do_blk_trace_setup (q=q@entry=0xffff88800d6da490, name=name@entry=0xffffc90001d43e78 "vdc", dev=dev@entry=265289760, bdev=bdev@entry=0xffff888008d5d140, buts=buts@entry=0xffffc90001d43df0) at kernel/trace/blktrace.c:612
#3  0xffffffff8129b423 in __blk_trace_setup (q=q@entry=0xffff88800d6da490, name=name@entry=0xffffc90001d43e78 "vdc", dev=265289760, bdev=bdev@entry=0xffff888008d5d140, arg=arg@entry=0x7fff9222a030 "") at kernel/trace/blktrace.c:631
#4  0xffffffff8129cd2f in blk_trace_ioctl (bdev=bdev@entry=0xffff888008d5d140, cmd=cmd@entry=3225948787, arg=arg@entry=0x7fff9222a030 "") at kernel/trace/blktrace.c:741
#5  0xffffffff81795552 in blkdev_ioctl (file=<optimized out>, cmd=3225948787, arg=140735645130800) at block/ioctl.c:610
#6  0xffffffff8145a464 in vfs_ioctl (arg=140735645130800, cmd=<optimized out>, filp=0xffff88800a8ac100) at fs/ioctl.c:51
#7  __do_sys_ioctl (arg=140735645130800, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:870
#8  __se_sys_ioctl (arg=140735645130800, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:856
#9  __x64_sys_ioctl (regs=<optimized out>) at fs/ioctl.c:856
#10 0xffffffff820efd6b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001d43f58) at arch/x86/entry/common.c:50
#11 do_syscall_64 (regs=0xffffc90001d43f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#12 0xffffffff822000aa in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

如何理解 trace_note 的内容: 应该将内容放到 buffer 中。

```txt
#0  trace_note (bt=bt@entry=0xffff8880142c2780, pid=650, action=action@entry=67108864, data=data@entry=0xffff88800acb1918, len=len@entry=16, cgid=cgid@entry=0) at kernel/trace/blktrace.c:76
#1  0xffffffff8129be37 in trace_note_tsk (tsk=<optimized out>) at kernel/trace/blktrace.c:126 #2  __blk_add_trace (bt=bt@entry=0xffff8880142c2780, sector=0, bytes=4096, opf=<optimized out>, what=1114113, what@entry=1048577, error=error@entry=0, pdu_len=0, pdu_data=0x0 <fixed_percpu_data>, cgid=0) at kernel/trace/blktrace.c:268
#3  0xffffffff8129c978 in blk_add_trace_bio (error=0, what=1048577, bio=0xffff88800a684d00, q=0xffff88800d6da490) at kernel/trace/blktrace.c:903
#4  blk_add_trace_bio_queue (ignore=<optimized out>, bio=0xffff88800a684d00) at kernel/trace/blktrace.c:935
#5  0xffffffff8177e7c2 in trace_block_bio_queue (bio=0xffff88800a684d00) at ./include/trace/events/block.h:381
#6  submit_bio_noacct_nocheck (bio=0xffff88800a684d00) at block/blk-core.c:689
```

blk_tracer_enabled

## 注意: 当使用 iosnoop 这种纯粹的 ftrace 接口的时候，并不会有 blk_add_trace_rq_issue

## 总结
