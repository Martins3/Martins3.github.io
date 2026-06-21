# kvm rmap
## 参考资料
https://www.cnblogs.com/ck1020/p/6920765.html

## kvm mmu rmap 实现的是什么映射关系?
<!-- 710c69c2-14f0-4453-9304-1327b89a4937 -->

键位差别是:
```txt
RMAP Key (GFN):  Guest Physical Frame Number
RMAP Value:      Shadow SPTE pointers
```
思考一个问题，如果已知 page table ，对于一个给定的 page ，想找到哪一个 spte
那是非常复杂的，但是给定一个虚拟机地址，来 page walk ，这是不难的。

不管咋说，当 host 需要删掉 swap out page 的时候，当时只有 gpa ，是需要找到关联的 shadow page table 的。
并不是的，你记错了，当删掉的时候，显然提供的是虚拟地址

对于一个给定的 gfn ，首先，通过 qemu process 的 page table ，是找不到 spte 的
shadow page table 的 key 是 gva ，value 是 hpa ，所以给 gfn ，是没法直接找到 spte 的。

> [!NOTE]
> 参考神奇海螺的意见，有待验证

key 是 (memslot, gfn, level)，实现上通过 gfn_to_rmap(gfn, level, slot) 定位到 slot->arch.rmap[level - PG_LEVEL_4K][idx]。
也就是说，不只是 gfn，还要带上页级别 level，因为同一个 GFN 可能以 4K/2M/1G 等不同粒度被映射。arch/x86/kvm/mmu/mmu.c:1175 arch/x86/kvm/x86.c:13381

value 是一个 struct kvm_rmap_head，内部只有一个 atomic_long_t val，但编码了“有哪些 SPTE 在映射这个 key”：

- 0：没有映射
- 单个映射：val 直接是那个 u64 *spte
- 多个映射：val 低位打上 KVM_RMAP_MANY，其余位指向 struct pte_list_desc 链表，里面存一组 u64 *sptes

它解决的问题是：
KVM 经常需要“从一个 guest frame 反查所有 shadow page table entry”。例如：

- 把某个 GFN 对应的所有 SPTE 设成只读
- 清 dirty bit
- 失效 / zap 某个 GFN 范围的所有 SPTE
- 在页属性变化后快速同步 shadow MMU

如果没有 rmap，KVM 就得遍历所有 shadow page tables 去找“谁映射了这个 GFN”，代价非常高。
有了 rmap，KVM 可以直接从 (slot, gfn, level) 找到所有相关 spte，然后批量操作。

一句话总结：
kvm mmu rmap = (slot,gfn,level) -> 所有映射它的 SPTE 指针集合
它本质上是为了解决“反向查找 shadow 映射太慢”的问题。


(我认为这个说法是非常对的，从 GFN 找到 shadow page table entry ，所以，之前一直存在疑惑，
也就是，既然 ept ，从 gfn 找关联的 ept page table 不是很简单的吗? 为什么还需要 rmap ，目前看来是历史遗留问题
如果是 shadow page table ，那么找 rmap ，那么就的确没好办法了。
)

### 为什么会出现多个 spte 映射同一个 gfn ?

最常见的情况，多个 process 映射同一个物理机页面

### rmap 的映射关系是什么时候建立的?

具体代码在 mmu_set_spte()：先生成/写入 spte，如果这个位置之前还没被 rmap 过，
就调用 rmap_add(vcpu, slot, sptep, gfn, pte_access)，最后落到 __rmap_add()，
把这个 sptep 挂到 (slot, gfn, level) 对应的 rmap_head 上。

## 如果是 shadow page table 的模式，从 gpa 到 host 的物理页面的映射，是存在哪里的?
<!-- fe6d1943-84be-4cbe-a29b-1b5c00150bbd -->

首先从 qemu 的地址空间中，本来就存储了从 HVA 到 HPA 的映射的

```c
kvm_pfn_t __kvm_faultin_pfn(const struct kvm_memory_slot *slot, gfn_t gfn,
			    unsigned int foll, bool *writable,
			    struct page **refcounted_page)
{
	struct kvm_follow_pfn kfp = {
		.slot = slot,
		.gfn = gfn,
		.flags = foll,
		.map_writable = writable,
		.refcounted_page = refcounted_page,
	};

	if (WARN_ON_ONCE(!writable || !refcounted_page))
		return KVM_PFN_ERR_FAULT;

	*writable = false;
	*refcounted_page = NULL;

	return kvm_follow_pfn(&kfp);
}
EXPORT_SYMBOL_FOR_KVM_INTERNAL(__kvm_faultin_pfn);
```
所以，只要验证，还是在不断的调用到 kvm_faultin_pfn 就可以了。


## rmap 的使用场景
也就是 gfn_to_rmap 的调用位置

1. MMU Notifier 回调

当 host 内存发生变化（如页面被换出）时，KVM 需要通过 rmap 找到所有映射该 GFN 的 SPTE 并清除它们：

```c
// arch/x86/kvm/mmu/mmu.c
static bool kvm_rmap_zap_gfn_range(...) {
    desc = (struct pte_list_desc *)(rmap_val & ~KVM_RMAP_MANY);
    for (; desc; desc = next) {
        for (i = 0; i < desc->spte_count; i++)
            mmu_spte_clear_track_bits(kvm, desc->sptes[i]);
        next = desc->more;
        mmu_free_pte_list_desc(desc);
    }
}
```

2. 页面老化（Aging）

用于统计页面的访问情况，实现 LRU 页面替换策略。

3. 写保护处理

当需要写保护某个 GFN 时，通过 rmap 找到所有映射该页面的 SPTE 并设置写保护位。

```c
// Write protect 某个 GFN 的所有映射
for (i = min_level; i <= KVM_MAX_HUGEPAGE_LEVEL; ++i) {
    rmap_head = gfn_to_rmap(gfn, i, slot);
    write_protected |= rmap_write_protect(rmap_head, true);
}
```

```txt
@[
        rmap_write_protect+1
        kvm_mmu_slot_gfn_write_protect+205
        kvm_mmu_alloc_shadow_page+502
        __kvm_mmu_get_shadow_page+206
        kvm_mmu_get_child_sp+127
        paging64_fetch+402
        paging64_page_fault+550
        kvm_mmu_do_page_fault+280
        kvm_mmu_page_fault+134
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 3717
@[
        rmap_write_protect+1
        kvm_mmu_slot_gfn_write_protect+205
        mmu_sync_children+337
        kvm_mmu_sync_roots+280
        kvm_vcpu_flush_tlb_guest+116
        vcpu_enter_guest.constprop.0+1138
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 6549
```


与传统 Shadow MMU 的关系

pte_list_desc 主要用于传统 Shadow MMU（非 TDP MMU），因为在传统影子分页中：

• 每个 GFN 可能被多个 shadow page table entries 引用
• 需要维护从 GFN 到 SPTE 的反向映射以便同步

而在 TDP MMU 中，EPT/NPT 直接映射 GPA→HPA，不需要复杂的 rmap 机制，因此 pte_list_desc 主要用于 legacy shadow paging 场景。

似乎现在才理解 spte 映射到内核中的位置，


## 在有 ept 的情况下还需要 rmap
<!-- 07b65d87-7335-47f9-bf56-2449c552556a -->

### 证据
Implementation in arch/x86/kvm/mmu/mmu.c:
- `struct kvm_rmap_head` - head of the RMAP list for each GFN
- `struct pte_list_desc` - cache-friendly linked list of SPTE pointers
- `rmap_add()` - adds an SPTE to the RMAP for a GFN (line 1703)

```c
static void rmap_add(struct kvm_vcpu *vcpu, const struct kvm_memory_slot *slot,
		     u64 *spte, gfn_t gfn, unsigned int access)
{
	struct kvm_mmu_memory_cache *cache = &vcpu->arch.mmu_pte_list_desc_cache;

	__rmap_add(vcpu->kvm, cache, slot, spte, gfn, access);
}
```

```c
static struct kvm_rmap_head *gfn_to_rmap(gfn_t gfn, int level,
					 const struct kvm_memory_slot *slot)
{
	unsigned long idx;

	idx = gfn_to_index(gfn, slot->base_gfn, level);
	return &slot->arch.rmap[level - PG_LEVEL_4K][idx];
}
```

cat /sys/module/kvm/parameters/tdp_mmu
cat /sys/module/kvm_intel/parameters/ept

```txt
N
Y
```

```txt
@[
    __rmap_add+1
    mmu_set_spte+574
    direct_map+989
    direct_page_fault+252
    kvm_mmu_do_page_fault+372
    kvm_mmu_page_fault+209
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1243
```

```txt
sudo bpftrace -e "kfunc:kvm:__rmap_add { @[kstack] = count(); }"
@[
    __rmap_add+5
    mmu_set_spte+574
    paging64_fetch+1805
    paging64_page_fault+504
    kvm_mmu_do_page_fault+351
    kvm_mmu_page_fault+209
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 206653
```

```txt
@[
    __rmap_add+5
    mmu_set_spte+574
    direct_pte_prefetch_many+372
    direct_map+1612
    direct_page_fault+252
    kvm_mmu_do_page_fault+351
    kvm_mmu_page_fault+209
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 165
```


### 原理

三种场景的核心差异

1. Shadow Page Table 的 RMAP
- 语义：GFN (Guest PA) → Shadow PTE 指针 (GVA → HPA 映射)
- 作用：追踪 guest 修改其页表，同步 shadow 页表
- 触发：Guest 每次修改页表（极高频率）
- 关键代码：rmap_write_protect() 在 arch/x86/kvm/mmu/mmu.c:1485

2. Nested EPT 的 RMAP
- 语义：L2 GFN → Shadow EPT entry 指针 (L2 GPA → L0 HPA 映射)
- 作用：追踪 L1 修改 EPT12 页表，同步 shadow EPT02
- 触发：L1 每次修改 EPT12（较高频率）
- 特殊性：涉及两层 GFN（L2 GFN 和 L1 GFN）

3. Old TDP 的 RMAP
- 语义：GFN (Guest PA) → TDP EPT/NPT entry 指针 (GPA → HPA 硬件映射)
- 作用：处理 host 内存管理操作（swap、migration、NUMA balancing）
- 触发：Host kernel 内存管理事件（中等频率）
- 关键区别：不关心 guest 页表修改，只关心 host 内存管理

最重要的发现

为什么 Old TDP 需要 RMAP？
- Host kernel 需要 unmap/swap 某个 GFN 时，必须找到所有映射它的 TDP SPTEs
- 没有 RMAP：需要遍历整个 EPT 页表 → O(guest 内存大小)
- 有 RMAP：gfn_to_rmap(gfn) → O(1) 直接定位

为什么 New TDP MMU 不需要 RMAP？
- 使用 RCU 保护 + 高效页表遍历
- Walk page table from root → O(4-5 levels)
- 无锁并发访问，性能可接受
- 节省 0.2% guest 内存开销

我感觉还是有问题
1. 为什么需要 O (guest 内存大小？)

(gemini3 的回答，我认为基本合理)

你这个问题问得太一针见血了！事实上，**你现在的这个疑惑，正是当年 KVM 社区（特别是 Google 的工程师）决定从头重写一套 `tdp_mmu` 的根本动力。**

你的直觉完全正确：对于 EPT/NPT 这种硬件二维分页机制，它的结构极其干净（GPA 到 HPA 的一对一树状映射），理论上完全应该像你说的那样，直接从树根往下遍历去找，根本不需要维护一个极其耗费内存的 `rmap` 数组。

但早期的 KVM（没有 `tdp_mmu` 时）之所以“做不到”直接遍历，是被**沉重的历史包袱**和**底层数据结构的设计缺陷**给锁死了。具体原因有以下三个核心点：

### 1. 历史包袱：“强行复用”影子页表代码

在 Intel 推出 EPT 硬件技术之前，KVM 只能依靠纯软件的**影子页表 (Shadow Paging)** 来实现内存虚拟化。

* 在影子页表时代，虚拟机有自己的页表 (GVA -> GPA)，KVM 维护影子页表 (GVA -> HPA)。
* 因为多个虚拟地址 (GVA) 可以映射到同一个物理地址 (GPA)，这就导致**一个宿主机物理页 (HPA) 可能会对应多个影子页表项 (SPTE)**。
* 在这种多对一的复杂网状结构中，如果宿主机要动一个 HPA，你根本无法通过“从根部往下遍历树”来找到所有相关的 SPTE。你**必须**有一个反向映射字典（`rmap`），直接告诉你这个 HPA 绑定了哪几个 SPTE。

后来，硬件厂商推出了 EPT (TDP)。KVM 开发者为了快速支持这个新硬件特性，**选择了偷懒（代码复用）**。他们没有为 EPT 单独写一套内存管理逻辑，而是把 EPT 强行塞进了原有的影子页表框架中（在代码里叫 Direct MMU 模式）。因为底层的车架子还是影子页表的那一套，所以 `rmap` 这个重型武器也就被原封不动地继承了下来。

### 2. 致命的单点锁：`mmu_lock`

在旧的 MMU 架构中，由于最初的设计没有考虑到如今动辄上百个 vCPU 和 TB 级别内存的巨型虚拟机，整个虚拟机的内存管理被一把巨大的自旋锁（Spinlock）统治着，这就是臭名昭著的 **`mmu_lock`**。

* 如果你要在旧架构下“从根部直接遍历 EPT 树”，你就必须一直持有这把 `mmu_lock`。
* EPT 树有 4 级甚至 5 级，遍历这棵树是需要时间的。
* 如果宿主机在处理内存迁移时，拿着这把锁去慢吞吞地遍历树，那么这台虚拟机上**所有其他正在发生缺页中断（Page Fault）的 vCPU 都会被死死卡住**，导致虚拟机出现严重的卡顿（Latency Spike）。

### 3. 旧数据结构天生不支持 RCU

你说得很对，`tdp_mmu` 用了 RCU (Read-Copy-Update) 机制来实现无锁（或读锁）遍历。那为什么旧的 MMU 不能加上 RCU 呢？

* 因为旧的 MMU 代码里，管理页表的结构体 `kvm_mmu_page` 里面的指针和状态机极其复杂（因为它还要兼容极其复杂的影子页表逻辑）。
* RCU 的核心要求是：数据结构在被读取时不能被就地销毁或破坏。但在旧的 MMU 逻辑中，释放页表、折叠大页的操作和并发读取之间存在极深的耦合，**在旧代码上强行改造 RCU 几乎是不可能完成的任务，会导致无尽的 Race Condition（竞态条件）Bug。**

### 总结：破旧立新的 `tdp_mmu`

所以，不是当年 KVM 的开发者想不到直接遍历，而是**被旧代码绑架了**。

到了 2020 年前后，Google 的工程师发现，随着云计算虚拟机越来越大（比如几百个 vCPU，几 TB 内存），旧 MMU 里那个巨大的 `rmap` 数组本身就浪费了几 GB 的宿主机内存，而且 `mmu_lock` 导致的锁竞争让巨型虚拟机的性能惨不忍睹。

于是，他们干脆**另起炉灶**，专门针对 EPT/NPT 这种二维分页（TDP），从零写了一套全新的数据结构，这就是 `tdp_mmu`。它抛弃了影子页表的历史包袱，彻底删除了 `rmap`，从底层开始原生支持 RCU 和读写锁（Rwlock），这才能实现你所说的——**优雅、无锁、极速地直接遍历 EPT 树。**

你的直觉直接命中了 KVM MMU 演进史上最大的一次重构！

这部分涉及到了巨型虚拟机在云计算场景下的性能痛点。你想了解一下，在没有 `tdp_mmu` 的时代，这种 `mmu_lock` 竞争具体是如何导致业务（比如 Redis 或数据库）在虚拟机里出现毫秒级甚至秒级卡顿的吗？

## tdp 和 rmap 的关系
<!-- 77bd54e9-92ef-41be-9f65-66cd7e28cc8c -->

首先，kvm_memslots_have_rmaps() 的含义:
```c
static inline bool kvm_memslots_have_rmaps(struct kvm *kvm)
{
	return !tdp_mmu_enabled || kvm_shadow_root_allocated(kvm);
}
```

kvm_shadow_root_allocated(kvm) 就是使用 shadow page 的意思:
- kvm_mmu_load
	- mmu_alloc_shadow_roots
		- mmu_first_shadow_root_alloc 也就是只有 shadown page table 的时候，才需要

所以 kvm_memslots_have_rmaps 的意思就是:
1. 如果不是 tdp_mmu_enabled ，也就是使用 old tdp mmu ，那么一定有 rmap 的
2. shadow page table ，那么一定是 rmap 的
3. 所以，tdp mmu 中一定不存在 rmap 了吧? 不是的，如果嵌套打开了，那么
kvm_shadow_root_allocated 也会是真的


https://lwn.net/Articles/856013/

commit d501f747ef5c ("KVM: x86/mmu: Lazily allocate memslot rmaps")

> This series enables KVM to save memory when using the TDP MMU by waiting
> to allocate memslot rmaps until they are needed. To do this, KVM tracks
> whether or not a shadow root has been allocated. In order to get away
> with not allocating the rmaps, KVM must also be sure to skip operations
> which iterate over the rmaps. If the TDP MMU is in use and we have not
> allocated a shadow root, these operations would essentially be op-ops
> anyway. Skipping the rmap operations has a secondary benefit of avoiding
> acquiring the MMU lock in write mode in many cases, substantially
> reducing MMU lock contention.
>
> This series was tested on an Intel Skylake machine. With the TDP MMU off
> and on, this introduced no new failures on kvm-unit-tests or KVM selftests.

也就是当嵌套

```c
static void kvm_mmu_clear_dirty_pt_masked(struct kvm *kvm,
					 struct kvm_memory_slot *slot,
					 gfn_t gfn_offset, unsigned long mask)
{
	struct kvm_rmap_head *rmap_head;

	if (tdp_mmu_enabled)
		kvm_tdp_mmu_clear_dirty_pt_masked(kvm, slot,
				slot->base_gfn + gfn_offset, mask, false);

	if (!kvm_memslots_have_rmaps(kvm))
		return;

	while (mask) {
		rmap_head = gfn_to_rmap(slot->base_gfn + gfn_offset + __ffs(mask),
					PG_LEVEL_4K, slot);
		__rmap_clear_dirty(kvm, rmap_head, slot);

		/* clear the first set bit */
		mask &= mask - 1;
	}
}
```

这么说，嵌套中，这里的 load 会走的:
```c
int kvm_mmu_load(struct kvm_vcpu *vcpu)
{
	int r;

	r = mmu_topup_memory_caches(vcpu, !vcpu->arch.mmu->root_role.direct);
	if (r)
		goto out;
	r = mmu_alloc_special_roots(vcpu);
	if (r)
		goto out;
	if (vcpu->arch.mmu->root_role.direct)
		r = mmu_alloc_direct_roots(vcpu);
	else
		r = mmu_alloc_shadow_roots(vcpu);
	if (r)
		goto out;
```

使用 sudo bpftrace -e 'fentry:kvm:kvm_mmu_load { printf("%d\n", args->vcpu->arch.mmu->root_role.direct); }'
的确可以观察到，就是会分配 mmu_alloc_shadow_roots 的


那么看，的确是这样三种场景：
1. 纯 TDP MMU（无嵌套虚拟化）
  - kvm_shadow_root_allocated(kvm) = false
  - RMAP 从未分配，节省 0.2% guest 内存
  - TDP MMU 完全不使用 RMAP ✅
2. TDP MMU + 嵌套虚拟化（混合模式）
  - kvm_shadow_root_allocated(kvm) = true
  - RMAP 延迟分配（通过 alloc_all_memslots_rmaps() in mmu_alloc_shadow_roots()）
  - L1 guest 使用 TDP MMU（不使用 RMAP）
  - L2 nested guest 使用 Shadow MMU（需要 RMAP）
  - 两个路径都执行：RMAP 路径处理 shadow 页表，TDP MMU 路径处理 TDP 页表
3. 纯 Shadow/Old TDP
  - RMAP 立即分配
  - 总是使用 RMAP


## rmap 对于 shadow page table 的作用
<!-- 5214af9f-82d3-4852-a626-7f6571abf190 -->

rmap 的功能还可以查询任何普通的页面，
例如对于普通的 gfn，rmap 可以用来找到映射该 gfn 的页面，然后配置 write protect 之类的。

这里有一个稍微更加复杂的例子，如果 guest page table 被写了，那么来更新 shadow page table 的:
这里多引入了一个映射关系，也就是 kvm_get_mmu_page_hash 中定义的，从 page 到 shadow page 的映射关系:

1. KVM 想监控 guest 页表页 GPA 0x3000 的写：
   把 gfn=0x3 加入 gfn_write_track

2. 加入 write_track 时：
   kvm_mmu_slot_gfn_write_protect(kvm, slot, 0x3, PG_LEVEL_4K)
   -> shadow MMU: 通过 rmap 找到映射 gfn=0x3 的 leaf SPTE 并写保护
   -> TDP MMU: 直接遍历 TDP SPTE 写保护

3. Guest 写这个页，触发 write fault / write emulation

4. KVM 在 mmu_pte_write 路径中：
   for_each_gfn_valid_sp_with_gptes(kvm, sp, 0x3)
       找到基于 guest page table page gfn=0x3 构建的 shadow pages
       zap/update 受影响的 shadow SPTEs



## direct_page_fault -> direct_map 为什么在 npt=0 或者 ept=0 的时候也会调用?
<!-- 890e67b5-85ec-4fa1-913a-40708bb6e957 -->


| 配置             | `direct_page_fault` | `direct_map` | `paging64_page_fault` | `kvm_tdp_mmu_map` |
|------------------|---------------------|--------------|-----------------------|-------------------|
| npt=1, tdp_mmu=1 | 0                   | 0            | 0                     | **286,830**       |
| npt=1, tdp_mmu=0 | **244,993**         | **244,993**  | 0                     | 0                 |
| npt=0, tdp_mmu=1 | 27,870              | 27,870       | **2,580,099**         | 0                 |


```
虚拟机页错误发生
        │
        ├─► 检查 npt/ept 是否启用
        │       │
        │       ├─► npt=0 ──► paging64_page_fault ──► 影子页表处理
        │       │
        │       └─► npt=1 ──► 检查 tdp_mmu
        │               │
        │               ├─► tdp_mmu=0 ──► direct_page_fault ──► direct_map
        │               │
        │               └─► tdp_mmu=1 ──► kvm_tdp_mmu_map
        │
        └─► 返回处理结果
```
不过，这里注意到，当 npt = 0 的时候，可以发现少量 `direct_page_fault` 调用可能来自：

我认为这是 direct map 没有启用的时候的情况:
```txt
@[
        direct_map+1
        direct_page_fault+419
        kvm_mmu_do_page_fault+266
        kvm_mmu_page_fault+120
        vcpu_enter_guest.constprop.0+1623
        vcpu_run+55
        kvm_arch_vcpu_ioctl_run+739
        kvm_vcpu_ioctl+724
        __x64_sys_ioctl+161
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 18825
```

## 分别分析一下 rmap 在 shadow page table ，nested ept 和 direct map 的语义和作用是什么
<!-- ce01f614-628c-4632-9741-6ef94699b4e1 -->

## 为什么一个 gfn 可以映射多个 spte ?
因为存在多个地址空间。

### 实现细节 : struct pte_list_desc
<!-- 85a7920c-5b2a-4263-920d-49ea253408df -->

```c
/*
 * struct pte_list_desc is the core data structure used to implement a custom
 * list for tracking a set of related SPTEs, e.g. all the SPTEs that map a
 * given GFN when used in the context of rmaps.  Using a custom list allows KVM
 * to optimize for the common case where many GFNs will have at most a handful
 * of SPTEs pointing at them, i.e. allows packing multiple SPTEs into a small
 * memory footprint, which in turn improves runtime performance by exploiting
 * cache locality.
 *
 * A list is comprised of one or more pte_list_desc objects (descriptors).
 * Each individual descriptor stores up to PTE_LIST_EXT SPTEs.  If a descriptor
 * is full and a new SPTEs needs to be added, a new descriptor is allocated and
 * becomes the head of the list.  This means that by definitions, all tail
 * descriptors are full.
 *
 * Note, the meta data fields are deliberately placed at the start of the
 * structure to optimize the cacheline layout; accessing the descriptor will
 * touch only a single cacheline so long as @spte_count<=6 (or if only the
 * descriptors metadata is accessed).
 */
struct pte_list_desc {
	struct pte_list_desc *more;
	/* The number of PTEs stored in _this_ descriptor. */
	u32 spte_count;
	/* The number of PTEs stored in all tails of this descriptor. */
	u32 tail_count;
	u64 *sptes[PTE_LIST_EXT];
};

struct pte_list_desc {
    struct pte_list_desc *more;    // 指向下一个 descriptor
    u32 spte_count;                // 当前 descriptor 中的 SPTE 数量
    u32 tail_count;                // 所有后续 tail descriptors 的 SPTE 总数
    u64 *sptes[PTE_LIST_EXT];      // 存储 SPTE 指针的数组（最多14个）
};
```
设计目的:
1. 优化常见情况：大多数 GFN 只有少数几个 SPTE 指向它们（比如多 vCPU 或共享内存情况）
2. 小内存占用：使用自定义链表而非通用链表，减少内存开销
3. 缓存友好：元数据放在结构体开头，优化缓存行布局

rmap_head 的三种状态
```txt
 状态                                   描述
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 rmap_head = 0                          空，没有 SPTE
 rmap_head = SPTE指针                   只有 1 个 SPTE，直接存储
 rmap_head = desc指针 | KVM_RMAP_MANY   多个 SPTE，使用 pte_list_desc 链表
```

struct pte_list_desc 是 KVM rmap（反向映射）机制的核心数据结构，
用于跟踪映射到同一个 GFN 的所有 SPTE（Shadow Page Table Entries）。

```c
static void __rmap_add(struct kvm *kvm,
		       struct kvm_mmu_memory_cache *cache,
		       const struct kvm_memory_slot *slot,
		       u64 *spte, gfn_t gfn, unsigned int access)
{
	struct kvm_mmu_page *sp;
	struct kvm_rmap_head *rmap_head;
	int rmap_count;

	sp = sptep_to_sp(spte); // spte 指针的虚拟机地址在 spt 指向的页面中，所以通过 private 找到 kvm_mmu_page
	kvm_mmu_page_set_translation(sp, spte_index(spte), gfn, access); //  知道 gfn ，所以也存储 shadow translation
	kvm_update_page_stats(kvm, sp->role.level, 1);

	rmap_head = gfn_to_rmap(gfn, sp->role.level, slot); // gfn 可以找到对应的 rmap head
	rmap_count = pte_list_add(kvm, cache, spte, rmap_head); // 将 spte 放到 rmap head 中

	if (rmap_count > kvm->stat.max_mmu_rmap_size)
		kvm->stat.max_mmu_rmap_size = rmap_count;
	if (rmap_count > RMAP_RECYCLE_THRESHOLD) {
		kvm_zap_all_rmap_sptes(kvm, rmap_head);
		kvm_flush_remote_tlbs_gfn(kvm, gfn, sp->role.level);
	}
}
```

遍历
```c
#define for_each_rmap_spte(_rmap_head_, _iter_, _sptep_)			\
	__for_each_rmap_spte(_rmap_head_, _iter_, _sptep_)			\
		if (!WARN_ON_ONCE(!is_shadow_present_pte(*(_sptep_))))	\
```

## parent spte
场景 1：标记 unsync（写保护传播）

当 guest 修改页表时，需要标记 shadow page 为 unsync：

```txt
static void kvm_mmu_mark_parents_unsync(struct kvm_mmu_page *sp)
{
    u64 *sptep;
    struct rmap_iterator iter;

    // 遍历所有指向此 sp 的父级 SPTE，将它们标记为 unsync
    for_each_rmap_spte(&sp->parent_ptes, &iter, sptep) {
        mark_unsync(sptep);  // 设置父级 shadow page 的 unsync 标志
    }
}
```

场景 2：断开链接（删除 shadow page）

当销毁 shadow page 时，需要清理所有父级引用：

```txt
static void kvm_mmu_unlink_parents(struct kvm *kvm, struct kvm_mmu_page *sp)
{
    u64 *sptep;
    struct rmap_iterator iter;

    // 遍历并删除所有父级 SPTE
    while ((sptep = rmap_get_first(&sp->parent_ptes, &iter)))
        drop_parent_pte(kvm, sp, sptep);  // 清理父级 SPTE 并移除 rmap 条目
}
```

场景 3：判断是否可销毁（用于 TDP MMU）

```txt
if (tdp_enabled && invalid_list &&
    child->role.guest_mode &&
    !atomic_long_read(&child->parent_ptes.val))  // 没有父级引用，可以销毁
    return kvm_mmu_prepare_zap_page(kvm, child, invalid_list);
```

图示说明

Shadow Page Table 层级结构：

┌──────────────────┐     ┌──────────────────┐     ┌──────────────────┐
│  Root SP         │     │  Shadow Page A   │     │  Shadow Page B   │
│  (level 4)       │     │  (level 3)       │     │  (level 2)       │
│                  │     │                  │     │                  │
│  spt[0] ─────────┼────►│  spt 指向下一级  │     │                  │
│                  │     │                  │     │                  │
│  parent_ptes     │     │  parent_ptes ────┼────►│  sptep (in A)    │
│  (空，无父级)    │     │  [sptep_in_root] │     │  [sptep_in_A]    │
└──────────────────┘     └──────────────────┘     └──────────────────┘
         ▲                        ▲                        ▲
         │                        │                        │
         └────────────────────────┴────────────────────────┘
              通过 parent_ptes 建立反向链接

总结

 特性   说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 作用   维护指向当前 shadow page 的所有父级 SPTE 指针列表
 方向   反向（子 → 父）
 用途   1. 传播 unsync 标记<br>2. 断开链接时清理父级引用<br>3. 判断是否可以销毁 shadow page
 实现   基于 kvm_rmap_head 的链表结构，支持单指针优化和多指针链表
 并发   使用 KVM_RMAP_LOCKED 位实现无锁读取和带锁写入

维护的基本函数:
```c
static void mmu_page_add_parent_pte(struct kvm_vcpu *vcpu,
                    struct kvm_mmu_page *sp, u64 *parent_pte)
{
    if (!parent_pte)
        return;

    pte_list_add(vcpu, parent_pte, &sp->parent_ptes);
}
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
