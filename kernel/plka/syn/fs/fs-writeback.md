# fs-writeback.md
1. 这里并不是处理 inode sb 之类的写会，此处和 page-writeback.c 配合使用，共同完成 writeback 机制。


## TODO
1. blk_plug 机制
2. 如何找到所有的 dirty pages
3. 如何确定时间的 ?
4. 和 page-writeback.c 的沟通方式是什么 ?

## doc
- [](https://lwn.net/Articles/326552/)


## core struct
1. 这个如何创建的

```c
struct backing_dev_info {
	u64 id;
	struct rb_node rb_node; /* keyed by ->id */
	struct list_head bdi_list;
	unsigned long ra_pages;	/* max readahead in PAGE_SIZE units */
	unsigned long io_pages;	/* max allowed IO size */
	congested_fn *congested_fn; /* Function pointer if device is md/dm */
	void *congested_data;	/* Pointer to aux data for congested func */

	const char *name;

	struct kref refcnt;	/* Reference counter for the structure */
	unsigned int capabilities; /* Device capabilities */
	unsigned int min_ratio;
	unsigned int max_ratio, max_prop_frac;

	/*
	 * Sum of avg_write_bw of wbs with dirty inodes.  > 0 if there are
	 * any dirty wbs, which is depended upon by bdi_has_dirty().
	 */
	atomic_long_t tot_write_bandwidth;

	struct bdi_writeback wb;  /* the root writeback info for this bdi */
	struct list_head wb_list; /* list of all wbs */
#ifdef CONFIG_CGROUP_WRITEBACK
	struct radix_tree_root cgwb_tree; /* radix tree of active cgroup wbs */
	struct rb_root cgwb_congested_tree; /* their congested states */
	struct mutex cgwb_release_mutex;  /* protect shutdown of wb structs */
	struct rw_semaphore wb_switch_rwsem; /* no cgwb switch while syncing */
#else
	struct bdi_writeback_congested *wb_congested;
#endif
	wait_queue_head_t wb_waitq;

	struct device *dev;
	struct device *owner;

	struct timer_list laptop_mode_wb_timer;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debug_dir;
#endif
};

/*
 * Each wb (bdi_writeback) can perform writeback operations, is measured
 * and throttled, independently.  Without cgroup writeback, each bdi
 * (bdi_writeback) is served by its embedded bdi->wb.
 *
 * On the default hierarchy, blkcg implicitly enables memcg.  This allows
 * using memcg's page ownership for attributing writeback IOs, and every
 * memcg - blkcg combination can be served by its own wb by assigning a
 * dedicated wb to each memcg, which enables isolation across different
 * cgroups and propagation of IO back pressure down from the IO layer upto
 * the tasks which are generating the dirty pages to be written back.
 *
 * A cgroup wb is indexed on its bdi by the ID of the associated memcg,
 * refcounted with the number of inodes attached to it, and pins the memcg
 * and the corresponding blkcg.  As the corresponding blkcg for a memcg may
 * change as blkcg is disabled and enabled higher up in the hierarchy, a wb
 * is tested for blkcg after lookup and removed from index on mismatch so
 * that a new wb for the combination can be created.
 */
struct bdi_writeback {
	struct backing_dev_info *bdi;	/* our parent bdi */

	unsigned long state;		/* Always use atomic bitops on this */
	unsigned long last_old_flush;	/* last old data flush */

	struct list_head b_dirty;	/* dirty inodes */
	struct list_head b_io;		/* parked for writeback */
	struct list_head b_more_io;	/* parked for more writeback */
	struct list_head b_dirty_time;	/* time stamps are dirty */
	spinlock_t list_lock;		/* protects the b_* lists */

	struct percpu_counter stat[NR_WB_STAT_ITEMS];

	struct bdi_writeback_congested *congested;

	unsigned long bw_time_stamp;	/* last time write bw is updated */
	unsigned long dirtied_stamp;
	unsigned long written_stamp;	/* pages written at bw_time_stamp */
	unsigned long write_bandwidth;	/* the estimated write bandwidth */
	unsigned long avg_write_bandwidth; /* further smoothed write bw, > 0 */

	/*
	 * The base dirty throttle rate, re-calculated on every 200ms.
	 * All the bdi tasks' dirty rate will be curbed under it.
	 * @dirty_ratelimit tracks the estimated @balanced_dirty_ratelimit
	 * in small steps and is much more smooth/stable than the latter.
	 */
	unsigned long dirty_ratelimit;
	unsigned long balanced_dirty_ratelimit;

	struct fprop_local_percpu completions;
	int dirty_exceeded;
	enum wb_reason start_all_reason;

	spinlock_t work_lock;		/* protects work_list & dwork scheduling */
	struct list_head work_list;
	struct delayed_work dwork;	/* work item used for writeback */

	unsigned long dirty_sleep;	/* last wait */

	struct list_head bdi_node;	/* anchored at bdi->wb_list */

#ifdef CONFIG_CGROUP_WRITEBACK
	struct percpu_ref refcnt;	/* used only for !root wb's */
	struct fprop_local_percpu memcg_completions;
	struct cgroup_subsys_state *memcg_css; /* the associated memcg */
	struct cgroup_subsys_state *blkcg_css; /* and blkcg */
	struct list_head memcg_node;	/* anchored at memcg->cgwb_list */
	struct list_head blkcg_node;	/* anchored at blkcg->cgwb_list */

	union {
		struct work_struct release_work;
		struct rcu_head rcu;
	};
#endif
};


/*
 * Passed into wb_writeback(), essentially a subset of writeback_control
 */
struct wb_writeback_work {
	long nr_pages;
	struct super_block *sb;
	unsigned long *older_than_this;
	enum writeback_sync_modes sync_mode;
	unsigned int tagged_writepages:1;
	unsigned int for_kupdate:1;
	unsigned int range_cyclic:1;
	unsigned int for_background:1;
	unsigned int for_sync:1;	/* sync(2) WB_SYNC_ALL writeback */
	unsigned int auto_free:1;	/* free on completion */
	enum wb_reason reason;		/* why was writeback initiated? */

	struct list_head list;		/* pending work list */
	struct wb_completion *done;	/* set if the caller waits */
};
```


## wakeup_flusher_threads 
1. vmscan.c:shrink_inactive_list 调用此处
2. 其内容和 wakeup_dirtytime_writeback 几乎没有区别
    1. 但是增加很多检查
    2. 对于三级关系不是很了解 : backing_dev_info, bdi_writeback, 和 work_struct

```c
/*
 * Wakeup the flusher threads to start writeback of all currently dirty pages
 */
void wakeup_flusher_threads(enum wb_reason reason)
{
	struct backing_dev_info *bdi;

	/*
	 * If we are expecting writeback progress we must submit plugged IO.
	 */
	if (blk_needs_flush_plug(current)) // todo
		blk_schedule_flush_plug(current);

	rcu_read_lock();
	list_for_each_entry_rcu(bdi, &bdi_list, bdi_list) // 对于所有的bdi
		__wakeup_flusher_threads_bdi(bdi, reason);
	rcu_read_unlock();
}


/*
 * Start writeback of `nr_pages' pages on this bdi. If `nr_pages' is zero,
 * write back the whole world.
 */
static void __wakeup_flusher_threads_bdi(struct backing_dev_info *bdi,
					 enum wb_reason reason)
{
	struct bdi_writeback *wb;

	if (!bdi_has_dirty_io(bdi))
		return;

	list_for_each_entry_rcu(wb, &bdi->wb_list, bdi_node) // 对于所有的 bdi 上的所有的 wb
		wb_start_writeback(wb, reason);
}


static void wb_start_writeback(struct bdi_writeback *wb, enum wb_reason reason)
{
	if (!wb_has_dirty_io(wb))
		return;

	/*
	 * All callers of this function want to start writeback of all
	 * dirty pages. Places like vmscan can call this at a very
	 * high frequency, causing pointless allocations of tons of
	 * work items and keeping the flusher threads busy retrieving
	 * that work. Ensure that we only allow one of them pending and
	 * inflight at the time.
	 */
	if (test_bit(WB_start_all, &wb->state) ||
	    test_and_set_bit(WB_start_all, &wb->state))
		return;

	wb->start_all_reason = reason;
	wb_wakeup(wb);
}

static void wb_wakeup(struct bdi_writeback *wb)
{
	spin_lock_bh(&wb->work_lock);
	if (test_bit(WB_registered, &wb->state))
		mod_delayed_work(bdi_wq, &wb->dwork, 0); // 然后进入到 workqueue 中间了
	spin_unlock_bh(&wb->work_lock);
}
```


## wb_workfn : todo

```c
/*
 * Handle writeback of dirty data for the device backed by this bdi. Also
 * reschedules periodically and does kupdated style flushing.
 */
void wb_workfn(struct work_struct *work) // todo work_struct 的作用是什么 ?


/*
 * Retrieve work items and do the writeback they describe
 */
static long wb_do_writeback(struct bdi_writeback *wb)
{
	struct wb_writeback_work *work;
	long wrote = 0;

	set_bit(WB_writeback_running, &wb->state);
	while ((work = get_next_work_item(wb)) != NULL) { // 遍历所有的 wb_writeback_work 
		trace_writeback_exec(wb, work);
		wrote += wb_writeback(wb, work);
		finish_writeback_work(wb, work);
	}

	/*
	 * Check for a flush-everything request
	 */
	wrote += wb_check_start_all(wb);

	/*
	 * Check for periodic writeback, kupdated() style
	 */
	wrote += wb_check_old_data_flush(wb);
	wrote += wb_check_background_flush(wb);
	clear_bit(WB_writeback_running, &wb->state);

	return wrote;
}
```

## wb_writeback && writeback_sb_inodes : 真正的writeback工作完成者
1. wb_writeback 调用 writeback_sb_inodes


## dirtytime_work : 周期 writeback 工作

```c
/*
 * Wake up bdi's periodically to make sure dirtytime inodes gets
 * written back periodically.  We deliberately do *not* check the
 * b_dirtytime list in wb_has_dirty_io(), since this would cause the
 * kernel to be constantly waking up once there are any dirtytime
 * inodes on the system.  So instead we define a separate delayed work
 * function which gets called much more rarely.  (By default, only
 * once every 12 hours.)
 *
 * If there is any other write activity going on in the file system,
 * this function won't be necessary.  But if the only thing that has
 * happened on the file system is a dirtytime inode caused by an atime
 * update, we need this infrastructure below to make sure that inode
 * eventually gets pushed out to disk.
 */
static void wakeup_dirtytime_writeback(struct work_struct *w);
static DECLARE_DELAYED_WORK(dirtytime_work, wakeup_dirtytime_writeback);

static void wakeup_dirtytime_writeback(struct work_struct *w)
{
	struct backing_dev_info *bdi;

	rcu_read_lock();
	list_for_each_entry_rcu(bdi, &bdi_list, bdi_list) {
		struct bdi_writeback *wb;

		list_for_each_entry_rcu(wb, &bdi->wb_list, bdi_node)
			if (!list_empty(&wb->b_dirty_time))
				wb_wakeup(wb);
	}
	rcu_read_unlock();
	schedule_delayed_work(&dirtytime_work, dirtytime_expire_interval * HZ);
}
```
