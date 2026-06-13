# storage blk plug 机制

## 基本原理
使用 read_pages 作为例子:
- struct blk_plug plug; : 在 stack 上分配一个 plug ，并且 task_struct::plug
- blk_start_plug(&plug); : 初始化
- while ((folio = readahead_folio(rac)) != NULL) : 多次提交 io ，但是 io 都是挂到
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

## 开始 plug

## 结束 plug
blk_execute_rq_nowait / blk_mq_submit_bio => blk_add_rq_to_plug --> blk_mq_flush_plug_list

blk_finish_plug / blk_flush_plug => __blk_flush_plug  --> blk_mq_flush_plug_list

- blk_mq_flush_plug_list
  - blk_mq_dispatch_plug_list
    - blk_mq_sched_insert_requests
      - blk_mq_try_issue_list_directly
      - blk_mq_insert_requests
      - blk_mq_run_hw_queue
- blk_mq_free_plug_rqs

## blk_flush_plug 做什么的，和 blk_finish_plug 的关系
blk_finish_plug 和 blk_start_plug 是匹配的。

而 blk_flush_plug 看调用位置，都是和 writeback_sb_inodes / wakeup_flusher_threads
bio_poll
sched_submit_work
io_schedule_prepare

blk_finish_plug 会将这次的 current->plug 重新设置为 NULL, 但是 flush 并不会，看来 finish 做的事情更加充分。

> [!NOTE]
> 参考 Deepseeek ，有待验证

强制将当前插头中的所有积压请求提交出去，但不结束插头状态。
它会触发 unplug 操作，把 plug 中的请求提交，但 保留 plug 结构，允许后续继续积压新的请求。

<!-- ds end -->

算是很有道理了

## block layer plug 机制的原理
<!-- 0f17a8fa-b5fc-4ede-8603-a767fad55d06 -->

1. plug 机制的使用都是显示的操作的，例如在 aio 中，由于这一次本来就是需要提交多次
io ，那么就是先全部都提交合并，然后在 blk_finish_plug 中向下提交给硬件:
```c
	if (nr > AIO_PLUG_THRESHOLD)
		blk_start_plug(&plug);
	for (i = 0; i < nr; i++) {
		struct iocb __user *user_iocb;

		if (unlikely(get_user(user_iocb, iocbpp + i))) {
			ret = -EFAULT;
			break;
		}

		ret = io_submit_one(ctx, user_iocb, false);
		if (ret)
			break;
	}
	if (nr > AIO_PLUG_THRESHOLD)
		blk_finish_plug(&plug);
```

再例如:
```c
static int madvise_do_behavior(struct mm_struct *mm,
		unsigned long start, size_t len_in,
		struct madvise_behavior *madv_behavior)
{
	int behavior = madv_behavior->behavior;
	struct blk_plug plug;
	unsigned long end;
	int error;

	if (is_memory_failure(behavior))
		return madvise_inject_error(behavior, start, start + len_in);
	start = untagged_addr_remote(mm, start);
	end = start + PAGE_ALIGN(len_in);

	blk_start_plug(&plug);
	if (is_madvise_populate(behavior))
		error = madvise_populate(mm, start, end, behavior);
	else
		error = madvise_walk_vmas(mm, start, end, madv_behavior,
					  madvise_vma_behavior);
	blk_finish_plug(&plug);
	return error;
}
```



## 提交
- blk_mq_submit_bio
  - blk_mq_plug : 从 current 中获取 plug
  - blk_mq_get_cached_request : 如果有 cached 的 request ，那么使用其
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

### blk_mq_try_issue_list_directly 似乎现在不调用了?

之前记录一下是这样的:
- blk_mq_submit_bio
  - blk_mq_try_issue_directly

观察发现:

blk_mq_dispatch_plug_list 和 都不会调用
blk_mq_insert_requests 中:
```c
	if (!hctx->dispatch_busy && !run_queue_async) {
		blk_mq_run_dispatch_ops(hctx->queue,
			blk_mq_try_issue_list_directly(hctx, list));
		if (list_empty(list))
			goto out;
	}
```

```txt
- 26.58% __submit_bio
  - 17.28% __blk_flush_plug
    - 17.10% blk_mq_flush_plug_list
      - 16.08% nvme_queue_rqs
```

似乎走的完全不是一个路径啊

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


## 当使用 fio 测试磁盘的时候，发现 plug 机制几乎不会触发，不知道当时走的是什么路径
```txt
blk_mq_flush_plug_list+9
blk_add_rq_to_plug+69
blk_mq_submit_bio+853
submit_bio_noacct_nocheck+834
ext4_io_submit+36
ext4_do_writepages+1318
ext4_writepages+134
do_writepages+208
__writeback_single_inode+65
writeback_sb_inodes+521
__writeback_inodes_wb+76
wb_writeback+471
wb_workfn+608
process_one_work+482
worker_thread+84
kthread+218
ret_from_fork+41
```

## 如何理解 raid1 中的 plug

- raid1d
	- blk_start_plug(&plug);
	- submit_bio_noacct(wbio);
	- blk_finish_plug(&plug);


## 才意识到 plug 机制一直是延伸到 mq 的
blk_mq_dispatch_plug_list

中居然是会存在

```txt
		queue->elevator->type->ops.insert_requests(this_hctx,
```

另外一个调用 ops.insert_requests 的位置是 blk_mq_insert_request

## 深入分析 blk_mq_submit_bio，这里就是对于 plug 进行汇集的地方

`__submit_bio` 是 blk_mq_submit_bio 调用的唯一入口，这里就是 bio 层次到达 mq 层次的位置

- blk_mq_submit_bio
  - blk_queue_bounce : bounce 机制，
  - blk_mq_get_cached_request : 看看 plug 机制下是否已经缓存
  - blk_mq_get_new_requests
    - __blk_mq_alloc_requests : 创建一个新的 request，在这里分配一个 tag 出来

如果想要汇集

## 观察一个有趣的 plug 问题

当配置让 raid 进行 resync 的时候:
```sh
echo check > /sys/block/md127/md/sync_action
echo 20000000 > /proc/sys/dev/raid/speed_limit_max
```

cat /proc/mdstat
```txt
Personalities : [raid0] [raid1] [raid10] [raid6] [raid5] [raid4]
md126 : active (auto-read-only) raid1 vdb1[1] vda1[0]
      471726080 blocks super 1.2 [2/2] [UU]
        resync=PENDING
      bitmap: 2/4 pages [8KB], 65536KB chunk

md127 : active raid1 vda2[0] vdb2[1]
      1100871680 blocks super 1.2 [2/2] [UU]
      [=>...................]  check =  7.1% (78346880/1100871680) finish=4.9min speed=3406386K/sec
      bitmap: 0/9 pages [0KB], 65536KB chunk

unused devices: <none>
```

```txt
     ret_from_fork_asm
     ret_from_fork
     kthread
     md_thread
   - md_do_sync
      - 98.90% raid1_sync_request
         - 62.43% mempool_alloc
            - r1buf_pool_alloc
               - 52.50% alloc_pages_mpol
                  - 49.94% __alloc_pages
                     - 45.58% get_page_from_freelist
                        - 12.72% __rmqueue_pcplist
                             _raw_spin_unlock_irqrestore
                        - 12.12% _raw_spin_unlock
                             11.24% __raw_callee_save___pv_queued_spin_unlock
                             0.59% preempt_count_sub
                        - 7.45% _raw_spin_trylock
                             preempt_count_add
                        - 2.34% preempt_count_sub
                             0.67% tracer_preempt_on
                        - 1.56% preempt_count_add
                             in_lock_functions
                          0.83% tracer_preempt_off
                          0.77% prep_new_page
                    1.01% policy_nodemask
               - 8.12% __kmalloc
                  - 3.30% ___slab_alloc
                     - 0.64% get_partial_node
                          _raw_spin_unlock_irqrestore
                  - 0.84% should_failslab
                       0.58% should_fail_ex
                 0.96% alloc_pages
         - 11.48% schedule
            - 8.04% __blk_flush_plug
               - blk_mq_flush_plug_list
                  - 6.30% kblockd_mod_delayed_work_on
                       mod_delayed_work_on
            - 3.20% __schedule
                 finish_task_switch
           10.24% _raw_spin_unlock_irq
         - 7.20% submit_bio_noacct_nocheck
            - 6.47% blk_mq_submit_bio
               - 2.57% __blk_mq_alloc_requests
                    1.58% blk_mq_rq_ctx_init
                  - 0.86% blk_mq_get_tag
                     - 0.65% sbitmap_get
                          sbitmap_find_bit
               - 2.03% __bio_split_to_limits
                    bio_split_rw
         - 1.65% md_bitmap_start_sync
              1.50% _raw_spin_unlock_irq
           0.68% __bio_add_page
```
这里有两个有趣的事情:
1. 在虚拟机中，应该是同时对于两个盘进行 io ，但是可以达到 3.4G 的速度，其他的方法可以达到吗?
  - 要么是 raid1 的 resync 的速度计算存在问题，要么是因为没有 syscall 的原因，更短的提交路径?
2. resync thread 的居然 60% 的时间在内存分配上，真的有趣

3. 关于 plug : 在 md_do_sync 的位置，进行了一个 start plug
```c
	blk_start_plug(&plug);
	while (j < max_sectors) {
  // ...
	sectors = mddev->pers->sync_request(mddev, j, &skipped);

  }
```
之后，直到这个 while 循环结束的时候，才有一次 finish plug ，所以 plug 是做啥的来着 ?

而且看上去 plug 的位置是:
```txt
- 11.48% schedule
   - 8.04% __blk_flush_plug
      - blk_mq_flush_plug_list
         - 6.30% kblockd_mod_delayed_work_on
              mod_delayed_work_on
```

## plug 是将收集的 request 一并插入到 blk_mq_dispatch_plug_list 中

## raid 中的 bitmap 也是有 plug 机制的
```txt
root@bogon 15:30:11 ~]$ cat /proc/5/stack
[<ffffffff8afac5cd>] md_super_wait+0x6d/0xd0
[<ffffffff8afb2e11>] bitmap_wait_writes+0xc1/0xd0
[<ffffffff8afb452c>] bitmap_unplug+0xec/0x120
[<ffffffffc02db431>] flush_bio_list+0x21/0xd0 [raid1]
[<ffffffffc02dba7c>] raid1_unplug+0xdc/0xf0 [raid1]
[<ffffffff8ad57117>] blk_flush_plug_list+0xa7/0x230
[<ffffffff8ad576b4>] blk_finish_plug+0x14/0x40
[<ffffffffc04c8f15>] ext4_writepages+0x515/0xcf0 [ext4]
[<ffffffff8abcb051>] do_writepages+0x21/0x50
[<ffffffff8ac7d970>] __writeback_single_inode+0x40/0x260
[<ffffffff8ac7e4f4>] writeback_sb_inodes+0x1c4/0x430
[<ffffffff8ac7e7ff>] __writeback_inodes_wb+0x9f/0xd0
[<ffffffff8ac7ecd3>] wb_writeback+0x263/0x2f0
[<ffffffff8ac7f8cb>] bdi_writeback_workfn+0x2cb/0x460
[<ffffffff8aabdf1f>] process_one_work+0x17f/0x440
[<ffffffff8aabf036>] worker_thread+0x126/0x3c0
[<ffffffff8aac5ef1>] kthread+0xd1/0xe0
[<ffffffff8b194ddd>] ret_from_fork_nospec_begin+0x7/0x21
[<ffffffffffffffff>] 0xffffffffffffffff
```

### 而且为什么在 read / write plug 的时候，去唤醒 md_wakeup_thread

```txt
@[
        md_wakeup_thread+0
        end_sync_read+152
        bio_endio+388
        blk_mq_end_request_batch+608
        nvme_pci_complete_batch+100
        nvme_irq+128
        __handle_irq_event_percpu+148
        handle_irq_event+84
        handle_fasteoi_irq+168
        handle_irq_desc+60
        generic_handle_domain_irq+36
        gic_handle_irq+84
        call_on_irq_stack+36
        do_interrupt_handler+136
        el1_interrupt+52
        el1h_64_irq_handler+24
        el1h_64_irq+108
        __pi_memcmp+116
        md_thread+196
        kthread+316
        ret_from_fork+16
]: 184
```

> [!NOTE]
> 参考 Deepseeek ，有待验证


首先，我们需要区分新旧两种 plug 机制：

旧的单队列 plug (为 HDD 设计)：在过去，plug 的主要目的是为了排序。内核会暂时“塞住”队列，阻止 I/O 请求被发送到磁盘。这给了 I/O 调度器（如电梯算法）一个时间窗口来收集更多的请求，然后对它们进行排序，以最小化机械硬盘的磁头寻道时间。这对于 HDD 来说是至关重要的性能优化。

现代 blk-mq 的 plug (为 SSD/NVMe 优化)：对于没有寻道时间的 SSD 和 NVMe 来说，排序已经没有意义。因此，blk-mq 中的 plug 机制的唯一目的就是为了合并 (Merging)。

工作方式：plug 是一种per-CPU的机制。当一个进程在某个 CPU 核上提交第一个 bio 时，它可以在该核的软件队列上启动一个 plug。这个 plug 会被“塞住”一个极短的时间。如果这个进程立即提交了更多物理上相邻的 bio，这些 bio 就会被合并成一个单一的、更大的 request。

自动释放：这个 plug 会在很短的时间后（例如，当前系统调用结束时）自动被释放（unplugged），然后合并后的 request 会被发送到硬件队列。

<!-- ds 结束 -->

算是非常正确了，在现在的机器中，主要考虑的是合并请求。

两个关联的 tracepoint
1. block_unplug

```txt
312942.012 jbd2/dm-2-8/6328 block:block_unplug(nr_rq: 1, comm: "jbd2/dm-2-8")
312942.047 jbd2/dm-2-8/6328 block:block_unplug(nr_rq: 1, comm: "jbd2/dm-2-8")
312942.140 qemu-system-aa/1261844 block:block_unplug(nr_rq: 1, comm: "qemu-system-aar")
312942.337 qemu-system-aa/1261844 block:block_unplug(nr_rq: 1, comm: "qemu-system-aar")
312942.515 qemu-system-aa/1261844 block:block_unplug(nr_rq: 1, comm: "qemu-system-aar")
312942.689 jbd2/dm-2-8/6328 block:block_unplug(nr_rq: 1, comm: "jbd2/dm-2-8")
312942.719 jbd2/dm-2-8/6328 block:block_unplug(nr_rq: 1, comm: "jbd2/dm-2-8")
```
2. block_plug

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
