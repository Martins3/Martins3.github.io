# page reclaim

## keynote
- `lru_add_drain` transfer all pages from the per-CPU LRU caches to the global lists

- get_scan_count : 没有 swap 分区的时候，不会处理 anonymous page lru

## TODO
- [ ] 奔跑吧 P310 对于 shrink 函数逐行分析，并没有阅读


- [ ] 所以 shmem 的内存是如何被回收的
    - [ ] 将 shmem 的内存当做 swap cache ?
    - [ ] super_operations::nr_cached_objects 用于处理 transparent_hugepage
- [ ] 当内存出现压力的时候，是不是有限清理 clean page cache, 最后清理 anonymous page cache 的写回操作。


1. 理清楚一个调用路线图
3. 和 dirty page 无穷无尽的关联

理清楚 shrinker，page reclaim ，compaction 之间的联系 !
似乎是唯一的和 shrinerk 的 lwn  https://lwn.net/Articles/550463/
1. For the inode cache, that is done by a "shrinker" function provided by the virtual filesystem layer.[^2]

## 基本流程
1. direct reclaim 的过程

- try_to_free_pages
  - do_try_to_free_pages
    - shrink_node
      - shrink_node_memcgs
        - shrink_lruvec + shrink_slab
          - shrink_list
            - shrink_inactive_list (shrink_active_list)
              - shrink_folio_list

2. kswapd 的 reclaim 过程
```txt
#0  shrink_folio_list (folio_list=folio_list@entry=0xffffc9000126fc30, pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc9000126fdd8, stat=stat@entry=0xffffc9000126fcb8, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1632
#1  0xffffffff812af3b8 in shrink_inactive_list (lru=LRU_INACTIVE_FILE, sc=0xffffc9000126fdd8, lruvec=0xffff888162038800, nr_to_scan=<optimized out>) at mm/vmscan.c:2489
#2  shrink_list (sc=0xffffc9000126fdd8, lruvec=0xffff888162038800, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_FILE) at mm/vmscan.c:2716
#3  shrink_lruvec (lruvec=lruvec@entry=0xffff888162038800, sc=sc@entry=0xffffc9000126fdd8) at mm/vmscan.c:5885
#4  0xffffffff812afc1f in shrink_node_memcgs (sc=0xffffc9000126fdd8, pgdat=0xffff88823fff9000) at mm/vmscan.c:6074
#5  shrink_node (pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc9000126fdd8) at mm/vmscan.c:6105
#6  0xffffffff812b0357 in kswapd_shrink_node (sc=0xffffc9000126fdd8, pgdat=0xffff88823fff9000) at mm/vmscan.c:6894
#7  balance_pgdat (pgdat=pgdat@entry=0xffff88823fff9000, order=order@entry=0, highest_zoneidx=highest_zoneidx@entry=3) at mm/vmscan.c:7084
#8  0xffffffff812b090b in kswapd (p=0xffff88823fff9000) at mm/vmscan.c:7344
#9  0xffffffff81133853 in kthread (_create=0xffff8881211ff440) at kernel/kthread.c:376
#10 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#11 0x0000000000000000 in ?? ()
```

- [ ] shrink_lruvec 是什么操作
- [ ] shrink_node_memcgs 这个名字取得好啊

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
    nodemask_t  *nodemask;

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

## multi generation LRU
https://lwn.net/Articles/851184/ 似乎现在又发布了一个 LRU 了


// 简要说明一下在 mem/list_lru.c 中间的内容:

```c
 struct list_lru {
    struct list_lru_node    *node;
 #ifdef CONFIG_MEMCG_KMEM
    struct list_head    list;
    int         shrinker_id;
 #endif
 };
```
> 从其中跟踪的代码看，似乎只有inode 的管理在使用lru_list

- [ ] enum pageflags::PG_lru ? why is special ? why we need time for it ?
  - [ ] check `SetPageLRU`

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

## Per memcg lru locking
- [Per memcg lru locking 合并的过程](https://mp.weixin.qq.com/s/7eDqHR06TIBh6hqUMTrZKg)

- shrink_zones / balance_pgdat
  - mem_cgroup_soft_limit_reclaim
