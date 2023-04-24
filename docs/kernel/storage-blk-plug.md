# storage blk plug 机制

## 基本原理
使用 read_pages 作为例子:
- struct blk_plug plug; : 在 stack 上分配
- blk_start_plug(&plug); : 初始化
- while ((folio = readahead_folio(rac)) != NULL) : 多次提交 io
- blk_finish_plug(&plug); : 真正的提交


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

- [ ] request_queue 的锁是如何获取的？


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

使用 plug 中例子
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


## 什么时候真正的提交

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

关于被调度的时候，finish plug 的内容
- io_schedule
  - io_schedule_prepare
    - blk_flush_plug

- schedule
  - sched_submit_work
    - blk_flush_plug

## plug hook
主要的使用者都是几个 raid :

在 `raid1_write_request` 中调用 `blk_check_plugged` 来注册
```txt
#0  raid1_write_request (max_write_sectors=8, bio=0xffff888112c2c700, mddev=0xffff888100326000) at drivers/md/raid1.c:1356
#1  raid1_make_request (mddev=0xffff888100326000, bio=0xffff888112c2c700) at drivers/md/raid1.c:1660
#2  0xffffffff81e22962 in md_handle_request (mddev=0xffff888100326000, bio=0xffff888112c2c700) at drivers/md/md.c:431
#3  0xffffffff8173e7e6 in __submit_bio (bio=bio@entry=0xffff888112c2c700) at block/blk-core.c:604
#4  0xffffffff8173ecaf in __submit_bio_noacct (bio=0xffff888112c2c700) at block/blk-core.c:647
#5  submit_bio_noacct_nocheck (bio=<optimized out>) at block/blk-core.c:710
#6  0xffffffff8173f060 in submit_bio_noacct (bio=<optimized out>) at block/blk-core.c:807
#7  0xffffffff81472d4e in submit_bh_wbc (opf=<optimized out>, opf@entry=1048577, bh=<optimized out>, wbc=wbc@entry=0xffffc90000057ca0) at fs/buffer.c:2750
#8  0xffffffff81475a18 in __block_write_full_page (inode=<optimized out>, page=0xffffea0004681340, get_block=0xffffffff81735f30 <blkdev_get_block>, wbc=0xffffc90000057ca0, handler=0xffffffff814736f0 <end_buffer_async_write>) at fs/buffer.c:1835
#9  0xffffffff8133aa9b in writepage_cb (folio=0xffffffff822a86d1 <_raw_spin_unlock_irqrestore+49>, wbc=0x0 <fixed_percpu_data>, data=0x0 <fixed_percpu_data>) at mm/page-writeback.c:2535
#10 0xffffffff8133bb3c in write_cache_pages (mapping=mapping@entry=0xffff888100480b08, wbc=wbc@entry=0xffffc90000057ca0, writepage=writepage@entry=0xffffffff8133aa80 <writepage_cb>, data=data@entry=0xffff888100480b08) at mm/page-writeback.c:2473
#11 0xffffffff8133e311 in do_writepages (mapping=mapping@entry=0xffff888100480b08, wbc=wbc@entry=0xffffc90000057ca0) at mm/page-writeback.c:2556
#12 0xffffffff814636d1 in __writeback_single_inode (inode=inode@entry=0xffff888100480990, wbc=wbc@entry=0xffffc90000057ca0) at fs/fs-writeback.c:1603
#13 0xffffffff81463ea9 in writeback_sb_inodes (sb=sb@entry=0xffff888100161800, wb=wb@entry=0xffff88810712cc00, work=work@entry=0xffffc90000057e30) at fs/fs-writeback.c:1894
#14 0xffffffff8146419c in __writeback_inodes_wb (wb=wb@entry=0xffff88810712cc00, work=work@entry=0xffffc90000057e30) at fs/fs-writeback.c:1965
#15 0xffffffff81464417 in wb_writeback (wb=wb@entry=0xffff88810712cc00, work=work@entry=0xffffc90000057e30) at fs/fs-writeback.c:2070
#16 0xffffffff8146581e in wb_check_background_flush (wb=0xffff88810712cc00) at fs/fs-writeback.c:2136
#17 wb_do_writeback (wb=0xffff88810712cc00) at fs/fs-writeback.c:2224
#18 wb_workfn (work=0xffff88810712cd88) at fs/fs-writeback.c:2251
#19 0xffffffff81165899 in process_one_work (worker=worker@entry=0xffff88810082e0c0, work=0xffff88810712cd88) at kernel/workqueue.c:2390
#20 0xffffffff81165ea1 in worker_thread (__worker=0xffff88810082e0c0) at kernel/workqueue.c:2537
#21 0xffffffff8116eb49 in kthread (_create=0xffff8881003e7480) at kernel/kthread.c:376
#22 0xffffffff81002939 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

```txt
flush_bio_list at ffffffffc03f3f28 [raid1]
raid1_unplug at ffffffffc03f48fc [raid1]
blk_flush_plug_list at ffffffffac36f297
blk_finish_plug at ffffffffac36f8a4
ext4_writepages at ffffffffc076946f [ext4]
do_writepages at ffffffffac1d8a38
__filemap_fdatawrite_range at ffffffffac1cbcf3
filemap_write_and_wait_range at ffffffffac1cbe61
ext4_sync_file at ffffffffc075fcf1 [ext4]
vfs_fsync at ffffffffac29683e
loop_thread at ffffffffc09de3cc [loop]
kthread at ffffffffac0cc871
```

## [ ] https://lwn.net/Articles/438256/
