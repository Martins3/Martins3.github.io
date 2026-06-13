# page reclaim

## keynote
- `lru_add_drain` transfer all pages from the per-CPU LRU caches to the global lists

- get_scan_count : 没有 swap 分区的时候，不会处理 anonymous page lru

## TODO
- [ ] 对于 THP 的页面，如何进行 reclaim 的
- [ ] 奔跑吧 P310 对于 shrink 函数逐行分析，并没有阅读


- [ ] 所以 shmem 的内存是如何被回收的
    - [ ] 将 shmem 的内存当做 swap cache ?
    - [ ] super_operations::nr_cached_objects 用于处理 transparent_hugepage
- [ ] 当内存出现压力的时候，是不是有限清理 clean page cache, 最后清理 anonymous page cache 的写回操作。


1. 理清楚一个调用路线图

理清楚 shrinker，page reclaim ，compaction 之间的联系 !
似乎是唯一的和 shrinerk 的 lwn  https://lwn.net/Articles/550463/
1. For the inode cache, that is done by a "shrinker" function provided by the virtual filesystem layer.[^2]

## 总结

### dirty file page 是如何被释放的
- [ ] 出现内存压力，会加快的 dirty page 的写回吗?

shrink_inactive_list 中存在如下代码：
```c
	/*
	 * If dirty folios are scanned that are not queued for IO, it
	 * implies that flushers are not doing their job. This can
	 * happen when memory pressure pushes dirty folios to the end of
	 * the LRU before the dirty limits are breached and the dirty
	 * data has expired. It can also happen when the proportion of
	 * dirty folios grows not through writes but through memory
	 * pressure reclaiming all the clean cache. And in some cases,
	 * the flushers simply cannot keep up with the allocation
	 * rate. Nudge the flusher threads in case they are asleep.
	 */
	if (stat.nr_unqueued_dirty == nr_taken) {
		wakeup_flusher_threads(WB_REASON_VMSCAN);
		/*
		 * For cgroupv1 dirty throttling is achieved by waking up
		 * the kernel flusher here and later waiting on folios
		 * which are in writeback to finish (see shrink_folio_list()).
		 *
		 * Flusher may not be able to issue writeback quickly
		 * enough for cgroupv1 writeback throttling to work
		 * on a large system.
		 */
		if (!writeback_throttling_sane(sc))
			reclaim_throttle(pgdat, VMSCAN_THROTTLE_WRITEBACK);
	}
```

### dirty anon page
shrink_folio_list 中:

- 如果是匿名页面，调用 add_to_swap 加入到 swapcache 中:
- 写回，但是无需等待存储设备的返回，page 放到 inactive lru 的头中。
- 如果 clean 了，在下次扫描的过程中，在 `__remove_mapping` 中释放 page 。

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
- gfp_mask：申请分配的掩码，用户申请页面时可以通过设置标志来限制调用底层文件系统或不允许读写存储设备，最终传递给 LRU 处理；
- order：申请分配的阶数值，最终期望内存回收后能满足申请要求；
- nodemask：内存节点掩码，空指针则访问所有的节点；
- priority：扫描 LRU 链表的优先级，用于计算每次扫描页面的数量(total_size >> priority，初始值 12)，值越小，扫描的页面数越大，逐级增加扫描粒度；
- may_writepage：是否允许把修改过文件页写回存储设备；
- may_unmap：是否取消页面的映射并进行回收处理；
- may_swap：是否将匿名页交换到 swap 分区，并进行回收处理；
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
> 从其中跟踪的代码看，似乎只有 inode 的管理在使用 lru_list

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

### 4
- 注入给 swap 的错误最后返回给谁？
  - 不会返回给任何人的。

- 如果采样的都是 read 的，为什么 write 不成功会被阻塞的哇?

```txt
#3  shrink_list (sc=0xffffc9004337fc68, lruvec=0xffff888351c41800, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2767
#4  shrink_lruvec (lruvec=lruvec@entry=0xffff888351c41800, sc=sc@entry=0xffffc9004337fc68) at mm/vmscan.c:5951
#5  0xffffffff812fa20e in shrink_node_memcgs (sc=0xffffc9004337fc68, pgdat=0xffff8883bfffc000) at mm/vmscan.c:6138
#6  shrink_node (pgdat=pgdat@entry=0xffff8883bfffc000, sc=sc@entry=0xffffc9004337fc68) at mm/vmscan.c:6169
#7  0xffffffff812fb450 in shrink_zones (sc=0xffffc9004337fc68, zonelist=<optimized out>) at mm/vmscan.c:6407
#8  do_try_to_free_pages (zonelist=zonelist@entry=0xffff8884bfffbb00, sc=sc@entry=0xffffc9004337fc68) at mm/vmscan.c:6469
#9  0xffffffff812fc367 in try_to_free_mem_cgroup_pages (memcg=memcg@entry=0xffff888346219000, nr_pages=nr_pages@entry=1, gfp_mask=gfp_mask@entry=3264, reclaim_options=reclaim_options@entry=2, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/vmscan.c:6786
#10 0xffffffff813a0c2a in try_charge_memcg (memcg=memcg@entry=0xffff888346219000, gfp_mask=gfp_mask@entry=3264, nr_pages=1) at mm/memcontrol.c:2687
#11 0xffffffff813a1b9d in try_charge (nr_pages=1, gfp_mask=3264, memcg=0xffff888346219000) at mm/memcontrol.c:2830
#12 charge_memcg (folio=folio@entry=0xffffea0012ffc180, memcg=memcg@entry=0xffff888346219000, gfp=gfp@entry=3264) at mm/memcontrol.c:6947
#13 0xffffffff813a3498 in __mem_cgroup_charge (folio=0xffffea0012ffc180, mm=<optimized out>, gfp=gfp@entry=3264) at mm/memcontrol.c:6968
#14 0xffffffff8132a69b in mem_cgroup_charge (gfp=3264, mm=<optimized out>, folio=<optimized out>) at ./include/linux/memcontrol.h:671
#15 do_anonymous_page (vmf=0xffffc9004337fdf8) at mm/memory.c:4078
#16 handle_pte_fault (vmf=0xffffc9004337fdf8) at mm/memory.c:4929
#17 __handle_mm_fault (vma=vma@entry=0xffff88811ea858e8, address=address@entry=140668916269056, flags=flags@entry=597) at mm/memory.c:5073
#18 0xffffffff8132b104 in handle_mm_fault (vma=0xffff88811ea858e8, address=address@entry=140668916269056, flags=<optimized out>, flags@entry=597, regs=regs@entry=0xffffc9004337ff58) at mm/memory.c:5219
#19 0xffffffff8110d937 in do_user_addr_fault (regs=regs@entry=0xffffc9004337ff58, error_code=error_code@entry=6, address=address@entry=140668916269056) at arch/x86/mm/fault.c:1428
#20 0xffffffff8218d606 in handle_page_fault (address=140668916269056, error_code=6, regs=0xffffc9004337ff58) at arch/x86/mm/fault.c:1519
#21 exc_page_fault (regs=0xffffc9004337ff58, error_code=6) at arch/x86/mm/fault.c:1575
#22 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
- 各种 shrink_list 无法立刻保证，那么需要循环多少次?

- do_anonymous_page 中如果 mem_cgroup_charge 失败
  - try_charge_memcg
    - try_to_free_mem_cgroup_pages : 尝试释放内存


可以观测的一个的统计事件:
```c
memcg_memory_event(mem_over_limit, MEMCG_MAX);
```

从 shrink_folio_list 中看，只有 free 页面才会是算作 nr_reclaimed 的。

### 5
如果使用 fault injection 机制，就不会堵塞在哪里，将会是 100% CPU 消耗，但是阻塞的位置不是在 try_charge_memcg

shrink_folio_list 中，发现所有的 page 都没有办法写回，
```c
			switch (pageout(folio, mapping, &plug)) {
			case PAGE_KEEP:
				goto keep_locked;
			case PAGE_ACTIVATE:
				goto activate_locked;
			case PAGE_SUCCESS:
				stat->nr_pageout += nr_pages;

				if (folio_test_writeback(folio)){
					goto keep;
				}
				if (folio_test_dirty(folio)){
                    // 从这里直接跳转到 while 循环的尾部
					goto keep;
				}
```

shrink_inactive_list 中调用了 mem_cgroup_uncharge_list，
实际上，这些 page 还是在被使用的，folio_list 中都是没有释放的，
然后就被释放了，也许对于 mem_cgroup_uncharge_list

- shrink_lruvec 中是无法离开的!

```c
	while (nr[LRU_INACTIVE_ANON] || nr[LRU_ACTIVE_FILE] ||
					nr[LRU_INACTIVE_FILE]) {
		unsigned long nr_anon, nr_file, percentage;
		unsigned long nr_scanned;

		for_each_evictable_lru(lru) {
			if (nr[lru]) {
				nr_to_scan = min(nr[lru], SWAP_CLUSTER_MAX);
				nr[lru] -= nr_to_scan;

				nr_reclaimed += shrink_list(lru, nr_to_scan,
							    lruvec, sc);
			}
		}

		cond_resched();
		if (nr_reclaimed < nr_to_reclaim || proportional_reclaim)
			continue;
```

而且是无法杀掉的。

```c
		pr_info("[huxueshi:%s:%d] %ld %ld\n", __FUNCTION__, __LINE__, nr_reclaimed, nr_to_reclaim);
```
```txt
Dec 28 15:00:09 localhost.localdomain kernel: vmscan: [huxueshi:shrink_lruvec:5958] 1 32
```
按道理来说，是一个都不行的，但是 nr_reclaimed 居然还可以回收一个。

这里并不是死循环才对的，而且跳出还很快，细节之后分析吧。

### scsi debug 注入错误
如果是使用 scsi debug 的方式，会在 wbt 上阻塞。

其实是类似的方式，但是这个 write 循环太过于频繁，甚至导致 wbt 都其作用了。

```txt
#0  end_swap_bio_write (bio=0xffff888101892300) at mm/page_io.c:32
#1  0xffffffff816ccfcc in req_bio_endio (error=10 '\n', nbytes=524288, bio=0xffff888101892300, rq=0xffff88812793c800) at block/blk-mq.c:794
#2  blk_update_request (req=req@entry=0xffff88812793c800, error=error@entry=10 '\n', nr_bytes=524288) at block/blk-mq.c:926
#3  0xffffffff81ad5b42 in scsi_end_request (req=req@entry=0xffff88812793c800, error=error@entry=10 '\n', bytes=<optimized out>) at drivers/scsi/scsi_lib.c:539
#4  0xffffffff81ad6a9b in scsi_io_completion_action (result=<optimized out>, cmd=0xffff88812793c910) at drivers/scsi/scsi_lib.c:841
#5  scsi_io_completion (cmd=0xffff88812793c910, good_bytes=<optimized out>) at drivers/scsi/scsi_lib.c:996
#6  0xffffffff816c9c78 in blk_complete_reqs (list=<optimized out>) at block/blk-mq.c:1131
#7  0xffffffff8219fe14 in __do_softirq () at kernel/softirq.c:571
#8  0xffffffff811318ca in invoke_softirq () at kernel/softirq.c:445
#9  __irq_exit_rcu () at kernel/softirq.c:650
#10 0xffffffff8218cfa6 in sysvec_apic_timer_interrupt (regs=0xffffc90042adfae8) at arch/x86/kernel/apic/apic.c:1107
```

```txt
#0  __wbt_done (wb_acct=WBT_TRACKED, rqos=0xffff888354881558) at block/blk-wbt.c:176
#1  wbt_done (rqos=0xffff888354881558, rq=0xffff88812793c800) at block/blk-wbt.c:201
#2  0xffffffff816dc6f0 in __rq_qos_done (rqos=0xffff888354881558, rq=rq@entry=0xffff88812793c800) at block/blk-rq-qos.c:39
#3  0xffffffff816cbdf1 in rq_qos_done (rq=0xffff88812793c800, q=0xffff8883413b3390) at block/blk-rq-qos.h:178
#4  blk_mq_free_request (rq=rq@entry=0xffff88812793c800) at block/blk-mq.c:742
#5  0xffffffff816cbf03 in __blk_mq_end_request (rq=rq@entry=0xffff88812793c800, error=error@entry=10 '\n') at block/blk-mq.c:1044
#6  0xffffffff81ad5bf9 in scsi_end_request (req=req@entry=0xffff88812793c800, error=error@entry=10 '\n', bytes=<optimized out>) at drivers/scsi/scsi_lib.c:574
#7  0xffffffff81ad6a9b in scsi_io_completion_action (result=<optimized out>, cmd=0xffff88812793c910) at drivers/scsi/scsi_lib.c:841
#8  scsi_io_completion (cmd=0xffff88812793c910, good_bytes=<optimized out>) at drivers/scsi/scsi_lib.c:996
#9  0xffffffff816c9c78 in blk_complete_reqs (list=<optimized out>) at block/blk-mq.c:1131
#10 0xffffffff8219fe14 in __do_softirq () at kernel/softirq.c:571
#11 0xffffffff811318ca in invoke_softirq () at kernel/softirq.c:445
#12 __irq_exit_rcu () at kernel/softirq.c:650
#13 0xffffffff8218cfa6 in sysvec_apic_timer_interrupt (regs=0xffffc90042adfae8) at arch/x86/kernel/apic/apic.c:1107
```
虽然是迅速的返回失败，但是在 wbt 的眼中，这个还是相当于 write 提交太快了。


### scsi debug 没有 wbt，可以立刻 OOM 掉
去掉 wbt 机制，对于 fault injection 是没有影响的。

- 但是 为什么去掉 wbt ，那么就会立刻 OOM 掉的
  - 在 mem cgroup 的限制下，通过 charge 收缩内存，如果不成功，会如何?

```txt
./fault/inject.sh: line 16:  1597 Bus error               (core dumped) cgexec -g memory:mem ./a.out
```

内核日志显示，死掉了，然后进行 read 的时候，进一步触发死亡的:
```txt
[   31.408189] vmscan: [huxueshi:shrink_lruvec:5958] 0 32
[   31.408378] vmscan: [huxueshi:shrink_lruvec:5958] 0 32
[   31.408563] vmscan: [huxueshi:shrink_lruvec:5958] 0 32
[   31.408748] vmscan: [huxueshi:shrink_lruvec:5958] 0 32
[   31.408935] vmscan: [huxueshi:shrink_lruvec:5958] 0 32
[   31.409120] vmscan: [huxueshi:shrink_lruvec:5958] 0 32
[   31.410510] systemd-coredum (1552) used greatest stack depth: 12880 bytes left
[   31.588199] Read-error on swap-device (8:64:131160)
[   31.590922] a.out (1549) used greatest stack depth: 11224 bytes left
➜  ~ dmesg | grep try_charge_memcg
[   31.221360] [huxueshi:try_charge_memcg:2743]
```

这，是不是有点没啥关联?
```txt
#0  huxueshi () at mm/memcontrol.c:2633
#1  0xffffffff82122868 in try_charge_memcg (memcg=memcg@entry=0xffff88810632e000, gfp_mask=gfp_mask@entry=1843402, nr_pages=512) at mm/memcontrol.c:2687
#2  0xffffffff813a1b3d in try_charge (nr_pages=512, gfp_mask=1843402, memcg=0xffff88810632e000) at mm/memcontrol.c:2845
#3  charge_memcg (folio=folio@entry=0xffffea0004c10000, memcg=memcg@entry=0xffff88810632e000, gfp=gfp@entry=1843402) at mm/memcontrol.c:6962
#4  0xffffffff813a3438 in __mem_cgroup_charge (folio=0xffffea0004c10000, mm=<optimized out>, gfp=gfp@entry=1843402) at mm/memcontrol.c:6983
#5  0xffffffff8138a7af in mem_cgroup_charge (gfp=1843402, mm=<optimized out>, folio=<optimized out>) at ./include/linux/memcontrol.h:671
#6  __do_huge_pmd_anonymous_page (gfp=1843402, page=0xffffea0004c10000, vmf=0xffffc90043187df8) at mm/huge_memory.c:662
#7  do_huge_pmd_anonymous_page (vmf=vmf@entry=0xffffc90043187df8) at mm/huge_memory.c:837
#8  0xffffffff8132a8ab in create_huge_pmd (vmf=0xffffc90043187df8) at mm/memory.c:4790
#9  __handle_mm_fault (vma=vma@entry=0xffff8881134f7390, address=address@entry=139697269506048, flags=flags@entry=597) at mm/memory.c:5043
#10 0xffffffff8132b0a4 in handle_mm_fault (vma=0xffff8881134f7390, address=address@entry=139697269506048, flags=<optimized out>, flags@entry=597, regs=regs@entry=0xffffc90043187f58) at mm/memory.c:5219
#11 0xffffffff8110d937 in do_user_addr_fault (regs=regs@entry=0xffffc90043187f58, error_code=error_code@entry=6, address=address@entry=139697269506048) at arch/x86/mm/fault.c:1428
#12 0xffffffff8218b6d6 in handle_page_fault (address=139697269506048, error_code=6, regs=0xffffc90043187f58) at arch/x86/mm/fault.c:1519
#13 exc_page_fault (regs=0xffffc90043187f58, error_code=6) at arch/x86/mm/fault.c:1575
#14 0xffffffff82201286 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```
是的，但是纯粹就是巧合而已，当超过 100M 之后，恰好使用 transparent hugepage 了，实际上。

如果将 wbt 关掉，那么我们就可以看到 memory 总是可以下写的。

### 3 如果只是单纯的写入过慢，应该存在不一样的效果

1. 难道说，为什么 a.out 的代码段成为读取的内容了 ?
  - 这只是随机现象中的一种而已

### 将几个 printk 去掉之后，几乎立刻是 OOM 的

这个才是正常的，在
- try_charge_memcg
  - try_to_free_mem_cgroup_pages 返回值是 0，因为无法 reclaim 任何 page
  - mem_cgroup_oom : 出现 oom ，然后进程被 kill 掉
```txt
[  180.874870] CPU: 0 PID: 1791 Comm: a.out Not tainted 6.2.0-rc1-dirty #95
[  180.875115] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[  180.875361] Call Trace:
[  180.875455]  <TASK>
[  180.875538]  dump_stack_lvl+0x38/0x4c
[  180.875678]  dump_header+0x45/0x200
[  180.875811]  oom_kill_process.cold+0xb/0x10
[  180.875971]  out_of_memory+0x1a5/0x4f0
[  180.876113]  mem_cgroup_out_of_memory+0x131/0x150
[  180.876291]  try_charge_memcg+0x744/0x820
[  180.876442]  charge_memcg+0x2d/0xa0
[  180.876574]  __mem_cgroup_charge+0x28/0x80
[  180.876727]  __filemap_add_folio+0x355/0x430
[  180.876887]  ? __pfx_workingset_update_node+0x10/0x10
[  180.877075]  filemap_add_folio+0x36/0xa0
[  180.877222]  __filemap_get_folio+0x1fc/0x330
[  180.877385]  filemap_fault+0x150/0xa00
[  180.877527]  ? filemap_map_pages+0x12f/0x620
[  180.877687]  __do_fault+0x2c/0xb0
[  180.877813]  do_fault+0x1e1/0x590
[  180.877940]  __handle_mm_fault+0x5fa/0x12a0
[  180.878096]  handle_mm_fault+0xe4/0x2c0
[  180.878240]  do_user_addr_fault+0x1c7/0x670
[  180.878398]  ? kvm_read_and_reset_apf_flags+0x49/0x60
[  180.878585]  exc_page_fault+0x66/0x150
[  180.878725]  asm_exc_page_fault+0x26/0x30
[  180.878876] RIP: 0033:0x4012cc
[  180.878995] Code: Unable to access opcode bytes at 0x4012a2.
[  180.879203] RSP: 002b:00007ffd8431e4b0 EFLAGS: 00010206
[  180.879398] RAX: 00007fa6273e2000 RBX: 0000000000000000 RCX: 0000000000000000
[  180.879659] RDX: 0000000006422000 RSI: 0000000000b57910 RDI: 00007ffd8431df50
[  180.879919] RBP: 00007ffd8431e510 R08: 0000000000000000 R09: 0000000000000064
[  180.880180] R10: 0000000000000000 R11: 0000000000000246 R12: 00007ffd8431e628
[  180.880442] R13: 0000000000401192 R14: 0000000000403e08 R15: 00007fa633e09000
[  180.880703]  </TASK>
```

## 实际上 wbt 只是让时间推迟了而已，最后，会按照完全相同的剧本死掉
