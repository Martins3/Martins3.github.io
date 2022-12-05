## key questions
- [ ] set_spte 是不是仅仅用于设置 leaf
  - [ ] 如果是，其他 level ?
  - [ ] 如果不是，`spte |= (u64)pfn << PAGE_SHIFT;` 直接指向 pfn 如何理解

## mmu.rst
记录 mmu.rst 的内容:
虽然的确解释了 mmio 使用 generation 的原因，但是下面的问题值得理解:
- [ ] As mentioned in "Reaction to events" above, kvm will cache MMIO information in leaf sptes.
  - [ ] 如果不 cache, 这些数据放在那里

- [ ] When a new memslot is added or an existing memslot is changed, this information may become stale and needs to be invalidated.
  - [ ] 为什么 memslot 增加，导致数据失效

Unfortunately, a single memory access might access kvm_memslots(kvm) multiple
times, the last one happening when the generation number is retrieved and
stored into the MMIO spte.  Thus, the MMIO spte might be created based on
out-of-date information, but with an up-to-date generation number.

- [ ] To avoid this, the generation number is incremented again after synchronize_srcu
returns;

- [ ] 找到访问 pte 来比较 generation, 发现 out of date，然后 slow path 的代码


> Guest memory (gpa) is part of the user address space of the process that is
using kvm.  Userspace defines the translation between guest addresses and user
addresses (`gpa->hva`); note that two gpas may alias to the same hva, but not
vice versa.

> These hvas may be backed using any method available to the host: anonymous
memory, file backed memory, and device memory.  Memory might be paged by the
host at any time.

映射的空间是 mmio 的话，如何配置，如何捕获 ?

The principal data structure is the shadow page, 'struct kvm_mmu_page'.

> shadow page
```c
struct kvm_mmu_page {
	struct list_head link;
	struct hlist_node hash_link;
	struct list_head lpage_disallowed_link;

	bool unsync;
	u8 mmu_valid_gen;
	bool mmio_cached;
	bool lpage_disallowed; /* Can't be replaced by an equiv large page */

	/*
	 * The following two entries are used to key the shadow page in the
	 * hash table.
	 */
	union kvm_mmu_page_role role;
	gfn_t gfn;

	u64 *spt;
	/* hold the gfn of each spte inside spt */
	gfn_t *gfns;
	int root_count;          /* Currently serving as active root */
	unsigned int unsync_children;
	struct kvm_rmap_head parent_ptes; /* rmap pointers to parent sptes */
	DECLARE_BITMAP(unsync_child_bitmap, 512);

#ifdef CONFIG_X86_32
	/*
	 * Used out of the mmu-lock to avoid reading spte values while an
	 * update is in progress; see the comments in __get_spte_lockless().
	 */
	int clear_spte_count;
#endif

	/* Number of writes since the last time traversal visited this page.  */
	atomic_t write_flooding_count;
};
```
A shadow page contains 512 sptes, which can be either leaf or nonleaf sptes.  A
shadow page may contain a mix of leaf and nonleaf sptes.
> shadow page : 一个 page，存在 512 bytes
> **leaf and nonleaf**

A nonleaf spte allows the hardware mmu to reach the leaf pages and
is not related to a translation directly.  It points to other shadow pages.

A leaf spte corresponds to either one or two translations encoded into
one paging structure entry.  These are always the lowest level of the
translation stack, with optional higher level translations left to NPT/EPT.
*Leaf ptes point at guest pages.*

> *TODO* : kvm_mmu_page_role 的每一项目的含义

reverse map :
> The mmu maintains a reverse mapping whereby all ptes mapping a page can be
reached given its gfn.  This is used, for example, when swapping out a page.

在 kvm 下的 reverse map 有什么不同之处吗 ?


> *TODO* synchronized and unsynchronized pages
> A special case is when a guest page table is reachable from the current
guest cr3.  In this case, the guest is obliged to issue an invlpg instruction
before using the translation.

> reachable 为什么反而需要 cr3 来处理


mmio 的 generation number ?

> When KVM finds an MMIO spte, it checks the generation number of the spte.
If the generation number of the spte does not equal the global generation
number, it will ignore the cached MMIO information and handle the page
fault through the slow path.

#### role
>  role.level:
>    The level in the shadow paging hierarchy that this shadow page belongs to.
>    1=4k sptes, 2=2M sptes, 3=1G sptes, etc.


- [] shadow paging 机制似乎是独立于 guest 的类型，不然 for_each_shadow_entry 需要很多种吧
  - [ ] 对于 ept，其实模拟 gpa 的大小
  - [ ] 对于 shadow, 取决于 guest。找到默认设定 guest 的 paging 方式代码的位置


> role.direct:
>   If set, leaf sptes reachable from this page are for a linear range.
>   Examples include real mode translation, large guest pages backed by small
>   host pages, and gpa->hpa translations when NPT or EPT is active.
>   The linear range starts at (gfn << PAGE_SHIFT) and its size is determined
>   by role.level (2MB for first level, 1GB for second level, 0.5TB for third
>   level, 256TB for fourth level)
>   If clear, this page corresponds to a guest page table denoted by the gfn
>   field.

- [] direct 表示 linear range 的，如果 memslot 在用户地址空间，那么其实这很科学

> role.quadrant:
>   When role.gpte_is_8_bytes=0, the guest uses 32-bit gptes while the host uses 64-bit
>   sptes.  That means a guest page table contains more ptes than the host,
>   so multiple shadow pages are needed to shadow one guest page.
>   For first-level shadow pages, role.quadrant can be 0 or 1 and denotes the
>   first or second 512-gpte block in the guest page table.  For second-level
>   page tables, each 32-bit gpte is converted to two 64-bit sptes
>   (since each first-level guest page is shadowed by two first-level
>   shadow pages) so role.quadrant takes values in the range 0..3.  Each
>   quadrant maps 1GB virtual address space.

Only two function related with `kvm_mmu_get_page` and `get_written_sptes`
1. `kvm_mmu_get_page` : alloc a page as page table
2. `get_written_sptes`


## general mmu

TDP : two dimensional paging

KVM Forum :
- https://static.sched.com/hosted_files/kvmforum2019/25/MMU%20improvements%20KVM%20Forum%20Presentation%20-%20short.pdf
- https://stackoverflow.com/questions/49042280/reverse-mapping-in-kvm

问题是:
1. mmu 的设计不是在 vmx 中间吗 ? mmu.c 中间存放什么内容的 ?

mmu.c
```c
/*
 * x86 supports 4 paging modes (5-level 64-bit, 4-level 64-bit, 3-level 32-bit,
 * and 2-level 32-bit).  The kvm_mmu structure abstracts the details of the
 * current mmu mode.
 */
struct kvm_mmu {
```

```c
static inline int kvm_mmu_do_page_fault(struct kvm_vcpu *vcpu, gpa_t cr2_or_gpa,
					u32 err, bool prefault)
{
#ifdef CONFIG_RETPOLINE
	if (likely(vcpu->arch.mmu->page_fault == kvm_tdp_page_fault))
		return kvm_tdp_page_fault(vcpu, cr2_or_gpa, err, prefault);
#endif
	return vcpu->arch.mmu->page_fault(vcpu, cr2_or_gpa, err, prefault);
}
```

> 1. 初始化的顺序
> 2. create CPU 的时候调用

```c
void kvm_init_mmu(struct kvm_vcpu *vcpu, bool reset_roots)
{
	if (reset_roots) {
		uint i;

		vcpu->arch.mmu->root_hpa = INVALID_PAGE;

		for (i = 0; i < KVM_MMU_NUM_PREV_ROOTS; i++)
			vcpu->arch.mmu->prev_roots[i] = KVM_MMU_ROOT_INFO_INVALID;
	}

	if (mmu_is_nested(vcpu))
		init_kvm_nested_mmu(vcpu);
	else if (tdp_enabled)
		init_kvm_tdp_mmu(vcpu);
	else
		init_kvm_softmmu(vcpu); // softmmu 就是 shadowmmu
}
EXPORT_SYMBOL_GPL(kvm_init_mmu);
```


```c
#ifdef KVM_ARCH_NR_OBJS_PER_MEMORY_CACHE
/*
 * Memory caches are used to preallocate memory ahead of various MMU flows,
 * e.g. page fault handlers.  Gracefully handling allocation failures deep in
 * MMU flows is problematic, as is triggering reclaim, I/O, etc... while
 * holding MMU locks.  Note, these caches act more like prefetch buffers than
 * classical caches, i.e. objects are not returned to the cache on being freed.
 */
struct kvm_mmu_memory_cache {
	int nobjs;
	gfp_t gfp_zero;
	struct kmem_cache *kmem_cache;
	void *objects[KVM_ARCH_NR_OBJS_PER_MEMORY_CACHE];
};
#endif
```

## `async_pf`
Asynchronous page fault is a way to try and use guest vcpu more efficiently by allowing it to execute other tasks while page is brought back into memory[1].


路径1:
- `kvm_mmu_do_page_fault`
  - `kvm_tdp_page_fault`
    - `direct_page_fault`
      - `try_async_pf`
        - `kvm_arch_setup_async_pf`
          - `kvm_setup_async_pf` : 在其中初始化一个 workqueue 任务，并且放在队列中间

路径2:
- `kvm_handle_page_fault` : 入口函数, 靠近 exit handler
  - `kvm_mmu_page_fault`
  - `kvm_async_pf_task_wait_schedule`


- [ ] https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/03/24/kvm-async-page-fault
1. 需要修改内核kvm 外面的代码 ? 不然怎么来识别从 host inject 的
2. 内核如何调度 host 的另一个 task 过来运行的
> 算是最清楚的教程了，TODO
- [ ] https://lwn.net/Articles/817239/

## page fault
两条路径:
- `handle_ept_violation`
  - `kvm_mmu_page_fault`
    - `kvm_mmu_do_page_fault` ==> kvm_mmu::page_fault => kvm_tdp_page_fault ==> direct_page_fault ==> `__direct_map` ==> link_shadow_page ==>
- handle_exception_nmi ==> kvm_handle_page_fault ==> ...

#### shadow page fault
```c
/*
 * Page fault handler.  There are several causes for a page fault:
 *   - there is no shadow pte for the guest pte
 *   - write access through a shadow pte marked read only so that we can set
 *     the dirty bit
 *   - write access to a shadow pte marked read only so we can update the page
 *     dirty bitmap, when userspace requests it
 *   - mmio access; in this case we will never install a present shadow pte
 *   - normal guest page fault due to the guest pte marked not present, not
 *     writable, or not executable
 *
 *  Returns: 1 if we need to emulate the instruction, 0 otherwise, or
 *           a negative value on error.
 */
static int FNAME(page_fault)(struct kvm_vcpu *vcpu, gpa_t addr, u32 error_code,
			     bool prefault)
	/*
	 * Look up the guest pte for the faulting address.
	 */
	r = FNAME(walk_addr)(&walker, vcpu, addr, error_code);

  /*
   * Fetch a shadow pte for a specific level in the paging hierarchy.
   * If the guest tries to write a write-protected page, we need to
   * emulate this operation, return 1 to indicate this case.
   */
	r = FNAME(fetch)(vcpu, addr, &walker, write_fault, max_level, pfn,
			 map_writable, prefault, lpage_disallowed);
```
- [x] 从调用路径来说，gpa_t 存在误导，此处是 cr2 的数值，也就是其实应该 guest virtual address
- paging64_walk_addr : 对于 gpt 进行 walk, paging64_fetch : 对于 spt 进行 walk



#### ept page fault

- [ ] where is  paging64_gva_to_gpa ?
- [ ] mtrr : https://zhuanlan.zhihu.com/p/51023864 : 还是很迷，这个到底是做什么

https://stackoverflow.com/questions/60694243/how-does-kvm-qemu-and-guest-os-handles-page-fault

> while the EPT tables are used to translate Guest Physical Addresses into Host Physical Addresses.
>
> If a page is present in the guest page tables but not present in the EPT, it causes an EPT violation VM exit, so the VMM can handle the missing page.


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


## vpclock
https://opensource.com/article/17/6/timekeeping-linux-vms

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/virtualization_host_configuration_and_guest_installation_guide/chap-virtualization_host_configuration_and_guest_installation_guide-kvm_guest_timing_management

#### `kvm_arch_end_assignment`

## memslot
memslot : HPA 到 GVA 之间的映射关系

## link_shadow_page
难道还有反向映射:
1. mmu_page_add_parent_pte

```c
static bool is_mmio_spte(u64 spte)
{
	return (spte & SPTE_SPECIAL_MASK) == SPTE_MMIO_MASK;
}
```


## for_each_shadow_entry_lockless

- [ ] lockless 的实现方法
- [ ] 为什么需要 lock
- [ ] fast_page_fault 是怎么回事

## write_flooding_count

> write_flooding_count:
>    A guest may write to a page table many times, causing a lot of
>    emulations if the page needs to be write-protected (see "Synchronized
>    and unsynchronized pages" below).  Leaf pages can be unsynchronized
>    so that they do not trigger frequent emulation, but this is not
>    possible for non-leafs.  This field counts the number of emulations
>    since the last time the page table was actually used; if emulation
>    is triggered too frequently on this page, KVM will unmap the page
>    to avoid emulation in the future.

- [ ] mmu.rst 理解
  - flood 的避免 : 虽然可以 fork 的避免地址空间的拷贝，但是无法避免 page table 的拷贝，如果一个 page table 在连续在 fork 过程中间被拷贝，其实没有必要立刻同步 shadow page table 的
  - [ ] 为什么 leaf page 可以 unsynchronized，而 non leaf 的不可以 ?
      - [x] 猜测其中的原因是，invlpg addr 的 addr 是虚拟地址，该虚拟地址只能对应一个普通的 shadow page table 的内容
  - [x] 怎么才可以知道这个 translation 被使用过 ?
      - 正常情况下不知道，但是连续三次修改 page table 之后，这个 translation 已经被作废了，所以等到真正需要使用该 translation 的时候，一定会被检查到
  - [x] unmap page 的 unmap 指的是什么 ?
  - [ ] 也就是说 write_flooding_count 是完全针对于 non leaf 的

- [ ] clear_sp_write_flooding_count
    - [ ] FNAME(fetch) 调用多次 clear_sp_write_flooding_count

- [ ] detect_write_flooding
    - [ ] Skip write-flooding detected for the sp whose level is 1, because it can become unsync, then the guest page is not write-protected.
        - 不需要对于 level 1 (leaf 进行write procet 保护), 反正总是 invlpg 会主动同步的 的想法不对，进行了保护那么就可以知道那些 sp 是 unsync 的，在 tlb flush 的时候进行 unsync 就可以了
        - 所以，所有人都需要保护，从而可以截获消息，但是 leaf 可以标记为 unsync, 而 non leaf 需要立刻同步
    - [ ] 调用者 `kvm_mmu_pte_write` : 含义，如果出现了多次模拟, 如果 detect_write_flooding 条件成立，那么调用 kvm_mmu_prepare_zap_page 清理 sp 以及下面的所有的 page table, 递归的向下。
        - [ ] 似乎现在同步体系中间，存在如果一个 guest shadow page table 没有对应的 sp, 并不会因此而创建 sp, 而是等到在 page fault 的再创建

- [ ] 所以 invlpg 真的进行了同步吗 ?

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

## Generation
> As mentioned in "Reaction to events" above, kvm will cache MMIO
information in leaf sptes.

- [ ] cache 什么 MMIO information

> Since only 19 bits are used to store generation-number on mmio spte, all
pages are zapped when there is an overflow.

- [ ] 找到对应的 zap 代码

- [ ] 看不懂的 lock 机制


```c
static inline bool vcpu_match_mmio_gen(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.mmio_gen == kvm_memslots(vcpu->kvm)->generation;
}
```
> TODO mmu.rst 中间算是很清楚的吧，检查一下.

## dirty log
```c
static bool is_dirty_spte(u64 spte)
{
  // 存在一些 ad bit 的奇怪东西
  // 但是没有 ad bit 的支持，似乎 dirty log 是没有办法进行的
	u64 dirty_mask = spte_shadow_dirty_mask(spte);
	return dirty_mask ? spte & dirty_mask : spte & PT_WRITABLE_MASK;
}
```

- [ ] 找到传输 dirty map 的内容

```c
/**
 * kvm_vm_ioctl_get_dirty_log - get and clear the log of dirty pages in a slot
 * @kvm: kvm instance
 * @log: slot id and address to which we copy the log
 *
 * Steps 1-4 below provide general overview of dirty page logging. See
 * kvm_get_dirty_log_protect() function description for additional details.
 *
 * We call kvm_get_dirty_log_protect() to handle steps 1-3, upon return we
 * always flush the TLB (step 4) even if previous step failed  and the dirty
 * bitmap may be corrupt. Regardless of previous outcome the KVM logging API
 * does not preclude user space subsequent dirty log read. Flushing TLB ensures
 * writes will be marked dirty for next log read.
 *
 *   1. Take a snapshot of the bit and clear it if needed.
 *   2. Write protect the corresponding page.
 *   3. Copy the snapshot to the userspace.
 *   4. Flush TLB's if needed.
 */
static int kvm_vm_ioctl_get_dirty_log(struct kvm *kvm,
				      struct kvm_dirty_log *log)
{
	int r;

	mutex_lock(&kvm->slots_lock);

  // 核心函数
	r = kvm_get_dirty_log_protect(kvm, log);

	mutex_unlock(&kvm->slots_lock);
	return r;
}
```

```c
/**
 * kvm_get_dirty_log_protect - get a snapshot of dirty pages
 *	and reenable dirty page tracking for the corresponding pages.
 * @kvm:	pointer to kvm instance
 * @log:	slot id and address to which we copy the log
 *
 * We need to keep it in mind that VCPU threads can write to the bitmap
 * concurrently. So, to avoid losing track of dirty pages we keep the
 * following order:
 *
 *    1. Take a snapshot of the bit and clear it if needed.
 *    2. Write protect the corresponding page.
 *    3. Copy the snapshot to the userspace.
 *    4. Upon return caller flushes TLB's if needed.
 *
 * Between 2 and 4, the guest may write to the page using the remaining TLB
 * entry.  This is not a problem because the page is reported dirty using
 * the snapshot taken before and step 4 ensures that writes done after
 * exiting to userspace will be logged for the next call.
 *
 */
 // TODO 上面的注释
static int kvm_get_dirty_log_protect(struct kvm *kvm, struct kvm_dirty_log *log)

	dirty_bitmap = memslot->dirty_bitmap;

	kvm_arch_sync_dirty_log(kvm, memslot);

	n = kvm_dirty_bitmap_bytes(memslot);
	flush = false;
	if (kvm->manual_dirty_log_protect) { // TODO 这一行的 git message
		/*
		 * Unlike kvm_get_dirty_log, we always return false in *flush,
		 * because no flush is needed until KVM_CLEAR_DIRTY_LOG.  There
		 * is some code duplication between this function and
		 * kvm_get_dirty_log, but hopefully all architecture
		 * transition to kvm_get_dirty_log_protect and kvm_get_dirty_log
		 * can be eliminated.
		 */
		dirty_bitmap_buffer = dirty_bitmap;
```

```c
void kvm_arch_sync_dirty_log(struct kvm *kvm, struct kvm_memory_slot *memslot)
{
	/*
	 * Flush potentially hardware-cached dirty pages to dirty_bitmap.
	 */
	if (kvm_x86_ops.flush_log_dirty)
		kvm_x86_ops.flush_log_dirty(kvm);
}
```

==> 实际上注册的函数
```c
static void vmx_flush_log_dirty(struct kvm *kvm)
{
	kvm_flush_pml_buffers(kvm);
}
```

- [ ] PML buffer 是什么 ?
  - [ ] PMP buffer 为什么在 VMEXITs 的时候被刷新出去了
```c
/*
 * Flush all vcpus' PML buffer and update logged GPAs to dirty_bitmap.
 * Called before reporting dirty_bitmap to userspace.
 */
static void kvm_flush_pml_buffers(struct kvm *kvm)
{
	int i;
	struct kvm_vcpu *vcpu;
	/*
	 * We only need to kick vcpu out of guest mode here, as PML buffer
	 * is flushed at beginning of all VMEXITs, and it's obvious that only
	 * vcpus running in guest are possible to have unflushed GPAs in PML
	 * buffer.
	 */
	kvm_for_each_vcpu(i, vcpu, kvm)
		kvm_vcpu_kick(vcpu);
}
```


#### dirty flags
- [ ]  https://events.static.linuxfound.org/sites/events/files/slides/Guangrong-fast-write-protection.pdf

- [ ] 应该存在软件 和 硬件方式实现 dirty log

kvm_mmu_slot_set_dirty : 将 slot 对应 spte 全部设置为 dirty

这时候的想法是没有问题的 : 通过将所有的 page 全部 write protection, 从而可以知道那些是 dirty 的

- [ ] mmu_spte_update
    - caller : 只是改变 flags 而不修改指向的内容
      - [ ] spte_set_dirty && spte_clear_dirty
      - [ ] spte_write_protect

```c
/*
 * Write-protect on the specified @sptep, @pt_protect indicates whether
 * spte write-protection is caused by protecting shadow page table.
 *
 * Note: write protection is difference between dirty logging and spte
 * protection:
 * - for dirty logging, the spte can be set to writable at anytime if
 *   its dirty bitmap is properly set.
 * - for spte protection, the spte can be writable only after unsync-ing
 *   shadow page.
 *
 * Return true if tlb need be flushed.
 */
static bool spte_write_protect(u64 *sptep, bool pt_protect)
{
	u64 spte = *sptep;

	if (!is_writable_pte(spte) &&
	      !(pt_protect && spte_can_locklessly_be_made_writable(spte)))
		return false;

	rmap_printk("rmap_write_protect: spte %p %llx\n", sptep, *sptep);

	if (pt_protect)
		spte &= ~SPTE_MMU_WRITEABLE;
	spte = spte & ~PT_WRITABLE_MASK;

	return mmu_spte_update(sptep, spte);
}
```
- [x] dirty logging 和 spte protection 如何实现区分
  - [ ] pt_protect 参数就是用于区分到底谁才是 pt_protect

```c
/*
 * Currently, we have two sorts of write-protection, a) the first one
 * write-protects guest page to sync the guest modification, b) another one is
 * used to sync dirty bitmap when we do KVM_GET_DIRTY_LOG. The differences
 * between these two sorts are:
 * 1) the first case clears SPTE_MMU_WRITEABLE bit.
 * 2) the first case requires flushing tlb immediately avoiding corrupting
 *    shadow page table between all vcpus so it should be in the protection of
 *    mmu-lock. And the another case does not need to flush tlb until returning
 *    the dirty bitmap to userspace since it only write-protects the page
 *    logged in the bitmap, that means the page in the dirty bitmap is not
 *    missed, so it can flush tlb out of mmu-lock.
 *
 * So, there is the problem: the first case can meet the corrupted tlb caused
 * by another case which write-protects pages but without flush tlb
 * immediately. In order to making the first case be aware this problem we let
 * it flush tlb if we try to write-protect a spte whose SPTE_MMU_WRITEABLE bit
 * is set, it works since another case never touches SPTE_MMU_WRITEABLE bit.
 *
 * Anyway, whenever a spte is updated (only permission and status bits are
 * changed) we need to check whether the spte with SPTE_MMU_WRITEABLE becomes
 * readonly, if that happens, we need to flush tlb. Fortunately,
 * mmu_spte_update() has already handled it perfectly.
 *
 * The rules to use SPTE_MMU_WRITEABLE and PT_WRITABLE_MASK:
 * - if we want to see if it has writable tlb entry or if the spte can be
 *   writable on the mmu mapping, check SPTE_MMU_WRITEABLE, this is the most
 *   case, otherwise
 * - if we fix page fault on the spte or do write-protection by dirty logging,
 *   check PT_WRITABLE_MASK.
 *
 * TODO: introduce APIs to split these two cases.
 */
static inline int is_writable_pte(unsigned long pte)
{
	return pte & PT_WRITABLE_MASK;
}
```

- [ ] kvm_memory_slot::dirty_bitmap
    - [ ] gfn_to_memslot_dirty_bitmap
    - [ ] mark_page_dirty_in_slot
- [ ] intel 手册 关于 pml 怎么说的

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

[](Documentation/virt/kvm/locking.rst)


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


## lockless
- [ ] SPTE_HOST_WRITEABLE 和 SPTE_MMU_WRITEABLE

```c
static bool spte_can_locklessly_be_made_writable(u64 spte)
{
	return (spte & (SPTE_HOST_WRITEABLE | SPTE_MMU_WRITEABLE)) ==
		(SPTE_HOST_WRITEABLE | SPTE_MMU_WRITEABLE);
}
```

## ad bit
https://stackoverflow.com/questions/32093036/setting-of-intel-ept-accessed-and-dirty-flags-for-guest-page-tables

https://stackoverflow.com/questions/55589131/what-is-the-relation-between-ept-pte-and-host-pte-entry
> 当访问的时候，host pte 和 ept 都会设置 access / dirty bit (前提是 enable 了)

- [ ] 那么 shadow page table 的 ad bit 是如何处理的

#### ad bit manual
IA32_VMX_EPT_VPID_CAP

When accessed and dirty flags for EPT are enabled, software can track writes to guest-physical addresses using a
feature called page-modification logging.

*Software can enable page-modification logging by setting the “enable PML” VM-execution control (see Table 24-7
in Section 24.6.2).*

**If the processor updated a dirty flag for EPT (changing it from 0 to 1), it then operates
as follows:**
> 当 update a dirty flag for EPT 的自动将地址写入到 PML 中间

#### PML white paper
page modification logging

https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/page-modification-logging-vmm-white-paper.pdf

> extended page-table mechanism (EPT) by allowing that software to
monitor the guest-physical pages modified during guest virtual machine (VM) execution more efficiently.

- [ ] 在没有翻译的时候，ad bit 在什么位置 ?

> This feature(AD bit) enables VMMs
to implement memory-management and page-classification algorithms efficiently so
as to optimize VM memory operations such as defragmentation, paging, etc. Without
accessed and dirty flags, *VMMs may need to emulate them (by marking EPT pagingstructures as not-present or read-only)
and incur the overhead of the resulting EPT violations: VM exits and associated software processing.*

- [ ] 为什么通过 ept 的 read only 可以 intercept
  - [ ] read only 不可以吧，访问没有遇到任何阻碍，还是对于 EPT pagingstructures 存在疑惑
- [ ] 通过 intercept 可以实现什么东西

> For some usage models, VMMs may benefit from additional support to monitor the
working set size. As accessed and dirty flags for EPT are set without invoking the
VMM, there is no opportunity for the VMM to accumulate working-set statistics during
operation. To calculate such statistics the VMM must scan the EPT paging structures
to aggregate the information from the accessed and dirty flags. Such scans impose
both latency and bandwidth costs that may be unacceptable in some circumstances.

- [ ] 获取 working set 真的需要对于 ept 的 dirty bit 进行访问吗 ?
  - [ ] 不就是统计 vm 的内存使用量吗 ?

PML builds upon the processor’s support for accessed and dirty flags for EPT,
extending the processing that occurs when dirty flags for EPT are set. *When PML is
active each write that sets a dirty flag for EPT also generates an entry in an inmemory log, reporting the guest-physical address of the write (aligned to 4 KBytes).
When the log is full, a VM exit occurs, notifying the VMM.*


#### PML code

```c
static int handle_pml_full(struct kvm_vcpu *vcpu)
{
	unsigned long exit_qualification;

	trace_kvm_pml_full(vcpu->vcpu_id);

	exit_qualification = vmx_get_exit_qual(vcpu);

	/*
	 * PML buffer FULL happened while executing iret from NMI,
	 * "blocked by NMI" bit has to be set before next VM entry.
	 */
	if (!(to_vmx(vcpu)->idt_vectoring_info & VECTORING_INFO_VALID_MASK) &&
			enable_vnmi &&
			(exit_qualification & INTR_INFO_UNBLOCK_NMI))
		vmcs_set_bits(GUEST_INTERRUPTIBILITY_INFO,
				GUEST_INTR_STATE_NMI);

	/*
	 * PML buffer already flushed at beginning of VMEXIT. Nothing to do
	 * here.., and there's no userspace involvement needed for PML.
	 */
	return 1;
}
```
- [ ] PML buffer 在 VMEXIT 的时候被 flush 到哪里去了
- [ ] white paper 中间提到 exit_qualification 和 vmcs_set_bits 的原因

## hugepage
- [ ] 如果 guest 使用 huge page, ept 可以感知到吗 ?

- [ ] 如果 guest 使用 shadow page table, 并且使用 huge page tabel, 如何办 ?
  - [ ] 直接禁止使用
  - [ ] host 建立对应的 huge page table
    - 如果 guest 建议 huge page table，但是 host 根本没有这么多空间，如何 ?

- [ ] 不考虑 kvm 虚拟化，为了支持 THP 需要实现什么东西 ?
  - TLB 设置标志位，告诉一个一个 2M 的虚拟地址空间都是可以翻译
  - page walk 机制，当检测到是 page entry THP 的时候，可以得到物理地址了

使用 THP 的时候，Guest 的 page walk 可以少走一次，可以直接导致 ept 的 page walk 可以少走一次
Host 使用 page walk，可以让 ept 少走一次

#### kvm_mmu_hugepage_adjust

kvm_mmu_hugepage_adjust 的两个调用位置:
- `__direct_map`
- `FNAME(fetch)`

用于计算 host 的 page walk 的 level，实际上，前提是 host 是 huge page，
否则一般返回 PG_LEVEL_4K，但是实际上，

给定 max_level 以及 gfn, pfn，来判断，最多可以 level up 到何种大小的 huge

```c
/*
 * Return the pointer to the large page information for a given gfn,
 * handling slots that are not large page aligned.
 */
static struct kvm_lpage_info *lpage_info_slot(gfn_t gfn,
					      struct kvm_memory_slot *slot,
					      int level)
{
	unsigned long idx;

	idx = gfn_to_index(gfn, slot->base_gfn, level);
	return &slot->arch.lpage_info[level - 2][idx];
}
```
- [ ] kvm_lpage_info 的 level - 2 的原因是什么



在 kvm_alloc_memslot_metadata 中间，初始化 rmap 和 lpage_info 的信息
- 实际上，对于每一个 page 都会初始化一个 lpage
- 对其，等因素，会让其 disallow_lpage


```diff
 KVM: MMU: rename has_wrprotected_page to mmu_gfn_lpage_is_disallowed

 kvm_lpage_info->write_count is used to detect if the large page mapping
 for the gfn on the specified level is allowed, rename it to disallow_lpage
 to reflect its purpose, also we rename has_wrprotected_page() to
 mmu_gfn_lpage_is_disallowed() to make the code more clearer

 Later we will extend this mechanism for page tracking: if the gfn is
 tracked then large mapping for that gfn on any level is not allowed.
 The new name is more straightforward
```

#### disallowed_hugepage_adjust
- 可以查看该函数所在 git blame，就是对于 host huge page 的再次检查，
如果不可以，那么需要 break up 成为普通 page


```c
static void disallowed_hugepage_adjust(struct kvm_shadow_walk_iterator it,
				       gfn_t gfn, kvm_pfn_t *pfnp, int *levelp)
{
	int level = *levelp;
	u64 spte = *it.sptep;

	if (it.level == level && level > PG_LEVEL_4K &&
	    is_nx_huge_page_enabled() &&
	    is_shadow_present_pte(spte) &&
	    !is_large_pte(spte)) {
		/*
		 * A small SPTE exists for this pfn, but FNAME(fetch)
		 * and __direct_map would like to create a large PTE
		 * instead: just force them to go down another level,
		 * patching back for them into pfn the next 9 bits of
		 * the address.
		 */
		u64 page_mask = KVM_PAGES_PER_HPAGE(level) - KVM_PAGES_PER_HPAGE(level - 1);
		*pfnp |= gfn & page_mask;
		(*levelp)--;
	}
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


## nx huge page

```diff
History:   #0
Commit:    b8e8c8303ff28c61046a4d0f6ea99aea609a7dc0
Author:    Paolo Bonzini <pbonzini@redhat.com>
Committer: Thomas Gleixner <tglx@linutronix.de>
Date:      Mon 04 Nov 2019 11:22:02 AM UTC

kvm: mmu: ITLB_MULTIHIT mitigation

With some Intel processors, putting the same virtual address in the TLB
as both a 4 KiB and 2 MiB page can confuse the instruction fetch unit
and cause the processor to issue a machine check resulting in a CPU lockup.

Unfortunately when EPT page tables use huge pages, it is possible for a
malicious guest to cause this situation.

Add a knob to mark huge pages as non-executable. When the nx_huge_pages
parameter is enabled (and we are using EPT), all huge pages are marked as
NX. If the guest attempts to execute in one of those pages, the page is
broken down into 4K pages, which are then marked executable.

This is not an issue for shadow paging (except nested EPT), because then
the host is in control of TLB flushes and the problematic situation cannot
happen.  With nested EPT, again the nested guest can cause problems shadow
and direct EPT is treated in the same way.

[ tglx: Fixup default to auto and massage wording a bit ]

Originally-by: Junaid Shahid <junaids@google.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
```

#### mmu_topup_memory_caches
- 添加(topup) cache

- [ ] 什么 cache

#### shadow page table 的 dirty bit

他们甚至使用上了 : get_user_page

## sync page


> Synchronized and unsynchronized pages
> =====================================
>
> The guest uses two events to synchronize its tlb and page tables: tlb flushes
> and page invalidations (invlpg).
>
> A tlb flush means that we need to synchronize all sptes reachable from the
> guest's cr3.  **This is expensive, so we keep all guest page tables write
> protected, and synchronize sptes to gptes when a gpte is written.**
>
> *A special case is when a guest page table is reachable from the current
> guest cr3.*  In this case, the guest is obliged to issue an invlpg instruction
> before using the translation.  We take advantage of that by removing write
> protection from the guest page, and allowing the guest to modify it freely.
> We synchronize modified gptes when the guest invokes invlpg.  This reduces
> the amount of emulation we have to do when the guest modifies multiple gptes,
> or when the a guest page is no longer used as a page table and is used for
> random guest data.
>
> As a side effect we have to resynchronize all reachable unsynchronized shadow
> pages on a tlb flush.

- [ ] 这一段 mmu.rst 的理解:
  - [x] 对于所有的 guest page table 都是加以保护的
  - [ ] 在使用 translation 之前，一定会进行 invlpg 来同步，但是 tlb flush 相当于对于所有人都进行了一次 invlpg，那么所有的都需要进行同步

- [x] kvm_sync_pages : shadow 到同一个 gfn 的 page 全部 sync
    - [x] `__kvm_sync_page` => FNAME(sync_name)

- mark unsync 体系
  - [ ] mmu_need_write_protect(gfn),
      - [ ] 注释中间说，存在一种操作 : 将 sp 设置为 unsync 的，然后去掉 spte 的写保护去掉。
          - 当没有了写保护之后，对于 leaf guest page table 就可以随意修改，而且保证其中的数值总是正确的
          - [ ] 只有 leaf page 才可以 unsync 吧
          - [x] race condition 的解释 : 如果在第一个 cpu 中间修改，进行 tlb flush 操作，而第二个中间没有实现标记 unsync，那么就不妙
      - [ ] set_spte 的参数 can_unsync:
        - [ ] mmu_set_spte : true
        - [ ] paging64_sync_page: false : 被 sync 之后，默认都是保护的，然后在下一次写的时候进行可以 unsync ?
  - mark_unsync(spte)

mmu_spte_update : spte 指向的是物理地址，对于 high level 的 shadow page table，其中的 spte 指向是下一级的 shadow page table

```c
/*
 * Using the cached information from sp->gfns is safe because:
 * - The spte has a reference to the struct page, so the pfn for a given gfn
 *   can't change unless all sptes pointing to it are nuked first.
 *
 * Note:
 *   We should flush all tlbs if spte is dropped even though guest is
 *   responsible for it. Since if we don't, kvm_mmu_notifier_invalidate_page
 *   and kvm_mmu_notifier_invalidate_range_start detect the mapping page isn't
 *   used by guest then tlbs are not flushed, so guest is allowed to access the
 *   freed pages.
 *   And we increase kvm->tlbs_dirty to delay tlbs flush in this case.
 */
static int FNAME(sync_page)(struct kvm_vcpu *vcpu, struct kvm_mmu_page *sp)

	first_pte_gpa = FNAME(get_level1_sp_gpa)(sp);
  // 获取 sp 指向的所有 gfn

  // 注意 PT64_ENT_PER_PAGE 一个 shadow page 中间的全部的 page 的数量
  // 所以 sync 一个 gfn 就是将该 gfn 的内容全部传递到其对应的 shadow page table 上
	for (i = 0; i < PT64_ENT_PER_PAGE; i++) {
    // pte_gpa 是 spte 持有的 gfn
		pte_gpa = first_pte_gpa + i * sizeof(pt_element_t);
    // 通过 copy_from_user 来获取的 gpte
		if (kvm_vcpu_read_guest_atomic(vcpu, pte_gpa, &gpte,
					       sizeof(pt_element_t)))
    // 通过 gpte 获取 gfn
		gfn = gpte_to_gfn(gpte);
    // 更新 spte
		set_spte_ret |= set_spte(vcpu, &sp->spt[i],
					 pte_access, PG_LEVEL_4K,
					 gfn, spte_to_pfn(sp->spt[i]),
					 true, false, host_writable);
    }
}
```
- [ ] rvsd

sync 的源头： kvm_mmu_sync_roots 和 kvm_sync_pages
- kvm_mmu_sync_roots 应该是(就是) 将整个 guest page table tree 同步到 shadow page 上。
- kvm_sync_pages 在 kvm_mmu_get_page 创建完成 sp 的时候，那么需要将原来 guest 中间的内容同步过来一下。

#### mmu_sync_children
```c
static int mmu_unsync_walk(struct kvm_mmu_page *sp,
			   struct kvm_mmu_pages *pvec)
{
	pvec->nr = 0; // pvec 就是返回值
	if (!sp->unsync_children) // 如果没有 unsync_children 就直接返回，
		return 0;

	mmu_pages_add(pvec, sp, INVALID_INDEX); // 加入一个 pvec，
	return __mmu_unsync_walk(sp, pvec);
}

// 对于该 page 所有的含有被设置 bitmap 的 page 进行 sync
static int __mmu_unsync_walk(struct kvm_mmu_page *sp,
			   struct kvm_mmu_pages *pvec)
{
	int i, ret, nr_unsync_leaf = 0;

	for_each_set_bit(i, sp->unsync_child_bitmap, 512) {
		struct kvm_mmu_page *child;
		u64 ent = sp->spt[i];

		if (!is_shadow_present_pte(ent) || is_large_pte(ent)) {
			clear_unsync_child_bit(sp, i);
			continue;
		}

		child = to_shadow_page(ent & PT64_BASE_ADDR_MASK);

		if (child->unsync_children) {
			if (mmu_pages_add(pvec, child, i))
				return -ENOSPC;

			ret = __mmu_unsync_walk(child, pvec);
			if (!ret) {
				clear_unsync_child_bit(sp, i);
				continue;
			} else if (ret > 0) {
				nr_unsync_leaf += ret;
			} else
				return ret;
		} else if (child->unsync) {
			nr_unsync_leaf++;
			if (mmu_pages_add(pvec, child, i))
				return -ENOSPC;
		} else
			clear_unsync_child_bit(sp, i);
	}

	return nr_unsync_leaf;
}
```

好的，终于到达了重头戏

```c
static void mmu_sync_children(struct kvm_vcpu *vcpu,
			      struct kvm_mmu_page *parent)
{
	int i;
	struct kvm_mmu_page *sp;
	struct mmu_page_path parents;
	struct kvm_mmu_pages pages;
	LIST_HEAD(invalid_list);
	bool flush = false;

  // 对于所有的 unsync 的 page
	while (mmu_unsync_walk(parent, &pages)) {
		bool protected = false;

    // 设置保护权限
		for_each_sp(pages, sp, parents, i)
			protected |= rmap_write_protect(vcpu, sp->gfn);

		if (protected) {
			kvm_flush_remote_tlbs(vcpu->kvm);
			flush = false;
		}

		for_each_sp(pages, sp, parents, i) {
			flush |= kvm_sync_page(vcpu, sp, &invalid_list);
			mmu_pages_clear_parents(&parents);
		}
		if (need_resched() || spin_needbreak(&vcpu->kvm->mmu_lock)) {
			kvm_mmu_flush_or_zap(vcpu, &invalid_list, false, flush);
			cond_resched_lock(&vcpu->kvm->mmu_lock);
			flush = false;
		}
	}

	kvm_mmu_flush_or_zap(vcpu, &invalid_list, false, flush);
}
```

## kvm_mmu_alloc_page
分配 kvm_mmu_page 和 shadow page，对于非 direct 甚至需要分配一个 gfn 映射数组

```c
	if (!direct) {
		/*
		 * we should do write protection before syncing pages
		 * otherwise the content of the synced shadow page may
		 * be inconsistent with guest page table.
		 */
		account_shadowed(vcpu->kvm, sp); // 对于 non-leaf，使用 page table
		if (level == PG_LEVEL_4K && rmap_write_protect(vcpu, gfn)) // 对于 leaf, 使用 rmap_write_protect
			kvm_flush_remote_tlbs_with_address(vcpu->kvm, gfn, 1);

		if (level > PG_LEVEL_4K && need_sync)
			flush |= kvm_sync_pages(vcpu, gfn, &invalid_list);
	}
```

现在理解一下 rmap_write_protect :


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

## kvm_mmu_page
- kvm_mmu_page : 被称为 page header
- kvm_mmu_page::spt : 指向一个 page，page 的 private 指向这个 page header
- kvm_mmu_page::gfns: shadow 专用的, 实现 rmap 的基础，当其中
- kvm_mmu_page::gfn

```c
static gfn_t kvm_mmu_page_get_gfn(struct kvm_mmu_page *sp, int index)
{
	if (!sp->role.direct)
		return sp->gfns[index];

  // TODO 让人疑惑，为什么 indirect 也要使用这个
	return sp->gfn + (index << ((sp->role.level - 1) * PT64_LEVEL_BITS));
}

static void kvm_mmu_page_set_gfn(struct kvm_mmu_page *sp, int index, gfn_t gfn)
{
	if (!sp->role.direct) {
		sp->gfns[index] = gfn;
		return;
	}

	if (WARN_ON(gfn != kvm_mmu_page_get_gfn(sp, index)))
		pr_err_ratelimited("gfn mismatch under direct page %llx "
				   "(expected %llx, got %llx)\n",
				   sp->gfn,
				   kvm_mmu_page_get_gfn(sp, index), gfn);
}

static int rmap_add(struct kvm_vcpu *vcpu, u64 *spte, gfn_t gfn)
{
	struct kvm_mmu_page *sp;
	struct kvm_rmap_head *rmap_head;

	sp = sptep_to_sp(spte); // 获取 spte 所在的 page 的 shadow page
  // shadow page 到 gfn 的映射，注意，shadow page 中间持有的变换地址是 host physical address
  // XXX : 如此，那么每一个 shadow page table 其实保存着其关联的 guest page table 的内容
	kvm_mmu_page_set_gfn(sp, spte - sp->spt, gfn);
	rmap_head = gfn_to_rmap(vcpu->kvm, gfn, sp);
	return pte_list_add(vcpu, spte, rmap_head);
}
```
所以，gfns 就是shadow page 用于存放自己关联的 guest page table 的内容。

下面分析 gfn 的作用:
1. kvm_mmu_get_page 是唯一赋值的位置
2. gfn 是 ept page fault 的时候，出现问题的 guest physical address


#### parent page
-  kvm_unsync_page  : 需要递归的向上告知 parent page 为 unsync, 所以也需要反向指向的链接


```c
static void drop_parent_pte(struct kvm_mmu_page *sp,
			    u64 *parent_pte)
{
	mmu_page_remove_parent_pte(sp, parent_pte); // 通过 kvm_mmu_page::parent_ptes 这个链表管理
	mmu_spte_clear_no_track(parent_pte); // parent_pte 不在指向该位置
}

static void mmu_page_add_parent_pte(struct kvm_vcpu *vcpu,
				    struct kvm_mmu_page *sp, u64 *parent_pte)
{
	if (!parent_pte)
		return;

	pte_list_add(vcpu, parent_pte, &sp->parent_ptes);
}
```

rmap : 多个 parent page table 会指向 同一个下一级 page table

- [ ]  kvm_mmu_unlink_parents 和 kvm_mmu_page_unlink_children 可以增强理解 mmu

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

- [ ] 是不是host 使用的页面，只有页表被设置为 write protect 的 ?

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

## page track
kvm_mmu_get_page ==> account_shadowed ==> kvm_slot_page_track_remove_page / kvm_slot_page_track_add_page ==> update_gfn_track

```c
static void account_shadowed(struct kvm *kvm, struct kvm_mmu_page *sp)
{
	struct kvm_memslots *slots;
	struct kvm_memory_slot *slot;
	gfn_t gfn;

	kvm->arch.indirect_shadow_pages++;
	gfn = sp->gfn;
	slots = kvm_memslots_for_spte_role(kvm, sp->role);
	slot = __gfn_to_memslot(slots, gfn);

	/* the non-leaf shadow pages are keeping readonly. */
	if (sp->role.level > PG_LEVEL_4K)
		return kvm_slot_page_track_add_page(kvm, slot, gfn,
						    KVM_PAGE_TRACK_WRITE);

	kvm_mmu_gfn_disallow_lpage(slot, gfn);
}
```
- [ ] 为什么 leaf shadow page 被保持 readonly ?
- [ ] 至少说明如果一个 page 被 account，那么就 writable


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
{

	if (WARN_ON(!page_track_mode_is_valid(mode)))
		return;

	update_gfn_track(slot, gfn, mode, 1);

	/*
	 * new track stops large page mapping for the
	 * tracked page.
	 */
	kvm_mmu_gfn_disallow_lpage(slot, gfn);

  // 必然执行，当前只有这种模式
	if (mode == KVM_PAGE_TRACK_WRITE)
    // 将所有的和这一个 gfn 关联的 spte 设置保护
		if (kvm_mmu_slot_gfn_write_protect(kvm, slot, gfn))
			kvm_flush_remote_tlbs(kvm);
}
```




```c
struct kvm_page_track_notifier_node {
	struct hlist_node node;

	/*
	 * It is called when guest is writing the write-tracked page
	 * and write emulation is finished at that time.
	 *
	 * @vcpu: the vcpu where the write access happened.
	 * @gpa: the physical address written by guest.
	 * @new: the data was written to the address.
	 * @bytes: the written length.
	 * @node: this node
	 */
	void (*track_write)(struct kvm_vcpu *vcpu, gpa_t gpa, const u8 *new,
			    int bytes, struct kvm_page_track_notifier_node *node);
	/*
	 * It is called when memory slot is being moved or removed
	 * users can drop write-protection for the pages in that memory slot
	 *
	 * @kvm: the kvm where memory slot being moved or removed
	 * @slot: the memory slot being moved or removed
	 * @node: this node
	 */
	void (*track_flush_slot)(struct kvm *kvm, struct kvm_memory_slot *slot,
			    struct kvm_page_track_notifier_node *node);
};
```


1. kvm_page_track_notifier_node::track_write

- [ ]  在 `kvm_mmu_init_vm` 中间注册的函数 **kvm_mmu_pte_write**
  - [ ] mmu_pte_write_fetch_gpte : 获取 guest 到底想要写入的内容是什么, 根据 gpa 指向的位置
      - [ ] mmu_pte_write_fetch_gpte :  数值在 `emulator_write_phys` 中间刚刚写入
  - [ ] for_each_gfn_indirect_valid_sp : 找到和该 gfn 关联的 shadow page, 强调一次，shadow 和 guest page table 总是关联这的，parent guest page table 可以定位 child page table 的 gfn，根据这个 gfn, 可以定位 child shadow page table
      - [ ] 通过 gpa 就是正在写的 guest page table，从而计算出需要同步更新的 spte 是谁，更新的内容应该是 guest 写入的 page table **关联** 的 shadow page table 的 gfn
      - [ ] 如果 emulator_write_phys 总是调用到 kvm_mmu_pte_write 的话，也就是只要是 emulate 的 write 就是为了实现 shadow page 的更新 ?
  - [ ] mmu_pte_write_new_pte : 进一步调用的前提是 `sp->role.level == PG_LEVEL_4K`
      - [ ] paging64_update_pte
        - [ ] paging64_prefetch_gpte
          - [ ] pte_prefetch_gfn_to_pfn : 这个 pfn 是 host 物理地址, check 是如何使用的。
          - [ ] paging64_protect_clean_gpte : 应该跟踪一下 pte_access 的作用
          - [ ] mmu_set_spte
              - [ ] drop_spte
              - [ ] 变量 : was_rmapped
                  - [x] rmap_add : 一个 gfn 可能被 超过 1000 个 spte 映射
                  - [ ] rmap_recycle ==> kvm_unmap_rmapp ==> pte_list_remove ==> mmu_spte_clear_track_bits : 将 spte 的内容清空。rmap 在说，如果你(spte)指向了我（gfn），那么就存在 rmap，当需要 gfn 发生改动的时候，指向我的 gfn 都是需要修改的，虽然你应该指向我，只要你的内容是空，可以通过 page fault 最后指向此处。
  - [ ] 为什么参数是物理地址，从虚拟地址到物理地址的装换是谁来操作的 ?
  - [x] 更新的策略是 : 如果是 level 4，那么和 pfn 指向相同的位置，否则 invalid 该 spte 下面关联的所有的 shadow page table
      - [x] kvm_mmu_get_page 分别调用 `rmap_write_protect` 和 `account_shadowed`, 对于 leaf 和 nonleaf 都是加以保护了
- [ ] 从 `FNAME(page_fault)` 的注释看，调用该函数的原因中没有由于 write protection 的存在
    - [ ] 通过 async_pf 从 gfn 得到其 host 物理页面。虽然 guest 自己处理自己的 page frame, 但是分配了 gfn 之后，还是需要对应的 host 物理页面，我猜测其中还是利用 gup 实现的功能
    - [ ] set_spte 的结果说， Write protection  is responsibility of mmu_get_page / kvm_sync_page.
- [ ] 只有 high level 的 gpt 需要 track, 是不是因为 leaf page table 中间的页面是可以同时进行 protect 保护 和 dirty bitmap 的，所以相对于 non leaf 的保护其实更加简单。
    - [ ] 保护只能靠 rmap 保护，找到来映射 gpt gfn 的 spt，设置保护，在 page fault 的时候，如果检查到这是 track 的 page，加以同步
    - [ ] 实际上，由于 rmap 保护，将 shadow page 标记为 unsync，之后再 sync, 也即是通过 page track 实现同步修改，而 sync 实现异步修改
    - [ ] 重新理解一下 cow, 为什么可以 mmap 一个没有访问权限的文件并且加以读去，而且还会进入到 gup 中间。



- [ ] 似乎只是完成对于 guest page table 的模拟，但是 shadow page table 没有被同步
- [ ] kvm_page_track_init : 内核可以提供多个 kvm 的环境来运行, 当发送消息之后，会给所有的 kvm instance 通知

2. 调用位置
- emulate_ops::write_emulated ==> emulator_write_emulated ==> emulator_read_write ==> emulator_read_write_onepage ==> write_emultor::read_write_emulate ==> write_emulate ==> emulator_write_phys
- emulate_ops::cmpxchg_emulated ==>  mulator_cmpxchg_emulated

分析其中一个分支:
segmented_write ==> emulate_ops::write_emulated

direct_page_fault 和 paging64_page_fault 的位置都调用，page_fault_handle_page_track 检查，如果访问的 page 被 track 那么访问需要被 emulate，具体在 `kvm_mmu_page_fault` 中间。

- [ ] git show .... : 这是一个主线
    - [ ] 猜测，使用 write track ，在 `emulator_write_phys` 中:
        - [ ] kvm_vcpu_write_guest : 完成写的模拟
        - [ ] 通知所有人，将写操作可以同步到 sync 中间了
    - [ ] reexecute_instruction : 猜测，是因为访问 guest page table 被截获了
      - [ ] write_fault_to_shadow_pgtable
    - [x] 有一件事情没有考虑 : guest 的物理地址是什么时候分配的，现在只是关注了 shadow page table 和 ept page table 的分配.
        - 这是 guest 自己的事情，谢谢
    - [ ] 对于 instruction emulation 的理解，当使用一条指令进行修改 guest page table 的时候，显然 vmexit, 然后在 emulation 中间更新 shadow page table 以及 guest page table.
    - [ ] kvm_mmu_unprotect_page : 检查其调用位置
    - [ ] is_writable_pte()
      - [ ] it flush tlb if we try to write-protect a spte whose `SPTE_MMU_WRITEABLE`
      - [ ] mmu_spte_update() : Update the state bits, it means the mapped pfn is not changed.
          - [x] 只是修改其中的 ad bit 以及 权限等，而不是修改其中 pfn
          - [ ] spte_write_protect <= `__rmap_write_protect` <= `rmap_write_protect` : 我们理解的 write protect 的体系
      - [ ] `#define SPTE_HOST_WRITEABLE	(1ULL << PT_FIRST_AVAIL_BITS_SHIFT)` 和 `#define SPTE_MMU_WRITEABLE	(1ULL << (PT_FIRST_AVAIL_BITS_SHIFT + 1))`
    - [ ] 同步完成 shadow page table 之后需要 flush tlb
    - [ ] 为什么 dirty bitmap log 需要进行保护，是不是这又是一种硬件模拟，为了判断 guest 的那些页面是 dirty 的

```c
/*
 * Currently, we have two sorts of write-protection, a) the first one
 * write-protects guest page to sync the guest modification, b) another one is
 * used to sync dirty bitmap when we do KVM_GET_DIRTY_LOG. The differences
 * between these two sorts are:
 * 1) the first case clears SPTE_MMU_WRITEABLE bit.
 * 2) the first case requires flushing tlb immediately avoiding corrupting
 *    shadow page table between all vcpus so it should be in the protection of
 *    mmu-lock. And the another case does not need to flush tlb until returning
 *    the dirty bitmap to userspace since it only write-protects the page
 *    logged in the bitmap, that means the page in the dirty bitmap is not
 *    missed, so it can flush tlb out of mmu-lock.
 *
 * So, there is the problem: the first case can meet the corrupted tlb caused
 * by another case which write-protects pages but without flush tlb
 * immediately. In order to making the first case be aware this problem we let
 * it flush tlb if we try to write-protect a spte whose SPTE_MMU_WRITEABLE bit
 * is set, it works since another case never touches SPTE_MMU_WRITEABLE bit.
 *
 * Anyway, whenever a spte is updated (only permission and status bits are
 * changed) we need to check whether the spte with SPTE_MMU_WRITEABLE becomes
 * readonly, if that happens, we need to flush tlb. Fortunately,
 * mmu_spte_update() has already handled it perfectly.
 *
 * The rules to use SPTE_MMU_WRITEABLE and PT_WRITABLE_MASK:
 * - if we want to see if it has writable tlb entry or if the spte can be
 *   writable on the mmu mapping, check SPTE_MMU_WRITEABLE, this is the most
 *   case, otherwise
 * - if we fix page fault on the spte or do write-protection by dirty logging,
 *   check PT_WRITABLE_MASK.
 *
 * TODO: introduce APIs to split these two cases.
 */
static inline int is_writable_pte(unsigned long pte)
{
	return pte & PT_WRITABLE_MASK;
}
```
从注释的可以解读的东西:
1. 使用通过 SPTE_MMU_WRITEABLE 来实现对于 guest 的保护
2. 使用 PT_WRITABLE_MASK 用于 dirty bitmap 操作的


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
    1. 文档 https://www.kernel.org/doc/html/latest/virt/kvm/locking.html
    2. 李强的相关章节
    3. https://events.static.linuxfound.org/sites/events/files/slides/Guangrong-fast-write-protection.pdf
    4. 再次从 mmu notifier 入手，理解其中的基本问题
    5. 理解这一个注释的含义
```
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
