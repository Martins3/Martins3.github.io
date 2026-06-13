## mq 核心结构体
<!-- 5e26e61e-19b8-4569-960e-54d43ec4dc05 -->

- request_queue
- request
- blk_mq_hw_ctx
  - 硬件队列
- blk_mq_ctx
  - 软件队列
- blk_mq_tags : 用于描述 tag 和 request 的集合
  - 管理一组 tag 使用 bitmap 来管理
  - 每个 blk_mq_hw_ctx 和 blk_mq_ctx 都有对应的blk_mq_tags
  - 主要成员 :
    - active_queues : 记录当前有多少个 request_queue 在使用这个 blk_mq_tags
- blk_mq_tag_set : tag set that can be shared between request queues
  - 用于描述与存储器件相关的 tag 集合

一个 blk_mq_tag_set 可以在以下情况下被多个 request_queue 共享：
- NVMe 驱动器，多个命名空间共享硬件资源。 (有待验证，不过基本)
- 设备共享：多个块设备（如 virtio-scsi 下的盘）在同一个 HBA 卡下单。

在热插 virtio-scsi 盘的时候，大致是这个流程:
```txt
@[
        blk_alloc_queue+5
        blk_mq_alloc_queue+98
        scsi_alloc_sdev+640
        scsi_probe_and_add_lun+499
        __scsi_scan_target+271
        scsi_scan_target+231
        virtscsi_handle_event+666
        process_one_work+501
        worker_thread+462
        kthread+268
        ret_from_fork+479
        ret_from_fork_asm+26
]: 1
```
所以，简单来说，
- 一个 hba 对应多个盘:
- 一个 hba 卡一个 tag_set ，tag_sets 来控制 hw queue 和 sw queue
- 一个 hctx 对应一个控制器的一个 queue
- 一个盘一个 request_queue 的，所以在 gendisk 中存在一个 request_queue 的指针。

他们的数量关系也就是:
- gendisk : request_queue = 1 : 1
- request_queue : blk_mq_hw_ctx = 1 : N
- request_queue : blk_mq_tag_set = N : 1 这个可以共享，不一定独占


事已至此，应该完全清楚了吧:
```text
struct gendisk
  └── queue --------------------------> struct request_queue
                                          ├── tag_set ----------> struct blk_mq_tag_set
                                          │                        ├── tags ---------> struct blk_mq_tags *[nr_hw_queues]
                                          │                        │                    └── rqs -----> struct request *[nr_tags]
                                          │                        └── shared_tags ---> struct blk_mq_tags (可选，共享tag场景)
                                          │
                                          ├── queue_hw_ctx ------> struct blk_mq_hw_ctx *[nr_hw_queues]
                                          │                        ├── queue --------> 回指 request_queue
                                          │                        ├── ctxs ---------> struct blk_mq_ctx *[nr_ctx]
                                          │                        ├── tags ---------> 对应这个 hctx 的 blk_mq_tags
                                          │                        └── sched_tags ---> 调度器用的 blk_mq_tags
                                          │
                                          ├── queue_ctx ---------> percpu struct blk_mq_ctx
                                          │                        ├── queue --------> 回指 request_queue
                                          │                        ├── hctxs[] ------> 这个 CPU 映射到的 blk_mq_hw_ctx
                                          │                        └── rq_lists[] ---> 本 CPU 的软件派发链表
                                          │
                                          ├── elevator ----------> IO scheduler
                                          └── disk --------------> 回指 gendisk
```

基本的 io 流程:
```text
用户提交 I/O
   |
   v
bio
   |
   v
blk_mq_submit_bio()
   |
   v
request_queue (q)
   |
   | 1. 先选当前 CPU 对应的软件队列
   v
blk_mq_ctx (ctx, per-cpu software queue)
   |\
   | \-- rq_lists[]              软件暂存/合并队列
   | \-- hctxs[type] ----------> 这个 ctx 对应的硬件队列
   |
   | 2. 根据 opf/type 映射
   v
blk_mq_hw_ctx (hctx, hardware dispatch queue)
   |\
   | \-- ctxs[] ----------------> 归这个 hctx 服务的一组 ctx
   | \-- dispatch               派发但暂时没下发成功的 request
   | \-- tags -----------------> struct blk_mq_tags
   |                              \-- rqs[tag] --> struct request *
   | \-- sched_tags ------------> 若启用 scheduler，则分配调度 tag
   |
   | 3. 分配 tag，拿到 request
   v
struct request (rq)
   |
   | 4. 驱动下发
   v
blk_mq_ops.queue_rq(hctx, bd)
   |
   v
设备驱动 / 硬件
   |
   | 5. 完成中断/轮询
   v
blk_mq_complete_request(rq)
   |
   v
根据 rq->tag 回到 blk_mq_tags->rqs[tag]
   |
   v
request 完成，bio 回调结束
```




## blk_mq_ctx 是软件队列

如果 scheduler 是空，那么 blk_mq_ctx 是什么定位?

## 核心文件
- block/blk-mq-sched.c
- block/blk-mq-tag.c


## 核心流程
- blk_mq_submit_bio
  - blk_mq_get_cached_request
    - blk_mq_insert_request
  - bio_queue_enter : 增加 queue 的引用计数，如果 queeu 被 freeze ，那么阻塞在此处
  - blk_mq_get_new_requests
    - blk_mq_attempt_bio_merge
        - blk_mq_sched_bio_merge
          - dd_bio_merge
            - blk_mq_sched_try_merge
              - dd_request_merge
              - elv_merge
    - rq_qos_throttle : 在这里进行限流
    - __blk_mq_alloc_requests : 开始并且初始化 request

	- 如果存在调度的情况
		- blk_mq_insert_request
			- dd_insert_requests : 插入到软件队列，sched 或者 硬件队列中
		- blk_mq_run_hw_queue
	- 如果不存在调度情况
		- blk_mq_try_issue_directly


- blk_flush_plug : 从软件到硬件提交的过程
  - `__blk_flush_plug`
    - blk_mq_flush_plug_list
      - blk_mq_dispatch_plug_list
        - 如果 queue 存在调度器
          - blk_mq_dispatch_plug_list::queue->elevator->type->ops.insert_requests
          - blk_mq_run_hw_queue
        - 否则，直接向下提交
          - blk_mq_insert_requests
            - list_splice_tail_init : 存放到软件队列中
            - blk_mq_run_hw_queue
              - blk_mq_sched_dispatch_requests
                - blk_mq_dispatch_rq_list
                  - q->mq_ops->queue_rq(hctx, &bd);

## blk_mq_insert_request

- [ ] 分析下，blk_mq_insert_request 的调用位置， 调用位置很多
- [ ] 高速路径不会调用这个

 - `__submit_bio_noacct_mq`
  - blk_mq_insert_request
    - dd_insert_requests

1. blk_rq_is_passthrough

通过 blk_mq_request_bypass_insert 将 blk_mq_hw_ctx::dispatch 上，
也就是放到硬件队列上。

2. req_op(rq) == REQ_OP_FLUSH

也是将 request 放到硬件队列上，但是会直接放到队列的头上。

3. q->elevator

将 request 插入到 elevator 的队列上

4. fallback 的情况

放到软件队列上
```c
			list_add(&rq->queuelist, &ctx->rq_lists[hctx->type]);
```

### 从哪里进入到软件队列中的，从哪里进入硬件队列的?

blk_mq_try_issue_directly

```txt
		blk_mq_insert_request(rq, 0);
		blk_mq_run_hw_queue(hctx, false);
```

## 分配以及释放一个 request

```txt
@[
    blk_mq_get_tag
    __blk_mq_alloc_requests+757
    blk_mq_submit_bio+442
    submit_bio_noacct_nocheck+653
    blkdev_direct_IO.part.0+574
    blkdev_read_iter+176
    __io_read+234
    io_read+21
    io_issue_sqe+96
    io_submit_sqes+507
    __do_sys_io_uring_enter+948
    do_syscall_64+67
    entry_SYSCALL_64_after_hwframe+111
]: 547488
```


```c
	struct blk_mq_tags *tags = blk_mq_tags_from_data(data);
```

而 blk_mq_tags_from_data 会是否使用 sched 自动确定。


```txt
#0  blk_mq_put_tag (tags=0xffff888103ae2900, ctx=ctx@entry=0xffffe8ffffd42700, tag=211) at block/blk-mq-tag.c:220
#1  0xffffffff816ca4de in __blk_mq_free_request (rq=0xffff8881041d3d80) at block/blk-mq.c:720
#2  0xffffffff81aa26f9 in virtblk_done (vq=0xffff888100897200) at drivers/block/virtio_blk.c:291
#3  0xffffffff8180bf09 in vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2470
#4  vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2445
#5  0xffffffff811a4d05 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100ce7e00) at kernel/irq/handle.c:158
```

- virtblk_done
  - blk_mq_complete_request
    - virtblk_request_done
      - blk_mq_end_request
        - `__blk_mq_end_request`
          - blk_mq_free_request
            - `__blk_mq_free_request`
              - blk_mq_put_tag


## blk_mq_insert_request 中会加入软件队列，在 blk_mq_sched_dispatch_requests 离开队列

```txt
#0  blk_mq_sched_dispatch_requests (hctx=0xffff88810aedce00) at block/blk-mq-sched.c:320
#1  0xffffffff8190f0ea in blk_mq_run_hw_queue (hctx=0xffff88810aedce00, async=<optimized out>) at block/blk-mq.c:2285
#2  0xffffffff8190f3d3 in blk_mq_run_hw_queues (q=q@entry=0xffff8881061c1c40, async=async@entry=false) at block/blk-mq.c:2334
#3  0xffffffff8190f7a2 in blk_mq_requeue_work (work=0xffff8881061c1e50) at block/blk-mq.c:1503
#4  0xffffffff81168b88 in process_one_work (worker=worker@entry=0xffff888107466240, work=0xffff8881061c1e50) at kernel/workqueue.c:2633
#5  0xffffffff81169045 in process_scheduled_works (worker=<optimized out>) at kernel/workqueue.c:2706
#6  worker_thread (__worker=0xffff888107466240) at kernel/workqueue.c:2787
```

```c
static int __blk_mq_sched_dispatch_requests(struct blk_mq_hw_ctx *hctx)
{
	/* dequeue request one by one from sw queue if queue is busy */
	if (need_dispatch)
		return blk_mq_do_dispatch_ctx(hctx);
	blk_mq_flush_busy_ctxs(hctx, &rq_list);
	blk_mq_dispatch_rq_list(hctx, &rq_list, 0);
```

- blk_mq_sched_dispatch_requests
  - __blk_mq_sched_dispatch_requests
    - blk_mq_do_dispatch_ctx
      - blk_mq_dequeue_from_ctx
        - dispatch_rq_from_ctx

## elevator_mq_ops

- dd_insert_requests 两个调用位置
  - blk_mq_dispatch_plug_list
  - blk_mq_insert_request

- dd_finish_request : 主要处理 zone block devices
```txt
#0  dd_finish_request (rq=0xffff888005374000) at block/mq-deadline.c:919
#1  0xffffffff81794e09 in blk_mq_free_request (rq=rq@entry=0xffff888005374000) at block/blk-mq.c:712
#2  0xffffffff81795073 in __blk_mq_end_request (rq=rq@entry=0xffff888005374000, error=error@entry=0 '\000') at block/blk-mq.c:1026
#3  0xffffffff819eeeb0 in scsi_end_request (req=req@entry=0xffff888005374000, error=error@entry=0 '\000', bytes=bytes@entry=4096) at drivers/scsi/scsi_lib.c:576
#4  0xffffffff819ef8ba in scsi_io_completion (cmd=0xffff888005374120, good_bytes=4096) at drivers/scsi/scsi_lib.c:978
#5  0xffffffff8179367d in blk_complete_reqs (list=<optimized out>) at block/blk-mq.c:1113
#6  0xffffffff82177923 in __do_softirq () at kernel/softirq.c:553
```

- dd_prepare_request
  - blk_mq_rq_ctx_init : 在分配 request 的时候


- dd_bio_merge : 合并逻辑参考 block/blk-merge.c
  执行路径:
  - blk_mq_get_cached_request
  - blk_mq_get_new_requests
    - blk_mq_attempt_bio_merge
      - blk_mq_sched_bio_merge
        - `bool (*elevator_mq_ops::bio_merge)(struct request_queue *, struct bio *, unsigned int)`
  - 如果合并了，会将该 request 直接释放掉

- dd_request_merge :
  执行路径:
	- blk_mq_sched_try_merge
		- elv_merge
			- `int (*elevator_mq_ops::request_merge)(struct request_queue *, struct request **, struct bio *)`

- dd_dispatch_request : 两个调用路径
```txt
#0  dd_dispatch_request (hctx=0xffff888009878a00) at block/mq-deadline.c:597
#1  0xffffffff8179f3d2 in __blk_mq_do_dispatch_sched (hctx=0xffff888009878a00) at block/blk-mq-sched.c:118
#2  blk_mq_do_dispatch_sched (hctx=0xffff888009878a00) at block/blk-mq-sched.c:184
#3  __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff888009878a00) at block/blk-mq-sched.c:309
#4  0xffffffff8179f7c7 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff888009878a00) at block/blk-mq-sched.c:333
#5  0xffffffff81793441 in blk_mq_run_work_fn (work=0xffff888009878a40) at block/blk-mq.c:2418
#6  0xffffffff8116ba8a in process_one_work (worker=worker@entry=0xffff88810a3046c0, work=0xffff888009878a40) at kernel/workqueue.c:2597


#0  dd_dispatch_request (hctx=0xffff888104990400) at block/mq-deadline.c:597
#1  0xffffffff81917b6b in __blk_mq_do_dispatch_sched (hctx=0xffff888104990400) at block/blk-mq-sched.c:118
#2  blk_mq_do_dispatch_sched (hctx=0xffff888104990400) at block/blk-mq-sched.c:184
#3  __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff888104990400) at block/blk-mq-sched.c:309
#4  0xffffffff81917e1c in blk_mq_sched_dispatch_requests (hctx=0xffff888104990400) at block/blk-mq-sched.c:331
#5  0xffffffff8190f0ea in blk_mq_run_hw_queue (hctx=0xffff888104990400, async=<optimized out>) at block/blk-mq.c:2285
#6  0xffffffff8191582e in blk_mq_get_tag (data=data@entry=0xffffc90001e4bb50) at block/blk-mq-tag.c:170
#7  0xffffffff8190fdb9 in __blk_mq_alloc_requests (data=data@entry=0xffffc90001e4bb50) at block/blk-mq.c:500
#8  0xffffffff819125d9 in blk_mq_get_new_requests (nsegs=<optimized out>, bio=0xffff88810ea0df80, plug=0x0 <fixed_percpu_data>, q=0xffff88810ddcfb98)
```

- blk_mq_dispatch_plug_list
- blk_mq_insert_request

- [ ] dd_bio_merge 和 dd_request_merge 的区别是什么?

## elevator 队列

分析 dd_insert_requests 和 dd_dispatch_request 的调用路径，可以知道，显然，
要么将新创建的，或者 plug 的放到 scheduler 中，之后发送给硬件队列


## blk_mq_run_hw_queue

- blk_mq_run_hw_queue
  - blk_mq_sched_dispatch_requests
    - __blk_mq_sched_dispatch_requests
      - 在这里从 sched 或者软件队列中搜刮 request ，然后通过 blk_mq_dispatch_rq_list 发送给 driver ，例如调用 request_queue::mq_ops::queue_rq 进入到 virtio_queue_rq

在 __blk_mq_sched_dispatch_requests 中，分别处理将 sched 中和软件队列中取出来:
```c
	if (hctx->queue->elevator)
		return blk_mq_do_dispatch_sched(hctx);

	/* dequeue request one by one from sw queue if queue is busy */
	if (need_dispatch)
		return blk_mq_do_dispatch_ctx(hctx);
	blk_mq_flush_busy_ctxs(hctx, &rq_list);
	blk_mq_dispatch_rq_list(hctx, &rq_list, 0);
	return 0;
```

调用的位置:
1. 从 plug 中取出来
```txt
__blk_mq_delay_run_hw_queue+1
blk_mq_sched_insert_requests+110
blk_mq_flush_plug_list+291
__blk_flush_plug+262
io_schedule+65
rq_qos_wait+191
wbt_wait+160
__rq_qos_throttle+36
blk_mq_submit_bio+645
```

2. requeue 中取出来
```txt
    blk_mq_dispatch_rq_list+5
    __blk_mq_sched_dispatch_requests+171
    blk_mq_sched_dispatch_requests+57
    __blk_mq_run_hw_queue+145
    blk_mq_run_hw_queues+105
    blk_mq_requeue_work+340
    process_one_work+482
    worker_thread+84
```

## [ ] 常见，但是没有兴趣
1. blk_mq_hctx_mark_pending
2. blk_mq_rq_from_pdu
3. flush 机制
```txt
@[
    blk_queue_exit+5
    blk_flush_complete_seq+352
    mq_flush_data_end_io+130
    __blk_mq_end_request+70
    nvme_poll_cq+442
    nvme_irq+69
    __handle_irq_event_percpu+74
    handle_irq_event+62
    handle_edge_irq+157
    __common_interrupt+66
    common_interrupt+127
    asm_common_interrupt+38
    cpuidle_enter_state+204
    cpuidle_enter+45
    do_idle+452
    cpu_startup_entry+29
    start_secondary+277
    secondary_startup_64_no_verify+224
]: 52
```
4. blk_mq_ops 注册的 hook 逐个解析，这几个感觉比较重要，但是也没太搞懂
  - commit_rqs : scsi_mq_setup_tags 中 scsi_mq_ops 和 scsi_mq_ops_no_commit 仅仅的区分在于 是否注册了 commit_rqs
  - map_queues
5. requeue 机制
  - blk_mq_requeue_request : 中断返回了，但是仅仅完成了部分 io ，在 scsi 中，错误重试也是一种可能。
6. 如何保证动态修改 io scheduler 的稳定性


## 软件队列和硬件队列的初始化

创建队列存在两个过程，第一个过程是探测 HBA 卡 / nvme controller 的时候，这个过程会创建 tag_set
- nvme_probe
  - nvme_alloc_admin_tag_set
    - blk_mq_alloc_tag_set 将会分配如下内容:
      - blk_mq_tags : Tag address space map
      - blk_mq_queue_map : Map software queues to hardware queues
- do_floppy_init
- loop_init
  - loop_add
    - blk_mq_alloc_tag_set
- virtio_dev_probe
  - virtblk_probe
    - blk_mq_alloc_tag_set
- scsi_add_host
  - scsi_add_host_with_dma
    - scsi_mq_setup_tags
      - blk_mq_alloc_tag_set


第二个过程是探测到了具体的盘，使用 tag_set 作为参数，来创建
创建出来 hctx 和 ctx ，并且构建映射:

- call_driver_probe
  - virtio_dev_probe
    - virtblk_probe
      - __blk_mq_alloc_disk
        - blk_mq_init_queue_data

- virtscsi_probe
  - scsi_scan_host
    - scsi_scan_host_selected
      - scsi_scan_channel
        - __scsi_scan_target
          - scsi_probe_and_add_lun
            - scsi_alloc_sdev
              - blk_mq_init_queue
                - blk_mq_init_queue_data

- blk_mq_init_queue
  - blk_alloc_queue : 创建 request_queue
  - blk_mq_init_allocated_queue
    - blk_mq_alloc_ctxs : 给每一个盘创建 software queue : blk_mq_ctx
    - blk_mq_realloc_hw_ctxs
      - blk_mq_alloc_and_init_hctx
        - blk_mq_init_hctx
          - 将 hctx 和 hctx_idx 放到 request_queue::hctx_table 中
          - 将 tag_set 中分配的 blk_mq_tags 传递给 hctx : `hctx->tags = set->tags[hctx_idx];` ，hctx 是无需额外创建。
    - blk_mq_init_cpu_queues : 给软件队列映射
    - blk_mq_map_swqueue : 建立软件队列和硬件队列的关系


1. 一个盘一个 request_queue, 而不是是一个 HBA 卡一个 request queue
2. HBA 卡每一个盘两个盘 sda 和 sdb ，其中 /sys/kernel/debug/block/sd*/hctx0 指向是同一个 hctx0 吗?
  - 是的，硬件队列是一个控制器一个
3. HBA 卡每一个盘两个盘 sda 和 sdb  同一个 HBA 卡下盘会共享软件队列吗
  - 不会，软件队列是每一个盘一个
4. blk_mq_tag_set 就是给 shared tags 使用的，如果不是共享 tags 就不需要这个东西了?
  - 不是，tag_set 就是将所有的硬件队列的 tags 放到一起，这些硬件队列可以共享一个 blk_mq_tags ，这个时候称为 shared
```txt
 struct blk_mq_tag_set - tag set that can be shared between request queues
```
  - 这里的共享是 tag_set 被多个盘共享


## [ ] queue 的引用计数问题分析

- blk_mq_freeze_queue -> blk_freeze_queue
  - blk_freeze_queue_start
    - 上锁，并且让 queue 中的 request 赶快提交
  - blk_mq_freeze_queue_wait
	  - wait_event(q->mq_freeze_wq, percpu_ref_is_zero(&q->q_usage_counter));

总是 blk_queue_enter 和 bio_queue_enter 和 blk_queue_exit 匹配的

- [ ] put 的位置，主要是三个地方:
  - blk_mq_dispatch_plug_list : blk_mq_run_hw_queue 或者 blk_mq_insert_requests 就开始 percpu_ref_put ，这个有点不符合理解
  - blk_mq_end_request_batch
    - blk_mq_flush_tag_batch
      - percpu_ref_put_many : 这个符合预期
  - blk_queue_exit : 这是一个主要调用的位置

但是实际上，主要的提交位置是，这个应该不是常用路径吧
```txt
@[
    blk_queue_exit+5
    blk_mq_submit_bio+1301
    submit_bio_noacct_nocheck+818
    __block_write_full_page+509
    writepage_cb+26
    write_cache_pages+319
    do_writepages+356
    __writeback_single_inode+61
    writeback_sb_inodes+493
    __writeback_inodes_wb+76
    wb_writeback+516
    wb_workfn+852
    process_one_work+453
    worker_thread+81
    kthread+219
    ret_from_fork+41
]: 1376
```

- blk_mq_free_request
  - __blk_mq_free_request
    - blk_queue_exit : 只要还存在 request 的时候，那么 queue 就是无法释放的

逻辑上应该是没有问题的，这里也可以印证:
- del_gendisk
  - blk_queue_start_drain : 从这里开始阻塞新的 io
    - blk_freeze_queue_start
  - 处理其他的事情
  - blk_mq_freeze_queue_wait : 等待 io 结束

### [ ] request_queue::q_usage_counter 和 request_queue::refs 是什么关系
```c
bool blk_get_queue(struct request_queue *q)
{
	if (likely(!blk_queue_dying(q))) {
		__blk_get_queue(q);
		return true;
	}

	return false;
}

void blk_put_queue(struct request_queue *q)
{
	kobject_put(&q->kobj);
}
```

## block/blk-flush.c 的功能

block/blk-flush.c 开头的位置包含了详细的注释。这里需要 flush 的内容是将
磁盘的缓存进行 flush 。

Documentation/block/writeback_cache_control.rst

- blk_mq_submit_bio : 进入到 blk-mq.c 中了
  - blk_mq_get_cached_request
  - blk_insert_flush
    - blk_flush_queue_rq
      - blk_mq_add_to_requeue_list
        - blk_mq_kick_requeue_list

当上层从 blkdev_issue_flush 调用过之后，那么该 request 将会携带 flush 机制了。

到底什么样子的盘是需要这个机制，存疑。

## 这个看看，也是加深理解了
https://gitee.com/anolis/cloud-kernel/commit/80d6ee240ea37f36ecd6dda5c8a6d7723cba73d3

## 到底 io timeout 统计的延迟是发送给硬件还是发送给驱动开始的？

会出现处理速度过慢导致用户态观察到 slow io 吗?
例如让 qemu 1s 才返回一次。

应该是发生给硬件的，和使用 `sudo biolatency -d sdb -T` 一个问题。

## 我就知道啊 iouring 和 hctx 是有关联的

```c
int blk_rq_poll(struct request *rq, struct io_comp_batch *iob,
		unsigned int poll_flags)
{
	struct request_queue *q = rq->q;
	int ret;

	if (!blk_rq_is_poll(rq))
		return 0;
	if (!percpu_ref_tryget(&q->q_usage_counter))
		return 0;

	ret = blk_hctx_poll(q, rq->mq_hctx, iob, poll_flags);
	blk_queue_exit(q);

	return ret;
}
```

## block/blk-map.c
<!-- e96e5caa-99c7-4b43-9a00-0525fce4ee82 -->

才注意到这个东西，似乎是给 nvme 和 scsi 直接提交命令的，

这个文件主要服务于passthrough I/O(直通I/O),例如:
  - SCSI Generic (SG_IO) 命令
  - 用户空间直接发送块设备命令的场景
  - O_DIRECT 标志的直接I/O操作
  - 内核模块需要构造块设备请求时


> [!NOTE]
> 参考神奇海螺的意见，有待验证

(按道理虚拟机中可以用来测试这个 iouring cmd 的)

关系链条

io_uring URING_CMD
    ↓
设备驱动的 uring_cmd 实现（如 NVMe）
    ↓
blk_rq_map_user_iov/blk_rq_map_user_io
    ↓
block/blk-map.c 的映射函数

具体证据

1. NVMe 的 io_uring cmd 实现

在 drivers/nvme/host/ioctl.c:140,142 中，NVMe 的 nvme_uring_cmd_io 函数使用了：
ret = blk_rq_map_user_iov(q, req, NULL, iter, GFP_KERNEL);
ret = blk_rq_map_user_io(req, NULL, nvme_to_user_ptr(ubuffer), ...);

这些函数正是 blk-map.c 中导出的。

2. 块设备的 uring_cmd

block/fops.c:972 注册了 blkdev_uring_cmd，定义在 block/ioctl.c:851。目前主要用于 DISCARD 等命令，不直接使用数据映射

3. 工作流程

当用户通过 io_uring 提交 IORING_OP_URING_CMD 操作时：
1. io_uring 核心代码调用设备的 ->uring_cmd() 回调
2. 设备驱动（如 NVMe）需要将用户提供的 buffer 映射到块请求
3. 此时调用 blk_rq_map_user_iov() 等 blk-map.c 的函数
4. blk-map.c 负责处理零拷贝或拷贝映射，将数据关联到 bio

总结

blk-map.c 是 io_uring passthrough 命令的基础设施之一，专门负责处理用户空间数据到块请求的映射。对于需要传输数据的
io_uring cmd（如 NVMe 的读写命令），都需要通过 blk-map.c 完成数据映射。


## block/blk-mq-cpumap.c
<!-- 6a2a1cec-a53b-47d4-9dae-a8009ebab7a8 -->

## 下一步的代办
1. 继续理解一下 blk_mq_ctx 在有软件 scheduler 的作用
2. 没有 scheduler 的作用是什么
3. 应该测试一下一个 HDD 没有 scheduler 时候的性能是什么样的

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
