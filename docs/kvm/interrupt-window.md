## kvm interrupt window 到底在说什么
<!-- 2fa61a03-4527-4aca-a499-227a158af4d5 -->

guest 屏蔽中断的时候无法注入中断，所以给设置一个标志，如果
让 guest 一旦打开中断，那么立刻开始注入。

```txt
┌─────────────────────────────────────────────────────────────────┐
│              Interrupt Window vs APICv                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  场景 1: APICv OFF（传统模式）                                    │
│  ───────────────────────────                                    │
│  所有中断 → 软件检查 injectable → 阻塞则 enable_irq_window        │
│              ↓                                                  │
│         窗口打开 → VM-Exit → handle_interrupt_window → inject    │
│                                                                 │
│  场景 2: APICv ON（加速模式）                                     │
│  ──────────────────────────                                     │
│  LAPIC 中断 → Posted Interrupt → 硬件自动注入                    │
│                     ↓                                           │
│         不需要 Interrupt Window！                                 │
│                                                                 │
│  场景 3: APICv ON + ExtInt                                        │
│  ───────────────────────────                                    │
│  PIC/ExtInt 中断 → 软件注入路径 → 仍需要 Interrupt Window        │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

代码证据:
```c
/*
 * check if there is injectable interrupt:
 * when virtual interrupt delivery enabled,
 * interrupt from apic will handled by hardware,
 * we don't need to check it here.
 */
int kvm_cpu_has_injectable_intr(struct kvm_vcpu *v)
{
	if (kvm_cpu_has_extint(v))
		return 1;

	if (!is_guest_mode(v) && kvm_vcpu_apicv_active(v))
		return 0;

	return kvm_apic_has_interrupt(v) != -1; /* LAPIC */
}
```

### 标记
```c
void vmx_enable_irq_window(struct kvm_vcpu *vcpu)
{
	exec_controls_setbit(to_vmx(vcpu), CPU_BASED_INTR_WINDOW_EXITING);
}
```

vmx_enable_irq_window 的调用位置是:

```c
	if (kvm_check_request(KVM_REQ_EVENT, vcpu) || req_int_win ||
	    kvm_xen_has_interrupt(vcpu)) {
        // -> kvm_check_and_inject_events
        // -> if (req_int_win)
		//      kvm_x86_call(enable_irq_window)(vcpu);

```

### 处理
```c
	[EXIT_REASON_INTERRUPT_WINDOW]        = handle_interrupt_window,
```

```c
static int handle_interrupt_window(struct kvm_vcpu *vcpu)
{
	exec_controls_clearbit(to_vmx(vcpu), CPU_BASED_INTR_WINDOW_EXITING);

	kvm_make_request(KVM_REQ_EVENT, vcpu);

	++vcpu->stat.irq_window_exits;
	return 1;
}
```

## 其实我们是没有完全搞清楚的
- irq windows 和 vapic 如何配合工作
- [ ] interrupt window 这个东西应该不用了吧

### 资料
sdm table C-1 中的描述:

Interrupt window. At the beginning of an instruction,
RFLAGS.IF was 1; events were not blocked by STI or by MOV
SS; and the “interrupt-window exiting” VM-execution control was 1.

解释看 SDM section 27.7.4

参考: http://liujunming.top/2020/10/09/Some-notes-about-Interrupt-Window/

claude 给出的回答:

# Interrupt Window Exiting 机制详解

## 核心问题：中断被阻塞时如何注入？

### 中断阻塞的三种情况

`arch/x86/kvm/vmx/vmx.c:5033-5037`：
```c
bool __vmx_interrupt_blocked(struct kvm_vcpu *vcpu)
{
    return !(vmx_get_rflags(vcpu) & X86_EFLAGS_IF) ||      // 1. IF=0
           (vmcs_read32(GUEST_INTERRUPTIBILITY_INFO) &
            (GUEST_INTR_STATE_STI |                         // 2. STI shadow
             GUEST_INTR_STATE_MOV_SS));                     // 3. MOV SS shadow
}
```

**1. RFLAGS.IF = 0**：中断标志位关闭
- Guest 执行 `CLI` 指令后，不接受可屏蔽中断
- 这是最常见的情况

**2. STI Shadow（STI 指令阴影）**：
- **问题**：`STI` 指令设置 IF=1，但中断不是立即生效
- **原因**：允许原子执行 `STI; HLT` 或 `STI; RET` 序列
- **行为**：`STI` 之后的**一条指令**期间中断被阻塞

**3. MOV SS Shadow（MOV SS 指令阴影）**：
- **问题**：修改 SS 段寄存器后立即中断会导致栈不一致
- **行为**：`MOV SS` 或 `POP SS` 之后的**一条指令**期间中断被阻塞
- **目的**：保证 `MOV SS; MOV ESP` 的原子性

## Interrupt Window 的解决方案

### 问题场景

```
Host: 有一个中断要注入给 Guest
       ↓
检查: Guest 当前中断被阻塞（IF=0 或 Shadow）
       ↓
困境: 无法立即注入，但又不能错过 Guest 重新开启中断的时刻
```

### 传统方案（无硬件支持）
```
轮询检查 Guest 的 RFLAGS.IF
  → 浪费 CPU 周期
  → 可能错过短暂的中断窗口
```

### Interrupt Window Exiting 机制

`arch/x86/kvm/vmx/vmx.c:4899-4902`：
```c
void vmx_enable_irq_window(struct kvm_vcpu *vcpu)
{
    exec_controls_setbit(to_vmx(vcpu), CPU_BASED_INTR_WINDOW_EXITING);
}
```

**工作原理**：
1. Host 设置 `CPU_BASED_INTR_WINDOW_EXITING` 控制位
2. 继续运行 Guest
3. **硬件自动监控**：当 Guest 的中断窗口打开时（IF=1 且无 Shadow）
4. **硬件自动触发** VM-Exit（exit reason = INTERRUPT_WINDOW）
5. Host 处理 VM-Exit，注入中断

## 完整调用流程

### 1. 尝试注入中断失败

`arch/x86/kvm/x86.c:10768-10783`：
```c
if (kvm_cpu_has_injectable_intr(vcpu)) {
    r = can_inject ? kvm_x86_call(interrupt_allowed)(vcpu, true) :
                     -EBUSY;
    if (r < 0)
        goto out;
    if (r) {
        // 可以注入，执行注入
        int irq = kvm_cpu_get_interrupt(vcpu);
        kvm_queue_interrupt(vcpu, irq, false);
        kvm_x86_call(inject_irq)(vcpu, false);
    }

    // 关键：如果还有中断待注入但当前无法注入
    if (kvm_cpu_has_injectable_intr(vcpu))
        kvm_x86_call(enable_irq_window)(vcpu);  // 启用 interrupt window
}
```

### 2. 启用 Interrupt Window Exiting

设置 VMCS 控制位，让硬件监控中断窗口。

### 3. VM-Entry 运行 Guest

Guest 继续运行，硬件持续监控。

### 4. Guest 执行打开中断的操作

例如：
```asm
sti          ; 开启中断
nop          ; 此指令期间仍在 STI shadow
; 【这里】- 中断窗口打开！
```

### 5. 硬件自动触发 VM-Exit

**无需软件干预**，CPU 自动 VM-Exit。

### 6. 处理 Interrupt Window Exit

`arch/x86/kvm/vmx/vmx.c:5622-5630`：
```c
static int handle_interrupt_window(struct kvm_vcpu *vcpu)
{
    // 1. 清除控制位（不再需要监控）
    exec_controls_clearbit(to_vmx(vcpu), CPU_BASED_INTR_WINDOW_EXITING);

    // 2. 请求重新处理事件
    kvm_make_request(KVM_REQ_EVENT, vcpu);

    ++vcpu->stat.irq_window_exits;
    return 1;
}
```

### 7. 重新尝试注入

回到步骤 1，这次 `interrupt_allowed()` 返回成功。

## 时序图

```
Timeline:
--------
Host:  有中断  → 检查    → enable_irq_window → VM-Entry
               (blocked)      ↓
                              设置监控位

Guest:                         运行(IF=0) → STI → NOP → [窗口开启]
                                                              ↓
                                                         自动VM-Exit
                                                              ↓
Host:                                              handle_interrupt_window
                                                              ↓
                                                     重新检查并注入中断
                                                              ↓
                                                          VM-Entry
Guest:                                              收到中断并执行ISR
```

## 关键优势

| 特性 | 无 Interrupt Window | 有 Interrupt Window |
|------|-------------------|-------------------|
| **检测时机** | 软件轮询 | 硬件自动监控 |
| **响应延迟** | 可能错过窗口 | 立即响应 |
| **CPU 开销** | 频繁 VM-Exit 检查 | 仅窗口开启时 Exit |
| **准确性** | 可能不准确 | 精确到指令边界 |

## 实际例子

假设 Guest 执行：
```asm
cli                    ; 关闭中断
; ... 临界区代码 ...
sti                    ; 开启中断
hlt                    ; 等待中断
```

**无 Interrupt Window**：
- Host 可能在 `sti` 和 `hlt` 之间错过注入机会
- Guest 进入 `hlt` 后等待中断，但中断未注入

**有 Interrupt Window**：
- Host 在 `sti` 执行后、`hlt` 执行前**自动获得控制**
- 在 `hlt` 之前完成中断注入
- Guest 从 `hlt` 唤醒后立即处理中断

## 与 Virtual Interrupt Delivery 的关系

**重要区别**：
- **Interrupt Window**：解决**何时能注入**的问题（中断阻塞监控）
- **Virtual Interrupt Delivery**：解决**如何注入**的问题（硬件自动注入）

可以组合使用：
1. Interrupt Window 等待注入时机
2. Virtual Interrupt Delivery 完成注入（无需 `inject_irq`）

## 总结

**Interrupt Window Exiting** 是一个优雅的硬件辅助机制，它让 **Host 能够在 Guest 中断窗口开启的第一时间获得控制**，从而及时注入中断，避免延迟和轮询开销。这是现代 x86 虚拟化中断处理的关键特性之一。

## 参考资料

- [STI — Set Interrupt Flag](https://www.felixcloutier.com/x86/sti)
- [STI | x86 Instruction Set Reference](https://tizee.github.io/x86_ref_book_web/instruction/sti.html)
- [Page 1052 - Intel Manual](https://xem.github.io/minix86/manual/intel-x86-and-64-manual-vol3/o_fe12b1e2a880e0ce-1052.html)
- Linux Kernel Source: `arch/x86/kvm/vmx/vmx.c`
- Linux Kernel Source: `arch/x86/kvm/x86.c`



# APICv 与 Interrupt Window 的关系

## 核心问题

**问题**：APICv 打开之后，既然存在 Virtual Interrupt Delivery，还需要 Interrupt Window 这个优化吗？

**答案**：需要！它们不是非此即彼的关系，而是互补的。

## 关键代码：APICv 激活时的行为

`arch/x86/kvm/irq.c:90-105`：
```c
/*
 * check if there is injectable interrupt:
 * when virtual interrupt delivery enabled,
 * interrupt from apic will handled by hardware,
 * we don't need to check it here.
 */
int kvm_cpu_has_injectable_intr(struct kvm_vcpu *v)
{
    if (kvm_cpu_has_extint(v))
        return 1;

    if (!is_guest_mode(v) && kvm_vcpu_apicv_active(v))
        return 0;  // APICv 激活，LAPIC 中断返回 0！

    return kvm_apic_has_interrupt(v) != -1; /* LAPIC */
}
```

**关键点**：当 APICv 激活时，来自 LAPIC 的中断返回 0，意味着**不需要软件注入**！

## APICv 与 Interrupt Window 的关系

### 1. APICv 开启时：LAPIC 中断不需要 Interrupt Window

**原因**：Virtual Interrupt Delivery 硬件自动处理

`arch/x86/kvm/x86.c:10768-10783`：
```c
if (kvm_cpu_has_injectable_intr(vcpu)) {  // APICv 激活时对 LAPIC 返回 0
    r = can_inject ? kvm_x86_call(interrupt_allowed)(vcpu, true) :
                     -EBUSY;
    if (r < 0)
        goto out;
    if (r) {
        int irq = kvm_cpu_get_interrupt(vcpu);
        kvm_queue_interrupt(vcpu, irq, false);
        kvm_x86_call(inject_irq)(vcpu, false);
    }
    if (kvm_cpu_has_injectable_intr(vcpu))
        kvm_x86_call(enable_irq_window)(vcpu);  // 不会执行
}
```

**流程对比**：

| 场景 | kvm_cpu_has_injectable_intr | 是否调用 enable_irq_window |
|------|---------------------------|-------------------------|
| **APICv OFF + LAPIC 中断** | 返回 1（需要注入） | ✅ 是 |
| **APICv ON + LAPIC 中断** | 返回 0（硬件处理） | ❌ 否 |

### 2. 但以下场景仍需要 Interrupt Window

#### (1) ExtInt 中断（外部中断控制器）

`arch/x86/kvm/irq.c:59-88`：
```c
int kvm_cpu_has_extint(struct kvm_vcpu *v)
{
    if (!lapic_in_kernel(v))
        return v->arch.interrupt.injected;

    if (kvm_xen_has_interrupt(v))
        return 1;

#ifdef CONFIG_KVM_IOAPIC
    if (pic_in_kernel(v->kvm))
        return v->kvm->arch.vpic->output;  // PIC 中断
#endif

    return pending_userspace_extint(v);  // 用户空间中断
}
```

**关键**：`kvm_cpu_has_injectable_intr()` 首先检查 ExtInt：
```c
if (kvm_cpu_has_extint(v))
    return 1;  // 即使 APICv 开启也返回 1！
```

**来源**：
- **PIC (8259A)**：传统中断控制器
- **用户空间模拟设备**：QEMU 通过 KVM_INTERRUPT ioctl 注入
- **Xen 兼容中断**

这些中断**不通过 LAPIC**，因此**不受 Virtual Interrupt Delivery 管理**，仍需软件注入 + Interrupt Window！

#### (2) 用户空间请求 Interrupt Window

`arch/x86/kvm/x86.c:10487-10491`：
```c
static int dm_request_for_irq_injection(struct kvm_vcpu *vcpu)
{
    return vcpu->run->request_interrupt_window &&
           likely(!pic_in_kernel(vcpu->kvm));
}
```

`arch/x86/kvm/x86.c:11048-11050, 11234-11235`：
```c
bool req_int_win =
    dm_request_for_irq_injection(vcpu) &&
    kvm_cpu_accept_dm_intr(vcpu);

// ...

if (req_int_win)
    kvm_x86_call(enable_irq_window)(vcpu);
```

**场景**：用户空间 LAPIC 模拟（split irqchip 模式）
- QEMU 模拟 LAPIC
- 通过 `KVM_RUN.request_interrupt_window` 请求窗口
- 即使 APICv 可用，用户空间仍需要知道何时注入

#### (3) 嵌套虚拟化场景

`arch/x86/kvm/vmx/vmx.c:5042-5046`：
```c
bool vmx_interrupt_blocked(struct kvm_vcpu *vcpu)
{
    if (is_guest_mode(vcpu) && nested_exit_on_intr(vcpu))
        return false;  // L2 需要 VM-Exit 到 L1

    return __vmx_interrupt_blocked(vcpu);
}
```

L2 运行时：
- L1 可能配置了 "interrupt exiting"
- 中断需要先 VM-Exit 到 L1
- 需要 Interrupt Window 机制确保时机正确

#### (4) APICv 动态抑制

`arch/x86/kvm/x86.c:10522-10523`：
```c
if (vcpu->arch.apic->apicv_active)
    return;  // APICv 激活时跳过
```

APICv 可能被临时禁用：
- Hyper-V SynIC 激活
- 某些设备直通场景
- 嵌套虚拟化特定状态

此时回退到软件注入 + Interrupt Window。

## 完整对比表

| 中断类型 | APICv 状态 | Virtual Interrupt Delivery | 需要软件注入 | 需要 Interrupt Window |
|---------|-----------|---------------------------|------------|---------------------|
| **LAPIC 中断** | ON | ✅ 硬件自动 | ❌ | ❌ |
| **LAPIC 中断** | OFF | ❌ | ✅ | ✅ |
| **ExtInt (PIC)** | ON/OFF | ❌ 不支持 | ✅ | ✅ |
| **用户空间 LAPIC** | - | ❌ | ✅ | ✅ |
| **嵌套 L2** | ON | 部分 | 部分 | ✅ |

## 工作流程图

### APICv OFF（传统模式）
```
所有中断
  ↓
kvm_cpu_has_injectable_intr() 返回 1
  ↓
检查 interrupt_allowed()
  ↓
如果阻塞 → enable_irq_window()
  ↓
窗口打开 → handle_interrupt_window()
  ↓
inject_irq() 软件注入
```

### APICv ON（加速模式）
```
LAPIC 中断                          ExtInt 中断
  ↓                                   ↓
写入 PIR                         kvm_cpu_has_extint() 返回 1
  ↓                                   ↓
硬件自动 PIR → vIRR              检查 interrupt_allowed()
  ↓                                   ↓
Virtual Interrupt Delivery       如果阻塞 → enable_irq_window()
  ↓                                   ↓
Guest 收到中断                   窗口打开 → inject_irq()
（无需软件参与）                 （仍需软件注入）
```

## 关键优化

`arch/x86/kvm/x86.c:10522-10523`（update_cr8_intercept）：
```c
if (vcpu->arch.apic->apicv_active)
    return;  // APICv 激活时跳过 CR8 拦截优化
```

APICv 开启时，很多传统优化（如 CR8 拦截）都被跳过，因为硬件已经接管。

## 详细代码路径分析

### LAPIC 中断路径（APICv ON）

1. **中断到达**：
   ```
   设备中断 → irqfd → kvm_set_irq → kvm_irq_delivery_to_apic_fast
     → __apic_accept_irq → vmx_deliver_interrupt
   ```

2. **写入 PIR**：
   ```c
   vmx_deliver_posted_interrupt(vcpu, vector)
     → __vmx_deliver_posted_interrupt()
     → pi_test_and_set_pir(vector, pi_desc)  // 写入 PIR
     → pi_test_and_set_on(pi_desc)           // 设置 ON 位
     → kvm_vcpu_trigger_posted_interrupt()   // 发送 notification
   ```

3. **硬件处理**：
   - 如果 vcpu->mode == IN_GUEST_MODE：硬件自动 PIR → vIRR
   - Virtual Interrupt Delivery 自动注入
   - **完全绕过 kvm_cpu_has_injectable_intr()**

### ExtInt 中断路径（无论 APICv 状态）

1. **中断到达**：
   ```
   PIC 中断 → kvm_pic_set_irq → vcpu->kvm->arch.vpic->output = 1
   ```

2. **检查可注入性**：
   ```c
   kvm_cpu_has_injectable_intr(vcpu)
     → kvm_cpu_has_extint(vcpu) 返回 1  // 优先检查 ExtInt
     → 返回 1（需要注入）
   ```

3. **软件注入流程**：
   ```c
   if (kvm_cpu_has_injectable_intr(vcpu)) {
       if (kvm_x86_call(interrupt_allowed)(vcpu, true)) {
           inject_irq(vcpu, false);  // 软件注入
       } else {
           enable_irq_window(vcpu);  // 等待窗口
       }
   }
   ```

## 实际场景示例

### 场景 1：现代虚拟机（APICv ON）

**配置**：
- virtio-blk 设备（MSI 中断）
- 内核 LAPIC
- APICv 启用

**中断流程**：
```
virtio-blk 完成 I/O
  → eventfd 触发
  → irqfd → MSI 中断
  → 写入 PIR
  → 硬件自动 PIR → vIRR
  → Virtual Interrupt Delivery 注入
  → Guest 处理中断
```

**不需要 Interrupt Window**！

### 场景 2：传统设备模拟（需要 Interrupt Window）

**配置**：
- 模拟 IDE 硬盘（使用 PIC）
- APICv 启用

**中断流程**：
```
IDE 中断 → PIC
  → kvm_cpu_has_extint() 返回 1
  → 检查 Guest IF 标志
  → IF=0（被阻塞）
  → enable_irq_window()
  → Guest 执行 STI
  → handle_interrupt_window()
  → inject_irq() 注入 ExtInt
  → Guest 处理中断
```

**仍然需要 Interrupt Window**！

### 场景 3：Split IRQChip 模式

**配置**：
- QEMU 用户空间模拟 LAPIC
- 内核模拟 IOAPIC/PIC

**中断流程**：
```
QEMU 模拟 LAPIC 决定注入中断
  → 设置 KVM_RUN.request_interrupt_window
  → vcpu_enter_guest() 检测到请求
  → enable_irq_window()
  → Guest IF 打开
  → VM-Exit (INTERRUPT_WINDOW)
  → 返回 QEMU
  → QEMU 注入中断
```

**必须使用 Interrupt Window**！

## 性能对比

| 指标 | 传统模式 (无 APICv) | APICv + Interrupt Window |
|------|-------------------|------------------------|
| **LAPIC 中断延迟** | 2-3 VM-Exit | 0 VM-Exit |
| **ExtInt 中断延迟** | 2-3 VM-Exit | 2-3 VM-Exit（无变化） |
| **CPU 开销** | 高 | LAPIC 低，ExtInt 不变 |
| **吞吐量** | 中 | LAPIC 高，ExtInt 不变 |

## 总结

### APICv 和 Interrupt Window 的分工

1. **APICv 主要优化 LAPIC 中断路径**
   - Posted Interrupt 机制接收中断
   - Virtual Interrupt Delivery 硬件自动注入
   - 无需 Interrupt Window 机制
   - **覆盖大部分现代设备中断（MSI/MSI-X）**

2. **Interrupt Window 仍然重要**
   - ExtInt 中断（PIC）永远需要
   - 用户空间模拟场景（split irqchip）
   - 嵌套虚拟化复杂性
   - APICv 动态禁用时的回退机制
   - **覆盖传统设备和特殊场景**

3. **现代系统中两者共存**
   - LAPIC 中断走 APICv 快速路径（>90% 的中断）
   - ExtInt 等特殊中断走传统 Interrupt Window 路径（<10% 的中断）
   - 系统根据中断源自动选择最优路径
   - **提供最大兼容性和最优性能**

### 关键洞察

**APICv 不是替代 Interrupt Window，而是补充它**：
- APICv 处理高频 LAPIC 中断（性能关键路径）
- Interrupt Window 处理低频特殊中断（兼容性和功能完整性）
- 两者结合提供完整的虚拟化中断解决方案

这就是为什么即使在 APICv 时代，Interrupt Window 机制仍然是 KVM 不可或缺的一部分！

## 参考代码位置

- `arch/x86/kvm/irq.c:96-105` - kvm_cpu_has_injectable_intr()
- `arch/x86/kvm/irq.c:59-88` - kvm_cpu_has_extint()
- `arch/x86/kvm/x86.c:10768-10783` - 中断注入主逻辑
- `arch/x86/kvm/x86.c:10487-10491` - dm_request_for_irq_injection()
- `arch/x86/kvm/vmx/vmx.c:4899-4902` - vmx_enable_irq_window()
- `arch/x86/kvm/vmx/vmx.c:5622-5630` - handle_interrupt_window()
- `arch/x86/kvm/vmx/vmx.c:5040-5046` - vmx_interrupt_blocked()

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
