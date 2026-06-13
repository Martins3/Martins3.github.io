# 为什么 kvm 需要特殊处理 hugepage
<!-- 54fd2d69-0d5f-4cc9-8240-ecb43b9d5572 -->

1. 热迁移
2. nx hugepage 来解决 itlb multihit 漏洞

## qemu 如何配置使用大页

如果不是 hugepage 的方法: -object
memory-backend-memfd,id=mem0,size=${ramsize}G,prealloc=off,share=on,hugetlb=true

如果是 transparent hugepage :

也是没问题的，但是我不知道为什么，是需要是关掉 share 才可以
也就是配置为:
```txt
	-object memory-backend-ram,id=mem0,size=8G,prealloc=off,share=off
```
测试结果:
```txt
pages_1g:0
pages_2m:1076
pages_4k:463
```


## kvm 使用 hugepage 的决策过程
<!-- 4ae71447-faab-4bf3-9b4b-434dcb58f672 -->

```
页面故障
    ↓
kvm_mmu_hugepage_adjust()
    ↓
检查限制条件：
    ├─ 脏页追踪？          → 强制 4KB
    ├─ NX 大页缓解？       → 降级到 4KB
    ├─ 主机不支持大页？     → 降级
    └─ Guest 页表不支持？   → 降级
    ↓
计算 goal_level
    ↓
构建页表时使用 goal_level 停止遍历
```

**因素 1：主机内存映射**

```c
int kvm_mmu_max_mapping_level(struct kvm *kvm, struct kvm_page_fault *fault,
                              const struct kvm_memory_slot *slot, gfn_t gfn)
{
    // 查询主机 page table 中该地址的映射级别
    host_level = host_pfn_mapping_level(kvm, gfn, slot);

    // 返回 min(主机级别, KVM 支持的最大级别)
    return min(host_level, max_level);
}
```
例如:
- 主机用 2MB hugepage 映射 → KVM 最多用 2MB
- 主机用 4KB 页映射 → KVM 只能用 4KB
- 主机用 1GB hugepage 映射 → KVM 最多用 1GB（如果支持）

默认情况:

```txt
root@localhost:/sys/kernel/debug/kvm# cat pages_*
0
0
1697386
```

当使用大页的时候，也就是

```txt
-object memory-backend-memfd,id=mem0,size=8G,prealloc=off,share=on,hugetlb=true
```

**因素 2：脏页追踪状态**

```c
if (kvm_slot_dirty_track_enabled(slot))
    return;  // 强制使用 4KB 页
```

**因素3 : NX 大页缓解**

```c
fault->huge_page_disallowed = fault->exec && fault->nx_huge_page_workaround_enabled;
```

## kvm eager_page_split 的作用
<!-- d659a23a-c640-458d-8113-099294d3d29f -->

原来是给热迁移的 dirty page tracking 用的

```txt
当启用脏页追踪（如 VM 迁移）时，KVM 需要使用 4KB 小页来精确追踪每个页面是否被修改。传统方式（Lazy Split）是：
使用 2MB 大页 → 启用追踪 → 写保护大页 →
Guest 写入时触发页面故障 → 在故障处理中分割大页 → 性能抖动

Eager Split 的解决方案

使用 2MB 大页 → 启用追踪 → 立即主动分割所有大页为 4KB →
Guest 写入时无需页面故障 → 性能稳定
```

基本的调用路线为:

- kvm_set_memslot
  - kvm_commit_memory_region
    - kvm_arch_commit_memory_region
      - kvm_mmu_slot_apply_flags

这是 eager_page_split 的唯一使用地方

```txt
if (READ_ONCE(eager_page_split))
	kvm_mmu_slot_try_split_huge_pages(kvm, new, PG_LEVEL_4K);
```

```txt
VM 启动
    ↓
[ 正常运行 - 使用大页 ]
    ├─ 性能最优
    └─ TLB 压力最小
    ↓
[ 准备迁移 - 启用脏页追踪 ]
    ├─ eager_page_split=1: 立即分割所有大页（推荐）
    └─ eager_page_split=0: 延迟分割（首次写入时）
    ↓
[ 迁移中 - 全部 4KB 页 ]
    ├─ 精确追踪脏页
    └─ 只传输修改的页面
    ↓
[ 迁移完成 - 关闭脏页追踪 ]
    └─ 恢复大页映射
    ↓
[ 继续运行 - 使用大页 ]
```

恢复过程 : kvm_tdp_mmu_recover_huge_pages

## nx hugepage
<!-- 0262e69b-25c5-4735-9b38-4f6a19e0826a -->

原理参考 Documentation/admin-guide/hw-vuln/multihit.rst

Under this errata, instructions are fetched from a linear address translated
using a 4 KB translation cached in the iTLB. Privileged software modifies the
paging structure so that the same linear address using large page size (2 MB, 4
MB, 1 GB) with a different physical address or memory type. After the page
structure modification but before the software invalidates any iTLB entries for
the linear address, a code fetch that happens on the same linear address may
cause a machine-check error which can result in a system hang or shutdown.

If EPT is disabled or not available on the host, KVM is in control of TLB
flushes and the problematic situation cannot happen. However, the shadow EPT
paging mechanism used by nested virtualization is vulnerable, because the nested
guest can trigger multiple iTLB hits by modifying its own (non-nested) page
tables. For simplicity, KVM will make large pages non-executable in all shadow
paging modes.

If the guest comes from a trusted source, you may assume that the guest will not
attempt to maliciously exploit these errata and no further action is required.

总结一下，就是恶意的 Guest OS 修改 page table ，将原来的 4k 修改为 2M / 1G
但是不去 刷新 iTLB ，那么执行对应代码的时候， iTLB
同时命中两个条目，就会触发问题。

检查方法: /sys/devices/system/cpu/vulnerabilities/itlb_multihit

KVM 的缓解方案的 核心策略：将所有大页标记为不可执行（NX）

```txt
分配大页 → 标记为 NX（不可执行）
    ↓
Guest 尝试执行 → EPT Violation
    ↓
KVM 分割大页为 4KB 页
    ↓
标记 4KB 页为可执行
    ↓
所有 iTLB 条目都是 4KB → 无法触发 multihit
```

https://docs.redhat.com/zh_hans/documentation/red_hat_enterprise_linux/8/html/8.2_release_notes/kernel_parameters_changes

```txt
kvm.nx_huge_pages = [KVM]
这个参数控制 X86_BUG_ITLB_MULTIHIT bug 的软件临时解决方案。

这些选项是：
force - 始终部署临时解决方案。
off - 无法部署临时解决方案。
auto （默认）- 根据 X86_BUG_ITLB_MULTIHIT 的存在部署临时解决方案。
如果为主机启用了软件临时解决方案，则客户机不需要为嵌套的客户机启用它。
```

### 影响

```txt
/*
 * We cannot overwrite existing page tables with an NX
 * large page, as the leaf could be executable.
 */
if (fault->nx_huge_page_workaround_enabled)
	disallowed_hugepage_adjust(fault, *it.sptep, it.level);
```

```c
void disallowed_hugepage_adjust(struct kvm_page_fault *fault, u64 spte, int cur_level)
{
    // 检查条件：
    // 1. cur_level > PG_LEVEL_4K - 当前级别不是最底层
    // 2. cur_level == fault->goal_level - 想要在当前级别创建大页
    // 3. is_shadow_present_pte(spte) - 当前 SPTE 存在
    // 4. !is_large_pte(spte) - 当前 SPTE 不是大页（是页表指针）
    // 5. sp->nx_huge_page_disallowed - 子页表被标记为禁止 NX 大页
    if (cur_level > PG_LEVEL_4K &&
        cur_level == fault->goal_level &&
        is_shadow_present_pte(spte) &&
        !is_large_pte(spte) &&
        spte_to_child_sp(spte)->nx_huge_page_disallowed) {

        /*
         * A small SPTE exists for this pfn, but FNAME(fetch),
         * direct_map(), or kvm_tdp_mmu_map() would like to create a
         * large PTE instead: just force them to go down another level,
         * patching back for them into pfn the next 9 bits of the
         * address.
         */

        // 计算页面掩码
        u64 page_mask = KVM_PAGES_PER_HPAGE(cur_level) -
                        KVM_PAGES_PER_HPAGE(cur_level - 1);

        // 调整 PFN（恢复下一级的偏移）
        fault->pfn |= fault->gfn & page_mask;

        // 降级目标级别
        fault->goal_level--;
    }
}
```

**没有这个检查会发生什么**：

```
错误流程（假设没有 disallowed_hugepage_adjust）：
  t2: 页面故障，goal_level = PG_LEVEL_2M
      ├─ 遍历到 Level 2
      ├─ it.level (2) == goal_level (2) → 停止遍历
      ├─ 在 Level 2 创建 2MB 大页
      └─ 覆盖掉原来的页表指针

  结果：
      ├─ 原来的 PT（页表页）被丢弃
      ├─ PT[0]（可执行页面）消失
      ├─ Guest 尝试执行 PT[0] → NX 故障（因为新大页是 NX）
      └─ 破坏了 Guest 的正常功能！
```




### 优化措施

NX 大页缓解虽然解决了安全问题，但带来了性能问题：

启用 NX 缓解 → 大页标记为 NX → Guest 执行 → 分割为 4KB 页 → 永久保持 4KB →
性能下降

Recovery Worker 的解决方案

它是一个后台线程，周期性地将"不再执行"的 4KB 页面恢复为大页，实现了：

执行时 → 4KB 页（安全） 非执行时 → 2MB 大页（性能）

工作原理

每隔一段时间（默认约 1 分钟）： 1. 唤醒恢复线程 2. 从 possible_nx_huge_pages
链表中选择一部分页面（默认 1/60） 3. 检查是否正在脏页追踪 4.
如果未追踪：清除（zap）这些 4KB 页表 5. 下次 Guest 访问时： - 如果执行 → 分割为
4KB - 如果不执行 → 保持为大页

关键特性

1. 渐进式恢复：每次只恢复一小部分（1/60）
2. 智能跳过：脏页追踪期间不恢复
3. 低开销：每分钟约 200μs 的暂停
4. 可配置：支持调整周期和比例

控制参数

```txt
# 恢复周期（毫秒）
/sys/module/kvm/parameters/nx_huge_pages_recovery_period_ms
# 默认：0（自动计算）

# 恢复比例（每次恢复的比例）
/sys/module/kvm/parameters/nx_huge_pages_recovery_ratio
# 默认：60（每次恢复 1/60 ≈ 1.67%）
# PREEMPT_RT：0（禁用）

# 禁用恢复
echo 0 > /sys/module/kvm/parameters/nx_huge_pages_recovery_ratio
```

这就是有时候我们可以观察到 kthread kvm-nx-lpage-recovery-3487793 的原因:

```txt
1 S root     3487925       2  0  80   0 -     0 -      Feb04 ?        00:00:00 [kvm-nx-lpage-recovery-3487793]
```

典型场景

场景 1：代码很少执行 执行 → 分割为 4KB → 1 分钟后恢复为大页 → 保持大页 ✅ 性能好

场景 2：代码频繁执行 执行 → 分割为 4KB → 1 分钟后恢复 → 立即再次执行 → 再次分割
结果：大部分时间保持 4KB（符合预期）

场景 3：VM 迁移期间 启用脏页追踪 → 恢复暂停 → 保持 4KB（精确追踪） 迁移完成 →
恢复继续 → 逐步恢复大页

设计价值

这是一个经典的性能优化案例：

问题：NX 缓解导致永久使用 4KB 页 方案：周期性恢复不再执行的页面
效果：在安全（iTLB multihit 缓解）和性能（大页优化）之间取得动态平衡

核心思想：基于观察"Guest
通常不会持续执行所有代码页"，通过智能的后台恢复，显著减轻了安全缓解的性能损失。

## 扩展
- https://lore.kernel.org/all/20210507110322.23348-3-zhukeqian1@huawei.com/#r
- https://lore.kernel.org/all/20201025230626.18501-1-gshan@redhat.com/#r

ARM 的 hugetlb 问题

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
