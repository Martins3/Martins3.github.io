## tlb flush 的基本原理
<!-- db098ab6-d3c2-4e6e-9806-ae51dc1980eb -->

当我们来观察 tlb flush 的时候，需要注意这些问题:
1. 多线程程序共享地址空间，一个 thread 修改了映射，需要同步到其他的 core 中，也就是 mprotect 结束之后，其他的 thread 都需要感知到
2. 存在多个地址空间

```c
#define TLB_FLUSH_REASON						\
	EM(  TLB_FLUSH_ON_TASK_SWITCH,	"flush on task switch" )	\
	EM(  TLB_REMOTE_SHOOTDOWN,	"remote shootdown" )		\
	EM(  TLB_LOCAL_SHOOTDOWN,	"local shootdown" )		\
	EM(  TLB_LOCAL_MM_SHOOTDOWN,	"local MM shootdown" )		\
	EM(  TLB_REMOTE_SEND_IPI,	"remote IPI send" )		\
	EMe( TLB_REMOTE_WRONG_CPU,	"remote wrong CPU" )
```
然后观察 trace_tlb_flush 的使用地方。


在任何一个系统中，sudo perf top -e tlb:tlb_flush 都是可以观察到这样的效果的:
```txt
  41.19%  pages:1 reason:local MM shootdown (3)
  37.93%  pages:-1 reason:flush on task switch (0)
   9.38%  pages:1 reason:remote IPI send (4)
   2.02%  pages:0 reason:remote wrong CPU (5)
   1.58%  pages:1 reason:remote shootdown (1)
   1.46%  pages:16 reason:local MM shootdown (3)
   1.15%  pages:2 reason:local MM shootdown (3)
   1.11%  pages:-1 reason:local MM shootdown (3)
   0.88%  pages:16 reason:remote IPI send (4)
   0.71%  pages:-1 reason:remote IPI send (4)
   0.60%  pages:2 reason:remote IPI send (4)
   0.39%  pages:-1 reason:remote shootdown (1)
   0.36%  pages:4 reason:local MM shootdown (3)
   0.36%  pages:3 reason:local MM shootdown (3)
   0.10%  pages:4 reason:remote IPI send (4)
   0.09%  pages:9 reason:local MM shootdown (3)
   0.09%  pages:2 reason:remote shootdown (1)
   0.09%  pages:5 reason:local MM shootdown (3)
   0.07%  pages:5 reason:remote IPI send (4)
   0.05%  pages:6 reason:local MM shootdown (3)
   0.04%  pages:16 reason:remote shootdown (1)
   0.04%  pages:3 reason:remote IPI send (4)
   0.03%  pages:6 reason:remote IPI send (4)
   0.03%  pages:12 reason:local MM shootdown (3)

```

## TLB_LOCAL_SHOOTDOWN vs TLB_LOCAL_MM_SHOOTDOWN 使用场景
<!-- 5b32758b-b11b-44ff-b58a-a55ffcd33c4e -->

TLB_LOCAL_SHOOTDOWN（mm = NULL，内核全局刷新）

用于刷新内核全局映射，与任何用户进程无关：

 使用场景                说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 vmalloc() / vfree()     内核虚拟地址空间分配/释放后刷新
 set_memory_*()          修改内核页表属性（如将代码段设为只读）
 kasan 影子内存          修改 KASAN 影子内存映射
 hugetlb vmemmap         大页虚拟内存映射优化时
 percpu 内存             percpu 内存映射变更
 secretmem               机密内存映射
 ioremap() / iounmap()   设备内存映射
 KMSAN shadow            内存消毒器影子内存
 hibernate/suspend       系统休眠/恢复时

典型调用示例：

// mm/vmalloc.c: 修改 vmalloc 区域后
flush_tlb_kernel_range(addr, end);

// mm/kasan/shadow.c: KASAN 影子内存修改后
flush_tlb_kernel_range((unsigned long)shadow_start, ...);

// arch/x86/mm/pgtable.c: 清除 PGD 页表项后
flush_tlb_kernel_range(addr, addr + PAGE_SIZE-1);

────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
TLB_LOCAL_MM_SHOOTDOWN（mm 有效，特定进程刷新）

用于刷新特定用户进程的 TLB 条目：

 使用场景             说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 exit_mmap()          进程退出时清理内存映射
 khugepaged           透明大页合并失败时回滚
 try_to_unmap()       反向映射批量 flush
 mprotect()           修改内存保护属性
 munmap()             解除内存映射
 page migration       页面迁移
 madvise()            内存建议操作
 remap_file_pages()   文件页重映射

典型调用示例：

// mm/mmap.c: 进程退出时
flush_tlb_mm(oldmm);

// mm/khugepaged.c: 大页合并且回滚
flush_tlb_mm(mm);

// mm/rmap.c: 反向映射批量 flush
if (pending != flushed)
    flush_tlb_mm(mm);

核心区别总结

            TLB_LOCAL_SHOOTDOWN              TLB_LOCAL_MM_SHOOTDOWN
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 目标       内核全局地址空间                 特定用户进程地址空间
 mm 参数    NULL                             有效的 struct mm_struct *
 常见触发   vmalloc, ioremap, set_memory_*   munmap, mprotect, 页面回收/迁移
 影响范围   所有 CPU 的内核映射              仅目标进程的映射

## TLB_REMOTE_WRONG_CPU
<!-- a66a0ba5-0cd8-4792-ab57-a85536fff207 -->

`TLB_REMOTE_WRONG_CPU` 是 Linux 内核中用于统计特定竞态条件（race condition）的性能事件（tracepoint）。
**这不是错误**，而是 TLB 刷新机制的正常行为。

当多个 CPU 之间进行 TLB 刷新时，可能出现以下时序：

```
┌─────────────┐                      ┌─────────────┐
│   CPU A     │                      │   CPU B     │
├─────────────┤                      ├─────────────┤
│ 运行进程P1  │                      │             │
│ (mm1)       │                      │             │
│             │───1. 上下文切换─────▶│ 运行进程P2  │
│             │   到P2 (mm2)         │ (mm2)       │
│             │◀──2. 发送TLB刷新IPI──│ 需要刷新mm1 │
│             │   (基于旧的cpumask)  │ 的TLB       │
│             │                      │             │
│ 收到IPI,    │                      │             │
│ 但已不在mm1 │───3. 发现mm≠loaded_mm│             │
│             │   TLB_REMOTE_WRONG_CPU++           │
└─────────────┘                      └─────────────┘
```

**时序说明：**
1. CPU A 从进程 P1 上下文切换到 P2
2. CPU B 基于旧的 `mm_cpumask` 向 CPU A 发送 TLB 刷新 IPI
3. CPU A 收到 IPI，但发现自己已经不在 mm1 上运行

位于 `arch/x86/mm/tlb.c` 的 `flush_tlb_func()` 函数：

```c
/* The CPU was left in the mm_cpumask of the target mm. Clear it. */
if (f->mm && f->mm != loaded_mm) {
    cpumask_clear_cpu(raw_smp_processor_id(), mm_cpumask(f->mm));
    trace_tlb_flush(TLB_REMOTE_WRONG_CPU, 0);
    return;
}
```

当收到远程 TLB 刷新 IPI 时，CPU 检查：
- `f->mm`: 请求刷新的目标内存描述符
- `loaded_mm`: 当前 CPU 实际加载的内存描述符

如果两者**不一致**，说明 CPU 已经切换到其他进程，不需要刷新旧的 TLB。

`mm_cpumask` 是一个位图，记录哪些 CPU**可能**正在运行该进程的线程：

```
┌─────────────────────────────────────┐
│           mm_struct                 │
├─────────────────────────────────────┤
│  pgd: 页表基址                      │
│  mm_cpumask: {CPU0, CPU2, CPU5}     │  ← 这些CPU可能在运行此进程
│  context.tlb_gen: 版本号            │
└─────────────────────────────────────┘
```

**`mm_cpumask` 是延迟更新的**，存在窗口期：
1. 上下文切换时，CPU 可能还没来得及从旧 `mm_cpumask` 中移除自己
2. 其他 CPU 基于旧的 `mm_cpumask` 发送 IPI


## TLB_REMOTE_SEND_IPI 和 TLB_REMOTE_SHOOTDOWN
<!-- acbbe9b7-d92d-4947-8e6f-76d6ff62fb3e -->

 枚举值                 触发位置                 含义                      视角
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 TLB_REMOTE_SEND_IPI    native_flush_tlb_multi   发送 IPI 请求其他 CPU f   发起者视角
                        ()                       lush
 TLB_REMOTE_SHOOTDOWN   flush_tlb_func()         接收 IPI 并执行 flush     接收者视角

代码流程

进程 A 在 CPU 0 上执行
    │
    ▼
┌─────────────────────────────────────┐
│ native_flush_tlb_multi()            │
│   ├─ trace_tlb_flush(TLB_REMOTE_SEND_IPI, ...)  ← 记录"发送 IPI"
│   ├─ on_each_cpu_mask/cond_mask()   │
│   │      发送 IPI 给其他 CPU        │
│   └─ ...                            │
└─────────────────────────────────────┘
    │
    ▼
其他 CPU（如 CPU 1,2,3...）收到 IPI
    │
    ▼
┌─────────────────────────────────────┐
│ flush_tlb_func()                    │
│   ├─ 检查是否本地 CPU               │
│   ├─ trace_tlb_flush(TLB_REMOTE_SHOOTDOWN, ...)  ← 记录"收到 IPI 并执行"
│   └─ 实际执行 TLB flush             │
└─────────────────────────────────────┘

关键区别

1. TLB_REMOTE_SEND_IPI（发送方记录）
  • 在发起 flush 的 CPU 上执行
  • 记录的是请求发送的动作
  • pages 参数表示要刷新的页数（TLB_FLUSH_ALL 或范围）
2. TLB_REMOTE_SHOOTDOWN（接收方记录）
  • 在接收 IPI 的远程 CPU 上执行
  • 记录的是实际执行 flush 的动作
  • nr_invalidate 表示实际刷新的页数

为什么需要区分

这对性能分析很重要：

• TLB_REMOTE_SEND_IPI 多 → 系统频繁发起远程 flush 请求
• TLB_REMOTE_SHOOTDOWN 多 → 远程 CPU 频繁处理 flush 请求
• 两者差距大 → 可能有很多 IPI 被优化掉了（如 lazy TLB 模式跳过）

典型比例

在 128 核系统上，如果所有 CPU 都运行相关进程：

• 1 次 TLB_REMOTE_SEND_IPI（1 个发起者）
• N 次 TLB_REMOTE_SHOOTDOWN（N 个接收者）

## TLB_FLUSH_ON_TASK_SWITCH
<!-- 9411be31-7590-4464-a8a3-2be51ad68acc -->

(没有细看，似乎比较正确，不过，两个 amd 机器测试都是 pages = -1 的？)

> [!NOTE]
> 参考神奇海螺的意见，有待验证

触发位置
// arch/x86/mm/tlb.c:947-959
if (ns.need_flush) {
    load_new_mm_cr3(next->pgd, ns.asid, new_lam, true);
    trace_tlb_flush(TLB_FLUSH_ON_TASK_SWITCH, TLB_FLUSH_ALL);  // pages=-1
} else {
    load_new_mm_cr3(next->pgd, ns.asid, new_lam, false);
    trace_tlb_flush(TLB_FLUSH_ON_TASK_SWITCH, 0);              // pages=0
}

两种场景

 pages 值             含义                场景
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 TLB_FLUSH_ALL (-1)   需要完整 TLB 刷新   新 ASID 需要初始化，或 CPU 不支持 PCID
 0                    不需要刷新          ASID 已经是最新的，直接复用

核心机制：ASID（Address Space ID）

x86-64 使用 PCID（Process-Context Identifiers） 来优化 TLB：

无 PCID 时代：
  每次切换进程 → 必须刷新整个 TLB（代价大）

有 PCID 时代：
  每个进程分配一个 ASID
  切换进程时只需切换 ASID，不需要刷新 TLB
  只有 ASID 用尽时才需要刷新

具体场景

需要刷新（pages=-1）：

1. CPU 不支持 PCID → 每次切换都必须刷新
2. ASID 耗尽 → 需要回收 ASID，必须刷新
3. 新进程首次运行 → ASID 需要初始化

不需要刷新（pages=0）：

1. 同一进程的不同线程 → 共享同一 mm，ASID 相同
2. ASID 缓存命中 → 该 ASID 已经是最新版本
3. INVLPGB 全局 ASID → 硬件自动维护一致性

代码逻辑流程

choose_new_asid(next, next_tlb_gen)
    │
    ├── CPU 无 PCID? ──→ need_flush = 1   ──┐
    │                                       │
    ├── 全局 ASID (INVLPGB)? ──→ need_flush = 0   ──┤
    │                                               │
    └── 查找可用 ASID                               │
            ├── 找到匹配 ASID ──→ need_flush = 0 ──┼──→ switch_mm
            └── ASID 过期/需回收 ──→ need_flush = 1─┘

性能意义

• pages=-1（需要刷新）：性能开销大，TLB 缓存全部失效
• pages=0（无需刷新）：性能优化，TLB 缓存得以保留

在多任务系统中，理想的状况是大多数 TLB_FLUSH_ON_TASK_SWITCH 显示 pages=0，说明 PCID/AS ID 机制有效减少了不必要的 TLB 刷新。

## tlb flush 的经典使用

1. 修改 page table 的权限

```txt
@[
    kvm_flush_tlb_multi+5
    flush_tlb_mm_range+289
    ptep_clear_flush+65
    do_wp_page+3163
    handle_mm_fault+2263
    do_user_addr_fault+780
    exc_page_fault+117
    asm_exc_page_fault+38
    copy_fpstate_to_sigframe+130
    get_sigframe+442
    x64_setup_rt_frame+121
    arch_do_signal_or_restart+306
    irqentry_exit_to_user_mode+75
    asm_sysvec_reschedule_ipi+26
]: 5
```

```txt
@[
    kvm_flush_tlb_multi+5
    flush_tlb_mm_range+289
    ptep_clear_flush+65
    page_vma_mkclean_one+220
    page_mkclean_one+142
    rmap_walk_file+307
    folio_mkclean+186
    folio_clear_dirty_for_io+93
    mpage_prepare_extent_to_map+704
    ext4_do_writepages+865
    ext4_normal_submit_inode_data_buffers+238
    jbd2_journal_commit_transaction+1242
    kjournald2+176
    kthread+248
    ret_from_fork+55
    ret_from_fork_asm+26
]: 8
```

2. 修改 page table 的结构
```txt
@[
    kvm_flush_tlb_multi+5
    flush_tlb_mm_range+289
    pmdp_invalidate+153
    set_pmd_migration_entry+100
    try_to_migrate_one+526
    rmap_walk_anon+319
    try_to_migrate+214
    migrate_pages_batch+946
    migrate_pages+1758
    migrate_misplaced_folio+200
    do_huge_pmd_numa_page+567
    handle_mm_fault+1652
    __get_user_pages+1899
    get_user_pages_unlocked+264
    hva_to_pfn+264
    kvm_faultin_pfn+481
    kvm_tdp_page_fault+180
    kvm_mmu_do_page_fault+372
    kvm_mmu_page_fault+205
    vmx_handle_exit+1395
    kvm_arch_vcpu_ioctl_run+6726
    kvm_vcpu_ioctl+1562
    __se_sys_ioctl+107
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 29
```

## tlb 相关的手册
从 SDM 看起来: 25.6.12 Virtual-Processor Identifier (VPID)

## x86 使用什么指令来实现 tlb flush
<!-- 25f70ea0-9e58-4e3e-8109-09ff834a89e8 -->

| 指令      | 平台      | 标识符                                | 作用层次                               |
| --------- | --------- | --------                              | -----------                            |
| `invlpg`  | Intel/AMD |                                       | 当前地址空间                           |
| `invlpga` | AMD       | ASID                                  | 可以选择其他地址空间，实际仅仅用于 svm |
| `invvpid` | Intel     | **VPID**                              | Guest                                  |
| `invpcid` | Intel     | **PCID** (Process-Context Identifier) | Host                                   |

- [ ] 有点怀疑，其实 invpcid 是 amd 也可以使用的，可以找一下具体的代码了
- [ ] 似乎 invlpga 是具有通用的能力的，那么 invlpga 不能取代 invpcid ，在 amd 中

- `invlpg [addr]`
	- 使 当前地址空间 中 虚拟地址 addr 对应的 TLB 项失效，不区分 PCID / ASID（除非特殊模式）
- `invlpga rAX, rCX`
	- rAX：虚拟地址
	- rCX：ASID（Address Space ID）
	- 只失效 指定 ASID + 指定 VA 的 TLB 项
	- 此外，这个仅仅限于 amd ，主要用于在不切换 cr3 的状态下，精准打掉 Guest 的 TLB 项

## TLB 刷新的经典流程
<!-- 89a212a1-7c6f-42d8-aca6-26e9a41f9e95 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

通过 `mm_cpumask` 跟踪哪些 CPU 正在使用某个 mm，避免不必要的 IPI，这是 Linux TLB 刷性能的关键。

```txt
TLB 刷新请求
    │
    ├─► 需要广播到多 CPU？
    │   │
    │   ├─► 是 → 检查 X86_FEATURE_INVLPGB
    │   │       ├─► 支持 → 使用 INVLPGB + TLBSYNC
    │   │       └─► 不支持 → 发送 IPI，各 CPU 自行刷新
    │   │
    │   └─► 否（本地刷新）
    │       │
    │       ├─► 单页刷新？
    │       │   ├─► 是 → invlpg(addr)
    │       │   │       （如有 PCID + PTI，再用 invpcid_flush_one）
    │       │   │
    │       │   └─► 否（全刷新）
    │       │       ├─► 有 X86_FEATURE_INVPCID？
    │       │       │   ├─► 支持 → invpcid_flush_all() 或 invpcid_flush_all_nonglobals()
    │       │       │   └─► 不支持 → 翻转 CR4.PGE 或写 CR3
```

(感觉，这个是非常有道理的)

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
