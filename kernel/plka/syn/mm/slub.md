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

2. 
```c
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
```
> 所以 page 和 partial 有什么区别吗 ? page 不够就从 partial 中间来拿

3. 

```c
		struct {	/* slab, slob and slub */
			union {
				struct list_head slab_list;
				struct {	/* Partial pages */
					struct page *next;
					int pages;	/* Nr of pages left */ // 非常的诡异，并不知道是做什么的
					int pobjects;	/* Approximate count */
				};
			};
```

4. 其想要实现的 lockless 导致的
```c
__cmpxchg_double_slab
```



## todo
```c
struct vm_area_struct *vm_area_dup(struct vm_area_struct *orig)
{
	struct vm_area_struct *new = kmem_cache_alloc(vm_area_cachep, GFP_KERNEL); // todo 这是一个什么分配方法呀 !

	if (new) {
		*new = *orig;
		INIT_LIST_HEAD(&new->anon_vma_chain);
	}
	return new;
}
```
1. 了解一下 : kmalloc_info_struct new_kmalloc_cache(初始化的过程)
2. free_slab 上还有部分小的内容没有分析


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
```c
/*
 * Slow path. The lockless freelist is empty or we need to perform
 * debugging duties.
 *
 * Processing is still very fast if new objects have been freed to the
 * regular freelist. In that case we simply take over the regular freelist
 * as the lockless freelist and zap the regular freelist.
 *
 * If that is not working then we fall back to the partial lists. We take the
 * first element of the freelist as the object to allocate now and move the
 * rest of the freelist to the lockless freelist.
 *
 * And if we were unable to get a new slab from the partial slab lists then
 * we need to allocate a new slab. This is the slowest path since it involves
 * a call to the page allocator and the setup of a new slab.
 *
 * Version of __slab_alloc to use when we know that interrupts are
 * already disabled (which is the case for bulk allocation).
 */
static void *___slab_alloc(struct kmem_cache *s, gfp_t gfpflags, int node,
			  unsigned long addr, struct kmem_cache_cpu *c)
{
	void *freelist;
	struct page *page;

	page = c->page; // page 是 freelist 指向地址所在的 page 上
	if (!page)
		goto new_slab;
redo:

	if (unlikely(!node_match(page, node))) { // page 并不在该node 上
		int searchnode = node;

		if (node != NUMA_NO_NODE && !node_present_pages(node))
			searchnode = node_to_mem_node(node);

		if (unlikely(!node_match(page, searchnode))) {
			stat(s, ALLOC_NODE_MISMATCH);
			deactivate_slab(s, page, c->freelist, c);
			goto new_slab;
		}
	}

	/*
	 * By rights, we should be searching for a slab page that was
	 * PFMEMALLOC but right now, we are losing the pfmemalloc
	 * information when the page leaves the per-cpu allocator
	 */
	if (unlikely(!pfmemalloc_match(page, gfpflags))) {
		deactivate_slab(s, page, c->freelist, c);
		goto new_slab;
	}

	/* must check again c->freelist in case of cpu migration or IRQ */
	freelist = c->freelist;
	if (freelist)
		goto load_freelist;

	freelist = get_freelist(s, page);

	if (!freelist) {
		c->page = NULL;
		stat(s, DEACTIVATE_BYPASS);
		goto new_slab;
	}

	stat(s, ALLOC_REFILL);

load_freelist:
	/*
	 * freelist is pointing to the list of objects to be used.
	 * page is pointing to the page from which the objects are obtained.
	 * That page must be frozen for per cpu allocations to work.
	 */
	VM_BUG_ON(!c->page->frozen);
	c->freelist = get_freepointer(s, freelist); // todo 的确获取到了 freelist，但是page 里面的偏移量没有修改啊 !
	c->tid = next_tid(c->tid); // todo
	return freelist;

new_slab:

	if (slub_percpu_partial(c)) {  // 现在有 partial page 吗?
		page = c->page = slub_percpu_partial(c); // 将该 partial page 作为
		slub_set_percpu_partial(c, page); // 将该 page 从移除 c->partial
		stat(s, CPU_PARTIAL_ALLOC);
		goto redo;
	}

  // 这是从 buddy allocator 哪里申请，还是node 哪里申请
	freelist = new_slab_objects(s, gfpflags, node, &c);

	if (unlikely(!freelist)) {
		slab_out_of_memory(s, gfpflags, node);
		return NULL;
	}

	page = c->page;
	if (likely(!kmem_cache_debug(s) && pfmemalloc_match(page, gfpflags)))
		goto load_freelist;

	/* Only entered in the debug case */
	if (kmem_cache_debug(s) &&
			!alloc_debug_processing(s, page, freelist, addr))
		goto new_slab;	/* Slab failed checks. Next slab needed */

	deactivate_slab(s, page, get_freepointer(s, freelist), c);
	return freelist;
}
```


## new_slab_objects : 将工作分发给 paritial 和 buddy
```c
static inline void *new_slab_objects(struct kmem_cache *s, gfp_t flags,
			int node, struct kmem_cache_cpu **pc)
{
	void *freelist;
	struct kmem_cache_cpu *c = *pc;
	struct page *page;

	WARN_ON_ONCE(s->ctor && (flags & __GFP_ZERO));

	freelist = get_partial(s, flags, node, c); // 就是在此处，进入到 partial 的，表示 cpu partial 需要补充

	if (freelist)
		return freelist;

	page = new_slab(s, flags, node); // 有此处进入到 buddy allocator 的地方
	if (page) {
		c = raw_cpu_ptr(s->cpu_slab);
		if (c->page)
			flush_slab(s, c);

		/*
		 * No other reference to the page yet so we can
		 * muck around with it freely without cmpxchg
		 */
		freelist = page->freelist;
		page->freelist = NULL;

		stat(s, ALLOC_SLAB);
		c->page = page;
		*pc = c;
	} else
		freelist = NULL;

	return freelist;
}
```


## (3) page allocator

```c
static struct page *new_slab(struct kmem_cache *s, gfp_t flags, int node)
{
	if (unlikely(flags & GFP_SLAB_BUG_MASK)) {
		gfp_t invalid_mask = flags & GFP_SLAB_BUG_MASK;
		flags &= ~GFP_SLAB_BUG_MASK;
		pr_warn("Unexpected gfp: %#x (%pGg). Fixing up to gfp: %#x (%pGg). Fix your code!\n",
				invalid_mask, &invalid_mask, flags, &flags);
		dump_stack();
	}

	return allocate_slab(s,
		flags & (GFP_RECLAIM_MASK | GFP_CONSTRAINT_MASK), node);
}

static struct page *allocate_slab(struct kmem_cache *s, gfp_t flags, int node) // 很长的函数　line : 1636

// 最终到达 alloc_slab_page 的地方
/*
 * Slab allocation and freeing
 */
static inline struct page *alloc_slab_page(struct kmem_cache *s,
		gfp_t flags, int node, struct kmem_cache_order_objects oo)
{
	struct page *page;
	unsigned int order = oo_order(oo);

	if (node == NUMA_NO_NODE)
		page = alloc_pages(flags, order);
	else
		page = __alloc_pages_node(node, flags, order);

	if (page && memcg_charge_slab(page, flags, order, s)) {
		__free_pages(page, order);
		page = NULL;
	}

	return page;
}

// 似乎整个调用过程就是平铺直叙的，但是其中存在很多各种数据检查
```

## (2) partial list

```c
#define slub_percpu_partial(c)		((c)->partial)

#define slub_set_percpu_partial(c, p)		\
({						\
	slub_percpu_partial(c) = (p)->next;	\
})
```

```c
// todo 似乎把两个 partial 搞混了，这TMD 是 kmem_cache_node 的内容 !
/*
 * Management of partially allocated slabs.
 */
static inline void
__add_partial(struct kmem_cache_node *n, struct page *page, int tail)
{
	n->nr_partial++;
	if (tail == DEACTIVATE_TO_TAIL)
		list_add_tail(&page->lru, &n->partial);
	else
		list_add(&page->lru, &n->partial);
}

static inline void add_partial(struct kmem_cache_node *n,
				struct page *page, int tail)
{
	lockdep_assert_held(&n->list_lock);
	__add_partial(n, page, tail);
}

static inline void remove_partial(struct kmem_cache_node *n,　// remove_partial 被 acquire_slab 调用，当需要更多page 的时候使用
					struct page *page)
{
	lockdep_assert_held(&n->list_lock);
	list_del(&page->lru); // lru 虽然和函数是放在union 中间，但是此时其并没有启用，所以问题不大
	n->nr_partial--; 
}


/*
 * Put a page that was just frozen (in __slab_free) into a partial page
 * slot if available.
 *
 * If we did not find a slot then simply move all the partials to the
 * per node partial list.
 */
static void put_cpu_partial(struct kmem_cache *s, struct page *page, int drain) // todo node 和 cpu 上都有 partial，这个指的是 page 上的，上面是 node 上的吗?
```


> 分析 `__add_partial` 的内容 :

```c
deactivate_slab : // todo 烦的一匹，总是存在一个函数
__slab_free : // todo slab_free 的流程吧!
unfreeze_partials : // todo 
```
> 如果说，这些函数存在何种共同点，那就是 compare and exchange 函数 !

```c
/*
 * Get a partial page, lock it and return it.
 */
static void *get_partial(struct kmem_cache *s, gfp_t flags, int node, struct kmem_cache_cpu *c)
    /*
     * Get a page from somewhere. Search in increasing NUMA distances.
     */
    static void *get_any_partial(struct kmem_cache *s, gfp_t flags, struct kmem_cache_cpu *c)
        /*
         * Try to allocate a partial slab from a specific node.
         */
        // 从一个特定的node 上获取一个 partial slab 放到 cpu 的 patial 上，依赖 acquire_slab 将 kmem_cache_node 上 partial 取下来，
        // 利用 put_cpu_partial 将 partial 放到 cpu 的 partial 上
        // todo 因为 cpu partial 是私人访问的，是不是就不用上锁了
        static void *get_partial_node(struct kmem_cache *s, struct kmem_cache_node *n, struct kmem_cache_cpu *c, gfp_t flags)
              /*
               * Remove slab from the partial list, freeze it and
               * return the pointer to the freelist.
               *
               * Returns a list of objects or NULL if it fails.
               */
              static inline void *acquire_slab(struct kmem_cache *s, struct kmem_cache_node *n, struct page *page, int mode, int *objects)
              /*
               * Put a page that was just frozen (in __slab_free|get_partial_node) into a
               * partial page slot if available.
               *
               * If we did not find a slot then simply move all the partials to the
               * per node partial list.
               */
              static void put_cpu_partial(struct kmem_cache *s, struct page *page, int drain) // 什么叫做 partial page slot 啊
```


1. 
```c
/*
 * Try to allocate a partial slab from a specific node.
 */
static void *get_partial_node(struct kmem_cache *s, struct kmem_cache_node *n,
				struct kmem_cache_cpu *c, gfp_t flags)
{
	struct page *page, *page2;
	void *object = NULL;
	unsigned int available = 0;
	int objects;

	/*
	 * Racy check. If we mistakenly see no partial slabs then we
	 * just allocate an empty slab. If we mistakenly try to get a
	 * partial slab and there is none available then get_partials()
	 * will return NULL.
	 */
	if (!n || !n->nr_partial)
		return NULL;

	spin_lock(&n->list_lock);
	list_for_each_entry_safe(page, page2, &n->partial, slab_list) { // 对于 n->partial 上的 page 进行遍历
		void *t;

		if (!pfmemalloc_match(page, flags))
			continue;

		t = acquire_slab(s, n, page, object == NULL, &objects);
		if (!t)
			break;

		available += objects;

    // 首先放到 page 上，那么说明之前的时候 page 上的是空的才会调用 get_partial_node 来
		if (!object) {
			c->page = page;
			stat(s, ALLOC_FROM_PARTIAL);
			object = t;
		} else {
			put_cpu_partial(s, page, 0);
			stat(s, CPU_PARTIAL_NODE);
		}
		if (!kmem_cache_has_cpu_partial(s)
			|| available > slub_cpu_partial(s) / 2) // 向 cpu paritial 中间注入的数量到达某一个数值即可 !
			break;

	}
	spin_unlock(&n->list_lock);
	return object;
}
```

2.
```c
/*
 * Remove slab from the partial list, freeze it and
 * return the pointer to the freelist.
 *
 * Returns a list of objects or NULL if it fails.
 */
static inline void *acquire_slab(struct kmem_cache *s,
		struct kmem_cache_node *n, struct page *page,
		int mode, int *objects)
{
	void *freelist;
	unsigned long counters;
	struct page new;

	lockdep_assert_held(&n->list_lock);

	/*
	 * Zap the freelist and set the frozen bit.
	 * The old freelist is the list of objects for the
	 * per cpu allocation list.
	 */
	freelist = page->freelist;
	counters = page->counters;
	new.counters = counters;
	*objects = new.objects - new.inuse;
	if (mode) {
		new.inuse = page->objects; // 为什么将其中所有的都设置为不可用，并不清楚
		new.freelist = NULL;
	} else {
		new.freelist = freelist;
	}

	VM_BUG_ON(new.frozen);
	new.frozen = 1;

	if (!__cmpxchg_double_slab(s, page,
			freelist, counters,
			new.freelist, new.counters,
			"acquire_slab"))
		return NULL;

	remove_partial(n, page);
	WARN_ON(!freelist);
	return freelist;
}
```



3. 
```c
/*
 * Put a page that was just frozen (in __slab_free|get_partial_node) into a
 * partial page slot if available.
 *
 * If we did not find a slot then simply move all the partials to the
 * per node partial list.
 */
static void put_cpu_partial(struct kmem_cache *s, struct page *page, int drain)
{
#ifdef CONFIG_SLUB_CPU_PARTIAL
	struct page *oldpage;
	int pages;
	int pobjects;

	preempt_disable();
	do {
		pages = 0;
		pobjects = 0;
		oldpage = this_cpu_read(s->cpu_slab->partial);

		if (oldpage) {
			pobjects = oldpage->pobjects;
			pages = oldpage->pages;
			if (drain && pobjects > s->cpu_partial) { // drain 在 free 的路线上使用
				unsigned long flags;
				/*
				 * partial array is full. Move the existing
				 * set to the per node partial list.
				 */
				local_irq_save(flags);
				unfreeze_partials(s, this_cpu_ptr(s->cpu_slab));
				local_irq_restore(flags);
				oldpage = NULL;
				pobjects = 0;
				pages = 0;
				stat(s, CPU_PARTIAL_DRAIN);
			}
		}

		pages++;
		pobjects += page->objects - page->inuse;

		page->pages = pages;
		page->pobjects = pobjects;
		page->next = oldpage;

	} while (this_cpu_cmpxchg(s->cpu_slab->partial, oldpage, page)
								!= oldpage);
	if (unlikely(!s->cpu_partial)) {
		unsigned long flags;

		local_irq_save(flags);
		unfreeze_partials(s, this_cpu_ptr(s->cpu_slab));
		local_irq_restore(flags);
	}
	preempt_enable();
#endif	/* CONFIG_SLUB_CPU_PARTIAL */
}
```


## kmalloc 实现机制
- ref 两个简单的封装函数，处理诡异的边界条件 ?
```c
/**
 * kmalloc_array - allocate memory for an array.
 * @n: number of elements.
 * @size: element size.
 * @flags: the type of memory to allocate (see kmalloc).
 */
static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	size_t bytes;

	if (unlikely(check_mul_overflow(n, size, &bytes)))
		return NULL;
	if (__builtin_constant_p(n) && __builtin_constant_p(size))
		return kmalloc(bytes, flags);
	return __kmalloc(bytes, flags);
}

/**
 * kmalloc - allocate memory
 * @size: how many bytes of memory are required.
 * @flags: the type of memory to allocate.
 *
 * kmalloc is the normal method of allocating memory
 * for objects smaller than page size in the kernel.
 *
 * The @flags argument may be one of:
 *
 * %GFP_USER - Allocate memory on behalf of user.  May sleep.
 *
 * %GFP_KERNEL - Allocate normal kernel ram.  May sleep.
 *
 * %GFP_ATOMIC - Allocation will not sleep.  May use emergency pools.
 *   For example, use this inside interrupt handlers.
 *
 * %GFP_HIGHUSER - Allocate pages from high memory.
 *
 * %GFP_NOIO - Do not do any I/O at all while trying to get memory.
 *
 * %GFP_NOFS - Do not make any fs calls while trying to get memory.
 *
 * %GFP_NOWAIT - Allocation will not sleep.
 *
 * %__GFP_THISNODE - Allocate node-local memory only.
 *
 * %GFP_DMA - Allocation suitable for DMA.
 *   Should only be used for kmalloc() caches. Otherwise, use a
 *   slab created with SLAB_DMA.
 *
 * Also it is possible to set different flags by OR'ing
 * in one or more of the following additional @flags:
 *
 * %__GFP_HIGH - This allocation has high priority and may use emergency pools.
 *
 * %__GFP_NOFAIL - Indicate that this allocation is in no way allowed to fail
 *   (think twice before using).
 *
 * %__GFP_NORETRY - If memory is not immediately available,
 *   then give up at once.
 *
 * %__GFP_NOWARN - If allocation fails, don't issue any warnings.
 *
 * %__GFP_RETRY_MAYFAIL - Try really hard to succeed the allocation but fail
 *   eventually.
 *
 * There are other flags available as well, but these are not intended
 * for general use, and so are not documented here. For a full list of
 * potential flags, always refer to linux/gfp.h.
 */
static __always_inline void *kmalloc(size_t size, gfp_t flags)
{
	if (__builtin_constant_p(size)) {
		if (size > KMALLOC_MAX_CACHE_SIZE)
			return kmalloc_large(size, flags);
	}
	return __kmalloc(size, flags);
}
```



```c
/*
 * Find the kmem_cache structure that serves a given size of
 * allocation
 */
struct kmem_cache *kmalloc_slab(size_t size, gfp_t flags)
{
	unsigned int index;

	if (unlikely(size > KMALLOC_MAX_SIZE)) {
		WARN_ON_ONCE(!(flags & __GFP_NOWARN));
		return NULL;
	}

	if (size <= 192) {
		if (!size)
			return ZERO_SIZE_PTR;
		index = size_index[size_index_elem(size)];
	} else
		index = fls(size - 1);

#ifdef CONFIG_ZONE_DMA
	if (unlikely((flags & GFP_DMA)))
		return kmalloc_dma_caches[index];
#endif

	return kmalloc_caches[index];
}
```


```c
static __always_inline void *slab_alloc(struct kmem_cache *s,
		gfp_t gfpflags, unsigned long addr)
{
	return slab_alloc_node(s, gfpflags, NUMA_NO_NODE, addr);
}

static __always_inline void *slab_alloc_node(struct kmem_cache *s, // fast 和 slow 的入口
		gfp_t gfpflags, int node, unsigned long addr)


static void *__slab_alloc(struct kmem_cache *s, gfp_t gfpflags, int node, // ___slab_alloc 的 interrupt disable 封装函数
			  unsigned long addr, struct kmem_cache_cpu *c)

static void *___slab_alloc(struct kmem_cache *s, gfp_t gfpflags, int node, // slow path
			  unsigned long addr, struct kmem_cache_cpu *c)
```


## security

```Makefile
config SLAB_FREELIST_RANDOM
	default n
	depends on SLAB || SLUB
	bool "SLAB freelist randomization"
	help
	  Randomizes the freelist order used on creating new pages. This
	  security feature reduces the predictability of the kernel slab
	  allocator against heap overflows.

config SLAB_FREELIST_HARDENED
	bool "Harden slab freelist metadata"
	depends on SLUB
	help
	  Many kernel heap attacks try to target slab cache metadata and
	  other infrastructure. This options makes minor performance
	  sacrifies to harden the kernel slab allocator against common
	  freelist exploit methods.
```



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

## get_freepointer : 从一个未分配的 object 中间获取下一个 object 的地址

```c
static inline void *get_freepointer(struct kmem_cache *s, void *object)
{
	return freelist_dereference(s, object + s->offset);
}

/* Returns the freelist pointer recorded at location ptr_addr. */
static inline void *freelist_dereference(const struct kmem_cache *s,
					 void *ptr_addr)
{
	return freelist_ptr(s, (void *)*(unsigned long *)(ptr_addr),
			    (unsigned long)ptr_addr);
}

/*
 * Returns freelist pointer (ptr). With hardening, this is obfuscated
 * with an XOR of the address where the pointer is held and a per-cache
 * random number.
 */
static inline void *freelist_ptr(const struct kmem_cache *s, void *ptr,
				 unsigned long ptr_addr)
{
#ifdef CONFIG_SLAB_FREELIST_HARDENED
	/*
	 * When CONFIG_KASAN_SW_TAGS is enabled, ptr_addr might be tagged.
	 * Normally, this doesn't cause any issues, as both set_freepointer()
	 * and get_freepointer() are called with a pointer with the same tag.
	 * However, there are some issues with CONFIG_SLUB_DEBUG code. For
	 * example, when __free_slub() iterates over objects in a cache, it
	 * passes untagged pointers to check_object(). check_object() in turns
	 * calls get_freepointer() with an untagged pointer, which causes the
	 * freepointer to be restored incorrectly.
	 */
	return (void *)((unsigned long)ptr ^ s->random ^
			(unsigned long)kasan_reset_tag((void *)ptr_addr));
#else
	return ptr;
#endif
}
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

## get_freelist
```c
/*
 * Check the page->freelist of a page and either transfer the freelist to the
 * per cpu freelist or deactivate the page.
 *
 * The page is still frozen if the return value is not NULL.
 *
 * If this function returns NULL then the page has been unfrozen.
 *
 * This function must be called with interrupt disabled.
 */
static inline void *get_freelist(struct kmem_cache *s, struct page *page)
{
	struct page new;
	unsigned long counters;
	void *freelist;

	do {
		freelist = page->freelist;
		counters = page->counters;

		new.counters = counters;
		VM_BUG_ON(!new.frozen);

		new.inuse = page->objects;
		new.frozen = freelist != NULL;

	} while (!__cmpxchg_double_slab(s, page,
		freelist, counters,
		NULL, new.counters,
		"get_freelist"));

	return freelist;
}
```
1. 并不知道 counter 的修改作用是什么? 只是知道本函数返回 `page->freelist`


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


## slab_free

```c
static __always_inline void slab_free(struct kmem_cache *s, struct page *page,
				      void *head, void *tail, int cnt,
				      unsigned long addr)
{
	/*
	 * With KASAN enabled slab_free_freelist_hook modifies the freelist
	 * to remove objects, whose reuse must be delayed.
	 */
	if (slab_free_freelist_hook(s, &head, &tail))  // todo 我感觉这只是一个检查工具，但是实际上，我看不懂其中的内容
		do_slab_free(s, page, head, tail, cnt, addr);
}
```

## (1) slab_free : fast path
```c
/*
 * Fastpath with forced inlining to produce a kfree and kmem_cache_free that
 * can perform fastpath freeing without additional function calls.
 *
 * The fastpath is only possible if we are freeing to the current cpu slab
 * of this processor. This typically the case if we have just allocated
 * the item before.
 *
 * If fastpath is not possible then fall back to __slab_free where we deal
 * with all sorts of special processing.
 *
 * Bulk free of a freelist with several objects (all pointing to the
 * same page) possible by specifying head and tail ptr, plus objects
 * count (cnt). Bulk free indicated by tail pointer being set.
 */
static __always_inline void do_slab_free(struct kmem_cache *s,
				struct page *page, void *head, void *tail,
				int cnt, unsigned long addr)
{
	void *tail_obj = tail ? : head; // todo 从此处是不是说明，kmem_cache::offset 就是只有两种取值 ? 所以为什么需要设置两种模式 ?
	struct kmem_cache_cpu *c;
	unsigned long tid;
redo:
	/*
	 * Determine the currently cpus per cpu slab.
	 * The cpu may change afterward. However that does not matter since
	 * data is retrieved via this pointer. If we are on the same cpu
	 * during the cmpxchg then the free will succeed.
	 */
	do {
		tid = this_cpu_read(s->cpu_slab->tid); // todo percpu 和 preemption 的关系，其实也不是非常的麻烦。
		c = raw_cpu_ptr(s->cpu_slab);
	} while (IS_ENABLED(CONFIG_PREEMPTION) &&
		 unlikely(tid != READ_ONCE(c->tid)));

	/* Same with comment on barrier() in slab_alloc_node() */
	barrier();

	if (likely(page == c->page)) {
		set_freepointer(s, tail_obj, c->freelist); // fast freee 似乎非常简单，将被释放的位置指向原来的 c->freelist ，然后s->cpu_slab->freelist 指向被释放的位置。

		if (unlikely(!this_cpu_cmpxchg_double(
				s->cpu_slab->freelist, s->cpu_slab->tid,
				c->freelist, tid,
				head, next_tid(tid)))) {

			note_cmpxchg_failure("slab_free", s, tid);
			goto redo;
		}
		stat(s, FREE_FASTPATH);
	} else
		__slab_free(s, page, head, tail_obj, cnt, addr);

}
```

## (2) `__slab_free`: slow path

```c
/*
 * Slow path handling. This may still be called frequently since objects
 * have a longer lifetime than the cpu slabs in most processing loads.
 *
 * So we still attempt to reduce cache line usage. Just take the slab
 * lock and free the item. If there is no additional partial page
 * handling required then we can return immediately.
 */
static void __slab_free(struct kmem_cache *s, struct page *page,
			void *head, void *tail, int cnt,
			unsigned long addr)

{
	void *prior;
	int was_frozen;
	struct page new;
	unsigned long counters;
	struct kmem_cache_node *n = NULL;
	unsigned long uninitialized_var(flags);

	stat(s, FREE_SLOWPATH);

	if (kmem_cache_debug(s) &&
	    !free_debug_processing(s, page, head, tail, cnt, addr))
		return;

	do {
		if (unlikely(n)) {
			spin_unlock_irqrestore(&n->list_lock, flags);
			n = NULL;
		}
		prior = page->freelist;
		counters = page->counters;
		set_freepointer(s, tail, prior);
		new.counters = counters;
		was_frozen = new.frozen;
		new.inuse -= cnt;
		if ((!new.inuse || !prior) && !was_frozen) { // 当没有 frozen 以及 (释放之后为 slab 没有管理的 page ) // todo new.inuse 和 prior 不是一个含义吗 ?

			if (kmem_cache_has_cpu_partial(s) && !prior) { // 如果 prior 是一个 NULL, 说明当前的 page 是 full list

				/*
				 * Slab was on no list before and will be
				 * partially empty
				 * We can defer the list move and instead
				 * freeze it.
				 */
				new.frozen = 1; // todo 为什么 frozen 起来

			} else { /* Needs to be taken off a list */

				n = get_node(s, page_to_nid(page));
				/*
				 * Speculatively acquire the list_lock.
				 * If the cmpxchg does not succeed then we may
				 * drop the list_lock without any processing.
				 *
				 * Otherwise the list_lock will synchronize with
				 * other processors updating the list of slabs.
				 */
				spin_lock_irqsave(&n->list_lock, flags);

			}
		}

	} while (!cmpxchg_double_slab(s, page,
		prior, counters,
		head, new.counters,
		"__slab_free"));

	if (likely(!n)) {

		/*
		 * If we just froze the page then put it onto the
		 * per cpu partial list.
		 */
		if (new.frozen && !was_frozen) { // todo 选择这个时机的原因是什么 ?
			put_cpu_partial(s, page, 1); // alloc 的时候也调用过
			stat(s, CPU_PARTIAL_FREE);
		}
		/*
		 * The list lock was not taken therefore no list
		 * activity can be necessary.
		 */
		if (was_frozen)
			stat(s, FREE_FROZEN);
		return;
	}

	if (unlikely(!new.inuse && n->nr_partial >= s->min_partial)) // node 上的 partial 数量超标
		goto slab_empty;

	/*
	 * Objects left in the slab. If it was not on the partial list before
	 * then add it.
	 */
	if (!kmem_cache_has_cpu_partial(s) && unlikely(!prior)) {
		remove_full(s, n, page); // todo
		add_partial(n, page, DEACTIVATE_TO_TAIL); // todo
		stat(s, FREE_ADD_PARTIAL);
	}
	spin_unlock_irqrestore(&n->list_lock, flags);
	return;

slab_empty:
	if (prior) { // todo 这一步的操作是 ?
		/*
		 * Slab on the partial list.
		 */
		remove_partial(n, page);
		stat(s, FREE_REMOVE_PARTIAL);
	} else {
		/* Slab must be on the full list */
		remove_full(s, n, page);
	}

	spin_unlock_irqrestore(&n->list_lock, flags);
	stat(s, FREE_SLAB);
	discard_slab(s, page);
}
```

