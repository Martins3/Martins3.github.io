# Page Migration

- Documentation/vm/page_migration.rst

- https://man7.org/linux/man-pages/man2/migrate_pages.2.html

- [ ] 搜索一下 migration ，还是存在很多类似的积累的

- [ ] 似乎 ARM 和 大页都会影响 migration 的实现。


- move_to_new_folio
  - migrate_folio : 如果是 anon 映射
  - mapping->a_ops->migrate_folio : 取决于文件系统
  - fallback_migrate_folio : 如果文件系统没有注册

## 测试工具
> migratepages pid from-nodes to-nodes
>
> https://man7.org/linux/man-pages/man8/migratepages.8.html

## 两个 syscall 对比 migrate_pages move_pages
- migrate_pages : 在 mempolicy.c 中
- move_pages : 在 migrate.c 中 ，只是粒度更加细

使用 migrate pages 来计算得到的:
```txt
#0  migrate_pages (from=from@entry=0xffffc90001a8be08, get_new_page=0xffffffff813315c0 <alloc_migration_target>, put_new_page=put_new_page@entry=0x0 <fixed_percpu_data>, private=private@entry=18446683600597859864, mode=mode@entry=MIGRATE_SYNC, reason=reason@entry=3, ret_succeeded=0x0 <fixed_percpu_data>) at mm/migrate.c:1417
#1  0xffffffff8131e445 in migrate_to_node (mm=mm@entry=0xffff8881620a1100, source=source@entry=0, dest=dest@entry=1, flags=flags@entry=4) at mm/mempolicy.c:1087
#2  0xffffffff8131f554 in do_migrate_pages (mm=mm@entry=0xffff8881620a1100, from=from@entry=0xffffc90001a8bf00, to=to@entry=0xffffc90001a8bf08, flags=4) at mm/mempolicy.c:1186
#3  0xffffffff8131f894 in kernel_migrate_pages (pid=<optimized out>, maxnode=<optimized out>, old_nodes=<optimized out>, new_nodes=<optimized out>) at mm/mempolicy.c:1663
#4  0xffffffff8131f934 in __do_sys_migrate_pages (new_nodes=<optimized out>, old_nodes=<optimized out>, maxnode=<optimized out>, pid=<optimized out>) at mm/mempolicy.c:1682
#5  __se_sys_migrate_pages (new_nodes=<optimized out>, old_nodes=<optimized out>, maxnode=<optimized out>, pid=<optimized out>) at mm/mempolicy.c:1678
#6  __x64_sys_migrate_pages (regs=<optimized out>) at mm/mempolicy.c:1678
#7  0xffffffff81fa4bcb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001a8bf58) at arch/x86/entry/common.c:50
#8  do_syscall_64 (regs=0xffffc90001a8bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#9  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:12
```

## 记录一下 syscall 相关的内容

```txt
#0  migrate_pages (from=from@entry=0xffffc9000179fd88, get_new_page=get_new_page@entry=0xffffffff81331550 <alloc_misplaced_dst_page>, put_new_page=put_new_page@entry=0x0 <fixed_percpu_data>, private=private@entry=1, mode=mode@entry=MIGRATE_ASYNC, reason=reason@entry=5, ret_succeeded=0xffffc9000179fd84) at mm/migrate.c:1417
#1  0xffffffff81334631 in migrate_misplaced_page (page=page@entry=0xffffea0005a3f440, vma=vma@entry=0xffff88816549fbe0, node=node@entry=1) at mm/migrate.c:2193
#2  0xffffffff812dcf5a in do_numa_page (vmf=0xffffc9000179fdf8) at mm/memory.c:4783
#3  handle_pte_fault (vmf=0xffffc9000179fdf8) at mm/memory.c:4962
#4  __handle_mm_fault (vma=vma@entry=0xffff88816549fbe0, address=address@entry=140728201447760, flags=flags@entry=596) at mm/memory.c:5097
#5  0xffffffff812dd620 in handle_mm_fault (vma=0xffff88816549fbe0, address=address@entry=140728201447760, flags=flags@entry=596, regs=regs@entry=0xffffc9000179ff58) at mm/memory.c:5218
#6  0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc9000179ff58, error_code=error_code@entry=4, address=address@entry=140728201447760) at arch/x86/mm/fault.c:1428
#7  0xffffffff81fa8e02 in handle_page_fault (address=140728201447760, error_code=4, regs=0xffffc9000179ff58) at arch/x86/mm/fault.c:1519
#8  exc_page_fault (regs=0xffffc9000179ff58, error_code=4) at arch/x86/mm/fault.c:1575
#9  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

## migrate
- [ ] make_migration_entry()， 看来 migrate 的类型甚至可以出现在 pte 上，看来 migrate 不是简单的复制粘贴了

1. migrate 并不是为了实现 numa 而设计的，其实在 numa 节点之间迁移并没有什么难度，
虽然 numa 系统在访问速度上存在区别，但是寻址空间都是同一个，
所以完成迁移的工作只是拷贝而已。
2. 为了用户可以控制分配的内存，系统调用 move_pages(2) 和 migrate_pages
3. 在 migrate 实现 compaction 的基础

内核文档讲解的很清晰 : [^11]

migrate 什么类型的 page ?
1. 如果这个 page 是被内核数据，比如 page cache，inode cache 之类的 ? 应该没有办法 migrate 吧 ?
    1. 这不是透明，但是 page cache 之类的不能迁移还是太浪费了，但是需要 address_space_operations::migratepage 和 address_space_operations::isolate_page 辅助

看似只是拷贝，但是为什么写了好几千行
1. 解除一个 page 的联系，并且重新建立。
    1. page 可能在 TLB 中间，应该需要 invalid 特定地址上的 tlb
    2. page table 需要修改
2. hugepage 如何迁移 (似乎)[^10]


核心函数 A : migrate_pages 被 compaction 使用:
```c
/*
 * migrate_pages - migrate the pages specified in a list, to the free pages
 *       supplied as the target for the page migration
 *
 * @from:   The list of pages to be migrated.
 * @get_new_page: The function used to allocate free pages to be used
 *      as the target of the page migration.
 * @put_new_page: The function used to free target pages if migration
 *      fails, or NULL if no special handling is necessary.
 * @private:    Private data to be passed on to get_new_page()
 * @mode:   The migration mode that specifies the constraints for
 *      page migration, if any.
 * @reason:   The reason for page migration.
 *
 * The function returns after 10 attempts or if no pages are movable any more
 * because the list has become empty or no retryable pages exist any more.
 * The caller should call putback_movable_pages() to return pages to the LRU
 * or free list only if ret != 0.
 *
 * Returns the number of pages that were not migrated, or an error code.
 */
int migrate_pages(struct list_head *from, new_page_t get_new_page,
    free_page_t put_new_page, unsigned long private,
    enum migrate_mode mode, int reason)
// 调用 : unmap_and_move_huge_page 或者 unmap_and_move 维持一下生活
```
其实可以对于函数调用进行

对于实现 isolate 的猜测:
1. 按照 pageblock 的单位进行标记: 系统初始化的时候，其中的内容早就标记好了
2. 其他的 pageblock 根据 alloc_page 的 flags 确定。
> 不知道是否会选择合适的 pageblock 进行

```c
// 几乎是唯一初始化 ac->migratetype 的地方
// 另一个在 unreserve_highatomic_pageblock
static inline int gfpflags_to_migratetype(const gfp_t gfp_flags) {
  return (gfp_flags & GFP_MOVABLE_MASK) >> GFP_MOVABLE_SHIFT;
}
```

1. migrate 从哪里迁移到哪里? 确定谁需要被迁移 ! 或者说，都是谁发起的

## 虚拟机中启动 qemu ，结果观测到如下的结果
```txt
@[
    kvm_flush_tlb_multi+5
    flush_tlb_mm_range+287
    pmdp_invalidate+156
    set_pmd_migration_entry+100
    try_to_migrate_one+536
    rmap_walk_anon+319
    try_to_migrate+234
    migrate_pages_batch+926
    migrate_pages+1771
    migrate_misplaced_folio+115
    do_huge_pmd_numa_page+888
    handle_mm_fault+1671
    __get_user_pages+1130
    get_user_pages_unlocked+265
    hva_to_pfn+267
    kvm_faultin_pfn+481
    kvm_tdp_page_fault+186
    kvm_mmu_do_page_fault+378
    kvm_mmu_page_fault+212
    vmx_handle_exit+1282
    kvm_arch_vcpu_ioctl_run+6873
    kvm_vcpu_ioctl+1537
    __se_sys_ioctl+115
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 208
```

[^10]: [stackoverflow : Using move_pages() to move hugepages?](https://stackoverflow.com/questions/59726288/using-move-pages-to-move-hugepages)
[^11]: [kernel doc : page migratin](https://www.kernel.org/doc/html/latest/vm/page_migration.html)

## 一个有趣的 backtrace
```txt
- 5.16% npf_interception
   - 5.14% kvm_mmu_page_fault
      - 5.08% kvm_mmu_do_page_fault
         - 3.70% kvm_tdp_page_fault
            - 2.97% kvm_mmu_faultin_pfn
               - 2.88% __kvm_faultin_pfn
                  - 2.87% hva_to_pfn
                     - 2.71% get_user_pages_unlocked
                        - 2.61% __get_user_pages
                           - 2.40% handle_mm_fault
                              - 2.34% __handle_mm_fault
                                 - 2.08% migrate_misplaced_folio
                                    - 2.04% migrate_pages
                                       - 2.03% migrate_pages_batch
                                          - 1.58% try_to_migrate
                                             - 1.57% rmap_walk_anon
                                                - 1.53% try_to_migrate_one
                                                   - 1.32% __mmu_notifier_invalidate_range_start
                                                      - 1.31% kvm_mmu_notifier_invalidate_range_start
                                                           0.76% gfn_to_pfn_cache_invalidate_start
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
