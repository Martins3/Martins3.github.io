## 简单分析一下 folio 在 lru 中移动

### 结论 1 : 一个 folio 必须被显示的添加 lru

对于 folio_add_lru 的经典的调用:

- asm_exc_page_fault
  - exc_page_fault
    - handle_page_fault
      - do_user_addr_fault
        - handle_mm_fault
          - __handle_mm_fault
            - handle_pte_fault
              - do_fault
                - do_read_fault
                  - __do_fault
                    - filemap_fault
                      - do_sync_mmap_readahead
                        - do_page_cache_ra
                          - page_cache_ra_unbounded
                            - filemap_add_folio

- do_copy
  - xwrite
    - kernel_write
        - __kernel_write
          - __kernel_write_iter
            - generic_file_write_iter
              - __generic_file_write_iter
                - generic_perform_write
                  - simple_write_begin
                    - grab_cache_page_write_begin
                      - pagecache_get_page
                        - __filemap_get_folio
                          - filemap_add_folio
                            - folio_add_lru
                              - folio_batch_add_and_move
                                - folio_batch_move_lru

```txt
@[
    folio_batch_move_lru+1
    folio_add_lru+91
    do_anonymous_page+766
    __handle_mm_fault+2093
    handle_mm_fault+341
    do_user_addr_fault+351
    exc_page_fault+109
    asm_exc_page_fault+38
]: 152
```

filemap_add_folio 然后调用 filemap_add_folio

```txt
@[
    folio_add_lru+5
    do_anonymous_page+1517
    __handle_mm_fault+3105
    handle_mm_fault+383
    do_user_addr_fault+380
    exc_page_fault+127
    asm_exc_page_fault+38
]: 1413753
```

### 结论 2 : 添加的时候有多种选项

添加到 lru 通过多种辅助函数来实现将 folio 移动到不同的位置，是否移动到全局的 lru list 中:

- folio_add_lru : 将 page 添加到 folio_batch 中
- folio_activate : 将 page 从 inactive 移动到 active 中
- deactivate_file_folio :
- mark_page_lazyfree : 缓存匿名页，清除掉 PG_activate, PG_referenced, PG_swapbacked 标志后，将这些页加入到 LRU_INACTIVE_FILE 链表中
- lru_add_drain_cpu : 将 folio_batch 中的 page 移动到 lru 中

他们都使用 **folio_batch_add_and_move** 来用于遍历 folio_batch ，调用 `move_fn_t`

用于搬运一个 page 函数，都是 `move_fn_t` 类型的，他们负责具体的移动工作
- lru_add_fn
- lru_lazyfree_fn
- lru_move_tail_fn
- folio_activate_fn
- lru_deactivate_fn
- lru_deactivate_file_fn

### 结论 3 : 利用 folio_batch 可以批量操作

一共缓存在这里:
```c
/*
 * The following folio batches are grouped together because they are protected
 * by disabling preemption (and interrupts remain enabled).
 */
struct cpu_fbatches {
	local_lock_t lock;
	struct folio_batch lru_add;
	struct folio_batch lru_deactivate_file;
	struct folio_batch lru_deactivate;
	struct folio_batch lru_lazyfree;
#ifdef CONFIG_SMP
	struct folio_batch activate;
#endif
};
```

- __folio_alloc
    - __alloc_pages_slowpath
      - __alloc_pages_direct_reclaim
        - __perform_reclaim
          - try_to_free_pages
            - do_try_to_free_pages
              - shrink_zones
                - shrink_node
                  - shrink_node_memcgs
                    - shrink_lruvec
                      - shrink_list
                        - shrink_inactive_list
                          - lru_add_drain
                            - lru_add_drain_cpu


- `lru_add_drain` transfer all pages from the per-CPU LRU caches to the global lists

例如 folio_add_lru 中:
- folio_add_lru
  - folio_batch_add_and_move
    - folio_batch_add : 先添加缓存
    - folio_batch_move_lru : 然后整体添加

lru_add_drain 和 folio_add_lru 的关系是什么?

正如名字显示的那样，lru_add_drain 和 folio_add_lru ，前者要求立刻将 folio_batch 中内容 flush ，
后续是需要等待的

### 结论 4 : 加入到 lru 的 folio 必有 PG_lru 的 flag

- shrink_lruvec
  - shrink_list
    - shrink_inactive_list
      - lru_add_drain
        - lru_add_drain_cpu
          - folio_batch_move_lru

- shrink_lruvec
  - shrink_list
    - shrink_inactive_list
      - move_folios_to_lru


大致是这样的调用路径，最后都是调用到
folio_set_lru(folio);

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
