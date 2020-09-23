# compaction.c 文件分析


```c
// 将page 从 freelist 中间删除并且释放
static unsigned long release_freepages(struct list_head *freelist)
```


```c
// migrate 在当前的函数面前的含义是什么 ? mobility
// isolate 的作用 : 确定可以移动的page ? 那么确定free 的工作在哪里 ?

/*
 * Based on information in the current compact_control, find blocks
 * suitable for isolating free pages from and then isolate them.
 */
static void isolate_freepages(struct compact_control *cc)
// 看来扫描是按照 page block 进行的，TODO page block 的好处是什么 ?
// 标记page 移动性质是按照page block 大小进行的，而不是每一个page 单独分析的




// 
/*
 * Isolate all pages that can be migrated from the first suitable block,
 * starting at the block pointed to by the migrate scanner pfn within
 * compact_control.
 */
static isolate_migrate_t isolate_migratepages(struct zone *zone,
					struct compact_control *cc)
```

```c
// 文件的最后的部分, 看来回收的工作需要单独的线程维持的
int kcompactd_run(int nid)
```


# 总结

## page block
采用page block 的原因是什么 ? 按照Mobility 将page group 在一起。

- Free page goes to UNMOVABLE free listas the pageblock is UNMOVABLE.
> free list 中间显然挂的是page 而不是 page block

- Some pages are freed within UNMOVABLE pageblock, so they go to UNMOVABLE freelist
> 如果一个page 出现在page block 中间，然后就会被标记为 unmoveable 那岂不是 ?

- UNMOVABLE allocation steals all free pages from the pageblock
(too few to also “repaint” the pageblock) and grabs the smallest
> 找到对应的位置 !

page block 的好处 ? @todo 
如果不使用page block，那么，需要在page 中间使用3bit 来标识，


如果free list 中间含有migrate type，同时 page block 持有 migrate type ，没有冲突吗？
1. free list 的选择被 GFP 的确定
2. pageblock 应该是在扫描isolate的时候使用，当一个page处于被use 的状态，其mobility 被所处的page block 确定

> TODO
反向fallback 除非表示为 steal 整个page block 的，page block 让扫描变的简单，但是 reclaimable 中间放置一个 movable，
感觉也确定当前的page in use or not 也不是很好判断 !

## fallback
compaction.c 中间的内容，实现compaction 的动作，然而支持这个操作的复杂性都被放到 page_alloc.c 中间。

```c

// 唯一使用 fallback 的位置
/*
 * Check whether there is a suitable fallback freepage with requested order.
 * If only_stealable is true, this function returns fallback_mt only if
 * we can steal other freepages all together. This would help to reduce
 * fragmentation due to mixed migratetype pages in one pageblock.
 */
int find_suitable_fallback(struct free_area *area, unsigned int order,
			int migratetype, bool only_stealable, bool *can_steal)


/*
 * Try finding a free buddy page on the fallback list and put it on the free
 * list of requested migratetype, possibly along with other pages from the same
 * block, depending on fragmentation avoidance heuristics. Returns true if
 * fallback was found so that __rmqueue_smallest() can grab it.
 *
 * The use of signed ints for order and current_order is a deliberate
 * deviation from the rest of this file, to make the for loop
 * condition simpler.
 */
static __always_inline bool
__rmqueue_fallback(struct zone *zone, int order, int start_migratetype)
// TODO 这个函数，区分 fallback 和 steal 的两个操作的地方

```



# https://lwn.net/Articles/379859/


# [Memory compaction](https://lwn.net/Articles/368869/)

Over the years, the kernel developers have made various attempts to mitigate this problem; techniques like `ZONE_MOVABLE` and `lumpy` reclaim have been the result.

Eventually the two algorithms will meet somewhere toward the middle of the zone. At that point, it's mostly just a matter of invoking the **page migration** code (which is not just for NUMA systems anymore) to shift the used pages to the free space at the top of the zone.
> 两个算法，一个查找 reclaimable 和 movable page 另一个找到free page 然后采用 page migration 移动 page 就可以了

Not all pages can be moved at will; only those which are addressed through a layer of indirection and which are not otherwise pinned down are movable.

The running of the compaction algorithm can be triggered in either of two ways. One is to write a node number to /proc/sys/vm/compact_node, causing compaction to happen on the indicated NUMA node. The other is for the system to fail in an attempt to allocate a higher-order page; in this case, compaction will run as a preferable alternative to freeing pages through direct reclaim. In the absence of an explicit trigger, the compaction algorithm will stay idle; there is a cost to moving pages around which is best avoided if it is not needed.
> compaction 的代价很高，不会被轻易触发 !

# [Short topics in memory management](https://lwn.net/Articles/224829/)
Mel Gorman's fragmentation avoidance patches have been discussed here a few times in the past.
The core idea behind Mel's work is to identify pages which can be easily moved or reclaimed and group them together.
Movable pages include those allocated to user space; 
**moving them is just a matter of changing the relevant page table entries.**
Reclaimable pages include kernel caches which can be released should the need arise.
Grouping these pages together makes it easy for the kernel to free large blocks of memory, which is useful for enabling high-order allocations or for vacating regions of memory entirely.

In the past, reviewers of Mel's patches have disagreed over how they should work. 
Some argue in favor of maintaining separate free lists for the different types of allocations, 
while others feel that this sort of memory partitioning is just what the kernel's zone system was created to do. 
So, this time around, Mel has posted two sets of patches: a list-based grouping mechanism and a new ZONE_MOVABLE zone which is restricted to movable allocations.
> 将类似的放到list上或者zone上，但是其中的工作 ?

# [Avoiding - and fixing - memory fragmentation](https://lwn.net/Articles/211505/)
But there are a few situations where physically-contiguous memory is absolutely required. These include large kernel data structures (except those created with vmalloc()) and any memory which must appear contiguous to peripheral devices. DMA buffers for low-end devices (those which cannot do scatter/gather I/O) are a classic example. If a large ("high order") block of memory is not available when needed, something will fail and yet another user will start to consider switching to BSD.
> 介绍需要使用连续物理空间的地方

The core observation in Mel's patch set remains that some types of memory are more easily reclaimed than others.
*A page which is backed up on a filesystem somewhere can be readily discarded and reused*,
for example, while a page holding a process's task structure is pretty well nailed down.
One stubborn page is all it takes to keep an entire large block of memory from being consolidated and reused as a physically-contiguous whole.
But if all of the easily-reclaimable pages could be kept together, with the non-reclaimable pages grouped into a separate region of memory, 
it should be much easier to create larger blocks of free memory.
> 将不同的移动属性的page 放到一起。

So Mel's patch divides each memory zone into three types of blocks: non-reclaimable, easily reclaimable, and movable.
The "movable" type is a new feature in this patch set; it is used for pages which can be easily shifted elsewhere using the kernel's page migration mechanism. 
*In many cases, moving a page might be easier than reclaiming it, since there is no need to involve a backing store device.*
Grouping pages in this way should also make the creation of larger blocks "just happen" when a process is migrated from one NUMA node to another.

So, in this patch, movable pages (those marked with `__GFP_MOVABLE`) are generally those belonging to user-space processes. 
Moving a user-space page is just a matter of copying the data and changing the page table entry, so it is a relatively easy thing to do.
Reclaimable pages (`__GFP_RECLAIMABLE`), instead, usually belong to the kernel.
They are either allocations which are expected to be short-lived (some kinds of DMA buffers, for example, which only exist for the duration of an I/O operation)
or can be discarded if needed (various types of caches). Everything else is expected to be hard to reclaim.
> 1. 用户数据可以随便移动，copy 数据然后修改pte 即可。
> 2. 但是内核态的采用的是线性映射，如果将页面进行移动，那么指向其的指针全部需要进行修改，但是很难找到指向其的指针。
> 3. 内核backing store的方法 : 将该page 写会。
> 
> 总体来说，移动，虽然代价很大，但是也总比分配空间失败好!


# https://www.uninformativ.de/blog/postings/2017-12-23/0/POSTING-en.html
> 用户态的分析，

> `echo 1 >/proc/sys/vm/compact_memory` 在内核中间的handler

```c
/*
 * This is the entry point for compacting all nodes via
 * /proc/sys/vm/compact_memory
 */
int sysctl_compaction_handler(struct ctl_table *table, int write,
			void __user *buffer, size_t *length, loff_t *ppos)
{
	if (write)
		compact_nodes();

	return 0;
}


/* Compact all nodes in the system */
static void compact_nodes(void)
{
	int nid;

  // TODO 使用了 work queue 的内容，值的分析
  // drain 的含义也是值得分析的
	/* Flush pending updates to the LRU lists */
	lru_add_drain_all();

	for_each_online_node(nid)
		compact_node(nid);
}


// 由node 到 zone 的 compaction
static enum compact_result compact_zone(struct zone *zone, struct compact_control *cc)
  // ...
	migrate_prep_local(); // 调用 lru_add_drain_cpu(int cpu) ，显然尚且不知道在干什么 !

	while ((ret = compact_finished(zone, cc)) == COMPACT_CONTINUE) {
    // ...
    // 其工作就是移动页面，但是 TODO 为什么可以将 lru 和 migrate 联系在一起，不清楚啊 !
		err = migrate_pages(&cc->migratepages, compaction_alloc,
				compaction_free, (unsigned long)cc, cc->mode,
				MR_COMPACTION);
    // ...
check_drain:
```
