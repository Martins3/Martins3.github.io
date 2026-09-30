# async pf

当前协议状态见 [机制状态核查](../mechanism-evolution.md)：APF 机制仍在，
page-not-present 仍可使用 synthetic `#PF`；旧的 page-ready `#PF` 通知已被
`2635b5c4a0e4` 替换为 LAPIC 中断，当前没有旧 ready 通知方式的回退分支。
因此旧文章里的两类 APF 都使用 `#PF` 的描述不能直接套用当前源码。

## 关键设计

因为 vCPU 发生 page fault 了，被切换走，然后 Host 从磁盘中请求内容，然后 host
就很尴尬:

- 不能继续执行 vCPU 线程了，因为如果开始执行，就需要保证该页已经准备好。
- host 侧没有什么任务需要执行。

基本的设计:

- host 需要告诉 Guest ，正在进行 apf ，可以干其他事情。
- Guest 的 thread 中执行 page fault 的 thread 暂停

## 参考资料

MSR 位定义及 page-not-present / page-ready 协议见
[Linux KVM MSR 文档](https://www.kernel.org/doc/html/latest/virt/kvm/x86/msr.html)。
- https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/03/24/kvm-async-page-fault
- https://lwn.net/Articles/817239/
- [https://lwn.net/Articles/845473/](aarch64: Support Asynchronous Page Fault)
  - 这个 patch 描述的比较清楚了

## async pf 的几个基本问题
<!-- 94f5a5a0-07af-4d2a-a9e6-5db6db14d99a -->

### arm 支持吗
不支持

### 最多支持多少个 guest thread 的 page fault 被 stall

答案一个 CPU 是 64 个

```c
#define ASYNC_PF_PER_VCPU 64
```

```c
bool kvm_setup_async_pf(struct kvm_vcpu *vcpu, ...)
 {
     if (vcpu->async_pf.queued >= ASYNC_PF_PER_VCPU)
         return false;  // 超过限制，转为同步处理
     ...
 }
```

### 需要 Guest 和 Host 的参与
1. 为什么需要 Guest 感知到
	- guest 需要在触发 async page fault 的时候，将当前执行的 thread 挂起，
1. 为什么 host 需要 worker 来参与
	- 总是需要一个等待者，要么当前的 vCPU thread ，显然 vCPU thread 需要做其他的事情，所以就让 worker 来等待

## 实现细节

### 触发

```text
EPT/NPT violation（不一定意味着 backing page 不驻留）
  → kvm_mmu_page_fault()
  → kvm_mmu_do_page_fault()
  → kvm_tdp_page_fault() → kvm_tdp_mmu_page_fault() [启用 TDP MMU 时]
  → kvm_mmu_faultin_pfn() → __kvm_mmu_faultin_pfn()
      → __kvm_faultin_pfn(..., FOLL_NOWAIT, ...)
      → kvm_follow_pfn() → hva_to_pfn()
          ├─ 有效 PFN：外层继续 kvm_tdp_mmu_map()
          ├─ 其他错误编码：外层错误处理
          └─ KVM_PFN_ERR_NEEDS_IO
              ├─ prefetch / !kvm_can_do_async_pf()：同步取页
              ├─ kvm_find_async_pf_gfn() 找到已有任务
              │    → KVM_REQ_APF_HALT，返回 RET_PF_RETRY
              └─ kvm_arch_setup_async_pf() → kvm_setup_async_pf()
                   ├─ 失败：同步取页
                   └─ 成功建立任务
                        → kvm_arch_async_page_not_present()
                            ├─ 可以通知：注入 synthetic #PF
                            └─ 不能通知：KVM_REQ_APF_HALT
                        → schedule_work(async_pf_execute)
                        → 返回 RET_PF_RETRY

同步取页：__kvm_faultin_pfn(..., 清除 FOLL_NOWAIT，添加 FOLL_INTERRUPTIBLE, ...)
```

非常清晰，注入中断就是 #PF
```c
void kvm_inject_page_fault(struct kvm_vcpu *vcpu, struct x86_exception *fault,
			   bool from_hardware)
{
	++vcpu->stat.pf_guest;

	/*
	 * Async #PF in L2 is always forwarded to L1 as a VM-Exit regardless of
	 * whether or not L1 wants to intercept "regular" #PF.
	 */
	if (is_guest_mode(vcpu) && fault->async_page_fault)
		kvm_queue_exception_vmexit(vcpu, PF_VECTOR,
					   true, fault->error_code,
					   true, fault->address);
	else
		kvm_queue_exception_e_p(vcpu, PF_VECTOR, fault->error_code,
					fault->address);
}
```

KVM_PFN_ERR_NEEDS_IO 的原因，一定经典案例如下，其实就是 GUP 的时候没有
```text
get_user_pages_unlocked()
  → ... → __get_user_pages()
      → follow_page_mask()：当前还不能取得页面
      → faultin_page()
          → handle_mm_fault()
              → ... → do_swap_page()
                  → folio_lock_or_retry()
                      → folio_trylock() 失败
                      → __folio_lock_or_retry()
                          → 看到 RETRY_NOWAIT，返回 VM_FAULT_RETRY
          → 把 VM_FAULT_RETRY 转为 -EBUSY
      → 本次请求 1 页，却没有获得任何页，返回 0
  → hva_to_pfn_slow() 返回 0
  → hva_to_pfn()：普通 VMA 有效 + FOLL_NOWAIT
      → KVM_PFN_ERR_NEEDS_IO
```

`__kvm_mmu_faultin_pfn()` 三种情况来处理 io

- *同步重试* : prefetch、不满足 `kvm_can_do_async_pf()` 或建立任务失败时，
  清除 `FOLL_NOWAIT`、添加 `FOLL_INTERRUPTIBLE`，再次调用 `__kvm_faultin_pfn()`。
  需要等待时由当前 host vCPU 线程等；不是每次都必然睡眠，也不保证重试必然成功。
- *复用已有任务* : `kvm_find_async_pf_gfn()` 发现本 vCPU 已有这个 GFN 的任务，
  不重复创建 work，而是请求 `KVM_REQ_APF_HALT`，返回 `RET_PF_RETRY`。
- *创建新任务* : `kvm_arch_setup_async_pf()` 保存 token、GFN、原始 fault 信息，
  再调用 `virt/kvm/async_pf.c` 的 `kvm_setup_async_pf()`。
  后者检查队列限额、HVA、`GFP_NOWAIT` 分配等，失败则回到同步重试。

`RET_PF_RETRY` 是 KVM MMU 内部的处理结果，与 NEEDS_IO 不同:
它表示当前这次 fault 暂时没有建立最终映射，后续需要重试/重新处理；不会作为 errno 直接发给 guest。

### Worker 代理执行

本次 x86 配置走非 `CONFIG_KVM_ASYNC_PF_SYNC` 分支，实际时序是：

```text
host kworker：async_pf_execute()
  → get_user_pages_remote(kvm->mm, hva, 1, FOLL_WRITE, ...)
      [不带 FOLL_NOWAIT，允许等待]
  → work 加入 vcpu->async_pf.done
  → kvm_arch_async_page_present_queued()
      → KVM_REQ_APF_READY，必要时 kick vCPU
  → __kvm_vcpu_wake_up()

host vCPU：kvm_check_async_pf_completion()
  → kvm_arch_async_page_ready()
      → kvm_mmu_do_page_fault(..., prefetch=true, ...)
        [上下文仍匹配时，尝试预先建立映射]
  → kvm_arch_async_page_present()
      ├─ 先前确实注入过 not-present，且 guest APF 仍启用
      │   → apf_put_user_ready() 写 token
      │   → kvm_apic_set_irq() 投递 page-ready 中断
      └─ 清除 apf.halted，恢复 RUNNABLE

guest：sysvec_kvm_asyncpf_interrupt()
  → kvm_async_pf_task_wake(token)
  → 清除共享 token，写 MSR_KVM_ASYNC_PF_ACK
  → 原 task 可以再次获得调度并重试原指令
```

`kvm_arch_async_page_ready()`（MMU 预取）和 `kvm_arch_async_page_present()`
（guest 通知/解除 halt）是两个不同函数，不能因名字接近而混用。
Worker 不向 vCPU 直接移交一个最终 PFN；vCPU 的 MMU 路径会再次检查映射。
而且 worker 取页失败也会通知完成，避免 vCPU 永远等待，后续 fault 再处理错误。
因此“completed/ready”也不能当作“这次底层 GUP 一定成功”的绝对证明。

```c
void kvm_arch_async_page_present(struct kvm_vcpu *vcpu,
				 struct kvm_async_pf *work)
{
	struct kvm_lapic_irq irq = {
		.delivery_mode = APIC_DM_FIXED,
		// 这个就是 guest os 通过 wrmsrq(MSR_KVM_ASYNC_PF_INT, HYPERVISOR_CALLBACK_VECTOR); 来配置的
		.vector = vcpu->arch.apf.vec
	};

	if (work->wakeup_all)
		work->arch.token = ~0; /* broadcast wakeup */
	else
		kvm_del_async_pf_gfn(vcpu, work->arch.gfn);
	trace_kvm_async_pf_ready(work->arch.token, work->cr2_or_gpa);

	if ((work->wakeup_all || work->notpresent_injected) &&
	    kvm_pv_async_pf_enabled(vcpu) &&
	    !apf_put_user_ready(vcpu, work->arch.token)) {
		WRITE_ONCE(vcpu->arch.apf.pageready_pending, true);
		kvm_apic_set_irq(vcpu, &irq, NULL);
	}

	vcpu->arch.apf.halted = false;
	kvm_set_mp_state(vcpu, KVM_MP_STATE_RUNNABLE);
}
```

## 的角度


本来的 kvm 日志为:

arch/x86/include/asm/trapnr.h
```c
#define X86_TRAP_PF		14	/* Page Fault */
```

- KVM_FEATURE_ASYNC_PF（CPUID 0x40000001.EAX bit 4）：异步缺页（APF）的基础能力。
- KVM_FEATURE_ASYNC_PF_INT（bit 14，5.8 引入）：新增中断方式投递 page ready 事件，配套两个新 MSR：
	- MSR_KVM_ASYNC_PF_INT（配置 APIC  vector）
	- MSR_KVM_ASYNC_PF_ACK（确认 + 重新扫描队列

现在我们仅仅考虑 KVM_FEATURE_ASYNC_PF_INT

### 触发 page fault

在 arch/x86/mm/fault.c 中标准 page fault 的入口:

```c
DEFINE_IDTENTRY_RAW_ERRORCODE(exc_page_fault)
```

- kvm_handle_async_pf
  - `__kvm_handle_async_pf`
    - kvm_async_pf_task_wait_schedule : 被 swapout 的 page，所以睡眠

- asm_sysvec_kvm_asyncpf_interrupt 就是 guest 接受到 host 的信息说可以了。


注意，这个还是复用了 #PF 的入口。

### 接受消息

host 处理完成之后，挂到:
```c
/* Vector on which hypervisor callbacks will be delivered */
#define HYPERVISOR_CALLBACK_VECTOR	0xf3

DEFINE_IDTENTRY_SYSVEC(sysvec_kvm_asyncpf_interrupt)
```

- sysvec_kvm_asyncpf_interrupt
  - kvm_async_pf_task_wake
  - wrmsrl(MSR_KVM_ASYNC_PF_ACK, 1);

```txt
$ cat /proc/interrupts
 HYP:          1          1          1  Hypervisor callback interrupts
```

## 嵌套的 async pf 存在 bug

2026-09-15 不确定当前是否还存在

```txt
bogon login: [    8.974820] mount.nfs (3059) used greatest stack depth: 10416 bytes left
[  340.176347] Kernel panic - not syncing: Host injected async #PF in kernel mode
[  340.176843] CPU: 25 UID: 1000 PID: 6896 Comm: zsh Tainted: G           O       6.11.0 #136
[  340.177374] Tainted: [O]=OOT_MODULE
[  340.177614] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[  340.178059] Call Trace:
[  340.178244]  <TASK>
[  340.178406]  dump_stack_lvl+0x86/0xc0
[  340.178657]  panic+0x12f/0x310
[  340.178868]  __kvm_handle_async_pf+0xa3/0xb0
[  340.179145]  exc_page_fault+0x10c/0x1f0
[  340.179416]  asm_exc_page_fault+0x26/0x30
[  340.179678] RIP: 0010:__put_user_4+0x11/0x20
[  340.179957] Code: 1f 84 00 00 00 00 00 66 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 f3 0f 1e fa 48 89 cb 48 c1 fb 3f 48 09 d9 0f 01 cb <89> 01 31 c9 0f 01 ca c3
 cc cc cc cc 0f 1f 00 90 90 90 90 90 90 90
[  340.181079] RSP: 0018:ffffc900185d3f18 EFLAGS: 00050202
[  340.181430] RAX: 0000000000001af0 RBX: 0000000000000000 RCX: 00007fc378019510
[  340.181877] RDX: 0000000000000000 RSI: 0000000000000000 RDI: ffff8881047e2400
[  340.182356] RBP: 0000000000000000 R08: ffff8881047e2400 R09: ffffffff84076c00
[  340.182818] R10: 0000000000000001 R11: 0000004f341718de R12: 0000000000000000
[  340.183296] R13: 0000000000000000 R14: ffffffff81113ddc R15: 0000000000000000
[  340.183763]  ? ret_from_fork+0x1c/0x50
[  340.184033]  schedule_tail+0x8a/0xa0
[  340.184309]  ret_from_fork+0x1c/0x50
[  340.184568]  ret_from_fork_asm+0x1a/0x30
[  340.184848]  </TASK>
[  340.185227] Kernel Offset: disabled
[  340.185493] ---[ end Kernel panic - not syncing: Host injected async #PF in kernel mode ]---
```

启动嵌套的时候有这个错误。

1. 思考下，为什么 host 中不可以使用 async pf 啊

所以，我猜测，是本来应该注入给 L2 的 async pf 注入到 L1 中了，导致 L1 crash 了。

并不是，L1 用文件，L2 用普通的 memory ，还是有问题

测试一些程序的时候，似乎启动嵌套，其中 l2 使用的是普通内存，也会有这个问题:
也就是相当于 L1 是文件，或者是 memfd 就会有问题

```txt
[  245.347107][ T2261] Kernel panic - not syncing: Host injected async #PF in kernel mode
[  245.426245][ T2261] CPU: 33 PID: 2261 Comm: dockerd Not tainted 6.6.0-28.0.0.34.oe2403.x86_64 #1
[  245.437923][ T2261] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.3-32-g9029a010ec41 04/01/2014
[  245.456334][ T2261] Call Trace:
[  245.462508][ T2261]  <TASK>
[  245.467151][ T2261]  dump_stack_lvl+0x32/0x50
[  245.476485][ T2261]  panic+0x304/0x340
[  245.482298][ T2261]  ? restore_regs_and_return_to_kernel+0x22/0x22
[  245.490174][ T2261]  __kvm_handle_async_pf+0x98/0xb0
[  245.496845][ T2261]  exc_page_fault+0x358/0x780
[  245.502580][ T2261]  ? __futex_unqueue+0x25/0x40
[  245.508663][ T2261]  ? futex_unqueue+0x38/0x60
[  245.514184][ T2261]  asm_exc_page_fault+0x22/0x30
[  245.520170][ T2261] RIP: 0010:__get_user_8+0xd/0x20
[  245.541414][ T2261] Code: ca c3 cc cc cc cc 0f 1f 80 00 00 00 00 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 48 89 c2 48 c1 fa 3f 48 09 d0 0f 01 cb <48> 8b 10 31 c0 0f 01 ca c3 cc cc cc cc 66 0f 1f 44 00 00 90 90 90
[  245.618930][ T2261] RSP: 0018:ffffc900024fbe50 EFLAGS: 00050206
[  245.692158][ T2261] RAX: 00007f479a69cfe8 RBX: ffff8880208d4f80 RCX: 000000000001a408
[  245.701688][ T2261] RDX: 0000000000000000 RSI: ffffc900024fbe80 RDI: ffff8880208d4f80
[  245.711109][ T2261] RBP: ffffc900024fbee8 R08: ffff88805a0b5488 R09: 0000000000000000
[  245.720641][ T2261] R10: 0000000000000000 R11: 000000000080d6fe R12: ffffc900024fbf58
[  245.729971][ T2261] R13: 000055f1299597a3 R14: ffff8880208d4f80 R15: 0000000000000000
[  245.739432][ T2261]  rseq_get_rseq_cs+0x1d/0x270
[  245.746024][ T2261]  rseq_ip_fixup+0x46/0x190
[  245.751546][ T2261]  ? do_futex+0x106/0x1b0
[  245.756816][ T2261]  ? __se_sys_futex+0x6d/0x1b0
[  245.762258][ T2261]  __rseq_handle_notify_resume+0x26/0x50
[  245.768925][ T2261]  syscall_exit_to_user_mode+0xa9/0x1e0
[  245.775506][ T2261]  do_syscall_64+0x62/0x100
[  245.780837][ T2261]  entry_SYSCALL_64_after_hwframe+0x78/0xe2
[  245.788095][ T2261] RIP: 0033:0x55f1299597a3
[  245.793544][ T2261] Code: 24 20 c3 cc cc cc cc 48 8b 7c 24 08 8b 74 24 10 8b 54 24 14 4c 8b 54 24 18 4c 8b 44 24 20 44 8b 4c 24 28 b8 ca 00 00 00 0f 05 <89> 44 24 30 c3 cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc cc
[  245.816822][ T2261] RSP: 002b:00007f479a69bc48 EFLAGS: 00000202 ORIG_RAX: 00000000000000ca
[  245.826639][ T2261] RAX: ffffffffffffff92 RBX: 0000000000000000 RCX: 000055f1299597a3
[  245.835797][ T2261] RDX: 0000000000000000 RSI: 0000000000000080 RDI: 000055f12d56c780
[  245.845079][ T2261] RBP: 00007f479a69bc90 R08: 0000000000000000 R09: 0000000000000000
[  245.853816][ T2261] R10: 00007f479a69bc80 R11: 0000000000000202 R12: 00007f479a69bc80
[  245.863244][ T2261] R13: 0000000000000016 R14: 000000c0000069c0 R15: 00007f4799e9c000
[  245.872866][ T2261]  </TASK>
[  245.881821][ T2261] Kernel Offset: disabled
[  245.886099][ T2261] ---[ end Kernel panic - not syncing: Host injected async #PF in kernel mode ]---
```


## 基本执行流程

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                        APF 完整工作流程                                       │
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   PHASE 1: 页错误检测与异步判定                                               │
│   ─────────────────────────────────────                                     │
│                                                                              │
│   Guest: 访问虚拟地址 VA → EPT Violation → VM Exit                           │
│                                                                              │
│   Host Handler: kvm_mmu_page_fault()                                         │
│        ↓                                                                     │
│   kvm_tdp_page_fault() / paging64_page_fault()                               │
│        ↓                                                                     │
│   kvm_faultin_pfn()                                                          │
│        ↓                                                                     │
│   ┌────────────────────────────────────────────────────────────────┐        │
│   │ kvm_mmu_faultin_pfn -> __kvm_mmu_faultin_pfn():                                               │        │
│   │   1. 调用 gfn_to_pfn_prot() 尝试获取页面                        │        │
│   │   2. 如果页面不在内存中（需要 I/O）：                           │        │
│   │      - 检查 kvm_find_async_pf_gfn() 是否已有相同请求            │        │
│   │      - 没有则调用 kvm_arch_setup_async_pf() 创建异步请求        │        │
│   └────────────────────────────────────────────────────────────────┘        │
│        ↓                                                                     │
│   kvm_setup_async_pf() ──────→ 创建 async_pf 结构                           │
│        ↓                                                                     │
│   INIT_WORK(&apf->work, async_pf_execute)                                    │
│   queue_work(system_unbound_wq, &apf->work)                                  │
│        ↓                                                                     │
│   kvm_arch_async_page_not_present() ──→ 向 Guest 注入 "Page Not Present"    │
│                                                                              │
│   ═══════════════════════════════════════════════════════════════════════   │
│                                                                              │
│   PHASE 2: Guest 侧处理                                                       │
│   ─────────────────────────────────────                                     │
│                                                                              │
│   Guest 收到特殊 #PF（带有 PV 标志）                                          │
│        ↓                                                                     │
│   exc_page_fault() → kvm_handle_async_pf()                                  │
│        ↓                                                                     │
│   __kvm_handle_async_pf():                                                   │
│       - 识别这是 Host 注入的异步页错误                                        │
│       - 记录 apf_id 到 per-cpu 变量                                           │
│       - 将当前 task 加入 async_pf 等待队列                                    │
│       - 调用 kvm_async_pf_task_wait_schedule()                                │
│        ↓                                                                     │
│   schedule() ──────→ 当前线程睡眠，让出 CPU                                   │
│                                                                              │
│   ═══════════════════════════════════════════════════════════════════════   │
│                                                                              │
│   PHASE 3: 异步页面加载                                                       │
│   ─────────────────────────────────────                                     │
│                                                                              │
│   Worker Thread (async_pf_execute):                                          │
│        ↓                                                                     │
│   1. 调用 gup 获取页面（可能睡眠等待 I/O）                                    │
│   2. 页面就绪后，调用 kvm_arch_async_page_present()                           │
│        ↓                                                                     │
│   ┌────────────────────────────────────────────────────────────────┐        │
│   │ 两种通知方式（取决于 KVM_FEATURE_ASYNC_PF_INT）：                │        │
│   │                                                                │        │
│   │  Legacy (异常方式):                                            │        │
│   │    - 向 Guest 注入特殊 #PF (Page Ready)                        │        │
│   │    - 风险：可能与 Guest 自身的 #PF 冲突                        │        │
│   │                                                                │        │
│   │  Modern (中断方式, KVM_FEATURE_ASYNC_PF_INT):                  │        │
│   │    - 向 Guest 注入 KVM_ASYNC_PF_VECTOR 中断                    │        │
│   │    - 更安全，不会与正常 #PF 冲突                               │        │
│   └────────────────────────────────────────────────────────────────┘        │
│        ↓                                                                     │
│   list_add_tail(&apf->link, &vcpu->async_pf.done);                          │
│                                                                              │
│   ═══════════════════════════════════════════════════════════════════════   │
│                                                                              │
│   PHASE 4: Guest 恢复执行                                                     │
│   ─────────────────────────────────────                                     │
│                                                                              │
│   Guest 收到 "Page Ready" 通知：                                             │
│        ↓                                                                     │
│   Legacy: exc_page_fault() → kvm_handle_async_pf()                           │
│   Modern: sysvec_kvm_asyncpf_interrupt()                                     │
│        ↓                                                                     │
│   kvm_async_pf_task_wake():                                                  │
│       - 根据 apf_id 找到等待的 task                                           │
│       - 唤醒之前睡眠的线程                                                    │
│        ↓                                                                     │
│   被唤醒的线程重新执行缺页指令 → 页面已在内存中 → 正常执行                     │
│                                                                              │
└──────────────────────────────────────────────────────────────────────────────┘
```

```txt
kvm:kvm_try_async_get_page                         [Tracepoint event]
kvm:kvm_async_pf_completed                         [Tracepoint event]
kvm:kvm_async_pf_not_present                       [Tracepoint event]
kvm:kvm_async_pf_ready                             [Tracepoint event]
kvm:kvm_async_pf_repeated_fault                    [Tracepoint event]
```

1. 需要修改内核 kvm 外面的代码 ? 不然怎么来识别从 host inject 的
2. 内核如何调度 host 的另一个 task 过来运行的

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
