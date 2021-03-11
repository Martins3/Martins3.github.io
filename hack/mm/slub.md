# slub



认真重读一下:
- https://ruffell.nz/programming/writeups/2019/02/15/looking-at-kmalloc-and-the-slub-memory-allocator.html

[Wowotech](http://www.wowotech.net/memory_management/426.html)
[LoyenWang](https://www.cnblogs.com/LoyenWang/p/11922887.html)

- `struct kmem_cache`
- `struct kmem_cache_cpu`
- `struct kmem_cache_node`

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


```c
struct page {
	unsigned long flags;		/* Atomic flags, some possibly
					 * updated asynchronously */
	/*
	 * Five words (20/40 bytes) are available in this union.
	 * WARNING: bit 0 of the first word is used for PageTail(). That
	 * means the other users of this union MUST NOT use the bit to
	 * avoid collision and false-positive PageTail().
	 */
	union {
		struct {	/* slab, slob and slub */
			union {
				struct list_head slab_list;
				struct {	/* Partial pages */
					struct page *next;
#ifdef CONFIG_64BIT
					int pages;	/* Nr of pages left */
					int pobjects;	/* Approximate count */
#else
					short int pages;
					short int pobjects;
#endif
				};
			};
			struct kmem_cache *slab_cache; /* not slob */
			/* Double-word boundary */
			void *freelist;		/* first free object */
			union {
				void *s_mem;	/* slab: first object */
				unsigned long counters;		/* SLUB */
				struct {			/* SLUB */
					unsigned inuse:16;
					unsigned objects:15;
					unsigned frozen:1;
				};
			};
		};
  }
```

- [x] kmem_cache_create
![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191124161239520-610202035.png)


- [ ] kmem_cache_alloc
![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191124161307831-1244111963.png)

***Let's understand some basic function:***
- [x] get_freepointer_safe
    - `*(object + s->offset)` : because every free object contains the pointer points to next free object
    - ==> get_freepointer ==> freelist_dereference ==> freelist_ptr 
    - freelist_ptr(s, ptr, ptr_addr){return s}

- [ ] calculate_sizes()


- [x] get_freelist() : 
    -  So, we should check 
    - Check the `page->freelist` of a page and either transfer the freelist to the per cpu freelist or deactivate the page.
    - ![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191124161409130-2017775465.png)

- [x] new_slab_objects() : allocate and initialize it as a slab page, it called we can't find a page in partial

- [ ] `kmem_cache::inuse` and `page::(anonymous union)::(anonymous struct)::(anonymous union)::(anonymous struct)::inuse`
  - [ ] struct page结构体中inuse代表已经使用的obj数量。

- [x] discard_slab() : free page to buddy system if it's empty.
- [x] add_partial() : add to **node** partial list
- [x] put_cpu_partial() : add to **percpu** partial list
- [x] unfreeze_partials() : move all the pages in cpu partial list to node partial list, and unfreeze them.



***Let's understand some basic concepts:***

- [x] frozen
一个 slab 被某个 CPU freeze 后，就只有这个 CPU 能够从这个 slab 分配对象。
具体那些 slab 是冻结的 ? 首先，本地 slab 总是被冻结的；其次，脱离链表的全满 slba 在重新获得一个被释放的空闲对象后，
会进入本地 partial 链表并且被冻结。而其他的 slab, 如 numa node partial 列表, 刚刚从numa node 列表移入本地 partial 链表中的 slab,
以及脱离链表的全满 slab 都是不冻结的。[^1] 


- [x] partial
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

[^1]: 用芯探核:基于龙芯的 Linux 内核探索解析
