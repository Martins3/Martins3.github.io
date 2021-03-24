# mm/slub.md

## Keynote

1. If all of the objects within a given slab (as tracked by the **inuse** counter) are freed, the entire slab is given back to the page allocator for reuse.
    1. 找到利用 inuse 然后释放的
    2. a slab 其实指的是 a page managed by the slab

2. `page->objects` 表示一个 page中间可以持有的所有的 objects 总数 ?
    1. 存储在 kmem_cache 中间 ?

1. page::next : 用于指向下一个链表。

2. page::freelist : 指向 page 中间第一个 free 的地址，通过 on_freelist 可以很好的理解链表的遍历过程。
    1. 当该 page 上的所有的 object 都被使用的时候，那么 freelist = NULL


## 问题
1. 关于 get_freepointer 想到:
    1. 为什么不把指针放到开始的位置，使用 `s->offset` @question
    2. 没有一个 object 跨越两个 page 的情况吧 !
    3. 一个page 中间初始化中间链表的代码找一下 @todo
    4. 如果想要形成链表，那么，是不是page 开始的位置不仅仅需要存储下一个 page，以及其中 freepointer.
        1. 实际上，这两个都是 struct page 中间保存

## 要点
1. kmalloc_slab 分析大小确定其中的 size
2. 从 mm/Makefile， SLAB 根本不会被编译到其中，其中的ref 只是由于单个文件产生的
3. get_freepointer(s, object) 返回 `object + s->offset`


- 是不是 kmem_cache 只是创建一次，管理大小相同的区域。
- 所以这些 kmem_cache 控制的 page allocator 其实各自分离开来，kmem_cache 存储了关于大小的信息。
- page 来自于 kmem_cache::kmem_cache_cpu::page 

- 如果 partial 的含义是部分占用，那么为什么还是可以配置的 ? get_partial_node : Try to allocate a partial slab from a specific node

- 防止 cache bouncing ，让同一个CPU kmalloc 的数据总是在一起的

## doc & ref

- [](http://www.wowotech.net/memory_management/426.html)

- [](https://events.static.linuxfound.org/sites/events/files/slides/slaballocators.pdf)

- Queues to track cache hotness
- Queues per cpu and per node
- Queues for each remote node (alien caches)
- **Complex data structures** that are described in the following two slides.
- Object based memory policies and *interleaving*.
- Exponential growth of caches nodes * nr_cpus. Large systems have huge amount of memory trapped in caches.
- Cold object expiration: Every processor has to scan its queues of every slab cache every 2 seconds.

> 1. queue 在哪里 ?
> 2. per cpu 和 node remote node 建立 queue

SLUB memory layout:
- Enough of the queueing.
- “Queue” for a single slab page. Pages associated with per cpu. Increased locality.
- Per cpu partials
- Fast paths using this_cpu_ops and per cpu data.
- Page based policies and interleave.
- Defragmentation functionality on multiple levels.
- Current default slab allocator.

```Makefile
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

```c
/*
 * Slab cache management.
 */
struct kmem_cache {
	struct kmem_cache_cpu __percpu *cpu_slab;
	/* Used for retriving partial slabs etc */
	slab_flags_t flags;
	unsigned long min_partial;
	unsigned int size;	/* The size of an object including meta data */
	unsigned int object_size;/* The size of an object without meta data */
	unsigned int offset;	/* Free pointer offset. */

	kmem_cache_order_objects oo;

	/* Allocation and freeing of slabs */
	struct kmem_cache_order_objects max;
	struct kmem_cache_order_objects min;
	gfp_t allocflags;	/* gfp flags to use on each alloc */
	int refcount;		/* Refcount for slab cache destroy */
	void (*ctor)(void *);
	unsigned int inuse;		/* Offset to metadata */
	unsigned int align;		/* Alignment */
	unsigned int red_left_pad;	/* Left redzone padding size */
	const char *name;	/* Name (only for display!) */
	struct list_head list;	/* List of slab caches */


	unsigned int useroffset;	/* Usercopy region offset */
	unsigned int usersize;		/* Usercopy region size */

	struct kmem_cache_node *node[MAX_NUMNODES];
};

struct kmem_cache_cpu {
	void **freelist;	/* Pointer to next available object */
	unsigned long tid;	/* Globally unique transaction id */
	struct page *page;	/* The slab from which we are allocating */
#ifdef CONFIG_SLUB_CPU_PARTIAL
	struct page *partial;	/* Partially allocated frozen slabs */
#endif
#ifdef CONFIG_SLUB_STATS
	unsigned stat[NR_SLUB_STAT_ITEMS];
#endif
};

struct kmem_cache_node {
	spinlock_t list_lock;

#ifdef CONFIG_SLUB
	unsigned long nr_partial;
	struct list_head partial;
#endif
};
```

统计工具很有意思!

- [](https://events.static.linuxfound.org/images/stories/pdf/klf2012_kim.pdf)

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


- [](https://lwn.net/Articles/229984/)

Given the lack of per-slab metadata, one might well wonder just how that first free object is found.
The answer is that the SLUB allocator stuffs the relevant information into the system memory map - the `page` structures associated with the pages which make up the slab.
> @todo 调用过程中间 page 的角色是什么 ? 我他妈现在只是需要一点点内存，为什么结果会知道从哪一个 page 中间找到的 ?

When a slab is first created by the allocator, it has no objects allocated from it.
Once an object has been allocated, it becomes a "partial" slab which is stored on a list in the `kmem_cache` structure.
*Since this is a patch aimed at scalability, there is, in fact, one "partial" list for each NUMA node on the system.*
The allocator tries to keep allocations node-local, but it will reach across nodes before filling the system with partial slabs.
> @todo 每一个 numa 节点存在一个 partial list，多核，调用同一个函数，如何并发调用slab 呀!o

There is also a per-CPU array of active slabs, intended to prevent *cache line* bouncing even within a NUMA node.
There is a special thread which runs (via a workqueue) which monitors the usage of per-CPU slabs; if a per-CPU slab is not being used, it gets put back onto the partial list for use by other processors.
> @todo 难以理解为什么会出现 cache bouncing

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

## tid

> @todo 万万没有想到，和 preemption 有关的

```c
#ifdef CONFIG_PREEMPT
/*
 * Calculate the next globally unique transaction for disambiguiation
 * during cmpxchg. The transactions start with the cpu number and are then
 * incremented by CONFIG_NR_CPUS.
 */
#define TID_STEP  roundup_pow_of_two(CONFIG_NR_CPUS)
#else
/*
 * No preemption supported therefore also no need to check for
 * different cpus.
 */
#define TID_STEP 1
#endif

static inline unsigned long next_tid(unsigned long tid)
{
	return tid + TID_STEP;
}

static inline unsigned int tid_to_cpu(unsigned long tid)
{
	return tid % TID_STEP;
}

static inline unsigned long tid_to_event(unsigned long tid)
{
	return tid / TID_STEP;
}

static inline unsigned int init_tid(int cpu)
{
	return cpu;
}
```

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
 // todo 什么鸡巴是 IPI handler 呀 ?
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


## page->objects
> @todo 其唯一调用者非常有意思的，但是，

```c
static inline struct kmem_cache_order_objects oo_make(unsigned int order,
		unsigned int size)
{
	struct kmem_cache_order_objects x = {
		(order << OO_SHIFT) + order_objects(order, size)
	};

	return x;
}
```
