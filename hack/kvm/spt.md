## 问题
1. 为什么 is_tdp_mmu 的计算如此复杂，不应该是一个 macro 来实现吗?

```c
static inline bool is_tdp_mmu(struct kvm_mmu *mmu)
{
	struct kvm_mmu_page *sp;
	hpa_t hpa = mmu->root.hpa;

	if (WARN_ON(!VALID_PAGE(hpa)))
		return false;

	/*
	 * A NULL shadow page is legal when shadowing a non-paging guest with
	 * PAE paging, as the MMU will be direct with root_hpa pointing at the
	 * pae_root page, not a shadow page.
	 */
	sp = to_shadow_page(hpa);
	return sp && is_tdp_mmu_page(sp) && sp->root_count;
}
```
2. paging_tmpl.h 只是和 page table 相关吧

#### `__direct_map`
1. for_each_shadow_entry : 因为多个 shadow page 映射一个 page table

#### https://luohao-brian.gitbooks.io/interrupt-virtualization/content/kvmzhi-nei-cun-xu-ni531628-kvm-mmu-virtualization.html

获得缺页异常发生时的CR2,及当时访问的虚拟地址；
进入
```
kvm_mmu_page_fault()(vmx.c)->
r = vcpu->arch.mmu.page_fault(vcpu, cr2, error_code);(mmu.c)->
FNAME(page_fault)(struct kvm_vcpu *vcpu, gva_t addr, u32 error_code)(paging_tmpl.h)->
FNAME(walk_addr)()
```
查guest页表，物理地址是否存在， 这时肯定是不存在的
The page is not mapped by the guest. Let the guest handle it.
`inject_page_fault()->kvm_inject_page_fault()` 异常注入流程；

> 只要是 mmu 中间访问失败都是需要进行 vm exit 的，如果发现是 guest 的问题，那么通知 guest
> TODO 找到对于 guest 的 page table 进行 walk 的方法
> Guest 搞定之后，那么
> TODO TLB 的查找不到，被 VMM 截获应该是需要 硬件支持的吧!

为了快速检索GUEST页表所对应的的影子页表，KVM 为每个GUEST都维护了一个哈希
表，影子页表和GUEST页表通过此哈希表进行映射。对于每一个GUEST来说，GUEST
的页目录和页表都有唯一的GUEST物理地址，通过页目录/页表的客户机物理地址就
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


## sync shadow page
1. 利用 generation 来实现定位 ?

```c
static bool is_obsolete_sp(struct kvm *kvm, struct kvm_mmu_page *sp)
{
    return sp->role.invalid ||
           unlikely(sp->mmu_valid_gen != kvm->arch.mmu_valid_gen);
}
```


## paging_tmpl.h

We need the mmu code to access both 32-bit and 64-bit guest ptes,
so the code in this file is compiled twice, once per pte size.

- [x] 如何实现多次编译 ? 目的应该是提供三种不同编译属性的文件，其中只是少量偏移量的修改。通过三次 include 解决.
- [ ] 如果 guest 使用 transparent huge page 的时候，其提供的 page walk 怎么办 ?


```c
static void shadow_mmu_init_context(struct kvm_vcpu *vcpu, struct kvm_mmu *context,
                    u32 cr0, u32 cr4, u32 efer,
                    union kvm_mmu_role new_role)
{
    if (!(cr0 & X86_CR0_PG))
        nonpaging_init_context(vcpu, context);
    else if (efer & EFER_LMA)
        paging64_init_context(vcpu, context);
    else if (cr4 & X86_CR4_PAE)
        paging32E_init_context(vcpu, context);
    else
        paging32_init_context(vcpu, context);

    context->mmu_role.as_u64 = new_role.as_u64;
    reset_shadow_zero_bits_mask(vcpu, context);
}
```
> 都是提供的 shadow 的情况，那么 ept 和 tdp 所以没有出现 ?

## shadow page table
- [ ] shadow page table 是放在 qemu 的空间中间，还是内核地址空间
  - guest 通过 cr3 可以来访问
  - 内核可以操控 page table
- [ ] guest 的内核 vmalloc 修改 page table，是首先修改 shadow page table 造成的异常，然后之后才修改 guest page table ?
    - [ ] shadow page table 各个级别存放的地址是什么 ? 物理地址，因为是让 cr3 使用的
    - [x] guest page table 的内容 ? GVA 也就是 host 的虚拟地址
- [x] `FNAME(walk_addr)()` 存储的地址都是 guest 的虚拟地址 ? 是的，所以应该很容易 walk.

> FNAME(walk_addr)() 查 guest页表，物理地址是否存在，这时肯定是不存在的
`inject_page_fault()->kvm_inject_page_fault()` 异常注入流程；

在 Host 中间检查发现不存在，然后在使用 inject pg 到 guest.
因为 guest page table 存在多个模型

让 Host 越俎代庖来走一遍 guest 的 page walk，shadow page table 是 CR3 中间实际使用的 page table.
-> 使用 spt ，出现 exception 是不知道到底哪一个层次出现问题的, 所以都是需要抛出来检查的
-> *那么当 guest 通过 cr3 进行修改 shadow page table 的时候，通过 write protection 可以找到 ?*
-> *好像 shadow page 只能存放 512 个 page table entry,  利用 cr3 访问真的没有问题吗 ?*

> 影子页表又是载入到CR3中真正为物理MMU所利用进行寻址的页表，因此开始时任何的内存访问操作都会引起缺页异常；导致vm发生VM Exit；进入handle_exception();

## 找到 shadow 以及 ept 的 page table entry


## rmap
https://www.cnblogs.com/ck1020/p/6920765.html

在KVM中，逆向映射机制的作用是类似的，但是完成的却不是从HPA到对应的EPT页表项的定位，
而是从gfn到*对应的页表项*的定位。
*理论上讲根据gfn一步步遍历EPT也未尝不可，但是效率较低*况且在EPT所维护的页面不同于host的页表，*理论上讲是虚拟机之间是禁止主动的共享内存的*，为了提高效率，就有了当前的逆向映射机制。

- rmap: from guest page to shadow ptes that map it
- Shadow hash: from guest page to its shadow
- Parent pte chain: from shaow page to upperlevel shadow page
- Shadow pte: from shadow page to lower-level shadow page
- LRU: all active shadow pages

Walk the shadow page table, instantiating page tables as necessary
- Can involve an rmap walk and *write protecting the guest page table*


```c
struct kvm_arch_memory_slot {
  // 应该是一种 page size 然后提供一种 rmap 吧
    struct kvm_rmap_head *rmap[KVM_NR_PAGE_SIZES];
    struct kvm_lpage_info *lpage_info[KVM_NR_PAGE_SIZES - 1];
    unsigned short *gfn_track[KVM_PAGE_TRACK_MAX];
};

#define KVM_MAX_HUGEPAGE_LEVEL  PG_LEVEL_1G
#define KVM_NR_PAGE_SIZES   (KVM_MAX_HUGEPAGE_LEVEL - PG_LEVEL_4K + 1)

enum pg_level {
    PG_LEVEL_NONE,
    PG_LEVEL_4K,
    PG_LEVEL_2M,
    PG_LEVEL_1G,
    PG_LEVEL_512G,
    PG_LEVEL_NUM
};
```

```c
static int kvm_alloc_memslot_metadata(struct kvm_memory_slot *slot,
                      unsigned long npages)
    // 每一个 page 都会建立一个
        slot->arch.rmap[i] =
            kvcalloc(lpages, sizeof(*slot->arch.rmap[i]),
    // ....
}

// mmu_set_spte 的地方调用
static int rmap_add(struct kvm_vcpu *vcpu, u64 *spte, gfn_t gfn)
{
    struct kvm_mmu_page *sp;
    struct kvm_rmap_head *rmap_head;

  // 通过 pte 的指针，获取 spte 指向的 pte 所在的 page 的
    sp = sptep_to_sp(spte);
  // shadow 和 direct 都是需要 rmap
  // 但是，direct 其实并不会注册
    kvm_mmu_page_set_gfn(sp, spte - sp->spt, gfn);
    rmap_head = gfn_to_rmap(vcpu->kvm, gfn, sp);
    return pte_list_add(vcpu, spte, rmap_head);
}
```

```c
static gfn_t kvm_mmu_page_get_gfn(struct kvm_mmu_page *sp, int index)
{
    if (!sp->role.direct)
        return sp->gfns[index];

  // TODO guest 的物理页面应该就是连续的啊!
  // 当 level 在最底层的时候，sp->gfn + index 就可以了啊!
    return sp->gfn + (index << ((sp->role.level - 1) * PT64_LEVEL_BITS));
}


static struct kvm_rmap_head *gfn_to_rmap(struct kvm *kvm, gfn_t gfn,
                     struct kvm_mmu_page *sp)
{
    struct kvm_memslots *slots;
    struct kvm_memory_slot *slot;

    slots = kvm_memslots_for_spte_role(kvm, sp->role);
    slot = __gfn_to_memslot(slots, gfn);
    return __gfn_to_rmap(gfn, sp->role.level, slot);
}
```


- [ ] 建立反向映射的原因是 : 当 shadow page table 进行修改之后，需要知道其所在的 gfn
  - [ ] 真的存在根据 shadow page table 到 gfn 的需求吗 ?
- [ ] direct 需要 rmap 吗 ? 显然需要，不然 direct_map 不会调用 rmap_add


```c
    kvm_mmu_page_set_gfn(sp, spte - sp->spt, gfn); // 一个 shadow page 和 gfn 的关系
    rmap_head = gfn_to_rmap(vcpu->kvm, gfn, sp);
    return pte_list_add(vcpu, spte, rmap_head); // slot 的每一个 page 都会被 rmap
```

实际上，存在两个 rmap
- `sp->gfns` 获取每一个 pte 对应的 gfn
- `rmap_head->val` = spte : 这不是 rmap 吧

#### parent rmap
```c
static void mmu_page_add_parent_pte(struct kvm_vcpu *vcpu,
                    struct kvm_mmu_page *sp, u64 *parent_pte)
{
    if (!parent_pte)
        return;

    pte_list_add(vcpu, parent_pte, &sp->parent_ptes);
}
```

#### rmap iterator
- [x] rmap 总是构建的 rmap_head 到 sptep 吗 ?
  - rmap_add 和 mmu_page_add_parent_pte 都是的

解析 for_each_rmap_spte
```c
#define for_each_rmap_spte(_rmap_head_, _iter_, _spte_)         \
    for (_spte_ = rmap_get_first(_rmap_head_, _iter_);      \
         _spte_; _spte_ = rmap_get_next(_iter_))
```
使用位置:
kvm_mmu_write_protect_pt_masked : 给定 gfn_offset，将关联的所有的 spte 全部添加 flags

kvm_set_pte_rmapp : 将 rmap_head 的持有的所有的 sptep 进行设置

#### gfn_to_memslot_dirty_bitmap
`slot->dirty_bitmap` 都在 kvm_main 上面访问

pte_prefetch_gfn_to_pfn

- [ ] dirty 指的是 谁 相对于 谁 是 dirty 的

- 最后被 `__direct_map` 调用


## TODO : shadow flood
