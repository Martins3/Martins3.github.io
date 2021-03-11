## page reclaim

## TODO
- [ ] 奔跑吧 P315 workingset.c refault 这个主题
- [ ] 奔跑吧 P310 对于 shrink 函数逐行分析，并没有阅读


- [ ] 所以 shmem 的内存是如何被回收的
    - [ ] 将 shmem 的内存当做 swap cache ?
    - [ ] super_operations::nr_cached_objects 用于处理 transparent_hugepage


1. 理清楚一个调用路线图
2. mark page accessed 和 page reference
3. 和 dirty page 无穷无尽的关联

理清楚 shrinker，page reclaim ，compaction 之间的联系 !
似乎是唯一的和 shrinerk 的 lwn  https://lwn.net/Articles/550463/
1. For the inode cache, that is done by a "shrinker" function provided by the virtual filesystem layer.[^2]

1. vmscan.c 的 direct reclaim 的过程
```c
try_to_free_pages
  do_try_to_free_pages
    shrink_node
      shrink_node_memcgs
        shrink_lruvec + shrink_slab

          shrink_list
            shrink_inactive_list (shrink_active_list)
              shrink_page_list
```

```c
struct scan_control {
	/* How many pages shrink_list() should reclaim */
	unsigned long nr_to_reclaim;

	/* This context's GFP mask */
	gfp_t gfp_mask;

	/* Allocation order */
	int order;

	/*
	 * Nodemask of nodes allowed by the caller. If NULL, all nodes
	 * are scanned.
	 */
	nodemask_t	*nodemask;

	/*
	 * The memory cgroup that hit its limit and as a result is the
	 * primary target of this reclaim invocation.
	 */
	struct mem_cgroup *target_mem_cgroup;

	/* Scan (total_size >> priority) pages at once */
	int priority;

	/* The highest zone to isolate pages for reclaim from */
	enum zone_type reclaim_idx;

	/* Writepage batching in laptop mode; RECLAIM_WRITE */
	unsigned int may_writepage:1;

	/* Can mapped pages be reclaimed? */
	unsigned int may_unmap:1;

	/* Can pages be swapped as part of reclaim? */
	unsigned int may_swap:1;

	/*
	 * Cgroups are not reclaimed below their configured memory.low,
	 * unless we threaten to OOM. If any cgroups are skipped due to
	 * memory.low and nothing was reclaimed, go back for memory.low.
	 */
	unsigned int memcg_low_reclaim:1;
	unsigned int memcg_low_skipped:1;

	unsigned int hibernation_mode:1;

	/* One of the zones is ready for compaction */
	unsigned int compaction_ready:1;

	/* Incremented by the number of inactive pages that were scanned */
	unsigned long nr_scanned;

	/* Number of pages freed so far during a call to shrink_zones() */
	unsigned long nr_reclaimed;
};
```
- nr_to_reclaim：需要回收的页面数量；
- gfp_mask：申请分配的掩码，用户申请页面时可以通过设置标志来限制调用底层文件系统或不允许读写存储设备，最终传递给LRU处理；
- order：申请分配的阶数值，最终期望内存回收后能满足申请要求；
- nodemask：内存节点掩码，空指针则访问所有的节点；
- priority：扫描LRU链表的优先级，用于计算每次扫描页面的数量(total_size >> priority，初始值12)，值越小，扫描的页面数越大，逐级增加扫描粒度；
- may_writepage：是否允许把修改过文件页写回存储设备；
- may_unmap：是否取消页面的映射并进行回收处理；
- may_swap：是否将匿名页交换到swap分区，并进行回收处理；
- nr_scanned：统计扫描过的非活动页面总数；
- nr_reclaimed：统计回收了的页面总数


#### lru
// 首先阅读的内容 : https://lwn.net/Articles/550463/

- [ ] https://mp.weixin.qq.com/s/7eDqHR06TIBh6hqUMTrZKg
- [ ] LoyenWang seems cover this part too.

// 简要说明一下在 mem/list_lru.c 中间的内容:

```c
 struct list_lru {
 	struct list_lru_node	*node;
 #ifdef CONFIG_MEMCG_KMEM
 	struct list_head	list;
 	int			shrinker_id;
 #endif
 };
```
> 从其中跟踪的代码看，似乎只有inode 的管理在使用lru_list

- [ ] enum pageflags::PG_lru ? why is special ? why we need time for it ?
  - [ ] check `SetPageLRU`

move_pages_to_lru

- [ ] page_evictable() and PageMovable()
  - [ ] I think, if a page can be evicted to swap, so it can movable too.


![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191109175747860-1724945792.png)
上图中，主要实现的功能就是将CPU缓存的页面，转移到lruvec链表中，而在转移过程中，最终会调用pagevec_lru_move_fn函数，实际的转移函数是传递给pagevec_lru_move_fn的函数指针。在这些具体的转移函数中，会对Page结构状态位进行判断，清零，设置等处理，并最终调用del_page_from_lru_list/add_page_to_lru_list接口来从一个链表中删除，并加入到另一个链表中。

上述的每个CPU5种缓存struct pagevec，基本描述了LRU链表的几种操作：
- lru_add_pvec：缓存不属于LRU链表的页，新加入的页；
- lru_rotate_pvecs：缓存已经在INACTIVE LRU链表中的非活动页，将这些页添加到INACTIVE LRU链表的尾部；
- lru_deactivate_pvecs：缓存已经在ACTIVE LRU链表中的页，清除掉PG_activate, PG_referenced标志后，将这些页加入到INACTIVE LRU链表中；
- lru_lazyfree_pvecs：缓存匿名页，清除掉PG_activate, PG_referenced, PG_swapbacked标志后，将这些页加入到LRU_INACTIVE_FILE链表中；
- activate_page_pvecs：将LRU中的页加入到ACTIVE LRU链表中；

find reference of `pagevec_lru_move_fn`, we can find all the `void (*move_fn)(struct page *page, struct lruvec *lruvec, void *arg)`

```c
/*
 * Add the passed pages to the LRU, then drop the caller's refcount
 * on them.  Reinitialises the caller's pagevec.
 */
void __pagevec_lru_add(struct pagevec *pvec)
{
    //直接调用pagevec_lru_move_fn函数，并传入转移函数指针
	pagevec_lru_move_fn(pvec, __pagevec_lru_add_fn, NULL);
}
EXPORT_SYMBOL(__pagevec_lru_add);

static void pagevec_lru_move_fn(struct pagevec *pvec,
	void (*move_fn)(struct page *page, struct lruvec *lruvec, void *arg),
	void *arg)
{
	int i;
	struct pglist_data *pgdat = NULL;
	struct lruvec *lruvec;
	unsigned long flags = 0;

    //遍历缓存中的所有页
	for (i = 0; i < pagevec_count(pvec); i++) {
		struct page *page = pvec->pages[i];
		struct pglist_data *pagepgdat = page_pgdat(page);

       //判断是否为同一个node，同一个node不需要加锁，否则需要加锁处理
		if (pagepgdat != pgdat) {
			if (pgdat)
				spin_unlock_irqrestore(&pgdat->lru_lock, flags);
			pgdat = pagepgdat;
			spin_lock_irqsave(&pgdat->lru_lock, flags);
		}

       //找到目标lruvec，最终页转移到该结构中的LRU链表中
		lruvec = mem_cgroup_page_lruvec(page, pgdat);
		(*move_fn)(page, lruvec, arg);  //根据传入的函数进行回调
	}
	if (pgdat)
		spin_unlock_irqrestore(&pgdat->lru_lock, flags);
    //减少page的引用值，当引用值为0时，从LRU链表中移除页表并释放掉
	release_pages(pvec->pages, pvec->nr, pvec->cold);
    //重置pvec结构
	pagevec_reinit(pvec);
}

static void __pagevec_lru_add_fn(struct page *page, struct lruvec *lruvec,
				 void *arg)
{
	int file = page_is_file_cache(page);
	int active = PageActive(page);
	enum lru_list lru = page_lru(page);

	VM_BUG_ON_PAGE(PageLRU(page), page);
    //设置page的状态位，表示处于Active状态
	SetPageLRU(page);
    //加入到链表中
	add_page_to_lru_list(page, lruvec, lru);
    //更新lruvec中的reclaim_state统计信息
	update_page_reclaim_stat(lruvec, file, active);
	trace_mm_lru_insertion(page, lru);
}
```
具体的分析在注释中标明了，其余4种缓存类型的迁移都大体类似，至于何时进行迁移以及策略，这个在下文中关于内存回收的进一步分析中再阐述。

正常情况下，LRU链表之间的转移是不需要的，只有在需要进行内存回收的时候，才需要去在ACTIVE和INACTIVE之间去操作。

进入具体的回收分析吧。

#### direct shrink
- [ ] So, where's indirect shrink, polish documentations here.


![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191109175827519-2018632360.png)

-  [ ] [LoyenWang](https://www.cnblogs.com/LoyenWang/p/11827153.html) section 3.3



#### kswapd
- [^29] present a beautiful graph ![](https://oscimg.oschina.net/oscnet/33f9024d70cd92f9cc711df451500aa6047.jpg)

- relation between `kswapd` and `kcompactd`: if kswapd free some pages, then we can wake it up and compact pages

- [ ] [LoyenWang](https://www.cnblogs.com/LoyenWang/p/11827153.html) section 3.4


#### shrink_node

get_scan_count

```c
enum scan_balance {
	SCAN_EQUAL,  // 计算出的扫描值按原样使用
	SCAN_FRACT,  // 将分数应用于计算的扫描值
	SCAN_ANON,  // 对于文件页LRU，将扫描次数更改为0
	SCAN_FILE,     // 对于匿名页LRU，将扫描次数更改为0
};
```

![loading](https://img2018.cnblogs.com/blog/1771657/201911/1771657-20191109180043631-1027693945.png)

watch out : rename `shrink_node_memcg` to `shrink_lruvec`

**TO BE CONTINUE, the LoyenWang's blog with code**

#### shrink slab
1. 面试问题 : dcache 的需要 slab，slab 分配需要 page，那么 page cache, slab 和 dcache 的回收之间的关系是什么
   1. 思考 : slab 都是提供 kmalloc 的内容，凭什么，可以释放
   2. 或者本来，释放 slab 就不是特制一个例子，而是一个 shrinker 机制，专门用于释放内核的分配的数据
   3. 其他 page cache 以及用户页面就是我们熟悉的 lru 算法进行处理了

shrink 的接口和使用方法是什么 ?

这是唯一的使用位置:
```c
static unsigned long shrink_slab(gfp_t gfp_mask, int nid,
				 struct mem_cgroup *memcg,
				 int priority)
         // 对于所有的注册的 shrinker 循环调用 do_shrink_slab
         // 也就是说，其实每个 cache shrinker 之间其实没有什么关系

static unsigned long do_shrink_slab(struct shrink_control *shrinkctl,
				    struct shrinker *shrinker, int priority)
```
然后，根据上方的调用路线，课题知道，猜测应该是对的，对于 page 和 slab 分成两个，slab 利用各种 shrinker 进行
