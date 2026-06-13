## `async_pf`
Asynchronous page fault is a way to try and use guest vcpu more efficiently
by allowing it to execute other tasks while page is brought back into memory[1].

## 为什么需要这种设计
因为 vCPU 发生 page fault 了，被切换走，然后 Host 从磁盘中请求内容，然后 host 就很尴尬:
- 不能继续执行 vCPU 线程了，因为如果开始执行，就需要保证该页已经准备好。
- host 侧没有什么任务需要执行。

基本的设计:
- host 需要告诉 Guest ，正在进行 apf ，可以干其他事情。
- Guest 的 thread 中执行 page fault 的 thread 暂停


## host 流程
- kvm_tdp_mmu_page_fault
  - kvm_faultin_pfn : 获取到 folio
    - `__kvm_faultin_pfn`
      - `__gfn_to_pfn_memslot` : **参数** async 实际上表示这个 page 是否已经进获取到了
  - kvm_tdp_mmu_map : 填充获取到

在 `__kvm_faultin_pfn` 中，进行两个操作
  - kvm_find_async_pf_gfn : 已经在等待改 page 了
  - kvm_arch_setup_async_pf :
    - `kvm_setup_async_pf`
    - kvm_arch_async_page_present : 给虚拟机注入一个中断

## 关联结构体

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(结构体写的不对，但是无所谓)

2.1.1 `struct kvm_async_pf` - 异步页错误描述符

```c
// arch/x86/include/asm/kvm_host.h
struct kvm_async_pf {
    struct work_struct work;           // 工作队列项
    struct list_head link;             // 链接到 vcpu->async_pf.done 链表
    struct kvm_vcpu *vcpu;             // 关联的 vCPU
    gpa_t gpa;                         // Guest 物理地址（触发缺页的地址）
    u32 cr3;                           // 发生缺页时的 CR3
    bool inflight;                     // 是否正在处理中
    bool pageready_pending;            // 是否有待处理的 page ready 事件

    // APF 标识符，用于 Guest 区分不同的异步页错误请求
    u64 apf_id;                        // 唯一标识符
};
```

2.1.2 `async_pf_work` - 工作队列执行体

```c
// virt/kvm/async_pf.c
static void async_pf_execute(struct work_struct *work)
{
    struct kvm_async_pf *apf = container_of(work, struct kvm_async_pf, work);
    struct kvm_vcpu *vcpu = apf->vcpu;

    // 1. 获取页面（可能阻塞，因为可能需要 I/O）
    // 2. 页面准备好后，通知 Guest
    kvm_arch_async_page_present(vcpu, apf);
}
```

## guest 流程

### 触发 page fault
在 arch/x86/mm/fault.c 中标准 page fault 的入口:
```txt
DEFINE_IDTENTRY_RAW_ERRORCODE(exc_page_fault)
```
- kvm_handle_async_pf
  - `__kvm_handle_async_pf`
    - kvm_async_pf_task_wait_schedule : 被 swapout 的 page，所以睡眠

- asm_sysvec_kvm_asyncpf_interrupt 就是 guest 接受到 host 的信息说可以了。

### 接受信息
```txt
DEFINE_IDTENTRY_SYSVEC(sysvec_kvm_asyncpf_interrupt)
```

arch/x86/kernel/kvm.c


## 附录

- kvm_arch_async_page_present : 通知 guest 事情搞定了

一般路径
```txt
kvm_tdp_mmu_map+615
kvm_tdp_page_fault+191
kvm_mmu_do_page_fault+486
kvm_mmu_page_fault+130
vmx_handle_exit+300
kvm_arch_vcpu_ioctl_run+1765
kvm_vcpu_ioctl+558
__x64_sys_ioctl+148
do_syscall_64+193
entry_SYSCALL_64_after_hwframe+119
```


## 问题

- `KVM_FEATURE_ASYNC_PF_INT` : guest 是通过 cpuid 获取的
- 和 KVM_FEATURE_ASYNC_PF 的关系是什么？ KVM_FEATURE_ASYNC_PF 似乎根本没用啊

- sysvec_kvm_asyncpf_interrupt
  - kvm_async_pf_task_wake
  - wrmsrl(MSR_KVM_ASYNC_PF_ACK, 1);

```diff
History:        #0
Commit:         2635b5c4a0e407b84f68e188c719f28ba0e9ae1b
Author:         Vitaly Kuznetsov <vkuznets@redhat.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    2020年05月25日 星期一 22时41分20秒
Committer Date: 2020年06月01日 星期一 16时26分07秒

KVM: x86: interrupt based APF 'page ready' event delivery

Concerns were expressed around APF delivery via synthetic #PF exception as
in some cases such delivery may collide with real page fault. For 'page
ready' notifications we can easily switch to using an interrupt instead.
Introduce new MSR_KVM_ASYNC_PF_INT mechanism and deprecate the legacy one.

One notable difference between the two mechanisms is that interrupt may not
get handled immediately so whenever we would like to deliver next event
(regardless of its type) we must be sure the guest had read and cleared
previous event in the slot.

While on it, get rid on 'type 1/type 2' names for APF events in the
documentation as they are causing confusion. Use 'page not present'
and 'page ready' everywhere instead.

Signed-off-by: Vitaly Kuznetsov <vkuznets@redhat.com>
Message-Id: <20200525144125.143875-6-vkuznets@redhat.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```
看上去为来方式 exception ，所以专门做了一个新的入口。

看看
```txt
  kvm:kvm_try_async_get_page                         [Tracepoint event]
  kvm:kvm_async_pf_completed                         [Tracepoint event]
  kvm:kvm_async_pf_not_present                       [Tracepoint event]
  kvm:kvm_async_pf_ready                             [Tracepoint event]
  kvm:kvm_async_pf_repeated_fault                    [Tracepoint event]
```

### 等待阅读的资料
- [ ] https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/03/24/kvm-async-page-fault
1. 需要修改内核 kvm 外面的代码 ? 不然怎么来识别从 host inject 的
2. 内核如何调度 host 的另一个 task 过来运行的
- [ ] https://lwn.net/Articles/817239/
- [https://lwn.net/Articles/845473/](aarch64: Support Asynchronous Page Fault)
  - 这个 patch 描述的比较清楚了


### 调试一个 bug

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


测试 mmoc 的时候，似乎启动嵌套，其中 l2 使用的是普通内存，也会有这个问题:
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

## async pf 的几个基本问题
<!-- 94f5a5a0-07af-4d2a-a9e6-5db6db14d99a -->

1. arm 支持吗? 不支持
2. 最多支持多少个 guest thread 的 page fault 被 stall ，答案一个 CPU 是 64 个
```c
 bool kvm_setup_async_pf(struct kvm_vcpu *vcpu, ...)
  {
      if (vcpu->async_pf.queued >= ASYNC_PF_PER_VCPU)
          return false;  // 超过限制，转为同步处理
      ...
  }
```

3. 使用 async_pf 的调试的前提是什么?

似乎关键的判断就是在:
```txt
	if (!fault->prefetch && kvm_can_do_async_pf(vcpu)) {
		trace_kvm_try_async_get_page(fault->addr, fault->gfn);
```
1. !fault->prefetch : 如果不是提前建立映射
2. kvm_can_do_async_pf 容易理解，就是可以注入中断。

(没完全搞懂，但是一步之遥)

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

## apf 的确存在两个注入方法
<!-- 185bcdd2-51eb-454c-b934-e41bea81d630 -->

第一种接受方法:
```c
DEFINE_IDTENTRY_SYSVEC(sysvec_kvm_asyncpf_interrupt)
{
	struct pt_regs *old_regs = set_irq_regs(regs);
	u32 token;

	apic_eoi();

	inc_irq_stat(irq_hv_callback_count);

	if (__this_cpu_read(async_pf_enabled)) {
		token = __this_cpu_read(apf_reason.token);
		kvm_async_pf_task_wake(token);
		__this_cpu_write(apf_reason.token, 0);
		wrmsrq(MSR_KVM_ASYNC_PF_ACK, 1);
	}

	set_irq_regs(old_regs);
}
```

第二种还是这个:
```c
DEFINE_IDTENTRY_RAW_ERRORCODE(exc_page_fault)
{
	irqentry_state_t state;
	unsigned long address;

	address = cpu_feature_enabled(X86_FEATURE_FRED) ? fred_event_data(regs) : read_cr2();

	/*
	 * KVM uses #PF vector to deliver 'page not present' events to guests
	 * (asynchronous page fault mechanism). The event happens when a
	 * userspace task is trying to access some valid (from guest's point of
	 * view) memory which is not currently mapped by the host (e.g. the
	 * memory is swapped out). Note, the corresponding "page ready" event
	 * which is injected when the memory becomes available, is delivered via
	 * an interrupt mechanism and not a #PF exception
	 * (see arch/x86/kernel/kvm.c: sysvec_kvm_asyncpf_interrupt()).
	 *
	 * We are relying on the interrupted context being sane (valid RSP,
	 * relevant locks not held, etc.), which is fine as long as the
	 * interrupted context had IF=1.  We are also relying on the KVM
	 * async pf type field and CR2 being read consistently instead of
	 * getting values from real and async page faults mixed up.
	 *
	 * Fingers crossed.
	 *
	 * The async #PF handling code takes care of idtentry handling
	 * itself.
	 */
	if (kvm_handle_async_pf(regs, (u32)address))
		return;
```

这个东西，再次让疑惑，execption 和中断的区别到底是什么?

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
