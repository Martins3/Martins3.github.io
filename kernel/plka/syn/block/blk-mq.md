# blk-mq.md


## TODO

## hw_queue

```c
void blk_mq_start_hw_queue(struct blk_mq_hw_ctx *hctx)
{
	clear_bit(BLK_MQ_S_STOPPED, &hctx->state);

	blk_mq_run_hw_queue(hctx, false);
}

void blk_mq_start_hw_queues(struct request_queue *q)
{
	struct blk_mq_hw_ctx *hctx;
	int i;

	queue_for_each_hw_ctx(q, hctx, i)
		blk_mq_start_hw_queue(hctx);
}

#define queue_for_each_hw_ctx(q, hctx, i)				\
	for ((i) = 0; (i) < (q)->nr_hw_queues &&			\
	     ({ hctx = (q)->queue_hw_ctx[i]; 1; }); (i)++)

/*
 * This function is often used for pausing .queue_rq() by driver when
 * there isn't enough resource or some conditions aren't satisfied, and
 * BLK_STS_RESOURCE is usually returned.
 *
 * We do not guarantee that dispatch can be drained or blocked
 * after blk_mq_stop_hw_queues() returns. Please use
 * blk_mq_quiesce_queue() for that requirement.
 */
void blk_mq_stop_hw_queues(struct request_queue *q)
{
	struct blk_mq_hw_ctx *hctx;
	int i;

	queue_for_each_hw_ctx(q, hctx, i)
		blk_mq_stop_hw_queue(hctx);
}

/*
 * This function is often used for pausing .queue_rq() by driver when
 * there isn't enough resource or some conditions aren't satisfied, and
 * BLK_STS_RESOURCE is usually returned.
 *
 * We do not guarantee that dispatch can be drained or blocked
 * after blk_mq_stop_hw_queue() returns. Please use
 * blk_mq_quiesce_queue() for that requirement.
 */
void blk_mq_stop_hw_queue(struct blk_mq_hw_ctx *hctx)
{
	cancel_delayed_work(&hctx->run_work);

	set_bit(BLK_MQ_S_STOPPED, &hctx->state);
}
```

## freeze
```c
void blk_freeze_queue_start(struct request_queue *q)
{
	mutex_lock(&q->mq_freeze_lock);
	if (++q->mq_freeze_depth == 1) {
		percpu_ref_kill(&q->q_usage_counter);
		mutex_unlock(&q->mq_freeze_lock);
		if (queue_is_mq(q))
			blk_mq_run_hw_queues(q, false);
	} else {
		mutex_unlock(&q->mq_freeze_lock);
	}
}
EXPORT_SYMBOL_GPL(blk_freeze_queue_start);
```



## init
```c
/**
 * blk_mq_make_request - Create and send a request to block device.
 * @q: Request queue pointer.
 * @bio: Bio pointer.
 *
 * Builds up a request structure from @q and @bio and send to the device. The
 * request may not be queued directly to hardware if:
 * * This request can be merged with another one
 * * We want to place request at plug queue for possible future merging
 * * There is an IO scheduler active at this queue
 *
 * It will not queue the request if there is an error with the bio, or at the
 * request creation.
 *
 * Returns: Request queue cookie.
 */
static blk_qc_t blk_mq_make_request(struct request_queue *q, struct bio *bio)

struct request_queue *blk_mq_init_allocated_queue(struct blk_mq_tag_set *set,
						  struct request_queue *q,
						  bool elevator_init) // 该函数在此处被注册，使用 blk_queue_make_request 注册

/**
 * blk_queue_make_request - define an alternate make_request function for a device
 * @q:  the request queue for the device to be affected
 * @mfn: the alternate make_request function
 *
 * Description:
 *    The normal way for &struct bios to be passed to a device
 *    driver is for them to be collected into requests on a request
 *    queue, and then to allow the device driver to select requests
 *    off that queue when it is ready.  This works well for many block
 *    devices. However some block devices (typically virtual devices
 *    such as md or lvm) do not benefit from the processing on the
 *    request queue, and are served best by having the requests passed
 *    directly to them.  This can be achieved by providing a function
 *    to blk_queue_make_request().
 *
 * Caveat:
 *    The driver that does this *must* be able to deal appropriately
 *    with buffers in "highmemory". This can be accomplished by either calling
 *    kmap_atomic() to get a temporary kernel mapping, or by calling
 *    blk_queue_bounce() to create a buffer in normal memory.
 **/
void blk_queue_make_request(struct request_queue *q, make_request_fn *mfn) // 所以，对于 md 和 lvm 等东西，make defconfig 下 md 和 dm 两个设备注册过
{
	/*
	 * set defaults
	 */
	q->nr_requests = BLKDEV_MAX_RQ;

	q->make_request_fn = mfn;
	blk_queue_dma_alignment(q, 511); // todo 终于找到了 dma 的影子了。

	blk_set_default_limits(&q->limits);
}
```


## core struct : software and hardware queue

```c
/**
 * struct blk_mq_hw_ctx - State for a hardware queue facing the hardware
 * block device
 */
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

	/**
	 * @run_work: Used for scheduling a hardware queue run at a later time.
	 */
	struct delayed_work	run_work;
	/** @cpumask: Map of available CPUs where this hctx can run. */
	cpumask_var_t		cpumask;
	/**
	 * @next_cpu: Used by blk_mq_hctx_next_cpu() for round-robin CPU
	 * selection from @cpumask.
	 */
	int			next_cpu;
	/**
	 * @next_cpu_batch: Counter of how many works left in the batch before
	 * changing to the next CPU.
	 */
	int			next_cpu_batch;

	/** @flags: BLK_MQ_F_* flags. Defines the behaviour of the queue. */
	unsigned long		flags;

	/**
	 * @sched_data: Pointer owned by the IO scheduler attached to a request
	 * queue. It's up to the IO scheduler how to use this pointer.
	 */
	void			*sched_data;
	/**
	 * @queue: Pointer to the request queue that owns this hardware context.
	 */
	struct request_queue	*queue;
	/** @fq: Queue of requests that need to perform a flush operation. */
	struct blk_flush_queue	*fq;

	/**
	 * @driver_data: Pointer to data owned by the block driver that created
	 * this hctx
	 */
	void			*driver_data;

	/**
	 * @ctx_map: Bitmap for each software queue. If bit is on, there is a
	 * pending request in that software queue.
	 */
	struct sbitmap		ctx_map;

	/**
	 * @dispatch_from: Software queue to be used when no scheduler was
	 * selected.
	 */
	struct blk_mq_ctx	*dispatch_from;
	/**
	 * @dispatch_busy: Number used by blk_mq_update_dispatch_busy() to
	 * decide if the hw_queue is busy using Exponential Weighted Moving
	 * Average algorithm.
	 */
	unsigned int		dispatch_busy;

	/** @type: HCTX_TYPE_* flags. Type of hardware queue. */
	unsigned short		type;
	/** @nr_ctx: Number of software queues. */
	unsigned short		nr_ctx;
	/** @ctxs: Array of software queues. */
	struct blk_mq_ctx	**ctxs;

	/** @dispatch_wait_lock: Lock for dispatch_wait queue. */
	spinlock_t		dispatch_wait_lock;
	/**
	 * @dispatch_wait: Waitqueue to put requests when there is no tag
	 * available at the moment, to wait for another try in the future.
	 */
	wait_queue_entry_t	dispatch_wait;

	/**
	 * @wait_index: Index of next available dispatch_wait queue to insert
	 * requests.
	 */
	atomic_t		wait_index;

	/**
	 * @tags: Tags owned by the block driver. A tag at this set is only
	 * assigned when a request is dispatched from a hardware queue.
	 */
	struct blk_mq_tags	*tags;
	/**
	 * @sched_tags: Tags owned by I/O scheduler. If there is an I/O
	 * scheduler associated with a request queue, a tag is assigned when
	 * that request is allocated. Else, this member is not used.
	 */
	struct blk_mq_tags	*sched_tags;

	/** @queued: Number of queued requests. */
	unsigned long		queued;
	/** @run: Number of dispatched requests. */
	unsigned long		run;
#define BLK_MQ_MAX_DISPATCH_ORDER	7
	/** @dispatched: Number of dispatch requests by queue. */
	unsigned long		dispatched[BLK_MQ_MAX_DISPATCH_ORDER];

	/** @numa_node: NUMA node the storage adapter has been connected to. */
	unsigned int		numa_node;
	/** @queue_num: Index of this hardware queue. */
	unsigned int		queue_num;

	/**
	 * @nr_active: Number of active requests. Only used when a tag set is
	 * shared across request queues.
	 */
	atomic_t		nr_active;

	/** @cpuhp_dead: List to store request if some CPU die. */
	struct hlist_node	cpuhp_dead;
	/** @kobj: Kernel object for sysfs. */
	struct kobject		kobj;

	/** @poll_considered: Count times blk_poll() was called. */
	unsigned long		poll_considered;
	/** @poll_invoked: Count how many requests blk_poll() polled. */
	unsigned long		poll_invoked;
	/** @poll_success: Count how many polled requests were completed. */
	unsigned long		poll_success;

#ifdef CONFIG_BLK_DEBUG_FS
	/**
	 * @debugfs_dir: debugfs directory for this hardware queue. Named
	 * as cpu<cpu_number>.
	 */
	struct dentry		*debugfs_dir;
	/** @sched_debugfs_dir:	debugfs directory for the scheduler. */
	struct dentry		*sched_debugfs_dir;
#endif

	/** @hctx_list:	List of all hardware queues. */
	struct list_head	hctx_list;

	/**
	 * @srcu: Sleepable RCU. Use as lock when type of the hardware queue is
	 * blocking (BLK_MQ_F_BLOCKING). Must be the last member - see also
	 * blk_mq_hw_ctx_size().
	 */
	struct srcu_struct	srcu[0];
};
```

```c
/**
 * struct blk_mq_ctx - State for a software queue facing the submitting CPUs
 */
struct blk_mq_ctx {
	struct {
		spinlock_t		lock;
		struct list_head	rq_lists[HCTX_MAX_TYPES];
	} ____cacheline_aligned_in_smp;

	unsigned int		cpu;
	unsigned short		index_hw[HCTX_MAX_TYPES];
	struct blk_mq_hw_ctx 	*hctxs[HCTX_MAX_TYPES];

	/* incremented at dispatch time */
	unsigned long		rq_dispatched[2];
	unsigned long		rq_merged;

	/* incremented at completion time */
	unsigned long		____cacheline_aligned_in_smp rq_completed[2];

	struct request_queue	*queue;
	struct blk_mq_ctxs      *ctxs;
	struct kobject		kobj;
} ____cacheline_aligned_in_smp;
```



## blk_mq_make_request : generic_make_request 的核心调用函数
1. 这个真的 TMD 看不懂好吧 !

```c
/**
 * blk_mq_make_request - Create and send a request to block device.
 * @q: Request queue pointer.
 * @bio: Bio pointer.
 *
 * Builds up a request structure from @q and @bio and send to the device. The
 * request may not be queued directly to hardware if:
 * * This request can be merged with another one
 * * We want to place request at plug queue for possible future merging
 * * There is an IO scheduler active at this queue
 *
 * It will not queue the request if there is an error with the bio, or at the
 * request creation.
 *
 * Returns: Request queue cookie.
 */
static blk_qc_t blk_mq_make_request(struct request_queue *q, struct bio *bio)
{
	const int is_sync = op_is_sync(bio->bi_opf);
	const int is_flush_fua = op_is_flush(bio->bi_opf);
	struct blk_mq_alloc_data data = { .flags = 0};
	struct request *rq;
	struct blk_plug *plug;
	struct request *same_queue_rq = NULL;
	unsigned int nr_segs;
	blk_qc_t cookie;

	blk_queue_bounce(q, &bio); //  todo bounce 是什么鬼东西 ?
	__blk_queue_split(q, &bio, &nr_segs);

	if (!bio_integrity_prep(bio))
		return BLK_QC_T_NONE;

	if (!is_flush_fua && !blk_queue_nomerges(q) &&
	    blk_attempt_plug_merge(q, bio, nr_segs, &same_queue_rq))
		return BLK_QC_T_NONE;

	if (blk_mq_sched_bio_merge(q, bio, nr_segs))
		return BLK_QC_T_NONE;

	rq_qos_throttle(q, bio);

	data.cmd_flags = bio->bi_opf;
	rq = blk_mq_get_request(q, bio, &data);
	if (unlikely(!rq)) {
		rq_qos_cleanup(q, bio);
		if (bio->bi_opf & REQ_NOWAIT)
			bio_wouldblock_error(bio);
		return BLK_QC_T_NONE;
	}

	trace_block_getrq(q, bio, bio->bi_opf);

	rq_qos_track(q, rq, bio);  // todo qos 到底是干啥的 ?

	cookie = request_to_qc_t(data.hctx, rq);

	blk_mq_bio_to_request(rq, bio, nr_segs);

	plug = blk_mq_plug(q, bio);
	if (unlikely(is_flush_fua)) {
		/* Bypass scheduler for flush requests */
		blk_insert_flush(rq);
		blk_mq_run_hw_queue(data.hctx, true);
	} else if (plug && (q->nr_hw_queues == 1 || q->mq_ops->commit_rqs ||
				!blk_queue_nonrot(q))) {
		/*
		 * Use plugging if we have a ->commit_rqs() hook as well, as
		 * we know the driver uses bd->last in a smart fashion.
		 *
		 * Use normal plugging if this disk is slow HDD, as sequential
		 * IO may benefit a lot from plug merging.
		 */
		unsigned int request_count = plug->rq_count;
		struct request *last = NULL;

		if (!request_count)
			trace_block_plug(q);
		else
			last = list_entry_rq(plug->mq_list.prev);

		if (request_count >= BLK_MAX_REQUEST_COUNT || (last &&
		    blk_rq_bytes(last) >= BLK_PLUG_FLUSH_SIZE)) {
			blk_flush_plug_list(plug, false);
			trace_block_plug(q);
		}

		blk_add_rq_to_plug(plug, rq);
	} else if (q->elevator) {
		/* Insert the request at the IO scheduler queue */
		blk_mq_sched_insert_request(rq, false, true, true);
	} else if (plug && !blk_queue_nomerges(q)) {
		/*
		 * We do limited plugging. If the bio can be merged, do that.
		 * Otherwise the existing request in the plug list will be
		 * issued. So the plug list will have one request at most
		 * The plug list might get flushed before this. If that happens,
		 * the plug list is empty, and same_queue_rq is invalid.
		 */
		if (list_empty(&plug->mq_list))
			same_queue_rq = NULL;
		if (same_queue_rq) {
			list_del_init(&same_queue_rq->queuelist);
			plug->rq_count--;
		}
		blk_add_rq_to_plug(plug, rq);
		trace_block_plug(q);

		if (same_queue_rq) {
			data.hctx = same_queue_rq->mq_hctx;
			trace_block_unplug(q, 1, true);
			blk_mq_try_issue_directly(data.hctx, same_queue_rq,
					&cookie);
		}
	} else if ((q->nr_hw_queues > 1 && is_sync) ||
			!data.hctx->dispatch_busy) {
		/*
		 * There is no scheduler and we can try to send directly
		 * to the hardware.
		 */
		blk_mq_try_issue_directly(data.hctx, rq, &cookie);
	} else {
		/* Default case. */
		blk_mq_sched_insert_request(rq, false, true, true);
	}

	return cookie;
}
```

