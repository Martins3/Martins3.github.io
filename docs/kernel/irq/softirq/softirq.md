# softirq

A softirq never preempts another softirq. The only event that can preempt a softirq is
an interrupt handler.
Another softirq—even the same one—can run on another processor,
however.

## softirq 的主要函数
<!-- b8f3ff80-4018-4fb7-80f2-b2b95f63042a -->

1. raise_softirq : 请求 softirq
2. do_softirq : 在 task context 中打开的 bh 的时候执行
3. invoke_softirq : 硬中断执行完成之后，调用 invoke_softirq 来触发 softirq


## softirq 中是不可以睡眠的
<!-- a9837584-dbd4-4816-a126-9024aac13201 -->

## raise_softirq : 请求 softirq

- `raise_softirq` : 发送请求，说存在中断已经好了

```txt
@[
    raise_softirq+1
    trigger_load_balance+131
    update_process_times+176
    tick_sched_handle+52
    tick_sched_timer+122
    __hrtimer_run_queues+298
    hrtimer_interrupt+252
    __sysvec_apic_timer_interrupt+92
    sysvec_apic_timer_interrupt+55
    asm_sysvec_apic_timer_interrupt+18
]: 217
```

- `raise_softirq_irqoff` : 如果已经屏蔽中断的时候调用这个
所以 `raise_softirq` 的调用者很有趣，意味着他们非中断上下文中执行。

### [ ] raise_softirq 为什么必须屏蔽中断
```c
void raise_softirq(unsigned int nr)
{
	unsigned long flags;

	local_irq_save(flags);
	raise_softirq_irqoff(nr);
	local_irq_restore(flags);
}
```

## do_softirq / invoke_softirq :执行

> Pending softirqs are checked for and executed in the following places:
> - In the return from hardware interrupt code path
> - In the ksoftirqd kernel thread
> - In any code that explicitly checks for and executes pending softirqs, such as the networking subsystem
>
> LKD chapter 8

软中断中什么时候打开中断

就是这里:
```c
asmlinkage __visible void __softirq_entry __do_softirq(void){
  //
  local_irq_enable();
  //
}
```

### softirq 如何解决复用问题

简单来说，softirq 的处理者都是在内核的通用架构中
例如 block layer 中

在 `queue_request_irq` 注册 irq handler 为 `nvme_irq`

- `nvme_irq`
  - `nvme_process_cq`
    - `nvme_handle_cqe`
      - `nvme_try_complete_req`
        - `blk_mq_complete_request_remote`
          - `blk_mq_raise_softirq`

这里顺便会要处理的工作放到 blk_cpu_done 中
```c
static DEFINE_PER_CPU(struct llist_head, blk_cpu_done);
```

然后在 do_softirq 中从 blk_cpu_done 中取出来，然后执行:
```c
void blk_mq_complete_request(struct request *rq)
{
  if (!blk_mq_complete_request_remote(rq))
    rq->q->mq_ops->complete(rq);
}
```

### 执行的上下文

#### [ ] 运行 softirq 的 stack 的是在哪里
有函数来打印出来当前在什么 stack 上吗？

#### irq

- common_interrupt
  - irq_exit_rcu
    - __irq_exit_rcu
      - invoke_softirq
        - __do_softirq
          - blk_complete_reqs
            - scsi_io_completion
              - scsi_end_request
                - blk_update_request
                  - req_bio_endio
                    - reschedule_retry

#### process

do_softirq 是可以在 process 上下文中直接调用的:
```txt
@[
        do_softirq+5
        __local_bh_enable_ip+96
        __dev_queue_xmit+444
        ip_finish_output2+585
        ip_send_skb+137
        udp_send_skb+395
        udp_sendmsg+2431
        __sys_sendto+438
        __x64_sys_sendto+36
        do_syscall_64+95
        entry_SYSCALL_64_after_hwframe+118
]: 60
```

- `rcu_read_unlock_bh` 的时候，就可以发生 softirq
  - `__local_bh_enable_ip(_THIS_IP_, SOFTIRQ_DISABLE_OFFSET);`

## softirq 的用户
<!-- 3629f535-bddb-4b02-a8d2-4462125642e4 -->

```txt
cat /proc/softirqs
                    CPU0       CPU1       CPU2       CPU3
          HI:          0          0          0          0
       TIMER:     111058     250916     116765     396824
      NET_TX:          0          1          0          2
      NET_RX:    4352570        432    7701231     625148
       BLOCK:         11         33         61        120
    IRQ_POLL:          0          0          0          0
     TASKLET:    2173913     934747     174410     736856
       SCHED:     104932      75044      25442      36725
     HRTIMER:          0          0          0          0
         RCU:       7792       8180       8989       6558
```

```txt
                    CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7       CPU8       CPU9       CPU10      CPU11      CPU12      CPU13      CPU14      CPU15      CPU16      CPU17      CPU18      CPU19      CPU20      CPU21      CPU22      CPU23      CPU24      CPU25      CPU26      CPU27      CPU28      CPU29      CPU30      CPU31
          HI:          0          0          3          0          0          0          0          0          0          0          3          0          0         79          0          0          0          0          0          0          0          0          0          0          0          0          4          0          0          0          0          0
       TIMER:    2032453     406558    2123145     484743    1786474     410195    1614072     439216    4988303    1857903    5122219    1674609    3117306     428707    2149080     602442     654322     623533     427384     297189     259123     255785     252820     240827     229359     227181     353451     230917     414408     452206     310657     276031
      NET_TX:        164         24        148         26        366         28        292         27        212        129        247        128        350         47        344        216        105         63         33         22         13         18          5          8         18         11          8        106         15         87       1907         18
      NET_RX:      12907       2851    1117708     129963      42789      57776      28216      52660      48860       6372      50716       7972      23789      11276      16267    2571482      33414      53892      41697       8945       8320     212948     184977      63963      53971      13688     561298       8318    1789414       5593     786769     102217
       BLOCK:          1          0          0       1284        161        310        122        278         22         16          1          0        559          1          0         42          0         72        259          0         16         20       2590        288        240        691        181        191        143          0         26       1890
    IRQ_POLL:          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0          0
     TASKLET:          0          0       7146          0          0        151        124          0          0          1         12          8          2       5978          0      36769          0          0          0          1         27       3148        932          0          0          0       2373          0       4155          4       6646          1
       SCHED:   17421142    2239841    4444609     801394    3752957     688674    3433101     713689   13690789    3449551   14383379    3075826    8234605     693637    5181775     908993    1258948    1096136     771192     567560     589959     558081     538081     533229     510486     481307     616447     502706     725339     578757     566151     534993
     HRTIMER:          1          0         33       5457         43         17        129         36         13          0         11          2         12        167          7         23        595       3716        601          2          3        868       1043        332       3710         15       3407         19       1806         18          8         16
         RCU:    2560478    1050805    2503274     991432    2365579     995201    2339285    1005718    3886562    1734130    4030698    1617189    3085873     988126    2625399    1055021    1765536    1655260    1393958    1207987    1338298    1290265    1262681    1279912    1253805    1216058    1321909    1258287    1335142    1249225    1197843    1185257
```

### hrtimer 为什么会依赖 softirq
<!-- b77574fa-1894-4be2-9da2-cbead59a733f -->

简而言之，是由于存在部分用户就是希望 hrtimer 在 softirq 中执行:

```c
enum  hrtimer_base_type {
	HRTIMER_BASE_MONOTONIC,
	HRTIMER_BASE_REALTIME,
	HRTIMER_BASE_BOOTTIME,
	HRTIMER_BASE_TAI,
	HRTIMER_BASE_MONOTONIC_SOFT,
	HRTIMER_BASE_REALTIME_SOFT,
	HRTIMER_BASE_BOOTTIME_SOFT,
	HRTIMER_BASE_TAI_SOFT,
	HRTIMER_MAX_CLOCK_BASES,
};
```

如果发现是 softirq ，那么就调用 raise 了:
```c
	if (!ktime_before(now, cpu_base->softirq_expires_next)) {
		cpu_base->softirq_expires_next = KTIME_MAX;
		cpu_base->softirq_activated = 1;
		raise_timer_softirq(HRTIMER_SOFTIRQ);
	}
```

```diff
History:        #0
Commit:         5da70160462e80b0ab8a6960cdd0cdd476907523
Author:         Anna-Maria Gleixner <anna-maria@linutronix.de>
Committer:      Ingo Molnar <mingo@kernel.org>
Author Date:    Thu 21 Dec 2017 06:41:57 PM CST
Committer Date: Tue 16 Jan 2018 04:51:22 PM CST

hrtimer: Implement support for softirq based hrtimers

hrtimer callbacks are always invoked in hard interrupt context. Several
users in tree require soft interrupt context for their callbacks and
achieve this by combining a hrtimer with a tasklet. The hrtimer schedules
the tasklet in hard interrupt context and the tasklet callback gets invoked
in softirq context later.

That's suboptimal and aside of that the real-time patch moves most of the
hrtimers into softirq context. So adding native support for hrtimers
expiring in softirq context is a valuable extension for both mainline and
the RT patch set.

Each valid hrtimer clock id has two associated hrtimer clock bases: one for
timers expiring in hardirq context and one for timers expiring in softirq
context.

Implement the functionality to associate a hrtimer with the hard or softirq
related clock bases and update the relevant functions to take them into
account when the next expiry time needs to be evaluated.

Add a check into the hard interrupt context handler functions to check
whether the first expiring softirq based timer has expired. If it's expired
the softirq is raised and the accounting of softirq based timers to
evaluate the next expiry time for programming the timer hardware is skipped
until the softirq processing has finished. At the end of the softirq
processing the regular processing is resumed.

Suggested-by: Thomas Gleixner <tglx@linutronix.de>
Suggested-by: Peter Zijlstra <peterz@infradead.org>
Signed-off-by: Anna-Maria Gleixner <anna-maria@linutronix.de>
Cc: Christoph Hellwig <hch@lst.de>
Cc: John Stultz <john.stultz@linaro.org>
Cc: Linus Torvalds <torvalds@linux-foundation.org>
Cc: keescook@chromium.org
Link: http://lkml.kernel.org/r/20171221104205.7269-29-anna-maria@linutronix.de
Signed-off-by: Ingo Molnar <mingo@kernel.org>
```

### NET_TX_SOFTIRQ 为什么很少？

现代网卡多用 **NAPI**，发包走 poll 模式，而非传统的发送中断：

```
传统模式: 发送完成 → 硬中断 → NET_TX_SOFTIRQ
NAPI 模式: 发送完成 → 轮询处理（无中断）
```

```txt
  核心原因：现代网卡普遍采用 NAPI 轮询机制

  传统模式: 发送完成 → 硬中断 → NET_TX_SOFTIRQ
  NAPI 模式: 发送完成 → 轮询处理（无中断）

  详细解释

  1. 发送与接收的本质差异

   方向        特性                处理方式
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   接收 (RX)   被动、不可预测      必须用中断通知 CPU → 触发 NET_RX_SOFTIRQ
   发送 (TX)   主动、由 CPU 控制   可轮询检查完成状态，无需中断

  2. NAPI 机制的设计
  NAPI (New API) 是为了解决高负载下中断风暴问题而设计的：
  • 接收路径：硬中断触发 → 进入轮询模式 → NET_RX_SOFTIRQ 批量处理
  • 发送路径：直接在轮询中检查发送完成，不经过软中断
```
(还有一个解释是: (1) RX 是“包级事件”，TX 是“批量事件” ，我也勉强可以接受的)

### softirq 的用户中， TASKLET 的数量还是很多的
<!-- 3c57aedb-d67c-4516-a520-b696ceec2225 -->

### softirq SCHED 是做什么的
<!-- c78f2b3f-f2d8-49e7-9e4c-90b6a46eaed4 -->

```txt
核心功能：SMP 负载均衡

// 注册处理函数
open_softirq(SCHED_SOFTIRQ, sched_balance_softirq);

两个触发场景

 场景             触发函数                  说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 周期性负载均衡   sched_balance_trigger()   每个 tick 检查是否需要均衡
 NOHZ 空闲均衡    nohz_csd_func()           跨 CPU 调用，为停 tick 的 idle CPU 做均衡

处理流程

static void sched_balance_softirq(void)
{
    // 1. 先处理 NOHZ idle 均衡（为其他 idle CPU 做均衡）
    if (nohz_idle_balance(this_rq, idle))
        return;

    // 2. 正常负载均衡
    sched_balance_update_blocked_averages(this_rq->cpu);
    sched_balance_domains(this_rq, idle);
}

为什么需要 softirq？

原因：负载均衡可能涉及跨 CPU 操作（如检查其他 runqueue 的负载、迁移任务），这些操作：

• 不能在硬中断上下文做（太耗时）
• 不适合直接在当前任务上下文做（需要关调度）
• 用 softirq 是异步且安全的方案

总结：SCHED_SOFTIRQ 是 CFS 调度器在 SMP 环境下实现周期性负载均衡和NOHZ 空闲均衡的机制。
```

基本调用路线:
```c
  // kernel/sched/core.c
  void sched_tick(void)  // 定时器以 HZ 频率调用
  {
      // ... 更新统计信息 ...

      if (!scx_switched_all()) {
          rq->idle_balance = idle_cpu(cpu);  // 标记CPU是否空闲
          sched_balance_trigger(rq);         // 触发负载均衡检查
      }
  }

  // kernel/sched/fair.c
  void sched_balance_trigger(struct rq *rq)
  {
      // 检查是否在NULL domain 或 CPU未激活
      if (unlikely(on_null_domain(rq) || !cpu_active(cpu_of(rq))))
          return;

      // 检查是否到达下一次均衡时间点
      if (time_after_eq(jiffies, rq->next_balance))
          raise_softirq(SCHED_SOFTIRQ);  // ← 触发点1

      nohz_balancer_kick(rq);  // 同时尝试唤醒空闲CPU做均衡
  }
```


### rcu 为什么会依赖 softirq
<!-- 1bb4bbb3-d159-46ac-9d2f-103659890d63 -->

主要是需要赶快来处理各种回调工作，例如处理掉回调，似乎是这个意思，

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```c
void rcu_core(void) {
    // 1. 驱动状态机
    if (current_cpu_is_safe()) {
        report_quiescent_state(); // 告诉主控：我不占用旧数据了
    }

    // 2. 检查宽限期是否翻篇
    check_grace_period_progress();

    // 3. 处理回调
    if (list_has_ready_callbacks(rcu_data)) {
        // 尝试在软中断里执行一部分
        int count = rcu_do_batch(rcu_data);

        // 4. 如果没做完，或者被更高优先级任务抢占
        if (list_still_has_callbacks(rcu_data) && need_resched()) {
             // 唤醒 kthread (rcuc/n) 来做剩下的脏活累活
             wake_up_process(rcu_data->rcu_cpu_kthread_task);
        }
    }
}
```

经典的触发路径为:
```txt
@[
        invoke_rcu_core+1
        rcu_sched_clock_irq+583
        update_process_times+124
        tick_nohz_handler+143
        __hrtimer_run_queues+305
        hrtimer_interrupt+255
        __sysvec_apic_timer_interrupt+85
        sysvec_apic_timer_interrupt+108
        asm_sysvec_apic_timer_interrupt+26
        cpuidle_enter_state+192
        cpuidle_enter+45
        cpuidle_idle_call+261
        do_idle+114
        cpu_startup_entry+41
        start_secondary+276
        common_startup_64+318
]: 32187
```

### 存储

blk_mq_complete_request_remote 中已经说的很清楚了，
是不是放到 softirq 中取决于 hwirq 的数量，如果只有一个 hwirq
那么就走软中断

nvme 根本不会使用 softirq
```txt
@[
    blk_mq_complete_request_remote+5 <---- 在这里会进行判断，如果 rq->q->nr_hw_queues == 1 才会 blk_mq_raise_softirq
    nvme_poll_cq+347
    nvme_irq+66
    __handle_irq_event_percpu+74
    handle_irq_event+62
    handle_edge_irq+157
    __common_interrupt+63
    common_interrupt+67
    asm_common_interrupt+38
]: 197297
```

实际上，在这里就把事情搞完了:
```txt
@[
    nvme_pci_complete_batch+5
    nvme_irq+114
    __handle_irq_event_percpu+74
    handle_irq_event+62
    handle_edge_irq+157
    __common_interrupt+63
    common_interrupt+129
    asm_common_interrupt+38
    cpuidle_enter_state+204
    cpuidle_enter+45
    do_idle+472
    cpu_startup_entry+42
    start_secondary+286
    secondary_startup_64_no_verify+381
]: 104947
```

如果去分析 U 盘的结果，是的，他就是使用软中断，然后:

```txt
@[
    __do_softirq+5
    __irq_exit_rcu+147
    common_interrupt+134
    asm_common_interrupt+38
    cpuidle_enter_state+204
    cpuidle_enter+45
    do_idle+472
    cpu_startup_entry+42
    start_secondary+286
    secondary_startup_64_no_verify+381
]: 67418
```

## ksoftirqd
- 到底 softirq 和 hardirq 放到一起执行的，还是 softirq 在 ksoftirqd 中间执行:
  - 在 `invoke_softirq` 中间对于内核参数 `force_irqthreads` 进行判断，如果是，那么所有的 softirq 都是在 ksoftirqd 中间执行的
  - 似乎存在一些 softirq 无法立刻被执行(防止 starve 其他的代码), 这些可能被之后 `wakeup_softirqd` 的时候执行

https://stackoverflow.com/questions/13717166/how-to-activate-all-the-ksoftirqds-in-linux-about-net-stack-of-linux-kernel
看代码，其实的确是如此，就是当太多 softirq 出现的时候才会调用 softirqd

## [Diagnosing workqueues](https://lwn.net/Articles/967016/)

- 翻译 : https://mp.weixin.qq.com/s/KQcUCEdbF1NIzY5ft7Ufxw

软中断的设计目前存在两个问题:
1. 优先级反转，因为硬中断可以打断软中断，但是软中断无法打断软中断。
2. 如果持续一个 CPU 反复的检测到软中断，那么可能将一个永远不释放一个核。

> To balance these two problems, there are two heuristic limits used to balance latency against fairness.
> - `MAX_SOFTIRQ_TIME` is the maximum time that a software interrupt is allowed to run; it is set to 2ms.
> - `MAX_SOFTIRQ_RESTART` is the maximum number of times that a software interrupt that is itself interrupted by something else will be restarted;

看上去，第一个问题也不算是解决了。

这个文章配合这个阅读，非常有趣: https://stackoverflow.com/questions/22453739/why-do-we-need-interrupt-context

## 经典问题
### [ ] cpu 隔离对于 softirq 有影响吗?

### softirq 是否可以嵌套
看一个具体的例子，中断，然后软中断，然后软中断又被打断的
```txt
[   12.757102] [martins3:virtio_dummy_recv_cb:212] ------
[   12.757512] CPU: 2 PID: 369 Comm: kworker/2:1H Tainted: G           O       6.8.0-rc7-00231-g09e5c48fea17-dirty #17
[   12.758009] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[   12.758009] Workqueue: kblockd blk_mq_run_work_fn
[   12.758009] Call Trace:
[   12.758009]  <IRQ>
[   12.758009]  dump_stack_lvl+0x4a/0x80
[   12.758009]  virtio_dummy_recv_cb+0xc3/0xf0 [virtio_dummy]
[   12.758009]  vring_interrupt+0x5b/0x90
[   12.758009]  vp_vring_interrupt+0x57/0x90
[   12.758009]  __handle_irq_event_percpu+0x6d/0x1d0
[   12.758009]  handle_irq_event+0x38/0x80
[   12.758009]  handle_fasteoi_irq+0x7c/0x210
[   12.758009]  __common_interrupt+0x3c/0xa0
[   12.758009]  common_interrupt+0x44/0xa0
[   12.758009]  asm_common_interrupt+0x26/0x40
[   12.758009] RIP: 0010:bio_first_folio+0x2c/0xe0
[   12.758009] Code: d0 f6 46 14 02 0f 85 b4 00 00 00 0f b7 46 68 49 63 c8 48 c1 e1 04 48 03 4e 70 44 39 c0 0f 8e a5 00 00 00 48 8b 11 48 8b 42 08 <a8> 01 0f 85 a2 00 00 00 66 90 48 89 17 48 8b 01 8b 71 0c 48 29 d0
[   12.758009] RSP: 0018:ffffc90000218ea0 EFLAGS: 00000202
[   12.758009] RAX: ffffea00047cffc8 RBX: ffff888109894180 RCX: ffff888109894200
[   12.758009] RDX: ffffea00047d0000 RSI: ffff888109894180 RDI: ffffc90000218ea8
[   12.758009] RBP: ffffc90000218ea8 R08: 0000000000000000 R09: ffffffff818c0934
[   12.758009] R10: 0000000000000024 R11: 0000000000000019 R12: 0000000000001000
[   12.758009] R13: ffff888109894180 R14: 0000000000001000 R15: 0000000000001000
[   12.758009]  ? blkdev_bio_end_io_async+0x34/0x80
[   12.758009]  bio_check_pages_dirty+0x45/0x180
[   12.758009]  blk_update_request+0x106/0x480
[   12.758009]  ? sbitmap_queue_clear+0x3b/0x60
[   12.758009]  blk_mq_end_request+0x1c/0x30
[   12.758009]  blk_complete_reqs+0x3d/0x50
[   12.758009]  __do_softirq+0x10b/0x3a7
[   12.758009]  __irq_exit_rcu+0xab/0xd0
[   12.758009]  irq_exit_rcu+0xe/0x20
[   12.758009]  common_interrupt+0x88/0xa0
[   12.758009]  </IRQ>
[   12.758009]  <TASK>
[   12.758009]  asm_common_interrupt+0x26/0x40
[   12.758009] RIP: 0010:iowrite16+0x13/0x40
[   12.758009] Code: 1f 84 00 00 00 00 00 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90 f3 0f 1e fa 48 89 f2 48 81 fe ff ff 03 00 76 08 66 89 3e <c3> cc cc cc cc 48 81 fe 00 00 01 00 76 09 89 f8 66 ef c3 cc cc cc
[   12.758009] RSP: 0018:ffffc90001dafc68 EFLAGS: 00000292
[   12.771636] RAX: ffffffff81ab7860 RBX: ffff88810910d100 RCX: 0000000000000cc0
[   12.771636] RDX: ffffc900001b9000 RSI: ffffc900001b9000 RDI: 0000000000000000
[   12.772657] RBP: ffff888126061e00 R08: 0000000000000004 R09: 0000000000000004
[   12.772657] R10: ffff888126061e00 R11: 0000000000000001 R12: ffff88810af21c00
[   12.772657] R13: 0000000000000286 R14: ffff888118e5dc98 R15: 0000000000000000
[   12.772657]  ? __pfx_vp_notify+0x10/0x10
[   12.772657]  ? preempt_count_sub+0x4b/0x60
[   12.772657]  vp_notify+0x16/0x20
[   12.772657]  virtqueue_notify+0x1c/0x40
[   12.772657]  dummy_queue_rq+0x111/0x170 [virtio_dummy]
[   12.772657]  blk_mq_dispatch_rq_list+0x13e/0x790
[   12.772657]  ? sched_clock_cpu+0x5e/0x190
[   12.772657]  __blk_mq_sched_dispatch_requests+0xbb/0x5c0
[   12.772657]  blk_mq_sched_dispatch_requests+0x3f/0x60
[   12.772657]  blk_mq_run_work_fn+0x61/0x70
[   12.772657]  process_one_work+0x138/0x2f0
[   12.772657]  worker_thread+0x2f5/0x420
[   12.772657]  ? preempt_count_sub+0x4b/0x60
[   12.772657]  ? __pfx_worker_thread+0x10/0x10
[   12.772657]  kthread+0xe3/0x110
[   12.772657]  ? __pfx_kthread+0x10/0x10
[   12.772657]  ret_from_fork+0x31/0x50
[   12.772657]  ? __pfx_kthread+0x10/0x10
[   12.772657]  ret_from_fork_asm+0x1b/0x30
[   12.772657]  </TASK>
[   12.772657] [martins3:virtio_dummy_recv_cb:214] ------
```

### spin_lock_bh
既然 softirq 也可以放到线程去执行， 那么 spin_lock_bh 如何阻塞 softirq

要记住，disable bh 是希望不要让 softirq 打断 critical region 中代码来执行就可以了。

这并不是什么问题，硬中断来了之后，不让继续调度的机会，
必须立刻回来，不去执行 softirq handler 就可以了。并不是，让 softirq 不能同时执行。

### 中断线程化 和 softirq 的关系
<!-- 06dfbbed-6a61-47d2-9343-4752af235e09 -->

中断线程化 和 softirq 是属于两个体系的，例如这个输出，可以看到 ksoftirqd 线程和中断线程

```txt
ps -elf | grep irq
1 S root          14       2  0  80   0 -     0 -      21:24 ?        00:00:00 [ksoftirqd/0]
1 S root          22       2  0  80   0 -     0 -      21:24 ?        00:00:00 [ksoftirqd/1]
1 S root          27       2  0  80   0 -     0 -      21:24 ?        00:00:00 [ksoftirqd/2]
1 S root          32       2  0  80   0 -     0 -      21:24 ?        00:00:00 [ksoftirqd/3]
1 S root          57       2  0   9   - -     0 -      21:24 ?        00:00:00 [irq/50-pciehp]
1 S root          58       2  0   9   - -     0 -      21:24 ?        00:00:00 [irq/49-ACPI:Ged]
```
中断线程是可以在进程上下文中工作的，其吞吐量要更加小。

```c
static inline int __must_check
request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
	    const char *name, void *dev)
{
	return request_threaded_irq(irq, handler, NULL, flags | IRQF_COND_ONESHOT, name, dev);
}
```
request_threaded_irq 的第三个参数就是使用的线程，检查一下，其实使用者并不多。
对于每注册的一个任务，会创建一个新的 kernel thread 来等待任务的执行，当中断到的时候，开始执行任务。

M2 asahi linux 上找到了一个中断 thread

broadcom 的无线网卡:
```txt
sudo bpftrace -e "kprobe:brcmf_pcie_isr_thread { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    brcmf_pcie_isr_thread+0
    irq_thread+360
    kthread+244
    ret_from_fork+16
]: 697
```
对应的代码为:
drivers/net/wireless/broadcom/brcm80211/brcmfmac/pcie.c

```c
static int brcmf_pcie_request_irq(struct brcmf_pciedev_info *devinfo)
{
	struct pci_dev *pdev = devinfo->pdev;
	struct brcmf_bus *bus = dev_get_drvdata(&pdev->dev);

	brcmf_pcie_intr_disable(devinfo);

	brcmf_dbg(PCIE, "Enter\n");

	pci_enable_msi(pdev);
	if (request_threaded_irq(pdev->irq, brcmf_pcie_quick_check_isr,
				 brcmf_pcie_isr_thread, IRQF_SHARED,
				 "brcmf_pcie_intr", devinfo)) {
		pci_disable_msi(pdev);
		brcmf_err(bus, "Failed to request IRQ %d\n", pdev->irq);
		return -EIO;
	}
	devinfo->irq_allocated = true;
	return 0;
}
```

另外的一个经典例子就是 amd 中的注册的中断线程化的:
```c
static int iommu_setup_msi(struct amd_iommu *iommu)
{
	int r;

	r = pci_enable_msi(iommu->dev);
	if (r)
		return r;

	r = request_threaded_irq(iommu->dev->irq,
				 amd_iommu_int_handler,
				 amd_iommu_int_thread,
				 0, "AMD-Vi",
				 iommu);

	if (r) {
		pci_disable_msi(iommu->dev);
		return r;
	}

	return 0;
}
```

#### CONFIG_IRQ_FORCED_THREADING 的影响是什么

影响很多，已经看不懂了。

### RPS : 将 softirq 迁移到其他的 CPU 中

- [ ] https://stackoverflow.com/questions/45066524/can-hard-and-soft-irq-for-the-same-network-packet-be-executed-on-different-cpu-c
  - 深入分析了 RPS

> Since softirqs can reschedule themselves or other interrupts can occur that reschedules them, they can potentially lead to (temporary) process starvation if checks are not put into place. Currently, the Linux kernel does not allow running soft irqs for more than MAX_SOFTIRQ_TIME or rescheduling for more than MAX_SOFTIRQ_RESTART consecutive times.

## 可以一看
https://arthurchiao.art/blog/linux-irq-softirq-zh/

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
