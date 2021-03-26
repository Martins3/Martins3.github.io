# slub

## 问题
- [ ] 关键结构体 :
- [ ] free / alloc 慢速路径 和 快速路径上，分别因为什么原因

- [ ] CONFIG_SLUB_CPU_PARTIAL 的作用?
  - 猜测 : patial 必然存在，只是，没有这个选项，那么 CONFIG_SLUB_CPU_PARTIAL 的 paritial 是放到 node 上的，有了这个选项，可以放到 cpu 上

- [ ] get_freepointer 和 get_freelist 的关系是什么 ?


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

struct kmem_cache_node {
	spinlock_t list_lock;

	unsigned long nr_partial;
	struct list_head partial;
};
```
- [ ]  所以 page 和 partial 有什么区别吗 ? page 不够就从 partial 中间来拿
  - [ ] 应该首先从 partial 中间获取啊
  - [ ] 由于 free, 创建出来了多个 paritial page 怎么办啊 ?
  - kmem_cache_node 的内容来看，kmem_cache_cpu 最多只能持有一个 paritial, 其余的就会向 node 上存放

```c
		struct {	/* slab, slob and slub */
			union {
				struct list_head slab_list; // 挂载 kmem_cache_node::paritial 上
				struct {	/* Partial pages */
					struct page *next;
					int pages;	/* Nr of pages left */
					int pobjects;	/* Approximate count */
				};
			};
			struct kmem_cache *slab_cache; /* not slob */
			/* Double-word boundary */
			void *freelist;		/* first free object */
			union {
				unsigned long counters;		/* SLUB */
				struct {			/* SLUB */
					unsigned inuse:16; // 该 page 中间多少个对象被使用
					unsigned objects:15;
					unsigned frozen:1;
				};
			};
		};
```
- [ ] page 无法理解的成员：
  - objects : allocate_slab 中间初始化 `page->objects = oo_objects(oo);`, 所以描述就是 objects 总数
	
因为 partial 在 CPU 上只有一个，在 node 上持有多个，所以，在当前上下文 paritial list 指的是 node 的 paritial list.

## 笔记
认真重读一下:
- https://ruffell.nz/programming/writeups/2019/02/15/looking-at-kmalloc-and-the-slub-memory-allocator.html

[Wowotech](http://www.wowotech.net/memory_management/426.html)
[LoyenWang](https://www.cnblogs.com/LoyenWang/p/11922887.html)

- `struct kmem_cache` : 在函数 kmem_cache_create 的调用链条中间创建，被想要分配空间的用户使用, 用于管理一个种类 object 的分配信息。在初始化的过程中间，会进一步的初始化下面两个结构体:
  - `struct kmem_cache_cpu` : 管理 cpu 的 slab, 每一个 cpu 上分配一个
  - `struct kmem_cache_node` : 管理 node 的 slab, 每一个 numa node 分配一个

```c
/*
 * Slab cache management.
 */
struct kmem_cache {
	struct kmem_cache_cpu __percpu *cpu_slab;       //每个CPU slab页面
	/* Used for retriving partial slabs etc */
	unsigned long flags;
	unsigned long min_partial;
	int size;		/* The size of an object including meta data */
	int object_size;	/* The size of an object without meta data */
	int offset;		/* Free pointer offset. */
  // 既然每个object在没有分配之前不在乎每个object中存储的内容，
  // 那么完全可以在每个object中存储下一个object内存首地址，
  // 就形成了一个单链表, 那么这个地址数据存储在object什么位置呢？
  // offset就是存储下个object地址数据相对于这个object首地址的偏移。

#ifdef CONFIG_SLUB_CPU_PARTIAL
	/* Number of per cpu partial objects to keep around */
	unsigned int cpu_partial;
#endif
	struct kmem_cache_order_objects oo;     //该结构体会描述申请页面的order值，以及object的个数

	/* Allocation and freeing of slabs */
	struct kmem_cache_order_objects max;
	struct kmem_cache_order_objects min;
	gfp_t allocflags;	/* gfp flags to use on each alloc */
	int refcount;		/* Refcount for slab cache destroy */
	void (*ctor)(void *);           // 对象构造函数
	int inuse;		/* Offset to metadata */
	int align;		/* Alignment */
	int reserved;		/* Reserved bytes at the end of slabs */
	int red_left_pad;	/* Left redzone padding size */
	const char *name;	/* Name (only for display!) */
	struct list_head list;	/* List of slab caches */       //kmem_cache最终会链接在一个全局链表中
    struct kmem_cache_node *node[MAX_NUMNODES];     //Node管理slab页面
};
```

- [x] kmem_cache_create
![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191124161239520-610202035.png)


- [ ] kmem_cache_alloc
![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191124161307831-1244111963.png)

## Let's understand some basic function
- [x] get_freepointer_safe : 获取无锁空闲对象链表下一个对象的地址
    - `*(object + s->offset)` : because every free object contains the pointer points to next free object
    - ==> get_freepointer ==> freelist_dereference ==> freelist_ptr 
    - freelist_ptr(s, ptr, ptr_addr){return s}

- [ ] calculate_sizes()

- [x] get_freelist() : 
    - Check the `page->freelist` of a page and either transfer the freelist to the per cpu freelist or deactivate the page.
    - ![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191124161409130-2017775465.png)

- [ ] `kmem_cache::inuse` and `page::(anonymous union)::(anonymous struct)::(anonymous union)::(anonymous struct)::inuse`
  - [x] struct page结构体中inuse代表已经使用的obj数量。

## Let's understand some basic concepts

#### frozen

一个 slab 被某个 CPU freeze 后，就只有这个 CPU 能够从这个 slab 分配对象。
具体那些 slab 是冻结的 ? 首先，本地 slab 总是被冻结的；其次，脱离链表的全满 slab 在重新获得一个被释放的空闲对象后，
会进入本地 partial 链表并且被冻结。而其他的 slab, 如 numa node partial list, 刚刚从numa node list 移入本地 partial 中的 slab,
以及脱离链表的全满 slab 都是不冻结的。[^1] 

在 `__slab_free` 中间，是唯一利用 frozen 的判断，其余的存在检查 frozen 的正确性

#### partial
```c

static void *___slab_alloc(struct kmem_cache *s, gfp_t gfpflags, int node,
			  unsigned long addr, struct kmem_cache_cpu *c)
        // ...
new_slab:

	if (slub_percpu_partial(c)) {
		page = c->page = slub_percpu_partial(c);
		slub_set_percpu_partial(c, page);
		stat(s, CPU_PARTIAL_ALLOC);
		goto redo;
	}
```

#### [todo]transaction id
从 [^1] 的描述，这是专门给 本地无锁 (kmem_cache_cpu::freelist) 使用的

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
		VM_BUG_ON(!new.frozen); // kmem_cache_cpu::page 必须是被 frozen 的

		new.inuse = page->objects; // 因为 page 上的所有的 objects 全部转移到 kmem_cache_cpu::freelist 上，所以标记为 inuse = 0
		new.frozen = freelist != NULL;
    // 如果 page 的 freelist == NULL, 那么意味着需要从 cpu partial 中间分配 object 了
    // 而这个 page 已经变为全满的，所以需要 unfrozen 并且脱离管理

	} while (!__cmpxchg_double_slab(s, page,
		freelist, counters,
		NULL, new.counters,
		"get_freelist"));

	return freelist;
}
```
P215[^1] 指出，一共五种情况，从本地无锁(kmem_cache_cpu::freelist), 本地常规(kmem_cache_cpu::page), 本地 partial, node partial 和 buddy system

参考 P205[^1] 图 4-11 的说明:
1. kmem_cache_cpu::freelist 是管理本地分配和释放的
2. 调用 CPU A 上调用 slab_alloc 分配的对象，可能在 CPU B 释放，这种数据被放到 kmem_cache_cpu::page::freelist 上管理
3. 当 kmem_cache_cpu::freelist 的用完了，那么就将 kmem_cache_cpu::page::freelist 管理的 objects 全部转移给 kmem_cache_cpu::freelist, 如果 kmem_cache_cpu::page::freelist 也没有了，那么就需要从本地 partial 开始查找了。

// 或者你把你现在的配置仓库的地址给我，我看看在我的机器上的效果

## `__slab_free`
- [ ] page 214 深入的分析了 `__slab_free`，这个之后，算是就清楚了

[^1]: 用芯探核:基于龙芯的 Linux 内核探索解析
