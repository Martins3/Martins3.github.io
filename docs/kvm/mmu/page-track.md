## kvm track mode
<!-- 265f99b1-9b21-4dd7-9102-8261f4e1ec67 -->

似乎很接近了，可以继续分析一下

```diff
 KVM: page track: add the framework of guest page tracking

 The array, gfn_track[mode][gfn], is introduced in memory slot for every
 guest page, this is the tracking count for the gust page on different
 modes. If the page is tracked then the count is increased, the page is
 not tracked after the count reaches zero

 We use 'unsigned short' as the tracking count which should be enough as
 shadow page table only can use 2^14 (2^3 for level, 2^1 for cr4_pae, 2^2
 for quadrant, 2^3 for access, 2^1 for nxe, 2^1 for cr0_wp, 2^1 for
 smep_andnot_wp, 2^1 for smap_andnot_wp, and 2^1 for smm) at most, there
 is enough room for other trackers

 Two callbacks, kvm_page_track_create_memslot() and
 kvm_page_track_free_memslot() are implemented in this patch, they are
 internally used to initialize and reclaim the memory of the array

 Currently, only write track mode is supported
```

https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2018/08/11/dirty-pages-tracking-in-migration

> So here for every gfn, we remove the write access. After return from this ioctl, the guest’s RAM has been marked no write access, every write to this will exit to KVM make the page dirty. This means ‘start the dirty log’.


- [ ] kvm_mmu_slot_apply_flags : 实际作用是 dirty log

## gfn_track

```diff
 History:        #0
 Commit:         3d0c27ad6ee465f174b09ee99fcaf189c57d567a
 Author:         Xiao Guangrong <guangrong.xiao@linux.intel.com>
 Committer:      Paolo Bonzini <pbonzini@redhat.com>
 Author Date:    Wed 24 Feb 2016 09:51:11 AM UTC
 Committer Date: Thu 03 Mar 2016 01:36:21 PM UTC

 KVM: MMU: let page fault handler be aware tracked page

 The page fault caused by write access on the write tracked page can not
 be fixed, it always need to be emulated. page_fault_handle_page_track()
 is the fast path we introduce here to skip holding mmu-lock and shadow
 page table walking

 However, if the page table is not present, it is worth making the page
 table entry present and readonly to make the read access happy

 mmu_need_write_protect() need to be cooked to avoid page becoming writable
 when making page table present or sync/prefetch shadow page table entries

 Signed-off-by: Xiao Guangrong <guangrong.xiao@linux.intel.com>
 Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```
-  [ ] tracked 的 page 不能被 fixed, 必须被模拟，为啥 ?

gfn_track 其实没有什么特别的，告诉该 页面被 track 了，然后
kvm_mmu_page_fault 中间将会调用 x86_emulate_instruction 来处理，
似乎然后通过 mmu_notifier 使用 kvm_mmu_pte_write 来更新 guest page table

#### page_fault_handle_page_track
direct_page_fault 和 FNAME(page_fault) 调用，
似乎如果被 track，那么这两个函数会返回 RET_PF_EMULATE


## track 机制
track 和 dirty bitmap 实际上是两个事情吧!

对于加以维护的:
kvm_slot_page_track_add_page :
kvm_slot_page_track_remove_page :
==> update_gfn_track

- [ ] 两个函数，调用 update,  都是对于 gfn_track 的加减 1 而已

分别被 account_shadowed 和 unaccount_shadowed 调用

`__kvm_mmu_prepare_zap_page` : 被各种 zap page 调用，并且配合 commit_zap 使用
=> unaccount_shadowed

kvm_mmu_get_page :
=> account_shadowed

1. kvm_mmu_page_write

```c
void kvm_mmu_init_vm(struct kvm *kvm)
{
    struct kvm_page_track_notifier_node *node = &kvm->arch.mmu_sp_tracker;

    node->track_write = kvm_mmu_pte_write;
    node->track_flush_slot = kvm_mmu_invalidate_zap_pages_in_memslot;
    kvm_page_track_register_notifier(kvm, node);
}
```
kvm_mmu_get_page: 当不是 direct 模式，那么需要对于 kvm_mmu_alloc_page 的 page 进行 account_shadowed
=> account_shadowed :
=> kvm_slot_page_track_add_page

**所以，保护的是 shadow page table ?**

- [ ] 为什么不保护 leaf shadow page ?

> TOBECON

#### gfn_to_rmap
RMAP_RECYCLE_THRESHOLD 居然是 1000

## parent_ptes
```c
static void kvm_mmu_mark_parents_unsync(struct kvm_mmu_page *sp)
{
    u64 *sptep;
    struct rmap_iterator iter;

    for_each_rmap_spte(&sp->parent_ptes, &iter, sptep) {
        mark_unsync(sptep);
    }
}

static void mark_unsync(u64 *spte)
{
    struct kvm_mmu_page *sp;
    unsigned int index;

    sp = sptep_to_sp(spte);
    index = spte - sp->spt;
    if (__test_and_set_bit(index, sp->unsync_child_bitmap))
        return;
    if (sp->unsync_children++)
        return;
    kvm_mmu_mark_parents_unsync(sp);
}
```
递归向上，当发现存在有人 没有 unsync 的时候，在 unsync_child_bitmap 中间设置标志位，
并且向上传导，直到发现没人检测过

link_shadow_page : mark_unsync 的唯一调用位置
kvm_unsync_page : kvm_mmu_mark_parents_unsync 唯一调用位置

mmu_need_write_protect : 对于 sp

#### mmu_need_write_protect
for_each_gfn_indirect_valid_sp : 一个 gfn 可以
同时对应多个 shadow page，原因是一个 guest page 可以对应多个 shadow page


> hash : 实现 guest page tabel 和 shadow page 的映射

> rmap_add 处理的是 :  gfn 和其对应的 pte 的对应关系


## role.quadrant
作用: 一个 guest 地址对应的 page table

get_written_sptes : 依靠 gpa 的 page_offset 计算出来，然后和 `sp->role.quadrant` 对比

#### obsolete sp

```c
static bool is_obsolete_sp(struct kvm *kvm, struct kvm_mmu_page *sp)
{
    return sp->role.invalid ||
           unlikely(sp->mmu_valid_gen != kvm->arch.mmu_valid_gen);
}
```

## 问题
1. kvm_page_track_create_memslot
2. kvm_page_track_write_tracking_alloc

kvm_page_track_create_memslot


## 这个和 dirty page tracking 是一个东西吗?

```txt
config DRM_I915_GVT_KVMGT
	tristate "Enable KVM host support Intel GVT-g graphics virtualization"
	depends on DRM_I915
	depends on KVM_X86
	depends on 64BIT
	depends on VFIO
	select DRM_I915_GVT
	select KVM_EXTERNAL_WRITE_TRACKING
	select VFIO_MDEV

	help
	  Choose this option if you want to enable Intel GVT-g graphics
	  virtualization technology host support with integrated graphics.
	  With GVT-g, it's possible to have one integrated graphics
	  device shared by multiple VMs under KVM.

	  Note that this driver only supports newer device from Broadwell on.
	  For further information and setup guide, you can visit:
	  https://github.com/intel/gvt-linux/wiki.

	  If in doubt, say "N".
```

### KVM_EXTERNAL_WRITE_TRACKING

```config
config KVM_EXTERNAL_WRITE_TRACKING
	bool
```

```diff
History:        #0
Commit:         deae4a10f16649d9c8bfb89f38b61930fb938284
Author:         David Stevens <stevensd@chromium.org>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Wed 22 Sep 2021 12:58:59 PM CST
Committer Date: Fri 01 Oct 2021 03:44:58 PM CST

KVM: x86: only allocate gfn_track when necessary

Avoid allocating the gfn_track arrays if nothing needs them. If there
are no external to KVM users of the API (i.e. no GVT-g), then page
tracking is only needed for shadow page tables. This means that when tdp
is enabled and there are no external users, then the gfn_track arrays
can be lazily allocated when the shadow MMU is actually used. This avoid
allocations equal to .05% of guest memory when nested virtualization is
not used, if the kernel is compiled without GVT-g.

Signed-off-by: David Stevens <stevensd@chromium.org>
Message-Id: <20210922045859.2011227-3-stevensd@google.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```
- intel 显卡: VGT-g 技术
  - https://wiki.archlinux.org/title/Intel_GVT-g
  - https://news.ycombinator.com/item?id=28944426 : 但是支持的不咋样

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

- [ ] git show …… : 这是一个主线
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


## 所以 page_track 实际上是用来处理 write protect page 的
目前注册的 kvm_page_track_write，所以很清楚了，就是通过 gfn track 来跟踪的

```txt
drivers/gpu/drm/i915/gvt/kvmgt.c
109:static void kvmgt_page_track_write(struct kvm_vcpu *vcpu, gpa_t gpa,
667:    vgpu->track_node.track_write = kvmgt_page_track_write;
1607:static void kvmgt_page_track_write(struct kvm_vcpu *vcpu, gpa_t gpa,
```

- kvm_slot_page_track_remove_page
- kvm_slot_page_track_add_page

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
