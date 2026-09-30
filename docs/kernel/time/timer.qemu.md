# QEMU 中的时钟

<!-- vim-markdown-toc GitLab -->

* [time-meter](#time-meter)
* [timer](#timer)
* [timerlist](#timerlist)
* [timerlistgroup](#timerlistgroup)
* [misc](#misc)
* [为什么静止不动的 guest 有 2% 的 CPU 消耗啊](#为什么静止不动的-guest-有-2-的-cpu-消耗啊)
* [不知道为什么](#不知道为什么)

<!-- vim-markdown-toc -->

QEMU 中的 timer 需要完成两个事情，计时和定时。

[time-meter](#time-meter) 和
[timer](#timer) 简要分析 QEMU 如何实现计时器和定时器的功能。
因为 QEMU 有四个不同的时钟，相同的 timer 放到一个 [timerlist](#timerlist) 中。
一个事件监听 thread  需要持有一个 [timerlistgroup](#timerlistgroup)。

## time-meter
因为 guest 可能会停止，而 host 不会停止，所以会出现 guest 和 host 的 timer 会出现差异的，
然后因为的 replay 机制，实际上 timer 变得更加的复杂和奇怪了。

```c
/**
 * QEMUClockType:
 *
 * The following clock types are available:
 *
 * @QEMU_CLOCK_REALTIME: Real time clock
 *
 * The real time clock should be used only for stuff which does not
 * change the virtual machine state, as it runs even if the virtual
 * machine is stopped.
 *
 * @QEMU_CLOCK_VIRTUAL: virtual clock
 *
 * The virtual clock only runs during the emulation. It stops
 * when the virtual machine is stopped.
 *
 * @QEMU_CLOCK_HOST: host clock
 *
 * The host clock should be used for device models that emulate accurate
 * real time sources. It will continue to run when the virtual machine
 * is suspended, and it will reflect system time changes the host may
 * undergo (e.g. due to NTP).
 *
 * @QEMU_CLOCK_VIRTUAL_RT: realtime clock used for icount warp
 *
 * Outside icount mode, this clock is the same as @QEMU_CLOCK_VIRTUAL.
 * In icount mode, this clock counts nanoseconds while the virtual
 * machine is running.  It is used to increase @QEMU_CLOCK_VIRTUAL
 * while the CPUs are sleeping and thus not executing instructions.
 */

typedef enum {
    QEMU_CLOCK_REALTIME = 0,
    QEMU_CLOCK_VIRTUAL = 1,
    QEMU_CLOCK_HOST = 2,
    QEMU_CLOCK_VIRTUAL_RT = 3,
    QEMU_CLOCK_MAX
} QEMUClockType;
```


```c
int64_t qemu_clock_get_ns(QEMUClockType type)
{
    switch (type) {
    case QEMU_CLOCK_REALTIME:
        return get_clock(); // 调用到 clock_gettime 或者 get_clock_realtime, 取决于 CLOCK_MONOTONIC 选项是否打开
    default:
    case QEMU_CLOCK_VIRTUAL:
        if (use_icount) {
            return cpu_get_icount();
        } else {
            return cpu_get_clock();
        }
    case QEMU_CLOCK_HOST:
        return REPLAY_CLOCK(REPLAY_CLOCK_HOST, get_clock_realtime());
    case QEMU_CLOCK_VIRTUAL_RT:
        return REPLAY_CLOCK(REPLAY_CLOCK_VIRTUAL_RT, cpu_get_clock());
    }
}
```
- 如果没有 replay 的情况，后面两个 macro 都是可以直接退化为 get_clock_realtime 和 cpu_get_clock 的。
- cpu_get_clock 和 get_clock 区别在于 vm 停止之后是否计时
- get_clock_realtime 和 get_clock 的区别在于: 当 CLOCK_MONOTONIC 定义了的时候，get_clock 会去调用 clock_gettime 而不是 get_clock_realtime
  - clock_gettime 和 gettimeofday 的区别参看 [stackoverflow](https://stackoverflow.com/questions/12392278/measure-time-in-linux-time-vs-clock-vs-getrusage-vs-clock-gettime-vs-gettimeof)

- [x] timers_state.cpu_clock_offset 总是不变的
  - rnm, 太真实了，就是在 cpu_enable_ticks 中减去当时的 get_clock 的呀
- [x] 似乎退出的时候，会导致 timers_state.cpu_clock_offset 发生修改
  - 那是因为 cpu_disable_ticks 的原因

cpu_get_clock 的及时就是靠 cpu_disable_ticks 和 cpu_enable_ticks 实现的了

qemu_clock_enable 的调用位置:
- resume_all_vcpus
- qemu_tcg_rr_cpu_thread_fn : 因为单步调试的时候屏蔽中断和 timer 这是一个很正确的操作实际上。

qemu_clock_enable 的原理:
- 如果将 clock 装换为可以使用，立刻检查一下是否有 timer 结束了
- 如果 disable，那么自然需要等待这些 timer list 结束才可以的呀

## timer
因为 guest 需要周期性的注入时钟中断(apic / ioapic)，各种时钟设备(pit / hpet)设备的模拟，以及一些模拟设备的需求。
QEMU 需要实现定时器的功能。

下面是 timer_new 的调用位置:
- apic_realize
- ioapic_realize
- pci_std_vga_realize
- hpet_realize
- rtc_realizefn
- pit_realizefn
- serial_realize_core
- fdctrl_realize_common
- pci_e1000_realize
- ide_init1
- acpi_pm_tmr_init
- text_console_do_init
- gui_setup_refresh
- nvme_init_cq / nvme_init_sq

实现 timer 本来是使用 timer_create 系统调用的，但是在 [aio / timers: Remove alarm timers](https://github.com/qemu/qemu/commit/6d327171551a12b937c5718073b9848d0274c74d)
中，将 timer 机制和 QEMU 本身的事件监听机制合并到一起了。

在 main loop 中，大致的执行流程为:
- main_loop_wait
  - timerlistgroup_deadline_ns : 获取最近将会 timerout 时间
    - qemu_soonest_timeout
  - os_host_main_loop_wait
    - qemu_poll_ns
      - ppoll : 等待
  - qemu_clock_run_all_timers

在 main loop 中使用 ppoll(2) 来监听 fd，一旦 fd 事件到达(比如 socket 上有数据发送)，那么 ppoll 就可以返回了。
实际上，QEMU 监听 timer timeout 的机制和监听 fd 的机制合并在一起的。
对于 timer，让 main loop 从 ppoll 上返回的方法是设置其参数 timeout，这样，只要无论是 fd ready 还是 timer timeout 都会导致 ppoll 返回。
```c
    timeout_ns = qemu_soonest_timeout(timeout_ns,
                                      timerlistgroup_deadline_ns(
                                          &main_loop_tlg));
```

这是一个经典的处理 timer timeout hook 的路径:
```txt
#0  rtc_update_timer (opaque=0x55555669b400) at /home/maritns3/core/xqm/hw/rtc/mc146818rtc.c:427
#1  0x0000555555caea96 in timerlist_run_timers (timer_list=0x5555565d6470) at /home/maritns3/core/xqm/util/qemu-timer.c:595
#2  timerlist_run_timers (timer_list=0x5555565d6470) at /home/maritns3/core/xqm/util/qemu-timer.c:506
#3  0x0000555555caec90 in qemu_clock_run_timers (type=<optimized out>) at /home/maritns3/core/xqm/util/qemu-timer.c:695
#4  qemu_clock_run_all_timers () at /home/maritns3/core/xqm/util/qemu-timer.c:695
#5  0x0000555555caf0c1 in main_loop_wait (nonblocking=<optimized out>) at /home/maritns3/core/xqm/util/main-loop.c:525
#6  0x00005555559bbf59 in main_loop () at /home/maritns3/core/xqm/vl.c:1812
#7  0x000055555582b2f9 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4473
```
## timerlist
不同种类的 timer 上的时间的进度不同，为了方便管理，一个类型的 timer 都会插入到相同的
timerlist 上。

比如 timerlist_deadline_ns 就是扫描一个 timerlist 上的所有 timerout 的时间，从而计算出最近的 timeout 时间。

```c
/* A QEMUTimerList is a list of timers attached to a clock. More
 * than one QEMUTimerList can be attached to each clock, for instance
 * used by different AioContexts / threads. Each clock also has
 * a list of the QEMUTimerLists associated with it, in order that
 * reenabling the clock can call all the notifiers.
 */

struct QEMUTimerList {
    QEMUClock *clock;
    QemuMutex active_timers_lock;
    QEMUTimer *active_timers;
    QLIST_ENTRY(QEMUTimerList) list;
    QEMUTimerListNotifyCB *notify_cb;
    void *notify_opaque;

    /* lightweight method to mark the end of timerlist's running */
    QemuEvent timers_done_ev;
};
```
timers_done_ev 是为了防止一个 thread 正在执行 timerlist 的 timer 的 callback 的时候，
结果另一个 thread 却在 enable 或者 disable 其。


因为一旦监听到事件，那么就可以 poll / ppoll / iouring 上返回，然后就会去执行 qemu_clock_run_all_timers
通过 timerlist_notify 可以让立刻从 poll / ppoll / iouring 返回，因为在不同的 thread 中使用监听方法稍有不同。

```c
void timerlist_notify(QEMUTimerList *timer_list)
{
    if (timer_list->notify_cb) {
        timer_list->notify_cb(timer_list->notify_opaque, timer_list->clock->type);
    } else {
        // 似乎并没有人调用到此处
        qemu_notify_event();
    }
}
```

用于注册的 hook 为:
```c
void qemu_timer_notify_cb(void *opaque, QEMUClockType type);

void aio_notify(AioContext *ctx);
```

为什么存在立刻执行 qemu_clock_run_all_timers 的需求?
- 如果添加的 timer 是 soonest 的时候(分析 timer_mod_ns_locked)，这要求 ppoll 提前返回，否则等到 ppoll 返回的时候，这个 timer 要求的时间已经过去了

## timerlistgroup
```c
struct QEMUTimerListGroup {
    QEMUTimerList *tl[QEMU_CLOCK_MAX];
};
```

因为 iothread 的引入，QEMU 不仅仅在 main loop 使用 ppoll 等待，还有可能在 iothread 中来等待时钟。
在 [aio / timers: Split QEMUClock into QEMUClock and QEMUTimerList](https://github.com/qemu/qemu/commit/ff83c66eccf5b5f6b6530d504e3be41559250dcb)
创建出来了 QEMUTimerListGroup，一个事件监听 thread 持有一个 group。

创建一个 QEMUTimerListGroup 的时候， 对于每一个 QEMUClockType 创建一个 timerlist。

- 使用 QEMUTimerListGroup 的地方
  - timerlistgroup_run_timers
  - timerlistgroup_init
  - timerlistgroup_deadline_ns
  - timer_init_full

main_loop_tlg 是 main loop 的 QEMUTimerListGroup, 也是默认使用的。

[timer](#timer) 中列举了各种 timer_new 的调用位置，那些 timer 都是添加到 main_loop_tlg 上的
而添加到其他 QEMUTimerListGroup 的位置默认情况下只有:

```txt
>>> bt
#0  aio_timer_new (type=QEMU_CLOCK_VIRTUAL, scale=1000000, cb=0x555555bdf550 <cache_clean_timer_cb>, opaque=0x555556850630, ctx=0x5555565d75b0) at /home/maritns3/core/xqm/include/block/aio.h:433
#1  cache_clean_timer_init (bs=0x555556850630, context=0x5555565d75b0) at /home/maritns3/core/xqm/block/qcow2.c:828
#2  0x0000555555be2b72 in qcow2_update_options_commit (bs=0x555556850630, r=0x7fffe841fe20) at /home/maritns3/core/xqm/block/qcow2.c:1208
#3  0x0000555555be423c in qcow2_update_options (bs=0x555556850630, options=<optimized out>, flags=<optimized out>, errp=<optimized out>) at /home/maritns3/core/xqm/block/qcow2.c:1235
#4  0x0000555555be59e2 in qcow2_do_open (bs=0x555556850630, options=0x555556bda000, flags=139266, errp=0x7fffffffcbe0) at /home/maritns3/core/xqm/block/qcow2.c:1513
#5  0x0000555555be6626 in qcow2_open_entry (opaque=0x7fffffffcb80) at /home/maritns3/core/xqm/block/qcow2.c:1792
#6  0x0000555555cc7283 in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at /home/maritns3/core/xqm/util/coroutine-ucontext.c:115
#7  0x00007ffff5a6f660 in __start_context () at ../sysdeps/unix/sysv/linux/x86_64/__start_context.S:91
```

## misc
- 如果一个 Timer 被添加到了 timerlist 中，那么 QEMUTimer::expire_time 不应该等于 -1
- timer_mod_anticipate_ns 和 timer_mod_ns 的区别: 前者要求，只有提前这个 timer 的时候，才可以修改 timerlist，否则此次操作为空
- 因为 QEMU_CLOCK_VIRTUAL 类型的 clock 只有在 CPU 运行的时候才可以运行的

[^1]: https://airbus-seclab.github.io/qemu_blog/timers.html

> @question 是不是说: timespec 和 timeval 是 低精度的接口，还是用户态的接口，但是 ktime_t 是 hrtime 的接口

## 为什么静止不动的 guest 有 2% 的 CPU 消耗啊

```txt
     _start                                                                                                                                                          ▒
     __libc_start_main@@GLIBC_2.34                                                                                                                                   ▒
     __libc_start_call_main                                                                                                                                          ▒
     main                                                                                                                                                            ▒
     qemu_default_main                                                                                                                                               ▒
   - qemu_main_loop                                                                                                                                                  ▒
      - 95.46% main_loop_wait                                                                                                                                        ▒
         - 86.01% os_host_main_loop_wait (inlined)                                                                                                                   ▒
            - 68.14% qemu_poll_ns                                                                                                                                    ▒
               - ppoll (inlined)                                                                                                                                     ▒
               - ppoll                                                                                                                                               ▒
                  - 67.49% entry_SYSCALL_64_after_hwframe
                    - do_syscall_64                                                                                                                                 ▒
                        - 66.85% __x64_sys_ppoll                                                                                                                     ▒
                           - 66.11% do_sys_poll                                                                                                                      ▒
                              - 14.49% poll_freewait                                                                                                                 ▒
                                 - 6.57% remove_wait_queue                                                                                                           ▒
                                    - 5.20% _raw_spin_lock_irqsave                                                                                                   ▒
                                         0.92% trace_hardirqs_off                                                                                                    ▒
                                         0.86% preempt_count_add                                                                                                     ▒
                                      0.66% __list_del_entry_valid_or_report                                                                                         ▒
                                 - 4.32% fput                                                                                                                        ▒
                                      1.01% preempt_count_sub                                                                                                        ▒
                                 - 3.13% _raw_spin_unlock_irqrestore                                                                                                 ▒
                                      1.49% preempt_count_sub                                                                                                        ▒
                                    - 1.01% trace_hardirqs_on                                                                                                        ▒
                                         tracer_hardirqs_on                                                                                                          ▒
                              - 12.27% fput                                                                                                                          ▒
                                 - 3.24% preempt_count_sub                                                                                                           ▒
                                      0.63% trace_preempt_on                                                                                                         ▒
                                   1.44% preempt_count_add                                                                                                           ▒
                                   0.79% tracer_preempt_off                                                                                                          ▒
                                   0.77% trace_preempt_off                                                                                                           ▒
                              - 12.27% eventfd_poll                                                                                                                  ▒
                                 - 4.46% add_wait_queue                                                                                                              ▒
                                    - 3.39% _raw_spin_lock_irqsave                                                                                                   ▒
                                         0.57% trace_hardirqs_off                                                                                                    ▒
                                 - 2.61% _raw_spin_unlock_irqrestore                                                                                                 ▒
                                      1.41% preempt_count_sub                                                                                                        ▒
                                      0.70% trace_hardirqs_on                                                                                                        ▒
                                   2.47% __pollwait                                                                                                                  ▒
                              - 9.36% fdget                                                                                                                          ▒
                                   0.67% __rcu_read_unlock                                                                                                           ▒
                                   0.63% __rcu_read_lock                                                                                                             ▒
                              - 6.37% sock_poll                                                                                                                      ▒
                                 - 3.69% udp_poll                                                                                                                    ▒
                                    - datagram_poll                                                                                                                  ▒
                                       - 1.38% __pollwait                                                                                                            ▒
                                          - get_free_pages_noprof
                                    - alloc_pages_noprof                                                                                                       ▒
                                             - alloc_pages_mpol                                                                                                      ▒
                                             - __alloc_frozen_pages_noprof                                                                                           ▒
                                                  0.51% get_page_from_freelist                                                                                       ▒
                                       - 0.95% add_wait_queue                                                                                                        ▒
                                            0.74% _raw_spin_lock_irqsave                                                                                             ▒
                                   0.82% tcp_poll                                                                                                                    ▒
                              - 4.57% schedule_hrtimeout_range                                                                                                       ▒
                                 - 4.05% schedule                                                                                                                    ▒
                                    - 3.95% __schedule                                                                                                               ▒
                                       - 1.87% dequeue_task_fair                                                                                                     ▒
                                          - dequeue_entities                                                                                                         ▒
                                               1.21% dequeue_entity                                                                                                  ▒
                                               0.50% dl_server_stop                                                                                                  ▒
                                0.65% __kmalloc_noprof                                                                                                               ▒
                                0.64% signalfd_poll                                                                                                                  ▒
                                0.60% __check_object_size                                                                                                            ▒
            - 10.70% glib_pollfds_poll (inlined)                                                                                                                     ▒
               - 9.66% g_main_context_check                                                                                                                          ▒
                  - 9.57% g_main_context_check_unlocked                                                                                                              ▒
                     - 2.97% aio_ctx_check                                                                                                                           ▒
                          2.30% aio_pending                                                                                                                          ▒
                          0.57% timerlistgroup_deadline_ns                                                                                                           ▒
                     - 1.85% clock_gettime@@GLIBC_2.17                                                                                                               ▒
                          __vdso_clock_gettime                                                                                                                       ▒
                       0.83% g_source_iter_next                                                                                                                      ▒
               - 0.88% g_main_context_dispatch                                                                                                                       ▒
                  - g_main_context_dispatch_unlocked                                                                                                                 ▒
                       0.64% aio_ctx_dispatch                                                                                                                        ▒
            - 6.44% glib_pollfds_fill (inlined)                                                                                                                      ▒
               - 4.28% g_main_context_prepare                                                                                                                        ▒
                  - 4.19% g_main_context_prepare_unlocked                                                                                                            ▒
                     - 1.56% clock_gettime@@GLIBC_2.17                                                                                                               ▒
                          __vdso_clock_gettime                                                                                                                       ▒
                       0.63% g_source_iter_next                                                                                                                      ▒
               - 2.03% g_main_context_query                                                                                                                          ▒
                    1.98% g_main_context_query_unlocked
         - 5.96% qemu_clock_run_all_timers                                                                                                                           ▒
            - qemu_clock_run_timers (inlined)                                                                                                                        ▒
            - timerlist_run_timers (inlined)                                                                                                                         ▒
               - timerlist_run_timers (inlined)                                                                                                                      ▒
                  - 4.19% gsi_handler                                                                                                                                ▒
                     - 2.86% kvm_pic_set_irq                                                                                                                         ▒
                        - kvm_set_irq                                                                                                                                ▒
                           - kvm_vm_ioctl                                                                                                                            ▒
                              - 2.59% __GI___ioctl                                                                                                                   ▒
                                 - 2.26% entry_SYSCALL_64_after_hwframe                                                                                              ▒
                                    - do_syscall_64                                                                                                                  ▒
                                       - 2.08% __x64_sys_ioctl                                                                                                       ▒
                                          - 1.62% kvm_vm_ioctl                                                                                                       ▒
                                             - 1.36% kvm_vm_ioctl_irq_line                                                                                           ▒
                                                - kvm_set_irq                                                                                                        ▒
                                                   - 0.69% kvm_pic_set_irq                                                                                           ▒
                                                        0.62% pic_unlock                                                                                             ▒
                     - 1.23% kvm_ioapic_set_irq                                                                                                                      ▒
                        - 1.16% kvm_set_irq                                                                                                                          ▒
                           - kvm_vm_ioctl                                                                                                                            ▒
                           - __GI___ioctl                                                                                                                            ▒
                              - 1.01% entry_SYSCALL_64_after_hwframe                                                                                                 ▒
                                 - do_syscall_64                                                                                                                     ▒
                                    - 0.94% __x64_sys_ioctl                                                                                                          ▒
                                       - 0.79% kvm_vm_ioctl                                                                                                          ▒
                                          - 0.70% kvm_vm_ioctl_irq_line                                                                                              ▒
                                               kvm_set_irq                                                                                                           ▒
         - 2.65% notifier_list_notify                                                                                                                                ▒
            - 1.25% slirp_pollfds_fill                                                                                                                               ▒
               - net_slirp_add_poll                                                                                                                                  ▒
                    g_array_append_vals                                                                                                                              ▒
            - 1.15% slirp_pollfds_poll                                                                                                                               ▒
                 0.59% net_slirp_get_revents                                                                                                                         ▒
         - 0.71% timerlistgroup_deadline_ns                                                                                                                          ▒
              timerlist_deadline_ns
```
的确，调用太多 ppoll 了:
```txt
🧀  sudo syscount -i 10
Tracing syscalls, printing top 10... Ctrl+C to quit.
[00:09:29]
SYSCALL                   COUNT
ppoll                     20788
ioctl                     20488
write                      8291
read                       6311
recvfrom                   5445
futex                      5135
openat                     2459
epoll_wait                 2305
close                      1975
recvmsg                    1575
```
问题是 ioctl 也很多啊

使用 gdb 可以容易的找到这个东西，很显然，这个是不对的:
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - qemu_clock_run_all_timers
          - qemu_clock_run_timers
            - timerlist_run_timers
              - timerlist_run_timers
                - gsi_handler
                  - kvm_pic_set_irq
                    - kvm_set_irq
                      - kvm_vm_ioctl
                        - ioctl

可以用其他的环境做一下对比一下。

从这里观察，可以看到 irq = 0
```c
int kvm_set_irq(KVMState *s, int irq, int level)
{
    struct kvm_irq_level event;
    int ret;

    assert(kvm_async_interrupts_enabled());

    event.level = level;
    event.irq = irq;
    ret = kvm_vm_ioctl(s, s->irq_set_ioctl, &event);
    if (ret < 0) {
        perror("kvm_set_irq");
        abort();
    }

    return (s->irq_set_ioctl == KVM_IRQ_LINE) ? 1 : event.status;
}
```

可以观察到 gsi 一直都是 0 的:
```txt
 qemu-system-x86  334043 [008] 86636.413609: kvm:kvm_set_irq: gsi 0 level 0 source 0
 qemu-system-x86  334043 [008] 86636.413609: kvm:kvm_set_irq: gsi 0 level 0 source 0
 qemu-system-x86  330541 [010] 86636.413644: kvm:kvm_set_irq: gsi 0 level 0 source 0
 qemu-system-x86  330541 [010] 86636.413645: kvm:kvm_set_irq: gsi 0 level 0 source 0
 qemu-system-x86  334043 [008] 86636.414608: kvm:kvm_set_irq: gsi 0 level 0 source 0
 qemu-system-x86  334043 [008] 86636.414608: kvm:kvm_set_irq: gsi 0 level 0 source 0
 qemu-system-x86  330541 [010] 86636.414645: kvm:kvm_set_irq: gsi 0 level 0 source 0
 qemu-system-x86  330541 [010] 86636.414645: kvm:kvm_set_irq: gsi 0 level 0 source 0
```

也可以这个来统计:
```sh
sudo bpftrace -e 'tracepoint:kvm:kvm_set_irq { @[args->gsi]=count() }'
```

通过 cat /proc/interrupts 的统计来看，这个中断都是被忽视掉了。


此外一个问题，当 guest 无事可做的时候，vCPU thread 是在内核态的。
所以，main loop thread 按道理无事可做才对啊。

```txt
SYSCALL                   COUNT
read                      10016
openat                     6111
close                      5261
fstat                      3744
fcntl                      3382
getdents64                 1294
write                       247
epoll_pwait                 211
epoll_wait                  152
futex                       137
```

结果是由于 net_slirp_poll_notify 导致的吗?

```txt
@[
    __free_pages+5
    poll_freewait+133
    do_sys_poll+1450
    __se_sys_ppoll+291
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 10840
```

先使用 vmtest 作为对比，发现也有这个问题，
检查 timer_init_full 注册的 callback ，一会就可以找到是 hpet_timer 了。
去掉之后，qemu 就基本上看不到了。

继续思考，为什么 guest kernel 可以忽视掉 hpet 的中断?




## 不知道为什么

为什么 aio_poll 中， 获取时间占用了 20% 的时间:
```txt
     thread_start
     start_thread
   - qemu_thread_start
      - 43.12% iothread_run
         - 43.09% aio_poll
            + 20.52% qemu_clock_get_ns
            - 9.97% virtio_queue_host_notifier_aio_poll
               - virtio_queue_empty
                    0.68% get_ptr_rcu_reader
            + 3.79% aio_bh_poll
            - 3.25% aio_dispatch_handler
               - 2.90% virtio_queue_notify_vq.part.0
                  - 2.87% virtio_blk_handle_vq
                     - 1.51% virtqueue_split_pop
                          0.67% vring_split_desc_read
                     - 0.56% virtio_blk_submit_multireq
                          0.54% blk_aio_prwv
            + 1.10% fdmon_poll_wait
              0.66% aio_poll_disabled
```

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
