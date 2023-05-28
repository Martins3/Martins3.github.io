## KVM_FEATURE_STEAL_TIME
1. `kvm_register_steal_time` : 通过 msr 寄存器告知
2. kvm_steal_clock
```txt
#0  kvm_steal_clock (cpu=18) at arch/x86/kernel/kvm.c:404
#1  0xffffffff811a82d4 in paravirt_steal_clock (cpu=<optimized out>) at ./arch/x86/include/asm/paravirt.h:45
#2  steal_account_process_time (maxtime=18446744073709551615) at kernel/sched/build_policy.c:4660
#3  account_process_tick (p=0xffff888100a21180, user_tick=user_tick@entry=0) at kernel/sched/build_policy.c:4901
#4  0xffffffff81206bd4 in update_process_times (user_tick=0) at kernel/time/timer.c:2069
#5  0xffffffff8121bf31 in tick_sched_handle (ts=ts@entry=0xffff888333a9fac0, regs=regs@entry=0xffffc90000137e38) at kernel/time/tick-sched.c:243
#6  0xffffffff8121c2f1 in tick_sched_timer (timer=0xffff888333a9fad0) at kernel/time/tick-sched.c:1481
#7  0xffffffff81207a86 in __run_hrtimer (flags=6, now=0xffffc90000560f48, timer=0xffff888333a9fad0, base=0xffff888333a9f5c0, cpu_base=0xffff888333a9f580) at kernel/time/hrtimer.c:1685
#8  __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff888333a9f580, now=11370726342, flags=flags@entry=6, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#9  0xffffffff81208650 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#10 0xffffffff811092b0 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1095
#11 __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1112
#12 0xffffffff822a94d1 in sysvec_apic_timer_interrupt (regs=0xffffc90000137e38) at arch/x86/kernel/apic/apic.c:1106
```

