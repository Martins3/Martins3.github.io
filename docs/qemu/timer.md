## how to port clock
实际上，timer 的监听就是靠 mian loop 中处理的

- [ ] 为什么制作出来了 timer_list_group 的概念
  - timerlist_run_timers
- [ ] 全局变量的 rtc_clock 正确初始化，其中影响是什么?
  - 在 rtc_configure 的地方初始化的
- 为什么搞出来四种 clock 的呀
  - QEMU_CLOCK_VIRTUAL_RT 和 icount 有关，暂时不管了
  - [ ] 主要是 QEMU_CLOCK_HOST 和 QEMU_CLOCK_REALTIME

- [ ] 似乎是 TimersState 主要处理什么的
  - [ ] cpu_get_clock 和 cpu_get_ticks cpu_get_icount 是一个高度对称的

提交之前:
- [ ] 写一个 blog
- [ ] 清理掉所有的 PORT_RTC

## 构建的层次关系
读读这个注释吧:
```diff
History:        #0
Commit:         ff83c66eccf5b5f6b6530d504e3be41559250dcb
Author:         Alex Bligh <alex@alex.org.uk>
Committer:      Stefan Hajnoczi <stefanha@redhat.com>
Author Date:    Wed 21 Aug 2013 11:02:46 PM CST
Committer Date: Fri 23 Aug 2013 01:10:27 AM CST

aio / timers: Split QEMUClock into QEMUClock and QEMUTimerList

Split QEMUClock into QEMUClock and QEMUTimerList so that we can
have more than one QEMUTimerList associated with the same clock.

Introduce a main_loop_timerlist concept and make existing
qemu_clock_* calls that actually should operate on a QEMUTimerList
call the relevant QEMUTimerList implementations, using the clock's
default timerlist. This vastly reduces the invasiveness of this
change and means the API stays constant for existing users.

Introduce a list of QEMUTimerLists associated with each clock
so that reenabling the clock can cause all the notifiers
to be called. Note the code to do the notifications is added
in a later patch.

Switch QEMUClockType to an enum. Remove global variables vm_clock,
host_clock and rt_clock and add compatibility defines. Do not
fix qemu_next_alarm_deadline as it's going to be deleted.

Add qemu_clock_use_for_deadline to indicate whether a particular
clock should be used for deadline calculations. When use_icount
is true, vm_clock should not be used for deadline calculations
as it does not contain a nanosecond count. Instead, icount
timeouts come from the execution thread doing aio_notify or
qemu_notify as appropriate. This function is used in the next
patch.

Signed-off-by: Alex Bligh <alex@alex.org.uk>
Signed-off-by: Stefan Hajnoczi <stefanha@redhat.com>
```

```c
struct QEMUTimerListGroup {
    QEMUTimerList *tl[QEMU_CLOCK_MAX];
};
```

将 QEMUClock 和 timerlist 关联在一起，从而可以
> Introduce a list of QEMUTimerLists associated with each clock
> so that reenabling the clock can cause all the notifiers
> to be called.

这很有道理，因为 QEMUClock 本身持有所有的 timerlist 无论是在哪一个线程中间。
只要 vCPU 停止下来了，都是需要停下来的。

而 timerlistgroup_deadline_ns 就是和每一个 group 关联的呀。

- [x] 调查一下 qemu_clock_enable 的使用位置
1. 为什么只是有 qemu_tcg_rr_cpu_thread_fn 需要这个东西呀
```c
/*
#0  qemu_clock_enable (type=type@entry=QEMU_CLOCK_VIRTUAL, enabled=true) at /home/maritns3/core/xqm/util/qemu-timer.c:156
#1  0x0000555555878b88 in qemu_tcg_rr_cpu_thread_fn (arg=arg@entry=0x5555567c21c0) at /home/maritns3/core/xqm/cpus.c:1566
#2  0x0000555555cb2953 in qemu_thread_start (args=<optimized out>) at /home/maritns3/core/xqm/util/qemu-thread-posix.c:519
#3  0x00007ffff5c0c609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#4  0x00007ffff5b33293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```
2. resume_all_vcpus

### QEMUTimerListGroup
```c
QEMUTimerListGroup main_loop_tlg;
static QEMUClock qemu_clocks[QEMU_CLOCK_MAX];
```
- [x] 是不是一共就是只有 4 个 clock 的: 是的，从 qemu_clock_ptr 中就是可以看出来的
  - [ ] 对于 rr 的 tcg 是如何模拟多个 cpu 的

一个 QEMUClock 会持有属于该类型的所有的 timerlist
  - timerlist_new 的时候就是通过调用 qemu_clock_ptr 获取 QEMUClock

timer_init_full 默认使用 main_loop_tlg 的，但是下面的调用路径下，还有一种可能的:
```c
/*
#1  timer_init_full (ts=ts@entry=0x55555669a480, timer_list_group=timer_list_group@entry=0x5555565d6688, type=type@entry=QEMU_CLOCK_VIRTUAL, scale=scale@entry=1000000,
 attributes=attributes@entry=0, cb=cb@entry=0x555555bdf250 <cache_clean_timer_cb>, opaque=0x555556815350) at /home/maritns3/core/xqm/util/qemu-timer.c:368
#2  0x0000555555bdf1f3 in timer_new_full (opaque=0x555556815350, cb=0x555555bdf250 <cache_clean_timer_cb>, attributes=0, scale=1000000, type=QEMU_CLOCK_VIRTUAL, timer_
list_group=0x5555565d6688) at /home/maritns3/core/xqm/include/qemu/timer.h:531
#3  aio_timer_new (opaque=0x555556815350, cb=0x555555bdf250 <cache_clean_timer_cb>, scale=1000000, type=QEMU_CLOCK_VIRTUAL, ctx=0x5555565d65b0) at /home/maritns3/core/
xqm/include/block/aio.h:432
#4  cache_clean_timer_init (bs=0x555556815350, context=0x5555565d65b0) at /home/maritns3/core/xqm/block/qcow2.c:828
#5  0x0000555555be1a02 in qcow2_update_options_commit (bs=bs@entry=0x555556815350, r=r@entry=0x7fffe841fe20) at /home/maritns3/core/xqm/block/qcow2.c:1208
#6  0x0000555555be3e5c in qcow2_update_options (bs=bs@entry=0x555556815350, options=options@entry=0x555556bd0c00, flags=flags@entry=139266, errp=errp@entry=0x7fffffffc
cc0) at /home/maritns3/core/xqm/block/qcow2.c:1235
#7  0x0000555555be5602 in qcow2_do_open (bs=0x555556815350, options=0x555556bd0c00, flags=139266, errp=0x7fffffffccc0) at /home/maritns3/core/xqm/block/qcow2.c:1513
#8  0x0000555555be6246 in qcow2_open_entry (opaque=0x7fffffffcc60) at /home/maritns3/core/xqm/block/qcow2.c:1792
#9  0x0000555555cc6d53 in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at /home/maritns3/core/xqm/util/coroutine-ucontext.c:115
#10 0x00007ffff5a6f660 in __start_context () at ../sysdeps/unix/sysv/linux/x86_64/__start_context.S:91
```

```c
AioContext *qemu_get_aio_context(void)
{
    return qemu_aio_context;
}
```

- 使用 QEMUTimerListGroup 的地方
  - timerlistgroup_run_timers
  - timerlistgroup_init
  - timerlistgroup_deadline_ns
  - timer_init_full

- 添加 timerlist 到 QEMUTimerListGroup，timerlist 是在创建 QEMUTimerListGroup 的时候自动创建
  - timerlistgroup_init : aio_context_new 的时候调用
  - qemu_clock_init


```c
/*
#0  timerlistgroup_run_timers (tlg=tlg@entry=0x5555565d7688) at /home/maritns3/core/xqm/util/qemu-timer.c:630
#1  0x0000555555cb0858 in aio_poll (ctx=ctx@entry=0x5555565d75b0, blocking=blocking@entry=true) at /home/maritns3/core/xqm/util/aio-posix.c:736
#2  0x0000555555be67cd in qcow2_open (bs=0x5555567b33f0, options=<optimized out>, flags=<optimized out>, errp=<optimized out>) at /home/maritns3/core/xqm/block/qcow2.c
:1823
#3  0x0000555555bbda16 in bdrv_open_driver (bs=bs@entry=0x5555567b33f0, drv=drv@entry=0x5555563c0fa0 <bdrv_qcow2>, node_name=<optimized out>, options=options@entry=0x5
555567ab000, open_flags=139266, errp=errp@entry=0x7fffffffcc88) at /home/maritns3/core/xqm/block.c:1295
#4  0x0000555555bc1994 in bdrv_open_common (errp=0x7fffffffcc88, options=0x5555567ab000, file=0x5555568c6d60, bs=0x5555567b33f0) at /home/maritns3/core/xqm/block.c:155
3
#5  bdrv_open_inherit (filename=<optimized out>, filename@entry=0x555556778f10 "/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2", reference=reference@entry=0x0
, options=0x5555567ab000, options@entry=0x55555685c940, flags=<optimized out>, flags@entry=0, parent=parent@entry=0x0, child_role=child_role@entry=0x0, errp=0x7fffffff
cf50) at /home/maritns3/core/xqm/block.c:3103
#6  0x0000555555bc2755 in bdrv_open (filename=filename@entry=0x555556778f10 "/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2", reference=reference@entry=0x0, o
ptions=options@entry=0x55555685c940, flags=flags@entry=0, errp=errp@entry=0x7fffffffcf50) at /home/maritns3/core/xqm/block.c:3196
#7  0x0000555555c0ab06 in blk_new_open (filename=filename@entry=0x555556778f10 "/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2", reference=reference@entry=0x0
, options=options@entry=0x55555685c940, flags=flags@entry=0, errp=errp@entry=0x7fffffffcf50) at /home/maritns3/core/xqm/block/block-backend.c:397
#8  0x00005555559af5a3 in blockdev_init (file=file@entry=0x555556778f10 "/home/maritns3/core/vn/hack/qemu/x64-e1000/alpine.qcow2", bs_opts=bs_opts@entry=0x55555685c940
, errp=errp@entry=0x7fffffffcf50) at /home/maritns3/core/xqm/blockdev.c:604
#9  0x00005555559b0419 in drive_new (all_opts=<optimized out>, block_default_type=<optimized out>, errp=0x555556424eb0 <error_fatal>) at /home/maritns3/core/xqm/blockd
ev.c:996
#10 0x00005555559bb316 in drive_init_func (opaque=<optimized out>, opts=<optimized out>, errp=<optimized out>) at /home/maritns3/core/xqm/vl.c:1144
#11 0x0000555555cc2172 in qemu_opts_foreach (list=<optimized out>, func=0x5555559bb300 <drive_init_func>, opaque=0x55555652a348, errp=0x555556424eb0 <error_fatal>) at
/home/maritns3/core/xqm/util/qemu-option.c:1170
#12 0x000055555582a9b0 in configure_blockdev (snapshot=0, machine_class=0x55555652a290, bdo_queue=0x7fffffffd0d0) at /home/maritns3/core/xqm/vl.c:1211
#13 main (argc=<optimized out>, argv=0x7fffffffd258, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4157
```
- 我感觉，在 main loop 中调用 aio_poll 只是因为 aio_poll 的功能过于复杂，在 iothread 中，aio_poll 调用 timerlistgroup_run_timers 的行为就和 main 的行为相同的了。

实际上， aio_poll 也会调用 aio_compute_timeout -> timerlistgroup_deadline_ns 来分析的
### QEMUTimerList

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

```c
/* Disabling the clock will wait for related timerlists to stop
 * executing qemu_run_timers.  Thus, this functions should not
 * be used from the callback of a timer that is based on @clock.
 * Doing so would cause a deadlock.
 *
 * Caller should hold BQL.
 */
void qemu_clock_enable(QEMUClockType type, bool enabled)
```

```c
typedef struct QEMUClock {
    /* We rely on BQL to protect the timerlists */
    QLIST_HEAD(, QEMUTimerList) timerlists;

    QEMUClockType type;
    bool enabled;
} QEMUClock;
```

调用 timerlist_new 的位置:
- timerlistgroup_init
- qemu_clock_init : 对于


- [x] qemu_notify_event
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

这就是 notify_cb 了吧！
```c
void qemu_timer_notify_cb(void *opaque, QEMUClockType type)
{
    if (!use_icount || type != QEMU_CLOCK_VIRTUAL) {
        qemu_notify_event();
        return;
    }

    // 因为没有使用 icount，所以不会向下走了
    if (qemu_in_vcpu_thread()) {
        /* A CPU is currently running; kick it back out to the
         * tcg_cpu_exec() loop so it will recalculate its
         * icount deadline immediately.
         */
        qemu_cpu_kick(current_cpu);
    } else if (first_cpu) {
        /* qemu_cpu_kick is not enough to kick a halted CPU out of
         * qemu_tcg_wait_io_event.  async_run_on_cpu, instead,
         * causes cpu_thread_is_idle to return false.  This way,
         * handle_icount_deadline can run.
         * If we have no CPUs at all for some reason, we don't
         * need to do anything.
         */
        async_run_on_cpu(first_cpu, do_nothing, RUN_ON_CPU_NULL);
    }
}
```

## timer
在 aio 只是添加了这一个 timer 而已:
```c
/*
>>> bt
#0  aio_timer_new (type=QEMU_CLOCK_VIRTUAL, scale=1000000, cb=0x555555bdf550 <cache_clean_timer_cb>, opaque=0x555556850630, ctx=0x5555565d75b0) at /home/maritns3/core/
xqm/include/block/aio.h:433
#1  cache_clean_timer_init (bs=0x555556850630, context=0x5555565d75b0) at /home/maritns3/core/xqm/block/qcow2.c:828
#2  0x0000555555be2b72 in qcow2_update_options_commit (bs=0x555556850630, r=0x7fffe841fe20) at /home/maritns3/core/xqm/block/qcow2.c:1208
#3  0x0000555555be423c in qcow2_update_options (bs=0x555556850630, options=<optimized out>, flags=<optimized out>, errp=<optimized out>) at /home/maritns3/core/xqm/blo
ck/qcow2.c:1235
#4  0x0000555555be59e2 in qcow2_do_open (bs=0x555556850630, options=0x555556bda000, flags=139266, errp=0x7fffffffcbe0) at /home/maritns3/core/xqm/block/qcow2.c:1513
#5  0x0000555555be6626 in qcow2_open_entry (opaque=0x7fffffffcb80) at /home/maritns3/core/xqm/block/qcow2.c:1792
#6  0x0000555555cc7283 in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at /home/maritns3/core/xqm/util/coroutine-ucontext.c:115
#7  0x00007ffff5a6f660 in __start_context () at ../sysdeps/unix/sysv/linux/x86_64/__start_context.S:91
```

```c
/*
#0  timer_init_full (ts=ts@entry=0x5555566b4980, timer_list_group=timer_list_group@entry=0x0, type=type@entry=QEMU_CLOCK_VIRTUAL_RT, scale=scale@entry=1, attributes=at
tributes@entry=0, cb=cb@entry=0x555555876c80 <cpu_throttle_timer_tick>, opaque=0x0) at /home/maritns3/core/xqm/util/qemu-timer.c:366
#1  0x0000555555877d42 in timer_new_full (opaque=0x0, cb=0x555555876c80 <cpu_throttle_timer_tick>, attributes=0, scale=1, type=QEMU_CLOCK_VIRTUAL_RT, timer_list_group=
0x0) at /home/maritns3/core/xqm/include/qemu/timer.h:531
#2  timer_new (opaque=0x0, cb=0x555555876c80 <cpu_throttle_timer_tick>, scale=1, type=QEMU_CLOCK_VIRTUAL_RT) at /home/maritns3/core/xqm/include/qemu/timer.h:551
#3  timer_new_ns (opaque=0x0, cb=0x555555876c80 <cpu_throttle_timer_tick>, type=QEMU_CLOCK_VIRTUAL_RT) at /home/maritns3/core/xqm/include/qemu/timer.h:569
#4  cpu_ticks_init () at /home/maritns3/core/xqm/cpus.c:866
#5  0x000055555582abf6 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4254


#0  timer_init_full (ts=ts@entry=0x55555678ea00, timer_list_group=timer_list_group@entry=0x0, type=type@entry=QEMU_CLOCK_VIRTUAL, scale=scale@entry=1000000, attributes
=attributes@entry=1, cb=cb@entry=0x555555cde930 <ra_timer_handler>, opaque=0x55555671ee10) at /home/maritns3/core/xqm/util/qemu-timer.c:366
#1  0x0000555555b6809d in timer_new_full (opaque=0x55555671ee10, cb=0x555555cde930 <ra_timer_handler>, attributes=1, scale=1000000, type=QEMU_CLOCK_VIRTUAL, timer_list
_group=0x0) at /home/maritns3/core/xqm/include/qemu/timer.h:531
#2  net_slirp_timer_new (cb=0x555555cde930 <ra_timer_handler>, cb_opaque=0x55555671ee10, opaque=<optimized out>) at /home/maritns3/core/xqm/net/slirp.c:180
#3  0x0000555555cde446 in icmp6_init (slirp=slirp@entry=0x55555671ee10) at /home/maritns3/core/xqm/slirp/src/ip6_icmp.c:30
#4  0x0000555555cde289 in ip6_init (slirp=slirp@entry=0x55555671ee10) at /home/maritns3/core/xqm/slirp/src/ip6_input.c:16
#5  0x0000555555cd0780 in slirp_new (cfg=cfg@entry=0x7fffffffca00, callbacks=callbacks@entry=0x5555562b8460 <slirp_cb>, opaque=opaque@entry=0x555556949010) at /home/ma
ritns3/core/xqm/slirp/src/slirp.c:304
#6  0x0000555555cd0b24 in slirp_init (restricted=restricted@entry=0, in_enabled=in_enabled@entry=true, vnetwork=..., vnetmask=..., vhost=..., in6_enabled=in6_enabled@e
ntry=true, vprefix_addr6=..., vprefix_len=64 '@', vhost6=..., vhostname=0x0, tftp_server_name=0x0, tftp_path=0x0, bootfile=0x0, vdhcp_start=..., vnameserver=..., vname
server6=..., vdnssearch=0x0, vdomainname=0x0, callbacks=0x5555562b8460 <slirp_cb>, opaque=0x555556949010) at /home/maritns3/core/xqm/slirp/src/slirp.c:376
#7  0x0000555555b6926f in net_slirp_init (model=0x555555e6e5bc "user", errp=0x7fffffffcf10, tftp_server_name=0x0, vdomainname=0x0, dnssearch=0x0, vsmbserver=0x0, smb_e
xport=0x0, vnameserver6=0x0, vnameserver=<optimized out>, vdhcp_start=0x0, bootfile=0x0, tftp_export=0x0, vhostname=0x0, vhost6=0x0, vprefix6_len=64, vprefix6=<optimiz
ed out>, ipv6=<optimized out>, vhost=<optimized out>, vnetwork=<optimized out>, ipv4=<optimized out>, restricted=0, name=0x0, peer=0x5555566e5740) at /home/maritns3/co
re/xqm/net/slirp.c:562
#8  net_init_slirp (netdev=<optimized out>, name=0x0, peer=0x5555566e5740, errp=0x7fffffffcf10) at /home/maritns3/core/xqm/net/slirp.c:1116
#9  0x0000555555b5f06b in net_client_init1 (object=<optimized out>, is_netdev=<optimized out>, errp=0x7fffffffcf10) at /home/maritns3/core/xqm/net/net.c:1055
#10 0x0000555555b5f6ff in net_client_init (opts=0x55555678ee70, is_netdev=<optimized out>, errp=0x7fffffffd0c0) at /home/maritns3/core/xqm/net/net.c:1155
#11 0x0000555555cc2152 in qemu_opts_foreach (list=<optimized out>, func=func@entry=0x555555b5f7b0 <net_init_client>, opaque=opaque@entry=0x0, errp=errp@entry=0x7ffffff
fd0c0) at /home/maritns3/core/xqm/util/qemu-option.c:1170
#12 0x0000555555b61bcf in net_init_clients (errp=0x7fffffffd0c0) at /home/maritns3/core/xqm/net/net.c:1576
#13 0x000055555582ac4b in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4276

#0  timer_init_full (ts=ts@entry=0x5555566e5a60, timer_list_group=timer_list_group@entry=0x0, type=type@entry=QEMU_CLOCK_VIRTUAL, scale=scale@entry=1, attributes=attri
butes@entry=0, cb=cb@entry=0x5555558cec30 <apic_timer>, opaque=0x555556675980) at /home/maritns3/core/xqm/util/qemu-timer.c:366
#1  0x00005555558ce303 in timer_new_full (opaque=0x555556675980, cb=0x5555558cec30 <apic_timer>, attributes=0, scale=1, type=QEMU_CLOCK_VIRTUAL, timer_list_group=0x0)
at /home/maritns3/core/xqm/include/qemu/timer.h:531
#2  timer_new (type=QEMU_CLOCK_VIRTUAL, scale=1, cb=0x5555558cec30 <apic_timer>, opaque=0x555556675980) at /home/maritns3/core/xqm/include/qemu/timer.h:551
#3  timer_new_ns (opaque=0x555556675980, cb=0x5555558cec30 <apic_timer>, type=QEMU_CLOCK_VIRTUAL) at /home/maritns3/core/xqm/include/qemu/timer.h:569
#4  apic_realize (dev=<optimized out>, errp=0x7fffffffcbc0) at /home/maritns3/core/xqm/hw/intc/apic.c:897

#0  timer_init_full (ts=ts@entry=0x5555569c4e70, timer_list_group=timer_list_group@entry=0x0, type=type@entry=QEMU_CLOCK_VIRTUAL, scale=scale@entry=1, attributes=attri
butes@entry=0, cb=cb@entry=0x5555558d0c80 <delayed_ioapic_service_cb>, opaque=0x555556726080) at /home/maritns3/core/xqm/util/qemu-timer.c:366
#1  0x00005555558d10b6 in timer_new_full (opaque=0x555556726080, cb=0x5555558d0c80 <delayed_ioapic_service_cb>, attributes=0, scale=1, type=QEMU_CLOCK_VIRTUAL, timer_l
ist_group=0x0) at /home/maritns3/core/xqm/include/qemu/timer.h:531
#2  timer_new (type=QEMU_CLOCK_VIRTUAL, scale=1, cb=0x5555558d0c80 <delayed_ioapic_service_cb>, opaque=0x555556726080) at /home/maritns3/core/xqm/include/qemu/timer.h:551
#3  timer_new_ns (opaque=0x555556726080, cb=0x5555558d0c80 <delayed_ioapic_service_cb>, type=QEMU_CLOCK_VIRTUAL) at /home/maritns3/core/xqm/include/qemu/timer.h:569
#4  ioapic_realize (dev=0x555556726080, errp=<optimized out>) at /home/maritns3/core/xqm/hw/intc/ioapic.c:444
#5  0x0000555555a74440 in ioapic_common_realize (dev=0x555556726080, errp=0x7fffffffcd40) at /home/maritns3/core/xqm/hw/intc/ioapic_common.c:164

#0  timer_init_full (ts=ts@entry=0x5555569f4370, timer_list_group=timer_list_group@entry=0x0, type=type@entry=QEMU_CLOCK_REALTIME, scale=scale@entry=1000000, attribute
s=attributes@entry=0, cb=cb@entry=0x555555b87290 <text_console_update_cursor>, opaque=0x0) at /home/maritns3/core/xqm/util/qemu-timer.c:366
#1  0x0000555555b88470 in timer_new_full (opaque=0x0, cb=0x555555b87290 <text_console_update_cursor>, attributes=0, scale=1000000, type=QEMU_CLOCK_REALTIME, timer_list
_group=0x0) at /home/maritns3/core/xqm/include/qemu/timer.h:531
#2  timer_new (type=QEMU_CLOCK_REALTIME, scale=1000000, opaque=0x0, cb=0x555555b87290 <text_console_update_cursor>) at /home/maritns3/core/xqm/include/qemu/timer.h:551
#3  timer_new_ms (opaque=0x0, cb=0x555555b87290 <text_console_update_cursor>, type=QEMU_CLOCK_REALTIME) at /home/maritns3/core/xqm/include/qemu/timer.h:605
#4  get_alloc_displaystate () at /home/maritns3/core/xqm/ui/console.c:1841
#5  0x0000555555b8b843 in graphic_console_init (dev=0x55555724a640, head=head@entry=0, hw_ops=hw_ops@entry=0x555556108d20 <vga_ops>, opaque=opaque@entry=0x55555724af20
) at /home/maritns3/core/xqm/ui/console.c:1895
#6  0x0000555555a5554a in pci_std_vga_realize (dev=0x55555724a640, errp=<optimized out>) at /home/maritns3/core/xqm/hw/display/vga-pci.c:245
#7  0x0000555555abaefb in pci_qdev_realize (qdev=0x55555724a640, errp=<optimized out>) at /home/maritns3/core/xqm/hw/pci/pci.c:2099

#0  timer_init_full (ts=ts@entry=0x555556a0be70, timer_list_group=timer_list_group@entry=0x0, type=type@entry=QEMU_CLOCK_VIRTUAL, scale=scale@entry=1, attributes=attri
butes@entry=0, cb=cb@entry=0x555555af11e0 <hpet_timer>, opaque=0x555556801e38) at /home/maritns3/core/xqm/util/qemu-timer.c:366
#1  0x0000555555af1d50 in timer_new_full (opaque=0x555556801e38, cb=0x555555af11e0 <hpet_timer>, attributes=0, scale=1, type=QEMU_CLOCK_VIRTUAL, timer_list_group=0x0)
at /home/maritns3/core/xqm/include/qemu/timer.h:531
#2  timer_new (type=QEMU_CLOCK_VIRTUAL, scale=1, cb=0x555555af11e0 <hpet_timer>, opaque=0x555556801e38) at /home/maritns3/core/xqm/include/qemu/timer.h:551
#3  timer_new_ns (opaque=0x555556801e38, cb=0x555555af11e0 <hpet_timer>, type=QEMU_CLOCK_VIRTUAL) at /home/maritns3/core/xqm/include/qemu/timer.h:569
#4  hpet_realize (dev=0x555556801910, errp=<optimized out>) at /home/maritns3/core/xqm/hw/timer/hpet.c:774

#0  timer_init_full (ts=ts@entry=0x555556970610, timer_list_group=timer_list_group@entry=0x0, type=type@entry=QEMU_CLOCK_VIRTUAL, scale=scale@entry=1, attributes=attri
butes@entry=0, cb=cb@entry=0x555555af2780 <pit_irq_timer>, opaque=0x55555677d688) at /home/maritns3/core/xqm/util/qemu-timer.c:366
#1  0x0000555555af28b8 in timer_new_full (opaque=0x55555677d688, cb=0x555555af2780 <pit_irq_timer>, attributes=0, scale=1, type=QEMU_CLOCK_VIRTUAL, timer_list_group=0x
0) at /home/maritns3/core/xqm/include/qemu/timer.h:531
#2  timer_new (type=QEMU_CLOCK_VIRTUAL, scale=1, cb=0x555555af2780 <pit_irq_timer>, opaque=0x55555677d688) at /home/maritns3/core/xqm/include/qemu/timer.h:551
#3  timer_new_ns (opaque=0x55555677d688, cb=0x555555af2780 <pit_irq_timer>, type=QEMU_CLOCK_VIRTUAL) at /home/maritns3/core/xqm/include/qemu/timer.h:569
#4  pit_realizefn (dev=0x55555677d500, errp=0x7fffffffcce0) at /home/maritns3/core/xqm/hw/timer/i8254.c:340

#0  timer_init_full (ts=ts@entry=0x555556a12b90, timer_list_group=timer_list_group@entry=0x0, type=type@entry=QEMU_CLOCK_VIRTUAL, scale=scale@entry=1, attributes=attri
butes@entry=0, cb=cb@entry=0x555555a20840 <serial_update_msl>, opaque=0x55555665fa20) at /home/maritns3/core/xqm/util/qemu-timer.c:366
#1  0x0000555555a216dd in timer_new_full (opaque=0x55555665fa20, cb=0x555555a20840 <serial_update_msl>, attributes=0, scale=1, type=QEMU_CLOCK_VIRTUAL, timer_list_grou
p=0x0) at /home/maritns3/core/xqm/include/qemu/timer.h:531
#2  timer_new (type=QEMU_CLOCK_VIRTUAL, scale=1, opaque=0x55555665fa20, cb=0x555555a20840 <serial_update_msl>) at /home/maritns3/core/xqm/include/qemu/timer.h:551
#3  timer_new_ns (opaque=0x55555665fa20, cb=0x555555a20840 <serial_update_msl>, type=QEMU_CLOCK_VIRTUAL) at /home/maritns3/core/xqm/include/qemu/timer.h:569
#4  serial_realize_core (s=s@entry=0x55555665fa20, errp=errp@entry=0x7fffffffcca0) at /home/maritns3/core/xqm/hw/char/serial.c:938
#5  0x0000555555a21b21 in serial_isa_realizefn (dev=0x55555665f980, errp=0x7fffffffcca0) at /home/maritns3/core/xqm/hw/char/serial-isa.c:78
```
- apic_realize
- ioapic_realize
- pci_std_vga_realize
- hpet_realize
- pit_realizefn


## trigger
在  main_loop_wait 中，会根据 timer 的时间来设置等待时间的。
```c
    timeout_ns = qemu_soonest_timeout(timeout_ns,
                                      timerlistgroup_deadline_ns(
                                          &main_loop_tlg));
```

```c
/*
#0  rtc_update_timer (opaque=0x55555669b400) at /home/maritns3/core/xqm/hw/rtc/mc146818rtc.c:427
#1  0x0000555555caea96 in timerlist_run_timers (timer_list=0x5555565d6470) at /home/maritns3/core/xqm/util/qemu-timer.c:595
#2  timerlist_run_timers (timer_list=0x5555565d6470) at /home/maritns3/core/xqm/util/qemu-timer.c:506
#3  0x0000555555caec90 in qemu_clock_run_timers (type=<optimized out>) at /home/maritns3/core/xqm/util/qemu-timer.c:695
#4  qemu_clock_run_all_timers () at /home/maritns3/core/xqm/util/qemu-timer.c:695
#5  0x0000555555caf0c1 in main_loop_wait (nonblocking=<optimized out>) at /home/maritns3/core/xqm/util/main-loop.c:525
#6  0x00005555559bbf59 in main_loop () at /home/maritns3/core/xqm/vl.c:1812
#7  0x000055555582b2f9 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4473
```

通过 timer_mod_ns 将 tiemr 添加到 QEMUTimerList::timer_list 中间去

## 如何从 realtime 的 clock 到模拟其他的三种 clock
比如下面的实现，为什么可以表示的是 CPU 运行的时间呀。
```c
qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL)
```

## rr 中怎么处理的
- [ ] 是不是存在多个 vCPU timer 的，然后尽可能保证所有的 timer 的时间相同的

## icount
很多大函数只是为了处理 icount 的:
- qemu_clock_deadline_ns_all

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。

[^1]: https://airbus-seclab.github.io/qemu_blog/timers.html
