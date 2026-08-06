# context_tracking

简而言之是用来做 context switch tracking ，然后来辅助 rcu 的。

kernel/context_tracking.c

```c
struct context_tracking {
#ifdef CONFIG_CONTEXT_TRACKING_USER
	/*
	 * When active is false, probes are unset in order
	 * to minimize overhead: TIF flags are cleared
	 * and calls to user_enter/exit are ignored. This
	 * may be further optimized using static keys.
	 */
	bool active;
	int recursion;
#endif
#ifdef CONFIG_CONTEXT_TRACKING
	atomic_t state;
#endif
#ifdef CONFIG_CONTEXT_TRACKING_IDLE
	long dynticks_nesting;		/* Track process nesting level. */
	long dynticks_nmi_nesting;	/* Track irq/NMI nesting level. */
#endif
};
```

## 基本观察
```txt
🧀  zcat /proc/config.gz | grep CONTEXT_TRACKING
CONFIG_CONTEXT_TRACKING=y
CONFIG_CONTEXT_TRACKING_IDLE=y
CONFIG_CONTEXT_TRACKING_USER=y
# CONFIG_CONTEXT_TRACKING_USER_FORCE is not set
CONFIG_HAVE_CONTEXT_TRACKING_USER=y
CONFIG_HAVE_CONTEXT_TRACKING_USER_OFFSTACK=y
```

```txt
config CONTEXT_TRACKING_USER
	bool
	depends on HAVE_CONTEXT_TRACKING_USER
	select CONTEXT_TRACKING
	help
	  Track transitions between kernel and user on behalf of RCU and
	  tickless cputime accounting. The former case relies on context
	  tracking to enter/exit RCU extended quiescent states.

config CONTEXT_TRACKING_USER_FORCE
	bool "Force user context tracking"
	depends on CONTEXT_TRACKING_USER
	default y if !NO_HZ_FULL
	help
	  The major pre-requirement for full dynticks to work is to
	  support the user context tracking subsystem. But there are also
	  other dependencies to provide in order to make the full
	  dynticks working.

	  This option stands for testing when an arch implements the
	  user context tracking backend but doesn't yet fulfill all the
	  requirements to make the full dynticks feature working.
	  Without the full dynticks, there is no way to test the support
	  for user context tracking and the subsystems that rely on it: RCU
	  userspace extended quiescent state and tickless cputime
	  accounting. This option copes with the absence of the full
	  dynticks subsystem by forcing the user context tracking on all
	  CPUs in the system.

	  Say Y only if you're working on the development of an
	  architecture backend for the user context tracking.

	  Say N otherwise, this option brings an overhead that you
	  don't want in production.
```

```txt
config CONTEXT_TRACKING
	bool

config CONTEXT_TRACKING_IDLE
	bool
	select CONTEXT_TRACKING
	help
	  Tracks idle state on behalf of RCU.
```

### 简单分析下
```txt
sudo bpftrace -e "tracepoint:rcu:rcu_dyntick { @[kstack] = count(); }"
```

这个是 rcu_dyntick ，居然可以得到这样的结果:
```txt
@[
    ct_kernel_enter.isra.0+188
    ct_kernel_enter.isra.0+188
    ct_idle_exit+30
    cpuidle_enter_state+811
    cpuidle_enter+45
    do_idle+436
    cpu_startup_entry+41
    start_secondary+284
    common_startup_64+318
]: 31214
```

- ct_kernel_exit
- ct_kernel_enter

## 这么复杂吗?
- https://lore.kernel.org/linux-kernel//465c71e018de9800ba22a84b9c16f56f99aabefd.camel@kernel.org/T/#m72d553398b545c724d5a531b51eb57941abddc74


## codex 的解释

kernel/context_tracking.c 用来追踪“当前 CPU 正运行在哪种高层上下文”，主要服务于 RCU 和 NO_HZ_FULL。

这里的 context 不是调度器的进程上下文切换，而是：

```txt
KERNEL  内核态
USER    用户态
GUEST   虚拟机 guest 态
IDLE    CPU 空闲态
```

状态定义在 include/linux/context_tracking_state.h:12。

核心问题是：RCU 需要知道某个 CPU 是否可能还在 RCU 读侧临界区。

kernel                         idle/user/guest
可能使用 rcu_read_lock()       不会执行内核 RCU 读侧临界区
RCU 必须观察该 CPU             可以视为扩展静止状态 EQS

当 CPU 进入 idle、user 或 guest 后，可能很长时间没有调度和周期 tick。如果没有 context tracking，RCU 可能一直等待这个
CPU，或者必须保留周期 tick 来确认它已经经过静止状态。

因此 context tracking 会明确通知 RCU：

进入 user/idle/guest：
    这个 CPU 已进入 Extended Quiescent State
    RCU 不再需要观察它

返回 kernel：
    这个 CPU 又可能执行 RCU 读侧代码
    RCU 必须重新观察它

文件头也直接说明了这一目的：kernel/context_tracking.c:3。

主要入口如下：

```txt
 场景         进入                               离开
━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 CPU idle     ct_idle_enter()                    ct_idle_exit()
───────────  ─────────────────────────────────  ────────────────────────────────
 用户态       __ct_user_enter(CT_STATE_USER)     __ct_user_exit(CT_STATE_USER)
───────────  ─────────────────────────────────  ────────────────────────────────
 KVM guest    __ct_user_enter(CT_STATE_GUEST)    __ct_user_exit(CT_STATE_GUEST)
───────────  ─────────────────────────────────  ────────────────────────────────
 IRQ/NMI      ct_irq_enter() / ct_nmi_enter()    ct_irq_exit() / ct_nmi_exit()
```

例如系统调用边界：

返回用户空间
    user_enter_irqoff()
        -> RCU not watching
        -> USER / EQS

用户执行
    不需要为了 RCU 保留周期 tick

发生系统调用
    user_exit_irqoff()
        -> RCU watching
        -> KERNEL

通用系统调用入口、出口分别调用它们：include/linux/irq-entry-common.h:98 和 include/linux/irq-entry-common.h:267。

### 中断为什么需要特殊处理

即使 CPU 正处于 idle/user/guest，IRQ 或 NMI 仍可能进入内核并使用 RCU：

IDLE，RCU not watching
        |
        | IRQ
        v
IRQ handler，RCU watching
        |
        | IRQ return
        v
IDLE，RCU not watching

所以 ct_nmi_enter() 会在必要时临时让 RCU 重新观察 CPU；最外层 IRQ/NMI 返回后，再恢复原来的 EQS 状态。nesting 和
nmi_nesting 用来处理嵌套，避免每层中断都错误地切换 RCU 状态。

### 每个 CPU 记录什么

每 CPU 都有一个：

struct context_tracking {
	bool active;
	int recursion;
	atomic_t state;
	long nesting;
	long nmi_nesting;
};

参见 include/linux/context_tracking_state.h:21。

其中 state 同时编码：

高位：RCU watching 状态/变化序列
低位：KERNEL、IDLE、USER、GUEST

状态更新使用原子操作和内存屏障，使其他 CPU 上的 RCU grace-period 代码能够可靠观察到这次状态转换。

### 与配置的关系

CONFIG_TREE_RCU 会选择 CONFIG_CONTEXT_TRACKING_IDLE，所以 Tree RCU 默认需要追踪 idle：kernel/rcu/Kconfig:8。

CONFIG_NO_HZ_FULL 还需要 CONFIG_CONTEXT_TRACKING_USER，以便 CPU 长时间运行单一用户任务时关闭调度 tick：kernel/time/
Kconfig:130。

简而言之：

> context_tracking.c 是 CPU 执行状态与 RCU/nohz 之间的边界通知器。它告诉 RCU：“这个 CPU 现在可能使用 RCU”或者“这个 CPU
> 已进入安全静止状态，不需要继续等它，也不需要为它维持 tick”。

## 主要使用用户

- CONFIG_CONTEXT_TRACKING_IDLE 部分基本就是为 RCU 服务的。
- CONFIG_CONTEXT_TRACKING_USER 部分除了 RCU，还服务于 CPU 时间记账、NO_HZ_FULL 和调试/跟踪。

主要用途如下。

### 1. RCU

这是最核心的用途，包括普通 Tree RCU 和 Tasks RCU。

它通知 RCU CPU 是否处于扩展静止状态：

KERNEL                 RCU watching
IDLE/USER/GUEST         RCU not watching
IRQ/NMI                 临时恢复 RCU watching

在 NO_HZ_FULL 下，它还维护 current->rcu_tasks_idle_cpu，供 Tasks RCU 判断任务是否已经进入静止状态：kernel/
context_tracking.c:41。

### 2. 精确 CPU 时间记账

CONFIG_VIRT_CPU_ACCOUNTING_GEN 不能依赖周期 tick 统计用户态和内核态时间，因为 NO_HZ_FULL CPU 的 tick 可能已经停止。

所以它利用 context tracking 的边界进行记账：

kernel -> user:
    vtime_user_enter()
    开始统计用户时间

user -> kernel:
    vtime_user_exit()
    结算用户时间，开始统计系统时间

直接调用位于 kernel/context_tracking.c:467 和 kernel/context_tracking.c:603。

具体记账实现在 kernel/sched/cputime.c:722：

void vtime_user_enter(struct task_struct *tsk)
{
	vtime_account_system(tsk, vtime);
	vtime->state = VTIME_USER;
}

void vtime_user_exit(struct task_struct *tsk)
{
	vtime->utime += get_vtime_delta(vtime);
	vtime->state = VTIME_SYS;
}

Kconfig 也明确说明 CPU 时间记账通过 context tracking 观察每次 kernel/user 边界：init/Kconfig:591。

### 3. 支撑 NO_HZ_FULL

NO_HZ_FULL 希望 CPU 即使正在运行用户任务，也能关闭周期调度 tick。

关闭 tick 前必须解决两个依赖：

RCU 需要 tick 判断静止状态
CPU 时间统计需要 tick 区分 user/system 时间

context tracking 提供精确边界后：

进入用户态
    -> 通知 RCU：CPU 已静止
    -> 切换 vtime 到 USER
    -> 周期 tick 可以停止

进入内核
    -> 恢复 RCU watching
    -> 切换 vtime 到 SYSTEM
    -> 必要时恢复 tick

NO_HZ_FULL 初始化时只为 nohz_full= 指定的 CPU 激活用户态 tracking：kernel/time/tick-sched.c:649。

因此更准确地说：

> context tracking 不是“关闭 tick”的代码，但它提供了关闭 tick 所必需的上下文信息。

### 4. KVM guest 状态

KVM 进入 guest 与返回用户态类似：CPU 可能长时间不执行 host 内核代码，因此 guest 被视为 RCU 静止状态：

HOST KERNEL
    guest_context_enter_irqoff()
        -> CT_STATE_GUEST
        -> RCU not watching

GUEST

    guest_context_exit_irqoff()
        -> CT_STATE_KERNEL
        -> RCU watching

调用入口位于 include/linux/kvm_host.h:426。

Guest CPU 时间本身主要由 guest_timing_enter/exit 和 vtime_guest_enter/exit 处理；context tracking 主要记录 guest/host 边界及其
RCU 状态。

### 5. Tracepoint 和状态校验

它还提供：

context_tracking:user_enter
context_tracking:user_exit

可以通过 ftrace/perf 等观察 kernel/user 转换，定义见 include/trace/events/context_tracking.h:27。

内核入口代码和调度器也使用 ct_state() 检查状态机是否损坏。例如如果调度时仍标记为 CT_STATE_USER，会触发警告：kernel/sched/
core.c:5968。

所以整体关系是：

                    context_tracking
                           |
        +------------------+------------------+
        |                  |                  |
       RCU          VIRT_CPU_ACCOUNTING     状态跟踪
        |                  |              trace/debug
        +------------------+
                 |
             NO_HZ_FULL
          可以安全关闭 tick

它的主要消费者仍然是 RCU，但 CONTEXT_TRACKING_USER 已经是 RCU、full-dynticks 和虚拟 CPU 时间记账共用的基础设施。

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
