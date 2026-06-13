## hugepage
- [x] 如果 guest 使用 huge page, ept 可以感知到吗 ?
- [ ] 如果 guest 使用 shadow page table, 并且使用 huge page tabel, 如何办 ?
  - [ ] 直接禁止使用
  - [ ] host 建立对应的 huge page table
    - 如果 guest 建议 huge page table，但是 host 根本没有这么多空间，如何 ?

- [ ] 不考虑 kvm 虚拟化，为了支持 THP 需要实现什么东西 ?
  - TLB 设置标志位，告诉一个一个 2M 的虚拟地址空间都是可以翻译
  - page walk 机制，当检测到是 page entry THP 的时候，可以得到物理地址了

使用 THP 的时候，Guest 的 page walk 可以少走一次，可以直接导致 ept 的 page walk 可以少走一次
Host 使用 page walk，可以让 ept 少走一次

## 无穷嵌套


如果 L1 就没有使用 ept ，L1 中如何继续维持 L2 的运行?

## 在 l2 中启动虚拟机还是支持 ept 的

那么 L3 虚拟机中的 kvm 呢?

## 如何判断是 nGPA -> GPA 还是 GPA -> HPA 的差别?

(这段记录似乎有点问题)

SDM 中的 Table 28-7. Exit Qualification for EPT Violations

```c
#define EPT_VIOLATION_GVA_IS_VALID_BIT	7
```

Set if the guest linear-address field is valid.
The guest linear-address field is valid for all EPT violations except those resulting from an attempt to load the
guest PDPTEs as part of the execution of the MOV CR instruction and those due to trace-address pre-translation
(TAPT; Section 26.5.4).

```c
#define EPT_VIOLATION_GVA_TRANSLATED_BIT 8
```

If bit 7 is 1:
• Set if the access causing the EPT violation is to a guest-physical address that
is the translation of a linear address.
• Clear if the access causing the EPT violation is to a paging-structure entry as
part of a page walk or the update of an accessed or dirty bit. Reserved if bit 7
is 0 (cleared to 0)

SDM `26.5.3 Translation of Guest-Physical Addresses Using EPT`

所以，现在很简单了，通过这个判断就可以了:
```c
#define EPT_VIOLATION_GVA_IS_VALID	(1 << EPT_VIOLATION_GVA_IS_VALID_BIT)
#define EPT_VIOLATION_GVA_TRANSLATED	(1 << EPT_VIOLATION_GVA_TRANSLATED_BIT)
```

核心判断在:

handle_ept_violation
```c
	if (error_code & EPT_VIOLATION_GVA_IS_VALID)
		error_code |= (exit_qualification & EPT_VIOLATION_GVA_TRANSLATED) ?
			      PFERR_GUEST_FINAL_MASK : PFERR_GUEST_PAGE_MASK;
```

- kvm_fixup_and_inject_pf_error : 没有调用
- kvm_inject_emulated_page_fault

```txt
@[
    kvm_inject_emulated_page_fault+5
    ept_page_fault+499
    kvm_mmu_do_page_fault+275
    kvm_mmu_page_fault+130
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 94749
```

## SDM 关于嵌套虚拟化的章节
<!-- 5e26c00c-41b3-4eb3-a955-a920b0954d6f -->

CHAPTER 26 VMX NON-ROOT OPERATION

### [Nested EPT to Make Nested VMX Faster](https://www.linux-kvm.org/images/8/8c/Kvm-forum-2013-nested-ept.pdf)
- [x] 嵌套虚拟化必须使用 spte 吗？
  - 是的

> - shadow paging code is a template
> - All differences are template parameters
> - Template code is compiled for each paging mode
> - `vcpu->mmu` is initialized according to current guest mode


- 这里有一个考虑，到底是将 nGVA 到 GPA 设置为 shadow page table 还是 GVA HPA 的设置为 shadow page table
  - 如果使用第一种方法，此时 host 对于 L1 Guest 使用了虚拟化完全不知道，L1 Guest 来使用 spte 来处理
     - L0 对于 L1

- 但是如何热迁移?
  - shadow page table : 是从 GVA 到 HPA 中映射的压缩，GVA 到 HPA 因为持有 vmcs 中，然后软件来 page walk 来找到的

## 似乎 amd 的嵌套虚拟化好卡啊

L1 : 2048G 128core

L2 如果两个 core 还可以，如果是 12 core ，根本无法开机:
```txt
   - 51.56% __GI___ioctl
        entry_SYSCALL_64_after_hwframe
        do_syscall_64
        __x64_sys_ioctl
        kvm_vcpu_ioctl
      - kvm_arch_vcpu_ioctl_run
         - 26.94% kvm_mmu_sync_roots
            - 26.93% queued_write_lock_slowpath
                 native_queued_spin_lock_slowpath
         - 24.62% npf_interception
            - kvm_mmu_page_fault
               - 24.61% kvm_mmu_do_page_fault
                  - 14.67% kvm_tdp_page_fault
                     - 12.62% queued_read_lock_slowpath
                          12.62% native_queued_spin_lock_slowpath
                     - 2.04% kvm_mmu_faultin_pfn
                          __kvm_faultin_pfn
                          hva_to_pfn
                          get_user_pages_unlocked
                        - __get_user_pages
                           - 2.04% handle_mm_fault
                              - __handle_mm_fault
                                 - 2.04% migrate_misplaced_folio
                                      migrate_pages
                                      migrate_pages_batch
                                      try_to_migrate
                                      rmap_walk_anon
                                    - try_to_migrate_one
                                       - 1.52% __mmu_notifier_invalidate_range_start
                                          - kvm_mmu_notifier_invalidate_range_start
                                             - 1.47% queued_write_lock_slowpath
```

看上去主要是在 kvm_mmu_sync_roots 中
```txt
-   27.82%     4.47%  qemu-system-x86  [kernel.vmlinux]
   - 23.34% queued_write_lock_slowpath
        23.27% native_queued_spin_lock_slowpath
   - 4.47% __GI___ioctl
        entry_SYSCALL_64_after_hwframe
        do_syscall_64
        __x64_sys_ioctl
        kvm_vcpu_ioctl
      - kvm_arch_vcpu_ioctl_run
         - 4.39% kvm_mmu_sync_roots
              queued_write_lock_slowpath
```

开启几个大规格虚拟机，可以看到 kvm_mmu_sync_roots 调用次数很多，
虚拟机启动会卡到构建 struct page 数组的地方(猜测)
```txt
FUNC                                    COUNT
kvm_mmu_sync_roots                     600256
```

但是 kvm_mmu_sync_roots 是常规路径

来源是:
```txt
@[
    nested_svm_transition_tlb_flush+1
    nested_svm_vmexit+1333
    svm_check_nested_events+205
    kvm_arch_vcpu_ioctl_run+4853
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+739
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 224
@[
    nested_svm_transition_tlb_flush+1
    nested_svm_vmexit+1333
    paging64_page_fault+487
    kvm_mmu_do_page_fault+279
    kvm_mmu_page_fault+130
    npf_interception+143
    kvm_arch_vcpu_ioctl_run+2045
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+739
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 312
@[
    nested_svm_transition_tlb_flush+1
    nested_svm_vmexit+1333
    nested_svm_exit_handled+84
    svm_handle_exit+343
    kvm_arch_vcpu_ioctl_run+2045
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+739
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 17542
@[
    nested_svm_transition_tlb_flush+1
    nested_vmcb02_prepare_control+48
    enter_svm_guest_mode+217
    nested_svm_vmrun+654
    kvm_arch_vcpu_ioctl_run+2045
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+739
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 18078
```

原来这个地方一直都是需要修复一下的代码:

```c
static void nested_svm_transition_tlb_flush(struct kvm_vcpu *vcpu)
{
	/* Handle pending Hyper-V TLB flush requests */
	kvm_hv_nested_transtion_tlb_flush(vcpu, npt_enabled);

	/*
	 * TODO: optimize unconditional TLB flush/MMU sync.  A partial list of
	 * things to fix before this can be conditional:
	 *
	 *  - Flush TLBs for both L1 and L2 remote TLB flush
	 *  - Honor L1's request to flush an ASID on nested VMRUN
	 *  - Sync nested NPT MMU on VMRUN that flushes L2's ASID[*]
	 *  - Don't crush a pending TLB flush in vmcb02 on nested VMRUN
	 *  - Flush L1's ASID on KVM_REQ_TLB_FLUSH_GUEST
	 *
	 * [*] Unlike nested EPT, SVM's ASID management can invalidate nested
	 *     NPT guest-physical mappings on VMRUN.
	 */
	kvm_make_request(KVM_REQ_MMU_SYNC, vcpu);
	kvm_make_request(KVM_REQ_TLB_FLUSH_CURRENT, vcpu);
}
```

## ZONE_DEVICE

```c
bool kvm_is_reserved_pfn(kvm_pfn_t pfn)
{
	/*
	 * ZONE_DEVICE pages currently set PG_reserved, but from a refcounting
	 * perspective they are "normal" pages, albeit with slightly different
	 * usage rules.
	 */
	if (pfn_valid(pfn))
		return PageReserved(pfn_to_page(pfn)) &&
		       !is_zero_pfn(pfn) &&
		       !kvm_is_zone_device_pfn(pfn);

	return true;
}
```

## pae 在这里到底是个什么概念

在 `__kvm_mmu_create` 中存在
```c
	mmu->pae_root = page_address(page);
```

## 看看蚂蚁的 2023 sosp 上的文章，利用 shadow page table 来实现 secure container

## 既然发现每一级都是可以使用 ept 的，那么可以将 L1 的 ept disbale 掉
似乎 kvm 也是存在参数的来 disable ept 的。

## 看看 tdp mmu 下， struct kvm_mmu_page 是如何分配的?

## kvm mmu 似乎就要拿下了

即将就绪:
- docs/kvm/mmu/page_track.md
- docs/kvm/mmu/mmu.rst.md
- docs/kvm/mmu/async-pf.md
- docs/kvm/mmu/ad.md
- docs/kvm/mmu/tdp_mmu.md
- docs/kvm/mmu/shadow-page.md
- docs/kvm/mmu/nested.md
- docs/kvm/mmu/mmu.md : 全是垃圾

## 经典函数了
看看这个如何运用了:
```c
static inline unsigned long nested_ept_get_eptp(struct kvm_vcpu *vcpu)
{
	/* return the page table to be shadowed - in our case, EPT12 */
	return get_vmcs12(vcpu)->ept_pointer;
}
```

## [ ] shadow page table 需要给每一个 guest process 建立一个 mmu root 吧
不是，但是缓存机制也是非常诡异的

一共缓存四个，然后呢?

## what
https://luohao-brian.gitbooks.io/interrupt-virtualization/content/kvmzhi-nei-cun-xu-ni531628-kvm-mmu-virtualization.html

获得缺页异常发生时的 CR2,及当时访问的虚拟地址；
进入
```plain
kvm_mmu_page_fault()(vmx.c)->
r = vcpu->arch.mmu.page_fault(vcpu, cr2, error_code);(mmu.c)->
FNAME(page_fault)(struct kvm_vcpu *vcpu, gva_t addr, u32 error_code)(paging_tmpl.h)->
FNAME(walk_addr)()
```
查 guest 页表，物理地址是否存在， 这时肯定是不存在的
The page is not mapped by the guest. Let the guest handle it.
`inject_page_fault()->kvm_inject_page_fault()` 异常注入流程；

> 只要是 mmu 中间访问失败都是需要进行 vm exit 的，如果发现是 guest 的问题，那么通知 guest
> TODO 找到对于 guest 的 page table 进行 walk 的方法
> Guest 搞定之后，那么
> TODO TLB 的查找不到，被 VMM 截获应该是需要 硬件支持的吧!

为了快速检索 GUEST 页表所对应的的影子页表，KVM 为每个 GUEST 都维护了一个哈希
表，影子页表和 GUEST 页表通过此哈希表进行映射。对于每一个 GUEST 来说，GUEST
的页目录和页表都有唯一的 GUEST 物理地址，通过页目录/页表的客户机物理地址就
可以在哈希链表中快速地找到对应的影子页目录/页表。
> 显然不可能使用保存所有的物理地址，从虚拟机只会将虚拟机使用的物理地址处理掉

> 填充过程

mmu_alloc_root =>
`__direct_map` => kvm_mmu_get_page =>


感觉这里还是 shadow 的处理机制，那么 ept 在哪里 ?
```c
static int __direct_map(struct kvm_vcpu *vcpu, gpa_t gpa, int write,
            int map_writable, int max_level, kvm_pfn_t pfn,
            bool prefault, bool account_disallowed_nx_lpage)
{
  // TODO 是在对于谁进行 walk ? 应该不是是对于 shadow page 进行的
  // shadow page 也是划分为 leaf 和 nonleaf 的，也就是这是对于 shadow 的
  //
  // shadow page 形成一个层次结构的目的是什么 ?
    struct kvm_shadow_walk_iterator it;
    struct kvm_mmu_page *sp;
    int level, ret;
    gfn_t gfn = gpa >> PAGE_SHIFT;
    gfn_t base_gfn = gfn;

    if (WARN_ON(!VALID_PAGE(vcpu->arch.mmu->root_hpa)))
        return RET_PF_RETRY;

  // TODO level generation 的含义
  // level : 难道 shadow page table 也是需要多个 level
    level = kvm_mmu_hugepage_adjust(vcpu, gfn, max_level, &pfn);

    for_each_shadow_entry(vcpu, gpa, it) {
        /*
         * We cannot overwrite existing page tables with an NX
         * large page, as the leaf could be executable.
         */
        disallowed_hugepage_adjust(it, gfn, &pfn, &level);

        base_gfn = gfn & ~(KVM_PAGES_PER_HPAGE(it.level) - 1);
        if (it.level == level)
            break;

        drop_large_spte(vcpu, it.sptep);
        if (!is_shadow_present_pte(*it.sptep)) {
            sp = kvm_mmu_get_page(vcpu, base_gfn, it.addr,
                          it.level - 1, true, ACC_ALL);

            link_shadow_page(vcpu, it.sptep, sp);
            if (account_disallowed_nx_lpage)
                account_huge_nx_page(vcpu->kvm, sp);
        }
    }

    ret = mmu_set_spte(vcpu, it.sptep, ACC_ALL,
               write, level, base_gfn, pfn, prefault,
               map_writable);
    direct_pte_prefetch(vcpu, it.sptep);
    ++vcpu->stat.pf_fixed;
    return ret;
}
```
==> kvm_mmu_get_page : 应该修改为 get_shadow_page
==> kvm_page_table_hashfn : 利用 gfn 作为 hash 快速定位 shadow_page
==> kvm_mmu_alloc_page : 分配并且初始化一个 shadow page table

注意 : shadow page table 似乎可以存放 shadow page table entry 的

**TODO** 调查 kvm_mmu_alloc_page 的创建的 kvm_mmu_page 的管理内容, 似乎 rule 说明了很多东西

The hypervisor computes the guest virtual to
host physical mapping on the fly and stores it in
a new set of page tables

https://www.linux-kvm.org/images/e/e5/KvmForum2007%24shadowy-depths-of-the-kvm-mmu.pdfhttps://www.linux-kvm.org/images/e/e5/KvmForum2007%24shadowy-depths-of-the-kvm-mmu.pdf

emmmm : 一个物理页面，在 host 看来是给 host 使用的，write protect  可以在 guest 中间，
也是可以放在 host 中间。

emmmm : 什么情况下，一个 hva 可以被多个 gpa 映射 ?

对于 guest 的那些 page table，需要通过 `page->private` 关联起来.

- When we shadow a guest page, we iterate over
the reverse map and remove write access

- When adding write permission to a page, we
check whether the page has a shadow

- **We can have multiple shadow pages for a
single guest page – one for each role**

#### shadow page descriptor
TODO : shadow page table 在 TLB miss 的时候，触发 exception 吗 ?

- [x] 既然 hash table 可以查询，为什么还要建立 hierarchy 的 shadow page table ?
- [x] hash page table 中间放置所有的从 gva 到 hpa 的地址 ?

- 建立 hash 是为了让 guest 的 page table 和 host 的 shadow page table 之间可以快速查找.
- shadow page table : gva 到 hpa 的映射，这个映射是一个 tree 的结构

## shadow page table
- [ ] shadow page table 是放在 qemu 的空间中间，还是内核地址空间
  - guest 通过 cr3 可以来访问
  - 内核可以操控 page table
- [ ] guest 的内核 vmalloc 修改 page table，是首先修改 shadow page table 造成的异常，然后之后才修改 guest page table ?
    - [ ] shadow page table 各个级别存放的地址是什么 ? 物理地址，因为是让 cr3 使用的
    - [x] guest page table 的内容 ? GVA 也就是 host 的虚拟地址
- [x] `FNAME(walk_addr)()` 存储的地址都是 guest 的虚拟地址 ? 是的，所以应该很容易 walk.

> FNAME(walk_addr)() 查 guest 页表，物理地址是否存在，这时肯定是不存在的
`inject_page_fault()->kvm_inject_page_fault()` 异常注入流程；

在 Host 中间检查发现不存在，然后在使用 inject pg 到 guest.
因为 guest page table 存在多个模型

让 Host 越俎代庖来走一遍 guest 的 page walk，shadow page table 是 CR3 中间实际使用的 page table.
-> 使用 spt ，出现 exception 是不知道到底哪一个层次出现问题的, 所以都是需要抛出来检查的
-> *那么当 guest 通过 cr3 进行修改 shadow page table 的时候，通过 write protection 可以找到 ?*
-> *好像 shadow page 只能存放 512 个 page table entry,  利用 cr3 访问真的没有问题吗 ?*

> 影子页表又是载入到 CR3 中真正为物理 MMU 所利用进行寻址的页表，因此开始时任何的内存访问操作都会引起缺页异常；导致 vm 发生 VM Exit；进入 handle_exception();


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
