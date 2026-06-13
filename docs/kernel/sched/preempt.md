# preempt
## CONFIG_PREEMPT_DYNAMIC
<!-- 923b90ea-7083-44ce-b3d9-d208ccccf455 -->

如果打开了 CONFIG_PREEMPT_DYNAMIC 选项，
那么可以通过内核命令行参数控制 preempt 的模式:

`__sched_dynamic_update` 中检查 preempt_dynamic_disable 和 preempt_dynamic_enable 的实现很容易知道

下面这些函数，当 enable 的时候 ，会调用对应的 `f##_dynamic_enabled` 版本，如果 disable 就直接一个空函数
- cond_resched
- might_resched
- preempt_schedule
- preempt_schedule_notrace
- irqentry_exit_cond_resched

```c
/*
 * SC:cond_resched
 * SC:might_resched
 * SC:preempt_schedule
 * SC:preempt_schedule_notrace
 * SC:irqentry_exit_cond_resched
 *
 *
 * NONE:
 *   cond_resched               <- __cond_resched
 *   might_resched              <- RET0
 *   preempt_schedule           <- NOP
 *   preempt_schedule_notrace   <- NOP
 *   irqentry_exit_cond_resched <- NOP
 *
 * VOLUNTARY:
 *   cond_resched               <- __cond_resched
 *   might_resched              <- __cond_resched
 *   preempt_schedule           <- NOP
 *   preempt_schedule_notrace   <- NOP
 *   irqentry_exit_cond_resched <- NOP
 *
 * FULL:
 *   cond_resched               <- RET0
 *   might_resched              <- RET0
 *   preempt_schedule           <- preempt_schedule
 *   preempt_schedule_notrace   <- preempt_schedule_notrace
 *   irqentry_exit_cond_resched <- irqentry_exit_cond_resched
 */

enum {
	preempt_dynamic_undefined = -1,
	preempt_dynamic_none,
	preempt_dynamic_voluntary,
	preempt_dynamic_full,
};
```

当打开 full 的时候，相对于 voluntary 的变化:
1. irqentry_exit -> irqentry_exit_cond_resched
2. preempt_enable -> preempt_schedule 和 preempt_schedule_notrace ( notrace 没有细究)

## 通过 __schedule 函数的注释理解 preempt
<!-- c59e6896-2484-4a59-9448-6f6094720abd -->

```c
/*
 * __schedule() is the main scheduler function.
 *
 * The main means of driving the scheduler and thus entering this function are:
 *
 *   1. Explicit blocking: mutex, semaphore, waitqueue, etc.
 *
 *   2. TIF_NEED_RESCHED flag is checked on interrupt and userspace return
 *      paths. For example, see arch/x86/entry_64.S.
 *
 *      To drive preemption between tasks, the scheduler sets the flag in timer
 *      interrupt handler scheduler_tick().
 *
 *   3. Wakeups don't really cause entry into schedule(). They add a
 *      task to the run-queue and that's it.
 *
 *      Now, if the new task added to the run-queue preempts the current
 *      task, then the wakeup sets TIF_NEED_RESCHED and schedule() gets
 *      called on the nearest possible occasion:
 *
 *       - If the kernel is preemptible (CONFIG_PREEMPT=y):
 *
 *         - in syscall or exception context, at the next outmost
 *           preempt_enable(). (this might be as soon as the wake_up()'s
 *           spin_unlock()!)
 *
 *         - in IRQ context, return from interrupt-handler to
 *           preemptible context
 *
 *       - If the kernel is not preemptible (CONFIG_PREEMPT is not set)
 *         then at the next:
 *
 *          - cond_resched() call
 *          - explicit schedule() call
 *          - return from syscall or exception to user-space
 *          - return from interrupt-handler to user-space
 *
 * WARNING: must be called with preemption disabled!
 */
static void __sched notrace __schedule(bool preempt)
```
2026-04-12 还是感觉写的很乱:
这个注释的意思，什么时候会发生 __schedule 进程的上下文切换。
1. 通过 explicit 的操作，直接切换走
2. 如果任务被 wake up 不会立刻切换，只是加入到 run-queue 中，不会立刻切换
	- 正在运行的任务被检查到存在 TIF_NEED_RESCHED
	- 此外检查的位置是特定的位置的，这个取决于 CONFIG_PREEMPT

所以，我感觉这里不清楚的地方在于，为什么第二条提到 TIF_NEED_RESCHED 的检查位置，
应该是想表达 TIF_NEED_RESCHED 总是会检查，但是未必会立刻切换，中断的时候总是会检查 TIF_NEED_RESCHED

这里需要说的时候， 如果时间片没有用完，但是还是不小心遇到了，是不会立刻切换的
- cond_resched
- might_resched / might_sleep
- preempt_enable -> preempt_schedule
- irq



1. 为什么 must be called with preemption disabled ?


需要设置上 TIF_NEED_RESCHED 才可以，在 __schedule -> pick_next_task 可能还是当前的进程的，不一定会切换走。

However, in nonpreemptive kernels, the current process cannot be replaced unless it is about to switch to User Mode.
Therefore, the main characteristic of a preemptive kernel is that a process running in
Kernel Mode can be replaced by another process while in the middle of a kernel
function.

> So preemption will only happen after an interrupt, but an interrupt doesn't always cause preemption.
> https://stackoverflow.com/questions/40204506/what-is-the-difference-between-nonpreemptive-and-preemptive-kernels-when-switch

- 当内核不能抢占，那么想要切换，只有在返回到用户态的时候才可以，打开之后，在 interrupt handle 的时候都会进行检查

- 一旦 preempt_disable()，可以保证接下来执行的代码都是在同一个 CPU 上的

在配合这个理解一下，不过为什么 ftrace function graph 的时候，
为什么第一个函数总是有中断，这个可以思考一下为什么:
```txt
 113)               |  vfio_iommu_type1_ioctl [vfio_iommu_type1]() {
 113)               |    irq_enter_rcu() {
 113)   0.230 us    |      irqtime_account_irq();
 113)   0.770 us    |    }
 113)               |    __sysvec_irq_work() {
 113)               |      __wake_up() {
 113)   0.180 us    |        _raw_spin_lock_irqsave();
 113)               |        __wake_up_common.isra.0() {
 113)               |          pollwake() {
 113)               |            default_wake_function() {
 113)               |              try_to_wake_up() {
 113)   0.170 us    |                _raw_spin_lock_irqsave();
 113)               |                select_task_rq_fair() {
 113)               |                  select_idle_sibling() {
 113)   0.600 us    |                    available_idle_cpu();
 113)   1.200 us    |                  }
 113)   0.180 us    |                  rcu_read_unlock_strict();
 113)   2.120 us    |                }
 113)               |                ttwu_queue() {
 113)               |                  ttwu_queue_wakelist.part.0() {
 113)               |                    __smp_call_single_queue() {
 113)   0.450 us    |                      send_call_function_single_ipi();
 113)   1.330 us    |                    }
 113)   1.790 us    |                  }
 113)   2.170 us    |                }
 113)   0.180 us    |                _raw_spin_unlock_irqrestore();
 113)   0.170 us    |                ttwu_stat();
 113)   7.200 us    |              }
 113)   7.550 us    |            }
 113)   8.150 us    |          }
 113)   9.100 us    |        }
 113)   0.180 us    |        _raw_spin_unlock_irqrestore();
 113) + 10.160 us   |      }
 113) + 10.880 us   |    }
 113)               |    irq_exit_rcu() {
 113)   0.210 us    |      irqtime_account_irq();
 113)   0.190 us    |      idle_cpu();
 113)   0.920 us    |    }
 113) + 17.930 us   |  }
```

## CONFIG_PREEMPT_COUNT

`CONFIG_PREEMPT` will select `CONFIG_PREEMPT_COUNT`

https://lwn.net/Articles/831678/ : valuable

- [ ] when will preempt happens ?

```c
asmlinkage __visible void __sched notrace preempt_schedule(void)
```


## CONFIG_PREEMPT_NOTIFIERS
<!-- 003ef725-f2e1-4b80-9553-2e4b5e4c9142 -->

其实，不是被 preempted 之后需要执行这个 hook ，其实只是简单的被 sched in 和 sched out 需要执行，
原因很简单，当一个物理 CPU 上运行的 vCPU 发生变化之后，对应的 vmcs 也是需要切换的。
```txt
@[
        kvm_sched_in+0
        __schedule+720
        schedule+48
        kvm_vcpu_block+96
        kvm_vcpu_halt+100
        kvm_vcpu_wfi+52
        kvm_handle_wfx+396
        handle_exit+104
        kvm_arch_vcpu_ioctl_run+596
        kvm_vcpu_ioctl+424
        __arm64_sys_ioctl+188
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 17418
```

```txt
@[
        kvm_sched_out+5
        prepare_task_switch+338
        __schedule+574
        schedule+39
        kvm_vcpu_block+76
        kvm_vcpu_halt+389
        vcpu_run+437
        kvm_arch_vcpu_ioctl_run+781
        kvm_vcpu_ioctl+276
        __x64_sys_ioctl+151
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 32254
```


调用路径为:
- `context_switch`
  - `prepare_task_switch`
    - `fire_sched_out_preempt_notifiers`

就目前的代码，只有 kvm 机制使用 preempt_notifier ，当 vcpu_load 的时候，注册上
preempt_notifier ，当 vcpu_put 的时候，将 preempt_notifier 取消掉注册。
```c
/*
 * Switches to specified vcpu, until a matching vcpu_put()
 */
void vcpu_load(struct kvm_vcpu *vcpu)
{
	int cpu = get_cpu();

	__this_cpu_write(kvm_running_vcpu, vcpu);
	preempt_notifier_register(&vcpu->preempt_notifier);
	kvm_arch_vcpu_load(vcpu, cpu);
	put_cpu();
}
EXPORT_SYMBOL_GPL(vcpu_load);

void vcpu_put(struct kvm_vcpu *vcpu)
{
	preempt_disable();
	kvm_arch_vcpu_put(vcpu);
	preempt_notifier_unregister(&vcpu->preempt_notifier);
	__this_cpu_write(kvm_running_vcpu, NULL);
	preempt_enable();
}
EXPORT_SYMBOL_GPL(vcpu_put);
```

```txt
@[
        vcpu_load+0
        kvm_vcpu_ioctl+424
        __arm64_sys_ioctl+180
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 12
```

也就是说，在 vcpu_load 和  vcpu_put 之间，kvm thread 线程可以被调度走，
当离开的时候，需要执行操作，把 vCPU 资源和物理 CPU 解绑，而当被调度回来，
继续运行的时候，vCPU 需要将资源重新和物理 CPU 绑定 (该物理 CPU 可能已经是另外一个物理 CPU 了)。

## PREEMPT 的三个模式的差别是什么?
<!-- 7d589cf3-be20-4b02-91f6-02e11d423157 -->

如果 CONFIG_PREEMPT_DYNAMIC 打开之后，
具体定义在: kernel/Kconfig.preempt
```txt
	preempt=	[KNL]
			Select preemption mode if you have CONFIG_PREEMPT_DYNAMIC
			none - Limited to cond_resched() calls
			voluntary - Limited to cond_resched() and might_sleep() calls
			full - Any section that isn't explicitly preempt disabled
			       can be preempted anytime.  Tasks will also yield
			       contended spinlocks (if the critical section isn't
			       explicitly preempt disabled beyond the lock itself).
			lazy - Scheduler controlled. Similar to full but instead
			       of preempting the task immediately, the task gets
			       one HZ tick time to yield itself before the
			       preemption will be forced. One preemption is when the
			       task returns to user space.
```
再次强调一次，CONFIG_PREEMPT_VOLUNTARY 多出来的地方就是 might_sleep()

`kernel/Kconfig.preempt`

- No Forced Preemption (Server)
- Voluntary Kernel Preemption (Desktop) `CONFIG_PREEMPT_VOLUNTARY`
- Preemptible Kernel (Low-Latency Desktop) `CONFIG_PREEMPT`


server 中的 config :
```txt
zcat /proc/config.gz | grep PREEMPT

CONFIG_PREEMPT_NONE_BUILD=y
CONFIG_PREEMPT_NONE=y
# CONFIG_PREEMPT_VOLUNTARY is not set
# CONFIG_PREEMPT is not set
# CONFIG_PREEMPT_RT is not set
# CONFIG_PREEMPT_DYNAMIC is not set
CONFIG_HAVE_PREEMPT_DYNAMIC=y
CONFIG_HAVE_PREEMPT_DYNAMIC_KEY=y
CONFIG_PREEMPT_NOTIFIERS=y
# CONFIG_PREEMPTIRQ_DELAY_TEST is not set
```
这个是服务器的配置，想不到默认连 PREEMPT 都没有打开。
由于 PREEMPT 没有打开，所以 PREEMPT_RCU 没有打开。

nixos 中的结果:
```txt
CONFIG_PREEMPT_BUILD=y
CONFIG_ARCH_HAS_PREEMPT_LAZY=y
# CONFIG_PREEMPT_NONE is not set
CONFIG_PREEMPT_VOLUNTARY=y
# CONFIG_PREEMPT is not set
# CONFIG_PREEMPT_LAZY is not set
# CONFIG_PREEMPT_RT is not set
CONFIG_PREEMPT_COUNT=y
CONFIG_PREEMPTION=y
CONFIG_PREEMPT_DYNAMIC=y
CONFIG_PREEMPT_RCU=y
CONFIG_HAVE_PREEMPT_DYNAMIC=y
CONFIG_HAVE_PREEMPT_DYNAMIC_CALL=y
CONFIG_PREEMPT_NOTIFIERS=y
# CONFIG_DEBUG_PREEMPT is not set
# CONFIG_PREEMPT_TRACER is not set
CONFIG_PREEMPTIRQ_DELAY_TEST=m
```

再次体会他们的关系的地方:
include/linux/kernel.h
```c
#ifdef CONFIG_PREEMPT_VOLUNTARY_BUILD

extern int __cond_resched(void);
# define might_resched() __cond_resched()

#elif defined(CONFIG_PREEMPT_DYNAMIC) && defined(CONFIG_HAVE_PREEMPT_DYNAMIC_CALL)

extern int __cond_resched(void);

DECLARE_STATIC_CALL(might_resched, __cond_resched);

static __always_inline void might_resched(void)
{
	static_call_mod(might_resched)();
}

#elif defined(CONFIG_PREEMPT_DYNAMIC) && defined(CONFIG_HAVE_PREEMPT_DYNAMIC_KEY)

extern int dynamic_might_resched(void);
# define might_resched() dynamic_might_resched()

#else

# define might_resched() do { } while (0)

#endif /* CONFIG_PREEMPT_* */
```
从这里可以看到:
1. cond_resched 和 might_resched 被 enable 之后，两个函数都是调用 __cond_resched
2. dynamic 存在两个机制，一个 dynamic call ，一个是 dynamic key

```c
/**
 * might_sleep - annotation for functions that can sleep
 *
 * this macro will print a stack trace if it is executed in an atomic
 * context (spinlock, irq-handler, ...). Additional sections where blocking is
 * not allowed can be annotated with non_block_start() and non_block_end()
 * pairs.
 *
 * This is a useful debugging help to be able to catch problems early and not
 * be bitten later when the calling function happens to sleep when it is not
 * supposed to.
 */
# define might_sleep() \
	do { __might_sleep(__FILE__, __LINE__); might_resched(); } while (0)
```
3. might_sleep 在没有额外调试选项的，等价于 might_resched
	- 不过，might_resched 一般 scheduler 机制内部使用，外部使用都是使用 might_sleep 的，毕竟添加了一个调试方法

### might_sleep/might_resched 和 cond_resched 的关系是什么?
might_sleep/might_resched 其实有点像是一个断言

cond_resched 一般用于循环中，所以 CONFIG_PREEMPT_VOLUNTARY 的差别是 might_sleep 不能正常用

(无论如何，测试一下 preempt=none ，voluntary 他们的效果，感觉 chatgpt 都是一口咬死，说 none 的时候，
cond_resched 等于空操作)

### 继续思考的东西
- CONFIG_PREEMPT_COUNT : 看看
- CONFIG_PREEMPT_NOTIFIERS : easy
- CONFIG_PREEMPT_RCU : 直接放弃

- commit c793a62823d1 ("sched/core: Drop spinlocks on contention iff kernel is preemptible")
    - https://lore.kernel.org/kvm/832697b9-3652-422d-a019-8c0574a188ac@proxmox.com/
        - proxmox 真的立功
- preempt=lazy


## preempt locking rules
<!-- 9749b2b6-17ab-40b2-8657-e3d19c77c55b -->

- [Documentation/locking/preempt-locking.rst](https://www.kernel.org/doc/html/latest/locking/preempt-locking.html)
  - 回答了问题，什么时候才可以关闭抢占。

> Note that you do not need to explicitly prevent preemption
if you are holding any locks or interrupts are disabled, since preemption is implicitly disabled in those cases.

> But keep in mind that 'irqs disabled' is a fundamentally unsafe way of
disabling preemption - any cond_resched() or cond_resched_lock() might trigger
a reschedule if the preempt count is 0. A simple printk() might trigger a
reschedule. So use this implicit preemption-disabling property only if you
know that the affected codepath does not do any of this. Best policy is to use
this only for small, atomic code that you wrote and which calls no complex
functions.

为什么感觉这两段话是互相矛盾的啊

- https://stackoverflow.com/questions/18254713/why-linux-disables-kernel-preemption-after-the-kernel-code-holds-a-spinlock


```sh
local_irq_disable();
schedule();
local_irq_enable();
```
通常不合法，至少“语义上不对”，普通内核代码不应该这样写：


在你这棵树里，普通 schedule() 入口是 kernel/sched/core.c:6998；它内部进入 __schedule() 后会自己 local_irq_disable()，但最终通过 kernel/sched/
sched.h:1628 的 raw_spin_rq_unlock_irq() 把 IRQ 打开。所以：

1. 你前面的 local_irq_disable() 不能跨过 schedule() 保持住。
2. schedule() 返回时，本地 IRQ 实际上已经被调度器打开了，你后面的 local_irq_enable() 只是再次 enable，没有形成成对保护。
3. 如果你的本意是“关中断保护一段临界区”，那这段代码完全达不到目的。
4. 如果同时还持有 spinlock、preempt_disable()、RCU read lock 等 atomic 上下文，schedule() 会触发 BUG: scheduling while atomic，见 kernel/sched/
   core.c:5873 和 kernel/sched/core.c:5834。

更细一点说，local_irq_disable() 本身未必立刻触发这个 atomic 检查，因为调度器这里主要看 preempt_count，定义在 include/linux/preempt.h:190。但这不代表这
种写法是允许的，只代表“未必当场被那条检查抓住”。

内核里确实有一个“带 IRQ disabled 进入调度”的专用入口 preempt_schedule_irq()，但那是调度器/异常路径内部使用的，不是普通代码该模仿的，见 kernel/sched/
core.c:7220。

结论：普通内核代码里不要写 local_irq_disable(); schedule(); local_irq_enable();。
正确做法通常是：先在关中断区里更新状态，然后先恢复 IRQ，再 schedule()；或者直接用现成的等待原语，如 wait_event*()、completion、mutex 等。


## 为什么访问 percpu 变量一定需要关闭抢占
<!-- 5b53a0ac-e042-4c5b-a768-82a08576241f -->

```c
// BUG 示例：错误的 per-CPU 访问
void buggy_percpu_access(void)
{
    // 错误：没有禁止抢占
    int cpu = smp_processor_id();

    // 这里可能被抢占，调度到其他 CPU！
    per_cpu(my_data, cpu)++;  // 可能修改了错误的 CPU 数据
}

// FIX: 正确做法
void fixed_percpu_access(void)
{
    preempt_disable();
    // 或使用 get_cpu()/put_cpu() 包装

    int cpu = smp_processor_id();
    per_cpu(my_data, cpu)++;

    preempt_enable();
}
```

例如：
1. 这段代码一开始在 CPU0 上运行
2. smp_processor_id() 返回 0
3. 线程被抢占了
4. 调度器把它恢复到 CPU1 上继续执行
5. 执行 per_cpu(my_data, 0)++

于是结果变成：
- 你现在人已经在 CPU1
- 但你改的是 CPU0 的那份数据

## 通过 ftrace 来知道函数执行的上下文
<!-- 67bbbb02-59f0-4f86-83a8-070bdea76272 -->

```txt
# tracer: function
#
# entries-in-buffer/entries-written: 230/230   #P:32
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
   (udev-worker)-349     [002] d..2.     0.619341: irte_ga_set_affinity <-irq_remapping_activate
```

```txt
 qemu-system-x86-15440   [019] ..... 25595.802457: __alloc_pages_noprof <-___kmalloc_large_node
 qemu-system-x86-15440   [019] ..... 25595.802615: __alloc_pages_noprof <-___kmalloc_large_node
          <idle>-0       [021] ..s1. 25596.915160: __alloc_pages_noprof <-igb_alloc_rx_buffers
          <idle>-0       [021] ..s1. 25596.915335: __alloc_pages_noprof <-igb_alloc_rx_buffers
```
- __alloc_pages_noprof <-igb_alloc_rx_buffers : 这个是在软中断的，所以 preempt-depth 是 1


检查一下，这个数值应该参考这个实现才可以:
```txt
#define preemptible()   (preempt_count() == 0 && !irqs_disabled())
```

## 分析 yield() schedule() 和 cond_resched() 的关系

```c
/**
 * yield - yield the current processor to other threads.
 *
 * Do not ever use this function, there's a 99% chance you're doing it wrong.
 *
 * The scheduler is at all times free to pick the calling task as the most
 * eligible task to run, if removing the yield() call from your code breaks
 * it, it's already broken.
 *
 * Typical broken usage is:
 *
 * while (!event)
 *	yield();
 *
 * where one assumes that yield() will let 'the other' process run that will
 * make event true. If the current task is a SCHED_FIFO task that will never
 * happen. Never use yield() as a progress guarantee!!
 *
 * If you want to use yield() to wait for something, use wait_event().
 * If you want to use yield() to be 'nice' for others, use cond_resched().
 * If you still want to use yield(), do not!
 */
void __sched yield(void)
{
	set_current_state(TASK_RUNNING);
	do_sched_yield();
}
EXPORT_SYMBOL(yield);
```

## preempt count 的检测位置在哪里

preempt_disable 只是增加了 preempt_count 就可以了

```c
void raw_irqentry_exit_cond_resched(void)
{
    if (!preempt_count()) {
        /* Sanity check RCU and thread stack */
        rcu_irq_exit_check_preempt();
        if (IS_ENABLED(CONFIG_DEBUG_ENTRY))
            WARN_ON_ONCE(!on_thread_stack());
        if (need_resched())
            preempt_schedule_irq();
    }
}
```



## migrate_disable
<!-- 19365690-ad93-46af-9e68-0c6c9ac22cdc -->

参考 https://docs.kernel.org/locking/locktypes.html
中的文档，migrate_disable 不会关闭抢占，但是 migrate_disable
的作用是保证当前的 process 不会离开这个 CPU 。
- 可以被重入
- 可以调用 schedule


## nr_involuntary_switches 是来自于什么地方?
<!-- 66647e5e-f8de-4c55-87cd-7691839d36e3 -->

不是一个 voluntary preempt 的吗?

似乎已经搞懂过，但是没整理

```txt
🧀  cat /proc/3023637/sched
qemu-system-x86 (3023637, #threads: 46)
-------------------------------------------------------------------
se.exec_start                                :     950329463.742954
se.vruntime                                  :        138997.306307
se.sum_exec_runtime                          :        116211.255183
se.nr_migrations                             :                12579
nr_switches                                  :               402748
nr_voluntary_switches                        :               400078
nr_involuntary_switches                      :                 2670
se.load.weight                               :              1048576
se.avg.load_sum                              :                   59
se.avg.runnable_sum                          :                60418
se.avg.util_sum                              :                60418
se.avg.load_avg                              :                    1
se.avg.runnable_avg                          :                    1
se.avg.util_avg                              :                    1
se.avg.last_update_time                      :      950267385219072
se.avg.util_est.ewma                         :                    9
se.avg.util_est.enqueued                     :                    0
policy                                       :                    0
prio                                         :                  120
clock-delta                                  :                   19
```



## 如何通往 scheduler 的

- shrink_node_memcgs
  - shrink_lruvec
    - shrink_list
      - shrink_inactive_list
        - shrink_folio_list
          - folio_check_references
            - folio_referenced
              - rmap_walk
                - rmap_walk_anon
                  - _cond_resched
                    - __cond_resched
                      - preempt_schedule_common
                        - __schedule


- asm_sysvec_apic_timer_interrupt
  - irqentry_exit_to_user_mode
    - exit_to_user_mode_prepare
      - exit_to_user_mode_loop
        - schedule
          - __schedule_loop
            - __schedule

这个也算是 schedule() 在中断上下文中调用了。


## 为什么 preempt 没有打开的情况下，会出现 nonvoluntary_ctxt_switches ?

cat /proc/pid/status

```txt
voluntary_ctxt_switches:        28734
nonvoluntary_ctxt_switches:     94
```

随便找个 qemu 进程就可以看。

具体在 __schedule 的实现中，每次发上下文切换的时候都是有这两个数值的变化，具体哪一个判断为
```c
	switch_count = &prev->nivcsw; // 一般为 nonvoluntary_ctxt_switches

  if (sched_mode == SM_IDLE) {
		/* SCX must consult the BPF scheduler to tell if rq is empty */
		if (!rq->nr_running && !scx_enabled()) {
			next = prev;
			goto picked;
		}
	} else if (!preempt && prev_state) {
		try_to_block_task(rq, prev, &prev_state);
		switch_count = &prev->nvcsw; //
	}
```
所以，我们不是在说一个东西。

## 整理一下
```c
#define get_cpu()		({ preempt_disable(); __smp_processor_id(); })
#define put_cpu()		preempt_enable()
```

<p align="center">
  <img src="./img/preempt-LoyenWang.png" alt="drawing" align="center"/>
</p>
<p align="center">
from LoyenWang
</p>
- [ ] 这个图有点问题，没有 `PREEMPT_NEED_RESCHED`

https://github.com/torvalds/linux/blob/70664fc10c0d722ec79d746d8ac1db8546c94114/include/linux/preempt.h#L14-L30

## Questions

- arch/arm64/arm.c:kvm_arch_vcpu_ioctl_run 中，有一个 handle_exit_early 必须在打开 preemption 之前执行的代码
  - 我是一直没有理解什么样的代码不能被 preemption 的。
- gather_bootmem_prealloc 为什么需要有 cond_resched

- preemption 让内核必须给每一个 thread 配置一个内核栈，而不是每一个 CPU 一个。

有趣的阅读:
https://stackoverflow.com/questions/5283501/what-does-it-mean-to-say-linux-kernel-is-preemptive

1. 可以找个例子，如果 preempt 之后，代码变得更加复杂了的例子吗?

https://stackoverflow.com/questions/49414559/linux-kernel-why-preemption-is-disabled-when-use-per-cpu-variable

> If the kernel can be preempted, even uniprocessor systems will behave like SMP systems. Consider that
the kernel is working inside a critical region when it is preempted. The next task also operates in kernel
mode, and unfortunately also wants to access the same critical region. This is effectively equivalent to
two processors working in the critical region at the same time and must be prevented. Every time the
kernel is inside a critical region, kernel preemption must be disabled.


## 可以看看 CONFIG_DEBUG_PREEMPT=y 的调试效果
这个会引入很大的开销，他到底是需要解决什么问题的?


## 看看
https://lwn.net/Articles/944686/

## preempt 和 rcu 的关系
<!-- 81c26226-28cf-4aac-9834-8a3214ccb8d2 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

- NONE
    - cond_resched -> __cond_resched
    - might_resched -> RET0
    - preempt_schedule* -> NOP
    - irqentry_exit_cond_resched -> NOP
    - 含义：内核态不会被抢占，RCU 主要靠显式的阻塞/调度点前进。
    - 如果 cond_resched() 真调度了，就会走 rcu_note_context_switch()。
    - 如果没调度，在非 PREEMPT_RCU 下仍可能通过 rcu_all_qs() 给 RCU 补一个 QS。
    - 所以 NONE 下 cond_resched() 对 RCU 很重要，长循环里不放它，宽限期可能推进很慢。
- VOLUNTARY
    - cond_resched -> __cond_resched
    - might_resched -> __cond_resched
    - 其余还是 NOP
    - 含义：本质仍是“不允许内核任意点抢占”，只是把更多“自愿让出 CPU”的点打开了。
    - 对 RCU 来说，和 NONE 的核心差别是：might_resched() 现在也会变成一个真正的 QS/调度机会。
    - 所以宽限期推进通常比 NONE 更顺，因为代码里大量 might_resched() 也开始贡献机会了。
- FULL
    - cond_resched -> RET0
    - might_resched -> RET0
    - preempt_schedule -> preempt_schedule
    - preempt_schedule_notrace -> preempt_schedule_notrace
    - irqentry_exit_cond_resched -> irqentry_exit_cond_resched
    - 含义：RCU 不再依赖显式 cond_resched() 去“喂 QS”，而是依赖可抢占内核本身。
    - 一旦发生内核抢占或 IRQ 返回内核态时需要抢占，就会进 __schedule(SM_PREEMPT)，然后 rcu_note_context_switch(true)。
    - 在 PREEMPT_RCU 下，如果任务在 rcu_read_lock() 里被抢占，rcu_note_context_switch() 会把这个 task 挂到 blocked readers 链表，而不是错误地把它当成已经结束。
    - 所以 FULL 下 cond_resched() 变成 RET0 对普通 RCU 不是问题，因为 reader 的存在已经能被显式跟踪，QS 也主要靠抢占/上下文切换/用户态/idle/tick 来观察。

再压缩成一句话：

- NONE/VOLUNTARY：RCU 更依赖“显式自愿调度点”，尤其 cond_resched()，必要时它还能通过 rcu_all_qs() 补 QS。
- FULL：RCU 更依赖“真实的内核抢占和上下文切换”；cond_resched() 被关掉基本不影响普通 TREE_RCU 的正确性。

一个容易混淆的点：

- 这里说的是普通 RCU/TREE_RCU。
- RCU Tasks 不是一回事，所以内核专门有 cond_resched_tasks_rcu_qs()，即使 cond_resched() 被动态关掉，它也会先显式调用 rcu_tasks_qs()。
  include/linux/rcupdate.h:246


- FULL 下，rcu_read_lock() 内允许被抢占。
- FULL 下，rcu_read_lock() 内仍然不允许主动 sleep / 阻塞。
如果你需要“可以睡眠的 RCU 读侧”，那不是普通 rcu_read_lock()，而是 SRCU，例如 srcu_read_lock() / srcu_read_unlock()。

### cond_resched 的含义

```txt
 关键行为
   条件                     行为
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   should_resched(0) 为真   检查是否有 TIF_NEED_RESCHED 标志且 preempt_count 为 0
   中断未禁用               确保可以安全地调用 schedule()
   条件都满足               调用 preempt_schedule_common() 进行调度，返回 1
   条件不满足               仅报告 RCU 静止状态（非抢占式 RCU 内核），返回 0
  配置相关的语义差异
  • 非抢占式内核（!CONFIG_PREEMPTION）：cond_resched() 实际执行检查并可能调度
  • 动态抢占（CONFIG_PREEMPT_DYNAMIC）：可通过动态键控制是否启用
  • 完全抢占式内核（CONFIG_PREEMPTION && !CONFIG_PREEMPT_DYNAMIC）：cond_resched() 直接返回
    （因为内核已是可抢占的，无需显式检查）
  使用场景
  cond_resched() 主要用于以下场景：
  1. 大循环中：避免长时间占用 CPU，导致其他任务饥饿
  2. 内核路径中：在可能耗时的算法中（如排序、哈希表操作）定期插入检查点
  3. 减少延迟：在 Voluntary Preempt（自愿抢占）内核中提供显式抢占点
  相关变体
  • cond_resched_lock(lock)：在持有自旋锁时安全地进行条件重新调度
  • cond_resched_tasks_rcu_qs()：同时报告 RCU-tasks 静止状态
  • cond_resched_rcu()：在 RCU 读临界区内安全地进行条件重新调度
```

哦，原来 cond 提现在这里

cond_resched() 的 cond（conditional，条件）体现在触发调度的前提条件检查上。具体来说，有以下几个关键条件：
1. 需要重新调度的标志（核心条件）
```c
// include/asm-generic/preempt.h
static __always_inline bool should_resched(int preempt_offset)
{
    return unlikely(preempt_count() == preempt_offset &&
            tif_need_resched());
}
```
条件：preempt_count() == 0 且 tif_need_resched() 为真
- tif_need_resched()：检查当前任务的 TIF_NEED_RESCHED 标志
- 这个标志由调度器设置（如当更高优先级任务就绪时）
- 如果标志未设置，cond_resched() 不会触发调度


当然，内核配置条件（行为差异）
cond 还体现在根据内核配置有不同的行为：
 配置                                                         cond_resched() 的行为
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 !CONFIG_PREEMPTION（非抢占式）                               实际检查条件并可能调度
 CONFIG_PREEMPT_DYNAMIC（动态抢占）                           可通过运行时开关控制是否检查
 CONFIG_PREEMPTION && !CONFIG_PREEMPT_DYNAMIC（完全抢占式）   直接返回 0，无操作
在完全抢占式内核中，cond_resched() 是"无条件"不执行，因为内核本身已经是强制抢占了，不需要显式检查点。

4. 与 schedule() 的区别
 函数             行为
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 schedule()       无条件 让出 CPU，调用者必须确保可以睡眠
 cond_resched()   有条件 让出 CPU，只有上述条件都满足时才调度

总结
cond_resched() 的 cond 体现在：
1. 软件条件：TIF_NEED_RESCHED 标志必须设置
2. 中断必须启用
3. preempt_count 必须为 0（不在任何临界区）
5. 不在 rcu critical region 中
4. 配置条件：取决于内核抢占模式

### cond_resched 和 might_sleep 的区别是什么?
```txt
特性           cond_resched()              might_sleep()
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 主要目的       实际让出 CPU                调试检查（检查是否在原子上下文）
 行为           条件满足时调用 schedule()   检查是否安全睡眠，不安全则打印警告
 生产环境开销   有（实际检查调度条件）      无（调试配置下才有实际代码）
 使用场景       长循环中主动让出 CPU        可能睡眠的函数开头做声明/检查
```

## 下一次问题，nohz=full 到底意味着什么，如何解决的?
<!-- da9441c8-857a-45e4-bfa2-3fb5e0f1bd84 -->

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
