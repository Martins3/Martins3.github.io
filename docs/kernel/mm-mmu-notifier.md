# mmu notifier

## mmu_notifier
- [ ] hva_to_pfn : 这里应该封装了 gup 的实现

kvm_mmu_notifier_invalidate_range_start : 这是关键，end 几乎没有内容，invalidate_range 是给 mmu notifier 使用的.

kvm_mmu_notifier_invalidate_range_start ==> kvm_unmap_hva_range ==> kvm_handle_hva_range
  - [ ] kvm_unmap_rmapp ==> kvm_zap_rmapp
    - [ ] mmu_spte_clear_track_bits : 将 sptep 的内容清空，也就是 shadow page table 无法访问到位置, and sync spte's ad bit to host pte's ad bit
    - [ ] pte_list_remove : The `spte` is no longer managed by this `gfn`
  - [ ] kvm_handle_hva_range : 获取到 range 对应的 gfn
    - [ ] for_each_slot_rmap_range :
        - [ ] `__gfn_to_rmap`

```c
static struct kvm_rmap_head *__gfn_to_rmap(gfn_t gfn, int level,
					   struct kvm_memory_slot *slot)
{
	unsigned long idx;

	idx = gfn_to_index(gfn, slot->base_gfn, level);
	return &slot->arch.rmap[level - PG_LEVEL_4K][idx];
}
```


- [ ] kvm_mmu_notifier_clear_young
  - [ ] kvm_age_hva
  - [ ] kvm_handle_hva_range kvm_age_rmapp



```c
static int kvm_age_rmapp(struct kvm *kvm, struct kvm_rmap_head *rmap_head,
			 struct kvm_memory_slot *slot, gfn_t gfn, int level,
			 unsigned long data)
{
	u64 *sptep;
	struct rmap_iterator iter;
	int young = 0;

	for_each_rmap_spte(rmap_head, &iter, sptep)
		young |= mmu_spte_age(sptep);

	trace_kvm_age_page(gfn, level, slot, young);
	return young;
}
```

暂时的理解是 : host 告诉一个区域的 page 有问题了, 根据 HVA 可以知道 GPA 的物理地址出现问题，
那么需要知道找到对应的 shadow page table 进行处理.

由此可以想象 : 当 host 将 guest 的物理页面 swap 出去，由此可以
从而修改 shadow page table


kvm_arch_mmu_notifier_invalidate_range: 似乎是一个为 APIC 制作的

```c
void kvm_arch_mmu_notifier_invalidate_range(struct kvm *kvm,
					    unsigned long start, unsigned long end)
{
	unsigned long apic_address;

	/*
	 * The physical address of apic access page is stored in the VMCS.
	 * Update it when it becomes invalid.
	 */
	apic_address = gfn_to_hva(kvm, APIC_DEFAULT_PHYS_BASE >> PAGE_SHIFT);
	if (start <= apic_address && apic_address < end)
		kvm_make_all_cpus_request(kvm, KVM_REQ_APIC_PAGE_RELOAD);
}
```
- [ ] 无法理解 kvm_arch_mmu_notifier_invalidate_range 最后是给 KVM_REQ_APIC_PAGE_RELOAD



对应的通知机制是是给 guest 发出通知消息

- [ ] mmu_notifier 和 gfn_track 存在什么关联吗 ?

- [ ] 检测 kvm_page_track_notifier_node:node 的使用位置
- [ ] 似乎多个 kvm_page_track_notifier_node 可以挂到一个 kvm_page_track_notifier_head 上



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

- [ ] do shadow page table and ept have a different perspective with mmu notifier ?
- [x] what if guest physical memory is swapped out ?
    - that's why mmu notifier make sense, if guest physical memory is swapped out, mmu notifier will tell kvm to invalid pages and spte in it.



#### https://lwn.net/Articles/266320/
Guest systems maintain their own page tables, but they are not the tables used by the memory management unit. Instead, whenever the guest makes a change to its tables, the host system intercepts the operation, checks it for validity, then mirrors the change in the real page tables, which "shadow" those maintained by the guest.
> - 物理上的是 shadow page table，guest 无法访问 shadow page table，否则存在安全问题

In particular, if the host system decides that it wants to push a given page out to swap, it can't tell the guest that the page is no longer resident.
> 通过 notifier 告诉他的物理地址被换出了

#### https://lwn.net/Articles/732952/




check it /home/martins3/core/linux/Documentation/mm/mmu_notifier.rst

配合 mmu notifier 理解:
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

- dune 中观察到，需要让 ept 的二级映射指向新创建出来的 page 上:
```txt
[25556.799013] Hardware name: Timi TM1701/TM1701, BIOS XMAKB5R0P0603 02/02/2018
[25556.799013] Call Trace:
[25556.799019]  dump_stack+0x6d/0x9a
[25556.799037]  ept_mmu_notifier_invalidate_range_start.cold+0x5/0xfe [dune]
[25556.799039]  __mmu_notifier_invalidate_range_start+0x5e/0xa0
[25556.799041]  wp_page_copy+0x6be/0x790
[25556.799042]  ? vsnprintf+0x39e/0x4e0
[25556.799043]  do_wp_page+0x94/0x6a0
[25556.799045]  ? sched_clock+0x9/0x10
[25556.799046]  __handle_mm_fault+0x771/0x7a0
[25556.799047]  handle_mm_fault+0xca/0x200
[25556.799048]  __get_user_pages+0x251/0x7d0
[25556.799049]  get_user_pages_unlocked+0x145/0x1f0
[25556.799050]  get_user_pages_fast+0x180/0x1a0
[25556.799051]  ? ept_lookup_gpa.isra.0+0xb2/0x1a0 [dune]
[25556.799053]  vmx_do_ept_fault+0xe3/0x450 [dune]
```

- https://www.linux-kvm.org/images/3/33/KvmForum2008%24kdf2008_15.pdf

#### mmu notifier
[^24] is worth reading !

- some notifier triggers:
  - try_to_unmap_one
  - ptep_clear_flush_notify

- [ ] how kvm work with mmu notifier ?

- [ ] mmu_notifier.rst

- [x] so why kvm need mmu notifier ?
[Integrating KVM with the Linux Memory Management](https://www.linux-kvm.org/images/3/33/KvmForum2008%24kdf2008_15.pdf)

Guest ram is mostly allocated by user process with `memalign()` and
kvm get physical memory with `get_user_pages`.

> The 'MMU Notifier' functionality can be also used
by other subsystems like GRU and XPMEM to
export the user virtual address space of
computational tasks to other nodes

> This will also allow KVM guest physical ram itself
to be exported to other nodes through GRU and
XPMEM or any other RDMA engine

- TODO really interesting RDMA and XPMEM

> - The KVM page fault is the one that instantiates the shadow pagetables
> - Shadow pagetables works similarly to a TLB
> - They translate a virtual (or physical with EPT/NPT) guest address to a physical host address
> - They can be discarded at any time and they will be recreated later as new KVM page fault triggers, just like the primary CPU TLB can be flushed at any time and the CPU will refill it from the ptes
> - The sptes are recreated by the KVM page fault by calling get_user_pages (i.e. looking at the Linux ptes) to translate a guest physical address (the malloced region) to a host physical address

------------  function calling chain -------------------------- begin ---

- [ ] *unless we can understand hugetlb and thp, we can't understand mmu_notifier*

```plain
mmu_notifier_invalidate_range_start
  --> __mmu_notifier_invalidate_range_start
    --> mn_itree_invalidate
    --> mn_hlist_invalidate_range_start : call list one by one
```

```plain
mmu_notifier_invalidate_range_end
  --> __mmu_notifier_invalidate_range_end
    --> mn_itree_inv_end
    --> mn_hlist_invalidate_end
```

```plain
__mmu_notifier_register :
1. if mm->notifier_subscriptions is NULL, alloc and init one for it
2. is parameter subscription is not NULL, add it to mm->notifier_subscriptions list, mm->notifier_subscriptions->has_itree = true; otherwise
  mm_drop_all_locks(mm);
```
------------  function calling chain -------------------------- begin ---



------------ critical struct -------------------------- begin ---
```c
/*
 * The notifier chains are protected by mmap_lock and/or the reverse map
 * semaphores. Notifier chains are only changed when all reverse maps and
 * the mmap_lock locks are taken.
 *
 * Therefore notifier chains can only be traversed when either
 *
 * 1. mmap_lock is held.
 * 2. One of the reverse map locks is held (i_mmap_rwsem or anon_vma->rwsem).
 * 3. No other concurrent thread can access the list (release)
 */
struct mmu_notifier {
  struct hlist_node hlist;
  const struct mmu_notifier_ops *ops;
  struct mm_struct *mm;
  struct rcu_head rcu;
  unsigned int users;
};

/**
 * struct mmu_interval_notifier_ops
 * @invalidate: Upon return the caller must stop using any SPTEs within this
 *              range. This function can sleep. Return false only if sleeping
 *              was required but mmu_notifier_range_blockable(range) is false.
 */
struct mmu_interval_notifier_ops {
  bool (*invalidate)(struct mmu_interval_notifier *interval_sub,
         const struct mmu_notifier_range *range,
         unsigned long cur_seq);
};

struct mmu_interval_notifier {
  struct interval_tree_node interval_tree;
  const struct mmu_interval_notifier_ops *ops;
  struct mm_struct *mm;
  struct hlist_node deferred_item;
  unsigned long invalidate_seq;
};


struct mmu_notifier_range {
  struct vm_area_struct *vma;
  struct mm_struct *mm;
  unsigned long start;
  unsigned long end;
  unsigned flags;
  enum mmu_notifier_event event;
  void *migrate_pgmap_owner;
};

/*
 * The mmu_notifier_subscriptions structure is allocated and installed in
 * mm->notifier_subscriptions inside the mm_take_all_locks() protected
 * critical section and it's released only when mm_count reaches zero
 * in mmdrop().
 */
struct mmu_notifier_subscriptions {
  /* all mmu notifiers registered in this mm are queued in this list */
  struct hlist_head list;
  bool has_itree;
  /* to serialize the list modifications and hlist_unhashed */
  spinlock_t lock;
  unsigned long invalidate_seq;
  unsigned long active_invalidate_ranges;
  struct rb_root_cached itree;
  wait_queue_head_t wq;
  struct hlist_head deferred_list;
};
```

- `mmu_notifier` and `mmu_interval_notifier` are chained into `mmu_notifier_subscriptions`
- `mmu_notifier_range` is interface for memory management part
------------ critical struct -------------------------- end ---


[^24]: [lwn : Memory management notifiers](https://lwn.net/Articles/266320/)
