# swap.c 分析

1. lurvec 和 pagevec 各自的作用: lrulist 封装和 batch 操作封装
2. 本文件处理的内容和 swap 没有什么蛇皮关系，虽然主要的内容是 pagevec 的各种操作，但是实际上是向各种 lrulist 中间添加。

```txt
#0  lru_add_drain_cpu (cpu=3) at mm/swap.c:665
#1  0xffffffff812a7d2b in lru_add_drain () at mm/swap.c:773
#2  0xffffffff812a7d84 in __pagevec_release (pvec=pvec@entry=0xffffc9000005fb88) at mm/swap.c:1072
#3  0xffffffff812a8ab1 in pagevec_release (pvec=0xffffc9000005fb88) at include/linux/pagevec.h:71
#4  folio_batch_release (fbatch=0xffffc9000005fb88) at include/linux/pagevec.h:135
```

- 一个 node 至少持有一个 lruvec
- 但是一个 node 也是持有一个 lruvec 的
  - 如果一个页，被 memcg 使用了，那么应该放到那个 lruvec 中
- 分析 folio_lruvec，实际上，一个 memcg 在每一个 node 上都有 lruvec

## 调用环节 : 莫名奇妙的
> vmscan.c 整个维持 swap 页面的替换回去，但是 page cache 的刷新回去的操作谁来控制 ?
> page cache 和 swap cache 是不是采用相同的模型进行的 ? 如果说，其中，将 anon memory 当做 swap 形成的 file based 那么岂不是很好。

```txt
#0  add_to_swap (folio=folio@entry=0xffffea0002ef9e00) at mm/swap_state.c:182
#1  0xffffffff812ade01 in shrink_folio_list (folio_list=folio_list@entry=0xffffc900012bbc30, pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc900012bbdd8, stat=stat@entry=0xffffc900012bbcb8, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1834
#2  0xffffffff812af5d8 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc900012bbdd8, lruvec=0xffff888164d5c000, nr_to_scan=<optimized out>) at mm/vmscan.c:2489
#3  shrink_list (sc=0xffffc900012bbdd8, lruvec=0xffff888164d5c000, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2716
#4  shrink_lruvec (lruvec=lruvec@entry=0xffff888164d5c000, sc=sc@entry=0xffffc900012bbdd8) at mm/vmscan.c:5885
#5  0xffffffff812afe3f in shrink_node_memcgs (sc=0xffffc900012bbdd8, pgdat=0xffff88823fff9000) at mm/vmscan.c:6074
#6  shrink_node (pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc900012bbdd8) at mm/vmscan.c:6105
#7  0xffffffff812b0577 in kswapd_shrink_node (sc=0xffffc900012bbdd8, pgdat=0xffff88823fff9000) at mm/vmscan.c:6894
#8  balance_pgdat (pgdat=pgdat@entry=0xffff88823fff9000, order=order@entry=9, highest_zoneidx=highest_zoneidx@entry=3) at mm/vmscan.c:7084
#9  0xffffffff812b0b2b in kswapd (p=0xffff88823fff9000) at mm/vmscan.c:7344
#10 0xffffffff81133853 in kthread (_create=0xffff888004058240) at kernel/kthread.c:376
```

## folio_add_lru

提供给外部的接口:
- folio_add_lru : 将 page 添加到 folio_batch 中
- folio_activate : 将 page 从 inactive 移动到 active 中
- deactivate_page
- mark_page_lazyfree : 缓存匿名页，清除掉 PG_activate, PG_referenced, PG_swapbacked 标志后，将这些页加入到 LRU_INACTIVE_FILE 链表中
- lru_add_drain_cpu : 将 folio_batch 中的 page 移动到 lru 中

folio_batch_move_lru : 用于遍历 folio_batch

用于搬运一个 page 函数，都是 move_fn_t 类型的:
- lru_add_fn
- lru_lazyfree_fn
- lru_move_tail_fn
- folio_activate_fn
- lru_deactivate_fn
- lru_deactivate_file_fn

```txt
#0  lru_add_drain_cpu (cpu=1) at mm/swap.c:665
#1  0xffffffff812a7f4b in lru_add_drain () at mm/swap.c:773
#2  0xffffffff812af51d in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc90000127ba8, lruvec=0xffff888164d5c000, nr_to_scan=32) at mm/vmscan.c:2470
#3  shrink_list (sc=0xffffc90000127ba8, lruvec=0xffff888164d5c000, nr_to_scan=32, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2716
#4  shrink_lruvec (lruvec=lruvec@entry=0xffff888164d5c000, sc=sc@entry=0xffffc90000127ba8) at mm/vmscan.c:5885
#5  0xffffffff812afe3f in shrink_node_memcgs (sc=0xffffc90000127ba8, pgdat=0xffff88823fff9000) at mm/vmscan.c:6074
#6  shrink_node (pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc90000127ba8) at mm/vmscan.c:6105
#7  0xffffffff812b1030 in shrink_zones (sc=0xffffc90000127ba8, zonelist=<optimized out>) at mm/vmscan.c:6343
#8  do_try_to_free_pages (zonelist=zonelist@entry=0xffff88823fffab00, sc=sc@entry=0xffffc90000127ba8) at mm/vmscan.c:6405
#9  0xffffffff812b1aaa in try_to_free_pages (zonelist=0xffff88823fffab00, order=order@entry=0, gfp_mask=gfp_mask@entry=1314250, nodemask=<optimized out>) at mm/vmscan.c:6640
#10 0xffffffff812ffb89 in __perform_reclaim (ac=0xffffc90000127d28, order=0, gfp_mask=1314250) at mm/page_alloc.c:4755
#11 __alloc_pages_direct_reclaim (did_some_progress=<synthetic pointer>, ac=0xffffc90000127d28, alloc_flags=2240, order=0, gfp_mask=1314250) at mm/page_alloc.c:4777
#12 __alloc_pages_slowpath (gfp_mask=<optimized out>, gfp_mask@entry=1314250, order=order@entry=0, ac=ac@entry=0xffffc90000127d28) at mm/page_alloc.c:5183
#13 0xffffffff81300718 in __alloc_pages (gfp=gfp@entry=1314250, order=order@entry=0, preferred_nid=<optimized out>, nodemask=0x0 <fixed_percpu_data>) at mm/page_alloc.c:5568
#14 0xffffffff81301022 in __folio_alloc (gfp=gfp@entry=1052106, order=order@entry=0, preferred_nid=<optimized out>, nodemask=<optimized out>) at mm/page_alloc.c:5587
```

对于 folio_add_lru 的一个经典的调用:
```txt
#0  filemap_add_folio (mapping=mapping@entry=0xffff8880369cb9c0, folio=folio@entry=0xffffea0008731dc0, index=index@entry=0, gfp=gfp@entry=1125578) at mm/filemap.c:929
#1  0xffffffff812a47af in page_cache_ra_unbounded (ractl=ractl@entry=0xffffc9000189fd18, nr_to_read=71, lookahead_size=<optimized out>) at mm/readahead.c:251
#2  0xffffffff812a4e27 in do_page_cache_ra (lookahead_size=<optimized out>, nr_to_read=<optimized out>, ractl=0xffffc9000189fd18) at mm/readahead.c:300
#3  0xffffffff8129a2ba in do_sync_mmap_readahead (vmf=0xffffc9000189fdf8) at mm/filemap.c:3043
#4  filemap_fault (vmf=0xffffc9000189fdf8) at mm/filemap.c:3135
#5  0xffffffff812d39ef in __do_fault (vmf=vmf@entry=0xffffc9000189fdf8) at mm/memory.c:4203
#6  0xffffffff812d7ef1 in do_read_fault (vmf=0xffffc9000189fdf8) at mm/memory.c:4554
#7  do_fault (vmf=vmf@entry=0xffffc9000189fdf8) at mm/memory.c:4683
#8  0xffffffff812dcad4 in handle_pte_fault (vmf=0xffffc9000189fdf8) at mm/memory.c:4955
#9  __handle_mm_fault (vma=vma@entry=0xffff8880626a5260, address=address@entry=94203785161552, flags=flags@entry=596) at mm/memory.c:5097
#10 0xffffffff812dd7b0 in handle_mm_fault (vma=0xffff8880626a5260, address=address@entry=94203785161552, flags=flags@entry=596, regs=regs@entry=0xffffc9000189ff58) at mm/memory.c:5218
#11 0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc9000189ff58, error_code=error_code@entry=4, address=address@entry=94203785161552) at arch/x86/mm/fault.c:1428
#12 0xffffffff81faf042 in handle_page_fault (address=94203785161552, error_code=4, regs=0xffffc9000189ff58) at arch/x86/mm/fault.c:1519
#13 exc_page_fault (regs=0xffffc9000189ff58, error_code=4) at arch/x86/mm/fault.c:1575
#14 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#15 0x000055ad88e4d310 in ?? ()
```

1. filemap.c : do_read_cache_page  pagecache_get_page generic_file_buffered_read
2. buffered-io.c : 看不懂
3. fs/buffer.c : `__find_get_block`
4. shmem.c : 没有分析


> @todo 同样的，此处分析的内容缺少 anon 的分析，除非 shmem.c 中间的是用于 /tmp 的，所以似乎没有用于 anon 的
> 除非，mark_page_accessed 表示这次，这个东西是和 fs 打过交道的
> 所以，对于 anon 处理的地方在于 swap cache 中间吗 ?


> @todo 非常的怀疑，page_referenced 的调用，只有当发现其实在上一次调用 page_referenced 到此次，根本没有任何
> 对于该 page frame 的映射发生过访问，所以，决定下调。
> 而 mark_page_accessed 出现的位置表示 : 该 page 刚刚读入进来，如果此时换出，非常的不应该。

lruvec 中间的类型只有 : anon 以及 file 和 unevictable，都是映射而已。
    1. 如果是靠这个怀疑 io page cache 不在 reclaim 的机制之下，那么 generic_file_buffered_read 中间也是调用过 mark_page_accessed 的
    2. page-writeback 的功能只是表示 dirty 数量不要超过某一个阈值，和到底存在多少个 page 在内存中间没有关系

## swap

3. 当进行 swap 机制开始回收的时候，一个物理页面需要被清楚掉，但是映射到该物理页面的 pte_t 的需要被重写为 swp_entry_t 的内容，由于可能共享，所以需要 rmap 实现找到这些 pte，
4. page reclaim 机制可能需要清理 swap cache 的内容
5. transparent hugetlb 的页面能否换出，如何换出 ?
1. 4. swap_slots 的工作原理是什么 ?

swap 机制主要组成部分是什么 :
    0. swap cache 来实现其中
    1. page 和 设备上的 io : page-io.c
    2. swp_entry_t 空间的分配 : swapfile.c
    3. policy :
        1. 确定那些页面需要被放到 swap 中间
        2. swap cache 的页面如何处理
    4. 特殊的 swap

在 mm/ 文件夹下涉及到 swap 的文件，和对于 swap 的作用:
| Name        | description                 |
|-------------|-----------------------------|
| swapfile    |                             |
| swap_state  | 维护 swap cache，swap 的 readahead                           |
| swap        | pagevec 和 lrulist 的操作，其实和 swap 的关系不大 |
| swap_slot   |                             |
| page_io     | 进行通往底层的 io                             |
| mlock       |                             |
| workingset  |                             |
| frontswap   |                             |
| zswap       |                             |
| swap_cgroup |                             |

struct page 的支持
1. `page->private` 用于存储 swp_entry_t.val，表示其中的
2. TODO 还有其他的内容吗


swap_slot.c 主要内容:
```c
static DEFINE_PER_CPU(struct swap_slots_cache, swp_slots);
struct swap_slots_cache {
  bool    lock_initialized;
  struct mutex  alloc_lock; /* protects slots, nr, cur */
  swp_entry_t *slots;
  int   nr;
  int   cur;
  spinlock_t  free_lock;  /* protects slots_ret, n_ret */
  swp_entry_t *slots_ret;
  int   n_ret;
};

// 两个对外提供的接口
int free_swap_slot(swp_entry_t entry);
swp_entry_t get_swap_page(struct page *page)
```
当 get_swap_page 将 cache 耗尽之后，会调用 swapfile::get_swap_pages 来维持生活
也就是 swap_slots.c 其实是 slots cache 机制。

2. 为什么 `page->private` 需要保存 `swp_entry_t`　的内容, 难道不是 page table entry 保存吗 ? (当其需要再次被写回的时候，依靠这个确定位置，和删除在 radix tree 的关系!)
