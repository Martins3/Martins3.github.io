## 选项 BLK_WBT
- buffered writeback throttling
```c
/*
 * buffered writeback throttling. loosely based on CoDel. We can't drop
 * packets for IO scheduling, so the logic is something like this:
 *
 * - Monitor latencies in a defined window of time.
 * - If the minimum latency in the above window exceeds some target, increment
 *   scaling step and scale down queue depth by a factor of 2x. The monitoring
 *   window is then shrunk to 100 / sqrt(scaling step + 1).
 * - For any window where we don't have solid data on what the latencies
 *   look like, retain status quo.
 * - If latencies look good, decrement scaling step.
 * - If we're only doing writes, allow the scaling step to go negative. This
 *   will temporarily boost write performance, snapping back to a stable
 *   scaling step of 0 if reads show up or the heavy writers finish. Unlike
 *   positive scaling steps where we shrink the monitoring window, a negative
 *   scaling step retains the default step==0 window size.
 *
 * Copyright (C) 2016 Jens Axboe
 *
 */
```
但是 blk-wq.c 中没有关联任何 cgroup 的内容。

观测接口在此处:
```plain
/sys/kernel/debug/block/sda/rqos/wbt
```

```c
static const struct blk_mq_debugfs_attr wbt_debugfs_attrs[] = {
	{"curr_win_nsec", 0400, wbt_curr_win_nsec_show},
	{"enabled", 0400, wbt_enabled_show},
	{"id", 0400, wbt_id_show},
	{"inflight", 0400, wbt_inflight_show},
	{"min_lat_nsec", 0400, wbt_min_lat_nsec_show},
	{"unknown_cnt", 0400, wbt_unknown_cnt_show},
	{"wb_normal", 0400, wbt_normal_show},
	{"wb_background", 0400, wbt_background_show},
	{},
};
```

这个机制和 linux/fs/fs-writeback.c 的关系?

```txt
[<0>] rq_qos_wait+0xba/0x130
[<0>] wbt_wait+0x9b/0x100
[<0>] __rq_qos_throttle+0x1f/0x40
[<0>] blk_mq_submit_bio+0x266/0x510
[<0>] submit_bio_noacct_nocheck+0x25a/0x2b0
[<0>] __swap_writepage+0x13c/0x480
[<0>] pageout+0xcf/0x260
[<0>] shrink_folio_list+0x5e2/0xbd0
[<0>] shrink_lruvec+0x5f4/0xbe0
[<0>] shrink_node+0x2ce/0x6f0
[<0>] do_try_to_free_pages+0xd0/0x560
[<0>] try_to_free_mem_cgroup_pages+0x107/0x230
[<0>] try_charge_memcg+0x19a/0x820
[<0>] charge_memcg+0x2d/0xa0
[<0>] __mem_cgroup_charge+0x28/0x80
[<0>] __filemap_add_folio+0x355/0x430
[<0>] filemap_add_folio+0x36/0xa0
[<0>] __filemap_get_folio+0x1fc/0x330
[<0>] filemap_fault+0x150/0xa00
[<0>] __do_fault+0x2c/0xb0
[<0>] do_fault+0x1e1/0x590
[<0>] __handle_mm_fault+0x5eb/0x12a0
[<0>] handle_mm_fault+0xe4/0x2c0
[<0>] do_user_addr_fault+0x1c7/0x670
[<0>] exc_page_fault+0x66/0x150
[<0>] asm_exc_page_fault+0x26/0x30
```
- blk_mq_submit_bio
  - blk_mq_get_new_requests
    - rq_qos_throttle
  - blk_mq_get_cached_request
    - rq_qos_throttle

在 rq_qos_throttle 中，会将所有的注册的 rq_qos_ops::throttle 全部执行一次。


## 我靠，这个机制真的只是用于阻塞 dirty writeback 的!
- 阻塞的标准是什么?

```txt
#0  wbt_issue (rqos=0xffff888102464e58, rq=0xffff888145890600) at block/blk-wbt.c:613
#1  0xffffffff816dd4c3 in __rq_qos_issue (rqos=0xffff888102464e58, rq=rq@entry=0xffff888145890600) at block/blk-rq-qos.c:48
#2  0xffffffff816cc7e4 in rq_qos_issue (rq=0xffff888145890600, q=0xffff8881046df850) at block/blk-rq-qos.h:184
#3  blk_mq_start_request (rq=rq@entry=0xffff888145890600) at block/blk-mq.c:1258
#4  0xffffffff81b206c9 in nvme_start_request (rq=0xffff888145890600) at drivers/nvme/host/nvme.h:1031
#5  nvme_prep_rq (dev=dev@entry=0xffff8881023f8000, req=req@entry=0xffff888145890600) at drivers/nvme/host/pci.c:912
#6  0xffffffff81b20d26 in nvme_prep_rq (req=0xffff888145890600, dev=0xffff8881023f8000) at drivers/nvme/host/pci.c:897
#7  nvme_queue_rq (hctx=<optimized out>, bd=0xffffc9003f12bdf0) at drivers/nvme/host/pci.c:943
#8  0xffffffff816cf643 in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff888103958a00, list=list@entry=0xffffc9003f12be40, nr_budgets=nr_budgets@entry=0) at block/blk-mq.c:2056
#9  0xffffffff816d5fa8 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff888103958a00) at block/blk-mq-sched.c:319
#10 0xffffffff816d6034 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff888103958a00) at block/blk-mq-sched.c:339
#11 0xffffffff816cbaac in __blk_mq_run_hw_queue (hctx=0xffff888103958a00) at block/blk-mq.c:2174
#12 0xffffffff8114cd57 in process_one_work (worker=worker@entry=0xffff88810293d0c0, work=0xffff888103958a40) at kernel/workqueue.c:2289
#13 0xffffffff8114cf7c in worker_thread (__worker=0xffff88810293d0c0) at kernel/workqueue.c:2436
#14 0xffffffff811556c7 in kthread (_create=0xffff888102d69180) at kernel/kthread.c:376
#15 0xffffffff8100265c in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

```txt
#0  wbt_issue (rqos=0xffff888102464e58, rq=0xffff888145babe00) at block/blk-wbt.c:613
#1  0xffffffff816dd4c3 in __rq_qos_issue (rqos=0xffff888102464e58, rq=rq@entry=0xffff888145babe00) at block/blk-rq-qos.c:48
#2  0xffffffff816cc7e4 in rq_qos_issue (rq=0xffff888145babe00, q=0xffff8881046df850) at block/blk-rq-qos.h:184
#3  blk_mq_start_request (rq=rq@entry=0xffff888145babe00) at block/blk-mq.c:1258
#4  0xffffffff81b206c9 in nvme_start_request (rq=0xffff888145babe00) at drivers/nvme/host/nvme.h:1031
#5  nvme_prep_rq (dev=dev@entry=0xffff8881023f8000, req=req@entry=0xffff888145babe00) at drivers/nvme/host/pci.c:912
#6  0xffffffff81b20f69 in nvme_prep_rq (req=0xffff888145babe00, dev=0xffff8881023f8000) at drivers/nvme/host/pci.c:897
#7  nvme_prep_rq_batch (req=0xffff888145babe00, nvmeq=0xffff888102400600) at drivers/nvme/host/pci.c:978
#8  nvme_queue_rqs (rqlist=0xffffc9003f807d90) at drivers/nvme/host/pci.c:989
#9  0xffffffff816d01ef in __blk_mq_flush_plug_list (q=<optimized out>, q=<optimized out>, plug=0xffffc9003f807d90) at block/blk-mq.c:2731
#10 __blk_mq_flush_plug_list (plug=0xffffc9003f807d90, q=0xffff8881046df850) at block/blk-mq.c:2726
#11 blk_mq_flush_plug_list (plug=plug@entry=0xffffc9003f807d90, from_schedule=from_schedule@entry=false) at block/blk-mq.c:2787
#12 0xffffffff816d0260 in blk_add_rq_to_plug (plug=plug@entry=0xffffc9003f807d90, rq=rq@entry=0xffff888145bac000) at block/blk-mq.c:1296
#13 0xffffffff816d0896 in blk_mq_submit_bio (bio=<optimized out>) at block/blk-mq.c:2991
#14 0xffffffff816c1382 in __submit_bio (bio=<optimized out>) at block/blk-core.c:604
#15 0xffffffff816c190a in __submit_bio_noacct_mq (bio=<optimized out>) at block/blk-core.c:681
#16 submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:698
#17 submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:687
#18 0xffffffff81415843 in __block_write_full_page (inode=<optimized out>, page=0xffffea00059b3d00, get_block=0xffffffff816b8f10 <blkdev_get_block>, wbc=0xffffc9003f807ca0, handler=0xffffffff81414610 <end_buffer_async_write>) at fs/buffer.c:1792
#19 0xffffffff812e9c39 in __writepage (page=page@entry=0xffffea00059b3d00, wbc=wbc@entry=0xffffc9003f807ca0, data=data@entry=0xffff888103d80508) at mm/page-writeback.c:2537
#20 0xffffffff812eac8b in write_cache_pages (mapping=mapping@entry=0xffff888103d80508, wbc=wbc@entry=0xffffc9003f807ca0, writepage=writepage@entry=0xffffffff812e9c20 <__writepage>, data=data@entry=0xffff888103d80508) at mm/page-writeback.c:2472
#21 0xffffffff812ed738 in generic_writepages (wbc=0xffffc9003f807ca0, mapping=0xffff888103d80508) at mm/page-writeback.c:2563
#22 generic_writepages (wbc=0xffffc9003f807ca0, mapping=0xffff888103d80508) at mm/page-writeback.c:2552
#23 do_writepages (mapping=mapping@entry=0xffff888103d80508, wbc=wbc@entry=0xffffc9003f807ca0) at mm/page-writeback.c:2583
#24 0xffffffff81404b4c in __writeback_single_inode (inode=inode@entry=0xffff888103d80390, wbc=wbc@entry=0xffffc9003f807ca0) at fs/fs-writeback.c:1598
#25 0xffffffff81405304 in writeback_sb_inodes (sb=sb@entry=0xffff888100059800, wb=wb@entry=0xffff88811ce51000, work=work@entry=0xffffc9003f807e30) at fs/fs-writeback.c:1889
#26 0xffffffff814055f7 in __writeback_inodes_wb (wb=wb@entry=0xffff88811ce51000, work=work@entry=0xffffc9003f807e30) at fs/fs-writeback.c:1960
#27 0xffffffff81405872 in wb_writeback (wb=wb@entry=0xffff88811ce51000, work=work@entry=0xffffc9003f807e30) at fs/fs-writeback.c:2065
#28 0xffffffff81406be9 in wb_check_background_flush (wb=0xffff88811ce51000) at fs/fs-writeback.c:2131
#29 wb_do_writeback (wb=0xffff88811ce51000) at fs/fs-writeback.c:2219
#30 wb_workfn (work=0xffff88811ce51188) at fs/fs-writeback.c:2246
#31 0xffffffff8114cd57 in process_one_work (worker=worker@entry=0xffff8881487da3c0, work=0xffff88811ce51188) at kernel/workqueue.c:2289
#32 0xffffffff8114cf7c in worker_thread (__worker=0xffff8881487da3c0) at kernel/workqueue.c:2436
#33 0xffffffff811556c7 in kthread (_create=0xffff8881037f3640) at kernel/kthread.c:376
#34 0xffffffff8100265c in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

- 如果使用 direct io ，是不是就不会出现这些 backtrace 了！

并不是:
```txt
#0  wbt_issue (rqos=0xffff888102464e58, rq=0xffff8881459f0000) at block/blk-wbt.c:613
#1  0xffffffff816dd4c3 in __rq_qos_issue (rqos=0xffff888102464e58, rq=rq@entry=0xffff8881459f0000) at block/blk-rq-qos.c:48
#2  0xffffffff816cc7e4 in rq_qos_issue (rq=0xffff8881459f0000, q=0xffff8881046df850) at block/blk-rq-qos.h:184
#3  blk_mq_start_request (rq=rq@entry=0xffff8881459f0000) at block/blk-mq.c:1258
#4  0xffffffff81b206c9 in nvme_start_request (rq=0xffff8881459f0000) at drivers/nvme/host/nvme.h:1031
#5  nvme_prep_rq (dev=dev@entry=0xffff8881023f8000, req=req@entry=0xffff8881459f0000) at drivers/nvme/host/pci.c:912
#6  0xffffffff81b20f69 in nvme_prep_rq (req=0xffff8881459f0000, dev=0xffff8881023f8000) at drivers/nvme/host/pci.c:897
#7  nvme_prep_rq_batch (req=0xffff8881459f0000, nvmeq=0xffff888102400300) at drivers/nvme/host/pci.c:978
#8  nvme_queue_rqs (rqlist=0xffffc9003ffcbca8) at drivers/nvme/host/pci.c:989
#9  0xffffffff816d01ef in __blk_mq_flush_plug_list (q=<optimized out>, q=<optimized out>, plug=0xffffc9003ffcbca8) at block/blk-mq.c:2731
#10 __blk_mq_flush_plug_list (plug=0xffffc9003ffcbca8, q=0xffff8881046df850) at block/blk-mq.c:2726
#11 blk_mq_flush_plug_list (plug=plug@entry=0xffffc9003ffcbca8, from_schedule=from_schedule@entry=false) at block/blk-mq.c:2787
#12 0xffffffff816c2315 in __blk_flush_plug (plug=0xffffc9003ffcbca8, plug@entry=0xffffc9003ffcbc50, from_schedule=from_schedule@entry=false) at block/blk-core.c:1139
#13 0xffffffff816c25d4 in blk_finish_plug (plug=0xffffc9003ffcbc50) at block/blk-core.c:1163
#14 blk_finish_plug (plug=plug@entry=0xffffc9003ffcbca8) at block/blk-core.c:1160
#15 0xffffffff816b9497 in blkdev_write_iter (iocb=0xffff8881009f8f00, from=0xffffc9003ffcbd08) at block/fops.c:538
#16 0xffffffff817113ca in call_write_iter (iter=0xffffc9003ffcbd08, kio=0xffff8881009f8f00, file=<optimized out>) at ./include/linux/fs.h:2186
#17 io_write (req=0xffff8881009f8f00, issue_flags=2147483649) at io_uring/rw.c:918
#18 0xffffffff81701d24 in io_issue_sqe (req=req@entry=0xffff8881009f8f00, issue_flags=issue_flags@entry=2147483649) at io_uring/io_uring.c:1836
#19 0xffffffff81702766 in io_queue_sqe (req=0xffff8881009f8f00) at io_uring/io_uring.c:2008
#20 io_submit_sqe (sqe=0xffff88816e029c00, req=0xffff8881009f8f00, ctx=0xffff88814625f800) at io_uring/io_uring.c:2266
#21 io_submit_sqes (ctx=ctx@entry=0xffff88814625f800, nr=nr@entry=1) at io_uring/io_uring.c:2377
#22 0xffffffff81703095 in __do_sys_io_uring_enter (fd=<optimized out>, to_submit=1, min_complete=0, flags=1, argp=0x0 <fixed_percpu_data>, argsz=0) at io_uring/io_uring.c:3313
#23 0xffffffff82180c3f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9003ffcbf58) at arch/x86/entry/common.c:50
#24 do_syscall_64 (regs=0xffffc9003ffcbf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#25 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```


```txt
#0  latency_exceeded (stat=<optimized out>, rwb=<optimized out>) at block/blk-wbt.c:251
#1  wb_timer_fn (cb=0xffff888102449000) at block/blk-wbt.c:364
#2  0xffffffff811d7c82 in call_timer_fn (timer=timer@entry=0xffff888102449010, fn=fn@entry=0xffffffff816d44c0 <blk_stat_timer_fn>, baseclk=baseclk@entry=4296117448) at kernel/time/timer.c:1700
#3  0xffffffff811d7f8e in expire_timers (head=0xffffc900000f8f00, base=0xffff888237c5cdc0) at kernel/time/timer.c:1751
#4  __run_timers (base=base@entry=0xffff888237c5cdc0) at kernel/time/timer.c:2022
#5  0xffffffff811d8060 in __run_timers (base=0xffff888237c5cdc0) at kernel/time/timer.c:2000
#6  run_timer_softirq (h=<optimized out>) at kernel/time/timer.c:2035
#7  0xffffffff82197e57 in __do_softirq () at kernel/softirq.c:571
#8  0xffffffff811328aa in invoke_softirq () at kernel/softirq.c:445
#9  __irq_exit_rcu () at kernel/softirq.c:650
#10 0xffffffff82184fc6 in sysvec_apic_timer_interrupt (regs=0xffffc90000093e38) at arch/x86/kernel/apic/apic.c:1107
```

```txt
#0  blk_stat_add (rq=rq@entry=0xffff888145a99e00, now=now@entry=1450591300245) at block/blk-stat.c:52
#1  0xffffffff816cd910 in __blk_mq_end_request_acct (now=1450591300245, rq=0xffff888145a99e00) at block/blk-mq.c:1029
#2  blk_mq_end_request_batch (iob=iob@entry=0xffffc900001a8e00) at block/blk-mq.c:1093
#3  0xffffffff81b1f474 in nvme_complete_batch (fn=<optimized out>, iob=0xffffc900001a8e00) at drivers/nvme/host/nvme.h:729
#4  0xffffffff81b2031e in nvme_irq (irq=<optimized out>, data=<optimized out>) at drivers/nvme/host/pci.c:1136
#5  0xffffffff811a4d05 in __handle_irq_event_percpu (desc=desc@entry=0xffff888103932000) at kernel/irq/handle.c:158
#6  0xffffffff811a4ee3 in handle_irq_event_percpu (desc=0xffff888103932000) at kernel/irq/handle.c:193
#7  handle_irq_event (desc=desc@entry=0xffff888103932000) at kernel/irq/handle.c:210
#8  0xffffffff811a9bbe in handle_edge_irq (desc=0xffff888103932000) at kernel/irq/chip.c:819
#9  0xffffffff810ce1d8 in generic_handle_irq_desc (desc=0xffff888103932000) at ./include/linux/irqdesc.h:158
#10 handle_irq (regs=0xffff888103932000, desc=0xffff888103932000) at arch/x86/kernel/irq.c:231
#11 __common_interrupt (regs=regs@entry=0xffffc900001a8ee8, vector=vector@entry=38) at arch/x86/kernel/irq.c:250
#12 0xffffffff82182880 in common_interrupt (regs=0xffffc900001a8ee8, error_code=38) at arch/x86/kernel/irq.c:240
#13 0xffffffff822013e6 in asm_common_interrupt () at ./arch/x86/include/asm/idtentry.h:640
```

## TODO
- [Determine the specific benefit of Writeback Throttling (CONFIG_WBT)](https://unix.stackexchange.com/questions/483269/determine-the-specific-benefit-of-writeback-throttling-config-wbt)
  - 尝试回答这个问题


## submit_bio 是异步的，这里是一个同步的版本
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

- wbt_wait : 几乎必然被调用

## 是通过这个来统计时间吗?  rq_wb::last_issue / last_comp


## 好像，实际上，是这里统计的

```c
struct rq_wait {
	wait_queue_head_t wait;
	atomic_t inflight;
};
```
在每一次 `__wbt_wait` 中，都会进行读取

和 get_limit 比较

其实在比较 inflight 的 request 的数量吗?

```c
struct rq_wb {
	/*
	 * Settings that govern how we throttle
	 */
	unsigned int wb_background;		/* background writeback */
	unsigned int wb_normal;			/* normal writeback */
```
实际上，是在这里的?


计算的位置:
```c
static void calc_wb_limits(struct rq_wb *rwb)
{
	if (rwb->min_lat_nsec == 0) {
		rwb->wb_normal = rwb->wb_background = 0;
	} else if (rwb->rq_depth.max_depth <= 2) {
		rwb->wb_normal = rwb->rq_depth.max_depth;
		rwb->wb_background = 1;
	} else {
		rwb->wb_normal = (rwb->rq_depth.max_depth + 1) / 2;
		rwb->wb_background = (rwb->rq_depth.max_depth + 3) / 4;
	}
}
```


- 这个函数是什么鬼 ?
  - rq_depth_calc_max_depth
    - 只是被 rq_depth_scale_down rq_depth_scale_up 调用，很奇怪啊
        - 进一步 scale_down 和 scale_up
              - 完全是在 wb_timer_fn 控制的

```txt
#0  0xffffffff816dd812 in rq_depth_calc_max_depth (rqd=<optimized out>) at block/blk-rq-qos.c:122
#1  rq_depth_scale_down (rqd=rqd@entry=0xffff888102464ee0, hard_throttle=hard_throttle@entry=false) at block/blk-rq-qos.c:193
#2  0xffffffff816f88db in scale_down (hard_throttle=false, rwb=0xffff888102464e00) at block/blk-wbt.c:323
#3  wb_timer_fn (cb=<optimized out>) at block/blk-wbt.c:400
#4  0xffffffff811d7c82 in call_timer_fn (timer=timer@entry=0xffff888102449010, fn=fn@entry=0xffffffff816d44c0 <blk_stat_timer_fn>, baseclk=baseclk@entry=4296398672) at kernel/time/timer.c:1700
#5  0xffffffff811d7f8e in expire_timers (head=0xffffc900000f8f00, base=0xffff888237c5cdc0) at kernel/time/timer.c:1751
#6  __run_timers (base=base@entry=0xffff888237c5cdc0) at kernel/time/timer.c:2022
#7  0xffffffff811d8060 in __run_timers (base=0xffff888237c5cdc0) at kernel/time/timer.c:2000
#8  run_timer_softirq (h=<optimized out>) at kernel/time/timer.c:2035
#9  0xffffffff82197e57 in __do_softirq () at kernel/softirq.c:571
#10 0xffffffff811328aa in invoke_softirq () at kernel/softirq.c:445
#11 __irq_exit_rcu () at kernel/softirq.c:650
#12 0xffffffff82184fc6 in sysvec_apic_timer_interrupt (regs=0xffffc90000093e38) at arch/x86/kernel/apic/apic.c:1107
```

## 分析 blk_stat_alloc_callback 的实现

blk_mq_init_allocated_queue 中出现了
```c
	q->poll_cb = blk_stat_alloc_callback(blk_mq_poll_stats_fn,
					     blk_mq_poll_stats_bkt,
					     BLK_MQ_POLL_STATS_BKTS, q);
```

似乎类似的发现了一个 io poll 的操作，blk_mq_poll_stats_fn 始终无法触发。

我靠，想不到居然还存在两个

## 基本的原理
1. wb_timer_fn 来分析是否存在超时的。
2. 如果超时，减小队列，让这些 fio 的时间增长。


主要是在此处计算时间吧：
```c
	/*
	 * If the 'min' latency exceeds our target, step down.
	 */
	if (stat[READ].min > rwb->min_lat_nsec) {
		trace_wbt_lat(bdi, stat[READ].min);
		trace_wbt_stat(bdi, stat);
		return LAT_EXCEEDED;
	}
```

blk_stat_callback::stat 是如何维护的

blk_stat_alloc_callback : 中分配，wbt 分配两个，分别记录 READ 和 WRITE 的

时间的统计的位置：
```txt
#0  blk_rq_stat_add (value=921592090, stat=0xffffe8ffffc42a68) at block/blk-stat.c:45
#1  blk_stat_add (rq=rq@entry=0xffff888103d08e80, now=now@entry=2637219438354) at block/blk-stat.c:74
#2  0xffffffff816ccd26 in __blk_mq_end_request_acct (now=2637219438354, rq=0xffff888103d08e80) at block/blk-mq.c:1029
#3  __blk_mq_end_request (rq=0xffff888103d08e80, error=<optimized out>) at block/blk-mq.c:1039
#4  0xffffffff81aa26f9 in virtblk_done (vq=0xffff888100dfba00) at drivers/block/virtio_blk.c:291
#5  0xffffffff8180bf09 in vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2470
#6  vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2445
#7  0xffffffff811a4d05 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100ba6600) at kernel/irq/handle.c:158
#8  0xffffffff811a4ee3 in handle_irq_event_percpu (desc=0xffff888100ba6600) at kernel/irq/handle.c:193
#9  handle_irq_event (desc=desc@entry=0xffff888100ba6600) at kernel/irq/handle.c:210
#10 0xffffffff811a9bbe in handle_edge_irq (desc=0xffff888100ba6600) at kernel/irq/chip.c:819
#11 0xffffffff810ce1d8 in generic_handle_irq_desc (desc=0xffff888100ba6600) at ./include/linux/irqdesc.h:158
#12 handle_irq (regs=<optimized out>, desc=0xffff888100ba6600) at arch/x86/kernel/irq.c:231
#13 __common_interrupt (regs=<optimized out>, vector=35) at arch/x86/kernel/irq.c:250
#14 0xffffffff821828d7 in common_interrupt (regs=0xffffc90000093e38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc900000f9018
```

## 应该不是采样，而是所有的 request 全部都会分析
`__blk_mq_end_request_acct` 中只有

```c
	ifplainplain (rq->rq_flags & RQF_STATS) {
```

这个 flags 插入的时间为:

- blk_mq_start_request 中插入 flags 的

```c
	if (test_bit(QUEUE_FLAG_STATS, &q->queue_flags)) {
		rq->io_start_time_ns = ktime_get_ns();
		rq->stats_sectors = blk_rq_sectors(rq);
		rq->rq_flags |= RQF_STATS;
		rq_qos_issue(q, rq);
	}
```
插入上时间戳，记录大小，并且触发 rq_qos_issue

rq_qos_issue 中调用 hook，也就是 wbt_issue，rwb_sync_issue_lat 中

```txt
#0  rwb_sync_issue_lat (rwb=<optimized out>) at block/blk-wbt.c:364
#1  latency_exceeded (stat=<optimized out>, rwb=<optimized out>) at block/blk-wbt.c:251
#2  wb_timer_fn (cb=0xffff888103a34700) at block/blk-wbt.c:364
#3  0xffffffff811d7c82 in call_timer_fn (timer=timer@entry=0xffff888103a34710, fn=fn@entry=0xffffffff816d44c0 <blk_stat_timer_fn>, baseclk=baseclk@entry=4297304824) at kernel/time/timer.c:1700
#4  0xffffffff811d7f8e in expire_timers (head=0xffffc900000f8f00, base=0xffff888237c5cdc0) at kernel/time/timer.c:1751
#5  __run_timers (base=base@entry=0xffff888237c5cdc0) at kernel/time/timer.c:2022
#6  0xffffffff811d8060 in __run_timers (base=0xffff888237c5cdc0) at kernel/time/timer.c:2000
#7  run_timer_softirq (h=<optimized out>) at kernel/time/timer.c:2035
#8  0xffffffff82197e57 in __do_softirq () at kernel/softirq.c:571
#9  0xffffffff811328aa in invoke_softirq () at kernel/softirq.c:445
#10 __irq_exit_rcu () at kernel/softirq.c:650
#11 0xffffffff82184fc6 in sysvec_apic_timer_interrupt (regs=0xffffc90000093e38) at arch/x86/kernel/apic/apic.c:1107
```

- blk_stat_timer_fn : 周期性的将，将所有的 cpu 的时间汇总起来
  - wb_timer_fn : 被调用的 hook，其中来比较这段时间中


- nvme_prep_rq
  - nvme_start_request
    - blk_mq_start_request : 将当时的时间写入到 request::io_start_time_ns

- virtblk_done : 当从硬件中返回的时候，重新统计
  - blk_mq_end_request
    - `__blk_mq_end_request`
      - `__blk_mq_end_request_acct`
        - blk_stat_add

## wb_timer_fn 的触发频率是多少

- rwb_arm_timer 中按照 rq_wb::cur_win_nsec 来设置的，最终来源为 RWB_WINDOW_NSEC，也就是 100ms 吧

## 这里的 inflight 是 wbt 单独统计的吗?
是的

## WBT_KSWAPD 是特殊管理的，为什么需要如此发
