# 应该使用的方法

1. swp entry 是如何构建出来的?

  -> 发生一个 page 给 server -> server 返回一个 swp entry

mm/swap_slots.c 具体实现在 folio_alloc_swap 中的:

需要从用户态空间看到，这个不太行


为什么 shmem_writepage 先需要 write to page cache ，然后 ...
```c
	if (add_to_swap_cache(folio, swap,
			__GFP_HIGH | __GFP_NOMEMALLOC | __GFP_NOWARN,
			NULL) == 0) {
		shmem_recalc_inode(inode, 0, 1);
		swap_shmem_alloc(swap);
		shmem_delete_from_page_cache(folio, swp_to_radix_entry(swap));

		mutex_unlock(&shmem_swaplist_mutex);
		BUG_ON(folio_mapped(folio));
		return swap_writepage(&folio->page, wbc);
	}
```

## 什么时候从 page cache 中离开的

__delete_from_swap_cache

```txt
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    folio_free_swap+309
    do_swap_page+1429
    handle_mm_fault+2086
    do_user_addr_fault+1218
    exc_page_fault+137
    asm_exc_page_fault+38
    filldir64+198
    nfs_do_filldir+195
    nfs_readdir+1484
    iterate_dir+127
    __se_sys_getdents64+105
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    folio_free_swap+309
    free_swap_cache+79
    do_wp_page+3427
    do_swap_page+2281
    handle_mm_fault+2086
    do_user_addr_fault+791
    exc_page_fault+137
    asm_exc_page_fault+38
]: 1
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    shmem_swapin_folio+1051
    shmem_get_folio_gfp+334
    shmem_fault+142
    __do_fault+67
    do_pte_missing+835
    handle_mm_fault+2121
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 1
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    folio_free_swap+309
    do_swap_page+1429
    handle_mm_fault+2086
    do_user_addr_fault+791
    exc_page_fault+137
    asm_exc_page_fault+38
    filldir64+191
    nfs_do_filldir+195
    nfs_readdir+1484
    iterate_dir+127
    __se_sys_getdents64+105
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    shmem_swapin_folio+1051
    shmem_get_folio_gfp+334
    shmem_fault+142
    __do_fault+67
    do_pte_missing+357
    handle_mm_fault+2121
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 5
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    folio_free_swap+309
    __try_to_reclaim_swap+199
    free_swap_and_cache_nr+551
    unmap_page_range+2290
    unmap_vmas+241
    exit_mmap+498
    __mmput+67
    exec_mmap+569
    begin_new_exec+1185
    load_elf_binary+654
    bprm_execve+648
    do_execveat_common+782
    __x64_sys_execve+58
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 6
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    folio_free_swap+309
    do_swap_page+1429
    handle_mm_fault+2086
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 90
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    folio_free_swap+309
    do_swap_page+1429
    handle_mm_fault+2086
    do_user_addr_fault+791
    exc_page_fault+137
    asm_exc_page_fault+38
]: 115
@[
    __delete_from_swap_cache+5
    __remove_mapping+369
    shrink_folio_list+2814
    evict_folios+5087
    try_to_shrink_lruvec+501
    shrink_one+150
    shrink_node+2912
    kswapd+2185
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 118826
```

## [ ] swap cache 有 clean 的说法吗?
如果 swap cache 支持 clean / dirty 的标记，
当把 swap cache page 再次 swap out 的时候，而且只是 read fault ，
如果需要换出的时候，可以像 clean page cache 一样直接换出。

实现方法可以按照 page cache 的 clean 和 dirty 方法，如果 swap in 之后，
需要设置 wp 的状态。

```txt
@[
    noop_dirty_folio+5
    try_to_unmap_one+1325
    rmap_walk_anon+319
    try_to_unmap+115
    shrink_folio_list+1429
    evict_folios+5087
    try_to_shrink_lruvec+501
    shrink_one+150
    shrink_node+2912
    kswapd+2185
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 1310316
@[
    noop_dirty_folio+5
    add_to_swap+704
    shrink_folio_list+1282
    evict_folios+5087
    try_to_shrink_lruvec+501
    shrink_one+150
    shrink_node+2912
    kswapd+2185
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 2673421
```

add_to_swap 中为什么需要再去标记，我认为这个其实没有必要的吧:
```c
	/*
	 * Normally the folio will be dirtied in unmap because its
	 * pte should be dirty. A special case is MADV_FREE page. The
	 * page's pte could have dirty bit cleared but the folio's
	 * SwapBacked flag is still set because clearing the dirty bit
	 * and SwapBacked flag has no lock protected. For such folio,
	 * unmap will not set dirty bit for it, so folio reclaim will
	 * not write the folio out. This can cause data corruption when
	 * the folio is swapped in later. Always setting the dirty flag
	 * for the folio solves the problem.
	 */
	folio_mark_dirty(folio);
```
而且根据现在 swap 的实现， folio_mark_dirty 实际上最后什么都不会做了，

目前看，anonymous page 的 pte 总是 dirty 吗?

1. try_to_unmap_one
```c
		/* Set the dirty flag on the folio now the pte is gone. */
		if (pte_dirty(pteval))
			folio_mark_dirty(folio);
```
3. file mapped page 按道理，如果是 dirty 的，早就应该配置为 dirty ，不应该等到要 unmpa 才去
  - 为什么要做这种 pte dirty 向 folio dirty 的移动

如果 madvise swap out 了，页面还会保留吗?

## [ ] 为什么感觉 add_to_swap 的方法如此奇怪

add_to_swap 先通过 add_to_swap_cache 添加到 swap 中，然后马上删除


使用测试
```sh
	sudo cgexec -g memory:$cgroup_name stress-ng --vm-bytes 200m --vm-keep --vm 1 --vm-hang 0
```

然后同时观测:
1.  __delete_from_swap_cache
2. add_to_swap

可以发现这就是事实，而且 __delete_from_swap_cache 调用次数比 add_to_swap 多一倍，不知道为什么。

## 命中 swap 的过程
细节在 do_swap_page 中，首先在 swap cache 中找，如果可以找到，就直接替换。

## swap out 的时候反向映射如何搞的?

不是在 add_to_swap

在 try_to_unmap_one 中填充的 swpa entry ，的确，是需要反向映射的。

原来 try_to_unmap 发生在这里?
```txt
@[
    try_to_unmap+5
    shrink_folio_list+1429
    evict_folios+5087
    try_to_shrink_lruvec+501
    shrink_one+150
    shrink_node+2912
    kswapd+2185
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 1252480
```

## 搞清楚基本路径

__delete_from_swap_cache 调用 folio_clear_swapcache :

从 swap cache 中移除掉，这个就是我说
```txt
@[
    __delete_from_swap_cache+5 <-- 如果发现被 swap 的 page 是 swap cache 中的
    __remove_mapping+369
    shrink_folio_list+2814
    evict_folios+5087
    try_to_shrink_lruvec+501
    shrink_lruvec+400
    shrink_node+534
    do_try_to_free_pages+305
    try_to_free_mem_cgroup_pages+430
    try_charge_memcg+735
    mem_cgroup_swapin_charge_folio+193
    __read_swap_cache_async+417
    swap_cluster_readahead+551
    swapin_readahead+112
    do_swap_page+422
    handle_mm_fault+2086
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 115494
@[
    __delete_from_swap_cache+5
    delete_from_swap_cache+93
    folio_free_swap+309
    do_swap_page+1429
    handle_mm_fault+2086
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 120226
```

- [ ] 前脚 add_to_swap 刚添加，后脚就释放吗?

swap cache 对于单线程没用啊 ?

## 新的整理
swap学习记录和研究 - lianux的文章 - 知乎
https://zhuanlan.zhihu.com/p/1911006969755578935

## 2026-06-02 似乎 7.0 内核开始，对于 swap 的使用开始变的很主动了

7.0.8-200.fc44.x86_64

```txt
               total        used        free      shared  buff/cache   available
Mem:           125Gi        75Gi        10Gi        37Gi        76Gi        49Gi
Swap:          8.0Gi       7.6Gi       365Mi
```

发现内存使用过半，基本上 swap 就会被用完。

## swap 到 GPU 显存中，真的有意思了
https://news.ycombinator.com/item?id=48377404

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
