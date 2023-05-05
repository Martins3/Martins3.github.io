# kernel lock questions

## [ ] 在 64bit 中间 kmap 和 `kmap_atomic` 的区别

```c
static inline void *kmap_atomic(struct page *page)
{
    // 经过了简化的代码
    preempt_disable();
    pagefault_disable();
    return page_address(page);
}

static inline void *kmap(struct page *page)
{
	might_sleep();
	return page_address(page);
}
```

唯一的区别是此时另一个 CPU 是否可以触发 page fault:
```txt
#0  pagefault_disabled () at ./include/linux/uaccess.h:250
#1  do_user_addr_fault (regs=0xffffc900015fff58, error_code=7, address=140063241728256) at arch/x86/mm/fault.c:1292
#2  0xffffffff822a9ad3 in handle_page_fault (address=140063241728256, error_code=7, regs=0xffffc900015fff58) at arch/x86/mm/fault.c:1534
#3  exc_page_fault (regs=0xffffc900015fff58, error_code=7) at arch/x86/mm/fault.c:1590
#4  0xffffffff82401286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
pagefault_disabled 中:
```c
	/*
	 * If we're in an interrupt, have no user context or are running
	 * in a region with pagefaults disabled then we must not take the fault
	 */
	if (unlikely(faulthandler_disabled() || !mm)) {
		bad_area_nosemaphore(regs, error_code, address);
		return;
	}
```

通过一个简单的数值增加来:
```c
/*
 * These routines enable/disable the pagefault handler. If disabled, it will
 * not take any locks and go straight to the fixup table.
 *
 * User access methods will not sleep when called from a pagefault_disabled()
 * environment.
 */
static inline void pagefault_disable(void)
{
	pagefault_disabled_inc();
	/*
	 * make sure to have issued the store before a pagefault
	 * can hit.
	 */
	barrier();
}
```
使用 kmap_atomic 的时候，将会禁止 page fault handler

不太理解的问题是:
1. 为什么需要 kmap_atomic 的常见需要 disable page fault handler
  - 检查 pagefault_disable 的位置，还是很多的
2. pagefault_disable 中使用的 barrer 可以优化一下为函数吗? 例如 smp_read_barrier
3. 为什么 pagefault_disabled_inc 只是简单的 ++ ，而不是 atomic 的

## [ ] 是不是说 folio_try_get 成功之后，就不可能修改 folio_test_lru(folio) 的结果

```c
static struct folio *page_idle_get_folio(unsigned long pfn)
{
	struct page *page = pfn_to_online_page(pfn);
	struct folio *folio;

	if (!page || PageTail(page))
		return NULL;

	folio = page_folio(page);
	if (!folio_test_lru(folio) || !folio_try_get(folio))
		return NULL;
	if (unlikely(page_folio(page) != folio || !folio_test_lru(folio))) {
		folio_put(folio);
		folio = NULL;
	}
	return folio;
}
```
首先，观察到 page_idle_get_folio 中存在两次 folio_try_get ，所以想到存在如此证据，至于为什么
存在题目中保证，暂时不清楚。

## arch_freq_get_on_cpu 中为什么使用 read_seqcount_retry

## [ ] 从 folio_try_get 的细节分析


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

## io uring 的 memory barrier 于用户态和内核态之间

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
