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
- [ ] 从代码的角度解释一下上面的注释。

## 基本设计思路
1. latency_exceeded 的判断方法：
  - `__blk_mq_end_request_acct` -> blk_stat_add -> blk_rq_stat_add 将每次硬件处理一次 IO 需要的时间在 blk_stat_callback::stat 中
  - rq_wb::sync_issue 中记录一个 READ 请求发起的 IO 时间，记录为 rwb_sync_issue_lat
  - 如果 rwb_sync_issue_lat 超过了 cur_win_nsec ，意味着出现了离谱的情况
  - 如果 blk_stat_callback::stat.min 超过了 min_lat_nsec，意味着这段时间的 IO 大多存在问题。
2. wb_timer_fn -> latency_exceeded -> scale_down / scale_up -> rq_depth_calc_max_depth ==> calc_wb_limits
计算 rq_depth::max_depth  进而计算出来 rq_wb::wb_background && rq_wb::wb_normal
3. get_limit 使用 rq_wb::wb_background && rq_wb::wb_normal 来得到当前允许 inflight 的上限。
4. 当开始要 submit_bio 的时候，会比较 inflight 和 get_limit 从而进行 throttle

## 观测和调试
blk-wq.c 中没有关联任何 cgroup 的内容。

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

这个机制和 linux/fs/fs-writeback.c 的关系，为什么

## 分析 rq_qos_ops 的调用时机

```c
static struct rq_qos_ops wbt_rqos_ops = {
	.throttle = wbt_wait,
	.issue = wbt_issue,
	.track = wbt_track,
	.requeue = wbt_requeue,
	.done = wbt_done,
	.cleanup = wbt_cleanup,
	.queue_depth_changed = wbt_queue_depth_changed,
	.exit = wbt_exit,
#ifdef CONFIG_BLK_DEBUG_FS
	.debugfs_attrs = wbt_debugfs_attrs,
#endif
};
```

### wbt_issue : 提交给硬件的时候

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

### wbt_wait : 提交 bio

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
      - `__rq_qos_throttle`
        - wbt_wait

在 rq_qos_throttle 中，会将所有的注册的 rq_qos_ops::throttle 全部执行一次，其中会执行到


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

### wbt_done : 硬件返回的时候

```txt
#0  __wbt_done (wb_acct=WBT_TRACKED, rqos=0xffff8881008a7758) at block/blk-wbt.c:176
#1  wbt_done (rqos=0xffff8881008a7758, rq=0xffff8881042d1680) at block/blk-wbt.c:201
#2  0xffffffff816dc733 in __rq_qos_done (rqos=0xffff8881008a7758, rq=rq@entry=0xffff8881042d1680) at block/blk-rq-qos.c:39
#3  0xffffffff816cbe31 in rq_qos_done (rq=0xffff8881042d1680, q=0xffff888100909ef0) at block/blk-rq-qos.h:178
#4  blk_mq_free_request (rq=0xffff8881042d1680) at block/blk-mq.c:742
#5  0xffffffff81aa9e99 in virtblk_done (vq=0xffff8881008a7000) at drivers/block/virtio_blk.c:291
#6  0xffffffff818134f9 in vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2470
#7  vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2445
#8  0xffffffff811a3d35 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100cb7a00) at kernel/irq/handle.c:158
#9  0xffffffff811a3f13 in handle_irq_event_percpu (desc=0xffff888100cb7a00) at kernel/irq/handle.c:193
#10 handle_irq_event (desc=desc@entry=0xffff888100cb7a00) at kernel/irq/handle.c:210
#11 0xffffffff811a8bee in handle_edge_irq (desc=0xffff888100cb7a00) at kernel/irq/chip.c:819
#12 0xffffffff810ce1d8 in generic_handle_irq_desc (desc=0xffff888100cb7a00) at ./include/linux/irqdesc.h:158
#13 handle_irq (regs=<optimized out>, desc=0xffff888100cb7a00) at arch/x86/kernel/irq.c:231
#14 __common_interrupt (regs=<optimized out>, vector=35) at arch/x86/kernel/irq.c:250
#15 0xffffffff8218a897 in common_interrupt (regs=0xffffc900000a3e38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```

## 计算时间的来源

### rq_wb::last_issue && rq_wb::last_comp
最近是否在 io

1. wbt_wait 中 last_issue 记录 READ submit_bio 的时间
2. wbt_done 中 last_comp 记录 READ 完成的时间
2. 在 get_limit 中 close_io 中，用于描述最近是否完成了 IO

### rq_wb::sync_issue
一次 io 的记录。

sync issue 也就是 read 。

在 wbt_issue 中将提交给硬件的时间放到 sync_issue 中。

在 latency_exceeded 中分析，如果一次 read 的时间超过了时间窗口，那么认为 write 过多，是时候该
约束一下 write 的速度了。

- 看上去 sync 就是 read 了。

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

- nvme_prep_rq
  - nvme_start_request
    - blk_mq_start_request : 将当时的时间写入到 request::io_start_time_ns

### 通用机制 blk_stat_alloc_callback 的实现
这段时间的 io 的记录。

不是采样，而是所有的 request 全部都会分析
`__blk_mq_end_request_acct` 中只有

```c
	if (rq->rq_flags & RQF_STATS) {
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


- virtblk_done : 当从硬件中返回的时候，重新统计
  - blk_mq_end_request
    - `__blk_mq_end_request`
      - `__blk_mq_end_request_acct`
        - blk_stat_add

## wb_timer_fn 的触发频率是多少

- rwb_arm_timer 中按照 rq_wb::cur_win_nsec 来设置的，最终来源为 RWB_WINDOW_NSEC，也就是 100ms 吧

## wbt inflight 理解

分别统计三种，并且对应的阻塞。

```c
static inline struct rq_wait *get_rq_wait(struct rq_wb *rwb,
					  enum wbt_flags wb_acct)
{
	if (wb_acct & WBT_KSWAPD)
		return &rwb->rq_wait[WBT_RWQ_KSWAPD];
	else if (wb_acct & WBT_DISCARD)
		return &rwb->rq_wait[WBT_RWQ_DISCARD];

	return &rwb->rq_wait[WBT_RWQ_BG];
}
```

```c
struct rq_wait {
	wait_queue_head_t wait;
	atomic_t inflight;
};
```
- wbt_wait / wbt_done 这两个时间点之间，也就是有多少提交给队列，还没有被硬件返回。
- 当 inflight 超过 get_limit 的时候，就要开始等待了


## 问题

### direct io 不是在阻塞范围内的
- wbt_wait
  - bio_to_wbt_flags
    - wbt_should_throttle : 在这里，会区分是不是 direct io，如果不是

虽然 direct IO

也就是如下的内容:
```c
enum wbt_flags {
	WBT_TRACKED		= 1,	/* write, tracked for throttling */
	WBT_READ		= 2,	/* read */
	WBT_KSWAPD		= 4,	/* write, from kswapd */
	WBT_DISCARD		= 8,	/* discard */

	WBT_NR_BITS		= 4,	/* number of bits */
};

enum {
	WBT_RWQ_BG		= 0,
	WBT_RWQ_KSWAPD,
	WBT_RWQ_DISCARD,
	WBT_NUM_RWQ,
};
```
1. WBT_RWQ_BG 和 WBT_TRACKED 对应，就是一般的 backgroud write
2. WBT_KSWAPD -> WBT_RWQ_KSWAPD
3. WBT_DISCARD -> WBT_RWQ_DISCARD

### direct io 是采样的范围内吗
不再的。

### 2
- [Determine the specific benefit of Writeback Throttling (CONFIG_WBT)](https://unix.stackexchange.com/questions/483269/determine-the-specific-benefit-of-writeback-throttling-config-wbt)
  - 尝试回答这个问题
    - 简单看了下，很难。

### 3
- [ ] 为什么 swapin 是同步的?
- [ ] read 都是同步的吗？

### 4
- [ ] 注入的错误最后返回给谁？
