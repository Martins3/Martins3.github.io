# swap cache

swap_state.c 主要内容:
| Function name               | desc                                                                                    |
|-----------------------------|-----------------------------------------------------------------------------------------|
| `read_swap_cache_async`     |                                                                                         |
| `swap_cluster_readahead`    | @todo 为什么 readahead 不是利用 page cache 中间的公共框架，最终调用在 do_swap_page 中间 |
| `swap_vma_readahead`        | 另一个 readahead 策略，swapin_readahead 中间被决定                                      |
| `total_swapcache_pages`     | 返回所有的 swap 持有的 page frame                                                       |
| `show_swap_cache_info`      | 打印 swap_cache_info 以及 swapfile 中间的                                               |
| `add_to_swap_cache`         | 将 page 插入到 radix_tree 中间                                                          |
| `add_to_swap`               | 利用 `swap_slots.c` 获取 get_swap_page 获取空闲 swp_entry                               |
| `__delete_from_swap_cache`  | 对称操作                                                                                |
| `delete_from_swap_cache`    |                                                                                         |
| `free_swap_cache`           | 调用 swapfile.c try_to_free_swap @todo swapfile.c 的内容比想象的多得多啊 !              |
| `free_page_and_swap_cache`  |                                                                                         |
| `free_pages_and_swap_cache` |                                                                                         |
| `lookup_swap_cache`         | find_get_page 如果不考虑处理 readahead 机制的话                                         |
| `__read_swap_cache_async`   |                                                                                         |
| `swapin_nr_pages`           | readahead 函数的读取策略 @todo                                                          |
| `init_swap_address_space`   | swapon syscall 调用，初始化 swap                                                        |

1. /sys/kernel/mm/swap/vma_ra_enabled 来控制是否 readahead
2. 建立 radix_tree 的过程，多个文件，多个分区，各自大小而且不同 ? init_swap_address_space 中说明的，对于一个文件，每 64M 创建一个 radix_tree，至于其来自于那个文件还是分区，之后寻址的时候不重要了。init_swap_address_space 被 swapon 唯一调用
```c
struct address_space *swapper_spaces[MAX_SWAPFILES] __read_mostly;
static unsigned int nr_swapper_spaces[MAX_SWAPFILES] __read_mostly;
```
3. 谁会调用 add_to_swap 这一个东西 ?
    1. 认为 : 当 anon page 发生 page fault 在 swap cache 中间没有找到的时候，创建了一个 page，于是乎将该 page 通过 add_to_swap 加入到 swap cache
    2. 实际上 : 只有 shrink_page_list 调用，这个想法 `__read_swap_cache_async` 实现的非常不错。
    3. 猜测 : 当一个 page 需要被写会的时候，首先将其添加到 swap cache 中间
```c
/**
 * add_to_swap - allocate swap space for a page
 * @page: page we want to move to swap
 *
 * Allocate swap space for the page and add the page to the
 * swap cache.  Caller needs to hold the page lock.
 */
int add_to_swap(struct page *page)
    get_swap_page     // 分配 swp_entry_t // todo 实现比想象的要复杂的多，首先进入到 swap_slot.c 但是 swap_slot.c 中间似乎根本不处理什么具体分配，而是靠 swapfile.c 的 get_swap_pages // todo 获取到 entry.val != 0 说明 page 已经被加入到 swap 中间 ?
    add_to_swap_cache // 将 page 和 swp_entry_t 链接起来，形成
    set_page_dirty // todo 和 page-writeback.c 有关，line 240 的注释看不懂
    put_swap_page // Called after dropping swapcache to decrease refcnt to swap entries ，和 get_swap_page 对称的函数，核心是调用 free_swap_slot

// 从 get_swap_page 和 put_swap_page 中间，感觉 swp_entry_t 存在引用计数 ? 应该不可能呀 !
```
4. 利用 swap_cache_info 来给管理员提供信息
```c
static struct {
  unsigned long add_total;
  unsigned long del_total;
  unsigned long find_success;
  unsigned long find_total;
} swap_cache_info;
```


问题:
1. 两种的 readahead 机制 swap_cluster_readahead 和 swap_vma_readahead 的区别 ?
```c
/**
 * swapin_readahead - swap in pages in hope we need them soon
 * @entry: swap entry of this memory
 * @gfp_mask: memory allocation flags
 * @vmf: fault information
 *
 * Returns the struct page for entry and addr, after queueing swapin.
 *
 * It's a main entry function for swap readahead. By the configuration,
 * it will read ahead blocks by cluster-based(ie, physical disk based)
 * or vma-based(ie, virtual address based on faulty address) readahead.
 */
struct page *swapin_readahead(swp_entry_t entry, gfp_t gfp_mask,
        struct vm_fault *vmf)
{
  return swap_use_vma_readahead() ?
      swap_vma_readahead(entry, gfp_mask, vmf) :
      swap_cluster_readahead(entry, gfp_mask, vmf);
}
```
2. 什么时候使用 readahead，什么时候使用 page-io.c:swap_readpage ?<br/> memory.c::do_swap_page 中间说明
3. add_to_swap 和 add_to_swap_cache 的关系是什么 ?<br/> add_to_swap 首先调用 swap_slot.c::get_swap_page 分配 swap slot，然后调用 add_to_swap_cache 将 page 和 swap slot 关联起来。
4. swap cache 的 page 和 page cache 的 page 在 page reclaim 机制中间有没有被区分对待 ? TODO
5. swap cache 不复用 page cache ? <br/>两者只是使用的机制有点类似，通过索引查询到 page frame，但是 swap cache 的 index 是 swp_entry_t，而 page cache 的 index 是文件的偏移量。对于每一个文件，都是存在一个 radix_tree 来提供索引功能，对于 swap，

## 为什么只有注册了一个 writepage

```c
/*
 * swapper_space is a fiction, retained to simplify the path through
 * vmscan's shrink_page_list.
 */
static const struct address_space_operations swap_aops = {
	.writepage	= swap_writepage,
	.dirty_folio	= noop_dirty_folio,
#ifdef CONFIG_MIGRATION
	.migrate_folio	= migrate_folio,
#endif
};
```

- swap_writepage 在 shmem 中被直接使用，因为 shmem 总是写入到 swap 中的，不存在写入到文件中的情况，无需通过。
