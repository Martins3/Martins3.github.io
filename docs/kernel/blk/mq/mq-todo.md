## [ ] blk_mq_queue_tag_busy_iter 的逻辑好奇怪啊

1. blk_mq_is_shared_tags(q->tag_set->flags) 来判断，如果是，那么使用 blk_mq_tag_set::shared_tags
  - [ ] blk_mq_tag_set::tags 什么时候使用

2. struct request_queue {

	struct blk_mq_tag_set	*tag_set;


## 这两个文件的关系是什么?
- block/elevator.c : 实现的公共文件
- block/blk-mq-sched.c : 的作用又是什么?

## 当 finish plug 之后，代码是如何流动的
显然，这就是 finish plug 的时候，让 io 从 plug 中向下移动

finish plug 是最开始吧 ?

finish plug 是 bio 还是 request ?

## blk_mq_alloc_request 和 blk_mq_get_new_requests 是什么关系? 他们都会调用 `__blk_mq_alloc_requests`

## blk_mq_get_driver_tag 是什么意思?

## 分析下 blk_mq_start_request 中的
```c
	rq->mq_hctx->tags->rqs[rq->tag] = rq;
```

```txt
@[
    blk_mq_start_request+5
    nvme_prep_rq.part.0+934
    nvme_queue_rq+124
    __blk_mq_issue_directly+72
    blk_mq_try_issue_directly+137
    blk_mq_submit_bio+1456
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
]: 518490
```

## [ ] 提交过程中，其他的组件是如何穿插进来的


## [ ] 如何理解这里的 io_schedule 的含义
```txt
@[
    blk_mq_run_hw_queue+5
    blk_mq_flush_plug_list.part.0+1368
    __blk_flush_plug+245
    io_schedule+65
    rq_qos_wait+192
    wbt_wait+166
    __rq_qos_throttle+36
    blk_mq_submit_bio+396
    submit_bio_noacct_nocheck+653
    ext4_bio_write_folio+371
    mpage_submit_folio+97
    mpage_process_page_bufs+300
    mpage_prepare_extent_to_map+944
    ext4_do_writepages+572
    ext4_writepages+188
    do_writepages+204
    __writeback_single_inode+61
    writeback_sb_inodes+501
    __writeback_inodes_wb+76
    wb_writeback+664
    wb_workfn+686
    process_one_work+371
    worker_thread+635
    kthread+227
    ret_from_fork+49
    ret_from_fork_asm+27
]: 7
```


## [ ] 什么时候会加入到软件队列中 ?


### 是如何使用软件队列的

- `blk_mq_ctx` : 这个写的比较简单

```c
struct request_queue {
	/* sw queues */
	struct blk_mq_ctx __percpu	*queue_ctx;
```

```c
static inline struct blk_mq_ctx *__blk_mq_get_ctx(struct request_queue *q,
					   unsigned int cpu)
{
	return per_cpu_ptr(q->queue_ctx, cpu);
}
```

## 如果一个很慢的盘，从 none 修改为 mq-deadline, 性能发生了下降

## 如果 deadline 的 schedule line 到底是保证的那个 deadline

bio 级别的 deadline，
保证的是进程级别 deadline ，
还是 partion 级别的 deadline  ?

## 到底什么是 blk_rq_is_passthrough
```c
static inline bool blk_rq_is_passthrough(struct request *rq)
{
	return blk_op_is_passthrough(rq->cmd_flags);
}
```
## request_queue::tag_set 和 request_queue::hctx_table 指向的内容是重复的

因为硬件队列是共享的
```c
struct request_queue {

	/* hw dispatch queues */
	unsigned int		nr_hw_queues;
	struct xarray		hctx_table;

	struct blk_mq_tag_set	*tag_set;
	struct list_head	tag_set_list;
```

## blk_mq_ops::get_budget

只有 scsi 才会使用 get_budget

分析 scsi 的实现:
- scsi_mq_get_budget
  - scsi_dev_queue_ready : 从 scsi_device::budget_map 中找到一个 bit ，如果找不到，意味着 budget 用完

分析 blk mq 的使用:

主要参考 blk_mq_get_budget_and_tag

- [ ] 问题是，既然存在 tag ，为什么还需要使用 budget 啊

## 是如何提交到软件队列中去的
- 为什么 request_queue::queue_ctx 来做什么

## [ ] 似乎这种 writeback 的机制的最后都是走的 plug ?

## 理解下这个细节
https://lore.kernel.org/all/20220522122350.743103-1-ming.lei@redhat.com/


## 如何理解 blk_mq_queue_tag_busy_iter

```c
static bool blk_mq_check_inflight(struct request *rq, void *priv)
{
	struct mq_inflight *mi = priv;

	if (rq->part && blk_do_io_stat(rq) &&
	    (!mi->part->bd_partno || rq->part == mi->part) &&
	    blk_mq_rq_state(rq) == MQ_RQ_IN_FLIGHT)
		mi->inflight[rq_data_dir(rq)]++;

	return true;
}

void blk_mq_in_flight_rw(struct request_queue *q, struct block_device *part,
		unsigned int inflight[2])
{
	struct mq_inflight mi = { .part = part };

	blk_mq_queue_tag_busy_iter(q, blk_mq_check_inflight, &mi);
	inflight[0] = mi.inflight[0];
	inflight[1] = mi.inflight[1];
}
```

每一个 parttion 都有自己的 inflight

blk_mq_queue_tag_busy_iter : 根据 queue_for_each_hw_ctx 找到所有的 hardware queue ，
然后根据 hctx 分析每一个 request 。

- [ ] 无法理解，为什么 blk_mq_queue_tag_busy_iter 只是遍历 hardware queue 就可以了

- blk_mq_tagset_busy_iter : 将一个 tagset 中的所有的 tag 遍历一次
- blk_mq_all_tag_iter : 给定一个 blk_mq_tags, 对其中进行遍历。

## request 的生命周期

blk_mq_submit_bio

### 从 sw queue 到 hw queue 的过程

blk_mq_hw_queue_mapped

```c
struct blk_mq_hw_ctx {
	struct {
		/** @lock: Protects the dispatch list. */
		spinlock_t		lock;
		/**
		 * @dispatch: Used for requests that are ready to be
		 * dispatched to the hardware but for some reason (e.g. lack of
		 * resources) could not be sent to the hardware. As soon as the
		 * driver can send new requests, requests at this list will
		 * be sent first for a fairer dispatch.
		 */
		struct list_head	dispatch;
		 /**
		  * @state: BLK_MQ_S_* flags. Defines the state of the hw
		  * queue (active, scheduled to restart, stopped).
		  */
		unsigned long		state;
	} ____cacheline_aligned_in_smp;
```

blk_mq_request_bypass_insert 将 request_queue 放到 dispatch 中，

似乎总是直接提交的，如果应为种种原因无法直接提交给硬件，那么就会放到 dispatch 中。

从 sw queue 中离开的位置:

__blk_mq_sched_dispatch_requests -> blk_mq_do_dispatch_ctx -> blk_mq_dequeue_from_ctx -> dispatch_rq_from_ctx

看来，似乎，如果没有 scheduler ，那么软件队列几乎不会怎么使用。

## 之前是通过 can_queue 可以实现压制 io 的数量的

megasas_reset_timer 中修改了 can_queue 之后
```c
/**
 * megasas_reset_timer - quiesce the adapter if required
 * @scmd:		scsi cmnd
 *
 * Sets the FW busy flag and reduces the host->can_queue if the
 * cmd has not been completed within the timeout period.
 */
static enum scsi_timeout_action megasas_reset_timer(struct scsi_cmnd *scmd)
{
	struct megasas_instance *instance;
	unsigned long flags;

	if (time_after(jiffies, scmd->jiffies_at_alloc +
				(scmd_timeout * 2) * HZ)) {
		return SCSI_EH_NOT_HANDLED;
	}

	instance = (struct megasas_instance *)scmd->device->host->hostdata;
	if (!(instance->flag & MEGASAS_FW_BUSY)) {
		/* FW is busy, throttle IO */
		spin_lock_irqsave(instance->host->host_lock, flags);

		instance->host->can_queue = instance->throttlequeuedepth;
		instance->last_time = jiffies;
		instance->flag |= MEGASAS_FW_BUSY;

		spin_unlock_irqrestore(instance->host->host_lock, flags);
	}
	return SCSI_EH_RESET_TIMER;
}
```

在 4.18 中 scsi_host_queue_ready 中的确会检查，但是主线不会

之后，又是 megaraid 来进行 can_queue 的恢复

(感觉似乎主线中，利用 can_queue 来阻塞的代码没有了，但是 megaraid 还是在利用这个特定

## 如何理解 requeue
例如在 scsi 中

- scsi_queue_insert
  - `__scsi_queue_insert`

blkcg_policy_unregister


## 整理下，除了 host_bloced 还有 device_blocked 之类


```txt
History:        #0
Commit:         cd9070c9c512ff7995f9019392e0ae548df3a088
Author:         Christoph Hellwig <hch@lst.de>
Author Date:    Thu 23 Jan 2014 07:07:41 PM CST
Committer Date: Sat 26 Jul 2014 05:15:48 AM CST

scsi: fix the {host,target,device}_blocked counter mess

Seems like these counters are missing any sort of synchronization for
updates, as a over 10 year old comment from me noted.  Fix this by
using atomic counters, and while we're at it also make sure they are
in the same cacheline as the _busy counters and not needlessly stored
to in every I/O completion.

With the new model the _busy counters can temporarily go negative,
so all the readers are updated to check for > 0 values.  Longer
term every successful I/O completion will reset the counters to zero,
so the temporarily negative values will not cause any harm.

Signed-off-by: Christoph Hellwig <hch@lst.de>
Reviewed-by: Martin K. Petersen <martin.petersen@oracle.com>
Reviewed-by: Webb Scales <webbnh@hp.com>
Acked-by: Jens Axboe <axboe@kernel.dk>
Tested-by: Bart Van Assche <bvanassche@acm.org>
Tested-by: Robert Elliott <elliott@hp.com>
```

而且似乎 host_bloced 和 device_blocked 路径特别的相似

## mddev 的 tag_set 和 queue 是分开的
```txt
	err = blk_mq_init_allocated_queue(tag_set, queue);
	if (err)
		goto out_tag_set;

```

## mddev 和 block_device 联系起来的
```c
	struct mddev *mddev = bio->bi_bdev->bd_disk->private_data;
```

## 问题总结

1. request_queue 到底位置在那里，和软件队列和硬件队列的关系是什么?
一个 request_queue 对应一个什么？一个 HBA 卡？ 一个盘 ？ 一个软件队列 ? 还是一个硬件队列。

2. 什么时候进入软件队列，什么时候进入到硬件队列中?


3. [ ] struct blk_mq_tag_set 中的 struct blk_mq_tags	**tags; 到底是指向的谁 ?

4. [ ] scsi_log 中记录的 tag 编号是从那里赋值的

5. elv 机制和 multiqueue 是什么关系

6. scheduler 在什么位置起作用

7. tags 是如何和 cgroup 之类的耦合到一起的?

8. 是不是一个 tag 对应一个 request ?
9. 如果将两个 request 合并之后，是不是自动释放一个 tag 和 request
  - 应该是只有 bio 合并，没有 request 合并吧

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
