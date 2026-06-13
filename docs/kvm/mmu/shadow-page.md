## for_each_shadow_entry
<!-- f340ce70-c8aa-4a77-97a8-af04d480e5ab -->

用于 page walk page table 的

1. walk shadow page table 的情况:

当 ept=0 的时候，主要是在 paging64_fetch 中使用 for_each_shadow_entry
```txt
@[
    paging64_fetch+2821
    paging64_page_fault+504
    kvm_mmu_do_page_fault+351
    kvm_mmu_page_fault+209
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 89039
```
这个过程极其正确，因为 paging64_page_fault 其实就是遍历 page table ，只不过
只是这个 page table 是 gva 到 hpa 的 page table ，而 for_each_shadow_entry
就是整个 page walk 的过程.

npt=1, tdp_mmu=0 时使用 `direct_page_fault` → `direct_map` 路径（即传统的 TDP 实现）

2. walk ept 的时候(应该是)

典型的就是 direct_map 中调用了。

### kvm_mmu_memory_cache 到底是做什么的

为什么需要存在这样的 cache
```c
#ifdef KVM_ARCH_NR_OBJS_PER_MEMORY_CACHE
/*
 * Memory caches are used to preallocate memory ahead of various MMU flows,
 * e.g. page fault handlers.  Gracefully handling allocation failures deep in
 * MMU flows is problematic, as is triggering reclaim, I/O, etc... while
 * holding MMU locks.  Note, these caches act more like prefetch buffers than
 * classical caches, i.e. objects are not returned to the cache on being freed.
 *
 * The @capacity field and @objects array are lazily initialized when the cache
 * is topped up (__kvm_mmu_topup_memory_cache()).
 */
struct kvm_mmu_memory_cache {
	int nobjs;
	gfp_t gfp_zero;
	gfp_t gfp_custom;
	struct kmem_cache *kmem_cache;
	int capacity;
	void **objects;
};
#endif
```
这个注释已经说很清楚了，在处理 page fault 的过程中再去 swap 之类的非常麻烦，因为持有 MMU locks 。
但是不知道为什么 kernel 不是如此。



但是这里定义了四个 cache 的:
```c
struct kvm_vcpu_arch {

	struct kvm_mmu_memory_cache mmu_pte_list_desc_cache; // 似乎是处理 rmap 的
	struct kvm_mmu_memory_cache mmu_shadow_page_cache;
	struct kvm_mmu_memory_cache mmu_shadowed_info_cache;
	struct kvm_mmu_memory_cache mmu_page_header_cache;
```
第一个就是 pte_list_add 中分配，其实 rmap 的时候，一个 gfn 中需要添加 page able

其中后面三个就是这样的:

```c
static struct kvm_mmu_page *kvm_mmu_alloc_shadow_page(struct kvm *kvm,
						      struct shadow_page_caches *caches,
						      gfn_t gfn,
						      struct hlist_head *sp_list,
						      union kvm_mmu_page_role role)
{
	struct kvm_mmu_page *sp;

	sp = kvm_mmu_memory_cache_alloc(caches->page_header_cache);
	sp->spt = kvm_mmu_memory_cache_alloc(caches->shadow_page_cache);
	if (!role.direct && role.level <= KVM_MAX_HUGEPAGE_LEVEL)
		sp->shadowed_translation = kvm_mmu_memory_cache_alloc(caches->shadowed_info_cache);
```


- 不对啊，映射一个 44G 的虚拟机，需要多少个 shadow page 啊
  - 取决于 Guest 是否使用大页

## shadow write flood 检测机制
<!-- a2ad55fb-25c4-4090-912e-6ac16aafb150 -->

- 相关函数:
  - shadow_page_table_clear_flood
  - clear_sp_write_flooding_count

基本流程:
- kvm_mmu_pte_write
  - mmu_pte_write_fetch_gpte
  - for_each_gfn_valid_sp_with_gptes
    * detect_write_misaligned
    * detect_write_flooding
    * kvm_mmu_prepare_zap_page : 如果发生如上的检测，处理掉
    * get_written_sptes

检测机制在:
```txt
@[
    kvm_mmu_pte_write+5
    kvm_page_track_write+107
    write_emulate+63
    emulator_read_write_onepage+266
    emulator_read_write+202
    x86_emulate_insn+2202
    x86_emulate_instruction+824
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 206
```

## sp_has_gptes
<!-- 75e4770f-5125-4b8b-947a-20c50ace230a -->

```c
static bool sp_has_gptes(struct kvm_mmu_page *sp)
{
	if (sp->role.direct)
		return false;

	if (sp->role.passthrough)
		return false;

	return true;
}
```

关键判断条件
 条件                      含义                              返回值
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 role.direct = true        直接映射（GPA→HPA，如 TDP/NPT）   false
 role.passthrough = true   直通模式                          false
 其他情况                  传统的影子页表（Shadow Paging）   true

使用场景

该函数主要用于以下三种情况：
1. 宏 for_each_gfn_valid_sp_with_gptes：遍历某个 GFN 对应的所有包含 Guest PTEs 的 shadow pages，跳过 direct 和 passthrough 的页。
2. 创建 shadow page 时（kvm_mmu_alloc_shadow_page）：如果页包含 Guest PTEs，调用 account_shadowed() 进行统计和写保护处理：
  - 增加 kvm->arch.indirect_shadow_pages 计数
  - 对 Guest 页表进行写保护跟踪
3. 销毁 shadow page 时（kvm_mmu_prepare_zap_page）：如果页包含 Guest PTEs，调用 unaccount_shadowed() 清理统计：
  - 减少 indirect_shadow_pages 计数
  - 移除写保护跟踪

总结 : sp_has_gptes 区分了需要跟踪 Guest 页表变化的页（传统影子分页）和不需要跟踪的页（直接映射/直通）。只有前者需要维护 indirect_shadow_pages 计数和写
保护机制，因为它们的翻译依赖于 Guest 页表的内容。

那么，有一个问题，那么每一个 ept page table 的 page ，都是会关联一个 kvm_mmu_page 吗?
答案是完全肯定的!

具体看 : kvm_tdp_mmu_map 中调用 tdp_mmu_alloc_sp
```c
static struct kvm_mmu_page *tdp_mmu_alloc_sp(struct kvm_vcpu *vcpu)
{
	struct kvm_mmu_page *sp;

	sp = kvm_mmu_memory_cache_alloc(&vcpu->arch.mmu_page_header_cache);
	sp->spt = kvm_mmu_memory_cache_alloc(&vcpu->arch.mmu_shadow_page_cache);

	return sp;
}
```

## vmx_load_mmu_pgd
<!-- 8e8db83a-4d15-490d-b74d-db64591e27eb -->

当 guest os 切换 cr3 的时候，那么必然需要 host 对应的切换
```txt
@[
    vmx_load_mmu_pgd+5
    kvm_mmu_load+881
    kvm_arch_vcpu_ioctl_run+4005
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 513
```
- kvm_mmu_load_pgd

## shadow page table 触发 page fault 的过程
<!-- 76293245-9b72-4629-bffc-159ac9ee3c97 -->

- kvm_mmu_do_page_fault
  - kvm_tdp_page_fault : 这就是最佳高速路径了
  - `vcpu->arch.mmu->page_fault(vcpu, &fault);`

如果是 shadow page table 异常了，这个其实和想象的不太一样，原来，总是会去 walk
```txt
@[
        paging64_page_fault+5
        kvm_mmu_do_page_fault+266
        kvm_mmu_page_fault+120
        vcpu_enter_guest.constprop.0+1623
        vcpu_run+55
        kvm_arch_vcpu_ioctl_run+739
        kvm_vcpu_ioctl+724
        __x64_sys_ioctl+161
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 240
```

```txt
  68)               |  paging64_page_fault [kvm]() {
  68)               |    arch_irq_work_raise() {
  68)   8.970 us    |      default_send_IPI_self();
  68)   0.500 us    |      apic_mem_wait_icr_idle();
  68) + 11.590 us   |    }
  68)               |    paging64_walk_addr_generic [kvm]() {
  68)   0.420 us    |      kvm_vcpu_gfn_to_memslot [kvm]();
  68)   0.400 us    |      gfn_to_hva_memslot_prot [kvm]();
  68)   0.390 us    |      kvm_vcpu_gfn_to_memslot [kvm]();
  68)   0.400 us    |      gfn_to_hva_memslot_prot [kvm]();
  68)   0.390 us    |      kvm_vcpu_gfn_to_memslot [kvm]();
  68)   0.400 us    |      gfn_to_hva_memslot_prot [kvm]();
  68)   0.390 us    |      kvm_vcpu_gfn_to_memslot [kvm]();
  68)   0.400 us    |      gfn_to_hva_memslot_prot [kvm]();
  68)   0.410 us    |      svm_get_rflags [kvm_amd]();
  68)   9.870 us    |    }
  68)   0.410 us    |    kvm_vcpu_gfn_to_memslot [kvm]();
  68)               |    mmu_topup_memory_caches [kvm]() {
  68)   0.860 us    |      kvm_mmu_topup_memory_cache [kvm]();
  68)   0.550 us    |      kvm_mmu_topup_memory_cache [kvm]();
  68)   0.540 us    |      kvm_mmu_topup_memory_cache [kvm]();
  68)   0.540 us    |      kvm_mmu_topup_memory_cache [kvm]();
  68)   5.150 us    |    }
  68)               |    kvm_mmu_faultin_pfn [kvm]() {
  68)   0.390 us    |      __rcu_read_lock();
  68)   0.410 us    |      __rcu_read_unlock();
  68)   9.000 us    |      __kvm_mmu_faultin_pfn [kvm]();
  68) + 12.490 us   |    }
  68)   0.410 us    |    _raw_write_lock();
  68)   0.420 us    |    is_page_fault_stale [kvm]();
  68)               |    paging64_fetch [kvm]() {
  68)   1.180 us    |      ept_gpte_changed [kvm]();
  68)   0.400 us    |      shadow_walk_init_using_root [kvm]();
  68)   0.440 us    |      kvm_mmu_get_child_sp [kvm]();
  68)   1.080 us    |      ept_gpte_changed [kvm]();
  68)   0.400 us    |      kvm_mmu_get_child_sp [kvm]();
  68)   1.120 us    |      ept_gpte_changed [kvm]();
  68)   0.420 us    |      kvm_mmu_hugepage_adjust [kvm]();
  68)   3.430 us    |      mmu_set_spte [kvm]();
  68)   0.950 us    |      drop_spte [kvm]();
  68) + 18.120 us   |    }
  68)               |    kvm_mmu_finish_page_fault.isra.0 [kvm]() {
  68)   1.230 us    |      kvm_release_page_dirty [kvm]();
  68)   2.350 us    |    }
  68)   0.400 us    |    _raw_write_unlock();
  68) + 70.260 us   |  }
  12)               |  paging64_page_fault [kvm]() {
```


## shadow page table 的基本工作原理
<!-- 07819967-ce52-4b96-b562-0c37e8dadf8e -->

大致给我分析一下，shadow page table 的流程，并且解释如下问题?
1. 一个需要监控 guest os 的那些行为? 例如 INVPCID ? INVLPG ? mov to cr3 ?
2. 需要监控 guest os 对于 page table 的写吗? 如果写了，该如何处理?
3. 如果 guest 修改了 page table ，如何检测，然后该如何同步?
4. guest 中有多个 process ，他们使用不同的 page table ，是不是就需要建立多个 shadow page table 来对应，如果是，那么当 guest os 一个 process 释放了，什么时候释放对应的 shadow page table


| Guest 行为           | 触发方式                      | KVM 处理                    | 频率                    |
|----------------------|-------------------------------|-----------------------------|-------------------------|
| **MOV to CR3**       | VM Exit (CR3 write)           | 切换 shadow page table root | 高（每次进程/线程切换） |
| **修改页表 PTE/PDE** | Write fault (write-protected) | 同步更新 shadow PTE         | 极高                    |
| **INVLPG**           | VM Exit (INVLPG instruction)  | 使对应 shadow SPTE 失效     | 中等                    |
| **INVPCID**          | VM Exit (INVPCID instruction) | 批量使 shadow SPTEs 失效    | 低                      |
| **MOV to CR4**       | VM Exit (CR4 write)           | 可能重建 shadow page tables | 低                      |
| **MOV to CR0.PG**    | VM Exit (CR0 write)           | 重置 MMU context            | 极低（启动时）          |

shadow page table 的释放是不感知的:

```txt
1. Guest OS 执行 exit() 系统调用
   - KVM 不感知！（纯 guest 内部操作）

2. Guest OS 释放进程 P 的页表
   - Guest 将页表页 free 给 buddy allocator
   - 这些页可能被重用为数据页

3. Guest OS 切换到其他进程（MOV to CR3）
   - VM Exit
   - KVM 处理：kvm_set_cr3(vcpu, other_process_cr3)
   - 进程 P 的 shadow page table root 被 unlink
   - 但内存可能不会立即释放！（缓存）

4. 如果进程 P 的页表页被重用为数据页
   - Guest 写这个页 → Write fault
   - KVM 检测到不再是页表操作
   - 释放对应的 shadow page

   代码：arch/x86/kvm/mmu/mmu.c:1884
   static void mmu_page_remove_parent_pte(struct kvm_mmu_page *sp,
                                          u64 *parent_pte)
   {
       // 从 parent 断开连接
       mmu_page_zap_pte(sp->kvm, sp->parent, parent_pte);
   }

5. 最终释放（三种可能）：
   a) Host 内存压力 → shrinker 释放
   b) Shadow page cache 满 → LRU 替换
   c) VM 关闭 → 释放所有 shadow pages
```

### guest 特殊指令的监控
<!-- 37e1e152-dc9a-4ed2-9884-f8fbb2a2f6b4 -->

```txt
🧀  t
sudo bpftrace -e "kprobe:handle_invlpg { @[kstack] = count(); }"
@[
    handle_invlpg+5
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 110598
```

```txt
@[
    handle_cr+5
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 19086
```

### page table 的写保护监控
<!-- e1a2d7b6-4219-4771-a001-b4272300eb5c -->

测试的时候，首先需要打开这个:
```txt
sudo modprobe -r kvm_amd
sudo modprobe kvm_amd npt=0
```

这里就是 debugfs 中的 mmu_pte_write :
```txt
@[
        kvm_mmu_track_write+5
        write_emulate+82
        emulator_read_write_onepage+288
        emulator_read_write+209
        x86_emulate_insn+1437
        x86_emulate_instruction+910
        vcpu_enter_guest.constprop.0+1623
        vcpu_run+55
        kvm_arch_vcpu_ioctl_run+739
        kvm_vcpu_ioctl+724
        __x64_sys_ioctl+161
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 132512
```

这个还是需要嵌套虚拟化来调试，L1 使用 ept ，然后 L2 使用 shadow page table 。
这个折叠搞的太厉害了，从 vcpu_enter_guest 直接到了 vcpu_enter_guest

## kvm_mmu_root_info
<!-- 77067c4c-e943-4bc2-95e8-07c0b4ac9888 -->

```c
struct kvm_mmu_root_info {
    gpa_t pgd;     // Guest CR3（GPA）
    hpa_t hpa;     // Shadow root 的 HPA
};
```

```c
struct kvm_mmu {
	// ...
	struct kvm_mmu_root_info root;                    // 当前使用的 root
	// ...
	struct kvm_mmu_root_info prev_roots[3];           // 缓存的 3 个 previous roots
};
```

```txt
**缓存替换策略**：
- 使用 LRU（最近最少使用）策略管理 prev_roots
- 最多缓存 3 个 previous roots
- 当缓存未命中时，需要通过 `kvm_mmu_reload()` → `kvm_mmu_load()` 分配新的 shadow root
```

```txt
kvm_mmu_new_pgd() - fast_pgd_switch 返回 false
    ↓
下次 VM entry 时调用 kvm_mmu_reload()
    ↓
kvm_mmu_load()
    ├─ mmu_alloc_shadow_roots()   // 分配 shadow root
    ├─ kvm_mmu_sync_roots()       // 同步 root
    └─ kvm_mmu_load_pgd()         // 加载到硬件
```

kvm_mmu_load() - 加载 Shadow MMU

```c
int kvm_mmu_load(struct kvm_vcpu *vcpu)
{
    // 1. 准备内存缓存
    r = mmu_topup_memory_caches(vcpu, !vcpu->arch.mmu->root_role.direct);

    // 2. 分配 special roots（如需要）
    r = mmu_alloc_special_roots(vcpu);

    // 3. 分配 shadow root
    if (vcpu->arch.mmu->root_role.direct)
        r = mmu_alloc_direct_roots(vcpu);    // TDP 模式
    else
        r = mmu_alloc_shadow_roots(vcpu);    // Shadow 模式

    // 4. 同步 root
    kvm_mmu_sync_roots(vcpu);

    // 5. 加载到硬件
    kvm_mmu_load_pgd(vcpu); // 这里和具体的 vmx 还是 svm 有关，vmx 的

    // 6. 刷新 TLB
    kvm_x86_call(flush_tlb_current)(vcpu);
}
```

**Q2: Host 是否需要为 guest 的每一个 CR3 维护一个页面？**

A: 不需要。KVM 采用缓存机制，只维护最多 4 个（1 个当前 + 3 个缓存）shadow roots。当缓存未命中时，会分配新的 shadow root，旧的会被回收或释放。

这里我有点奇怪，如果内核不命中缓存，岂不是所有之前建立的 shadow page table 都需要清理掉?
找到证据

## 7. Shadow Page Table 生命周期与缓存不命中处理

### 7.1 Shadow Page 的引用计数机制

每个 shadow page 通过以下机制管理生命周期：

```c
struct kvm_mmu_page {
    int root_count;                    // 作为 root 被引用的次数
    struct kvm_rmap_head parent_ptes;  // 父页面引用列表（反向映射）
    bool tdp_mmu_page;
    ...
};
```

**两种引用方式**：
1. **Root 引用**：当 shadow page 作为 root 使用时，`root_count` 增加
2. **Parent 引用**：当 shadow page 被其他 shadow page 的 SPTE 指向时，通过 `parent_ptes` 记录

### 7.2 缓存不命中时的完整处理流程

当 Guest 切换 CR3 且缓存不命中时：

```
kvm_mmu_new_pgd(vcpu, new_cr3)
    └── fast_pgd_switch() 返回 false（缓存未命中）
        └── kvm_mmu_free_roots(kvm, mmu, KVM_MMU_ROOT_CURRENT)
            └── mmu_free_root_page(kvm, &mmu->root.hpa, &invalid_list)
                └── sp->root_count--
                    └── 如果 root_count == 0 且 sp->role.invalid:
                        └── kvm_mmu_prepare_zap_page() → 递归清理子树
```

**递归清理过程详解**：

```c
// 1. 清理 unsync 的子页面（那些标记为 unsync 的叶子节点）
mmu_zap_unsync_children(kvm, sp, invalid_list);

// 2. 断开所有子页面链接（遍历 512 个 SPTE）
kvm_mmu_page_unlink_children(kvm, sp, invalid_list)
    └── for (i = 0; i < 512; i++)
        └── mmu_page_zap_pte(kvm, sp, sp->spt + i, invalid_list)
            └── if (是非叶子 SPTE):
                └── child = spte_to_child_sp(pte)
                └── drop_parent_pte(kvm, child, spte)  // 减少 child 的 parent 引用
                └── if (child 没有 parent 了 && 是 nested TDP):
                    └── kvm_mmu_prepare_zap_page(kvm, child, invalid_list)  // 递归清理

// 3. 断开与父页面的链接
kvm_mmu_unlink_parents(kvm, sp)
    └── while (还有 parent_pte):
        └── drop_parent_pte(kvm, sp, sptep)  // 减少自己的 parent 引用

// 4. 标记为无效
sp->role.invalid = 1;

// 5. 如果 root_count == 0，加入释放列表
if (!sp->root_count) {
    list_move(&sp->link, invalid_list);
    kvm_unaccount_mmu_page(kvm, sp);
}
```

### 7.3 为什么普通 Shadow Paging 不会立即清理子页面？

在代码中有这样一个关键条件（mmu.c:2622-2626）：

```c
if (tdp_enabled && invalid_list &&
    child->role.guest_mode &&
    !atomic_long_read(&child->parent_ptes.val))
    return kvm_mmu_prepare_zap_page(kvm, child, invalid_list);
```

**注意**：这个递归清理**仅在以下条件下触发**：
1. `tdp_enabled` 为真（硬件支持 NPT/EPT）
2. 是 nested guest mode
3. 子页面没有其他父引用了

**对于普通 shadow paging**（非 TDP，即软件 shadow paging）：
- 子页面（PMD/PTE 级别）只是断开与当前 root 的链接
- 但子页面仍然保留在 hash 表中（通过 `hash_link`）
- 如果其他 shadow page table 引用了相同的 guest page，这些子页面可以被复用
- 如果没有任何引用了，后续通过 shrinker 或重新分配时回收

### 7.4 缓存机制的价值

**为什么需要 prev_roots 缓存？**

```
场景：Guest 在 3 个地址空间 A、B、C 之间频繁切换

没有缓存时：
  切换到 A: 分配新的 shadow root → 构建 shadow page table → 使用
  切换到 B: 释放 A 的 shadow root → 分配新的 shadow root → 构建 shadow page table → 使用
  切换回 A: 释放 B 的 shadow root → 分配新的 shadow root → 重新构建 shadow page table → 使用

有缓存时（prev_roots[3]）：
  切换到 A: 分配新的 shadow root → 构建 shadow page table → 使用
  切换到 B: A 的 root 移到 prev_roots[0] → 分配 B 的 root → 使用
  切换回 A: 在 prev_roots 中找到 A 的 root → 直接复用！
```

**性能影响**：
- 缓存命中：O(1) 时间切换，无需重新构建 shadow page table
- 缓存不命中：需要分配新的 shadow root，逐步构建 shadow page table（按需 fault-in）

### 其实还是非常奇怪，既然有 kvm->arch.mmu_page_hash 来实现 gfn 到kvm_mmu_page 的链接，那么为什么还需要缓存 root ?

## 写保护在 KVM 的 Shadow Page Table (SPT) 中仍然是不可或缺的基石
<!-- 38ebb90a-e8b5-4b8c-b55e-2af560e244e1 -->

OOS (Out of Sync) Shadow Pages（不同步影子页表）。你的直觉很敏锐：在 OOS 机制下，KVM 确实取消了部分页表的写保护，转而依赖拦截 invlpg 指令和 CR3 寄存器的刷新来同步页表。

### 1. 并非所有页表修改都会触发 `invlpg`

这是最致命的一点。根据 x86 架构规范（Intel SDM），CPU 的 TLB **不会缓存“无效”（Invalid/Not Present）的页表项**。

* **修改已存在的映射（Valid -> Valid / Valid -> Invalid）：** 此时 TLB 中可能有旧的缓存，Guest OS 必须主动调用 `invlpg` 来刷新 TLB。KVM 拦截到 `invlpg`，确实可以借机同步。
* **创建新的映射（Invalid -> Valid）：** 当 Guest OS 映射一块全新的内存时，由于之前是无效的，TLB 根本没有缓存。因此，**Guest OS 在写完新的页表项后，根本不需要、也不会去执行 `invlpg**`。它会直接去访问那块内存。
* **结果：** 如果 KVM 完全放弃写保护且只盯着 `invlpg`，那么当 Guest 建立新映射时，KVM 会完全不知情。

(那么，对于新创建的映射，是如何跟踪到的)

### 2. KVM 必须知道“哪些内存页被用作了页表”

Guest OS 内部会频繁地分配和释放内存页。当 Guest 分配一个物理页（GPA）作为自己的页表（GPT）时，它不会发邮件通知 KVM：“嘿，我把这页当页表用了”。

KVM 是如何知道的？**靠的就是写保护。**

* 当 Guest 首次通过缺页异常（Page Fault）访问某个地址时，KVM 会 walk Guest 的页表（Walk GPT），以此来建立自己的影子页表（SPT）。
* 在这个过程中，KVM 发现了 Guest 正在使用某几个特定的内存页作为页表。为了防止 Guest 悄悄篡改这些页表结构，KVM 必须**将这些 Guest 页表所在的内存页设为只读（写保护）**。
* 一旦 Guest 尝试修改这些页表（例如写入新的 PTE），就会触发写保护异常（VM-Exit）。KVM 捕获异常后，才知道：“哦，Guest 修改了页表，我得更新我的影子页表了”。

(注意，也就是，并不会使用全量的 page table 的同步，而是遇到了 page fault 的时候才会对应的更新的)

### 3. 非叶子节点页表（Non-leaf Page Tables）无法使用 OOS

前面提到的“依赖 `invlpg` 取消写保护”的 OOS 优化，**仅仅适用于最底层的叶子节点页表（PTE 级别）**。

对于更高层级的目录结构（如 PGD, PUD, PMD）：

* 它们控制着巨大的内存范围（例如一个 PGD 项的变化可能影响数百 GB 的寻址）。
* 如果允许 Guest 随意修改这些上层目录而不立刻拦截，KVM 的影子页表树结构瞬间就会和 Guest 完全脱节，引发灾难性的状态混乱。
* 因此，**所有非叶子节点的页表必须保持严格的写保护**，任何修改都必须立刻触发 VM-Exit 进行同步。

### 4. 脏页追踪（Dirty Page Tracking）与热迁移

当虚拟机进行热迁移（Live Migration）或向磁盘 swapping 时，Host 需要知道哪些内存页被 Guest 修改过（Dirty Pages）。
在没有硬件 A/D 辅助或 EPT 脏页记录的年代，KVM 也是通过**写保护机制**来实现的：把正常的内存页设为只读，
Guest 一写就触发 VM-Exit，KVM 记录下这个脏页，然后再放行。

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
