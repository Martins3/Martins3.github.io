## soft lockup
- https://www.kernel.org/doc/Documentation/lockup-watchdogs.txt
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_monitoring_and_updating_the_kernel/keeping-kernel-panic-parameters-disabled-in-virtualized-environments_managing-monitoring-and-updating-the-kernel#doc-wrapper

- soft lockup : 如果一个进程 20s 独占 CPU，让其他的 task 不能使用
- hard lockup : 如果一个进程 20s 独占 CPU，连中断都不可以进入

If any CPU in the system does not receive any hrtimer interrupt during that time the
'hardlockup detector' (the handler for the NMI perf event) will generate a kernel warning or call panic, depending on the configuration.


## 分布的文件

kernel/watchdog.c
kernel/watchdog_hld.c
driver/watchdog/

## [Softlockup detector and hardlockup detector (aka nmi_watchdog)](https://docs.kernel.org/admin-guide/lockup-watchdogs.html)

nmi_watchdog=panic

## 测试一个 softlockup 的

linux/kernel/watchdog.c 中定义的

使用 hrtimer 来检测
```txt
#0  __crash_kexec (regs=regs@entry=0x0 <fixed_percpu_data>) at kernel/kexec_core.c:1048
#1  0xffffffff81140a66 in panic (fmt=fmt@entry=0xffffffff82be7406 "softlockup: hung tasks") at kernel/panic.c:359
#2  0xffffffff81257664 in watchdog_timer_fn (hrtimer=<optimized out>) at kernel/watchdog.c:444
#3  0xffffffff81207a86 in __run_hrtimer (flags=6, now=0xffffc90000560f48, timer=0xffff888333a9fd20, base=0xffff888333a9f5c0, cpu_base=0xffff888333a9f580) at kernel/time/hrtimer.c:1685
#4  __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff888333a9f580, now=36242272464, flags=flags@entry=6, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#5  0xffffffff81208650 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#6  0xffffffff811092b0 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1095
#7  __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1112
#8  0xffffffff822a94d1 in sysvec_apic_timer_interrupt (regs=0xffffc9000206bd48) at arch/x86/kernel/apic/apic.c:1106
```

初始化:
- lockup_detector_setup
  - `__lockup_detector_reconfigure`
    - softlockup_start_all
      - softlockup_start_fn
        - softlockup_start_fn

周期性清理:
```txt
#0  softlockup_fn (data=0x0 <fixed_percpu_data>) at kernel/watchdog.c:346
#1  0xffffffff8124373b in cpu_stopper_thread (cpu=<optimized out>) at kernel/stop_machine.c:511
#2  0xffffffff811794ec in smpboot_thread_fn (data=0xffff8881003e0490) at kernel/smpboot.c:164
#3  0xffffffff8116e6d2 in kthread (_create=0xffff8881013db880) at kernel/kthread.c:379
#4  0xffffffff81002939 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```
才知道 stop 是最高优先级:
[What is the use of stop_sched_class in linux kernel](https://stackoverflow.com/questions/15399782/what-is-the-use-of-stop-sched-class-in-linux-kernel)

## 测试一下 hardlockup 的
kernel/watchdog_hld.c : hardlockup_detector_event_create 中注册 perf event 来

使用 perf event 来检测:
```txt
watchdog_overflow_callback+5
__perf_event_overflow+255
handle_pmi_common+401
intel_pmu_handle_irq+295
perf_event_nmi_handler+44
nmi_handle+95
default_do_nmi+107
exc_nmi+288
asm_exc_nmi+192
```

- [ ] 这个函数，guest 中不会调用
  - 这导致我现在无法在 guest 中验 hardlockup 了，不过也有道理，在虚拟化平台中，硬件中断不应该成为问题。

- watchdog_timer_fn
 - watchdog_interrupt_count : 来刷新 hardlockup ，告诉时钟中断还是来的
