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
- kthread
  - kswapd
    - balance_pgdat
      - kswapd_shrink_node
        - shrink_node
          - shrink_node_memcgs
            - shrink_lruvec
              - shrink_list
                - shrink_inactive_list
                  - shrink_folio_list
                    - add_to_swap
```

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
| Name        | description                                       |
|-------------|---------------------------------------------------|
| swapfile    |                                                   |
| swap_state  | 维护 swap cache，swap 的 readahead                |
| swap        | pagevec 和 lrulist 的操作，其实和 swap 的关系不大 |
| swap_slot   |                                                   |
| page_io     | 进行通往底层的 io                                 |
| mlock       |                                                   |
| workingset  |                                                   |
| frontswap   |                                                   |
| zswap       |                                                   |
| swap_cgroup |                                                   |

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

## [Reconsidering swapping](https://lwn.net/Articles/690079/)
> As a general rule, reclaiming anonymous pages (swapping) is seen as being considerably more expensive than reclaiming file-backed pages.
One of the key reasons for this difference is that file-backed pages can be read from (and written to) persistent storage in large, contiguous chunks,
while anonymous pages tend to be scattered randomly on the swap device.

确实，因为内存的访问是随机的，进而导致 swap 访问很容易随机。

## 如何理解这里的代码
kernel/power/swap.c

是用于支持 hibernate 的吧

## 现在 swap 也是支持 polling mode 啊!
```diff
History:        #0
Commit:         b2d1f38b524121130befa3a9b37dca790cfa9ab9
Author:         Yosry Ahmed <yosryahmed@google.com>
Committer:      Andrew Morton <akpm@linux-foundation.org>
Author Date:    Fri 07 Jun 2024 12:55:15 PM CST
Committer Date: Thu 04 Jul 2024 10:30:06 AM CST

mm: swap: remove 'synchronous' argument to swap_read_folio()

Commit [1] introduced IO polling support duding swapin to reduce swap read
latency for block devices that can be polled.  However later commit [2]
removed polling support.  Commit [3] removed the remnants of polling
support from read_swap_cache_async() and __read_swap_cache_async().
However, it left behind some remnants in swap_read_folio(), the
'synchronous' argument.

swap_read_folio() reads the folio synchronously if synchronous=true or if
SWP_SYNCHRONOUS_IO is set in swap_info_struct.  The only caller that
passes synchronous=true is in do_swap_page() in the SWP_SYNCHRONOUS_IO
case.

Hence, the argument is redundant, it is only set to true when the swap
read would have been synchronous anyway. Remove it.

[1] Commit 23955622ff8d ("swap: add block io poll in swapin path")
[2] Commit 9650b453a3d4 ("block: ignore RWF_HIPRI hint for sync dio")
[3] Commit b243dcbf2f13 ("swap: remove remnants of polling from read_swap_cache_async")

Link: https://lkml.kernel.org/r/20240607045515.1836558-1-yosryahmed@google.com
Signed-off-by: Yosry Ahmed <yosryahmed@google.com>
Reviewed-by: "Huang, Ying" <ying.huang@intel.com>
Reviewed-by: Christoph Hellwig <hch@lst.de>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
```

现在已经改造为 : swap_read_folio_bdev_sync


#### (mem) migration 对于 swap 的影响是什么 ?

#### (swap) 虽然 page cache 实现使用内存缓存 disk, 但是这些 page 什么时候被放到 disk 和 swap　分区中间，需要 swap 机制完成，所以是不是说，swap 机制发

#### (swap) swap fs 的结构是什么样子，为了 efficient 和 功能，如何调整以及和一般的 fs 相区分的

#### (swap) swap partition 和 swap file 是什么关系 ?

#### (swap) swap_info 为什么需要建立成为一个数组，为了实现类似于 per cpu 的效果，实现对于 disk 的并发访问


#### (swap) swap cache 和 page cache 有什么相似的点吗?

#### (mem) 在分析 swap 中间的时候，以前表示 page 的属性采用 active 和 inactive ，但是如今进一步被划分成为 inactive file , inactive anon
1. swap 机制为什么要区分 file 和 anon 的内容，真的是因为 file 是被刷新到 ext4 而不是　swap fs 中间的吗 ?
2. page cache 是如何划分 page cache 的　？


## 关键问题

mm/swap_slots.c 维护了什么，是重复的吗?

## 为什么 do_swap_page 前面还有那么特殊情况?

也就是 下的判断
```c
if (unlikely(non_swap_entry(entry))) {
```

都是分别描述什么东西的?

## swapcache 是如何被释放的?

是放到哪里一起被释放的吗？


## 本周的 bonus
__swap_duplicate
SWAP_HAS_CACHE

- page_vma_mapped_walk : 检测的地方在这里
    check_pte  ===> 这里会通过 page 和 pte 算出来pfn ，看两个相等不

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
