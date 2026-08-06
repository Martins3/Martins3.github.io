# rcu slab
<!-- a3ef8d50-cdb4-4049-b3d2-caf16ef47d58 -->

如果 kmem_cache 创建的时候带有 SLAB_TYPESAFE_BY_RCU 这个 flag ，
其影响释放的时候是否要等待一个 grace period 的。

经典使用:
1. mm/rmap.c 中的 anon_vma_cachep : 看不懂
2. https://lore.kernel.org/all/20241007-brauner-file-rcuref-v2-3-387e24dc9163@kernel.org/ : 也看不懂
3. 看看 refcount_set_release 前面的注释


```c
/**
 * define SLAB_TYPESAFE_BY_RCU - **WARNING** READ THIS!
 *
 * This delays freeing the SLAB page by a grace period, it does _NOT_
 * delay object freeing. This means that if you do kmem_cache_free()
 * that memory location is free to be reused at any time. Thus it may
 * be possible to see another object there in the same RCU grace period.
 *
 * This feature only ensures the memory location backing the object
 * stays valid, the trick to using this is relying on an independent
 * object validation pass. Something like:
 *
 * ::
 *
 *  begin:
 *   rcu_read_lock();
 *   obj = lockless_lookup(key);
 *   if (obj) {
 *     if (!try_get_ref(obj)) // might fail for free objects
 *       rcu_read_unlock();
 *       goto begin;
 *
 *     if (obj->key != key) { // not the object we expected
 *       put_ref(obj);
 *       rcu_read_unlock();
 *       goto begin;
 *     }
 *   }
 *  rcu_read_unlock();
 *
 * This is useful if we need to approach a kernel structure obliquely,
 * from its address obtained without the usual locking. We can lock
 * the structure to stabilize it and check it's still at the given address,
 * only if we can be sure that the memory has not been meanwhile reused
 * for some other kind of object (which our subsystem's lock might corrupt).
 *
 * rcu_read_lock before reading the address, then rcu_read_unlock after
 * taking the spinlock within the structure expected at that address.
 *
 * Note that object identity check has to be done *after* acquiring a
 * reference, therefore user has to ensure proper ordering for loads.
 * Similarly, when initializing objects allocated with SLAB_TYPESAFE_BY_RCU,
 * the newly allocated object has to be fully initialized *before* its
 * refcount gets initialized and proper ordering for stores is required.
 * refcount_{add|inc}_not_zero_acquire() and refcount_set_release() are
 * designed with the proper fences required for reference counting objects
 * allocated with SLAB_TYPESAFE_BY_RCU.
 *
 * Note that it is not possible to acquire a lock within a structure
 * allocated with SLAB_TYPESAFE_BY_RCU without first acquiring a reference
 * as described above.  The reason is that SLAB_TYPESAFE_BY_RCU pages
 * are not zeroed before being given to the slab, which means that any
 * locks must be initialized after each and every kmem_struct_alloc().
 * Alternatively, make the ctor passed to kmem_cache_create() initialize
 * the locks at page-allocation time, as is done in __i915_request_ctor(),
 * sighand_ctor(), and anon_vma_ctor().  Such a ctor permits readers
 * to safely acquire those ctor-initialized locks under rcu_read_lock()
 * protection.
 *
 * Note that SLAB_TYPESAFE_BY_RCU was originally named SLAB_DESTROY_BY_RCU.
 */
#define SLAB_TYPESAFE_BY_RCU	__SLAB_FLAG_BIT(_SLAB_TYPESAFE_BY_RCU)
```

## 经典案例
1. mm/rmap.c 中的 anon_vma_cachep 使用
SLAB_TYPESAFE_BY_RCU


## 为什么需要 SLAB_TYPESAFE_BY_RCU

它解决的问题是:通过"裸指针"间接接近一个内核对象(注释里的
"approach a kernel structure obliquely")。

典型场景:从某个不受锁保护的地方(比如 `page->mapping`)读到一个
对象地址,想在 `rcu_read_lock()` 下直接解引用它、对它做原子操作甚至
上锁。此时没有任何锁阻止另一个 CPU 同时把这个对象 `kmem_cache_free()`
掉。

如果没有这个 flag,free 之后有两层危险:

1. slab page 可能被还给 buddy allocator,这块内存随后被用作页表、
   其他结构的页、甚至用户页。读者此时往 `obj->refcount` 做一次
   `atomic_inc`,就是在随机篡改别人的内存(type confusion / UAF 写)。
2. 即使 page 没还,普通 RCU 读者习惯假设"grace period 内对象内容不变",
   但 slab 语义并不保证这一点。

通常的替代方案是 `kfree_rcu()`:把对象本身的释放推迟到 grace period
之后,读者在 RCU 读侧临界区内拿到的一定还是原对象。但代价是每个对象
的释放都要挂 RCU callback、延迟回收,anon_vma 这种分配/释放极其频繁
的对象开销太大,也破坏 slab 缓存热度。

`SLAB_TYPESAFE_BY_RCU` 是折中:

- 只把 slab page 归还页分配器推迟一个 grace period;
- 对象 slot 本身不延迟,`kmem_cache_free()` 后立刻可以被同 cache 的
  新对象复用。

所以读者拿到的保证更弱但够用:grace period 内,该地址上要么还是原
对象,要么是一个同类型的新对象,绝不会变成别的东西。代价转移给了
读者:必须做一次"验证流程"——先 `try_get_ref`(`atomic_inc_not_zero`,
对死对象会失败),再校验对象身份(key 是否匹配),不匹配就重试。这就是
WARNING 注释里 `begin:` 循环的含义。

另外两条内存序规则:新对象必须先完整初始化、最后才设置 refcount
(`refcount_set_release`);读者用 `refcount_inc_not_zero_acquire()`
拿引用。refcount 充当发布/获取的同步点。

单个对象释放仍然立即进入 freelist：

只有整个 slab 空闲、准备归还底层页面时，SLUB 才延迟释放：
```c
static void free_slab(struct kmem_cache *s, struct slab *slab)
{
	if (kmem_cache_debug_flags(s, SLAB_CONSISTENCY_CHECKS)) {
		void *p;

		slab_pad_check(s, slab);
		for_each_object(p, s, slab_address(slab), slab->objects)
			check_object(s, slab, p, SLUB_RED_INACTIVE);
	}

	if (unlikely(s->flags & SLAB_TYPESAFE_BY_RCU))
		call_rcu(&slab->rcu_head, rcu_free_slab);
	else
		__free_slab(s, slab, true);
}
```
此外，该标志禁止 slab cache 合并，防止同一地址被另一个 cache 类型使用：mm/slab_common.c:45。


所以 reader 必须在取得引用之后重新验证对象身份：
```c
struct object *lookup_get(unsigned long key)
{
	for (;;) {
		struct object *obj;

		rcu_read_lock();

		obj = lookup_rcu(key);
		if (!obj) {
			rcu_read_unlock();
			return NULL;
		}

		if (!refcount_inc_not_zero_acquire(&obj->refs)) {
			rcu_read_unlock();
			continue;
		}

		/* 取得引用之后，重新验证对象身份。 */
		if (READ_ONCE(obj->key) == key) {
			rcu_read_unlock();
			return obj;
		}

		put_object(obj);
		rcu_read_unlock();
	}
}
```


## anon_vma_cachep 的具体分析

`anon_vma_cachep` 在 `mm/rmap.c` 的 `anon_vma_init()` 创建时就带了
`SLAB_TYPESAFE_BY_RCU|SLAB_PANIC|SLAB_ACCOUNT`。它的读者和写者:

- 读者(无锁):`folio_get_anon_vma()` 和 `folio_lock_anon_vma_read()`。
  它们从 `folio->mapping` 里抠出 anon_vma 指针,持的是 folio lock,
  对 `folio_remove_rmap_*()` 完全没有串行化。
- 写者:最后一个 unmap 使 refcount 归零,`put_anon_vma()` ->
  `anon_vma_free()` -> `kmem_cache_free()`。

### 竞争窗口与 flag 如何兜底

`folio_get_anon_vma()` 的关键三步:

```c
anon_mapping = READ_ONCE(folio->mapping);  // 读到指针的瞬间,对象可能正被 free
...
if (!atomic_inc_not_zero(&anon_vma->refcount))  // (a)
...
if (!folio_mapped(folio)) { ... return NULL; }  // (b) 二次校验
```

- 步骤 (a) 是最危险的一击:如果指针已经悬空,`atomic_inc` 就是在写
  随机内存。`SLAB_TYPESAFE_BY_RCU` 保证 grace period 内这块地址上仍然
  躺着一个 `struct anon_vma`(旧的尸体,或刚复用的新对象),`refcount`
  字段偏移不变,所以这次 atomic 最多是给一个无辜的新 anon_vma 临时加
  了一次引用,不会 corrupt 内存。这正是 `folio_get_anon_vma()` 里注释
  说的 "SLAB_TYPESAFE_BY_RCU guarantees that - so the
  atomic_inc_not_zero() above cannot corrupt"。
- 步骤 (b) 是身份校验:如果 folio 还 mapped,由 anon_vma 的 lifetime
  规则(unmap 才减引用、child 链挂 root)保证拿到的 anon_vma 必然还
  活着;如果已经 unmapped,说明可能拿到了尸体或复用对象,
  `put_anon_vma()` 抵消掉刚才误加的引用,返回 NULL。误加再误减,对
  新对象是平衡的,无害。

### ctor 的角色

```c
static void anon_vma_ctor(void *data)
{
	struct anon_vma *anon_vma = data;

	init_rwsem(&anon_vma->rwsem);
	atomic_set(&anon_vma->refcount, 0);
	anon_vma->rb_root = RB_ROOT_CACHED;
}
```

ctor 只在 slab page 分配时跑一次,不是每次 `kmem_cache_alloc()` 都跑。
这带来两个关键性质:

- `rwsem` 在 page 生命周期内始终是初始化好的,所以
  `folio_lock_anon_vma_read()` 可以在 `rcu_read_lock()` 下直接对
  `root->rwsem` 做 `down_read_trylock()`,哪怕那个 slot 当前是死对象,
  锁本身也是合法的。这就是 WARNING 注释里 "locks must be initialized
  by ctor" 一段的实例。
- 死对象的 `refcount` 保持 0,读者的 `atomic_inc_not_zero()` 必然失败、
  走重试/放弃路径,不可能"复活"一个尸体。

对应地,`anon_vma_alloc()` 里 `atomic_set(&anon_vma->refcount, 1)` 放在
其他字段初始化之后,就是 WARNING 注释要求的"先完整初始化、最后设
refcount"的发布顺序。

### free 侧的同步

`anon_vma_free()` 里的 `rwsem_is_locked()` 检查,是和
`folio_lock_anon_vma_read()` 的 trylock 做的握手:读者
`down_read_trylock()` 成功在前,free 方就一定能观察到锁被持有,于是
自己拿一次写锁再释放,等读者走出临界区后才真正 `kmem_cache_free()`。
这保证了"读者持锁期间对象不会被释放",补上 trylock 路径没有引用计数
的缺口。

### 总结

anon_vma 是教科书式用法:读者从 `folio->mapping` 无锁取指针 -> flag
保证 `atomic_inc_not_zero` 不会写坏别人的内存 -> ctor 保证死对象
refcount 为 0 且 rwsem 永远已初始化 -> `folio_mapped` 二次校验和
lifetime 规则保证拿到的引用确实属于这个 folio -> free 方用 rwsem 握手
补齐持锁路径。每一步都依赖这个 flag 提供的"地址上类型不变"这一底线。

笔记前面列的另外两个点(brauner 的 file rcuref 系列、
`refcount_set_release` 注释)本质上是同一模式:rcuref 是把这套
"验证流程"封装成通用引用计数原语,
`refcount_set_release`/`refcount_inc_not_zero_acquire` 就是其中要求的
内存序配对。

## struct file

reader 侧的对比:
```c
static struct file *__get_file_rcu(struct file __rcu **f)
{
	struct file __rcu *file;
	struct file __rcu *file_reloaded;
	struct file __rcu *file_reloaded_cmp;

	file = rcu_dereference_raw(*f);
	if (!file)
		return NULL;

	if (unlikely(!file_ref_get(&file->f_ref)))
		return ERR_PTR(-EAGAIN);

	file_reloaded = rcu_dereference_raw(*f);

	/*
	 * Ensure that all accesses have a dependency on the load from
	 * rcu_dereference_raw() above so we get correct ordering
	 * between reuse/allocation and the pointer check below.
	 */
	file_reloaded_cmp = file_reloaded;
	OPTIMIZER_HIDE_VAR(file_reloaded_cmp);

	/*
	 * file_ref_get() above provided a full memory barrier when we
	 * acquired a reference.
	 *
	 * This is paired with the write barrier from assigning to the
	 * __rcu protected file pointer so that if that pointer still
	 * matches the current file, we know we have successfully
	 * acquired a reference to the right file.
	 *
	 * If the pointers don't match the file has been reallocated by
	 * SLAB_TYPESAFE_BY_RCU.
	 */
	if (file == file_reloaded_cmp)
		return file_reloaded;

	fput(file);
	return ERR_PTR(-EAGAIN);
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
