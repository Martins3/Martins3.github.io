# iouring swap
<!-- 578b4d12-012a-499d-a9c6-22e73cc73144 -->

## anon page 首先加入到 swap cache 中，然后开始写入

- shrink_folio_list 中
```c
  		/*
		 * Anonymous process memory has backing store?
		 * Try to allocate it some swap space here.
		 * Lazyfree folio could be freed directly
		 */
		if (folio_test_anon(folio) && folio_test_swapbacked(folio)) {
			if (!folio_test_swapcache(folio)) {
```


## [ ]  那么什么时候从 swap 中离开?
```c
#ifdef CONFIG_SWAP
static __always_inline bool folio_test_swapcache(const struct folio *folio)
{
	return folio_test_swapbacked(folio) &&
			test_bit(PG_swapcache, const_folio_flags(folio, 0));
}

static __always_inline bool PageSwapCache(const struct page *page)
{
	return folio_test_swapcache(page_folio(page));
}

SETPAGEFLAG(SwapCache, swapcache, PF_NO_TAIL)
CLEARPAGEFLAG(SwapCache, swapcache, PF_NO_TAIL)

// 展开之后得到:
static __always_inline void folio_set_swapcache(struct folio *folio) {
  set_bit(PG_swapcache, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void SetPageSwapCache(struct page *page) {
  set_bit(PG_swapcache, &PF_NO_TAIL(page, 1)->flags);
}
static __always_inline void folio_clear_swapcache(struct folio *folio) {
  clear_bit(PG_swapcache, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void ClearPageSwapCache(struct page *page) {
  clear_bit(PG_swapcache, &PF_NO_TAIL(page, 1)->flags);
}
```

## swap backed 的问题
不过，这里为什么需要特殊 folio_test_swapcache 的?

```c
static __always_inline bool folio_test_swapcache(const struct folio *folio)
{
	return folio_test_swapbacked(folio) &&
			test_bit(PG_swapcache, const_folio_flags(folio, 0));
}

PAGEFLAG(SwapBacked, swapbacked, PF_NO_TAIL)
	__CLEARPAGEFLAG(SwapBacked, swapbacked, PF_NO_TAIL)
	__SETPAGEFLAG(SwapBacked, swapbacked, PF_NO_TAIL)
```

一个
1. folio_test_swapbacked
2. folio_set_swapbacked &&  __folio_set_swapbacked
3. folio_clear_swapbacked : 仅仅用于 Lazyfree 了

folio_mark_lazyfree

还是主要是为了区分哪些 shmem 产生的文件，他们是 file ，但是不是 anon

展开为:
```c
static __always_inline void folio_clear_swapbacked(struct folio *folio) {
  clear_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
static __always_inline void __folio_clear_swapbacked(struct folio *folio) {
  __clear_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
}
```


## 其实就是 swapbacked 的，但是这个 flag 的含义显然滥用了

一个 page 是 anon ，但是不是 folio_test_swapbacked

例如在 `shrink_folio_list`
```c
		/*
		 * Anonymous process memory has backing store?
		 * Try to allocate it some swap space here.
		 * Lazyfree folio could be freed directly
		 */
		if (folio_test_anon(folio) && folio_test_swapbacked(folio)) {
```

在 try_to_unmap_one 中:
```c
			/* MADV_FREE page check */
			if (!folio_test_swapbacked(folio)) {
      }
```

## 再次确认一下 io 路径

所以就是先分配，然后写入的
```txt
@[
    zram_submit_bio+5
    __submit_bio+308
    submit_bio_noacct_nocheck+477
    submit_bio_wait+91
    __swap_writepage+1037
    swap_writepage+114
    shrink_folio_list+2607
    evict_folios+5087
    try_to_shrink_lruvec+501
    shrink_lruvec+400
    shrink_node+534
    do_try_to_free_pages+305
    try_to_free_mem_cgroup_pages+430
    try_charge_memcg+735
    __mem_cgroup_charge+66
    do_pte_missing+2579
    handle_mm_fault+2121
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 44
@[
    zram_submit_bio+5
    __submit_bio+308
    submit_bio_noacct_nocheck+477
    submit_bio_wait+91
    swap_read_folio+676
    do_swap_page+2892
    handle_mm_fault+2086
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 25600
```

## 其实基本想法就是把 swapfile 的功能替代掉

include/linux/swap.h 中正好把这些功能都是放到一起的:
```c
extern void si_swapinfo(struct sysinfo *);
swp_entry_t folio_alloc_swap(struct folio *folio);
bool folio_free_swap(struct folio *folio);
void put_swap_folio(struct folio *folio, swp_entry_t entry);
extern swp_entry_t get_swap_page_of_type(int);
extern int get_swap_pages(int n, swp_entry_t swp_entries[], int order);
extern int add_swap_count_continuation(swp_entry_t, gfp_t);
extern void swap_shmem_alloc(swp_entry_t, int);
extern int swap_duplicate(swp_entry_t);
extern int swapcache_prepare(swp_entry_t entry, int nr);
extern void swap_free_nr(swp_entry_t entry, int nr_pages);
extern void swapcache_free_entries(swp_entry_t *entries, int n);
extern void free_swap_and_cache_nr(swp_entry_t entry, int nr);
int swap_type_of(dev_t device, sector_t offset);
int find_first_swap(dev_t *device);
extern unsigned int count_swap_pages(int, int);
extern sector_t swapdev_block(int, pgoff_t);
extern int __swap_count(swp_entry_t entry);
extern int swap_swapcount(struct swap_info_struct *si, swp_entry_t entry);
extern int swp_swapcount(swp_entry_t entry);
struct swap_info_struct *swp_swap_info(swp_entry_t entry);
extern int init_swap_address_space(unsigned int type, unsigned long nr_pages);
extern void exit_swap_address_space(unsigned int type);
extern struct swap_info_struct *get_swap_device(swp_entry_t entry);
sector_t swap_folio_sector(struct folio *folio);
```
1. backing_dev_info -> 让 folio->swap 的内容指向一个新的结构体而不是 backing_dev_info

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
