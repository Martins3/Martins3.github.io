# 关于 pagefault 的一切

## 一般情况
- mm-pagefault.md 中整理下，重点是 cow 等

## IOMMU 的 pagefault 软件可见吗？


## async page fault

## 嵌套虚拟化中的 page fault

### [ ]  L0 将 L1 的内存换走，过程如何？
1. 设置 qemu 的 page walk 过程
2. 设置 ept 的 page walk 过程

#### [ ] mmu_shrinker
- [ ] 如何才可以调用到这里去?

- kvm_mmu_alloc_shadow_page : 中会增加 mmu_shrink_count 的返回值，所以这个特指是 spte 使用的页面
```c
static struct shrinker mmu_shrinker = {
	.count_objects = mmu_shrink_count,
	.scan_objects = mmu_shrink_scan,
	.seeks = DEFAULT_SEEKS * 10,
};
```

```c
static const struct mmu_notifier_ops kvm_mmu_notifier_ops = {
	.invalidate_range	= kvm_mmu_notifier_invalidate_range,
	.invalidate_range_start	= kvm_mmu_notifier_invalidate_range_start,
	.invalidate_range_end	= kvm_mmu_notifier_invalidate_range_end,
	.clear_flush_young	= kvm_mmu_notifier_clear_flush_young,
	.clear_young		= kvm_mmu_notifier_clear_young,
	.test_young		= kvm_mmu_notifier_test_young,
	.change_pte		= kvm_mmu_notifier_change_pte,
	.release		= kvm_mmu_notifier_release,
};
```

将 QEMU 放到 cgroup 中，让 Guest 产生大量的 swap，最后可以看到如下内容:
```txt
@[
    kvm_mmu_notifier_invalidate_range+5
    __mmu_notifier_invalidate_range_end+109
    try_to_unmap_one+1084
    rmap_walk_anon+360
    try_to_unmap+137
    shrink_folio_list+1383
    evict_folios+557
    shrink_lruvec+552
    shrink_node+541
    do_try_to_free_pages+213
    try_to_free_mem_cgroup_pages+271
    try_charge_memcg+426
    charge_memcg+50
    __mem_cgroup_charge+45
    __handle_mm_fault+2303
    handle_mm_fault+259
    __get_user_pages+463
    __gup_longterm_locked+216
    get_user_pages_unlocked+119
    hva_to_pfn+268
    kvm_faultin_pfn+146
    direct_page_fault+774
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 2854446
```
- kvm_mmu_notifier_invalidate_range_start
  - 其 hook 为: kvm_unmap_gfn_range
    - kvm_memslots_have_rmaps : 清理反向映射
    - kvm_tdp_mmu_unmap_gfn_range : 清理 ept 的映射

- [ ] guest 中也是执行相同的路径吗？

### [ ]  L1 中 QEMU 的内存被 L0 换出了，对此 L1 并不知情，L2 也不知情，过程是如何的?
### [ ]  L1 中将 QEMU 的内存换出，L2 不知道，过程如何？

## 测试
1. 一次 page fault 的开销
2. 一次 tlb 不命中的开销

## iommu 偶尔发现的，原来还需要 flush tlb 啊
```c
/**
 * struct iommu_flush_ops - IOMMU callbacks for TLB and page table management.
 *
 * @tlb_flush_all:  Synchronously invalidate the entire TLB context.
 * @tlb_flush_walk: Synchronously invalidate all intermediate TLB state
 *                  (sometimes referred to as the "walk cache") for a virtual
 *                  address range.
 * @tlb_add_page:   Optional callback to queue up leaf TLB invalidation for a
 *                  single page.  IOMMUs that cannot batch TLB invalidation
 *                  operations efficiently will typically issue them here, but
 *                  others may decide to update the iommu_iotlb_gather structure
 *                  and defer the invalidation until iommu_iotlb_sync() instead.
 *
 * Note that these can all be called in atomic context and must therefore
 * not block.
 */
struct iommu_flush_ops {
	void (*tlb_flush_all)(void *cookie);
	void (*tlb_flush_walk)(unsigned long iova, size_t size, size_t granule,
			       void *cookie);
	void (*tlb_add_page)(struct iommu_iotlb_gather *gather,
			     unsigned long iova, size_t granule, void *cookie);
};
```

## qemu 中 tcg 的 page fault
