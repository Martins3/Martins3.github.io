## 基本内容

### x86
```txt
├── 475920-25
│   ├── blocking
│   ├── directed_yield_attempted
│   ├── directed_yield_successful
│   ├── exits
│   ├── fpu_reload
│   ├── halt_attempted_poll
│   ├── halt_exits
│   ├── halt_poll_fail_hist
│   ├── halt_poll_fail_ns
│   ├── halt_poll_invalid
│   ├── halt_poll_success_hist
│   ├── halt_poll_success_ns
│   ├── halt_successful_poll
│   ├── halt_wait_hist
│   ├── halt_wait_ns
│   ├── halt_wakeup
│   ├── guest_mode <- 是否在 guest mode
│   ├── host_state_reload <- 主机状态重新加载次数
│   ├── nested_run <- 嵌套运行多少次
│   ├── pf_guest <- 向 L1 注入的 #PF 次数
│   ├── hypercalls
│   ├── insn_emulation
│   ├── insn_emulation_fail
│   ├── io_exits
│   ├── irq_exits
│   ├── irq_injections
│   ├── irq_window_exits
│   ├── l1d_flush
│   ├── max_mmu_page_hash_collisions
│   ├── max_mmu_rmap_size
│   ├── mmio_exits
│   ├── mmu_cache_miss
│   ├── mmu_flooded
│   ├── mmu_pde_zapped
│   ├── mmu_pte_write
│   ├── mmu_recycled
│   ├── mmu_rmaps_stat
│   ├── mmu_shadow_zapped
│   ├── mmu_unsync
│   ├── nmi_injections
│   ├── nmi_window_exits
│   ├── notify_window_exits
│   ├── nx_lpage_splits
│   ├── pages_1g
│   ├── pages_2m
│   ├── pages_4k
│   ├── pf_emulate
│   ├── pf_fast
│   ├── pf_fixed
│   ├── pf_mmio_spte_created
│   ├── pf_spurious
│   ├── pf_taken
│   ├── preemption_other
│   ├── preemption_reported
│   ├── remote_tlb_flush
│   ├── remote_tlb_flush_requests
│   ├── tlb_flush
│   ├── req_event
│   ├── request_irq_exits
│   ├── signal_exits
│   ├── invlpg
│   ├── vcpu0
│   │   ├── guest_mode
│   │   ├── lapic_timer_advance_ns
│   │   ├── pid
│   │   ├── tsc-offset
│   │   ├── tsc-scaling-ratio
│   │   └── tsc-scaling-ratio-frac-bits
│   ├── vcpu1
│   │   ├── guest_mode
│   │   ├── lapic_timer_advance_ns
│   │   ├── pid
│   │   ├── tsc-offset
│   │   ├── tsc-scaling-ratio
│   │   └── tsc-scaling-ratio-frac-bits
│   ├── vcpu2
│   │   ├── guest_mode
│   │   ├── lapic_timer_advance_ns
│   │   ├── pid
│   │   ├── tsc-offset
│   │   ├── tsc-scaling-ratio
│   │   └── tsc-scaling-ratio-frac-bits
│   └── vcpu3
│       ├── guest_mode
│       ├── lapic_timer_advance_ns
│       ├── pid
│       ├── tsc-offset
│       ├── tsc-scaling-ratio
│       └── tsc-scaling-ratio-frac-bits
├── blocking
├── directed_yield_attempted
├── directed_yield_successful
├── exits
├── fpu_reload
├── guest_mode
├── halt_attempted_poll
├── halt_exits
├── halt_poll_fail_hist
├── halt_poll_fail_ns
├── halt_poll_invalid
├── halt_poll_success_hist
├── halt_poll_success_ns
├── halt_successful_poll
├── halt_wait_hist
├── halt_wait_ns
├── halt_wakeup
├── host_state_reload
├── hypercalls
├── insn_emulation
├── insn_emulation_fail
├── invlpg
├── io_exits
├── irq_exits
├── irq_injections
├── irq_window_exits
├── l1d_flush
├── max_mmu_page_hash_collisions
├── max_mmu_rmap_size
├── mmio_exits
├── mmu_cache_miss
├── mmu_flooded
├── mmu_pde_zapped
├── mmu_pte_write
├── mmu_recycled
├── mmu_shadow_zapped
├── mmu_unsync
├── nested_run
├── nmi_injections
├── nmi_window_exits
├── notify_window_exits
├── nx_lpage_splits
├── pages_1g
├── pages_2m
├── pages_4k
├── pf_emulate
├── pf_fast
├── pf_fixed
├── pf_guest
├── pf_mmio_spte_created
├── pf_spurious
├── pf_taken
├── preemption_other
├── preemption_reported
├── remote_tlb_flush
├── remote_tlb_flush_requests
├── req_event
├── request_irq_exits
├── signal_exits
└── tlb_flush
```


### aarch64
```txt
├── 1485961-25
│   ├── blocking
│   ├── exits
│   ├── halt_attempted_poll
│   ├── halt_poll_fail_hist
│   ├── halt_poll_fail_ns
│   ├── halt_poll_invalid
│   ├── halt_poll_success_hist
│   ├── halt_poll_success_ns
│   ├── halt_successful_poll
│   ├── halt_wait_hist
│   ├── halt_wait_ns
│   ├── halt_wakeup
│   ├── hvc_exit_stat
│   ├── mmio_exit_kernel
│   ├── mmio_exit_user
│   ├── remote_tlb_flush
│   ├── remote_tlb_flush_requests
│   ├── signal_exits
│   ├── vgic-state
│   ├── wfe_exit_stat
│   └── wfi_exit_stat
├── blocking
├── exits
├── halt_attempted_poll
├── halt_poll_fail_hist
├── halt_poll_fail_ns
├── halt_poll_invalid
├── halt_poll_success_hist
├── halt_poll_success_ns
├── halt_successful_poll
├── halt_wait_hist
├── halt_wait_ns
├── halt_wakeup
├── hvc_exit_stat
├── mmio_exit_kernel
├── mmio_exit_user
├── remote_tlb_flush
├── remote_tlb_flush_requests
├── signal_exits
├── wfe_exit_stat
```


## 基本代码结构

结构体:
```c
const struct _kvm_stats_desc kvm_vcpu_stats_desc[] = { };
const struct _kvm_stats_desc kvm_vm_stats_desc[] = { };
```


整个虚拟机的内容在 virt/kvm/kvm_main.c kvm_init_debug 和 kvm_create_vm_debugfs


如果是主机层面的:
```c
//
static int vcpu_stat_get(void *_offset, u64 *val){}
// struct kvm_vm_stat
static int vm_stat_get(void *_offset, u64 *val){}
```

如果是一个虚拟机层面，通过这个函数即可:

kvm_stat_data_get

vCPU 相关的初始化在 arch/x86/kvm/debugfs.c 中:

```txt
── vcpu3
   ├── guest_mode
   ├── lapic_timer_advance_ns
   ├── pid
   ├── tsc-offset
   ├── tsc-scaling-ratio
   └── tsc-scaling-ratio-frac-bits
```

## kvm 目录名称含义 : 786263-25

```txt
[root@nixos:/sys/kernel/debug/kvm/786263-25]#
```

kvm_create_vm_debugfs 中
```c
snprintf(dir_name, sizeof(dir_name), "%d-%s", task_pid_nr(current), fdname);
```

使用 fd 来区分，我意识到，其实一个 qemu ，想要的话，可以同时管理两个虚拟机。

(可以用 kvm 的 selftest 来创建一下)

## kvm debugfs 和 mmu 相关的统计
<!-- 93b9c4fb-489f-47d7-a8a2-e6b6490b54e3 -->

| 字段                     | 值      | 含义                                     | 说明                                                                                                                                                                        |
|--------------------------|---------|------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **pf_taken**             | 1033470 | 总共处理的页故障（Page Fault）次数       | L2 产生的 EPT violation 也会计入此项。嵌套时，L0 需要处理 L1 的 shadow page fault 和 L2 的 EPT violation，数值会显著增加                                                    |
| **pf_fixed**             | 1033329 | 成功修复的页故障数（建立映射后重新执行） | 嵌套时，L0 需要为 L1 维护 shadow page table，为 L2 维护 nested EPT，修复次数会增加                                                                                          |
| **pf_emulate**           | 86      | 需要模拟（emulate）的页故障数            | 通常由 MMIO 或特殊访问导致。嵌套时，L1 可能配置 EPT 权限使得某些访问需要 L0 模拟                                                                                            |
| **pf_fast**              | 0       | 快速路径处理的页故障数（无锁直接修复）   | 嵌套虚拟化通常走慢路径（需处理多层页表），此值为 0 符合预期                                                                                                                 |
| **pf_spurious**          | 53      | 虚假页故障数（发现时错误已不存在）       | 通常由竞争条件导致，与嵌套关系不大                                                                                                                                          |
| **pf_guest**             | 0       | **由客户机注入页故障的次数**             | **关键指标！** 专门用于统计向 L1 注入的 #PF 次数（L2 产生的 EPT violation 经 L0 处理后可能注入给 L1）。**当前为 0 说明可能没有运行 L2，或 L2 没有触发需要 L1 处理的页故障** |
| **pf_mmio_spte_created** | 86      | 为 MMIO 创建特殊页表项（SPTE）的次数     | MMIO 访问在嵌套场景下可能更复杂，但此值与 pf_emulate 接近，说明主要是 MMIO 导致的模拟                                                                                       |
| **tlb_flush**            | 38863   | TLB 刷新次数                             | 嵌套时，L1 执行 INVEPT/INVVPID 或 L0 维护嵌套页表都会导致额外 TLB 刷新                                                                                                      |
| **invlpg**               | 0       | 当捕获到 Guest 执行类 invlpg 指令的次数  |


| 字段                             | 值    | 含义                                             | 说明                                                                                                   |
|----------------------------------|-------|--------------------------------------------------|--------------------------------------------------------------------------------------------------------|
| **mmu_cache_miss**               | 0     | MMU 页缓存未命中次数                             | 分配新的 shadow page 时增加。嵌套时 shadow page 需求可能增加，但当前为 0 说明没有大量新分配            |
| **mmu_flooded**                  | 0     | MMU "洪水" 次数（页表被频繁写保护后重新 zapped） | 客户机频繁写页表时触发。嵌套时 L1 作为 hypervisor 会频繁修改页表，此值通常会增加                       |
| **mmu_pde_zapped**               | 0     | 被清除的大页目录项（PDE）数                      | 客户机修改页表时触发。嵌套时 L1 修改其页表会导致此值增加                                               |
| **mmu_pte_write**                | 0     | 捕获 Guest 写页表次数                            | shadow paging 的核心机制。嵌套时使用 TDP/NPT（EPT）通常不依赖此机制，值为 0 说明启用了硬件辅助嵌套分页 |
| **mmu_recycled**                 | 0     | 回收的 MMU 页数                                  | 内存压力时回收 shadow pages。嵌套时 shadow page 总量可能更大                                           |
| **mmu_shadow_zapped**            | 0     | 被清除（zapped）的影子页表页数                   | 页表失效时清除 shadow mapping。嵌套时由于多层页表，失效更频繁                                          |
| **mmu_unsync**                   | 0     | 当前未同步的 shadow 页数                         | 标记需要同步的 shadow pages。嵌套场景下会更频繁                                                        |
| **max_mmu_page_hash_collisions** | 0     | MMU 页哈希表最大冲突数                           | 内部数据结构统计，与功能无关                                                                           |
| **max_mmu_rmap_size**            | 0     | 反向映射（rmap）最大大小                         | 内部数据结构统计，与功能无关                                                                           |
| **mmio_exits**                   | 32706 | MMIO 导致的 VM exit 次数                         | 嵌套时，L2 的 MMIO 先 exit 到 L1，L1 可能再 exit 到 L0（如果 L1 不处理），数值可能增加                 |

如果 modprobe kvm_intel ept=0 的时候才可以观察到:

1. 直接执行 INVLPG 指令
	- VMX (Intel): 当 VM exit 原因为 EXIT_REASON_INVLPG 时，handle_invlpg() 函函调用 kvm_mmu_invlpg()
	- SVM (AMD): 当 VM exit 原因为 SVM_EXIT_INVLPG 时，invlpg_interception() 函数调用 kvm_mmu_invlpg()
2. 模拟执行 INVLPG
	- 当不支持 decode assists 或需要模拟时，通过指令模拟器 em_invlpg() 执行
3. INVPCID 指令
	- 执行 kvm_mmu_invpcid_gva() 时也会递增该计数器
4. Hyper-V 远程 TLB 刷新
	- 处理 Hyper-V 的 TLB flush hypercall 时

每次调用 kvm_mmu_invlpg() 或 kvm_mmu_invpcid_gva() 时，计数器会递增

(这里我记录一个问题，为什么 ept=1 的时候，打开嵌套的时候，L0 和 L1 中都是无法观察到 invlpg ，
按道理嵌套环境中，将两级 ept 压缩，需要类似 shadow page table 的机制，但是这里实际上没有，
也就是其实 ept 存在类似的检测机制，但是其实也是有区别的)

```txt
  cat /sys/kernel/debug/kvm/mmu_flooded
  cat /sys/kernel/debug/kvm/mmu_unsync
  cat /sys/kernel/debug/kvm/mmu_pte_write
```



### kvm tlb flush 相关的指标
<!-- e46f710d-4389-44f5-9e16-0d2a6ad832c0 -->

(应该将 tlb pv flush 也放过来才对的)

三个指标分别用于统计不同层级的 TLB flush 操作：

| 指标 | 类型 | 定义位置 | 说明 |
|------|------|----------|------|
| `remote_tlb_flush_requests` | VM 级 | `struct kvm_vm_stat_generic` | 远程 TLB flush 请求次数 |
| `remote_tlb_flush` | VM 级 | `struct kvm_vm_stat_generic` | 远程 TLB flush 实际执行次数（广播到所有 vCPU） |
| `tlb_flush` | vCPU 级 | `struct kvm_vcpu_stat` | 单个 vCPU 本地 TLB flush 次数 |

这两个指标在 `kvm_flush_remote_tlbs()` 中统计：

```c
void kvm_flush_remote_tlbs(struct kvm *kvm)
{
    ++kvm->stat.generic.remote_tlb_flush_requests;  // ① 每次请求都计数

    /*
     * 内存屏障确保页表修改在读取 vcpu->mode 前可见
     * - x86: smp_mb__after_srcu_read_unlock in vcpu_enter_guest
     * - powerpc: smp_mb in kvmppc_prepare_to_enter
     */
    if (!kvm_arch_flush_remote_tlbs(kvm)           // ② 尝试架构特定优化
        || kvm_make_all_cpus_request(kvm, KVM_REQ_TLB_FLUSH))  // ③ 广播请求
        ++kvm->stat.generic.remote_tlb_flush;       // ④ 失败或需广播时计数
}
```

tlb_flush (vCPU 本地) 的实现

`tlb_flush` 统计在以下三个函数中（x86 架构）：

```c
static void kvm_vcpu_flush_tlb_all(struct kvm_vcpu *vcpu)
{
    ++vcpu->stat.tlb_flush;
    kvm_x86_call(flush_tlb_all)(vcpu);
    kvm_clear_request(KVM_REQ_TLB_FLUSH_CURRENT, vcpu);
}

static void kvm_vcpu_flush_tlb_guest(struct kvm_vcpu *vcpu)
{
    ++vcpu->stat.tlb_flush;

    if (!tdp_enabled) {
        // 影子页表需要强制同步根页表
        kvm_mmu_sync_roots(vcpu);
        kvm_mmu_sync_prev_roots(vcpu);
    }
    kvm_make_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu);
}

static inline void kvm_vcpu_flush_tlb_current(struct kvm_vcpu *vcpu)
{
    ++vcpu->stat.tlb_flush;
    kvm_x86_call(flush_tlb_current)(vcpu);
}
```

## debugfs 中 directed_yield_attempted 和 directed_yield_successful
<!-- 7a73d6eb-d660-4f71-8813-934abb700a82 -->

**场景 1: Call Function IPI**

当 Guest 需要向其他 CPU 广播函数调用（如 TLB shootdown）时：

```c
static void kvm_smp_send_call_func_ipi(const struct cpumask *mask)
{
    native_send_call_func_ipi(mask);  // 先发送 IPI

    /* 检查目标 vCPU 是否被抢占，如果是则主动 yield */
    for_each_cpu(cpu, mask) {
        if (!idle_cpu(cpu) && vcpu_is_preempted(cpu)) {
            kvm_hypercall1(KVM_HC_SCHED_YIELD, per_cpu(x86_cpu_to_apicid, cpu));
            break;
        }
    }
}
```

**场景 2: PV Spinlock Kick**

当唤醒一个 halted 的 vCPU 时：

```c
static void kvm_kick_cpu(int cpu)
{
    u32 apicid = per_cpu(x86_cpu_to_apicid, cpu);
    kvm_hypercall2(KVM_HC_KICK_CPU, flags, apicid);
}
```

vCPU 抢占检测 : Guest 通过 steal time 结构体判断 vCPU 是否被抢占，
也就是在虚拟机中，也是可以看到当前的 vCPU 是否运行了

```c
__visible bool __kvm_vcpu_is_preempted(long cpu)
{
    struct kvm_steal_time *src = &per_cpu(steal_time, cpu);
    return !!(src->preempted & KVM_VCPU_PREEMPTED);
}
```

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              Guest Kernel                               │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────────────────┐  │
│  │ send IPI     │───▶│ check if     │───▶│ kvm_hypercall1()         │  │
│  │ (e.g., TLB   │    │ target vCPU  │    │ KVM_HC_SCHED_YIELD       │  │
│  │  shootdown)  │    │ preempted?   │    │ arg = target APIC ID     │  │
│  └──────────────┘    └──────────────┘    └──────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                               Host KVM                                  │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ handle_hypercall()                                               │   │
│  │   case KVM_HC_SCHED_YIELD:                                       │   │
│  │     vcpu->stat.directed_yield_attempted++                        │   │
│  │     target = find_vcpu_by_apic_id(arg)                           │   │
│  │     if (kvm_vcpu_yield_to(target) > 0)                           │   │
│  │         vcpu->stat.directed_yield_successful++                   │   │
│  └─────────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────────┘
```

1. 设计目的
**解决的问题**：当 Guest 发送 IPI 给多个 vCPU 时，如果某些目标 vCPU 被 Host 调度出去，它们无法立即响应 IPI。发送方 vCPU 继续运行会浪费 CPU 时间等待响应。
**解决方案**：发送 IPI 后检查目标 vCPU 是否被抢占，如果是则主动 yield CPU，让 Host 有机会调度被抢占的目标 vCPU 运行。
**适用场景**：
- 高锁竞争场景（如大量 TLB shootdown）
- 减少 vCPU 间的等待延迟
- 提升多 vCPU 协作效率

2. Hypercall 汇总
| Hypercall | 条件 | 说明 |
|-----------|------|------|
| `KVM_HC_KICK_CPU` | 客户机有 `KVM_FEATURE_PV_UNHALT` 特性 | 客户机唤醒另一个 vCPU 时，先尝试 yield 给它 |
| `KVM_HC_SCHED_YIELD` | 客户机有 `KVM_FEATURE_PV_SCHED_YIELD` 特性 | 客户机主动请求让出 CPU 给指定目标 |

3. 性能分析
- **`directed_yield_attempted` 远大于 `directed_yield_successful`**：说明客户机的 yield 请求经常失败，可能存在不必要的 yield 调用
- **成功率高**：说明该机制正在有效工作，帮助被抢占的 vCPU 更快获得运行机会
- **指标为 0**：说明客户机未启用 PV sched yield 特性，或没有触发相关场景

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
