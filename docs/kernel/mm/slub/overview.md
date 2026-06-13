# slub

## 结构体 kmem_cache

```c
/*
 * Slab cache management.
 */
struct kmem_cache {
#ifndef CONFIG_SLUB_TINY
	struct kmem_cache_cpu __percpu *cpu_slab;
#endif
	/* Used for retrieving partial slabs, etc. */
	slab_flags_t flags;
	unsigned long min_partial;
	unsigned int size;		/* Object size including metadata */
	unsigned int object_size;	/* Object size without metadata */
	struct reciprocal_value reciprocal_size;
	unsigned int offset;		/* Free pointer offset */
#ifdef CONFIG_SLUB_CPU_PARTIAL
	/* Number of per cpu partial objects to keep around */
	unsigned int cpu_partial;
	/* Number of per cpu partial slabs to keep around */
	unsigned int cpu_partial_slabs;
#endif
	struct kmem_cache_order_objects oo;

	/* Allocation and freeing of slabs */
	struct kmem_cache_order_objects min;
	gfp_t allocflags;		/* gfp flags to use on each alloc */
	int refcount;			/* Refcount for slab cache destroy */
	void (*ctor)(void *object);	/* Object constructor */
	unsigned int inuse;		/* Offset to metadata */
	unsigned int align;		/* Alignment */
	unsigned int red_left_pad;	/* Left redzone padding size */
	const char *name;		/* Name (only for display!) */
	struct list_head list;		/* List of slab caches */
#ifdef CONFIG_SYSFS
	struct kobject kobj;		/* For sysfs */
#endif

	/*
	 * Defragmentation by allocating from a remote node.
	 */
	unsigned int remote_node_defrag_ratio;

	struct kmem_cache_node *node[MAX_NUMNODES];
};
```
- kmem_cache::list 将所有的 struct kmem_cache 穿起来


## 结构体 kmem_cache_node
```c
/*
 * The slab lists for all objects.
 */
struct kmem_cache_node {
	spinlock_t list_lock;
	unsigned long nr_partial;
	struct list_head partial;
#ifdef CONFIG_SLUB_DEBUG
	atomic_long_t nr_slabs;
	atomic_long_t total_objects;
	struct list_head full;
#endif
};
```

## 结构体 kmem_cache_cpu
<!-- 23f0aefe-9938-474d-8080-7385de8f5a08 -->

```c
/*
 * When changing the layout, make sure freelist and tid are still compatible
 * with this_cpu_cmpxchg_double() alignment requirements.
 */
struct kmem_cache_cpu {
	union {
		struct {
			void **freelist;	/* Pointer to next available object */
			unsigned long tid;	/* Globally unique transaction id */
		};
		freelist_aba_t freelist_tid;
	};
	struct slab *slab;	/* The slab from which we are allocating */
#ifdef CONFIG_SLUB_CPU_PARTIAL
	struct slab *partial;	/* Partially allocated slabs */
#endif
	local_lock_t lock;	/* Protects the fields above */
```
问题:
1. freelist_tid 居然是一共 union 啊?
2. kmem_cache_cpu 中只是关联一个 slab 或者 partial 吗?
3. 为什么需要 lock ?

## 结构体 slab
<!-- 5182fe53-f7d0-467b-b064-264a881dbd8b -->

一个 struct slab 就是一个页面。
```c
/* Reuses the bits in struct page */
struct slab {
	unsigned long __page_flags;

	struct kmem_cache *slab_cache;
	union {
		struct {
			union {
				struct list_head slab_list;
				// 如果在 kmem_cache_node 中，slab_list 用于将所有的 slab 连起来
				// 一个 slab 中有多个 page ，这些 page 都是连续的，所以不需要额外的数据结构
#ifdef CONFIG_SLUB_CPU_PARTIAL
				struct { // 如果 kmem_cache_cpu 中
					struct slab *next;
					int slabs;	/* Nr of slabs left */
				};
#endif
			};
			/* Double-word boundary */
			union {
				struct {
					void *freelist;		/* first free object */
					union {
						unsigned long counters;
						struct {
							unsigned inuse:16;
							unsigned objects:15;
							/*
							 * If slab debugging is enabled then the
							 * frozen bit can be reused to indicate
							 * that the slab was corrupted
							 */
							unsigned frozen:1;
						};
					};
				};
#ifdef system_has_freelist_aba
				freelist_aba_t freelist_counter;
#endif
			};
		};
		struct rcu_head rcu_head;
	};

	unsigned int __page_type;
	atomic_t __page_refcount;
#ifdef CONFIG_SLAB_OBJ_EXT
	unsigned long obj_exts;
#endif
};
```

- rcu_head 和 anonymous struct 同一个层次
- anonymous struct 有两个 union
    - 管理 slab :
      - 如果在 node 中 : slab_list
      - 如果在 cpu partial 中 :
    - object 管理
      - 一个是 freelist :  object 链表
      - inuse objects : 记录 object 的数量

1. `slub->objects` 表示一个 struct slub 中间可以持有的所有的 objects 总数
，但是，struct kmem_cache 不是已经持有了吗? 简单检查了下，应该只是用于缓存 struct kmem_cache 中的内容的
```c
static int count_inuse(struct slab *slab)
{
	return slab->inuse;
}

static int count_total(struct slab *slab)
{
  return slab->objects;
}
```

2. slab::freelist : 指向 page 中间第一个 free 的地址，通过 `on_freelist` 可以很好的理解链表的遍历过程。
    1. 当该 page 上的所有的 object 都被使用的时候，那么 freelist = NULL

## struct slab 中的 ruc_head 是做什么的?

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

## CONFIG_SLUB_CPU_PARTIAL 的作用是什么?
<!-- 1db4ded5-8e15-4b8a-8006-b65e12710680 -->

主要问题在于，当 cpu_partial 中太多，需要把他们都移动到 node partial 中去:
```txt
config SLUB_CPU_PARTIAL
	default y
	depends on SLUB && SMP
	bool "SLUB per cpu partial cache"
	help
	  Per cpu partial caches accellerate objects allocation and freeing
	  that is local to a processor at the price of more indeterminism
	  in the latency of the free. On overflow these caches will be cleared
	  which requires the taking of locks that may cause latency spikes.
	  Typically one would choose no for a realtime system.
```

## pagesperslab

slub 一次分配多个 page ，例如 ext4_inode_cache 一个 slab 占据 8 个 page ，他们都是连续的
```txt
# name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab> : tunables <limit> <batchcount> <sharedfactor> : sla
bdata <active_slabs> <num_slabs> <sharedavail>
ext4_inode_cache    5017   5017   1096   29    8 : tunables    0    0    0 : slabdata    173    173      0
```

具体分配页面的地方: alloc_slab_page

所以，一个 slab 指的是一个 folio ，而不是一个 page 。

## Keynote

- oo_objects() : 一个 slab 中包含多少个 object
- oo_order() : 一个 slab 占用 2 ^^ order 个 page

- kzalloc_node 可以让分配的 node 的位置确定，这进一步在要求 slub 的实现。

内核 5.17 引入了 struct slab，描述了由 slab 分配器所管理的内存页面。
这个结构经过精心设计，与 struct page 可以完全 overlay 起来，也不会破坏其他用途下的结构字段。
这个改动不改变数据仍然存储在跟之前相同的 page struct 中的这个情况，但它使 slab 相关的部分变得更明确了，

也确保了 struct page 的其余部分在 slab 分配器中不再使用。
1. kmalloc_slab 分析大小确定其中的 size
3. get_freepointer(s, object) 返回 `object + s->offset`
3. kmem_cache 只是创建一次，管理大小相同的区域。

1. If all of the objects within a given slab (as tracked by the **inuse** counter) are freed, the entire slab is given back to the page allocator for reuse.
    1. 找到利用 inuse 然后释放的
    2. a slab 其实指的是 a page managed by the slab


## slowpath

```c
/*
 * Another one that disabled interrupt and compensates for possible
 * cpu changes by refetching the per cpu area pointer.
 */
static void *__slab_alloc(struct kmem_cache *s, gfp_t gfpflags, int node,
			  unsigned long addr, struct kmem_cache_cpu *c)
{
	void *p;
	unsigned long flags;

	local_irq_save(flags); // 用于 disable interrupt 的作用是什么 ? todo 和 preempted 的关系是什么 ?
#ifdef CONFIG_PREEMPT
	/*
	 * We may have been preempted and rescheduled on a different
	 * cpu before disabling interrupts. Need to reload cpu area
	 * pointer.
	 */
	c = this_cpu_ptr(s->cpu_slab); // 如果已经被抢占了，那么刷新一下 kmem_cache_cpu
#endif

	p = ___slab_alloc(s, gfpflags, node, addr, c);
	local_irq_restore(flags); // todo 应该是用于实现 lockless 吧!
	return p;
}
```

## (1)freed to the regular freelist
2. pfmemalloc_match 是干什么的 ?
1. 什么叫做 regular freelist ?

## flush 机制

```c
kmem_cache_init
	cpuhp_setup_state_nocalls(CPUHP_SLUB_DEAD, "slub:dead", NULL, slub_cpu_dead);  // slub_cpu_dead 的唯一调用，垃圾hotplug

/*
 * Use the cpu notifier to insure that the cpu slabs are flushed when
 * necessary.
 */
static int slub_cpu_dead(unsigned int cpu)
{
	struct kmem_cache *s;
	unsigned long flags;

	mutex_lock(&slab_mutex);
	list_for_each_entry(s, &slab_caches, list) {
		local_irq_save(flags);
		__flush_cpu_slab(s, cpu);
		local_irq_restore(flags);
	}
	mutex_unlock(&slab_mutex);
	return 0;
}


/*
 * Flush cpu slab.
 *
 * Called from IPI handler with interrupts disabled.
 */
static inline void __flush_cpu_slab(struct kmem_cache *s, int cpu)
{
	struct kmem_cache_cpu *c = per_cpu_ptr(s->cpu_slab, cpu);

	if (likely(c)) {
		if (c->page)
			flush_slab(s, c);

		unfreeze_partials(s, c); // 当需要使用的时候，那么就 freeze 上 !
	}
}

// 终于，flush_slab 和 unfreeze_partials 遇到了 !

static inline void flush_slab(struct kmem_cache *s, struct kmem_cache_cpu *c)
{
	stat(s, CPUSLAB_FLUSH);
	deactivate_slab(s, c->page, c->freelist, c);

	c->tid = next_tid(c->tid);
}

// todo deactivate_slab 存在一些不得了的东西的呀 !
```

## deactivate_slab
1. 对于 page 上所有的 object 全部释放
2. 还会处理向 node 的 partial 以及 full 中间添加，以及释放
    1. 但是添加的规则是什么，并不知道

```c
/*
 * Remove the cpu slab
 */
static void deactivate_slab(struct kmem_cache *s, struct page *page,
				void *freelist, struct kmem_cache_cpu *c)
{
	enum slab_modes { M_NONE, M_PARTIAL, M_FULL, M_FREE };
	struct kmem_cache_node *n = get_node(s, page_to_nid(page));
	int lock = 0;
	enum slab_modes l = M_NONE, m = M_NONE;
	void *nextfree;
	int tail = DEACTIVATE_TO_HEAD;
	struct page new;
	struct page old;

	if (page->freelist) {
		stat(s, DEACTIVATE_REMOTE_FREES);
		tail = DEACTIVATE_TO_TAIL;
	}

	/*
	 * Stage one: Free all available per cpu objects back
	 * to the page freelist while it is still frozen. Leave the
	 * last one.
	 *
	 * There is no need to take the list->lock because the page
	 * is still frozen.
	 */
	while (freelist && (nextfree = get_freepointer(s, freelist))) {
		void *prior;
		unsigned long counters;

		do {
			prior = page->freelist;
			counters = page->counters;
			set_freepointer(s, freelist, prior);
			new.counters = counters;
			new.inuse--;
			VM_BUG_ON(!new.frozen);

		} while (!__cmpxchg_double_slab(s, page,
			prior, counters,
			freelist, new.counters,
			"drain percpu freelist"));

		freelist = nextfree;
	}

	/*
	 * Stage two: Ensure that the page is unfrozen while the
	 * list presence reflects the actual number of objects
	 * during unfreeze.
	 *
	 * We setup the list membership and then perform a cmpxchg
	 * with the count. If there is a mismatch then the page
	 * is not unfrozen but the page is on the wrong list.
	 *
	 * Then we restart the process which may have to remove
	 * the page from the list that we just put it on again
	 * because the number of objects in the slab may have
	 * changed.
	 */
redo:

	old.freelist = page->freelist;
	old.counters = page->counters;
	VM_BUG_ON(!old.frozen);

	/* Determine target state of the slab */
	new.counters = old.counters;
	if (freelist) {
		new.inuse--;
		set_freepointer(s, freelist, old.freelist);
		new.freelist = freelist;
	} else
		new.freelist = old.freelist;

	new.frozen = 0;

  // 进行判断，得到 m 的内容
	if (!new.inuse && n->nr_partial >= s->min_partial)
		m = M_FREE;
	else if (new.freelist) {
		m = M_PARTIAL;
		if (!lock) {
			lock = 1;
			/*
			 * Taking the spinlock removes the possibility
			 * that acquire_slab() will see a slab page that
			 * is frozen
			 */
			spin_lock(&n->list_lock);
		}
	} else {
		m = M_FULL;
		if (kmem_cache_debug(s) && !lock) {
			lock = 1;
			/*
			 * This also ensures that the scanning of full
			 * slabs from diagnostic functions will not see
			 * any frozen slabs.
			 */
			spin_lock(&n->list_lock);
		}
	}

  // 根据分析的结果进行对应的处理
	if (l != m) {
		if (l == M_PARTIAL)
			remove_partial(n, page); // 将其从node 的 partial 上移开
		else if (l == M_FULL)
			remove_full(s, n, page); // 将其从 node fulll 上移动

		if (m == M_PARTIAL)
			add_partial(n, page, tail);
		else if (m == M_FULL)
			add_full(s, n, page);
	}

	l = m; // 这是神仙看的内容
	if (!__cmpxchg_double_slab(s, page,
				old.freelist, old.counters,
				new.freelist, new.counters,
				"unfreezing slab"))
		goto redo;

	if (lock)
		spin_unlock(&n->list_lock);

	if (m == M_PARTIAL)
		stat(s, tail);
	else if (m == M_FULL)
		stat(s, DEACTIVATE_FULL);
	else if (m == M_FREE) {
		stat(s, DEACTIVATE_EMPTY);
		discard_slab(s, page); // 释放到 slab 中间
		stat(s, FREE_SLAB);
	}

	c->page = NULL;
	c->freelist = NULL;
}
```

## alloc
- new_slab_objects : allocate and initialize it as a slab page, it called we can't find a page in partial
  -  get_partial
    - get_partial_node : 从 特定的一个 node 中间获取一个 paritial page 出来
        - acquire_slab : Remove slab from the partial list, freeze it and return the pointer to the freelist.
        - put_cpu_partial() : 将被 freeze 的 page 放到 Put a page that was just frozen (in `__slab_free`|get_partial_node) into a partial page slot if available.
            -  unfreeze_partials() : move all the pages in cpu partial list to node partial list, and unfreeze them.
              - add_partial : 将 page 挂载到 kmem_cache_node::paritial 上
              - discard_slab : 如果 node 持有 paritial page 的数量超过了限制, 而且该 page 没有被使用, 那么将该 page 释放到 buddy system 中间
    - get_any_partial : 如果从当前 node 中间找不到 paritial, 那么就从任意的 numa 上找
  - new_slab : 如果无法从 paritial 中间获取到资源，那么就只好分配新的页面这个样子了


## 看看
- [ ] [LoyenWang](https://www.cnblogs.com/LoyenWang/p/11922887.html)
- [ ] https://lwn.net/Articles/1010667/
- [ ] https://lwn.net/Articles/1016001/
- [ ] https://mp.weixin.qq.com/s/TLRCo3brKB3Fx2o-ZCCWAg
- [ ] https://mp.weixin.qq.com/s/qv5ljnsn77OJndgSdH4HVw
- [ ] https://www.kernel.org/doc/html/latest/mm/slub.html
    - debugfs 真的存在吗?
- https://richardweiyang-2.gitbook.io/kernel-exploring/00-memory_a_bottom_up_view/09-slub_in_graph : 不错

## 如何理解 slab 中的 frozen
<!-- 683c1fd3-817e-4f6f-a573-a2f85a3a9023 -->

参考 **用芯探核:基于龙芯的 Linux 内核探索解析**

P215 指出，一共五种情况，
- 从本地无锁(kmem_cache_cpu::freelist)
- 本地常规(kmem_cache_cpu::slab)
- kmem_cache_cpu::partial
- node partial
- buddy system

参考 P205 中的图 4-11 的说明:
1. kmem_cache_cpu::freelist 是管理本地分配和释放的
2. 调用 CPU A 上调用 slab_alloc 分配的对象，可能在 CPU B 释放，这种数据被放到 kmem_cache_cpu::slab::freelist 上管理
3. 当 kmem_cache_cpu::freelist 的用完了，那么就将 kmem_cache_cpu::slab::freelist 管理的 objects 全部转移给 kmem_cache_cpu::freelist, 如果 kmem_cache_cpu::page::freelist 也没有了，那么就需要从本地 partial 开始查找了。

- [ ] page 214 深入的分析了 `__slab_free`，这个之后，算是就清楚了，可以继续看看

一个 slab 被某个 CPU 冻结之后，就只有这个 CPU 能够从这个 slab 分配对象。
具体那些 slab 是冻结的 ?
1. 首先，本地 slab (kmem_cache_cpu::slab) 总是被冻结的；
2. 其次，脱离链表的全满 slab 在重新获得一个被释放的空闲对象后， 会进入本地 partial 链表并且被冻结;
3. 而其他的 slab, 如 numa node partial list, 刚刚从 numa node list 移入本地 partial 中的 slab,
以及脱离链表的全满 slab 都是不冻结的；

问题:
1. 为什么 frozen 要这么设计?
  - slab 中的 object 可以是不同的 CPU 分配的
  - slab 中的 object 可以在 CPU A 中分配，然后在 CPU B 中释放
2. 这么看，本地 partial slab 可以给其他 CPU 用于分配吗?
2. 在 `__slab_free` 中间，是唯一利用 frozen 的判断，其余的存在检查 frozen 的正确性
4. freeze_slab 的调用位置
5. 关键的问题在于 : 似乎有的 cpu partial 不是被冻结的

再看看内核中一点小小的注释:
```txt
Frozen slabs

If a slab is frozen then it is exempt from list management. It is
the cpu slab which is actively allocated from by the processor that
froze it and it is not on any list. The processor that froze the
slab is the one who can perform list operations on the slab. Other
processors may put objects onto the freelist but the processor that
froze the slab is the only one that can retrieve the objects from the
slab's freelist.
```
至少，可以确定的就是被 freeze 之后的，那么就只有该 CPU 可以访问。

在看看 gemini 的解释:

一个 slab 必须先被 frozen，才有资格被放到 cpu_partial 列表中。

1. 起点：全局列表 (node->partial)
  - 一个部分空闲的 slab 最初位于一个 NUMA 节点共享的 partial 列表中。访问这个列表需要获取一个全局性的 list_lock，这是性能瓶颈所在。
2. 变为 frozen (获得“本地户口”)
  - 当一个 CPU 核心（比如 CPU-0）需要分配内存，并且它的本地 cpu_slab 用完了，它就会去获取 list_lock，并从全局的 node->partial 列表中取出一个 slab。
  - 在将这个 slab 据为己有之前，它会给这个 slab 打上一个标记——将其状态设置为 frozen。
  - frozen 的核心含义：“这个 slab 现在归我 CPU-0 私有，它已从全局链表管理中豁免，其他 CPU 不能再从它上面分配对象。所有对它的列表操作（比如移动它）都只能由我 CPU-0 进行。”
  - 然后，这个被 frozen 的 slab 成为了 CPU-0 的当前活动 slab (cpu_slab)，用于满足接下来的分配请求。
3. 进入 cpu_partial 列表 (进入“本地缓存区”)
  - CPU-0 持续从它的 cpu_slab 中分配对象。
  - 假设此时，CPU-0 上的一个任务释放了一个对象，而这个对象恰好属于另一个更满的、同样被 CPU-0 frozen 的 slab。为了优化，SLUB 可能会决定将那个更满的 slab 切换为新的 cpu_slab。
  - 那么，旧的、刚刚还在使用的 cpu_slab（它现在是部分空闲的）该何去何从？它不能立即被“解冻”并放回全局的 node->partial 列表，因为那样又需要去获取昂贵的 list_lock。
  - 最佳选择：将这个旧的、但仍然是 frozen 状态的 slab，放入 CPU-0 的 cpu_partial 列表中。
  - cpu_partial 的核心含义：这是一个 per-CPU 的、私有的缓存列表，专门存放那些**曾经是活动 cpu_slab、现在暂时不用、但仍然归当前 CPU 所有（即 frozen 状态）**的部分空闲 slab。
4. 从 cpu_partial 中重新激活
  - 当 CPU-0 再次需要一个新的 slab 时，它会优先检查自己的 cpu_partial 列表。
  - 如果它在 cpu_partial 中找到了一个合适的 slab，它就可以直接、无锁地将其重新提升为活动的 cpu_slab。
  - 这才是这个机制的巨大性能优势所在：它避免了再次去争抢全局的 list_lock。

unfreeze 的时候:
- 刷新（Flush）Per-CPU 的 cpu_partial
- 釋放一個完全變空的 Slab 进入到 buddy system 中去

**凍結一個 slab 是為了讓它能成為當前活躍的 cpu_slab**。
cpu_partial 只是這個活躍 slab 在“輪休”時的暫存位置。
直接把它放到 cpu_partial 裡，CPU 還是沒有可用的 slab 來進行分配，
問題沒有解決。


但是看代码，似乎没有那么难

首先，从 node partial 中获取一个 slab ，然后 freeze ，freeze
```txt
	slab = get_partial(s, node, &pc); // 这个自动会从 partial 中把这个 slab 移除掉
		freelist = freeze_slab(s, slab);
```

不过还是没有搞清楚的问题，
2. 一个 slab 如果是 frozen 的，如何阻止其他的 cpu 分配的，具体的代码在哪里?


## slub 的锁规则
<!-- 3ba5e099-9032-40c3-9175-f50397620809 -->

用这个东西辅助理解一下吧

### 1. 锁的分层体系 (Lock Hierarchy)

SLUB 遵循严格的加锁顺序，以防止死锁。从宏观到微观依次为：

1. **slab_mutex**: 全局大锁，保护所有 slab 的列表，处理内存热插拔等重大元数据修改。
2. **node->list_lock**: 节点（NUMA Node）级别的自旋锁，保护该节点下的“部分空闲列表（partial list）”。
3. **kmem_cache->cpu_slab->lock**: 本地 CPU 锁，保护每个 CPU 私有的缓存信息。
4. **slab_lock**: 针对单个 slab 页面的锁（仅在不支持双字原子交换 `cmpxchg_double` 的架构上使用）。

### 2. 核心状态：冻结 (Frozen)

“冻结”是 SLUB 提高性能的神来之笔。

* **什么是冻结？** 当一个 slab 被某个 CPU 选中作为“当前分配源”时，它会被标记为 `frozen`。
* **特权：** 只有冻结它的那个 CPU 才能从该 slab 中提取对象。
* **豁免权：** 被冻结的 slab 会从节点（Node）的公共管理链表中剔除。
* **优势：** 既然这个 slab 是我这个 CPU 私有的，我分配内存时就不用去抢全局锁，从而实现了**无锁快路径（Lockless Fastpath）**。

### 3. Slab 的四种身份

文档总结了根据 `PG_Workingset` 标志位和 `frozen` 状态区分的四种 slab：

| 身份 | 状态组合 | 描述 |
| --- | --- | --- |
| **Node partial** | `Workingset` + `!frozen` | 放在内存节点公共列表上的半空 slab。 |
| **CPU partial** | `!Workingset` + `!frozen` | 预留在 CPU 本地以备后用的半空 slab（不冻结但也不在公共列表）。 |
| **CPU slab** | `!Workingset` + `frozen` | **当前正在使用**的分配来源，性能最高。 |
| **Full slab** | `!Workingset` + `!frozen` | 已满的 slab，通常不进链表，除非为了调试。 |

### 4. 分配路径：快与慢

* **快路径 (Fast Path):** * 直接从当前 CPU 的本地 `cpu_slab` 分配。
* 利用 `tid`（事务 ID）和 `cmpxchg` 原子操作，**完全无锁**，甚至不关中断。
* **慢路径 (Slow Path):** * 当本地 CPU 的 slab 用完了，就需要去“部分空闲列表”里找，这时会触发 `list_lock` 甚至申请新的物理页。

### 5. 关键字段及其保护对象

如果架构不支持高效的原子操作，`slab_lock` 会保护以下字段：

* `freelist`: 空闲对象链表。
* `inuse`: 已使用的对象数量。
* `objects`: slab 总共能容纳的对象数。
* `frozen`: 冻结状态位。

```c
/*
 * Lock order:
 *   1. slab_mutex (Global Mutex)
 *   2. node->list_lock (Spinlock)
 *   3. kmem_cache->cpu_slab->lock (Local lock)
 *   4. slab_lock(slab) (Only on some arches)
 *   5. object_map_lock (Only for debugging)
 *
 *   slab_mutex
 *
 *   The role of the slab_mutex is to protect the list of all the slabs
 *   and to synchronize major metadata changes to slab cache structures.
 *   Also synchronizes memory hotplug callbacks.
 *
 *   slab_lock
 *
 *   The slab_lock is a wrapper around the page lock, thus it is a bit
 *   spinlock.
 *
 *   The slab_lock is only used on arches that do not have the ability
 *   to do a cmpxchg_double. It only protects:
 *
 *	A. slab->freelist	-> List of free objects in a slab
 *	B. slab->inuse		-> Number of objects in use
 *	C. slab->objects	-> Number of objects in slab
 *	D. slab->frozen		-> frozen state
 *
 *   Frozen slabs
 *
 *   If a slab is frozen then it is exempt from list management. It is
 *   the cpu slab which is actively allocated from by the processor that
 *   froze it and it is not on any list. The processor that froze the
 *   slab is the one who can perform list operations on the slab. Other
 *   processors may put objects onto the freelist but the processor that
 *   froze the slab is the only one that can retrieve the objects from the
 *   slab's freelist.
 *
 *   CPU partial slabs
 *
 *   The partially empty slabs cached on the CPU partial list are used
 *   for performance reasons, which speeds up the allocation process.
 *   These slabs are not frozen, but are also exempt from list management,
 *   by clearing the PG_workingset flag when moving out of the node
 *   partial list. Please see __slab_free() for more details.
 *
 *   To sum up, the current scheme is:
 *   - node partial slab: PG_Workingset && !frozen
 *   - cpu partial slab: !PG_Workingset && !frozen
 *   - cpu slab: !PG_Workingset && frozen
 *   - full slab: !PG_Workingset && !frozen
 *
 *   list_lock
 *
 *   The list_lock protects the partial and full list on each node and
 *   the partial slab counter. If taken then no new slabs may be added or
 *   removed from the lists nor make the number of partial slabs be modified.
 *   (Note that the total number of slabs is an atomic value that may be
 *   modified without taking the list lock).
 *
 *   The list_lock is a centralized lock and thus we avoid taking it as
 *   much as possible. As long as SLUB does not have to handle partial
 *   slabs, operations can continue without any centralized lock. F.e.
 *   allocating a long series of objects that fill up slabs does not require
 *   the list lock.
 *
 *   For debug caches, all allocations are forced to go through a list_lock
 *   protected region to serialize against concurrent validation.
 *
 *   cpu_slab->lock local lock
 *
 *   This locks protect slowpath manipulation of all kmem_cache_cpu fields
 *   except the stat counters. This is a percpu structure manipulated only by
 *   the local cpu, so the lock protects against being preempted or interrupted
 *   by an irq. Fast path operations rely on lockless operations instead.
 *
 *   On PREEMPT_RT, the local lock neither disables interrupts nor preemption
 *   which means the lockless fastpath cannot be used as it might interfere with
 *   an in-progress slow path operations. In this case the local lock is always
 *   taken but it still utilizes the freelist for the common operations.
 *
 *   lockless fastpaths
 *
 *   The fast path allocation (slab_alloc_node()) and freeing (do_slab_free())
 *   are fully lockless when satisfied from the percpu slab (and when
 *   cmpxchg_double is possible to use, otherwise slab_lock is taken).
 *   They also don't disable preemption or migration or irqs. They rely on
 *   the transaction id (tid) field to detect being preempted or moved to
 *   another cpu.
 *
 *   irq, preemption, migration considerations
 *
 *   Interrupts are disabled as part of list_lock or local_lock operations, or
 *   around the slab_lock operation, in order to make the slab allocator safe
 *   to use in the context of an irq.
 *
 *   In addition, preemption (or migration on PREEMPT_RT) is disabled in the
 *   allocation slowpath, bulk allocation, and put_cpu_partial(), so that the
 *   local cpu doesn't change in the process and e.g. the kmem_cache_cpu pointer
 *   doesn't have to be revalidated in each section protected by the local lock.
 *
 * SLUB assigns one slab for allocation to each processor.
 * Allocations only occur from these slabs called cpu slabs.
 *
 * Slabs with free elements are kept on a partial list and during regular
 * operations no list for full slabs is used. If an object in a full slab is
 * freed then the slab will show up again on the partial lists.
 * We track full slabs for debugging purposes though because otherwise we
 * cannot scan all objects.
 *
 * Slabs are freed when they become empty. Teardown and setup is
 * minimal so we rely on the page allocators per cpu caches for
 * fast frees and allocs.
 *
 * slab->frozen		The slab is frozen and exempt from list processing.
 * 			This means that the slab is dedicated to a purpose
 * 			such as satisfying allocations for a specific
 * 			processor. Objects may be freed in the slab while
 * 			it is frozen but slab_free will then skip the usual
 * 			list operations. It is up to the processor holding
 * 			the slab to integrate the slab into the slab lists
 * 			when the slab is no longer needed.
 *
 * 			One use of this flag is to mark slabs that are
 * 			used for allocations. Then such a slab becomes a cpu
 * 			slab. The cpu slab may be equipped with an additional
 * 			freelist that allows lockless access to
 * 			free objects in addition to the regular freelist
 * 			that requires the slab lock.
 *
 * SLAB_DEBUG_FLAGS	Slab requires special handling due to debug
 * 			options set. This moves	slab handling out of
 * 			the fast path and disables lockless freelists.
 */
```

### [图解 slub](http://www.wowotech.net/memory_management/426.html)
### [Slab allocators in the Linux Kernel: SLAB, SLOB, SLUB](https://events.static.linuxfound.org/sites/events/files/slides/slaballocators.pdf) : 2014
### [How does the SLUB allocator work](https://events.static.linuxfound.org/images/stories/pdf/klf2012_kim.pdf)

分析了 slow_path 过程 :
```c
static __always_inline void *slab_alloc(struct kmem_cache *s,
		gfp_t gfpflags, unsigned long addr)
{
	return slab_alloc_node(s, gfpflags, NUMA_NO_NODE, addr);
}

/*
 * Inlined fastpath so that allocation functions (kmalloc, kmem_cache_alloc)
 * have the fastpath folded into their functions. So no function call
 * overhead for requests that can be satisfied on the fastpath.
 *
 * The fastpath works by first checking if the lockless freelist can be used.
 * If not then __slab_alloc is called for slow processing.
 *
 * Otherwise we can simply pick the next object from the lockless free list.
 */
static __always_inline void *slab_alloc_node(struct kmem_cache *s,
		gfp_t gfpflags, int node, unsigned long addr)
```


### [The SLUB allocator](https://lwn.net/Articles/229984/)

Given the lack of per-slab metadata, one might well wonder just how that first free object is found.
The answer is that the SLUB allocator stuffs the relevant information into the system memory map - the `page` structures associated with the pages which make up the slab.
> @todo 调用过程中间 page 的角色是什么 ? 我他妈现在只是需要一点点内存，为什么结果会知道从哪一个 page 中间找到的 ?

When a slab is first created by the allocator, it has no objects allocated from it.
Once an object has been allocated, it becomes a "partial" slab which is stored on a list in the `kmem_cache` structure.
*Since this is a patch aimed at scalability, there is, in fact, one "partial" list for each NUMA node on the system.*
The allocator tries to keep allocations node-local, but it will reach across nodes before filling the system with partial slabs.
> @todo 每一个 numa 节点存在一个 partial list，多核，调用同一个函数，如何并发调用 slab 呀!o

There is also a per-CPU array of active slabs, intended to prevent *cache line* bouncing even within a NUMA node.
There is a special thread which runs (via a workqueue) which monitors the usage of per-CPU slabs; if a per-CPU slab is not being used, it gets put back onto the partial list for use by other processors.
> @todo 难以理解为什么会出现 cache bouncing

## 基本的入口

- kmalloc
  - __do_kmalloc_node
    - kmalloc_slab(size, b, flags, caller)
    - kasan_kmalloc

- kmem_cache_alloc_noprof
  - trace_kmem_cache_alloc


## slub oracle 文章
<!-- 9c261345-033b-4714-9e7e-e4faec7c0720 -->
- https://blogs.oracle.com/linux/post/linux-slub-allocator-internals-and-debugging-1
- https://blogs.oracle.com/linux/post/linux-slub-allocator-internals-and-debugging-2
- https://blogs.oracle.com/linux/post/linux-slub-allocator-internals-and-debugging-3
- https://blogs.oracle.com/linux/post/linux-slub-allocator-internals-and-debugging-4

### 问题

slabs in the per-cpu partial slab list are connected via slab.next and
slabs in the per-node partial slab lists are connected via slab.slab_list

导致这种差异的原因是什么，既然都是链表?

kmem_cache_cpu::partial 是指向第一个 slab ，通过 slab::next 来位置链表。

#### object_size == slab_size ，也就是 object 中不会存放 free pointer ，那么一个 slab 中如何知道哪些释放，哪些没有释放?

object_size 就是 kmem_cache::object_size
slab_size 就是 kmem_cache::size

具体计算在 object_size 中:
```c
static ssize_t slab_size_show(struct kmem_cache *s, char *buf)
{
	return sysfs_emit(buf, "%u\n", s->size);
}
```

例如从 kmalloc-512 中看，4 一个 page 中正好 8 个 object ，完全无法容忍其他的数据啊
```txt
object_size:512
objs_per_slab:32
slab_size:512
order:2
```

```c
struct kmem_cache {
    // ...
	unsigned int offset;		/* Free pointer offset */
```
注意，freepointer 只会存储到 free object 中。
所以，就是不用占用空间的。

#### kmem_cache_cpu::freelist 中的 slab::freelist 有什么区别
slab::freelist 是一个通用的。

kmem_cache_cpu::freelist 如果释放了，那么立刻放到其中，而不是放到 slab 中去。


## 问题
- 所以这些 kmem_cache 控制的 page allocator 其实各自分离开来，kmem_cache 存储了关于大小的信息。

- get_partial_node : Try to allocate a partial slab from a specific node

1. 关于 get_freepointer  : 为什么不把 freepointer 放到开始的位置，使用 `s->offset`

3. 检查下 mm/slab_common.c 的内容
- kmem_cache 相关的内容

## 通过 get_freelist 理解 struct kmem_cache_cpu 的工作
```c
/*
 * Check the slab->freelist and either transfer the freelist to the
 * per cpu freelist or deactivate the slab.
 *
 * The slab is still frozen if the return value is not NULL.
 *
 * If this function returns NULL then the slab has been unfrozen.
 */
static inline void *get_freelist(struct kmem_cache *s, struct slab *slab)
{
	struct slab new;
	unsigned long counters;
	void *freelist;

	lockdep_assert_held(this_cpu_ptr(&s->cpu_slab->lock));

	do {
		freelist = slab->freelist;
		counters = slab->counters;

		new.counters = counters;

    // 因为 page 上的所有的 objects 全部转移到 kmem_cache_cpu::freelist 上，
    // 所以标记为 inuse = slab->objects
    // 表示所有的都被使用了
		new.inuse = slab->objects;
    // 脱离链表的全局的 struct slab 就是需要被标记为非 frozen 的
		new.frozen = freelist != NULL;

	} while (!__slab_update_freelist(s, slab,
		freelist, counters,
		NULL, new.counters,
		"get_freelist"));

	return freelist;
}
```

## 冻结 page
https://mp.weixin.qq.com/s/IjwDyftIPalIthmhB75uNw

和 slub 的 froze 不是一个东西

## 常用函数备忘
- put_cpu_partial : 把一个 slab 放到 cpu paritial 中
- get_partial_node : 从 node partial 中找一个
```c
		remove_partial(n, slab); // 先移除出来

    // get_partial_node 希望顺便填充一下 cpu partial 的
		if (!partial) {
			partial = slab; // 第一个被用于 get_partial_node 函数自身的返回
			stat(s, ALLOC_FROM_PARTIAL);

      // 如果这个 kmem_cache 不可以
			if ((slub_get_cpu_partial(s) == 0)) {
				break;
			}
		} else {
      // 其他 slab 的用于填充，这里不会导致超过限制，因为之所以调用 get_partial_node ，就是由于
      // 在 ___slab_alloc 中必然是先已经把 cpu partial 用完了，才找到这里的
			put_cpu_partial(s, slab, 0);
      // 结合注释
      // 	CPU_PARTIAL_NODE,	/* Refill cpu partial from node partial */
			stat(s, CPU_PARTIAL_NODE);

      // 一次填充的量不会超过一半
			if (++partial_slabs > slub_get_cpu_partial(s) / 2) {
				break;
			}
		}
```

- ___slab_alloc
  - slab = get_partial(s, node, &pc); 获取到的 partial 马上就是变为 kmem_cache_cpu::freelist 使用
  - freelist = freeze_slab(s, slab);

## 问题 : slub 的 shrink 是如何通知到 inode 和 dentry 之类的


## 原来这个可以写啊
echo 0 | sudo tee /sys/kernel/slab/kmalloc-512/cpu_partial


## 为什么系统中 kmalloc 有三个调用路线

4.19 内核的，主线上的也有这个问题吗?
```txt
  kmalloc_slab // 数量不对，之后的 N100 会有大约 7814 多个
  __kmalloc_track_caller
  kstrdup
  __kernfs_new_node
  kernfs_new_node
  __kernfs_create_file
  cgroup_addrm_files
  css_populate_dir
  cgroup_mkdir
  kernfs_iop_mkdir
  vfs_mkdir
  do_mkdirat
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    1944

  kmalloc_slab
  __kmalloc_track_caller // 认为不太可能，字符的长度不够
  kstrdup
  __kernfs_new_node
  kernfs_new_node
  __kernfs_create_file
  cgroup_addrm_files
  css_populate_dir
  cgroup_apply_control_enable
  cgroup_mkdir
  kernfs_iop_mkdir
  vfs_mkdir
  do_mkdirat
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    7009

  kmalloc_slab
  __kmalloc
  __register_sysctl_table
  register_leaf_sysctl_tables
  register_leaf_sysctl_tables
  register_leaf_sysctl_tables
  __register_sysctl_paths
  register_sched_domain_sysctl
  partition_sched_domains
  cpuset_write_resmask
  cgroup_file_write
  kernfs_fop_write
  __vfs_write
  vfs_write
  ksys_write
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    245872

  kmalloc_slab  // 这个似乎合理的
  __kmalloc
  __register_sysctl_table
  register_leaf_sysctl_tables
  register_leaf_sysctl_tables
  register_leaf_sysctl_tables
  __register_sysctl_paths
  register_sched_domain_sysctl
  partition_sched_domains
  cpuset_write_resmask
  cgroup_file_write
  kernfs_fop_write
  __vfs_write
  vfs_write
  ksys_write
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    307582
```
三个 alloc 我需要先确认这独自谁的

## slub_debug
看 slub 的描述，感觉已经是总结了大多数内存问题的调查方法:
- Documentation/mm/slub.rst

## slub merge
- http://raverstern.site/en/posts/slab-merging/

slub 之后的分析方法类似，也应该更多的使用内核模块测试。

## 为什么 slub 需要使用连续的物理内存，使用 vmalloc 不可以吗?


## slub Sheaves
<!-- 4513aa7f-0f9c-4aaa-83f8-31b667de36e4 -->

https://lore.kernel.org/lkml/20250723-slub-percpu-caches-v5-0-b792cd830f5d@suse.cz/#r

https://www.phoronix.com/forums/forum/software/general-linux-open-source/1576947-linux-s-new-sheaves-per-cpu-caching-layer-showing-massive-wins-for-amd-performance

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
