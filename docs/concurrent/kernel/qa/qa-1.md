## 什么时候需要使用 atomic 的版本

问题:
atomic_read_acquire(), atomic_set_release() 和 smp_load_acquire smp_store_release 有区别吗?

既然在内核中，atomic 在仅仅 load 或者 store 的时候，不是自动退化了吗?


简单找到如下的例子，可以发现，如果使用了 atomic_set_release 的，那么其他的位置一定只有
atomic_read_acquire ，一定会有 atomic 专属的操作。

### refcount
```c
/**
 * refcount_set_release - set a refcount's value with release ordering
 * @r: the refcount
 * @n: value to which the refcount will be set
 *
 * This function should be used when memory occupied by the object might be
 * reused to store another object -- consider SLAB_TYPESAFE_BY_RCU.
 *
 * Provides release memory ordering which will order previous memory operations
 * against this store. This ensures all updates to this object are visible
 * once the refcount is set and stale values from the object previously
 * occupying this memory are overwritten with new ones.
 *
 * This function should be called only after new object is fully initialized.
 * After this call the object should be considered visible to other tasks even
 * if it was not yet added into an object collection normally used to discover
 * it. This is because other tasks might have discovered the object previously
 * occupying the same memory and after memory reuse they can succeed in taking
 * refcount to the new object and start using it.
 */
static inline void refcount_set_release(refcount_t *r, int n)
{
	atomic_set_release(&r->refs, n);
}
```

发现有对应的:
```c
static inline void __refcount_dec(refcount_t *r, int *oldp)
{
	int old = atomic_fetch_sub_release(1, &r->refs);

	if (oldp)
		*oldp = old;

	if (unlikely(old <= 1))
		refcount_warn_saturate(r, REFCOUNT_DEC_LEAK);
}
```

### static_key_slow_inc_cpuslocked

```c
	if (!atomic_cmpxchg(&key->enabled, 0, -1)) {
		jump_label_update(key);
		/*
		 * Ensure that when static_key_fast_inc_not_disabled() or
		 * static_key_dec_not_one() observe the positive value,
		 * they must also observe all the text changes.
		 */
		atomic_set_release(&key->enabled, 1);
```

### trigger_backtrace

```c
		if (atomic_cmpxchg_acquire(&per_cpu(trigger_backtrace, cpu), 1, 0))
			dump_cpu_task(cpu);
		if (!cpu_cur_csd) {
			pr_alert("csd: Re-sending CSD lock (#%d) IPI from CPU#%02d to CPU#%02d\n", *bug_id, raw_smp_processor_id(), cpu);
			arch_send_call_function_single_ipi(cpu);
		}
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
