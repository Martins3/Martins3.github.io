# mempool
使用 mempool 的目的:
The purpose of mempools is to help out in situations where a memory allocation must succeed, but sleeping is not an option. To that end, mempools pre-allocate a pool of memory and reserve it until it is needed. [^16]
[^16]: [kernel doc : Driver porting: low-level memory allocation](https://lwn.net/Articles/22909/)

```txt
#0  r1bio_pool_alloc (gfp_flags=600064, data=0xffff888103acffd0) at drivers/md/raid1.c:131
#1  0xffffffff8133c353 in mempool_alloc (pool=pool@entry=0xffff888130eb10a0, gfp_mask=601088, gfp_mask@entry=3072) at mm/mempool.c:398
#2  0xffffffff81e27e52 in alloc_r1bio (bio=0xffff88810c470100, mddev=0xffff888107ca2000) at drivers/md/raid1.c:1209
```

- mempool_t::curr_nr : 现在还有多少存粮
- mempool_t::min_nr  : 初始化的 element 个数


- [ ] 如果存粮用玩了，如何?

## 如果一个正在 alloc ，但是正在调用 mempool_exit

- mempool_alloc
  - 使用特殊的 gfp_mask 首先分配一次，走标准路径
  - 如果失败，走 remove_element

在 mempool_init_node 预先分配内存

通过 mempool_t::lock 来实现互斥


## 基本的使用接口
- mempool_exit : 调用 mempool_t::free 接口将没有使用的元素的全部释放掉
- mempool_destroy : 将 mempool 也释放掉

## [ ] 如何理解这里的注释

```c
 */
void mempool_free(void *element, mempool_t *pool)
{
	unsigned long flags;

	if (unlikely(element == NULL))
		return;

	/*
	 * Paired with the wmb in mempool_alloc().  The preceding read is
	 * for @element and the following @pool->curr_nr.  This ensures
	 * that the visible value of @pool->curr_nr is from after the
	 * allocation of @element.  This is necessary for fringe cases
	 * where @element was passed to this task without going through
	 * barriers.
	 *
	 * For example, assume @p is %NULL at the beginning and one task
	 * performs "p = mempool_alloc(...);" while another task is doing
	 * "while (!p) cpu_relax(); mempool_free(p, ...);".  This function
	 * may end up using curr_nr value which is from before allocation
	 * of @p without the following rmb.
	 */
	smp_rmb();

	/*
	 * For correctness, we need a test which is guaranteed to trigger
	 * if curr_nr + #allocated == min_nr.  Testing curr_nr < min_nr
	 * without locking achieves that and refilling as soon as possible
	 * is desirable.
	 *
	 * Because curr_nr visible here is always a value after the
	 * allocation of @element, any task which decremented curr_nr below
	 * min_nr is guaranteed to see curr_nr < min_nr unless curr_nr gets
	 * incremented to min_nr afterwards.  If curr_nr gets incremented
	 * to min_nr after the allocation of @element, the elements
	 * allocated after that are subject to the same guarantee.
	 *
	 * Waiters happen iff curr_nr is 0 and the above guarantee also
	 * ensures that there will be frees which return elements to the
	 * pool waking up the waiters.
	 */
	if (unlikely(READ_ONCE(pool->curr_nr) < pool->min_nr)) {
		spin_lock_irqsave(&pool->lock, flags);
		if (likely(pool->curr_nr < pool->min_nr)) {
			add_element(pool, element);
			spin_unlock_irqrestore(&pool->lock, flags);
			wake_up(&pool->wait);
			return;
		}
		spin_unlock_irqrestore(&pool->lock, flags);
	}
	pool->free(element, pool->pool_data);
}
```

## 一个 mempool 的 element 是不可以 free 到另一个 mempool 中的

mempool 中支持持有存储指针，alloc 出来的元素大小是 mempool_s::alloc 决定的是靠当时

例如，一个存放 r1bio 的 mempool 的长度不是固定的
```c
static void * r1bio_pool_alloc(gfp_t gfp_flags, void *data)
{
	struct pool_info *pi = data;
	int size = offsetof(struct r1bio, bios[pi->raid_disks]);

	/* allocate a r1bio with room for raid_disks entries in the bios array */
	return kzalloc(size, gfp_flags);
}
```
所以是不能瞎鸡儿放置的。
