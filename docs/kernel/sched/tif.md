# TIF
<!-- 998715b0-cd74-4b91-8a9d-1d68813cd5c9 -->

```c
/*
 * thread information flags
 * - these are process state flags that various assembly files
 *   may need to access
 */
#define TIF_NOTIFY_RESUME	1	/* callback before returning to user */
#define TIF_SIGPENDING		2	/* signal pending */
#define TIF_NEED_RESCHED	3	/* rescheduling necessary */
#define TIF_SINGLESTEP		4	/* reenable singlestep on user return*/
#define TIF_SSBD		5	/* Speculative store bypass disable */
#define TIF_SPEC_IB		9	/* Indirect branch speculation mitigation */
#define TIF_SPEC_L1D_FLUSH	10	/* Flush L1D on mm switches (processes) */
#define TIF_USER_RETURN_NOTIFY	11	/* notify kernel of userspace return */
#define TIF_UPROBE		12	/* breakpointed or singlestepping */
#define TIF_PATCH_PENDING	13	/* pending live patching update */
#define TIF_NEED_FPU_LOAD	14	/* load FPU on return to userspace */
#define TIF_NOCPUID		15	/* CPUID is not accessible in userland */
#define TIF_NOTSC		16	/* TSC is not accessible in userland */
#define TIF_NOTIFY_SIGNAL	17	/* signal notifications exist */
#define TIF_MEMDIE		20	/* is terminating due to OOM killer */
#define TIF_POLLING_NRFLAG	21	/* idle is polling for TIF_NEED_RESCHED */
#define TIF_IO_BITMAP		22	/* uses I/O bitmap */
#define TIF_SPEC_FORCE_UPDATE	23	/* Force speculation MSR update in context switch */
#define TIF_FORCED_TF		24	/* true if TF in eflags artificially */
#define TIF_BLOCKSTEP		25	/* set when we want DEBUGCTLMSR_BTF */
#define TIF_LAZY_MMU_UPDATES	27	/* task is updating the mmu lazily */
#define TIF_ADDR32		29	/* 32-bit address space on 64 bits */
```

## TIF_NEED_FPU_LOAD
<!-- e6b28c73-09f5-4165-ad3e-ab3e337e22d3 -->

把这个整理到 fpu 吧

不想细究了，不过看上去，除掉普通的 context switch ，kvm ，signal 机制
也是都需要考虑 fpu 状态恢复的。
```txt
@[
        fpu_swap_kvm_fpstate+5
        kvm_arch_vcpu_ioctl_run+92
        kvm_vcpu_ioctl+276
        __x64_sys_ioctl+147
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 71
@[
        fpu_swap_kvm_fpstate+5
        kvm_put_guest_fpu+23
        kvm_arch_vcpu_ioctl_run+343
        kvm_vcpu_ioctl+276
        __x64_sys_ioctl+147
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 71
```

```txt
@[
        fpregs_mark_activate+5
        fpu__clear_user_states+101
        handle_signal+201
        arch_do_signal_or_restart+165
        exit_to_user_mode_loop+144
        do_syscall_64+431
        entry_SYSCALL_64_after_hwframe+118
]: 63
```

## TIF_NEED_RESCHED 的语义
<!-- 5772ca56-c6ce-46f2-b53b-5d651eaffe8a -->

配套测试的代码 : code/src/m/sched/tif.c

基本原理就是，在时间中断中，会去判断是否该给**正在运行**的 thread 设置上 TIF_NEED_RESCHED 。
1. 如果是抢占内核，中断结束， 那么立刻开始调度。
2. 如果不是，只是设置上标志位，等程序 执行到 cond_resched() might_sleep() schedule() 等位置或者 syscall 返回的时候检查这个 flag。

- `do_syscall_64`
  - `syscall_enter_from_user_mode`
  - `syscall_exit_to_user_mode`
    - `__syscall_exit_to_user_mode_work`
      - `syscall_exit_to_user_mode_prepare`
        - `exit_to_user_mode_loop`
          - 在其中检测是否存在有 `TIF_NEED_RESCHED`，如果有，调用 schedule 函数
    - `__exit_to_user_mode`


set_tsk_need_resched 主要 rcu 调用的 ，看 __schedule 的注释，TIF_NEED_RESCHED 是在 sched_tick()
插上的，基本上正确的，不过不太理解为什么是放到 rcu 中的。

- asm_sysvec_call_function_single
  - sysvec_call_function_single
    - instr_sysvec_call_function_single
      - __sysvec_call_function_single
        - generic_smp_call_function_single_interrupt
          - __flush_smp_call_function_queue
            - csd_do_func
              - rcu_exp_handler
                - rcu_exp_need_qs
                  - set_tsk_need_resched

- asm_sysvec_apic_timer_interrupt
  - sysvec_apic_timer_interrupt
    - instr_sysvec_apic_timer_interrupt
      - __sysvec_apic_timer_interrupt
        - local_apic_timer_interrupt
          - hrtimer_interrupt
            - __hrtimer_run_queues
              - __run_hrtimer
                - tick_nohz_handler
                  - tick_sched_handle
                    - update_process_times
                      - rcu_sched_clock_irq
                        - set_tsk_need_resched


在执行完成 __schedule 的时候来调用 clear_tsk_need_resched
```txt
#0  clear_tsk_need_resched (tsk=<optimized out>) at ./include/linux/sched.h:2044
#1  __schedule (sched_mode=sched_mode@entry=-1) at kernel/sched/core.c:6912
#2  0xffffffff81cb5593 in schedule_idle () at kernel/sched/core.c:7084
#3  0xffffffff813548e6 in do_idle () at kernel/sched/idle.c:358
#4  0xffffffff81354c99 in cpu_startup_entry (state=state@entry=CPUHP_AP_ONLINE_IDLE) at kernel/sched/idle.c:428
#5  0xffffffff8129d969 in start_secondary (unused=<optimized out>) at arch/x86/kernel/smpboot.c:315
#6  0xffffffff8126b506 in secondary_startup_64 () at arch/x86/kernel/head_64.S:418
#7  0x0000000000000000 in ?? ()
```

need_resched() 调用者大概有

参考:
- https://stackoverflow.com/questions/9473301/are-there-any-difference-between-kernel-preemption-and-interrupt
- https://www.cnblogs.com/LoyenWang/p/12386281.html
- https://stackoverflow.com/questions/18578947/what-does-tif-need-resched-do

## TIF_SIGPENDING 和 TIF_NOTIFY_SIGNAL

显然，TIF_SIGPENDING 就是大名鼎鼎的 signal_pending() 机制的核心。
而 TIF_NOTIFY_SIGNAL 是为了 io uring 机制的

```c
static inline int task_sigpending(struct task_struct *p)
{
	return unlikely(test_tsk_thread_flag(p,TIF_SIGPENDING));
}

static inline int signal_pending(struct task_struct *p)
{
	/*
	 * TIF_NOTIFY_SIGNAL isn't really a signal, but it requires the same
	 * behavior in terms of ensuring that we break out of wait loops
	 * so that notify signal callbacks can be processed.
	 */
	if (unlikely(test_tsk_thread_flag(p, TIF_NOTIFY_SIGNAL)))
		return 1;
	return task_sigpending(p);
}
```

https://lore.kernel.org/lkml/160396741001.397.4239619005773354881.tip-bot2@tip-bot2/
```txt
commit 12db8b690010ccfadf9d0b49a1e1798e47dbbe1a
Author: Jens Axboe <axboe@kernel.dk>
Date:   Mon Oct 26 14:32:28 2020 -0600

    entry: Add support for TIF_NOTIFY_SIGNAL

    Add TIF_NOTIFY_SIGNAL handling in the generic entry code, which if set,
    will return true if signal_pending() is used in a wait loop. That causes an
    exit of the loop so that notify_signal tracehooks can be run. If the wait
    loop is currently inside a system call, the system call is restarted once
    task_work has been processed.

    In preparation for only having arch_do_signal() handle syscall restarts if
    _TIF_SIGPENDING isn't set, rename it to arch_do_signal_or_restart().  Pass
    in a boolean that tells the architecture specific signal handler if it
    should attempt to get a signal, or just process a potential syscall
    restart.

    For !CONFIG_GENERIC_ENTRY archs, add the TIF_NOTIFY_SIGNAL handling to
    get_signal(). This is done to minimize the needed architecture changes to
    support this feature.

    Signed-off-by: Jens Axboe <axboe@kernel.dk>
    Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
    Reviewed-by: Oleg Nesterov <oleg@redhat.com>
    Link: https://lore.kernel.org/r/20201026203230.386348-3-axboe@kernel.dk
```

> io_uring relies on a signal-like mechanism for
> interrupting in-kernel waits just like normal signals.
>
> Signals and threads are not happy partners.
> Support for all architectures was added for
> TIF_NOTIFY_SIGNAL, which decouples the signal
> interruption from the shared struct
> sighand_struct.

__set_notify_signal 只有 io uring 使用

```c
/*
 * Returns 'true' if kick_process() is needed to force a transition from
 * user -> kernel to guarantee expedient run of TWA_SIGNAL based task_work.
 */
static inline bool __set_notify_signal(struct task_struct *task)
{
	return !test_and_set_tsk_thread_flag(task, TIF_NOTIFY_SIGNAL) &&
	       !wake_up_state(task, TASK_INTERRUPTIBLE);
}
```

TIF_NOTIFY_SIGNAL 的作用不是“触发唤醒”，而是 “告知被唤醒的线程：你是因为有信号需要处理才被叫醒的”。

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
