## arch_freq_get_on_cpu 中为什么使用 read_seqcount_retry

## [ ] 从 folio_try_get 的细节分析

git show bd225530a4c717714722c3731442b78954c765b3

其实最后的问题在于，为什么这里有 page
```c
static inline bool page_ref_add_unless(struct page *page, int nr, int u)
{
	bool ret = false;

	rcu_read_lock();
	/* avoid writing to the vmemmap area being remapped */
	if (page_count_writable(page, u))
		ret = atomic_add_unless(&page->_refcount, nr, u);
	rcu_read_unlock();

	if (page_ref_tracepoint_active(page_ref_mod_unless))
		__page_ref_mod_unless(page, nr, ret);
	return ret;
}
```

## raise_barrier

在 raid1.c 中，为什么 raise_barrier 中非要使用 smp_mb__after_atomic

而且 raise_barrier 和 `_wait_barrier` 中的顺序是反过来的，是故意的这么设计的吧!
```c
	/*
	 * In raise_barrier() we firstly increase conf->barrier[idx] then
	 * check conf->nr_pending[idx]. In _wait_barrier() we firstly
	 * increase conf->nr_pending[idx] then check conf->barrier[idx].
	 * A memory barrier here to make sure conf->nr_pending[idx] won't
	 * be fetched before conf->barrier[idx] is increased. Otherwise
	 * there will be a race between raise_barrier() and _wait_barrier().
	 */
```

## [ ] llist_for_each_entry_safe : 我靠， lockless 的接口
似乎和我们想象的 lockless 没有关系，但是这还存在类似的好几个接口，可以分析下 safe 体现在何处?

## 3af4a9e61e71117d5df2df3e1605fa062e04b869

## TODO
Code that is safe from concurrent access from an interrupt handler is said to be
**interrupt-safe**. Code that is safe from concurrency on symmetrical multiprocessing
machines is **SMP-safe**. Code that is safe from concurrency with kernel preemption is **preempt-safe**.

### page_counter_try_charge 中说明自己自己是如何避免一个 CSA 操作的
```c
/**
 * page_counter_try_charge - try to hierarchically charge pages
 * @counter: counter
 * @nr_pages: number of pages to charge
 * @fail: points first counter to hit its limit, if any
 *
 * Returns %true on success, or %false and @fail if the counter or one
 * of its ancestors has hit its configured limit.
 */
bool page_counter_try_charge(struct page_counter *counter,
			     unsigned long nr_pages,
			     struct page_counter **fail)
{
	struct page_counter *c;

	for (c = counter; c; c = c->parent) {
		long new;
		/*
		 * Charge speculatively to avoid an expensive CAS.  If
		 * a bigger charge fails, it might falsely lock out a
		 * racing smaller charge and send it into reclaim
		 * early, but the error is limited to the difference
		 * between the two sizes, which is less than 2M/4M in
		 * case of a THP locking out a regular page charge.
		 *
		 * The atomic_long_add_return() implies a full memory
		 * barrier between incrementing the count and reading
		 * the limit.  When racing with page_counter_set_max(),
		 * we either see the new limit or the setter sees the
		 * counter has changed and retries.
		 */
		new = atomic_long_add_return(nr_pages, &c->usage);
		if (new > c->max) {
			atomic_long_sub(nr_pages, &c->usage);
			/*
			 * This is racy, but we can live with some
			 * inaccuracy in the failcnt which is only used
			 * to report stats.
			 */
			data_race(c->failcnt++);
			*fail = c;
			goto failed;
		}
		propagate_protected_usage(c, new);
		/*
		 * Just like with failcnt, we can live with some
		 * inaccuracy in the watermark.
		 */
		if (new > READ_ONCE(c->watermark))
			WRITE_ONCE(c->watermark, new);
	}
	return true;

failed:
	for (c = counter; c != *fail; c = c->parent)
		page_counter_cancel(c, nr_pages);

	return false;
}
```

## 几个 memory model 的例子
### `__setup_APIC_LVTT`
```c
		/*
		 * See Intel SDM: TSC-Deadline Mode chapter. In xAPIC mode,
		 * writing to the APIC LVTT and TSC_DEADLINE MSR isn't serialized.
		 * According to Intel, MFENCE can do the serialization here.
		 */
		asm volatile("mfence" : : : "memory");
		return;
```

### wait_on_bit
kernel 8238b4579866b7c1bb99883cfe102a43db5506ff

### do_idle 中的 memory model

### virtio_wmb

## 如何理解下这里 smp_mb__after_atomic 的作用

```c
static int scsi_mq_get_budget(struct request_queue *q)
{
	struct scsi_device *sdev = q->queuedata;
	int token = scsi_dev_queue_ready(q, sdev);

	if (token >= 0)
		return token;

	atomic_inc(&sdev->restarts);

	/*
	 * Orders atomic_inc(&sdev->restarts) and atomic_read(&sdev->device_busy).
	 * .restarts must be incremented before .device_busy is read because the
	 * code in scsi_run_queue_async() depends on the order of these operations.
	 */
	smp_mb__after_atomic();

	/*
	 * If all in-flight requests originated from this LUN are completed
	 * before reading .device_busy, sdev->device_busy will be observed as
	 * zero, then blk_mq_delay_run_hw_queues() will dispatch this request
	 * soon. Otherwise, completion of one of these requests will observe
	 * the .restarts flag, and the request queue will be run for handling
	 * this request, see scsi_end_request().
	 */
	if (unlikely(scsi_device_busy(sdev) == 0 &&
				!scsi_device_blocked(sdev)))
		blk_mq_delay_run_hw_queues(sdev->request_queue, SCSI_QUEUE_DELAY);
	return -1;
}
```

## blk_queue_enter 中，wait_event 前面居然有一个 smp_read

## rw_sem

理解一下这里的内容
```c
/*
 * All writes to owner are protected by WRITE_ONCE() to make sure that
 * store tearing can't happen as optimistic spinners may read and use
 * the owner value concurrently without lock. Read from owner, however,
 * may not need READ_ONCE() as long as the pointer value is only used
 * for comparison and isn't being dereferenced.
 *
 * Both rwsem_{set,clear}_owner() functions should be in the same
 * preempt disable section as the atomic op that changes sem->count.
 */
static inline void rwsem_set_owner(struct rw_semaphore *sem)
{
	lockdep_assert_preemption_disabled();
	atomic_long_set(&sem->owner, (long)current);
}

static inline void rwsem_clear_owner(struct rw_semaphore *sem)
{
	lockdep_assert_preemption_disabled();
	atomic_long_set(&sem->owner, 0);
}
```
我记得就是有 onwer 的，用 crash 来分析一下吧


## 类似这种的，将直接访问修改为 READ_ONCE 的，理解下原理吧
git show b192812905e4b134f7b7994b079eb647e9d2d37e


## 例如这种中断重置如何处理
```txt
7cc148a32f1e7496e22c0005dd113a31d4a3b3d4
9c15eeb5362c48dd27d51bd72e8873341fa9383c
8f4b589595d01f882d63d21efe15af4a5ad7c59b
```
https://gitee.com/openeuler/kernel/commit/a95cc4dafae34dc6da8cb86c9c644290836c6684

## 具体案例的分析
只是加了两行代码，但是发了 50+ 封邮件
- https://lore.kernel.org/all/Zd4DXTyCf17lcTfq@debian.debian/
- https://lore.kernel.org/bpf/ZeFPz4D121TgvCje@debian.debian/

## 两行代码，几千字来实现分析


```diff
History:        #0
Commit:         a484c3dd9426e1f450c7ed8ad5a2002ea1b309ea
Author:         Paolo Bonzini <pbonzini@redhat.com>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Wed 23 Mar 2016 05:27:14 AM CST
Committer Date: Wed 23 Mar 2016 06:36:02 AM CST

eventfd: document lockless access in eventfd_poll

Since commit e22553e2a25e ("eventfd: don't take the spinlock in
eventfd_poll", 2015-02-17), eventfd is reading ctx->count outside
ctx->wqh.lock.

However, things aren't as simple as the read barrier in eventfd_poll
would suggest.  In fact, the read barrier, besides lacking a comment, is
not paired in any obvious manner with another read barrier, and it is
pointless because it is sitting between a write (deep in poll_wait) and
the read of ctx->count.  The read barrier is acting just as a compiler
barrier, for which we can use READ_ONCE instead.  This is what the code
change in this patch does.

The documentation change is just as important, however.  The question,
posed by Andrea Arcangeli, is then why the thing is safe on
architectures where spin_unlock does not imply a store-load memory
barrier.  The answer is that it's safe because writes of ctx->count use
the same lock as poll_wait, and hence an acquire barrier implicit in
poll_wait provides the necessary synchronization between eventfd_poll
and callers of wake_up_locked_poll.  This is sort of mentioned in the
commit message with respect to eventfd_ctx_read ("eventfd_read is
similar, it will do a single decrement with the lock held") but it
applies to all other callers too.  It's tricky enough that it should be
documented in the code.

Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
Reviewed-by: Andrea Arcangeli <aarcange@redhat.com>
Cc: Chris Mason <clm@fb.com>
Cc: Davide Libenzi <davidel@xmailserver.org>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>

diff --git a/fs/eventfd.c b/fs/eventfd.c
index ed70cf9fdc7b..1231cd1999d8 100644
--- a/fs/eventfd.c
+++ b/fs/eventfd.c
@@ -121,8 +121,46 @@ static unsigned int eventfd_poll(struct file *file, poll_table *wait)
 	u64 count;

 	poll_wait(file, &ctx->wqh, wait);
-	smp_rmb();
-	count = ctx->count;
+
+	/*
+	 * All writes to ctx->count occur within ctx->wqh.lock.  This read
+	 * can be done outside ctx->wqh.lock because we know that poll_wait
+	 * takes that lock (through add_wait_queue) if our caller will sleep.
+	 *
+	 * The read _can_ therefore seep into add_wait_queue's critical
+	 * section, but cannot move above it!  add_wait_queue's spin_lock acts
+	 * as an acquire barrier and ensures that the read be ordered properly
+	 * against the writes.  The following CAN happen and is safe:
+	 *
+	 *     poll                               write
+	 *     -----------------                  ------------
+	 *     lock ctx->wqh.lock (in poll_wait)
+	 *     count = ctx->count
+	 *     __add_wait_queue
+	 *     unlock ctx->wqh.lock
+	 *                                        lock ctx->qwh.lock
+	 *                                        ctx->count += n
+	 *                                        if (waitqueue_active)
+	 *                                          wake_up_locked_poll
+	 *                                        unlock ctx->qwh.lock
+	 *     eventfd_poll returns 0
+	 *
+	 * but the following, which would miss a wakeup, cannot happen:
+	 *
+	 *     poll                               write
+	 *     -----------------                  ------------
+	 *     count = ctx->count (INVALID!)
+	 *                                        lock ctx->qwh.lock
+	 *                                        ctx->count += n
+	 *                                        **waitqueue_active is false**
+	 *                                        **no wake_up_locked_poll!**
+	 *                                        unlock ctx->qwh.lock
+	 *     lock ctx->wqh.lock (in poll_wait)
+	 *     __add_wait_queue
+	 *     unlock ctx->wqh.lock
+	 *     eventfd_poll returns 0
+	 */
+	count = READ_ONCE(ctx->count);

 	if (count > 0)
 		events |= POLLIN;
```


## 如何理解 lock cookie

include/linux/backing-dev-defs.h
```c
struct wb_lock_cookie {
	bool locked;
	unsigned long flags;
};
```
例如在 folio_redirty_for_writepage 中，为什么要这么操作来着?

```c
		wb = unlocked_inode_to_wb_begin(inode, &cookie);
		current->nr_dirtied -= nr;
		node_stat_mod_folio(folio, NR_DIRTIED, -nr);
		wb_stat_mod(wb, WB_DIRTIED, -nr);
		unlocked_inode_to_wb_end(inode, &cookie);
```

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
