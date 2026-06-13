# page::private

```c
            /**
             * @private: Mapping-private opaque data.
             * Usually used for buffer_heads if PagePrivate.
             * Used for swp_entry_t if PageSwapCache.
             * Indicates order in the buddy system if PageBuddy.
             */
            unsigned long private;
```

### swap

```c
/*
 * Returns the page offset into bdev for the specified page's swap entry.
 */
sector_t map_swap_page(struct page *page, struct block_device **bdev)
{
    swp_entry_t entry;
    entry.val = page_private(page);
    return map_swap_entry(entry, bdev);
}
```
- 可以在物理页面中间存储该页面在 swap 中间的偏移量，显然是不可能放到 pte 中间的，中间最多放一个 flag 位表示被换到 swap 中间了，理解错误的地方
  1. PageSwapCache 是什么东西 ? 只有当在 page swap cache 中间的才有意义
  2. add_to_swap 被调用的条件 是什么 ? 分析 shrink_page_list
  3. swap cache 不是加快操作，而是必须存在的
- [ ] 似乎也是可以存放 page 的 order 的

### 各种文件系统中

### kvm 中

不知道做什么的，先记录一下吧
```txt
static struct kvm_mmu_page *kvm_mmu_alloc_shadow_page(struct kvm *kvm,
						      struct shadow_page_caches *caches,
						      gfn_t gfn,
						      struct hlist_head *sp_list,
						      union kvm_mmu_page_role role)
{
	struct kvm_mmu_page *sp;

	sp = kvm_mmu_memory_cache_alloc(caches->page_header_cache);
	sp->spt = kvm_mmu_memory_cache_alloc(caches->shadow_page_cache);
	if (!role.direct && role.level <= KVM_MAX_HUGEPAGE_LEVEL)
		sp->shadowed_translation = kvm_mmu_memory_cache_alloc(caches->shadowed_info_cache);

	set_page_private(virt_to_page(sp->spt), (unsigned long)sp);

	INIT_LIST_HEAD(&sp->possible_nx_huge_page_link);

	/*
	 * active_mmu_pages must be a FIFO list, as kvm_zap_obsolete_pages()
	 * depends on valid pages being added to the head of the list.  See
	 * comments in kvm_zap_obsolete_pages().
	 */
	sp->mmu_valid_gen = kvm->arch.mmu_valid_gen;
	list_add(&sp->link, &kvm->arch.active_mmu_pages);
	kvm_account_mmu_page(kvm, sp);

	sp->gfn = gfn;
	sp->role = role;
	hlist_add_head(&sp->hash_link, sp_list);
	if (sp_has_gptes(sp))
		account_shadowed(kvm, sp);

	return sp;
}
```

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
