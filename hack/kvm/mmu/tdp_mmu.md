# tdp_mmu


- kvm_tdp_page_fault
  - kvm_tdp_mmu_page_fault
    - kvm_tdp_mmu_map
  - direct_page_fault : 不知道这个是为了处理什么？


```txt
@[
    kvm_tdp_mmu_map+5
    direct_page_fault+862 <--------- 这个 backtrace 显示的有问题吧，应该是 kvm_tdp_mmu_page_fault
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 465
```

## 问题

### [x] kvm_tdp_mmu_map 中 为什么这里也有 shadow page table，只是统一的名称

```c
#define tdp_mmu_for_each_pte(_iter, _mmu, _start, _end)		\
	for_each_tdp_pte(_iter, to_shadow_page(_mmu->root.hpa), _start, _end)

/*
 * Iterates over every SPTE mapping the GFN range [start, end) in a
 * preorder traversal.
 */
#define for_each_tdp_pte_min_level(iter, root, min_level, start, end) \
	for (tdp_iter_start(&iter, root, min_level, start); \
	     iter.valid && iter.gfn < end;		     \
	     tdp_iter_next(&iter))

#define for_each_tdp_pte(iter, root, start, end) \
	for_each_tdp_pte_min_level(iter, root, PG_LEVEL_4K, start, end)
```

1. 参数: gfn
2. `to_shadow_page(_mmu->root.hpa)` : ept page table 的根
  - 每一个 ept page 都需要
```c
static inline struct kvm_mmu_page *to_shadow_page(hpa_t shadow_page)
{
	struct page *page = pfn_to_page((shadow_page) >> PAGE_SHIFT);

	return (struct kvm_mmu_page *)page_private(page);
}
```

设置时间:

```c
static void link_shadow_page(struct kvm_vcpu *vcpu, u64 *sptep,
			     struct kvm_mmu_page *sp)
{
	__link_shadow_page(vcpu->kvm, &vcpu->arch.mmu_pte_list_desc_cache, sptep, sp, true);
}
```

直接访问:
- kvm_mmu_alloc_shadow_page : 不开嵌套虚拟化的时候，不会调用
- tdp_mmu_init_sp
  - 实际上，每一个 ept page 都需要建立一个 kvm_mmu_page 的

```txt
@[
    tdp_mmu_init_sp+1
    kvm_tdp_mmu_get_vcpu_root_hpa+223
    kvm_mmu_load+1527
    kvm_arch_vcpu_ioctl_run+4482
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 54
@[
    tdp_mmu_init_sp+1
    kvm_tdp_mmu_map+296
    direct_page_fault+862
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 2192
```

## 清理
```txt
    kvm_tdp_mmu_zap_all+5
    kvm_mmu_zap_all+224
    kvm_mmu_notifier_release+47
    __mmu_notifier_release+116
    exit_mmap+656
    __mmput+67
    do_exit+723
    do_group_exit+49
    get_signal+2390
    arch_do_signal_or_restart+46
    exit_to_user_mode_prepare+267
    syscall_exit_to_user_mode+27
    do_syscall_64+76
    entry_SYSCALL_64_after_hwframe+114
```
- [ ] 非常奇怪，当 L2 结束的时候，没有跟踪到这个

## kvm_tdp_mmu_write_protect_gfn
- 没人调用，我谢天谢地哦

## 似乎操作 spte 比想想的复杂

* set_spte_gfn
* `__tdp_mmu_set_spte`
* tdp_mmu_set_spte_atomic
  - `__handle_changed_spte` : handle bookkeeping associated with an SPTE change
    - 处理 dirty 了之类的情况
    - handle_removed_pt : 专门来处理移除 page 的情况

```txt
@[
    __handle_changed_spte+1
    kvm_tdp_mmu_map+829
    direct_page_fault+862
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 447750
@[
    __handle_changed_spte+1
    __handle_changed_spte+905
    __handle_changed_spte+905
    __tdp_mmu_zap_root+238
    tdp_mmu_zap_root_work+65
    process_one_work+482
    worker_thread+84
    kthread+233
    ret_from_fork+41
]: 614912
```
