# GFP Flags

基本思想，分配内存失败，还可以做的操作
- reclaim
  - 安装下方的地方
    - 将 clean page 写下去，这个可以到文件系统中的
    - 将 anonymous page 写入到 swap 中，swap 是一个 disk
  - 触发 reclam 的方式
    - direct reclaim : 会 stsall
    - kswapd : 不会 stall ，对于后续的分配有好处
- compaction


## 逐个分析
1. Physical address zone modifiers : 经典例子 GFP_DMA
2. Page mobility and placement hints
  1. 注意 __GFP_MOVABLE 的含义不仅仅就可以 move ，还有 "or can be reclaimed"
  2. __GFP_WRITE : 只有在 `__filemap_get_folio` 的地方使用过 @todo
  3. __GFP_HARDWALL : enforces the cpuset memory allocation policy
  4. __GFP_ACCOUNT : kmemcg 几乎不用了，其实意义不大
3. Watermark modifiers -- controls access to emergency reserves
4. Reclaim modifiers
  - __GFP_IO : 意外着需要等待
  - __GFP_FS : fs 独享的 moment
  - `__GFP_NORETRY` `__GFP_NORETRY` `__GFP_NOFAIL` : 重试的三种态度
5. Action modifiers

通过以上，然后有 Useful GFP flag combinations

## 检查下具体效果

### __GFP_NOWARN

就是 warn_alloc 中是否打印。只有分配失败的时候，才会打印 warn_alloc

### __GFP_HIGH

似乎起作用的地方就是: try_charge_memcg 中的
```c
nomem:
	/*
	 * Memcg doesn't have a dedicated reserve for atomic
	 * allocations. But like the global atomic pool, we need to
	 * put the burden of reclaim on regular allocation requests
	 * and let these go through as privileged allocations.
	 */
	if (!(gfp_mask & (__GFP_NOFAIL | __GFP_HIGH)))
		return -ENOMEM;
```

filemap_fault 的实现中
filemap_get_folios 应该是 GFP_USER 才对啊

## 参考
- https://lwn.net/Kernel/Index/#Memory_management-GFP_flags
- [GFP flags and the end of`__GFP_ATOMIC`](https://lwn.net/Articles/920891/)

这个不是还有吗?
```txt
#define GFP_ATOMIC	(__GFP_HIGH|__GFP_KSWAPD_RECLAIM)
```
如何理解 GFP_ATOMIC 的组成? 为什么是


### NOFS
- [ ] https://lwn.net/Articles/596618/
- https://unix.stackexchange.com/questions/448414/allocation-mode-explanation

GFP_NOFS 使用的位置大多数都是具体的 fs 中，但是在两个 generic 的位置:
1. folio_alloc_buffers
2. `__mpage_writepage` : 分配 bio 结构体

带上这个 flags 表示我们在文件系统中申请内存 :

例如 folio_alloc_buffers 的位置，

- read_mapping_folio
  - read_cache_folio
    - do_read_cache_folio
      - filemap_read_folio
        - block_read_full_folio
          - folio_create_buffers
            - folio_create_empty_buffers
              - folio_alloc_buffers

- io_wq_worker : 调用写的时候，还是需要分配内存的:
  - io_worker_handle_work
    - io_wq_submit_work
      - io_issue_sqe
        - io_write
          - call_write_iter
            - ext4_buffered_write_iter
              - generic_perform_write
                - ext4_da_write_begin
                  - __block_write_begin_int
                    - folio_create_buffers
                      - folio_create_empty_buffers
                        - folio_alloc_buffers

```c
/**
 * memalloc_nofs_save - Marks implicit GFP_NOFS allocation scope.
 *
 * This functions marks the beginning of the GFP_NOFS allocation scope.
 * All further allocations will implicitly drop __GFP_FS flag and so
 * they are safe for the FS critical section from the allocation recursion
 * point of view. Use memalloc_nofs_restore to end the scope with flags
 * returned by this function.
 *
 * This function is safe to be used from any context.
 */
static inline unsigned int memalloc_nofs_save(void)
{
	unsigned int flags = current->flags & PF_MEMALLOC_NOFS;
	current->flags |= PF_MEMALLOC_NOFS;
	return flags;
}
```

### 体现的位置

```c
/*
 * Applies per-task gfp context to the given allocation flags.
 * PF_MEMALLOC_NOIO implies GFP_NOIO
 * PF_MEMALLOC_NOFS implies GFP_NOFS
 * PF_MEMALLOC_PIN  implies !GFP_MOVABLE
 */
static inline gfp_t current_gfp_context(gfp_t flags)
{
	unsigned int pflags = READ_ONCE(current->flags);

	if (unlikely(pflags & (PF_MEMALLOC_NOIO | PF_MEMALLOC_NOFS | PF_MEMALLOC_PIN))) {
		/*
		 * NOIO implies both NOIO and NOFS and it is a weaker context
		 * so always make sure it makes precedence
		 */
		if (pflags & PF_MEMALLOC_NOIO)
			flags &= ~(__GFP_IO | __GFP_FS);
		else if (pflags & PF_MEMALLOC_NOFS)
			flags &= ~__GFP_FS;

		if (pflags & PF_MEMALLOC_PIN)
			flags &= ~__GFP_MOVABLE;
	}
	return flags;
}
```

#### 如何体现出来不能 call down to fs interface ?

在 vmscan.c 中

```c
/*
 * This is the main entry point to direct page reclaim.
 *
 * If a full scan of the inactive list fails to free enough memory then we
 * are "out of memory" and something needs to be killed.
 *
 * If the caller is !__GFP_FS then the probability of a failure is reasonably
 * high - the zone may be full of dirty or under-writeback pages, which this
 * caller can't do much about.  We kick the writeback threads and take explicit
 * naps in the hope that some of these pages can be written.  But if the
 * allocating task holds filesystem locks which prevent writeout this might not
 * work, and the allocation attempt will fail.
 *
 * returns:	0, if no pages reclaimed
 * 		else, the number of pages reclaimed
 */
static unsigned long do_try_to_free_pages(struct zonelist *zonelist,
					  struct scan_control *sc)
{
```

应该是体现在此处的 : `shrink_folio_list`

```c
			if (!may_enter_fs(folio, sc->gfp_mask))
				goto keep_locked;
```

#### dead lock 体现在什么位置?


- `__perform_reclaim`
  - fs_reclaim_acquire
  - try_to_free_pages
    - do_try_to_free_pages
  - fs_reclaim_release


## 如何理解 ATOMIC


```c
/*
 * %GFP_ATOMIC users can not sleep and need the allocation to succeed. A lower
 * watermark is applied to allow access to "atomic reserves".
 * The current implementation doesn't support NMI and few other strict
 * non-preemptive contexts (e.g. raw_spin_lock). The same applies to %GFP_NOWAIT.
 */
```

```c
#define GFP_ATOMIC	(__GFP_HIGH|__GFP_KSWAPD_RECLAIM)

#define GFP_NOWAIT	(__GFP_KSWAPD_RECLAIM)
```

为什么单走一个 `__GFP_KSWAPD_RECLAIM` 就是 NOWAIT 的呀


## [ ] 深入理解 gfp.h

- current_gfp_context : 会根据 current 来修改分配的规则

```c
#define FGP_ACCESSED		0x00000001
#define FGP_LOCK		0x00000002
#define FGP_CREAT		0x00000004
#define FGP_WRITE		0x00000008
#define FGP_NOFS		0x00000010
#define FGP_NOWAIT		0x00000020
#define FGP_FOR_MMAP		0x00000040
#define FGP_HEAD		0x00000080
#define FGP_ENTRY		0x00000100
#define FGP_STABLE		0x00000200
```

- [ ] 按照道理来说，find_get_page 是去获取一个 page cache 的，page cache 总是 MOVABLE 的，但是传递给 pagecache_get_page 的参数是 0
```c
/**
 * find_get_page - find and get a page reference
 * @mapping: the address_space to search
 * @offset: the page index
 *
 * Looks up the page cache slot at @mapping & @offset.  If there is a
 * page cache page, it is returned with an increased refcount.
 *
 * Otherwise, %NULL is returned.
 */
static inline struct page *find_get_page(struct address_space *mapping,
					pgoff_t offset)
{
	return pagecache_get_page(mapping, offset, 0, 0);
}
```

## GFP_USER vs GFP_HIGHUSER
highmem : 因为 32 bit 的空间，内核地址虚拟地址空间只有 1G，

就是 highmem 导致的区别，如果是 GFP_USER 的，内存是直接映射的区域的，

其使用没有位置没有逐个比较分析。

## 具体的实现
__GFP_MEMALLOC

在 `__gfp_pfmemalloc_flags` 中使用，加上之后表示可以无视 ALLOC_NO_WATERMARKS


## 具体案例

### balloon
```c
struct page *balloon_page_alloc(void)
{
	struct page *page = alloc_page(balloon_mapping_gfp_mask() |
				       __GFP_NOMEMALLOC | __GFP_NORETRY |
				       __GFP_NOWARN);
	return page;
}
```

## 分析一个连续页的分配问题
```txt
[Mon Jun  3 19:59:56 2024] dockerd: page allocation failure: order:6, mode:0x40dc0(GFP_KERNEL|__GFP_COMP|__GFP_ZERO), nodemask=(null),cpuset=docker.service,mems_allowed=0
[Mon Jun  3 19:59:56 2024] Call Trace:
[Mon Jun  3 19:59:56 2024]  <TASK>
[Mon Jun  3 19:59:56 2024]  show_stack+0x52/0x5c
[Mon Jun  3 19:59:56 2024]  dump_stack_lvl+0x4a/0x63
[Mon Jun  3 19:59:56 2024]  dump_stack+0x10/0x16
[Mon Jun  3 19:59:56 2024]  warn_alloc+0x138/0x160
[Mon Jun  3 19:59:56 2024]  __alloc_pages_slowpath.constprop.0+0xa44/0xa80
[Mon Jun  3 19:59:56 2024]  __alloc_pages+0x311/0x330
[Mon Jun  3 19:59:56 2024]  alloc_pages+0x9e/0x1e0
[Mon Jun  3 19:59:56 2024]  kmalloc_order+0x2f/0xd0
[Mon Jun  3 19:59:56 2024]  kmalloc_order_trace+0x1d/0x90
[Mon Jun  3 19:59:56 2024]  __kmalloc+0x2b1/0x330
[Mon Jun  3 19:59:56 2024]  veth_alloc_queues+0x25/0x80 [veth]
[Mon Jun  3 19:59:56 2024]  veth_dev_init+0x72/0xd0 [veth]
[Mon Jun  3 19:59:56 2024]  register_netdevice+0x11c/0x650
[Mon Jun  3 19:59:56 2024]  veth_newlink+0x1ba/0x440 [veth]
[Mon Jun  3 19:59:56 2024]  __rtnl_newlink+0x77f/0xa50
[Mon Jun  3 19:59:56 2024]  ? __ext4_journal_get_write_access+0x8f/0x1b0
[Mon Jun  3 19:59:56 2024]  ? __cond_resched+0x1a/0x50
[Mon Jun  3 19:59:56 2024]  ? __getblk_gfp+0x2f/0xf0
[Mon Jun  3 19:59:56 2024]  ? ext4_get_group_desc+0x68/0x100
[Mon Jun  3 19:59:56 2024]  ? __nla_validate_parse+0x12f/0x1b0
[Mon Jun  3 19:59:56 2024]  ? netdev_name_node_lookup+0x36/0x80
[Mon Jun  3 19:59:56 2024]  ? __dev_get_by_name+0xe/0x20
[Mon Jun  3 19:59:56 2024]  rtnl_newlink+0x49/0x70
[Mon Jun  3 19:59:56 2024]  rtnetlink_rcv_msg+0x15d/0x400
[Mon Jun  3 19:59:56 2024]  ? rtnl_calcit.isra.0+0x130/0x130
[Mon Jun  3 19:59:56 2024]  netlink_rcv_skb+0x56/0x100
[Mon Jun  3 19:59:56 2024]  rtnetlink_rcv+0x15/0x20
[Mon Jun  3 19:59:56 2024]  netlink_unicast+0x223/0x340
[Mon Jun  3 19:59:56 2024]  netlink_sendmsg+0x24b/0x4c0
[Mon Jun  3 19:59:56 2024]  __sock_sendmsg+0x69/0x70
[Mon Jun  3 19:59:56 2024]  __sys_sendto+0x113/0x190
[Mon Jun  3 19:59:56 2024]  ? __sys_getsockname+0xce/0xe0
[Mon Jun  3 19:59:56 2024]  ? __x64_sys_getrandom+0x5e/0xb0
[Mon Jun  3 19:59:56 2024]  __x64_sys_sendto+0x24/0x30
[Mon Jun  3 19:59:56 2024]  x64_sys_call+0x1bcb/0x1fa0
[Mon Jun  3 19:59:56 2024]  do_syscall_64+0x56/0xb0
[Mon Jun  3 19:59:56 2024]  ? x64_sys_call+0x8a1/0x1fa0
[Mon Jun  3 19:59:56 2024]  ? do_syscall_64+0x63/0xb0
[Mon Jun  3 19:59:56 2024]  ? do_syscall_64+0x63/0xb0
[Mon Jun  3 19:59:56 2024]  ? exit_to_user_mode_prepare+0x37/0xb0
[Mon Jun  3 19:59:56 2024]  ? exit_to_user_mode_prepare+0x37/0xb0
[Mon Jun  3 19:59:56 2024]  ? syscall_exit_to_user_mode+0x35/0x50
[Mon Jun  3 19:59:56 2024]  ? x64_sys_call+0x926/0x1fa0
[Mon Jun  3 19:59:56 2024]  ? do_syscall_64+0x63/0xb0
[Mon Jun  3 19:59:56 2024]  entry_SYSCALL_64_after_hwframe+0x67/0xd1
[Mon Jun  3 19:59:56 2024] RIP: 0033:0x560b101baf4a
[Mon Jun  3 19:59:56 2024] Code: e8 9b 96 fa ff 48 8b 7c 24 10 48 8b 74 24 18 48 8b 54 24 20 4c 8b 54 24 28 4c 8b 44 24 30 4c 8b 4c 24 38 48 8b 44 24 08 0f 05 <48> 3d 01 f0 ff ff 76 20 48 c7 44 24 40 ff ff ff ff 48 c7 44 24 48
[Mon Jun  3 19:59:56 2024] RSP: 002b:000000c001945820 EFLAGS: 00000206 ORIG_RAX: 000000000000002c
[Mon Jun  3 19:59:56 2024] RAX: ffffffffffffffda RBX: 000000c000067400 RCX: 0000560b101baf4a
[Mon Jun  3 19:59:56 2024] RDX: 0000000000000074 RSI: 000000c0018c8800 RDI: 000000000000000e
[Mon Jun  3 19:59:56 2024] RBP: 000000c001945888 R08: 000000c000fc4b20 R09: 000000000000000c
[Mon Jun  3 19:59:56 2024] R10: 0000000000000000 R11: 0000000000000206 R12: 0000000000000000
[Mon Jun  3 19:59:56 2024] R13: 0000000000000000 R14: 000000c000fc0680 R15: ffffffffffffffff
[Mon Jun  3 19:59:56 2024]  </TASK>
[Mon Jun  3 19:59:56 2024] Mem-Info:
[Mon Jun  3 19:59:56 2024] active_anon:26033 inactive_anon:61950 isolated_anon:0
                            active_file:1619011 inactive_file:1926946 isolated_file:0
                            unevictable:7289 dirty:4051 writeback:0
                            slab_reclaimable:222031 slab_unreclaimable:64095
                            mapped:49739 shmem:549 pagetables:2098 bounce:0
                            kernel_misc_reclaimable:0
                            free:55278 free_pcp:143 free_cma:0
[Mon Jun  3 19:59:56 2024] Node 0 active_anon:104132kB inactive_anon:247800kB active_file:6476044kB inactive_file:7707784kB unevictable:29156kB isolated(anon):0kB isolated(file):0kB mapped:198956kB dirty:16204kB writeback:0kB shmem:2196kB shmem_thp: 0kB shmem_pmdmapped: 0kB anon_thp: 0kB writeback_tmp:0kB kernel_stack:14240kB pagetables:8392kB all_unreclaimable? no
[Mon Jun  3 19:59:56 2024] Node 0 DMA free:13312kB min:64kB low:80kB high:96kB reserved_highatomic:0KB active_anon:0kB inactive_anon:0kB active_file:0kB inactive_file:0kB unevictable:0kB writepending:0kB present:15992kB managed:15360kB mlocked:0kB bounce:0kB free_pcp:0kB local_pcp:0kB free_cma:0kB
[Mon Jun  3 19:59:56 2024] lowmem_reserve[]: 0 2831 15794 15794 15794
[Mon Jun  3 19:59:56 2024] Node 0 DMA32 free:86104kB min:14148kB low:17172kB high:20196kB reserved_highatomic:2048KB active_anon:14204kB inactive_anon:61736kB active_file:307096kB inactive_file:2283644kB unevictable:0kB writepending:1492kB present:3129180kB managed:3015048kB mlocked:0kB bounce:0kB free_pcp:384kB local_pcp:0kB free_cma:0kB
[Mon Jun  3 19:59:56 2024] lowmem_reserve[]: 0 0 12963 12963 12963
[Mon Jun  3 19:59:56 2024] Node 0 Normal free:121696kB min:55416kB low:69268kB high:83120kB reserved_highatomic:12288KB active_anon:89928kB inactive_anon:185740kB active_file:6169056kB inactive_file:5424092kB unevictable:29156kB writepending:14864kB present:13631488kB managed:13274116kB mlocked:27620kB bounce:0kB free_pcp:188kB local_pcp:0kB free_cma:0kB
[Mon Jun  3 19:59:56 2024] lowmem_reserve[]: 0 0 0 0 0
[Mon Jun  3 19:59:56 2024] Node 0 DMA: 0*4kB 0*8kB 0*16kB 0*32kB 0*64kB 0*128kB 0*256kB 0*512kB 1*1024kB (U) 2*2048kB (UM) 2*4096kB (M) = 13312kB
[Mon Jun  3 19:59:56 2024] Node 0 DMA32: 5179*4kB (UMEH) 2455*8kB (UMEH) 792*16kB (UMEH) 750*32kB (UMEH) 124*64kB (UMEH) 7*128kB (MH) 3*256kB (H) 0*512kB 0*1024kB 0*2048kB 0*4096kB = 86628kB
[Mon Jun  3 19:59:56 2024] Node 0 Normal: 8088*4kB (UMEH) 4920*8kB (UMEH) 1847*16kB (UMEH) 642*32kB (UMEH) 24*64kB (UME) 2*128kB (M) 0*256kB 0*512kB 0*1024kB 0*2048kB 0*4096kB = 123600kB
[Mon Jun  3 19:59:56 2024] Node 0 hugepages_total=0 hugepages_free=0 hugepages_surp=0 hugepages_size=2048kB
[Mon Jun  3 19:59:56 2024] 3549212 total pagecache pages
[Mon Jun  3 19:59:56 2024] 238 pages in swap cache
[Mon Jun  3 19:59:56 2024] Swap cache stats: add 392280, delete 392082, find 2066103/2264565
[Mon Jun  3 19:59:56 2024] Free swap  = 11997376kB
[Mon Jun  3 19:59:56 2024] Total swap = 12103872kB
[Mon Jun  3 19:59:56 2024] 4194165 pages RAM
[Mon Jun  3 19:59:56 2024] 0 pages HighMem/MovableOnly
[Mon Jun  3 19:59:56 2024] 118034 pages reserved
[Mon Jun  3 19:59:56 2024] 0 pages hwpoisoned
```

```txt
$ free -m
               total        used        free      shared  buff/cache   available
Mem:           15922         980         723           2       14218       14601
Swap:          11820         106       11714
```

```txt
$ cat /proc/buddyinfo
Node 0, zone      DMA      0      0      0      0      0      0      0      0      1      2      2
Node 0, zone    DMA32    605   1844    650    523    165     32    332    224     98     67     30
Node 0, zone   Normal   3070   4636   2026    110     34      2      0      0      0      0      0
```
的确，

drop 完成 page cache 之后，所以，很奇怪，即便是存在足够多的内存，也会分配连续的页失败.
```txt
$ cat /proc/buddyinfo
Node 0, zone      DMA      0      0      0      0      0      0      0      0      1      2      2
Node 0, zone    DMA32   9682   8464   6867   5570   4013   2675   1585    885    378    160     83
Node 0, zone   Normal  64788  83938  70164  51761  29937  12857   3897   1215    641    416    394
```

具体位置在
```c
static int veth_alloc_queues(struct net_device *dev)
{
	struct veth_priv *priv = netdev_priv(dev);
	int i;

	priv->rq = kvcalloc(dev->num_rx_queues, sizeof(*priv->rq),
			    GFP_KERNEL_ACCOUNT | __GFP_RETRY_MAYFAIL);
	if (!priv->rq)
		return -ENOMEM;

	for (i = 0; i < dev->num_rx_queues; i++) {
		priv->rq[i].dev = dev;
		u64_stats_init(&priv->rq[i].stats.syncp);
	}

	return 0;
}
```

他喵的，这么快就修复了:
```diff
History:        #0
Commit:         1ce7d306ea63f3e379557c79abd88052e0483813
Author:         Jakub Kicinski <kuba@kernel.org>
Committer:      Paolo Abeni <pabeni@redhat.com>
Author Date:    Sat 24 Feb 2024 07:59:08 AM CST
Committer Date: Tue 27 Feb 2024 08:56:54 PM CST

veth: try harder when allocating queue memory

struct veth_rq is pretty large, 832B total without debug
options enabled. Since commit under Fixes we try to pre-allocate
enough queues for every possible CPU. Miao Wang reports that
this may lead to order-5 allocations which will fail in production.

Let the allocation fallback to vmalloc() and try harder.
These are the same flags we pass to netdev queue allocation.

Reported-and-tested-by: Miao Wang <shankerwangmiao@gmail.com>
Fixes: 9d3684c24a52 ("veth: create by default nr_possible_cpus queues")
Link: https://lore.kernel.org/all/5F52CAE2-2FB7-4712-95F1-3312FBBFA8DD@gmail.com/
Signed-off-by: Jakub Kicinski <kuba@kernel.org>
Reviewed-by: Eric Dumazet <edumazet@google.com>
Link: https://lore.kernel.org/r/20240223235908.693010-1-kuba@kernel.org
Signed-off-by: Paolo Abeni <pabeni@redhat.com>

diff --git a/drivers/net/veth.c b/drivers/net/veth.c
index a786be805709..cd4a6fe458f9 100644
--- a/drivers/net/veth.c
+++ b/drivers/net/veth.c
@@ -1461,7 +1461,8 @@ static int veth_alloc_queues(struct net_device *dev)
 	struct veth_priv *priv = netdev_priv(dev);
 	int i;

-	priv->rq = kcalloc(dev->num_rx_queues, sizeof(*priv->rq), GFP_KERNEL_ACCOUNT);
+	priv->rq = kvcalloc(dev->num_rx_queues, sizeof(*priv->rq),
+			    GFP_KERNEL_ACCOUNT | __GFP_RETRY_MAYFAIL);
 	if (!priv->rq)
 		return -ENOMEM;

@@ -1477,7 +1478,7 @@ static void veth_free_queues(struct net_device *dev)
 {
 	struct veth_priv *priv = netdev_priv(dev);

-	kfree(priv->rq);
+	kvfree(priv->rq);
 }

 static int veth_dev_init(struct net_device *dev)
```

其实还是非常奇怪，当时系统中有很多 page cache ，但是没有触发 reclaim ，
也没有触发 compaction 。

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
