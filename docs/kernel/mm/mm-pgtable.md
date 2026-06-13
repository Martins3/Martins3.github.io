# Page Table Flags


## 一共都有那些 flags ?

## pte ，还可以 huge 吗?
```c
static inline pte_t pte_mkhuge(pte_t pte)
{
	return pte_set_flags(pte, _PAGE_PSE);
}
```

## 为什么要这么实现?
```c
pte_t pte_mkwrite(pte_t pte, struct vm_area_struct *vma)
{
	if (vma->vm_flags & VM_SHADOW_STACK) // 先不看这个 security 的东西了
		return pte_mkwrite_shstk(pte);

	pte = pte_mkwrite_novma(pte); // 什么叫做 novma ?

	return pte_clear_saveddirty(pte); // 多此一举吗?
}
```

看看 arch/x86/include/asm/pgtable.h

```c
static inline pte_t pte_mksaveddirty(pte_t pte)
{
	pteval_t v = native_pte_val(pte);

	v = mksaveddirty_shift(v);
	return native_make_pte(v);
}
```

## pte_mkdirty

这个容易理解:

```c
/*
 * Do pte_mkwrite, but only if the vma says VM_WRITE.  We do this when
 * servicing faults for write access.  In the normal case, do always want
 * pte_mkwrite.  But get_user_pages can cause write faults for mappings
 * that do not have writing enabled, when used by access_process_vm.
 */
static inline pte_t maybe_mkwrite(pte_t pte, struct vm_area_struct *vma)
{
	if (likely(vma->vm_flags & VM_WRITE))
		pte = pte_mkwrite(pte, vma);
	return pte;
}
```

pte_mkdirty 的调用位置主要是在 page fault 当分配新的 page 的时候，
立刻标记上 dirty
```c
static inline pte_t pte_mkdirty(pte_t pte)
{
	pte = pte_set_flags(pte, _PAGE_DIRTY | _PAGE_SOFT_DIRTY);

	return pte_mksaveddirty(pte);
}
```
要记住，之所以可以 pte_mkdirty ，是由于

```txt
@[
    pte_mkdirty+101
    do_swap_page+1740
    handle_mm_fault+2151
    do_user_addr_fault+780
    exc_page_fault+117
    asm_exc_page_fault+38
    copy_fpstate_to_sigframe+426
    get_sigframe+442
    x64_setup_rt_frame+121
    arch_do_signal_or_restart+306
    syscall_exit_to_user_mode+85
    do_syscall_64+250
    entry_SYSCALL_64_after_hwframe+119
]: 1

@[
    wp_page_reuse+203
    do_wp_page+1672
    handle_mm_fault+2657
    do_user_addr_fault+780
    exc_page_fault+117
    asm_exc_page_fault+38
    filldir64+208
    ext4_readdir+2428
    iterate_dir+123
    __se_sys_getdents64+105
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 2
```

### pte_mkclean : clean 的位置

- folio_clear_dirty_for_io
    - folio_mkclean : 这里需要反向映射，将映射该 page 的所有的 pte 都 clean 一下

任何时候，都不会给 anonymous page 来 mkclean 吧
应该是的，使用 stress-ng 制作 swap ，只能观测到
```txt
🧀  sudo bpftrace -e "tracepoint:kmem:hi { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    xueshi+73
    page_vma_mkclean_one+231
    page_mkclean_one+142
    rmap_walk_file+307
    folio_mkclean+184
    folio_clear_dirty_for_io+93
    mpage_prepare_extent_to_map+704
    ext4_do_writepages+865
    ext4_normal_submit_inode_data_buffers+238
    jbd2_journal_commit_transaction+1242
    kjournald2+176
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 8
```

### folio_mark_dirty 对比分析

这调用位置，只能说，只能说，这些调用位置都是预期之外的:

```txt
@[
    folio_mark_dirty+5
    fault_dirty_shared_page+74 // 被 do_shared_fault 和 wp_page_shared 无条件调用
    do_pte_missing+1014
    handle_mm_fault+2183
    __get_user_pages+1100
    get_user_pages_unlocked+264
    hva_to_pfn+264
    gfn_to_page+14
    kvm_alloc_apic_access_page+101
    vmx_vcpu_create+565
    kvm_arch_vcpu_create+471
    kvm_vm_ioctl_create_vcpu+348
    kvm_vm_ioctl+1804
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1
@[
    folio_mark_dirty+5
    filemap_page_mkwrite+349
    do_wp_page+2066
    handle_mm_fault+2657
    do_user_addr_fault+504
    exc_page_fault+117
    asm_exc_page_fault+38
]: 1
@[
    folio_mark_dirty+5 // 为什么 unmap 的也需要 ?
    unmap_page_range+2762
    unmap_vmas+241
    exit_mmap+498
    __mmput+67
    exit_mm+223
    do_exit+512
    do_group_exit+140
    __x64_sys_exit_group+23
    __pfx_ia32_emulation_override_cmdline+0
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 2
@[
    folio_mark_dirty+5
    unmap_page_range+4535
    unmap_vmas+241
    unmap_region+338
    do_vmi_align_munmap+946
    do_vmi_munmap+263
    __vm_munmap+199
    __x64_sys_munmap+27
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 3
@[
    folio_mark_dirty+5
    shmem_swapin_folio+1047
    shmem_get_folio_gfp+151
    shmem_fault+134
    __do_fault+67
    do_pte_missing+354
    handle_mm_fault+2183
    do_user_addr_fault+504
    exc_page_fault+117
    asm_exc_page_fault+38
]: 4
@[
    folio_mark_dirty+5
    fault_dirty_shared_page+74
    do_pte_missing+1014
    handle_mm_fault+2183
    do_user_addr_fault+780
    exc_page_fault+117
    asm_exc_page_fault+38
]: 5
@[
    folio_mark_dirty+5 // .. swap 进来，那么立刻标记下
    shmem_swapin_folio+1047
    shmem_get_folio_gfp+151
    shmem_fault+134
    __do_fault+67
    do_pte_missing+831
    handle_mm_fault+2183
    do_user_addr_fault+504
    exc_page_fault+117
    asm_exc_page_fault+38
]: 7
@[
    folio_mark_dirty+5
    unmap_page_range+2762
    unmap_vmas+241
    exit_mmap+498
    __mmput+67
    exit_mm+223
    do_exit+290
    do_group_exit+140
    __x64_sys_exit_group+23
    __pfx_ia32_emulation_override_cmdline+0
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 8
@[
    folio_mark_dirty+5
    unmap_page_range+2762
    unmap_vmas+241
    exit_mmap+498
    __mmput+67
    exit_mm+223
    do_exit+512
    do_group_exit+120
    get_signal+1710
    arch_do_signal_or_restart+142
    irqentry_exit_to_user_mode+75
    asm_sysvec_reschedule_ipi+26
]: 16
@[
    folio_mark_dirty+5
    shmem_write_end+367
    generic_perform_write+425
    shmem_file_write_iter+107
    vfs_write+935
    ksys_write+114
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 18
@[
    folio_mark_dirty+5
    unmap_page_range+2762
    unmap_vmas+241
    exit_mmap+498
    __mmput+67
    exit_mm+223
    do_exit+512
    vhost_task_fn+285
    ret_from_fork+55
    ret_from_fork_asm+26
]: 31
@[
    folio_mark_dirty+5
    unmap_page_range+2762
    unmap_vmas+241
    unmap_region+338
    do_vmi_align_munmap+946
    do_vmi_munmap+263
    __vm_munmap+199
    __x64_sys_munmap+27
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 37
@[
    folio_mark_dirty+5
    bio_set_pages_dirty+224
    iomap_dio_bio_iter+786
    __iomap_dio_rw+680
    iomap_dio_rw+18
    ext4_file_read_iter+234
    vfs_read+750
    __x64_sys_pread64+109
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 37
@[
    folio_mark_dirty+5
    fault_dirty_shared_page+74
    do_pte_missing+1014
    handle_mm_fault+2183
    do_user_addr_fault+504
    exc_page_fault+117
    asm_exc_page_fault+38
]: 49
@[
    folio_mark_dirty+5
    unmap_page_range+2762
    unmap_vmas+241
    exit_mmap+498
    __mmput+67
    exit_mm+223
    do_exit+512
    do_group_exit+120
    get_signal+1710
    arch_do_signal_or_restart+142
    syscall_exit_to_user_mode+85
    do_syscall_64+250
    entry_SYSCALL_64_after_hwframe+119
]: 82
@[
    folio_mark_dirty+5
    block_page_mkwrite+344
    ext4_page_mkwrite+851
    do_wp_page+2066
    handle_mm_fault+2657
    do_user_addr_fault+504
    exc_page_fault+117
    asm_exc_page_fault+38
]: 132
@[
    folio_mark_dirty+5
    fault_dirty_shared_page+74
    do_wp_page+2124
    handle_mm_fault+2657
    do_user_addr_fault+504
    exc_page_fault+117
    asm_exc_page_fault+38
]: 133
@[
    folio_mark_dirty+5
    try_to_migrate_one+906
    rmap_walk_anon+319
    try_to_migrate+214
    migrate_pages_batch+944
    migrate_pages+1911
    compact_zone+3555
    compact_node+195
    kcompactd+1397
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 1945
@[
    folio_mark_dirty+5 // 都 dio ，为什么还需要 mark ? 看看 bio_set_pages_dirty 的 comment
    bio_set_pages_dirty+224
    iomap_dio_bio_iter+786
    __iomap_dio_rw+680
    iomap_dio_rw+18
    ext4_file_read_iter+234
    aio_read+456
    io_submit_one+1608
    __se_sys_io_submit+189
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 11920
```

### 从 pte dirty 到 folio dirty 的传播
几乎都发生到 rmap 中，非常合理
```c
		/* Set the dirty flag on the folio now the pte is gone. */
		if (pte_dirty(pteval))
			folio_mark_dirty(folio);
```

### PG_dirty 的作用

案例 1 : 从 block_dirty_folio 中可以看到，检查了有这个 flag ，如果是 newly dirty ，那么才去
到 address space 中去标记这些东西:
```c
	if (newly_dirty)
		__folio_mark_dirty(folio, mapping, 1);
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
