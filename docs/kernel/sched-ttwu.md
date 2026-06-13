# 为什么感觉 try_to_wake_up 非常复杂的样子，是因为考虑到什么特殊的东西吗

一个有趣的 backtrace :
```txt
#0  select_task_rq_fair (p=0xffff88814a118000, prev_cpu=1, wake_flags=24) at kernel/sched/fair.c:7015
#1  0xffffffff8113cff4 in select_task_rq (wake_flags=24, cpu=1, p=0xffff88814a118000) at kernel/sched/core.c:3489
#2  try_to_wake_up (p=0xffff88814a118000, state=<optimized out>, wake_flags=16) at kernel/sched/core.c:4183
#3  0xffffffff8113d469 in default_wake_function (curr=curr@entry=0xffffc900000b3988, mode=<optimized out>, wake_flags=<optimized out>, key=<optimized out>) at kernel/sched/core.c:6818
#4  0xffffffff8136daff in __pollwake (wait=<optimized out>, wait=<optimized out>, key=<optimized out>, sync=<optimized out>, mode=<optimized out>) at fs/select.c:208
#5  pollwake (wait=<optimized out>, mode=<optimized out>, sync=<optimized out>, key=<optimized out>) at fs/select.c:218
#6  0xffffffff811561b1 in __wake_up_common (wq_head=wq_head@entry=0xffff88814a4bf440, mode=mode@entry=1, nr_exclusive=nr_exclusive@entry=1, wake_flags=wake_flags@entry=16, key=key@entry=0xc3, bookmark=bookmark@entry=0xffffc900000b3a10) at kernel/sched/build_utility.c:4403
#7  0xffffffff811562f7 in __wake_up_common_lock (wq_head=0xffff88814a4bf440, mode=mode@entry=1, nr_exclusive=nr_exclusive@entry=1, wake_flags=wake_flags@entry=16, key=key@entry=0xc3) at kernel/sched/build_utility.c:4433
#8  0xffffffff811567a7 in __wake_up_sync_key (wq_head=<optimized out>, mode=mode@entry=1, key=key@entry=0xc3) at kernel/sched/build_utility.c:4500
#9  0xffffffff81bd9747 in sock_def_readable (sk=0xffff88814a0b0000) at net/core/sock.c:3221
#10 0xffffffff81cc6248 in tcp_data_queue (sk=sk@entry=0xffff88814a0b0000, skb=skb@entry=0xffff888154a2d800) at net/ipv4/tcp_input.c:5078
#11 0xffffffff81cc6be3 in tcp_rcv_established (sk=sk@entry=0xffff88814a0b0000, skb=skb@entry=0xffff888154a2d800) at net/ipv4/tcp_input.c:6004
#12 0xffffffff81cd3a5e in tcp_v4_do_rcv (sk=sk@entry=0xffff88814a0b0000, skb=skb@entry=0xffff888154a2d800) at net/ipv4/tcp_ipv4.c:1661
#13 0xffffffff81cd5ceb in tcp_v4_rcv (skb=0xffff888154a2d800) at net/ipv4/tcp_ipv4.c:2078
#14 0xffffffff81ca54cd in ip_protocol_deliver_rcu (net=net@entry=0xffffffff83535980 <init_net>, skb=skb@entry=0xffff888154a2d800, protocol=<optimized out>) at net/ipv4/ip_input.c:205
#15 0xffffffff81ca573e in ip_local_deliver_finish (net=0xffffffff83535980 <init_net>, sk=<optimized out>, skb=0xffff888154a2d800) at net/ipv4/ip_input.c:233
#16 0xffffffff81bfffc0 in deliver_skb (orig_dev=0xffff888100b20000, pt_prev=<optimized out>, skb=<optimized out>) at net/core/dev.c:2189
#17 deliver_ptype_list_skb (ptype_list=0xffff888100b20090, type=8, orig_dev=0xffff888100b20000, pt=<synthetic pointer>, skb=<optimized out>) at net/core/dev.c:2204
#18 __netif_receive_skb_core (pskb=pskb@entry=0xffffc900000b3cd8, pfmemalloc=pfmemalloc@entry=false, ppt_prev=ppt_prev@entry=0xffffc900000b3ce0) at net/core/dev.c:5441
#19 0xffffffff81c00e95 in __netif_receive_skb_list_core (head=head@entry=0xffff888100b20d18, pfmemalloc=pfmemalloc@entry=false) at net/core/dev.c:5561
#20 0xffffffff81c015c7 in __netif_receive_skb_list (head=0xffff888100b20d18) at net/core/dev.c:5628
#21 netif_receive_skb_list_internal (head=head@entry=0xffff888100b20d18) at net/core/dev.c:5719
#22 0xffffffff81c0180a in gro_normal_list (napi=0xffff888100b20c10) at include/net/gro.h:430
#23 gro_normal_list (napi=0xffff888100b20c10) at include/net/gro.h:426
#24 napi_complete_done (n=n@entry=0xffff888100b20c10, work_done=<optimized out>) at net/core/dev.c:6060
#25 0xffffffff81a78591 in e1000e_poll (napi=0xffff888100b20c10, budget=64) at drivers/net/ethernet/intel/e1000e/netdev.c:2690
#26 0xffffffff81c01954 in __napi_poll (n=0xffff88814a118000, n@entry=0xffff888100b20c10, repoll=repoll@entry=0xffffc900000b3e37) at net/core/dev.c:6511
#27 0xffffffff81c01e70 in napi_poll (repoll=0xffffc900000b3e48, n=0xffff888100b20c10) at net/core/dev.c:6578
#28 net_rx_action (h=<optimized out>) at net/core/dev.c:6689
#29 0xffffffff822000e1 in __do_softirq () at kernel/softirq.c:571
#30 0xffffffff8110b2e5 in run_ksoftirqd (cpu=<optimized out>) at kernel/softirq.c:934
#31 run_ksoftirqd (cpu=<optimized out>) at kernel/softirq.c:926
#32 0xffffffff81130fb0 in smpboot_thread_fn (data=0xffff888100126130) at kernel/smpboot.c:164
#33 0xffffffff8112ac90 in kthread (_create=0xffff88810014b3c0) at kernel/kthread.c:376
#34 0xffffffff81001a6f in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

- try_to_wake_up : 这个函数存在特别多的注释
  - select_task_rq
  - ttwu_queue
    - ttwu_queue_wakelist
    - ttwu_do_activate

## 使用 perf 观测一下的

sudo bpftrace -e "kprobe:try_to_wake_up {  @[kstack] = count(); }"

```txt
@[
    try_to_wake_up+1
    wake_up_q+74
    futex_wake+333
    do_futex+185
    __x64_sys_futex+115
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+68
]: 1723
@[
    try_to_wake_up+1
    hrtimer_wakeup+30
    __hrtimer_run_queues+298
    hrtimer_interrupt+262
    __sysvec_apic_timer_interrupt+127
    sysvec_apic_timer_interrupt+157
    asm_sysvec_apic_timer_interrupt+18
    native_safe_halt+11
    default_idle+10
    default_idle_call+50
    do_idle+478
    cpu_startup_entry+25
    start_secondary+278
    secondary_startup_64_no_verify+213
]: 3855
```
- 这个 3855 次被唤醒的，东西到底是什么，我猜测是 swapper


sudo bpftrace -e "kprobe:try_to_wake_up {  @[comm] = count(); }"

```txt
@[syncthing]: 155
@[swapper/15]: 191
@[swapper/13]: 249
@[swapper/2]: 261
@[swapper/5]: 286
@[swapper/17]: 624
@[nvim]: 894
```

## activate_task
```c
void activate_task(struct rq *rq, struct task_struct *p, int flags)
{
    if (task_contributes_to_load(p))
        rq->nr_uninterruptible--;

    enqueue_task(rq, p, flags);
}
```

```txt
#0  activate_task (flags=8, p=0xffff888100219740, rq=0xffff888333c2b2c0) at kernel/sched/core.c:2091
#1  wake_up_new_task (p=p@entry=0xffff888100219740) at kernel/sched/core.c:4680
#2  0xffffffff811039b4 in kernel_clone (args=args@entry=0xffffc90000753eb0) at kernel/fork.c:2695
#3  0xffffffff81103de6 in __do_sys_clone (clone_flags=<optimized out>, newsp=<optimized out>, parent_tidptr=<optimized out>, child_tidptr=<optimized out>, tls=<optimized out>) at kernel/fork.c:2805
#4  0xffffffff81f3d6e8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000753f58) at arch/x86/entry/common.c:50
#5  do_syscall_64 (regs=0xffffc90000753f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#6  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

```txt
0  activate_task (flags=9, p=0xffff88814a10c5c0, rq=0xffff888333c2b2c0) at kernel/sched/core.c:2091
#1  ttwu_do_activate (rq=rq@entry=0xffff888333c2b2c0, p=p@entry=0xffff88814a10c5c0, wake_flags=wake_flags@entry=0, rf=<optimized out>) at kernel/sched/core.c:3670
#2  0xffffffff8113d0e0 in ttwu_queue (wake_flags=0, cpu=<optimized out>, p=0xffff88814a10c5c0) at kernel/sched/core.c:3875
#3  try_to_wake_up (p=0xffff88814a10c5c0, state=state@entry=3, wake_flags=wake_flags@entry=0) at kernel/sched/core.c:4198
#4  0xffffffff8113d3cc in wake_up_process (p=<optimized out>) at kernel/sched/core.c:4314
#5  0xffffffff8119b859 in hrtimer_wakeup (timer=<optimized out>) at kernel/time/hrtimer.c:1939
#6  0xffffffff8119bde2 in __run_hrtimer (flags=130, now=0xffffc9000086be90, timer=0xffffc9000080be90, base=0xffff888333c1e0c0, cpu_base=0xffff888333c1e080) at kernel/time/hrtimer.c:1685
#7  __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff888333c1e080, now=48944931561680, flags=flags@entry=130, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#8  0xffffffff8119ca71 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#9  0xffffffff810e25d7 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1095
```

> 下面两个顶层函数

![](../../img/source/ttwu_queue.png)
![](../../img/source/ttwu_remote.png)

![](../../img/source/ttwu_do_activate.png)
> 我们的核心函数 ?

![](../../img/source/ttwu_do_wakeup.png)
![](../../img/source/ttwu_activate.png)
![](../../img/source/ttwu_queue_remote.png)

![](../../img/source/try_to_wake_up_local.png)

1. try_to_wake_up 和 try_to_wake_up_local 的关系是什么 ?
2. remote wakeup 的含义是什么 ?

## sched_ttwu_pending

该函数似乎是将所有 rq 中的所有的，wake_list

```c
void sched_ttwu_pending(void)
{
    struct rq *rq = this_rq();
    struct llist_node *llist = llist_del_all(&rq->wake_list);
    struct task_struct *p, *t;
    struct rq_flags rf;

    if (!llist)
        return;

    rq_lock_irqsave(rq, &rf);
    update_rq_clock(rq);

    llist_for_each_entry_safe(p, t, llist, wake_entry)
        ttwu_do_activate(rq, p, p->sched_remote_wakeup ? WF_MIGRATED : 0, &rf);

    rq_unlock_irqrestore(rq, &rf);
}
```

![](../../img/source/sched_ttwu_pending.png)
