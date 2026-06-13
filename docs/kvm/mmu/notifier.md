## kvm mmu notifier
<!-- da3df8d4-dae0-4583-aec9-27d3595dcf77 -->

```c
static const struct mmu_notifier_ops kvm_mmu_notifier_ops = {
	.invalidate_range_start	= kvm_mmu_notifier_invalidate_range_start,
	.invalidate_range_end	= kvm_mmu_notifier_invalidate_range_end,
	.clear_flush_young	= kvm_mmu_notifier_clear_flush_young,
	.clear_young		= kvm_mmu_notifier_clear_young,
	.test_young		= kvm_mmu_notifier_test_young,
	.release		= kvm_mmu_notifier_release,
};
```

kvm_mmu_notifier_invalidate_range_start 这些使用的地址都是虚拟地址

简单来说，就是类 swap 场景，也能也有 ksm page migrate 之类的，使用
- invalidate_range_start
- invalidate_range_end

然后 idle page tracking 来使用
- clear_flush_young
- clear_young
- test_young

### page migrate
```txt
@[
    tdp_mmu_zap_leafs+926
    tdp_mmu_zap_leafs+926
    kvm_tdp_mmu_unmap_gfn_range+120
    kvm_unmap_gfn_range+434
    kvm_mmu_notifier_invalidate_range_start+352
    __mmu_notifier_invalidate_range_start+273
    try_to_migrate_one+389
    rmap_walk_anon+319
    try_to_migrate+214
    migrate_pages_batch+944
    migrate_pages+1736
    migrate_misplaced_folio+115
    do_huge_pmd_numa_page+865
    handle_mm_fault+1697
    __get_user_pages+1100
    get_user_pages_unlocked+264
    hva_to_pfn+264
    kvm_faultin_pfn+481
    kvm_tdp_page_fault+180
    kvm_mmu_do_page_fault+372
    kvm_mmu_page_fault+209
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 177
```

### swap 的场景
```txt
@[
    kvm_mmu_notifier_invalidate_range_end+5
    __mmu_notifier_invalidate_range_end+84
    try_to_unmap_one+3016
    rmap_walk_file+207
    try_to_unmap+109
    shrink_folio_list+1466
    evict_folios+636
    try_to_shrink_lruvec+420
    shrink_lruvec+282
    shrink_node+725
    do_try_to_free_pages+179
    try_to_free_mem_cgroup_pages+254
    memory_max_write+422
    kernfs_fop_write_iter+307
    vfs_write+673
    ksys_write+111
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 127456
@[
    kvm_mmu_notifier_invalidate_range_end+5
    __mmu_notifier_invalidate_range_end+84
    try_to_unmap_one+3016
    rmap_walk_file+207
    try_to_unmap+109
    shrink_folio_list+1466
    evict_folios+636
    try_to_shrink_lruvec+420
    shrink_lruvec+282
    shrink_node+725
    do_try_to_free_pages+179
    try_to_free_mem_cgroup_pages+254
    try_charge_memcg+418
    mem_cgroup_swapin_charge_folio+122
    __read_swap_cache_async+497
    swapin_readahead+963
    do_swap_page+649
    __handle_mm_fault+2141
    handle_mm_fault+383
    do_user_addr_fault+527
    exc_page_fault+127
    asm_exc_page_fault+38
    __rseq_handle_notify_resume+274
    xfer_to_guest_mode_handle_work+155
    kvm_arch_vcpu_ioctl_run+2245
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 779766
```

### idle page tracking
folio_referenced_one 是一定是通过这个来实现的，这个方法有点笨了:
```txt
			if (ptep_clear_flush_young_notify(vma, address,
						pvmw.pte))
				referenced++;
```

具体来说:
```txt
@[
    kvm_tdp_mmu_age_gfn_range+614
    kvm_tdp_mmu_age_gfn_range+614
    kvm_age_gfn+544
    kvm_mmu_notifier_clear_flush_young+291
    __mmu_notifier_clear_flush_young+88
    folio_referenced_one+350
    rmap_walk_anon+215
    folio_referenced+372
    shrink_folio_list+2219
    evict_folios+636
    try_to_shrink_lruvec+420
    shrink_lruvec+282
    shrink_node+725
    do_try_to_free_pages+179
    try_to_free_mem_cgroup_pages+254
    try_charge_memcg+418
    __mem_cgroup_charge+66
    do_huge_pmd_anonymous_page+271
    __handle_mm_fault+3123
    handle_mm_fault+383
    __get_user_pages+443
    get_user_pages_unlocked+230
    hva_to_pfn+188
    kvm_faultin_pfn+270
    kvm_tdp_page_fault+145
    kvm_mmu_do_page_fault+486
    kvm_mmu_page_fault+130
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 7940
```

进入到 age_gfn_range 之后，可以发现，走的是这个路线:

```c
	if (spte_ad_enabled(iter->old_spte)) {
		trace_hi(1);
		iter->old_spte = tdp_mmu_clear_spte_bits(iter->sptep,
							 iter->old_spte,
							 shadow_accessed_mask,
							 iter->level);
		new_spte = iter->old_spte & ~shadow_accessed_mask;
```

有趣，也就是 idle page tracking 也是会去使用

## mmu notifier 的其他使用场景
<!-- 57c923b7-5ad4-44b1-961a-4bd695795b5b -->

其他的高端玩家，现在也就 kvm 是看得懂的，其他的高级玩家。

知道有这个事情就可以了，不必继续着急的跟进:
```txt
 rg "static const struct mmu_notifier_ops"
virt/kvm/kvm_main.c
888:static const struct mmu_notifier_ops kvm_mmu_notifier_ops = {

arch/x86/kernel/cpu/sgx/encl.c
810:static const struct mmu_notifier_ops sgx_mmu_notifier_ops = {

drivers/iommu/intel/svm.c
107:static const struct mmu_notifier_ops intel_mmuops = {

drivers/iommu/arm/arm-smmu-v3/arm-smmu-v3-sva.c
203:static const struct mmu_notifier_ops arm_smmu_mmu_notifier_ops = {

drivers/iommu/amd/pasid.c
97:static const struct mmu_notifier_ops sva_mn = {

drivers/infiniband/hw/hfi1/mmu_rb.c
25:static const struct mmu_notifier_ops mn_opts = {

arch/s390/kvm/pv.c
614:static const struct mmu_notifier_ops kvm_s390_pv_mmu_notifier_ops = {

drivers/misc/sgi-gru/grutlbpurge.c
256:static const struct mmu_notifier_ops gru_mmuops = {

drivers/misc/ocxl/link.c
511:static const struct mmu_notifier_ops ocxl_mmu_notifier_ops = {

drivers/gpu/drm/nouveau/nouveau_svm.c
297:static const struct mmu_notifier_ops nouveau_mn_ops = {

drivers/gpu/drm/amd/amdkfd/kfd_process.c
1294:static const struct mmu_notifier_ops kfd_process_mmu_notifier_ops = {
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
