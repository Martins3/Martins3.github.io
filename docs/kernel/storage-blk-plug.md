# storage blk plug 机制

```c
/*
 * blk_plug permits building a queue of related requests by holding the I/O
 * fragments for a short period. This allows merging of sequential requests
 * into single larger request. As the requests are moved from a per-task list to
 * the device's request_queue in a batch, this results in improved scalability
 * as the lock contention for request_queue lock is reduced.
 *
 * It is ok not to disable preemption when adding the request to the plug list
 * or when attempting a merge. For details, please see schedule() where
 * blk_flush_plug() is called.
 */
struct blk_plug {
	struct request *mq_list; /* blk-mq requests */ // 一个

	/* if ios_left is > 1, we can batch tag/rq allocations */
	struct request *cached_rq;
	unsigned short nr_ios;

	unsigned short rq_count;

	bool multiple_queues;
	bool has_elevator;
	bool nowait;

	struct list_head cb_list; /* md requires an unplug callback */
};
```
- 为什么可以不用关闭抢占?
- request_queue 的锁是如何获取的？


> it is used for I/O request batching and merging to improve I/O performance.

- 在 nvme 场景中，这个还有意义吗
  - 会使用的
- 和 io scheduler 是什么关系？
  - 在 io_scheduler 的前面，用于聚合 io


## 结束
- blk_finish_plug
  - `__blk_flush_plug`
    - flush_plug_callbacks : 执行 callback
    - blk_mq_flush_plug_list
      - blk_mq_dispatch_plug_list
        - blk_mq_sched_insert_requests
          - blk_mq_try_issue_list_directly
          - blk_mq_insert_requests
          - blk_mq_run_hw_queue
    - blk_mq_free_plug_rqs

## 提交
- blk_mq_submit_bio
  - blk_mq_plug : 从 current 中获取 plug
  - blk_add_rq_to_plug : 将 request 放到 rq 中
  - blk_mq_try_issue_directly : 直接提交了

增加到 plug 中例子 ：
```txt
blk_add_rq_to_plug+1
blk_mq_submit_bio+853
submit_bio_noacct_nocheck+607
ext4_bio_write_page+484
mpage_submit_page+76
mpage_process_page_bufs+279
mpage_prepare_extent_to_map+512
ext4_do_writepages+673
ext4_writepages+161
do_writepages+208
__writeback_single_inode+65
writeback_sb_inodes+521
__writeback_inodes_wb+76
wb_writeback+471
wb_workfn+675
process_one_work+482
worker_thread+84
kthread+233
ret_from_fork+41
```
直接提交的例子:
```txt
nvme_queue_rq+5
__blk_mq_try_issue_directly+334
blk_mq_try_issue_directly+23
blk_mq_submit_bio+1194
submit_bio_noacct_nocheck+607
ext4_read_bh_lock+61
ext4_block_write_begin+725
ext4_da_write_begin+237
generic_perform_write+198
ext4_buffered_write_iter+131
vfs_write+706
ksys_write+99
do_syscall_64+60
entry_SYSCALL_64_after_hwframe+114
```

## 基本使用方法
`struct blk_plug`

- [ ] 但是  总是放在 stack 的，被回收了怎么办？

```c
/**
 * blk_start_plug - initialize blk_plug and track it inside the task_struct
 * @plug:	The &struct blk_plug that needs to be initialized
 *
 * Description:
 *   blk_start_plug() indicates to the block layer an intent by the caller
 *   to submit multiple I/O requests in a batch.  The block layer may use
 *   this hint to defer submitting I/Os from the caller until blk_finish_plug()
 *   is called.  However, the block layer may choose to submit requests
 *   before a call to blk_finish_plug() if the number of queued I/Os
 *   exceeds %BLK_MAX_REQUEST_COUNT, or if the size of the I/O is larger than
 *   %BLK_PLUG_FLUSH_SIZE.  The queued I/Os may also be submitted early if
 *   the task schedules (see below).
 *
 *   Tracking blk_plug inside the task_struct will help with auto-flushing the
 *   pending I/O should the task end up blocking between blk_start_plug() and
 *   blk_finish_plug(). This is important from a performance perspective, but
 *   also ensures that we don't deadlock. For instance, if the task is blocking
 *   for a memory allocation, memory reclaim could end up wanting to free a
 *   page belonging to that request that is currently residing in our private
 *   plug. By flushing the pending I/O when the process goes to sleep, we avoid
 *   this kind of deadlock.
 */
void blk_start_plug(struct blk_plug *plug)
{
	blk_start_plug_nr_ios(plug, 1);
}
EXPORT_SYMBOL(blk_start_plug);
```
总而言之，总体来说是在 blk_finish_plug 的时候 flush 的，但是一下情况是可以提前 flush 的:
- BLK_MAX_REQUEST_COUNT
- BLK_PLUG_FLUSH_SIZE
- 被调度走的时候

使用 read_pages 作为例子:
- struct blk_plug plug; : 在 stack 上分配
- blk_start_plug(&plug); : 初始化
- while ((folio = readahead_folio(rac)) != NULL) : 多次提交 io
- blk_finish_plug(&plug); : 真正的提交

关于被调度的时候，finish plug 的内容
- io_schedule
  - io_schedule_prepare
    - blk_flush_plug

- schedule
  - sched_submit_work
    - blk_flush_plug


## [ ] https://lwn.net/Articles/438256/
