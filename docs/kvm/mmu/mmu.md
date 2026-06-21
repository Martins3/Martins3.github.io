# kvm mmu
## TODO
- kvm_mmu_notifier_ops : 重新整理下，似乎 idle page tracking 是和这个有观的，这回导致性能发生多少的下降啊?

kvm_mmu_page_fault

  - mmu_set_spte ??

- `handle_ept_violation`
  - `kvm_mmu_page_fault`
    - `kvm_mmu_do_page_fault` ==> kvm_mmu::page_fault => kvm_tdp_page_fault ==> direct_page_fault ==> `__direct_map` ==> link_shadow_page

- kvm_mmu_do_page_fault

## 这一次，不可以放过
```txt
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
  2)               |  kvm_tdp_page_fault [kvm]() {
  2)   0.122 us    |    kvm_arch_has_noncoherent_dma [kvm]();
  2)   0.225 us    |    kvm_gfn_is_write_tracked [kvm]();
  2)               |    fast_page_fault [kvm]() {
  2)               |      walk_shadow_page_lockless_begin [kvm]() {
  2)   0.062 us    |        __rcu_read_lock();
  2)   0.207 us    |      }
  2)               |      kvm_tdp_mmu_fast_pf_get_last_sptep [kvm]() {
  2)               |        tdp_iter_start [kvm]() {
  2)   0.101 us    |          tdp_iter_restart [kvm]();
  2)   0.353 us    |        }
  2)   0.214 us    |        tdp_iter_next [kvm]();
  2)   0.146 us    |        tdp_iter_next [kvm]();
  2)   0.141 us    |        tdp_iter_next [kvm]();
  2)   0.063 us    |        tdp_iter_next [kvm]();
  2)   1.338 us    |      }
  2)               |      walk_shadow_page_lockless_end [kvm]() {
  2)   0.068 us    |        __rcu_read_unlock();
  2)   0.189 us    |      }
  2)   2.871 us    |    }
  2)               |    mmu_topup_memory_caches [kvm]() {
  2)               |      kvm_mmu_topup_memory_cache [kvm]() {
  2)   0.071 us    |        __kvm_mmu_topup_memory_cache [kvm]();
  2)   0.686 us    |      }
  2)               |      kvm_mmu_topup_memory_cache [kvm]() {
  2)   0.058 us    |        __kvm_mmu_topup_memory_cache [kvm]();
  2)   0.166 us    |      }
  2)               |      kvm_mmu_topup_memory_cache [kvm]() {
  2)   0.057 us    |        __kvm_mmu_topup_memory_cache [kvm]();
  2)   0.165 us    |      }
  2)   1.353 us    |    }
  2)               |    kvm_faultin_pfn [kvm]() {
  2)               |      __gfn_to_pfn_memslot [kvm]() {
  2)               |        hva_to_pfn [kvm]() {
  2)               |          get_user_pages_fast_only() {
  2)   0.072 us    |            is_valid_gup_args();
  2)               |            internal_get_user_pages_fast() {
  2)   0.069 us    |              pud_huge();
  2)   0.233 us    |              try_grab_folio();
  2)   0.634 us    |            }
  2)   0.878 us    |          }
  2)   1.011 us    |        }
  2)   1.145 us    |      }
  2)   1.298 us    |    }
  2)   0.095 us    |    _raw_read_lock();
  2)   0.080 us    |    is_page_fault_stale [kvm]();
  2)               |    kvm_tdp_mmu_map [kvm]() {
  2)               |      kvm_mmu_hugepage_adjust [kvm]() {
  2)   0.204 us    |        __kvm_mmu_max_mapping_level [kvm]();
  2)   0.349 us    |      }
  2)   0.057 us    |      __rcu_read_lock();
  2)               |      tdp_iter_start [kvm]() {
  2)   0.059 us    |        tdp_iter_restart [kvm]();
  2)   0.165 us    |      }
  2)   0.096 us    |      tdp_iter_next [kvm]();
  2)   0.986 us    |      tdp_iter_next [kvm]();
  2)   0.081 us    |      tdp_iter_next [kvm]();
  2)               |      make_spte [kvm]() {
  2)   0.130 us    |        kvm_is_mmio_pfn [kvm]();
  2)               |        vmx_get_mt_mask [kvm_intel]() {
  2)   0.067 us    |          kvm_arch_has_noncoherent_dma [kvm]();
  2)   0.197 us    |        }
  2)               |        mmu_try_to_unsync_pages [kvm]() {
  2)   0.066 us    |          kvm_gfn_is_write_tracked [kvm]();
  2)   0.285 us    |        }
  2)   0.999 us    |      }
  2)   0.756 us    |      handle_changed_spte [kvm]();
  2)   0.067 us    |      __rcu_read_unlock();
  2)   4.573 us    |    }
  2)   0.068 us    |    _raw_read_unlock();
  2)               |    kvm_release_pfn_clean [kvm]() {
  2)   0.110 us    |      kvm_pfn_to_refcounted_page [kvm]();
  2)               |      kvm_release_page_clean [kvm]() {
  2)               |        mark_page_accessed() {
  2)   0.080 us    |          folio_mark_accessed();
  2)   0.190 us    |        }
  2)   0.349 us    |      }
  2)   0.643 us    |    }
  2) + 16.619 us   |  }
```
1. kvmmmu:fast_page_fault 的输出很少
2. kvm_faultin_pfn : 的确，通往 gup
3. kvm_tdp_mmu_map

## TODO 1
- handle_exception_nmi ==> kvm_handle_page_fault ==> ...

## key questions
- [ ] set_spte 是不是仅仅用于设置 leaf
  - [ ] 如果是，其他 level ?
  - [ ] 如果不是，`spte |= (u64)pfn << PAGE_SHIFT;` 直接指向 pfn 如何理解


#### ept page fault

- [ ] where is  paging64_gva_to_gpa ?

```c
static int __direct_map(struct kvm_vcpu *vcpu, gpa_t gpa, int write,
			int map_writable, int max_level, kvm_pfn_t pfn,
			bool prefault, bool account_disallowed_nx_lpage)
{
	struct kvm_shadow_walk_iterator it;
	struct kvm_mmu_page *sp;
	int level, ret;
	gfn_t gfn = gpa >> PAGE_SHIFT;
	gfn_t base_gfn = gfn;

	if (WARN_ON(!VALID_PAGE(vcpu->arch.mmu->root_hpa)))
		return RET_PF_RETRY;

	level = kvm_mmu_hugepage_adjust(vcpu, gfn, max_level, &pfn);

	trace_kvm_mmu_spte_requested(gpa, level, pfn);
	for_each_shadow_entry(vcpu, gpa, it) {
		/*
		 * We cannot overwrite existing page tables with an NX
		 * large page, as the leaf could be executable.
		 */
		disallowed_hugepage_adjust(it, gfn, &pfn, &level);

		base_gfn = gfn & ~(KVM_PAGES_PER_HPAGE(it.level) - 1);
		if (it.level == level)
			break;

    // 可以在 ept page fault 的时候将 ept hugepage 拆分
		drop_large_spte(vcpu, it.sptep);

    // 如果 ept page 不存在
		if (!is_shadow_present_pte(*it.sptep)) {
      // 分配物理页面
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
1. for_each_shadow_entry : 对于 ept 进行 page walk, 利用参数 `gpa` 和 `vcpu->arch.mmu->root_hpa`

## link_shadow_page
难道还有反向映射:
1. mmu_page_add_parent_pte

```c
static bool is_mmio_spte(u64 spte)
{
	return (spte & SPTE_SPECIAL_MASK) == SPTE_MMIO_MASK;
}
```



## mmu_set_spte

pte_access :
1. `__direct_map` : 使用参数 ACC_ALL
2. 其他都是按照 gpte

mmu_set_spte 的参数 spte 是通过 shadow page walk 获取的，
也就是，无论是是什么页面(普通，或者 page table)，
spte 决定该页面是被保护的。注意，guest 想要修改 page table 的时候，还是要经过 shadow page walk，
因此就会被识别出来。

## set_spte

当 pte 存在 ACC_WRITE_MASK 的时候，调用
mmu_need_write_protect 检查，如果 page 被 tracked，那么设置为 RO，否认 unsync



```c
		/*
		 * Optimization: for pte sync, if spte was writable the hash
		 * lookup is unnecessary (and expensive). Write protection
		 * is responsibility of mmu_get_page / kvm_sync_page.
		 * Same reasoning can be applied to dirty page accounting.
		 */
```

## gfn
在 FNAME(fetch) 中间，出现了将 base_pfn 作为 pfn 传递的情况，进而
kvm_mmu_get_page 下面调用链的都是 发生 #PF 的 guest host page table


shadow 和 guest 的 hash 添加映射关系，是因为创建的 shadow page table 总是存在关联的 guest page table 的
此时 gfn 是 page table 地址，在 FNAME(fetch) 上。

FNAME(fetch):
```c
		if (!is_shadow_present_pte(*it.sptep)) {
      // 虽然不知道为什么 -2, 但是可以确定
      // 就是为了将 table 勾连起来
			table_gfn = gw->table_gfn[it.level - 2];
			sp = kvm_mmu_get_page(vcpu, table_gfn, addr, it.level-1,
					      false, access);
		}
```





```diff
 [PATCH] KVM: MMU: Shadow page table caching

 Define a hashtable for caching shadow page tables. Look up the cache on
 context switch (cr3 change) or during page faults.

 The key to the cache is a combination of
 - the guest page table frame number
 - the number of paging levels in the guest
    * we can cache real mode, 32-bit mode, pae, and long mode page
      tables simultaneously.  this is useful for smp bootup.
 - the guest page table table
    * some kernels use a page as both a page table and a page directory.  this
      allows multiple shadow pages to exist for that page, one per level
 - the "quadrant"
    * 32-bit mode page tables span 4MB, whereas a shadow page table spans
      2MB.  similarly, a 32-bit page directory spans 4GB, while a shadow
      page directory spans 1GB.  the quadrant allows caching up to 4 shadow page
      tables for one guest page in one level.
 - a "metaphysical" bit
    * for real mode, and for pse pages, there is no guest page table, so set
      the bit to avoid write protecting the page.

```




## direct_pte_prefetch
- [ ] 只有 direct 才有 prefetch
- [ ] 应该是将连续的 GPA 装配上 ept table 吧


## FNAME(fetch)
- [ ] 为什么需要进行两次 while 循环进行 shadow_walk
  - [ ] 调用 FNAME(fetch) 之前，FNAME(walk_addr) 进行的循环是走到最下面一个级别的，所以，实际上只是单独处理最后一个级别的内容吧!

#### fast page fault
- [ ] 阅读一下 fast_page_fault 中的代码，似乎被 track 的代码会去掉 flags, 而 fast page fault 只是 restore 上 flags 而已


- [ ] shadow_acc_track_mask : access track 是 ad bit 中间对于 a 的模拟吗 ?
    - [ ] restore_acc_track_spte(u64 spte)
    - [ ] page_fault_can_be_fast(u32 error_code)



```c
/*
 * SPTEs used by MMUs without A/D bits are marked with SPTE_AD_DISABLED_MASK;
 * shadow_acc_track_mask is the set of bits to be cleared in non-accessed
 * pages.
 */
static u64 __read_mostly shadow_acc_track_mask;

/*
 * The mask/shift to use for saving the original R/X bits when marking the PTE
 * as not-present for access tracking purposes. We do not save the W bit as the
 * PTEs being access tracked also need to be dirty tracked, so the W bit will be
 * restored only when a write is attempted to the page.
 */
static const u64 shadow_acc_track_saved_bits_mask = PT64_EPT_READABLE_MASK |
						    PT64_EPT_EXECUTABLE_MASK;
static const u64 shadow_acc_track_saved_bits_shift = PT64_SECOND_AVAIL_BITS_SHIFT;

// 和 page_fault_handle_page_track 的关系是什么 ?
static inline bool is_access_track_spte(u64 spte)
{
	return !spte_ad_enabled(spte) && (spte & shadow_acc_track_mask) == 0;
}
```

## PFERR_WRITE_MASK
1. page_fault_can_be_fast 自己就是 error_code 的判断，其中包含的注释:

```c
	/*
	 * #PF can be fast if:
	 * 1. The shadow page table entry is not present, which could mean that
	 *    the fault is potentially caused by access tracking (if enabled).
	 * 2. The shadow page table entry is present and the fault
	 *    is caused by write-protect, that means we just need change the W
	 *    bit of the spte which can be done out of mmu-lock.
	 *
	 * However, if access tracking is disabled we know that a non-present
	 * page must be a genuine page fault where we have to create a new SPTE.
	 * So, if access tracking is disabled, we return true only for write
	 * accesses to a present page.
	 */
	return shadow_acc_track_mask != 0 ||
	       ((error_code & (PFERR_WRITE_MASK | PFERR_PRESENT_MASK))
		== (PFERR_WRITE_MASK | PFERR_PRESENT_MASK));
```
- [ ] spte 不存在是因为 access tracking 机制 ?
- [ ] write-protect 只需要修改一下 W bit of  spte ?
- [ ] if access tracking is disabled we know that a non-present page must be a genuine page fault

```c
/*
 * SPTEs used by MMUs without A/D bits are marked with SPTE_AD_DISABLED_MASK;
 * shadow_acc_track_mask is the set of bits to be cleared in non-accessed
 * pages.
 */
static u64 __read_mostly shadow_acc_track_mask;
```

- [ ] TODO: 查看这一行的 commit messsage, TODO: 调查 ept A bits 是什么
- [ ] acc-track PTE



#### shadow page table 的 dirty bit

他们甚至使用上了 : get_user_page

## invlpg
`kvm_mmu_invalidate_gva` call kvm_mmu::invlpg

- [ ] kvm_mmu_invalidate_gva
  - [ ] caller
    - kvm_inject_emulated_page_fault
    - kvm_mmu_invlpg <- handle_invlpg
  - [ ] kvm_x86_ops.tlb_flush_gva(vcpu, gva); 判断条件和作用
  - [ ] ept_invlgp
    - [ ] kvm_flush_remote_tlbs_with_address
        - [ ] kvm_flush_remote_tlbs_with_range : goes to somewhere unknown
    - [ ] mmu_page_zap_pte : 从 guest 中间发射 invlpg 一般说明 guest 想要更新 page table 了，所以此时将 spt 进行更新一下是不错的


- [ ] kvm_mmu_invlpg
    - [ ] caller 分别在 x86.c 和 vmx.c 中间，用于 emulat 和 vmexit 的处理，所以 emulate 到底是干啥的
    - [ ] 似乎正常情况下，会走到 `FNAME(invlpg)`
        - [ ]


#### rmap
- [ ] 检查一下 rmap_add() 的调用位置


```c
// 获取一个 shadow page table 指针所在的位置
static inline struct kvm_mmu_page *sptep_to_sp(u64 *sptep)
{
	return to_shadow_page(__pa(sptep));
}

static inline struct kvm_mmu_page *to_shadow_page(hpa_t shadow_page)
{
	struct page *page = pfn_to_page(shadow_page >> PAGE_SHIFT);

	return (struct kvm_mmu_page *)page_private(page);
}
```



```c
#define for_each_slot_rmap_range(_slot_, _start_level_, _end_level_,	\
	   _start_gfn, _end_gfn, _iter_)				\
	for (slot_rmap_walk_init(_iter_, _slot_, _start_level_,		\
				 _end_level_, _start_gfn, _end_gfn);	\
	     slot_rmap_walk_okay(_iter_);				\
	     slot_rmap_walk_next(_iter_))
```
- [x] semantic :  对于范围的各种 level 的 page 进行遍历，找到该区域的映射该 page 的 guest page table 所在的页面


```c
#define for_each_valid_sp(_kvm, _sp, _list)				\
	hlist_for_each_entry(_sp, _list, hash_link)			\
		if (is_obsolete_sp((_kvm), (_sp))) {			\
		} else

#define for_each_gfn_indirect_valid_sp(_kvm, _sp, _gfn)			\
	for_each_valid_sp(_kvm, _sp,					\
	  &(_kvm)->arch.mmu_page_hash[kvm_page_table_hashfn(_gfn)])	\
		if ((_sp)->gfn != (_gfn) || (_sp)->role.direct) {} else
```
- 此处使用 hash 遍历的总是 shadow page table，参数 gfn 是一个 guest page table 的物理地址，需要找到 shadow 该 page 的 shadow page table.
- [ ] 为什么 guest page 在 gfn 上，会存在多个 shadow page table 来 shadow
- [ ]


```c
#define for_each_sp(pvec, sp, parents, i)			\
		for (i = mmu_pages_first(&pvec, &parents);	\
			i < pvec.nr && ({ sp = pvec.page[i].sp; 1;});	\
			i = mmu_pages_next(&pvec, &parents, i))

```
- [ ]  理解 parent 机制 ?

kvm_mmu_commit_zap_page : 移除 page

## kvm_mmu_get_page

- [ ] 查看一下 kvm_mmu_get_page 的内容用于 write_protected, account_shadowed 之类的吧


## write protect

- [ ] 如何触发, 一个普通的 write page table 的 protection 凭什么惊动 host ?
- [ ] 触发之后的处理机制在哪里 ?

为什么设置保护机制是在 shadow page table 上的 ?

Guest 想要修改 GPTE 的时候，其实也是需要 page walk 的，此时 page walk 经过的就是
shadow page table. 设置保护就是更新一下 spte 的标志位。当 page walk 失败的原因是
write protect 的时候，显然可以知道是在哪一个 spte 上的，更新 spte 指向的内容。
还用一个可能性是，guest 更新 page table 的时候，总是会首先进行 invlpg

- [ ] kvm_mmu_page::gfn 就是一个 shadow page table 关联的 guest page table 的 gfn，之所以一个 gfn 会关联多个 guest page table 是因为 32bit 的模拟问题吧。
  - [ ] 并且通过 parent 反向映射，可以在 parent shadow page table 中间设置当前 shadow page table 的权限

mmu_need_write_protect 的注释:
CPU1: 正确的顺序是 `sp->unsync`，然后 spte writable。

- [ ] 是不是 host 使用的页面，只有页表被设置为 write protect 的 ?

**当修改 guest page table 的时候，该页面由于被 write protection,
退出，根据 gfn(也就是) 可以找到其对应的 spte,
设置 unsync ，writable，然后继续修改，invlpg，sync page table**

- [ ] 这种设想，有一点多次一举的感觉

```c
static bool mmu_need_write_protect(struct kvm_vcpu *vcpu, gfn_t gfn,
				   bool can_unsync)
{
	struct kvm_mmu_page *sp;

	if (kvm_page_track_is_active(vcpu, gfn, KVM_PAGE_TRACK_WRITE))
		return true;

	for_each_gfn_indirect_valid_sp(vcpu->kvm, sp, gfn) {
		if (!can_unsync)
			return true;

		if (sp->unsync)
			continue;

		WARN_ON(sp->role.level != PG_LEVEL_4K);
		kvm_unsync_page(vcpu, sp);
	}
	smp_wmb();

	return false;
}
```
- 如果一个 page 被 track 了，那么就不需要 write protect
- 将其设置为 unsync 也是可以不设置保护，那么可以在 guest invlpg 的时候进行


```c
/*
 * add guest page to the tracking pool so that corresponding access on that
 * page will be intercepted.
 *
 * It should be called under the protection both of mmu-lock and kvm->srcu
 * or kvm->slots_lock.
 *
 * @kvm: the guest instance we are interested in.
 * @slot: the @gfn belongs to.
 * @gfn: the guest page.
 * @mode: tracking mode, currently only write track is supported.
 */
void kvm_slot_page_track_add_page(struct kvm *kvm,
				  struct kvm_memory_slot *slot, gfn_t gfn,
				  enum kvm_page_track_mode mode)
```

#### rmap_write_protect
函数的目的，从代码分析上来看，给定一个 gfn，然后找到所有映射了这个 gfn 的 shadow page table，在这些 shadow page table 上设置上 write protection，那么就可以实现保护了

- [ ] `__rmap_write_protect` : dirty 和 shadow 的不同
- [ ] 找到因为 shadow protection 而返回的操作
- [ ] 找到证据说明，这一个 gfn 实际上就是 shadow page table 的内容


调用位置:
- [ ] kvm_mmu_get_page
- mmu_sync_children


=> kvm_mmu_slot_gfn_write_protect
```c
bool kvm_mmu_slot_gfn_write_protect(struct kvm *kvm,
				    struct kvm_memory_slot *slot, u64 gfn)
{
	struct kvm_rmap_head *rmap_head;
	int i;
	bool write_protected = false;

  // 为什么 rmap 需要处理多个 levels, 应该是为了实现让所有层次的 page 映射其吧
	for (i = PG_LEVEL_4K; i <= KVM_MAX_HUGEPAGE_LEVEL; ++i) {
    // 通过 rmap 获取 shadow page table 指向其的 spte
    // 这样的也是可以说清楚，通过在 spte 上添加保护位
    // 就可以实现保护 spte
		rmap_head = __gfn_to_rmap(gfn, i, slot);
		write_protected |= __rmap_write_protect(kvm, rmap_head, true);
	}

	return write_protected;
}

static bool __rmap_write_protect(struct kvm *kvm,
				 struct kvm_rmap_head *rmap_head,
				 bool pt_protect)
{
	u64 *sptep;
	struct rmap_iterator iter;
	bool flush = false;

	for_each_rmap_spte(rmap_head, &iter, sptep)
		flush |= spte_write_protect(sptep, pt_protect);

	return flush;
}

```

- [ ] rmap 为什么是反向的, spte 是 page walk 的结果, 从 spte 到 gfn 才是 rmap 啊
  - [ ] rmap 的真正作用是可以让 gfn 可以同时持有多个 spte
  - [ ] 但是实际上只有一个 gfn 可以到达 sptep 啊

从 mmu_set_spte 中间的关于 `was_rmapped` 变量的分析，
rmap 机制是针对于 2mb 的 PMD 之类的

```c
static int rmap_add(struct kvm_vcpu *vcpu, u64 *spte, gfn_t gfn)
{
	struct kvm_mmu_page *sp;
	struct kvm_rmap_head *rmap_head;

	sp = sptep_to_sp(spte);
  // 从这一句说明，就是 spte 指向 gfn
	kvm_mmu_page_set_gfn(sp, spte - sp->spt, gfn);
	rmap_head = gfn_to_rmap(vcpu->kvm, gfn, sp);
	return pte_list_add(vcpu, spte, rmap_head);
}
```


## two cr3
似乎存在 Guest cr3 和 shadow Cr3


## 一般的 page walk 事情总结一下
1. 那些地方需要修改 page table 的，check 一下代码

Level 0 is PML4E(Page-Map Level-4 Offset)
Level 1 is PDPE(Page-Directory- Pointer Offset)
Level 2 is PDP(Page-Directory Offset)
Level 3 is PTE (Page Table Offset)

> An important note is almost all the above structures have a 36-bit Physical Address which means our hypervisor supports only 4-level paging. It is because every page table (and every EPT Page Table) consist of 512 entries which means you need 9 bits to select an entry and as long as we have 4 level tables, we can’t use more than 36 (4 * 9) bits. Another method with wider address range is not implemented in all major OS like Windows or Linux. I’ll describe EPT PML5E briefly later in this topic but we don’t implement it in our hypervisor as it’s not popular yet!
- https://en.wikipedia.org/wiki/X86-64 : 中间描述的是，寻址范围是 48bit，采用的是 4 个 level 的，甚至有 5 level 的
- [ ] host 的寻址模式和 Guest 的寻址模式一致吗 ?


## root_level 和 shadow_root_level


## shadow walk
思考一下一般的 page walk 要求 : 正在访问的 virtual address 和 cr3 指向的物理地址
```c
struct kvm_shadow_walk_iterator {
    u64 addr;            // 虚拟地址
    hpa_t shadow_addr;   // 物理地址，在 shadow_walk_init_using_root 的赋值从 page_walk 的含义是 cr3 的值
    u64 *sptep;          // 指向下一级EPT页表的指针
    int level;           // 当前所处的页表级别，逐级递减
    unsigned index;      // 页表的索引      *sptep + index = 下下一级EPT页表的指针
};
```

翻译 shadow page table :
- addr : Guest 虚拟地址
- shadow_addr : Host 物理地址


- [x] 似乎哪里有点问题，direct_map 被 shadow 使用过 ? 并没有
  - [x] 只是都是使用相同的 page walk 机制而已吧

shadow 的存在两种 page walk:
1. guest page table :  FNAME(walk_addr_generic)
2. shadow page table : FNAME(fetch)

root_hpa:
- shadow : shadow page table 的 pdt
- ept : ept 根地址
都是正常情况下 cr3 存储的内容，但是不知道分别在虚拟化的硬件是什么。


## tlb
kvm_flush_remote_tlbs

#### make_mmu_pages_available
- 检测 `kvm->arch.n_max_mmu_pages` 和 `kvm->arch.n_used_mmu_pages`
- 然后靠 kvm_mmu_zap_oldest_mmu_pages 维持生活

kvm_mmu_alloc_page : 导致 `kvm->arch.n_used_mmu_pages` ++

- mmu page 就是 页表的意思啊，shadow page 或者 ept page

-  kvm_mmu_zap_oldest_mmu_pages 其实就是根据 active_mmu_pages 的头部的 sp 去掉，知道满足足够的数量


- [ ] KVM_ADDRESS_SPACE_NUM


## remote TLB
https://stackoverflow.com/questions/3748384/what-is-tlb-shootdown
https://stackoverflow.com/questions/50256740/who-performs-the-tlb-shootdown


# ept(clear this)
- [ ] EPT violation or an `EPT misconfiguration` encountered during that translation.
- [ ] intel manual chapter 28
- guest page fault will not cause vmexit

- [ ] libdune has to handle page fault in guest mode ?
  - [ ] because mapping full, so we will not page fault
- [ ] dune process malloc and access them ?


  - [ ] why we need `struct pages`, now that all the pages are page table, and unable to free

## nonpaging
- [ ] 作为最终的部分理解, 用于验证自己的想法

## nested
`kvm_init_shadow_ept_mmu` register a `kvm_mmu::page_fault` with `ept_page_fault`,
but what we currently interests in  is `init_kvm_tdp_mmu`


## zap
- kvm_mmu_commit_zap_page
- kvm_mmu_prepare_zap_page

## ref
- [ ]  https://www.linux-kvm.org/images/3/33/KvmForum2008%24kdf2008_15.pdf

## restore
1. fast page fault:
    2. 李强的相关章节
    3. https://events.static.linuxfound.org/sites/events/files/slides/Guangrong-fast-write-protection.pdf
    4. 再次从 mmu notifier 入手，理解其中的基本问题
    5. 理解这一个注释的含义
```plain
/*
 * Fetch a shadow pte for a specific level in the paging hierarchy.
 * If the guest tries to write a write-protected page, we need to
 * emulate this operation, return 1 to indicate this case.
 */
static int FNAME(fetch)(struct kvm_vcpu *vcpu, gpa_t addr,
			 struct guest_walker *gw,
			 int write_fault, int max_level,
			 kvm_pfn_t pfn, bool map_writable, bool prefault,
			 bool lpage_disallowed)
```

## hva_to_pfn 的 backtrace

## 我们到底会为 memory region 分配什么东西
- kvm_arch_prepare_memory_region
  - kvm_alloc_memslot_metadata
    - `__kvm_mmu_slot_lpages` : 似乎是为了处理大页的场景
    - kvm_page_track_create_memslot

- kvm_page_track_create_memslot 创建的内容和 kvm_memory_slot::dirty_bitmap 关系是啥？
  - 没啥关系，kvm_page_track_create_memslot 应该是处理
  - 这个无法避免啊

- [ ] 为什么每一个页面需要分配两次？

## rmap : 从 gfn 到
- memslot_rmap_alloc : 会给每一个 node 分配

```txt
    memslot_rmap_alloc+5
    kvm_mmu_load+993
    kvm_arch_vcpu_ioctl_run+4482
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
```
- kvm_mmu_load
  - mmu_first_shadow_root_alloc
    - mmu_alloc_shadow_roots
      - memslot_rmap_alloc

### rmap 的销毁
- kvm_unmap_gfn_range

rmap 是
```c
static struct kvm_rmap_head *gfn_to_rmap(gfn_t gfn, int level,
					 const struct kvm_memory_slot *slot)
{
	unsigned long idx;

	idx = gfn_to_index(gfn, slot->base_gfn, level);
	return &slot->arch.rmap[level - PG_LEVEL_4K][idx];
}
```


## rmap 是否启动
应该是在 guest 中是否使用嵌套虚拟化会导致是否分配这些内存

- kvm_memslots_have_rmaps

```c
static inline bool kvm_memslots_have_rmaps(struct kvm *kvm)
{
	return !tdp_mmu_enabled || kvm_shadow_root_allocated(kvm);
}
```
如果不使用 rmap，如何在 swap 的时候怎么办？


## 分析嵌套虚拟化如何影响 Guest 实现的
```txt
@[
    kvm_mmu_load+5
    kvm_arch_vcpu_ioctl_run+4482
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 50973
```
在 kvm_mmu_load 取决于 `vcpu->arch.mmu->root_role.direct`
- mmu_alloc_direct_roots
- mmu_alloc_shadow_roots



## [ ] 找到证据，其实 shadow page table 使用的页 和 guest 中的页是一一对应的
- `__rmap_add` 一路找上去即可。

- 如何处理好几种模式的差别？ept nonpaging page32 page64 的？
- 如果 guest 使用大页？


## 分析下，为什么

```c
static __init int svm_hardware_setup(void)
{
  // ...
	/* Force VM NPT level equal to the host's paging level */
	kvm_configure_mmu(npt_enabled, get_npt_level(),
			  get_npt_level(), PG_LEVEL_1G);
```

搞了半天，就是为了这个接口提供的数值:
```c
static inline int kvm_mmu_get_tdp_level(struct kvm_vcpu *vcpu)
```

影响:
- kvm_calc_tdp_mmu_root_page_role
  - 不理解，为什么需要将 kvm_mmu_page_role 中初始化 kvm_mmu_page_role ，这不是给 shadow page table 用的吗?
- kvm_init_shadow_npt_mmu

这个做啥的?
```diff
tree 5bb406f69834e69eedde8e263eaa67757d76c611
parent f83a4a6932f002701db19f968938ada1289f5e3c
author Sean Christopherson <sean.j.christopherson@intel.com> Wed Jul 15 20:41:20 2020 -0700
committer Paolo Bonzini <pbonzini@redhat.com> Thu Jul 30 18:17:16 2020 -0400

KVM: x86: Dynamically calculate TDP level from max level and MAXPHYADDR

Calculate the desired TDP level on the fly using the max TDP level and
MAXPHYADDR instead of doing the same when CPUID is updated.  This avoids
the hidden dependency on cpuid_maxphyaddr() in vmx_get_tdp_level() and
also standardizes the "use 5-level paging iff MAXPHYADDR > 48" behavior
across x86.

Suggested-by: Paolo Bonzini <pbonzini@redhat.com>
Signed-off-by: Sean Christopherson <sean.j.christopherson@intel.com>
Message-Id: <20200716034122.5998-8-sean.j.christopherson@intel.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

## [ ] 为什么 kvm_tdp_mmu_page_fault 最后需要调用 kvm_release_pfn_clean

```txt
@[
    kvm_pfn_to_refcounted_page+5
    kvm_release_pfn_clean+28
    kvm_tdp_page_fault+363
    kvm_mmu_page_fault+613
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1713
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+145
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 191074
```

```txt
Attaching 1 probe...
^C

@[
    kvm_pfn_to_refcounted_page+5
    kvm_release_pfn_clean+28
    kvm_tdp_page_fault+363
    kvm_mmu_page_fault+613
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1713
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+145
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 50
```


### 虚拟机运行过程中，qemu 是否可以主动的，直接的将 page drop 掉，导致 guest 读到的内容为空

### 为什么 kvm 的 ept 是不用增加 refcount 的，这让 migration 和 swap 成为可能?

访问 kvm_release_page_clean 的虚拟机，但是

## 2023.9.15 重新分析

- kvm_calc_tdp_mmu_root_page_role : 这是引入 kvm_mmu_get_tdp_level 的位置

其使用位置实在是太多了:

kvm_x86_ops::load_mmu_pgd 中，

vmx_load_mmu_pgd

我忽然意识到，这是三个类型的 page table 的级数可以互相不同，我的龟龟，
- host page table
- guest page table
- ept page table

host 上是否打开 no5lvl 影响的是 host page table level，而不是 ept page table 的深度。


## 基本思路
hva_to_pfn_slow 中将结果放到 kvm_page_fault:pfn 中

然后在 make_spte 中使用 pfn
```txt
@[
    make_spte+5
    kvm_tdp_mmu_map+906
    kvm_tdp_page_fault+191
    kvm_mmu_do_page_fault+482
    kvm_mmu_page_fault+137
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+2832
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+145
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 228961
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
