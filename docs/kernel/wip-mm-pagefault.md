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

## 使用 BOOM 来看下硬件如何是如何实现的
- TLB 和 cache 是同时查找的

## vmid 还是 mmid 啥的，就是每一个 process 一个 id 的技术，找一下

## 总结
- 如果减少不命中的开销
  - 大页
- 减少 miss rate 的方法
  - 大页，mmid 的那个

## 原因
  - 缺页
  - protection
    - dirty bit tracking

## 分析一下函数细节
- do_user_addr_fault : 不知道为什么，这个函数充满了 `fatal_signal_pending` 的检测
  - fault_signal_pending

```c
	if (fault & VM_FAULT_OOM) {
		/* Kernel mode? Handle exceptions or die: */
		if (!user_mode(regs)) {
			kernelmode_fixup_or_oops(regs, error_code, address,
						 SIGSEGV, SEGV_MAPERR,
						 ARCH_DEFAULT_PKEY);
			return;
		}

		/*
		 * We ran out of memory, call the OOM killer, and return the
		 * userspace (which will retry the fault, or kill us if we got
		 * oom-killed):
		 */
		pagefault_out_of_memory();
	} else {
		if (fault & (VM_FAULT_SIGBUS|VM_FAULT_HWPOISON|
			     VM_FAULT_HWPOISON_LARGE))
			do_sigbus(regs, error_code, address, fault);
		else if (fault & VM_FAULT_SIGSEGV)
			bad_area_nosemaphore(regs, error_code, address);
		else
			BUG();
	}
```
在 page fault 的过程中，其中返回 `VM_FAULT_SIGBUS` 的位置超级多。

## 分析一下，如何在 copy_to_user 的过程中出现 page fault ，是如何处理的

## 整理一下这个

```txt
[ 2884.290877]  [<ffffffff8107e8d8>] native_flush_tlb_others+0xb8/0xc0
[ 2884.291268]  [<ffffffff8107e94b>] flush_tlb_mm_range+0x6b/0x140
[ 2884.291638]  [<ffffffff811edf0f>] tlb_flush_mmu_tlbonly+0x2f/0xc0
[ 2884.292217]  [<ffffffff811efa4f>] arch_tlb_finish_mmu+0x3f/0x80
[ 2884.292590]  [<ffffffff811efb73>] tlb_finish_mmu+0x23/0x30
[ 2884.292931]  [<ffffffff811fcf1b>] exit_mmap+0xdb/0x1a0
[ 2884.293254]  [<ffffffff81097bb7>] mmput+0x67/0xf0
[ 2884.293550]  [<ffffffff810a1938>] do_exit+0x288/0xa20
[ 2884.293864]  [<ffffffff810a214f>] do_group_exit+0x3f/0xa0
[ 2884.294204]  [<ffffffff810a21c4>] SyS_exit_group+0x14/0x20
[ 2884.294549]  [<ffffffff81794f92>] system_call_fastpath+0x25/0x2a
```

## 补充这个
- https://lore.kernel.org/all/20230227173632.3292573-9-surenb@google.com/T/#m2b041c67b39980bafcd16bc5f897297212b5ee36
  - https://lwn.net/Articles/906852/
