## Documentation/virt/kvm/x86/mmu.rst
<!-- 9ca50de3-ce3d-4a2c-843e-2e8c1b8008d3 -->

https://www.kernel.org/doc/html/latest/virt/kvm/x86/mmu.html

## kvm_mmu_page
The principal data structure is the shadow page, 'struct kvm_mmu_page'.  A
shadow page contains 512 sptes, which can be either leaf or nonleaf sptes.  A
shadow page may contain a mix of leaf and nonleaf sptes.

可以基本记忆为，其实一个 kvm_mmu_page 就是对应一个内核的 page table 吗?
```c
struct kvm_mmu_page {
    gfn_t gfn;                              // 这个 shadow page 对应的 guest 页表页

    bool unsync;                            // 这个 shadow page 是否与 guest 不同步

    struct kvm_rmap_head parent_ptes;       // 反向指针：哪些父 SPTEs 指向这个 shadow page

    unsigned int unsync_children;           // 有多少子 shadow pages 不同步

    DECLARE_BITMAP(unsync_child_bitmap, 512);  // 标记哪些子页面不同步（512 位 bitmap）

    u64 *spt;                               // Shadow page table (512 个 SPTEs)
};
```

A shadow page contains 512 sptes, which can be either leaf or nonleaf sptes.  A
shadow page may contain a mix of leaf and nonleaf sptes.

A nonleaf spte allows the hardware mmu to reach the leaf pages and
is not related to a translation directly.  It points to other shadow pages.

A leaf spte corresponds to either one or two translations encoded into
one paging structure entry.  These are always the lowest level of the
translation stack, with optional higher level translations left to NPT/EPT.
*Leaf ptes point at guest pages.*

### kvm_mmu_page::write_flooding_count
<!-- 6bcca3ff-46e0-407f-b40e-b86e491f1e01 -->

这说明了，page table 存在无法监控的地方!

write_flooding_count:
   A guest may write to a page table many times, causing a lot of
   emulations if the page needs to be write-protected (see "Synchronized
   and unsynchronized pages" below).  Leaf pages can be unsynchronized
   so that they do not trigger frequent emulation, but this is not
   possible for non-leafs.  This field counts the number of emulations
   since the last time the page table was actually used; if emulation
   is triggered too frequently on this page, KVM will unmap the page
   to avoid emulation in the future.

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
        - 不需要对于 level 1 (leaf 进行 write procet 保护), 反正总是 invlpg 会主动同步的 的想法不对，进行了保护那么就可以知道那些 sp 是 unsync 的，在 tlb flush 的时候进行 unsync 就可以了
        - 所以，所有人都需要保护，从而可以截获消息，但是 leaf 可以标记为 unsync, 而 non leaf 需要立刻同步
    - [ ] 调用者 `kvm_mmu_pte_write` : 含义，如果出现了多次模拟, 如果 detect_write_flooding 条件成立，那么调用 kvm_mmu_prepare_zap_page 清理 sp 以及下面的所有的 page table, 递归的向下。
        - [ ] 似乎现在同步体系中间，存在如果一个 guest shadow page table 没有对应的 sp, 并不会因此而创建 sp, 而是等到在 page fault 的再创建

### kvm_mmu_page::spt
<!-- 47d5c98f-f55a-40dc-85bf-f5debb369cb7 -->

1. sp->spt 是一个 4KB 对齐的物理页指向一个 page，page 的 private 反过来指向 kvm_mmu_page
2. sp->spt 是被硬件直接使用的

> spt:
> A pageful of 64-bit sptes containing the translations for this page.
> Accessed by both kvm and hardware. The page pointed to by spt will have its
> page->private pointing back at the shadow page structure. sptes in spt point either at guest pages,
> or at lower-level shadow pages. Specifically, if sp1 and sp2 are shadow pages, then `sp1->spt[n]` may point at `__pa(sp2->spt).`
> sp2 will point back at sp1 through parent_pte. The spt array forms a DAG
structure with the shadow page as a node, and guest pages as leaves.

(注: Directed acyclic graph 有向无环图)

理解了这些内容，再去理解 __rmap_add 就比较容易了:


#### 那么 tdp_mmu 的时候，spt 中包含的内容就是 ept page table 的吗?

sp_has_gptes 中也有分析:
```c
static struct kvm_mmu_page *tdp_mmu_alloc_sp(struct kvm_vcpu *vcpu)
{
	struct kvm_mmu_page *sp;

	sp = kvm_mmu_memory_cache_alloc(&vcpu->arch.mmu_page_header_cache);
	sp->spt = kvm_mmu_memory_cache_alloc(&vcpu->arch.mmu_shadow_page_cache);

	return sp;
}

```


### kvm_mmu_page::gfn
<!-- 107c7d85-5e2b-4d9b-81b1-6ae4d0b21de3 -->

kvm_mmu_page_get_gfn 是一个经典函数，如果使用 direct 了，那么映射就是固定的
可以根据 index 自动计算出来，但是如果是 shadow page table 模式，
那么就是存储的具体的映射的内容:
```c
static gfn_t kvm_mmu_page_get_gfn(struct kvm_mmu_page *sp, int index)
{
	if (sp->role.passthrough)
		return sp->gfn;

	if (sp->shadowed_translation)
		return sp->shadowed_translation[index] >> PAGE_SHIFT;

	return sp->gfn + (index << ((sp->role.level - 1) * SPTE_LEVEL_BITS));
}
```

> gfn:
> Either the guest page table containing the translations shadowed by this page,
or the base page frame for linear translations. See role.direct.


```txt
@[
    kvm_mmu_page_get_gfn+138
    drop_spte+119
    mmu_page_zap_pte+263
    __kvm_mmu_prepare_zap_page+542
    __kvm_mmu_prepare_zap_page+371
    kvm_mmu_track_write+511
    write_emulate+55
    emulator_read_write_onepage+424
    emulator_read_write+194
    writeback+636
    x86_emulate_insn+1374
    x86_emulate_instruction+504
    kvm_mmu_page_fault+541
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 32558
@[
    kvm_mmu_page_get_gfn+138
    paging64_sync_spte+387
    mmu_sync_children+932
    kvm_mmu_sync_roots+426
    kvm_arch_vcpu_ioctl_run+2299
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 141485
```
### kvm_mmu_page::shadowed_translation
<!-- 1fb465ad-2daf-4ca2-a032-dbe4a0c9b052 -->

> shadowed_translation:
> An array of 512 shadow translation entries, one for each present pte. Used to perform
> a reverse map from a pte to a gfn as well as its access permission.
> When role.direct is set, the shadow_translation array is not allocated. This is because the gfn contained in any element of
> this array can be calculated from the gfn field when used.
> In addition, when role.direct is set, KVM does not track access permission for each of the gfn. See role.direct and gfn.


spt 与 shadowed_translation 存的是不同维度的信息

spt[N]                      shadowed_translation[N]
─────────────────────        ──────────────────────────────
硬件 SPTE (u64)              KVM 内部格式 (u64)
CPU 走页表时读取              KVM 自己使用，CPU 不感知

内容：HPA + 硬件权限位         内容：GFN | 客户机权限
      ↑ 宿主机物理地址                ↑ 客户机帧号
      ↑ CPU 实际执行的权限            ↑ 客户机 GPTE 中指定的权限

编码方式（mmu_internal.h 注释 + set_translation 代码）：

// shadowed_translation[index] 的编码:
sp->shadowed_translation[index] = (gfn << PAGE_SHIFT) | access;
//                                  ^^^^^^^^^^^^^^^^    ^^^^^^
//                                  bits[63:12] = GFN   bits[2:0] = ACC_EXEC|WRITE|USER

// 读取：
gfn    = shadowed_translation[index] >> PAGE_SHIFT;
access = shadowed_translation[index] & ACC_ALL;   // ACC_ALL = 0x7

---
为什么需要两个数组？

关键原因：spt[N] 中的权限位可能比客户机 GPTE 更严格。

注释说得很清楚（mmu.c:651-654）：

For leaf SPTEs, fetch the *guest* access permissions being shadowed.
Note that the SPTE itself may have a more constrained access permissions
that what the guest enforces. For example, a guest may create an
executable huge PTE but KVM may disallow execution to mitigate iTLB multihit.

几个典型场景：

┌────────────────────┬──────────────────────┬──────────────────────────────────┐
│        场景        │   spt[N] 中的权限    │ shadowed_translation[N] 中的权限 │
├────────────────────┼──────────────────────┼──────────────────────────────────┤
│ dirty logging 开启 │ W 位被清除（写保护） │ 仍记录客户机要求可写             │
├────────────────────┼──────────────────────┼──────────────────────────────────┤
│ NX huge page 缓解  │ X 位被清除           │ 仍记录客户机要求可执行           │
├────────────────────┼──────────────────────┼──────────────────────────────────┤
│ write-tracking     │ W 位被清除           │ 仍记录客户机要求可写             │
└────────────────────┴──────────────────────┴──────────────────────────────────┘

KVM 需要保留客户机的原始意图，才能判断：
- 发生 page fault 时，是客户机本身的权限不足，还是 KVM 主动写保护触发的？
- 当 KVM 的约束解除时（如 dirty logging 关闭），哪些 SPTE 可以恢复可写？

---
另一个原因：non-direct 页面里 GFN 不可推算

对于 role.direct=1 的 shadow page，GFN 可直接计算：

// direct 情况（kvm_mmu_page_get_gfn）:
return sp->gfn + (index << ((sp->role.level - 1) * SPTE_LEVEL_BITS));
// shadowed_translation 不分配

对于 role.direct=0（影射一张客户机页表）：

客户机页表中的 512 条 GPTE 各自指向不同的 GFN：
  GPTE[0] → GFN 200
  GPTE[1] → GFN 350   ← 跳跃的，没有规律，无法推算
  GPTE[2] → GFN 201

只能显式存储，所以分配了 shadowed_translation[]。

---
完整对照

一个 non-direct shadow page（影射客户机 L1 页表）：

index    spt[N]（CPU 用）                shadowed_translation[N]（KVM 用）
─────    ─────────────────────────────   ───────────────────────────────
  0      HPA(pfn=200) | 硬件权限(R/W)    (gfn=200 << 12) | ACC_WRITE|ACC_USER
  1      HPA(pfn=350) | 硬件权限(R)      (gfn=350 << 12) | ACC_EXEC|ACC_USER
         ← KVM 因 iTLB 缓解清了 X 位      ← 客户机原始权限：可执行
  2      HPA(pfn=201) | 硬件权限(R)      (gfn=201 << 12) | ACC_WRITE|ACC_USER
         ← dirty log 开启，清了 W 位       ← 客户机原始权限：可写

spt 是给 CPU 看的真实页表；shadowed_translation 是 KVM 保留的"客户机原始翻译意图"，
用于 reverse map 查找和权限恢复。


启示，shadowed_translation 中的内容和 guest page table 中的内容基本一致了:

本质区别总结
┌──────────────┬─────────────────────────┬────────────────────────────────────────────┐
│              │       客户机 GPTE       │          shadowed_translation[N]           │
├──────────────┼─────────────────────────┼────────────────────────────────────────────┤
│ 格式         │ x86 硬件格式            │ KVM 内部格式                               │
├──────────────┼─────────────────────────┼────────────────────────────────────────────┤
│ 位宽         │ 64 bit，字段很多        │ 64 bit，只用高 52 位(GFN) + 低 3 位(权限)  │
├──────────────┼─────────────────────────┼────────────────────────────────────────────┤
│ 权限编码     │ P/R/W/U/S/NX 原始硬件位 │ ACC_EXEC|WRITE|USER，经 gpte_access() 转换 │
├──────────────┼─────────────────────────┼────────────────────────────────────────────┤
│ A/D/G/PAT 等 │ 有                      │ 无                                         │
├──────────────┼─────────────────────────┼────────────────────────────────────────────┤
│ 时效性       │ 客户机随时修改          │ 只在同步时更新，unsync 期间可能过时        │
├──────────────┼─────────────────────────┼────────────────────────────────────────────┤
│ 用途         │ CPU walk 页表用         │ KVM 做 reverse map 和权限判断用            │
└──────────────┴─────────────────────────┴────────────────────────────────────────────┘

shadowed_translation 是从 GPTE 提炼出来的缓存，只保留 KVM 自己需要的那一小部分信息，
格式也为 KVM 内部使用做了规整。

#### rmap 和 shadowed_translation 是互为反向的两个结构。
<!-- 36921377-ccf1-47a0-a34e-f11bde34e622 -->

(认为非常对，还是为了 rmap 机制，shadowed_translation 可以让 sptep 可以快速找到 gfn)

两个结构的关系:

rmap（正向）:   GFN ──────────────► [sptep, sptep, sptep, ...]
                                     所有映射该 GFN 的 SPTE 指针列表

shadowed_translation（反向）:
                sptep ─────────────► GFN + access
                已知 SPTE 指针，找出它映射的是哪个 GFN

rmap 的存储结构

// 存在 memslot 里，按 level 和 GFN 索引
struct kvm_arch_memory_slot {
    struct kvm_rmap_head *rmap[KVM_NR_PAGE_SIZES];  // [level][gfn_offset]
};

// 每个 rmap_head 是一个 sptep 指针链表
struct kvm_rmap_head {
    atomic_long_t val;
    // 若只有 1 个 SPTE: val 直接存该 sptep
    // 若有多个 SPTE:    val 指向 pte_list_desc 链
};

struct pte_list_desc {
    struct pte_list_desc *more;   // 下一个描述符
    u32 spte_count;
    u64 *sptes[14];               // 最多 14 个 sptep
};

增删操作如何配合

增加（__rmap_add）：

sp  = sptep_to_sp(spte);
kvm_mmu_page_set_translation(sp, spte_index(spte), gfn, access);
//  ────────────────────────────────────────────────────────────
//  shadowed_translation[index] = (gfn << PAGE_SHIFT) | access
//  记录：这个 SPTE 槽位 → 映射的是 gfn

rmap_head = gfn_to_rmap(gfn, sp->role.level, slot);
//          slot->arch.rmap[level][gfn_to_index(gfn)]
pte_list_add(kvm, cache, spte, rmap_head);
//  记录：gfn → 追加这个 sptep 指针

删除（rmap_remove）：

static void rmap_remove(struct kvm *kvm, u64 *spte)
{
    sp  = sptep_to_sp(spte);
    gfn = kvm_mmu_page_get_gfn(sp, spte_index(spte));
    //    ─────────────────────────────────────────────
    //    读 shadowed_translation[index] >> PAGE_SHIFT
    //    问题：只有 sptep，不知道对应哪个 GFN
    //    答案：从 shadowed_translation 里查
    rmap_head = gfn_to_rmap(gfn, sp->role.level, slot);
    pte_list_remove(kvm, spte, rmap_head);
}

shadowed_translation 在这里的核心作用：删除时只有 sptep，没有 GFN。必须先通过
shadowed_translation 查出 GFN，才能找到对应的 rmap 桶，才能从中摘除这个 sptep。

完整的使用场景

宿主机物理页被换出或迁移时，KVM 通过 rmap 找到所有 SPTE 并 zap：

宿主机通知：pfn X 要被回收
    │
    ▼
通过 pfn→gfn 找到对应的 GFN
    │
    ▼
gfn_to_rmap(gfn, level, slot)
    │
    ▼
遍历 rmap_head 链表，得到所有 sptep
    │
    ├─► zap sptep1 → 调用 rmap_remove(sptep1)
    │       └─ shadowed_translation → 得到 gfn → 从 rmap 删除 sptep1
    │
    ├─► zap sptep2 → 调用 rmap_remove(sptep2)
    │       └─ shadowed_translation → 得到 gfn → 从 rmap 删除 sptep2
    └─► ...

一句话总结

rmap:                GFN → [sptep 列表]   用于"给定物理页，找所有映射它的 SPTE"
shadowed_translation: sptep → GFN        用于"给定 SPTE，找它映射的是哪个 GFN"

两者互为反向索引，缺一不可。
shadowed_translation 是 rmap 能正确维护（特别是删除）的前提。


### kvm_mmu_page::role

```c
/*
 * kvm_mmu_page_role tracks the properties of a shadow page (where shadow page
 * also includes TDP pages) to determine whether or not a page can be used in
 * the given MMU context.  This is a subset of the overall kvm_cpu_role to
 * minimize the size of kvm_memory_slot.arch.gfn_write_track, i.e. allows
 * allocating 2 bytes per gfn instead of 4 bytes per gfn.
 *
 * Upper-level shadow pages having gptes are tracked for write-protection via
 * gfn_write_track.  As above, gfn_write_track is a 16 bit counter, so KVM must
 * not create more than 2^16-1 upper-level shadow pages at a single gfn,
 * otherwise gfn_write_track will overflow and explosions will ensue.
 *
 * A unique shadow page (SP) for a gfn is created if and only if an existing SP
 * cannot be reused.  The ability to reuse a SP is tracked by its role, which
 * incorporates various mode bits and properties of the SP.  Roughly speaking,
 * the number of unique SPs that can theoretically be created is 2^n, where n
 * is the number of bits that are used to compute the role.
 *
 * But, even though there are 20 bits in the mask below, not all combinations
 * of modes and flags are possible:
 *
 *   - invalid shadow pages are not accounted, mirror pages are not shadowed,
 *     so the bits are effectively 18.
 *
 *   - quadrant will only be used if has_4_byte_gpte=1 (non-PAE paging);
 *     execonly and ad_disabled are only used for nested EPT which has
 *     has_4_byte_gpte=0.  Therefore, 2 bits are always unused.
 *
 *   - the 4 bits of level are effectively limited to the values 2/3/4/5,
 *     as 4k SPs are not tracked (allowed to go unsync).  In addition non-PAE
 *     paging has exactly one upper level, making level completely redundant
 *     when has_4_byte_gpte=1.
 *
 *   - on top of this, smep_andnot_wp and smap_andnot_wp are only set if
 *     cr0_wp=0, therefore these three bits only give rise to 5 possibilities.
 *
 * Therefore, the maximum number of possible upper-level shadow pages for a
 * single gfn is a bit less than 2^13.
 */
union kvm_mmu_page_role {
	u32 word;
	struct {
		unsigned level:4;
		unsigned has_4_byte_gpte:1;
		unsigned quadrant:2;
		unsigned direct:1;
		unsigned access:3;
		unsigned invalid:1;
		unsigned efer_nx:1;
		unsigned cr0_wp:1;
		unsigned smep_andnot_wp:1;
		unsigned smap_andnot_wp:1;
		unsigned ad_disabled:1;
		unsigned guest_mode:1;
		unsigned passthrough:1;
		unsigned is_mirror:1;
		unsigned :4;

		/*
		 * This is left at the top of the word so that
		 * kvm_memslots_for_spte_role can extract it with a
		 * simple shift.  While there is room, give it a whole
		 * byte so it is also faster to load it from memory.
		 */
		unsigned smm:8;
	};
};
```

role.level:
   The level in the shadow paging hierarchy that this shadow page belongs to.
   1=4k sptes, 2=2M sptes, 3=1G sptes, etc.


role.direct:
  If set, leaf sptes reachable from this page are for a linear range.
  Examples include real mode translation, large guest pages backed by small
  host pages, and gpa->hpa translations when NPT or EPT is active.
  The linear range starts at (gfn << PAGE_SHIFT) and its size is determined
  by role.level (2MB for first level, 1GB for second level, 0.5TB for third
  level, 256TB for fourth level)
  If clear, this page corresponds to a guest page table denoted by the gfn
  field.


- [] shadow paging 机制似乎是独立于 guest 的类型，不然 for_each_shadow_entry 需要很多种吧
  - [ ] 对于 ept，其实模拟 gpa 的大小
  - [ ] 对于 shadow, 取决于 guest。找到默认设定 guest 的 paging 方式代码的位置

role.quadrant:
  When role.gpte_is_8_bytes=0, the guest uses 32-bit gptes while the host uses 64-bit
  sptes.  That means a guest page table contains more ptes than the host,
  so multiple shadow pages are needed to shadow one guest page.
  For first-level shadow pages, role.quadrant can be 0 or 1 and denotes the
  first or second 512-gpte block in the guest page table.  For second-level
  page tables, each 32-bit gpte is converted to two 64-bit sptes
  (since each first-level guest page is shadowed by two first-level
  shadow pages) so role.quadrant takes values in the range 0..3.  Each
  quadrant maps 1GB virtual address space.

Only two function related with `kvm_mmu_get_page` and `get_written_sptes`
1. `kvm_mmu_get_page` : alloc a page as page table
2. `get_written_sptes`

#### role.passthrough

> role.passthrough:
> The page is not backed by a guest page table, but its first entry points to one. This is set if NPT uses 5-level page tables (host CR4.LA57=1) and is shadowing L1’s 4-level NPT (L1 CR4.LA57=0).


> [!NOTE]
> 参考神奇海螺的意见，有待验证

role.passthrough 的场景

这是一个特殊的层级拼接问题：

宿主机：CR4.LA57=1 → 使用 5 级页表（PML5）
L1 客户机：CR4.LA57=0 → 使用 4 级页表（PML4）

NPT 需要影射 L1 的 4 级 NPT，但宿主机的硬件 MMU 走 5 级页表。
KVM 必须在顶部插入一个"假"的第 5 级 shadow page：

宿主机硬件期望的 5 级结构：
  PML5 shadow page（passthrough）← role.passthrough=1
    └─ entry[0] → PML4 shadow page（影射 L1 真实的 NPT 根）
                    └─ ...（正常的 4 级影射）

passthrough shadow page 的特点：
  - 没有对应的客户机页表（L1 根本没有 L5 页表）
  - 但它的第 0 个 entry 指向 L1 真实页表的 shadow
  - 纯粹为了弥补层级差距而存在

commit 84e5ffd045f3 ("KVM: X86/MMU: Fix shadowing 5-level NPT for 4-level NPT L1 guest")

也就是，按照这里的:
```c
static gfn_t kvm_mmu_page_get_gfn(struct kvm_mmu_page *sp, int index)
{
	if (sp->role.passthrough)
		return sp->gfn;

	if (sp->shadowed_translation)
		return sp->shadowed_translation[index] >> PAGE_SHIFT;

	return sp->gfn + (index << ((sp->role.level - 1) * SPTE_LEVEL_BITS));
}
```

### kvm_mmu_page::parent_ptes

为什么两者是一个 union 的存在:
```txt
	union {
		struct kvm_rmap_head parent_ptes; /* rmap pointers to parent sptes */
		tdp_ptep_t ptep;
	};
```
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

## reverse map
The mmu maintains a reverse mapping whereby all ptes mapping a page can be
reached given its gfn.  This is used, for example, when swapping out a page.

这个文档什么都没讲啊

## synchronized and unsynchronized pages
<!-- 87a5cf4f-ef95-4579-912f-856a65652336 -->

The guest uses two events to synchronize its tlb and page tables: tlb flushes and page invalidations (invlpg).

A tlb flush means that we need to synchronize all sptes reachable from the guest’s cr3.
This is expensive, so we keep all guest page tables write protected,
and synchronize sptes to gptes when a gpte is written.

A special case is when a guest page table is reachable from the current guest cr3.
In this case, the guest is obliged to issue an invlpg instruction before using the translation.
We take advantage of that by removing write protection from the guest page,
and allowing the guest to modify it freely.
We synchronize modified gptes when the guest invokes invlpg.
This reduces the amount of emulation we have to do when the guest modifies multiple gptes,
or when the a guest page is no longer used as a page table and is used for random guest data.

As a side effect we have to resynchronize all reachable unsynchronized shadow pages on a tlb flush.

```txt
→ arch/x86/kvm/x86.c:: kvm_make_request(KVM_REQ_MMU_SYNC)
→ arch/x86/kvm/x86.c:: kvm_mmu_sync_roots(vcpu)
→ arch/x86/kvm/mmu/mmu.c:: kvm_mmu_sync_roots()
→ arch/x86/kvm/mmu/mmu.c:: mmu_sync_children(vcpu, sp, true)
→ arch/x86/kvm/mmu/mmu.c:: mmu_sync_children()
    → mmu_unsync_walk() 遍历所有unsync pages
    → kvm_sync_page() 同步每个unsync page
        → __kvm_sync_page()
            → kvm_sync_spte() 同步每个SPTE
                → paging_tmpl.h:895: FNAME(sync_spte)()
                    读取guest页表的gpte，更新shadow页表的spte
```
### 基础

```txt
标准情况（Sync）：           优化情况（Unsync）：
┌──────────┐                ┌──────────┐
│ gpte     │──→ WP=1        │ gpte     │──→ WP=0（可写）
│ 写保护   │   每次写都trap │ 可写     │   自由修改
└──────────┘                └──────────┘
     ↓                           ↓
  同步 spte                  延迟到 INVLPG
                             再批量同步

关键条件：只有当前 CR3 可达的页表才能设为 unsync（可写），因为 guest 必须在用之前执行 INVLPG。
```

### 结合源码分析

```c
static void kvm_vcpu_flush_tlb_guest(struct kvm_vcpu *vcpu)
{
	++vcpu->stat.tlb_flush;

	if (!tdp_enabled) {
		/*
		 * A TLB flush on behalf of the guest is equivalent to
		 * INVPCID(all), toggling CR4.PGE, etc., which requires
		 * a forced sync of the shadow page tables.  Ensure all the
		 * roots are synced and the guest TLB in hardware is clean.
		 */
		kvm_mmu_sync_roots(vcpu);
		kvm_mmu_sync_prev_roots(vcpu);
	}

	kvm_x86_call(flush_tlb_guest)(vcpu);

	/*
	 * Flushing all "guest" TLB is always a superset of Hyper-V's fine
	 * grained flushing.
	 */
	kvm_hv_vcpu_purge_flush_tlb(vcpu);
}
```
当 guest 出现了 tlb flush 的时候，那么这个 vcpu 的所有的 shadow page table 全部都需要清理掉
原因也很简单，guest os 使用 tlb flush 的时候，我们搞不清楚，他当时修改了多少 page table ，
所以，不如所有的 page table 全部都清理掉。

## Fast invalidation of MMIO sptes
<!-- 28cf9d6b-d7ab-4dc1-ad60-96970c3fac8a -->
核心要义，这是 RCU 的经典问题了:

当 rcu 释放的时候，那么所有引用的资源都该释放，但是 mmio cache 的内容按道理
这个时候应该是 rcu callback 来清理的，但是那样集中了。
那么，解决办法就是，记录一个 generation id ，如果我使用的已经释放了，


### 具体分析
page table 中是需要缓存的，如果每次 page table 到最后，
结果发现，page table 都是没有任何的内容，然后检查一下 memslots ，那效率就太低了。

当 memslot 添加或者减少的时候，那么 page table 中的缓存可能不对了。

解决办法是:
1. 创建 MMIO spte 时：将当前全局 generation number 写入 spte
2. 访问 MMIO spte 时：检查 spte 的 generation 是否等于全局 generation
	- 相等：缓存有效，使用缓存的 MMIO 信息
	- 不相等：缓存失效，走慢路径重新处理页故障

Unfortunately, a single memory access might access kvm_memslots(kvm) multiple
times, the last one happening when the generation number is retrieved and
stored into the MMIO spte.  Thus, the MMIO spte might be created based on
out-of-date information, but with an up-to-date generation number.

存在一种复杂的竞态：单个内存访问可能多次访问 kvm_memslots(kvm)，最后一次才是获取 generation number 并存入 spte。这可能导致：
- MMIO spte 基于过时的信息创建
- 但却带有最新的 generation number

所以，结果是:
```txt
// 假设 kvm_memslots(kvm)->generation 初始为 10

// 处理 MMIO 访问时：
slots = kvm_memslots(kvm);        // 第 1 次：看到 generation = 10
...
slots = kvm_memslots(kvm);        // 第 2 次：可能仍然看到 generation = 10
...
// 此时另一个 CPU 修改了 memslot，generation 变为 11
...
slots = kvm_memslots(kvm);        // 第 3 次：看到 generation = 11

// 问题：spte 基于第 1/2 次的信息创建，但使用第 3 次的 generation = 11
```


解决方案：利用 Bit 63

Bit 63 = 1 表示更新正在进行（"update in-progress"）

- 在 synchronize_srcu() 返回后，generation number 递增，bit 63 被设为 1
- bit 63 不会存入 MMIO spte（implicitly zero）
- 这样即使 generation number 匹配，bit 63 的差异也会导致缓存失效

效果：如果 KVM 不幸在更新窗口期间创建了 MMIO spte，下次访问时：
- 更新期间访问：in-progress 标志不匹配 → cache miss
- 更新完成后访问：generation number 更高 → cache miss
这种机制避免了遍历所有 shadow 页来失效 MMIO spte，显著提升了可扩展性。

这是一个非常经典的问题了:

- [ ] To avoid this, the generation number is incremented again after synchronize_srcu
	- 这个 again 在什么地方?

```txt
To avoid this, the generation number is incremented again after synchronize_srcu
returns; thus, bit 63 of kvm_memslots(kvm)->generation set to 1 only during a
memslot update, while some SRCU readers might be using the old copy.

We do not want to use an MMIO sptes created with an odd generation number, and we can do
this without losing a bit in the MMIO spte.  The "update in-progress" bit of the
generation is not stored in MMIO spte, and is so is implicitly zero when the
generation is extracted out of the spte.  If KVM is unlucky and creates an MMIO
spte while an update is in-progress, the next access to the spte will always be
a cache miss.  For example, a subsequent access during the update window will
miss due to the in-progress flag diverging, while an access after the update
window closes will have a higher generation number (as compared to the spte).
```

```txt
kvm_vm_ioctl_set_memory_region()
  └─> kvm_set_memory_region()
        └─> kvm_set_memslot()
              ├─> [DELETE/MOVE] kvm_invalidate_memslot()
              │     └─> kvm_swap_active_memslots()  ← 第 1 次调用
              │
              ├─> [CREATE/MOVE/FLAGS_ONLY] kvm_activate_memslot()
              │     └─> kvm_swap_active_memslots()  ← 第 2 次调用
              │
              └─> kvm_commit_memory_region()
```

关键函数：kvm_swap_active_memslots() —— 实际更新 generation 的地方

其实这一段就是在解释 kvm_swap_active_memslots 这个函数。

```c
  static void kvm_swap_active_memslots(struct kvm *kvm, int as_id)
  {
      struct kvm_memslots *slots = kvm_get_inactive_memslots(kvm, as_id);

      // 获取当前 generation（必须是偶数，即不在更新中）
      u64 gen = __kvm_memslots(kvm, as_id)->generation;
      WARN_ON(gen & KVM_MEMSLOT_GEN_UPDATE_IN_PROGRESS);  // 确保没有正在进行的更新

      // 关键：设置 Bit 63 为 1，标记"更新进行中"
      slots->generation = gen | KVM_MEMSLOT_GEN_UPDATE_IN_PROGRESS;

      // 等待所有正在进行的 invalidation 完成
      spin_lock(&kvm->mn_invalidate_lock);
      while (kvm->mn_active_invalidate_count) {
          set_current_state(TASK_UNINTERRUPTIBLE);
          spin_unlock(&kvm->mn_invalidate_lock);
          schedule();
          spin_lock(&kvm->mn_invalidate_lock);
      }

      // 原子切换 active/inactive memslots
      rcu_assign_pointer(kvm->memslots[as_id], slots);
      spin_unlock(&kvm->mn_invalidate_lock);

      mutex_unlock(&kvm->slots_arch_lock);

      // 等待所有 SRCU reader 完成（确保没有 vCPU 在使用旧的 memslots）
      synchronize_srcu_expedited(&kvm->srcu);

      // 结束动作（清除标志并递增 generation）

      // 清除 Bit 63（in-progress 标志）
      gen = slots->generation & ~KVM_MEMSLOT_GEN_UPDATE_IN_PROGRESS;

      // 递增 generation（确保跨地址空间唯一）
      // 例如：2 个地址空间时，地址空间 0 使用 0,2,4...，地址空间 1 使用 1,3,5...
      gen += kvm_arch_nr_memslot_as_ids(kvm);

      // 通知架构代码（x86 会更新 MMIO spte 的 generation 检查）
      kvm_arch_memslots_updated(kvm, gen);

      // 最终设置新的 generation（偶数，表示更新完成）
      slots->generation = gen;
  }
```

include/linux/kvm_host.h 中的:
```c
/*
 * Bit 63 of the memslot generation number is an "update in-progress flag",
 * e.g. is temporarily set for the duration of kvm_swap_active_memslots().
 * This flag effectively creates a unique generation number that is used to
 * mark cached memslot data, e.g. MMIO accesses, as potentially being stale,
 * i.e. may (or may not) have come from the previous memslots generation.
 *
 * This is necessary because the actual memslots update is not atomic with
 * respect to the generation number update.  Updating the generation number
 * first would allow a vCPU to cache a spte from the old memslots using the
 * new generation number, and updating the generation number after switching
 * to the new memslots would allow cache hits using the old generation number
 * to reference the defunct memslots.
 *
 * This mechanism is used to prevent getting hits in KVM's caches while a
 * memslot update is in-progress, and to prevent cache hits *after* updating
 * the actual generation number against accesses that were inserted into the
 * cache *before* the memslots were updated.
 */
#define KVM_MEMSLOT_GEN_UPDATE_IN_PROGRESS	BIT_ULL(63)
```

### 基础知识

我们关于的点在于 struct kvm_memory_slot 和 kvm_memslots ，
generation number 是记录在 kvm_memslots 中的
```c
struct kvm_memslots {
	u64 generation;
	atomic_long_t last_used_slot;
	struct rb_root_cached hva_tree;
	struct rb_root gfn_tree;
	/*
	 * The mapping table from slot id to memslot.
	 *
	 * 7-bit bucket count matches the size of the old id to index array for
	 * 512 slots, while giving good performance with this slot count.
	 * Higher bucket counts bring only small performance improvements but
	 * always result in higher memory usage (even for lower memslot counts).
	 */
	DECLARE_HASHTABLE(id_hash, 7);
	int node_idx;
};
```

```c
static inline struct kvm_memslots *__kvm_memslots(struct kvm *kvm, int as_id)
{
	as_id = array_index_nospec(as_id, KVM_MAX_NR_ADDRESS_SPACES);
	return srcu_dereference_check(kvm->memslots[as_id], &kvm->srcu,
			lockdep_is_held(&kvm->slots_lock) ||
			!refcount_read(&kvm->users_count));
}

static inline struct kvm_memslots *kvm_memslots(struct kvm *kvm)
{
	return __kvm_memslots(kvm, 0);
}
```
kvm_memslots 是 RCU 保护的，注意，是 kvm_memslots 而不是 kvm_memory_slot

### 分析一个 patch
<!-- 6f7c26f5-bd86-45b4-a331-5fb99d5565cd -->

commit ee3d1570b586 ("kvm: fix potentially corrupt mmio cache")
commit 164bf7e56c5a ("KVM: Move the memslot update in-progress flag to bit 63") // 应该是这个 commit 让表述不对了


> [!NOTE]
> 参考神奇海螺的意见，有待验证

  核心概念仍然正确：使用一个标志位来标记"更新进行中"，使得在这个期间创建的 MMIO spte 会被视为无效。

  不准确的表述：

  • 文档说 "incremented again"（再次递增）
  • 现在的代码实际上是：
    1. 第一次：设置 Bit 63（不是递增）
    2. 第二次：清除 Bit 63 并递增（真正的递增）

  建议的文档修改：

  -To avoid this, the generation number is incremented again after synchronize_srcu
  -returns; thus, bit 63 of kvm_memslots(kvm)->generation set to 1 only during a
  +To avoid this, the generation number is updated after synchronize_srcu
  +returns; first by setting bit 63 to 1 before synchronize_srcu, then clearing
  +bit 63 and incrementing the generation after synchronize_srcu completes. Thus,
  +bit 63 of kvm_memslots(kvm)->generation is set to 1 only during a

  所以你的观察是对的——文档的表述反映了历史实现，虽然核心机制（利用一个位来避免竞争条件）没变，但具体的"两次递增"描述已经不再准确。


### vcpu->arch.mmio_gen 是什么东西?

似乎不是太相关?
```c
static inline bool vcpu_match_mmio_gen(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.mmio_gen == kvm_memslots(vcpu->kvm)->generation;
}
```
## Emulating cr0.wp
<!-- 2ac0886d-1aa3-4d5b-948f-d3c4f06874a6 -->

首先，在物理机环境中，CR0.WP 语义是:
CR0.WP = 1
	内核态也必须遵守页表的 W 位
	即：U=1, W=0 → 内核 不能写
CR0.WP = 0
	内核态可以无视 W 位
	即：U=1, W=0 →
	内核：可读 + 可写
	用户：只读

CR0.WP (Write Protect) 位的作用：
- 当 CR0.WP=1 时：内核态也受页表 W 位保护（现代操作系统的正常模式）
- 当 CR0.WP=0 时：内核态可以写任何页面，即使页表项标记为只读

所以问题就是在 cro.wp=0 spte.u=1 spte.w=0 的时候，这个时候需要特殊的处理。

如果是硬件模拟，大致如何实现:
- 不考虑虚拟化
	- page walk 的时候自动和 cr0.wp 比较
- 考虑虚拟化
	- 在 Guest page table 的 walk 的时候，自动和 cr0.wp 比较，和 ept 没有关系

TDP 未启用（shadow paging）的时候，虽然 shadow page table 在 non-root mode 中 page walk 的时候，
CPU 是可以访问到 page table 的 U 位的，而且是可以知道当前 CPU 是 user 还是 kernel mode 的，
所以，为什么硬件中不能直接支持 cr0.wp 我还是不能理解，先当做已知问题吧，可以简单理解为这个问题
在于内核

问题场景

当 guest 设置 CR0.WP=0 时，guest 页表项 gpte.u=1, gpte.w=0 的语义是：
- 允许内核态任何访问（读、写、执行）
- 允许用户态读访问
- 禁止用户态写访问

但这个语义无法直接映射到单个 SPTE，因为宿主机 CR0.WP=1。
这是非常奇怪的假设，我不知道为什么就会有了这个东西?

KVM 的解决方案

KVM 采用双 SPTE 策略，根据缺页类型动态选择：

1. 内核写缺页 (Documentation/virt/kvm/x86/mmu.rst:413-414)

spte.u=0, spte.w=1
- 允许完整的内核访问
- 禁止用户态访问

2. 读缺页 (Documentation/virt/kvm/x86/mmu.rst:415-416)

spte.u=1, spte.w=0
- 允许完整的读访问（内核+用户态）
- 禁止内核写访问

用户写缺页直接注入 #PF 给 guest。

额外复杂性
- 问题 1: CR4.SMEP
SMEP (Supervisor Mode Execution Prevention): 防止内核执行用户态页面。
问题： 当转换为内核页（spte.u=0）时，内核可能执行原本的用户页。

解决：
- 设置 spte.nx=1 禁止执行
- 当发生用户态 fetch/read 缺页时，恢复 spte.u=1, spte.nx=gpte.nx
- KVM 在影子分页时强制 EFER.NX=1

- 问题 2: CR4.SMAP
SMAP (Supervisor Mode Access Prevention): 防止内核访问用户态页面。
问题： 页面被转换为内核页后，CR4.SMAP 启用时无法重用。
解决： 将 CR4.SMAP && !CR0.WP 加入 shadow page 的 role。

Page Role 机制
为了防止不一致，KVM 将以下状态编码到 shadow page 的 role 中：

1. role.cr0_wp (第185-186行): CR0.WP 的值
2. role.smep_andnot_wp (第187-190行): CR4.SMEP && !CR0.WP
3. role.smap_andnot_wp (第191-194行): CR4.SMAP && !CR0.WP

```txt
role.cr0_wp:
Contains the value of cr0.wp for which the page is valid.

role.smep_andnot_wp:
Contains the value of cr4.smep && !cr0.wp for which the page is valid (pages for which this is true are different from other pages; see the treatment of cr0.wp=0 below).

role.smap_andnot_wp:
Contains the value of cr4.smap && !cr0.wp for which the page is valid (pages for which this is true are different from other pages; see the treatment of cr0.wp=0 below).
```

效果： CR0.WP 改变后，旧的 SPTE 会在查找时自动失效（role 不匹配），强制重新创建符合新语义的 SPTE。

具体的代码分析:
```c
static int FNAME(page_fault)(struct kvm_vcpu *vcpu, struct kvm_page_fault *fault)
{
    struct guest_walker walker;
    int r;

    // ... 前面代码：页表遍历、权限检查 ...

#if PTTYPE != PTTYPE_EPT
    /*
     * 将 guest PTE 权限视为 可写、仅内核态 如果：
     * - 这是一个内核写缺页
     * - CR0.WP=0（内核访问忽略 PTE.W）
     */
    if (fault->write &&                              // 1. 是写缺页
        !(walker.pte_access & ACC_WRITE_MASK) &&    // 2. guest PTE 不可写
        !is_cr0_wp(vcpu->arch.mmu) &&               // 3. CR0.WP=0
        !fault->user &&                              // 4. 内核态访问
        fault->slot) {                               // 5. 不是 MMIO

        walker.pte_access |= ACC_WRITE_MASK;         // 添加写权限
        walker.pte_access &= ~ACC_USER_MASK;         // 移除用户位（变成内核页）

        /*
         * 如果我们将用户页转换为内核页，
         * 以便内核在 cr0.wp=0 时可以写入，
         * 那么如果启用了 SMEP，我们应该防止内核执行它。
         */
        if (is_cr4_smep(vcpu->arch.mmu))
            walker.pte_access &= ~ACC_EXEC_MASK;     // 移除执行权限
    }
#endif

    // 使用调整后的 walker.pte_access 创建 SPTE
    r = RET_PF_RETRY;
    write_lock(&vcpu->kvm->mmu_lock);

    if (is_page_fault_stale(vcpu, fault))
        goto out_unlock;

    r = make_mmu_pages_available(vcpu);
    if (r)
        goto out_unlock;
    r = FNAME(fetch)(vcpu, fault, &walker);

out_unlock:
    kvm_mmu_finish_page_fault(vcpu, fault, r);
    write_unlock(&vcpu->kvm->mmu_lock);
    return r;
}
```

**这就是双 SPTE 策略的核心实现！**

场景 1：内核写缺页（fault->write=1, fault->user=0）
- **输入**：`gpte.u=1, gpte.w=0`
- **条件判断全部满足**
- **转换后权限**：`ACC_WRITE_MASK | ~ACC_USER_MASK | ~ACC_EXEC_MASK`
- **结果 SPTE**：`spte.u=0, spte.w=1, spte.nx=1`（内核可写，不可执行）

场景 2：读缺页（fault->write=0）
- **条件 `fault->write` 不满足**，不执行转换
- **保持原始权限**：`ACC_USER_MASK | ~ACC_WRITE_MASK`
- **结果 SPTE**：`spte.u=1, spte.w=0`（用户可读，不可写）

```c
static void update_permission_bitmask(struct kvm_mmu *mmu, bool ept)
{
    unsigned byte;

    const u8 x = BYTE_MASK(ACC_EXEC_MASK);
    const u8 w = BYTE_MASK(ACC_WRITE_MASK);
    const u8 u = BYTE_MASK(ACC_USER_MASK);

    bool cr4_smep = is_cr4_smep(mmu);
    bool cr4_smap = is_cr4_smap(mmu);
    bool cr0_wp = is_cr0_wp(mmu);
    bool efer_nx = is_efer_nx(mmu);

    for (byte = 0; byte < ARRAY_SIZE(mmu->permissions); ++byte) {
        unsigned pfec = byte << 1;  // Page Fault Error Code

        /*
         * 每个 "*f" 变量对应一种错误条件：
         * 如果该位为 1，表示对应的 UWX 值会导致缺页
         */

        /* 写非可写页面的错误 */
        u8 wf = (pfec & PFERR_WRITE_MASK) ? (u8)~w : 0;
        /* 用户态访问内核页面的错误 */
        u8 uf = (pfec & PFERR_USER_MASK) ? (u8)~u : 0;
        /* Fetch 不可执行页面的错误 */
        u8 ff = (pfec & PFERR_FETCH_MASK) ? (u8)~x : 0;
        /* SMEP 相关错误 */
        u8 smepf = 0;
        /* SMAP 相关错误 */
        u8 smapf = 0;

        if (!ept) {
            /* 内核态访问用户页 */
            u8 kf = (pfec & PFERR_USER_MASK) ? 0 : u;

            /* 如果 EFER.NX=0，忽略 NX 检查 */
            if (!efer_nx)
                ff = 0;

            /* ===== 关键：CR0.WP=0 的处理 ===== */
            /* 如果 !cr0.wp，允许内核态写 */
            if (!cr0_wp)
                wf = (pfec & PFERR_USER_MASK) ? wf : 0;
            // 意思是：如果是内核态访问（!PFERR_USER_MASK），清除写错误标志

            /* 如果 cr4.smep，禁止内核从用户页 fetch */
            if (cr4_smep)
                smepf = (pfec & PFERR_FETCH_MASK) ? kf : 0;

            /*
             * SMAP: 内核态数据访问用户态映射时应该出错
             * 满足以下所有条件时视为 SMAP 违规：
             *   - CR4.SMAP=1
             *   - 访问用户页
             *   - 不是 fetch 访问
             *   - 内核态访问
             *   - 隐式访问或 EFLAGS.AC=0
             *
             * 这里覆盖前四个条件
             * 第五个条件在 permission_fault() 中动态计算
             * 如果访问不受 SMAP 限制，PFEC 中会设置 PFERR_RSVD_MASK
             */
            if (cr4_smap)
                smapf = (pfec & (PFERR_RSVD_MASK|PFERR_FETCH_MASK)) ? 0 : kf;
        }

        mmu->permissions[byte] = ff | uf | wf | smepf | smapf;
    }
}
```
**这个函数预计算权限检查位掩码**，用于快速判断一个访问是否会产生 #PF。

当 CR0.WP=0 时：
- 内核态写访问（`!(pfec & PFERR_USER_MASK)`）**不会触发写保护错误**
- 用户态写访问仍然会触发错误


```c
void __kvm_mmu_refresh_passthrough_bits(struct kvm_vcpu *vcpu,
                                        struct kvm_mmu *mmu)
{
    const bool cr0_wp = kvm_is_cr0_bit_set(vcpu, X86_CR0_WP);

    BUILD_BUG_ON((KVM_MMU_CR0_ROLE_BITS & KVM_POSSIBLE_CR0_GUEST_BITS) != X86_CR0_WP);
    BUILD_BUG_ON((KVM_MMU_CR4_ROLE_BITS & KVM_POSSIBLE_CR4_GUEST_BITS));

    /* 如果 CR0.WP 没有变化，直接返回 */
    if (is_cr0_wp(mmu) == cr0_wp)
        return;

    /* 更新 role 中的 CR0.WP 位 */
    mmu->cpu_role.base.cr0_wp = cr0_wp;

    /* 重新计算权限位掩码 */
    reset_guest_paging_metadata(vcpu, mmu);
}
```

当 guest 修改 CR0.WP 时：
1. 更新 `mmu->cpu_role.base.cr0_wp`
2. 调用 `reset_guest_paging_metadata()` 重新计算权限位掩码
3. 旧的 shadow page 因 role 不匹配而在查找时自动失效


### 具体例子

```txt
初始状态：
- Guest CR0.WP = 0
- Guest PTE: u=1, w=0 (用户页，只读)
- Host CR0.WP = 1 (宿主机必须保持)
- 没有对应的 SPTE（首次访问）
```

第一步：内核写访问

```
Guest 执行: mov [addr], eax  (内核态，addr 对应 gpte.u=1, w=0)
       ↓
CPU: 触发 EPT Violation / NPT Page Fault
     error_code = PFERR_WRITE_MASK (不包含 USER_MASK)
     原因：没有 SPTE 映射
       ↓
KVM: 调用 FNAME(page_fault)() (paging_tmpl.h:765)
     ↓
     1. 调用 FNAME(walk_addr)() 遍历 guest 页表
        - 得到 walker.pte_access = ACC_USER_MASK (从 gpte.u=1 得到)
     ↓
     2. 检测到 fault->write=1, !fault->user, !cr0_wp (paging_tmpl.h:814)
        条件全部满足，执行权限转换：
        - walker.pte_access |= ACC_WRITE_MASK   // 添加写权限
        - walker.pte_access &= ~ACC_USER_MASK   // 移除用户位（变成内核页）
        - if (SMEP) walker.pte_access &= ~ACC_EXEC_MASK  // 移除执行权限

        转换结果：walker.pte_access = ACC_WRITE_MASK (+ ~ACC_EXEC_MASK if SMEP)
     ↓
     3. 调用 FNAME(fetch)() 创建 SPTE (paging_tmpl.h:614)
        - 使用转换后的 walker.pte_access
        - 创建 SPTE: spte.u=0, spte.w=1, spte.nx=1 (如果 SMEP=1)
        - 将 SPTE 写入影子页表
     ↓
Guest: 内核写成功，继续执行
       现在的 SPTE: u=0, w=1, nx=1
```

第二步：用户读访问（同一地址）

```
Guest 执行: mov eax, [addr]  (用户态)
       ↓
CPU: 触发 Page Fault
     error_code = PFERR_USER_MASK (不包含 WRITE_MASK)
     原因：spte.u=0，用户态无法访问
       ↓
KVM: 调用 FNAME(page_fault)()
     ↓
     1. 调用 FNAME(walk_addr)() 遍历 guest 页表
        - 得到 walker.pte_access = ACC_USER_MASK
     ↓
     2. 检测条件：fault->write=0
        条件不满足，不执行权限转换
        保持 walker.pte_access = ACC_USER_MASK (可读)
     ↓
     3. 调用 FNAME(fetch)() 创建新 SPTE
        - 删除旧的 SPTE (u=0, w=1)
        - 创建新 SPTE: spte.u=1, spte.w=0
        - 将新 SPTE 写入影子页表
     ↓
Guest: 用户读成功，继续执行
       现在的 SPTE: u=1, w=0
```

第三步：内核再次写访问

```
Guest 执行: mov [addr], ebx  (内核态)
       ↓
CPU: 触发 Page Fault
     error_code = PFERR_WRITE_MASK (不包含 USER_MASK)
     原因：spte.w=0，不允许写
       ↓
KVM: 再次执行第一步的流程
     - 权限转换：walker.pte_access = ACC_WRITE_MASK
     - 创建 SPTE: spte.u=0, spte.w=1, spte.nx=1
     ↓
Guest: 内核写成功
       现在的 SPTE: u=0, w=1, nx=1
```

第四步：用户写访问（预期失败）

```
Guest 执行: mov [addr], ecx  (用户态)
       ↓
情况 A: 当前 SPTE = u=1, w=0
       - CPU 直接触发 #PF (写保护错误)
       - KVM 检查 guest PTE 权限：gpte.u=1, gpte.w=0
       - 用户态写不可写页面，这是合法的 guest 错误
       - 注入 #PF 给 guest

情况 B: 当前 SPTE = u=0, w=1
       - CPU 触发 #PF (用户态访问内核页)
       - KVM 检查 guest PTE 权限：gpte.u=1, gpte.w=0
       - 用户态写不可写页面，这是合法的 guest 错误
       - 注入 #PF 给 guest
       ↓
Guest: 接收到 #PF，执行错误处理（这是正确的行为）
```

时序图

```
时间轴    SPTE 状态         触发访问           KVM 操作
-------------------------------------------------------------
t0       无 SPTE          内核写访问         创建 u=0,w=1,nx=1
t1       u=0,w=1,nx=1     (内核写成功)       -
t2       u=0,w=1,nx=1     用户读访问         替换为 u=1,w=0
t3       u=1,w=0          (用户读成功)       -
t4       u=1,w=0          内核写访问         替换为 u=0,w=1,nx=1
t5       u=0,w=1,nx=1     (内核写成功)       -
t6       u=0,w=1,nx=1     用户写访问         注入 #PF 给 guest
```

### kvm_mmu::permissions

```c
struct kvm_mmu {
	// ...
	/*
	 * Bitmap; bit set = permission fault
	 * Byte index: page fault error code [4:1]
	 * Bit index: pte permissions in ACC_* format
	 */
	u8 permissions[16];
```

配合 arch/x86/kvm/mmu/mmu.c:update_permission_bitmask

这个位图的设计非常巧妙：

1. **字节数组大小**: 16 字节 (索引 0-15)，对应页错误码的低4位 ([4:1]位)
	页错误码 (Page Fault Error Code) 的第4:1位用于索引这个数组：
	- 位1: `PFEC.P` (Present) - 页面不存在时置位
	- 位2: `PFEC.W/R` (Write/Read) - 写操作时置位
	- 位3: `PFEC.U/S` (User/Supervisor) - 用户模式访问时置位
	- 位4: `PFEC.I/D` (Instruction/Data or Reserved) - 取指时置位
2. **每个字节的位**: 每个字节的8个位表示不同的权限组合
	在每个字节中，位的位置对应不同的权限：
	- 位0: `ACC_EXEC_MASK` (执行权限)
	- 位1: `ACC_WRITE_MASK` (写权限，即PT_WRITABLE_MASK)
	- 位2: `ACC_USER_MASK` (用户权限，即PT_USER_MASK)
	- 其他位: 保留

如果某位被设置为1，则表示当对应的

在 `update_permission_bitmask` 函数中，整个权限检查流程如下：
   - `wf`: 写操作对不可写页面的故障
   - `uf`: 用户态访问内核页面的故障
   - `ff`: 对不可执行页面的取指操作故障
   - `smepf`: SMEP相关的故障
   - `smapf`: SMAP相关的故障

mmu->permissions[byte] = ff | uf | wf | smepf | smapf;

当需要进行权限检查时，系统会：
1. 根据当前的访问类型和处理器状态生成页错误码
2. 提取页错误码的[4:1]位作为索引
3. 使用该索引访问 `permissions` 数组得到对应的字节
4. 将该字节与页面的实际权限进行按位与运算
5. 如果结果非零，说明存在权限冲突，会产生页错误

最后看是如何使用的:
- walk_addr_generic
	- permission_fault (在这里来使用 mmu->permissions)
		- __kvm_mmu_refresh_passthrough_bits

- 为什么会来枚举所有的 page fault 类型组合 + pte 的权限
而不是直接计算一个 page fault + pte 权限，会有什么结果?

不是不同的 page fault 类型互相有干扰，只是单纯的更快。
假设我们不在 update_permission_bitmask 中计算，那么就是需要在 permission_fault 中计算
直接计算一个 page fault + pte 权限的结果，然后执行
```txt
permissions = ff | uf | wf | smepf | smapf;
```

### kvm_calc_cpu_role 的作用

`kvm_calc_cpu_role` 函数用于根据当前 VCPU 的寄存器状态计算出一个 CPU 角色 (CPU Role)，
这个角色描述了当前 CPU 的配置状态，包括分页模式、保护特性等。这个角色信息会被用来决定如何处理影子页表项 (SPT)。

1. 初始化基本属性
```c
role.base.access = ACC_ALL;           // 设置默认访问权限
role.base.smm = is_smm(vcpu);        // 是否处于系统管理模式
role.base.guest_mode = is_guest_mode(vcpu); // 是否处于客户机模式
role.ext.valid = 1;                   // 扩展角色有效
```

2. 处理非分页模式
```c
if (!____is_cr0_pg(regs)) {
    role.base.direct = 1;             // 直接映射模式
    return role;
}
```

3. 设置分页相关属性
- `efer_nx`: 是否启用 NX (No-eXecute) 位
- `cr0_wp`: 是否启用写保护
- `smep_andnot_wp`: SMEP 启用但 CR0.WP 禁用的情况
- `smap_andnot_wp`: SMAP 启用但 CR0.WP 禁用的情况
- `has_4_byte_gpte`: 是否使用4字节页表项

4. 设置页表层级
根据 EFER.LMA、CR4.LA57、CR4.PAE 等位确定页表层级

5. 设置扩展属性
- `cr4_smep`: SMEP (Supervisor Mode Execution Prevention) 状态
- `cr4_smap`: SMAP (Supervisor Mode Access Prevention) 状态
- `cr4_pse`: PSE (Page Size Extension) 状态
- `cr4_pke`: PKE (Protection Keys Enable) 状态
- `efer_lma`: LMA (Long Mode Active) 状态


### role 的作用

一旦切换 role ，那么所有的 spte 全部都清理掉。
所以，也就是前面文档中提到的那样，一旦这个功能记录到 role 中，那么

是的，之前调查的所有的:
```c
static void init_kvm_tdp_mmu(struct kvm_vcpu *vcpu,
			     union kvm_cpu_role cpu_role)
{
	struct kvm_mmu *context = &vcpu->arch.root_mmu;
	union kvm_mmu_page_role root_role = kvm_calc_tdp_mmu_root_page_role(vcpu, cpu_role);

	if (cpu_role.as_u64 == context->cpu_role.as_u64 &&
	    root_role.word == context->root_role.word)
		return;

```

```c
static void init_kvm_nested_mmu(struct kvm_vcpu *vcpu,
				union kvm_cpu_role new_mode)
{
	struct kvm_mmu *g_context = &vcpu->arch.nested_mmu;

	if (new_mode.as_u64 == g_context->cpu_role.as_u64)
		return;
```

### kvm_mmu_page_role 和 kvm_cpu_role

```c
union kvm_cpu_role {
	u64 as_u64;
	struct {
		union kvm_mmu_page_role base;
		union kvm_mmu_extended_role ext;
	};
};
```

#### kvm_cpu_role 与 CPU 绑定的证据
```c
struct kvm_mmu {
    // ...
    union kvm_cpu_role cpu_role;      // 直接包含 cpu_role 字段
    union kvm_mmu_page_role root_role;
    // ...
};
```

__kvm_mmu_refresh_passthrough_bits


#### kvm_mmu_page_role 与单个页表页

1. 结构定义证据
在 struct kvm_mmu_page 中直接包含 role 字段：

```c
struct kvm_mmu_page {
    // ...
    union kvm_mmu_page_role role;  // 直接包含 role 字段
    gfn_t gfn;
    // ...
};
```
例如在 kvm_mmu_get_child_sp 中动态的计算
```c
static struct kvm_mmu_page *kvm_mmu_get_child_sp(struct kvm_vcpu *vcpu,
						 u64 *sptep, gfn_t gfn,
						 bool direct, unsigned int access)
{
	union kvm_mmu_page_role role;

	if (is_shadow_present_pte(*sptep) && !is_large_pte(*sptep))
		return ERR_PTR(-EEXIST);

	role = kvm_mmu_child_role(sptep, direct, access);
	return kvm_mmu_get_shadow_page(vcpu, gfn, role);
}
```

### CR4.SMEP 和 smep_andnot_wp 的作用

**SMEP (Supervisor Mode Execution Prevention)** 是一种安全特性，防止内核模式执行用户空间的代码。

在 `kvm_calc_cpu_role` 中有两个相关的字段：

1. **`role.ext.cr4_smep = ____is_cr4_smep(regs)`**:
   - 记录当前 CR4.SMEP 位的状态
   - 用于软件页表遍历中的权限检查

2. **`role.base.smep_andnot_wp = ____is_cr4_smep(regs) && !____is_cr0_wp(regs)`**:
   - 这是一个复合条件，表示 SMEP 启用且 WP 禁用
   - 这个特殊组合很重要，因为当 CR0.WP=0 时，内核可以写用户页，这会影响权限检查逻辑

**为什么要特别关注 `smep_andnot_wp` 组合？**
- 当 CR0.WP=0 且 CR4.SMEP=1 时，权限检查规则与标准情况不同
- 这种组合下，即使页面是用户页，内核也可能被允许执行（由于 WP=0）
- 因此需要将这种组合状态编码到角色中，确保正确的权限检查


## Large pages

The mmu supports all combinations of large and small guest and host pages. Supported page sizes include 4k, 2M, 4M, and 1G. 4M pages are treated as two separate 2M pages, on both guest and host, since the mmu always uses PAE paging.

To instantiate a large spte, four constraints must be satisfied:

the spte must point to a large host page

the guest pte must be a large pte of at least equivalent size (if tdp is enabled, there is no guest pte and this condition is satisfied)

if the spte will be writeable, the large page frame may not overlap any write-protected pages

the guest page must be wholly contained by a single memory slot

To check the last two conditions, the mmu maintains a ->disallow_lpage set of arrays for each memory slot and large page size. Every write protected page causes its disallow_lpage to be incremented, thus preventing instantiation of a large spte. The frames at the end of an unaligned memory slot have artificially inflated ->disallow_lpages so they can never be instantiated.

## Reaction to events

就是 shadow page 基本工作原理:

如果写入对应的位置，那么最后的路径是:
```c
/*
 * Attempt to unsync any shadow pages that can be reached by the specified gfn,
 * KVM is creating a writable mapping for said gfn.  Returns 0 if all pages
 * were marked unsync (or if there is no shadow page), -EPERM if the SPTE must
 * be write-protected.
 */
int mmu_try_to_unsync_pages(struct kvm *kvm, const struct kvm_memory_slot *slot,
			    gfn_t gfn, bool synchronizing, bool prefetch)
```

不会立刻同步， 而是 mmu_sync_children 来同步

```c
// 在某些操作时（如 TLB flush, CR3 修改）
mmu_sync_children(vcpu, parent, can_yield)
  ↓
  mmu_unsync_walk(parent, &pages)
    // 找到所有 unsync 的 child shadow pages
    // 包括 sp (sp->unsync = 1)
  ↓
  kvm_vcpu_write_protect_gfn(vcpu, sp->gfn)
    // 重新 write-protect GPA 0x3000
  ↓
  kvm_mmu_commit_sync_page(vcpu, sp)
    ↓
    // 重新读取 guest 页表
    guest_pte = read_guest_pte(0x3000 + 0)
    // guest_pte = GVA 0x0000 → GPA 0x7000
    ↓
    // 更新 shadow SPTE
    sp->spt[0] = GVA 0x0000 → HPA_of_0x7000  ← 新值
    ↓
    // 从 RMAP 移除旧映射
    rmap_remove(rmap[0x5], &sp->spt[0])
    ↓
    // 添加到 RMAP 新映射
    rmap_add(rmap[0x7], &sp->spt[0])
    ↓
    sp->unsync = 0  // 清除 unsync 标志
```

## 三种反向查询机制
<!-- 30983db4-fd2a-4b90-9fa9-a856b37cf50d -->

┌────────────────┬─────────────────────────┬───────────────────────────┬──────────────────────────────────────────────────────────┐
│      机制      │        查询目标         │         数据结构          │                           用途                           │
├────────────────┼─────────────────────────┼───────────────────────────┼──────────────────────────────────────────────────────────┤
│ RMAP           │ GFN → leaf SPTEs        │ slot->arch.rmap[gfn]      │ 找到"映射 GPA 0x3000"的 SPTEs（write-protect）           │
├────────────────┼─────────────────────────┼───────────────────────────┼──────────────────────────────────────────────────────────┤
│ GFN Hash Table │ GFN → shadow pages      │ kvm->arch.mmu_page_hash[] │ 找到"基于 GPA 0x3000 构建"的 kvm_mmu_page（标记 unsync） │
├────────────────┼─────────────────────────┼───────────────────────────┼──────────────────────────────────────────────────────────┤
│ parent_ptes    │ Child SP → Parent SPTEs │ sp->parent_ptes           │ 反向指针，zap child 时清除 parent SPTE                   │
└────────────────┴─────────────────────────┴───────────────────────────┴──────────────────────────────────────────────────────────┘

从子 shadow page 找到所有引用它的父 SPTEs

```c
static void mmu_page_add_parent_pte(struct kvm *kvm,
                                    struct kvm_memory_cache *cache,
                                    struct kvm_mmu_page *sp, u64 *parent_pte)
{
    if (!parent_pte)
        return;

    // 将 parent_pte 添加到 sp->parent_ptes 列表
    pte_list_add(kvm, cache, parent_pte, &sp->parent_ptes);
}
```
似乎是这个原理吧:

```txt
Root (L4)
 └── L3
     └── L2   <-- sp
         └── L1
```

现在：
L1 中某个 guest PTE 被修改
对应的 shadow L1 page 被标记 unsync

如果你只标记 L2：
L3、Root 仍然被认为是 完全 sync 的
KVM 在某些路径上会：
跳过 resync
或认为该分支是稳定的


## TODO
### tdp_mmu 是如何使用 struct kvm_mmu_page 的

```c
static void tdp_mmu_init_sp(struct kvm_mmu_page *sp, tdp_ptep_t sptep,
			    gfn_t gfn, union kvm_mmu_page_role role)
{
	INIT_LIST_HEAD(&sp->possible_nx_huge_page_link);

	set_page_private(virt_to_page(sp->spt), (unsigned long)sp);

	sp->role = role;
	sp->gfn = gfn;
	sp->ptep = sptep;
	sp->tdp_mmu_page = true;

	trace_kvm_mmu_get_page(sp, true);
}
```


## TODO shadowed_translation
<!-- ee51b7ed-e042-4249-ada6-9c6c1e824b9d -->

我理解是不难的，和这个文档中其他的内容一起总结一下:
```c
	/*
	 * Stores the result of the guest translation being shadowed by each
	 * SPTE.  KVM shadows two types of guest translations: nGPA -> GPA
	 * (shadow EPT/NPT) and GVA -> GPA (traditional shadow paging). In both
	 * cases the result of the translation is a GPA and a set of access
	 * constraints.
	 *
	 * The GFN is stored in the upper bits (PAGE_SHIFT) and the shadowed
	 * access permissions are stored in the lower bits. Note, for
	 * convenience and uniformity across guests, the access permissions are
	 * stored in KVM format (e.g.  ACC_EXEC_MASK) not the raw guest format.
	 */
	u64 *shadowed_translation;
```

以前将 gfn 命名为 shadowed_translation 了

```diff
History:        #0
Commit:         6a97575d5cffb71aa9a95d33f0ca03c8a4bb3b2b
Author:         David Matlack <dmatlack@google.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Thu 23 Jun 2022 03:27:04 AM CST
Committer Date: Fri 24 Jun 2022 04:51:58 PM CST

KVM: x86/mmu: Cache the access bits of shadowed translations

Splitting huge pages requires allocating/finding shadow pages to replace
the huge page. Shadow pages are keyed, in part, off the guest access
permissions they are shadowing. For fully direct MMUs, there is no
shadowing so the access bits in the shadow page role are always ACC_ALL.
But during shadow paging, the guest can enforce whatever access
permissions it wants.

In particular, eager page splitting needs to know the permissions to use
for the subpages, but KVM cannot retrieve them from the guest page
tables because eager page splitting does not have a vCPU.  Fortunately,
the guest access permissions are easy to cache whenever page faults or
FNAME(sync_page) update the shadow page tables; this is an extension of
the existing cache of the shadowed GFNs in the gfns array of the shadow
page.  The access bits only take up 3 bits, which leaves 61 bits left
over for gfns, which is more than enough.

Now that the gfns array caches more information than just GFNs, rename
it to shadowed_translation.

While here, preemptively fix up the WARN_ON() that detects gfn
mismatches in direct SPs. The WARN_ON() was paired with a
pr_err_ratelimited(), which means that users could sometimes see the
WARN without the accompanying error message. Fix this by outputting the
error message as part of the WARN splat, and opportunistically make
them WARN_ONCE() because if these ever fire, they are all but guaranteed
to fire a lot and will bring down the kernel.

Signed-off-by: David Matlack <dmatlack@google.com>
Message-Id: <20220516232138.1783324-18-dmatlack@google.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

初始化的位置:
- kvm_mmu_page_set_translation : 在其中存储 gfn 和 access bit

```txt
@[
    kvm_mmu_page_set_translation+1
    __rmap_add+125
    mmu_set_spte+366
    direct_pte_prefetch_many+301
    ept_page_fault+2296
    kvm_mmu_page_fault+935
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 97671
@[
    kvm_mmu_page_set_translation+1
    __rmap_add+125
    mmu_set_spte+366
    ept_page_fault+1602
    kvm_mmu_page_fault+935
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 186478
```
- 使用的位置 : 有一些和大页有关。

## sync page
<!-- fdba1839-3387-4885-9eda-e882c1c6d943 -->

也是需要整理一下


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

## shadow page table 可以和 guest page table 是一一对应的
<!-- 29bdd8de-9c6e-493f-ad58-e1c40224637d -->

为什么?
Guest 中，touch 了那些虚拟地址，无论 touch 的顺序如何，
最后形成的 page table 的 tree 都是一样的。
这些用于记录这些 page table 的 page 的位置自然是不受控制。


这意味着什么?
由于可以跟踪 page fault ，所以建立每一个 guest page tabel 的时候，
都可以构建对应的 shadow page table 可以与其对应

从 GVA 到 GPA 构建的的 guest page table 的形状和 shadow page table 的形状
是一模一样的。那么每一个 guest page table 的 page 都是正好有一个 shadow page table 的 page
与之对应、当检测到 guest page table page 被修改之后，那么对应的 shadow page table page
的页面关联的映射可以全部清理掉。


以上只是基本想法，这里有一个前提的，就是 guet page table 的级数和 shadow page table 必须完全相同，
也就是如果 guest 是 4 级，那么 shadow page table 也必须是 4 级，不可以是 5 级，
这个需要验证测试一下。

### kvm_mmu_get_child_sp 就是验证这个想法经典函数

(对于 gfn 对应的 guest page table ，我们希望可以构建其对应的 shadow page table ，所以
我猜测 kvm_mmu_get_shadow_page 这个函数中，是可以在同时 walk shadow page table 和 guest page table 的才对的
)

经典函数: 当 shadow page table 在构建下一级的 shadow page table
```c
static struct kvm_mmu_page *kvm_mmu_get_child_sp(struct kvm_vcpu *vcpu,
						 u64 *sptep, gfn_t gfn,
						 bool direct, unsigned int access)
{
	union kvm_mmu_page_role role;

	if (is_shadow_present_pte(*sptep) && !is_large_pte(*sptep))
		return ERR_PTR(-EEXIST);

	role = kvm_mmu_child_role(sptep, direct, access);
	return kvm_mmu_get_shadow_page(vcpu, gfn, role);
}
```
这两个参数的含义:
- u64 *sptep : shadow page table entry pointer
- gfn_t gfn : sptep 指向的 entry 所映射的页面的 gfn ，这个 gfn 需要是一个shadow page table

核心逻辑:
通过 kvm_mmu_find_shadow_page 来找，如果没有，那么就创建一个新的 kvm_mmu_page 关联到 shadow page table 上:
```txt
	sp_list = &kvm->arch.mmu_page_hash[kvm_page_table_hashfn(gfn)];
```

这里我们可以看到一个经典例子，通过 gfn 找到关联的 shadow page table 的例子了

## sync and unsync page 的经典使用的地方

mmu_try_to_unsync_pages

make_spte 在 tdp 中也会被调用

也就是，也可能调用到 mmu_try_to_unsync_pages 中去

tdp 的确会调用这个，但是我感觉这个是可以优化的:
```txt
@[
        mmu_try_to_unsync_pages+5
        make_spte+860
        tdp_mmu_map_handle_target_level+245
        kvm_tdp_mmu_map+1129
        kvm_tdp_page_fault+239
        kvm_mmu_do_page_fault+455
        kvm_mmu_page_fault+120
        npf_interception+147
        vcpu_enter_guest.constprop.0+1623
        vcpu_run+55
        kvm_arch_vcpu_ioctl_run+739
        kvm_vcpu_ioctl+724
        __x64_sys_ioctl+161
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 168635
```

## 为什么 direct=1 也需要 kvm_mmu_page ?
<!-- 924d40b0-7536-4822-9a5e-92bf10fd389e -->

```txt
┌─────────────────────────────────────────────┬───────────────────────────────┐
│                    需求                     │    为什么需要 kvm_mmu_page    │
├─────────────────────────────────────────────┼───────────────────────────────┤
│ 内存 unmap 时，找到所有映射该 GPA 的 SPTE   │ reverse mapping (parent_ptes) │
├─────────────────────────────────────────────┼───────────────────────────────┤
│ Hash 表快速查找：GPA 范围内有无现成页表节点 │ gfn + role 作为 key           │
├─────────────────────────────────────────────┼───────────────────────────────┤
│ 销毁页表树时，知道子节点覆盖的 GPA 范围     │ sp->gfn + sp->role.level      │
├─────────────────────────────────────────────┼───────────────────────────────┤
│ NX huge page 缓解措施追踪                   │ nx_huge_page_disallowed       │
└─────────────────────────────────────────────┴───────────────────────────────┘
```
kvm_mmu_page::gfn 如何理解?

看看 arch/x86/kvm/mmu/tdp_mmu.c 中的 handle_removed_pt 就可以理解了

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
