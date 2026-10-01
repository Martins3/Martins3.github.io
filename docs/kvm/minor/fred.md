# fred

在 kvm 中时不时看到 fred 机制:

- https://cdrdv2-public.intel.com/779982/346446-flexible-return-and-event-delivery.pdf
- https://www.phoronix.com/news/Intel-FRED-Incompatible-ENDBR64
- https://lore.kernel.org/lkml/e7dd1510-6ffa-429a-9b07-55ad83d40d7b@zytor.com/#r
- https://liujunming.top/2023/09/10/Intel-FRED-feature/

handle_external_interrupt_irqoff
```c
	if (cpu_feature_enabled(X86_FEATURE_FRED))
		fred_entry_from_kvm(EVENT_TYPE_EXTINT, vector);
	else
		vmx_do_interrupt_irqoff(gate_offset((gate_desc *)host_idt_base + vector));
```
但是 13900k 中没有 enable




> [!NOTE]
> 参考神奇海螺的意见，有待验证

## 很遗憾，我没有测试平台
Intel 消费级 CPU 从 Panther Lake 开始支持 FRED，对应 Core Ultra Series 3（酷睿 Ultra 第三代），这代产品于 2026
年初上市。Intel 产品资料
(https://www.intel.com/content/www/us/en/ark/products/codename/237132/products-formerly-panther-lake.html)

按产品线区分：

 产品线                开始支持 FRED 的架构
━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━
 消费级 Core Ultra     Panther Lake
────────────────────  ──────────────────────
 服务器 Xeon E-core    Clearwater Forest
────────────────────  ──────────────────────
 服务器 Xeon P-core    Diamond Rapids

这些架构列在 Intel 官方指令集扩展手册的 FRED 引入表中。Intel 官方手册，表 1-2
(https://cdrdv2-public.intel.com/869288/319433-060-architecture-instruction-set-extensions-programming-reference.pdf)

所以你这台 13 代 i9-13900K（Raptor Lake）不支持 FRED，升级内核也无法开启。

## 为什么需要 fred

**FRED（Flexible Return and Event Delivery）是 Intel 为 x86-64 设计的一套新的中断、异常和特权级切换机制。**

它的核心目标有两个：

1. **提高性能**：替代传统的 IDT 中断分发和 `IRET` 返回机制，降低事件处理延迟。
2. **简化内核**：让 CPU 自动完成更多中断入口和出口的上下文管理，消除传统 x86 中断处理中的一些复杂问题。

理解 FRED，最重要的是理解它为什么要取代传统的 IDT 机制。

## 1. 传统 x86 中断处理有什么问题？

假设一个用户态程序运行时发生了 Page Fault。

传统 x86 的处理过程大致如下：

```text
User Mode (Ring 3)
       |
       | Page Fault (#PF)
       v
CPU 查询 IDT
       |
       | 查找对应的 Interrupt Gate
       | 切换到内核栈
       | 保存部分 CPU 状态
       v
内核异常入口
       |
       | SWAPGS（如果需要）
       | 保存寄存器
       | 建立内核执行环境
       v
Page Fault Handler
       |
       | 恢复寄存器
       | 恢复用户态执行环境
       v
      IRETQ
       |
       v
User Mode (Ring 3)
```

问题在于，CPU 只完成了部分上下文切换工作，剩下的工作需要操作系统完成。

例如：

**问题一：SWAPGS 的复杂性。**

传统 x86-64 Linux 内核通过 `GS` 寄存器访问 per-CPU 数据。

用户态进入内核时，需要确保 `GS.base` 指向内核的 per-CPU 数据。

Linux 通常使用：

```asm
swapgs
```

但是，`SWAPGS` 必须正确判断执行时机。

例如，假设 CPU 已经进入内核，但是尚未执行 `SWAPGS`，此时又发生 NMI。

NMI 处理程序需要判断当前 `GS` 究竟指向用户态还是内核。

这就是传统 x86 内核中断入口代码复杂的原因之一。

**问题二：IST 无法自然处理嵌套异常。**

传统 x86 的 Interrupt Stack Table（IST）允许某些异常使用独立的栈。

例如：

- NMI
- Double Fault
- Machine Check

但是，IST 本身没有完善的嵌套管理机制。

如果一个使用 IST 的异常处理程序执行期间，再次发生使用同一个 IST 的事件，CPU 可能重新使用该 IST 栈顶，从而覆盖已有的栈帧。

操作系统必须额外处理这些情况。

**问题三：IRET 的复杂性。**

传统的 `IRETQ` 不只是恢复指令指针和栈。

它还涉及特权级检查、状态恢复以及 NMI 阻塞状态等问题。

尤其是在 NMI 处理期间又发生异常时，`IRET` 无条件解除 NMI 阻塞的行为可能引入复杂情况。

FRED 就是为系统性地改善这些问题而设计的。:chatgpt-content-reference{index="0"}

---

## 2. FRED 如何工作？

FRED 不再要求 CPU 根据 IDT 中的不同 Gate 直接跳转到各个处理程序。

它采用统一的事件入口机制。

```text
User Mode
    |
    | Exception / Interrupt
    v
FRED Event Delivery
    |
    | CPU 自动：
    | - 选择内核栈
    | - 保存事件上下文
    | - 切换 GS.base
    | - 建立内核执行环境
    v
统一事件入口
    |
    | 根据 Event Type
    | 和 Event Vector 分发
    v
具体 Handler
    |
    v
ERETU
    |
    v
User Mode
```

这里有一个重要区别。

传统 IDT 机制：

```text
Event Vector
      |
      v
     IDT
      |
      v
不同的入口地址
```

FRED：

```text
Event
  |
  v
统一事件入口
  |
  v
Software Dispatcher
  |
  +--> Exception Handler
  |
  +--> Interrupt Handler
  |
  +--> NMI Handler
```

FRED 将具体事件的分发工作交给操作系统。

Linux 使用两级分发机制：

1. 根据 Event Type 进行第一次分发。
2. 根据 Event Vector 进行第二次分发。

需要注意，FRED 并不是把所有事件处理工作都交给软件。CPU 仍然负责事件交付、必要的特权级切换和上下文保存。:chatgpt-content-reference{index="1"}

---

## 3. FRED 最重要的几个改进

### 3.1 不再需要 SWAPGS

这是我认为理解 FRED 时尤其值得关注的一个变化。

传统方式：

```asm
; User -> Kernel

swapgs

; Kernel handler
...

; Kernel -> User

swapgs
iretq
```

FRED 会在用户态进入内核时自动交换相关 GS base 状态。

而从内核返回用户态时，`ERETU` 会执行相应操作。

因此，FRED 模式下不再需要 `SWAPGS`，实际上也不允许执行这条指令。

这能消除一类与 `SWAPGS` 执行时机有关的内核入口问题。

### 3.2 引入新的返回指令

FRED 提供两条新的返回指令：

| 指令 | 功能 |
|---|---|
| `ERETU` | 从 Ring 0 返回 Ring 3 |
| `ERETS` | 返回时仍然处于 Ring 0 |

与传统 `IRETQ` 不同，FRED 的返回机制可以明确控制 NMI 解除阻塞的行为。

这使得嵌套异常处理更加容易。

### 3.3 使用 Stack Level 替代传统 IST

FRED 引入四个栈级别：

```text
Stack Level 0
Stack Level 1
Stack Level 2
Stack Level 3
```

每个级别可以配置自己的专用栈。

FRED 事件发生时，CPU 可以维持当前栈级别，也可以提升到更高级别。

只有执行 FRED 返回指令时，当前栈级别才会降低。

这与传统 IST 存在本质区别。

例如：

```text
Kernel
   |
   | NMI
   v
Stack Level 2
   |
   | Nested event
   v
继续根据 Stack Level 规则处理
```

如果新事件没有提升栈级别，CPU 就继续使用当前事件栈，而不是无条件重新切换到某个固定的 IST 栈顶。

因此，FRED 可以更自然地处理嵌套事件。

不过，它并不意味着所有栈溢出或嵌套问题都会自动消失。:chatgpt-content-reference{index="2"}

---

## 4. FRED 与 SYSCALL 是什么关系？

这是一个容易混淆的地方。

**FRED 并不简单等于用一种新的指令替代 `SYSCALL`。**

传统 Linux 中：

- `SYSCALL/SYSRET` 主要负责快速系统调用。
- IDT 负责中断和异常。
- `IRETQ` 负责相应的事件返回。

FRED 旨在统一并改善事件交付和返回机制，提供一套更完整的特权级切换体系。

不过，FRED 模式下的 64 位系统调用仍然可以使用 `SYSCALL`。

因此，不应该把 FRED 理解成完全废除 `SYSCALL/SYSRET`。

---

## 5. Linux 内核如何实现 FRED？

Linux 已经具有 FRED 支持。

建议关注三个源码文件：

| 文件 | 作用 |
|---|---|
| `arch/x86/kernel/fred.c` | 初始化 FRED，配置相关 MSR 和事件栈 |
| `arch/x86/entry/entry_fred.c` | FRED 事件分发 |
| `arch/x86/include/asm/fred.h` | FRED 相关定义 |

例如，在 `fred.c` 中可以看到 Linux 对不同事件配置不同的 Stack Level。

内核为 `#DB`、NMI、`#MC`、`#DF` 配置事件栈级别，同时设置相应的栈指针 MSR。

启用 FRED 后，内核会设置 `CR4.FRED`，并使传统 IDT 失效。:chatgpt-content-reference{index="3"}

具体实现可以参考：

[Linux FRED 文档](https://kernel.org/doc/html/latest/arch/x86/x86_64/fred.html?utm_source=chatgpt.com)

[Linux FRED 事件入口源码](https://github.com/torvalds/linux/blob/master/arch/x86/entry/entry_fred.c?utm_source=chatgpt.com)

[Intel FRED 架构规范](https://www.intel.com/content/www/us/en/content-details/795033/flexible-return-and-event-delivery-fred-specification.html?utm_source=chatgpt.com)

---

## 6. 总结

从操作系统设计的角度看，FRED 的意义不仅仅在于提高中断性能。

传统 x86 的中断机制经历了长期演进，产生了 IDT、IST、TSS、`SWAPGS`、`IRETQ` 等多种需要相互协调的机制。

这种设计使内核入口代码相当复杂，而且容易出现特殊情况下的状态不一致问题。

**FRED 的核心思想，就是让 CPU 为内核提供一个更加完整、一致的事件入口和返回机制。**

它试图同时解决三个问题：减少事件处理延迟、降低操作系统入口代码的复杂性，以及提高嵌套异常处理的可靠性。

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
