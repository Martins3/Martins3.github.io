# blk-core.c

## TODO
1. 希望搞清楚 bio request 和 request_queue 的关系

## 对于 queue 的各种操作
1. 从 280 行之后的一段位置，前面的内容并不知道是干什么的

```c
/**
 * blk_sync_queue - cancel any pending callbacks on a queue
 * @q: the queue
 *
 * Description:
 *     The block layer may perform asynchronous callback activity
 *     on a queue, such as calling the unplug function after a timeout.
 *     A block device may call blk_sync_queue to ensure that any
 *     such activity is cancelled, thus allowing it to release resources
 *     that the callbacks might use. The caller must already have made sure
 *     that its ->make_request_fn will not re-add plugging prior to calling
 *     this function.
 *
 *     This function does not cancel any asynchronous activity arising
 *     out of elevator or throttling code. That would require elevator_exit()
 *     and blkcg_exit_queue() to be called with queue lock initialized.
 *
 */
void blk_sync_queue(struct request_queue *q)
{
	del_timer_sync(&q->timeout);
	cancel_work_sync(&q->timeout_work);
}
```

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
1. 我是万万没有想到 : 居然我们可以使用kobject 管理 request_queue
2. 这些调用位置也是异常的神奇。


```c
/**
 * blk_cleanup_queue - shutdown a request queue
 * @q: request queue to shutdown
 *
 * Mark @q DYING, drain all pending requests, mark @q DEAD, destroy and
 * put it.  All future requests will be failed immediately with -ENODEV.
 */
void blk_cleanup_queue(struct request_queue *q)
```

```c
struct request_queue *blk_alloc_queue(gfp_t gfp_mask)  // 对于简单的封装 driver md 是唯一调用者
struct request_queue *blk_mq_init_queue(struct blk_mq_tag_set *set) // mq 是其唯一调用者
      /**
       * blk_alloc_queue_node - allocate a request queue
       * @gfp_mask: memory allocation flags
       * @node_id: NUMA node to allocate memory from
       */
      struct request_queue *blk_alloc_queue_node(gfp_t gfp_mask, int node_id) // 唯一创建 request_queue 的函数
```


## generic_make_request
1. 居然可能被递归调用
2. 核心工作在 : make_request_fn 上

```c
/**
 * generic_make_request - hand a buffer to its device driver for I/O
 * @bio:  The bio describing the location in memory and on the device.
 *
 * generic_make_request() is used to make I/O requests of block
 * devices. It is passed a &struct bio, which describes the I/O that needs
 * to be done.
 *
 * generic_make_request() does not return any status.  The
 * success/failure status of the request, along with notification of
 * completion, is delivered asynchronously through the bio->bi_end_io
 * function described (one day) else where.
 *
 * The caller of generic_make_request must make sure that bi_io_vec
 * are set to describe the memory buffer, and that bi_dev and bi_sector are
 * set to describe the device address, and the
 * bi_end_io and optionally bi_private are set to describe how
 * completion notification should be signaled.
 *
 * generic_make_request and the drivers it calls may use bi_next if this
 * bio happens to be merged with someone else, and may resubmit the bio to
 * a lower device by calling into generic_make_request recursively, which
 * means the bio should NOT be touched after the call to ->make_request_fn.
 */
blk_qc_t generic_make_request(struct bio *bio)
{
	/*
	 * bio_list_on_stack[0] contains bios submitted by the current
	 * make_request_fn.
	 * bio_list_on_stack[1] contains bios that were submitted before
	 * the current make_request_fn, but that haven't been processed
	 * yet.
	 */
	struct bio_list bio_list_on_stack[2];
	blk_qc_t ret = BLK_QC_T_NONE;

	if (!generic_make_request_checks(bio))
		goto out;

	/*
	 * We only want one ->make_request_fn to be active at a time, else
	 * stack usage with stacked devices could be a problem.  So use
	 * current->bio_list to keep a list of requests submited by a
	 * make_request_fn function.  current->bio_list is also used as a
	 * flag to say if generic_make_request is currently active in this
	 * task or not.  If it is NULL, then no make_request is active.  If
	 * it is non-NULL, then a make_request is active, and new requests
	 * should be added at the tail
	 */
	if (current->bio_list) { // todo 似乎是处理递归的
		bio_list_add(&current->bio_list[0], bio);
		goto out;
	}

	/* following loop may be a bit non-obvious, and so deserves some
	 * explanation.
	 * Before entering the loop, bio->bi_next is NULL (as all callers
	 * ensure that) so we have a list with a single bio.
	 * We pretend that we have just taken it off a longer list, so
	 * we assign bio_list to a pointer to the bio_list_on_stack,
	 * thus initialising the bio_list of new bios to be
	 * added.  ->make_request() may indeed add some more bios
	 * through a recursive call to generic_make_request.  If it
	 * did, we find a non-NULL value in bio_list and re-enter the loop
	 * from the top.  In this case we really did just take the bio
	 * of the top of the list (no pretending) and so remove it from
	 * bio_list, and call into ->make_request() again.
	 */
	BUG_ON(bio->bi_next);
	bio_list_init(&bio_list_on_stack[0]);
	current->bio_list = bio_list_on_stack;
	do {
		struct request_queue *q = bio->bi_disk->queue;
		blk_mq_req_flags_t flags = bio->bi_opf & REQ_NOWAIT ?
			BLK_MQ_REQ_NOWAIT : 0;

		if (likely(blk_queue_enter(q, flags) == 0)) {  // todo try to increase q->q_usage_counter
			struct bio_list lower, same;

			/* Create a fresh bio_list for all subordinate requests */
			bio_list_on_stack[1] = bio_list_on_stack[0];
			bio_list_init(&bio_list_on_stack[0]);
			ret = q->make_request_fn(q, bio); // 将事情交给 mq 之类的东西上了

			blk_queue_exit(q); // decrease q->q_usage_counter

      // todo 下面处理 bio list 的各种合并工作，但是不知道为什么这么做 !
			/* sort new bios into those for a lower level
			 * and those for the same level
			 */
			bio_list_init(&lower);
			bio_list_init(&same);
			while ((bio = bio_list_pop(&bio_list_on_stack[0])) != NULL)
				if (q == bio->bi_disk->queue)
					bio_list_add(&same, bio);
				else
					bio_list_add(&lower, bio);
			/* now assemble so we handle the lowest level first */
			bio_list_merge(&bio_list_on_stack[0], &lower);
			bio_list_merge(&bio_list_on_stack[0], &same);
			bio_list_merge(&bio_list_on_stack[0], &bio_list_on_stack[1]);
		} else {
			if (unlikely(!blk_queue_dying(q) &&
					(bio->bi_opf & REQ_NOWAIT)))
				bio_wouldblock_error(bio);
			else
				bio_io_error(bio);
		}
		bio = bio_list_pop(&bio_list_on_stack[0]);
	} while (bio);
	current->bio_list = NULL; /* deactivate */

out:
	return ret;
}
```


## merge
1. 各种 try merge 函数，其使用位置都是各种 mq 了


## submit_bio
1. 这是fs 和 dev 进行 io 的唯一的接口吗 ?
2. submit_bio 和 generic_make_request 的关系是什么 ? 两者都是类似接口，但是 submit_bio 做出部分检查，最后调用 generic_make_request


iomap 直接来调用 submit_bio，有趣:
```txt
#0  submit_bio (bio=0xffff88822baa1f00) at block/blk-cgroup.h:398
#1  0xffffffff813bc63d in iomap_readahead (rac=<optimized out>, ops=0xffffffff8246d090 <xfs_read_iomap_ops>) at fs/iomap/buffered-io.c:422
#2  0xffffffff8128a334 in read_pages (rac=rac@entry=0xffffc900020afad0) at mm/readahead.c:158
#3  0xffffffff8128a6b0 in page_cache_ra_unbounded (ractl=ractl@entry=0xffffc900020afad0, nr_to_read=35, lookahead_size=<optimized out>) at mm/readahead.c:263
#4  0xffffffff8128ac39 in do_page_cache_ra (lookahead_size=<optimized out>, nr_to_read=<optimized out>, ractl=0xffffc900020afad0) at mm/readahead.c:293
#5  0xffffffff8127fc77 in do_sync_mmap_readahead (vmf=0xffffc900020afb98) at mm/filemap.c:3028
#6  filemap_fault (vmf=0xffffc900020afb98) at mm/filemap.c:3120
#7  0xffffffff812bacaf in __do_fault (vmf=vmf@entry=0xffffc900020afb98) at mm/memory.c:4173
#8  0xffffffff812bf1bf in do_cow_fault (vmf=0xffffc900020afb98) at mm/memory.c:4548
#9  do_fault (vmf=vmf@entry=0xffffc900020afb98) at mm/memory.c:4649
#10 0xffffffff812c3ba0 in handle_pte_fault (vmf=0xffffc900020afb98) at mm/memory.c:4911
#11 __handle_mm_fault (vma=vma@entry=0xffff8882235f1000, address=address@entry=93979750306376, flags=flags@entry=533) at mm/memory.c:5053
#12 0xffffffff812c4440 in handle_mm_fault (vma=0xffff8882235f1000, address=address@entry=93979750306376, flags=flags@entry=533, regs=regs@entry=0xffffc900020afcf8) at mm/memory.c:5151
#13 0xffffffff810f2983 in do_user_addr_fault (regs=regs@entry=0xffffc900020afcf8, error_code=error_code@entry=2, address=address@entry=93979750306376) at arch/x86/mm/fault.c:1397
#14 0xffffffff81edffa2 in handle_page_fault (address=93979750306376, error_code=2, regs=0xffffc900020afcf8) at arch/x86/mm/fault.c:1488
#15 exc_page_fault (regs=0xffffc900020afcf8, error_code=2) at arch/x86/mm/fault.c:1544
#16 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

## account
