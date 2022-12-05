## track mode

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

mmu_need_write_protect : 对于sp

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
