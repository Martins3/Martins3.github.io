## 文档
https://www.kernel.org/doc/html/latest//RCU/stallwarn.html

## rcu statll 基本实现原理
<!-- e95fdea2-699c-4737-86b5-1d5b8d98cb8e -->

(还是非常粗略的)

- update_process_times
  - rcu_sched_clock_irq
    - rcu_pending
      - check_cpu_stall : 检查 jiffies 的变化的
    - invoke_rcu_core : 只有 rcu_pending 采取检查

参考下 ds 的意见:
1. 如果 CPU 长时间卡住，qsmask bit 保持 set，GP 无法推进到下一阶段。
```c
	self_detected = READ_ONCE(rnp->qsmask) & rdp->grpmask;
```

2. rcu_state.jiffies_stall，表示预期检测 stall 的时间点，通常基于 grace period (GP)
开始时间 (gp_start) 加上一个超时阈值

似乎正确
- rcu_gp_fqs_loop
  - rcu_gp_fqs 中结果是:
```txt
WRITE_ONCE(rcu_state.jiffies_stall, jiffies + rcu_jiffies_till_stall_check());
```

联合分析:


```c
	gs1 = READ_ONCE(rcu_state.gp_seq);
	smp_rmb(); /* Pick up ->gp_seq first... */
	js = READ_ONCE(rcu_state.jiffies_stall);
	smp_rmb(); /* ...then ->jiffies_stall before the rest... */
	gps = READ_ONCE(rcu_state.gp_start);
	smp_rmb(); /* ...and finally ->gp_start before ->gp_seq again. */
	gs2 = READ_ONCE(rcu_state.gp_seq);

  // 1. 如果这段时间 gp_seq 变化了，那么显然不行
  // 2. 如果当前的时间 j (刚刚从 jiffies 中获取的) 没到 js ，那么不用看
  // 3. 如果 gps 也就是 gp 开始的时间大于等于 js ，说明新的 gp 开始了，所以跳过
  // 4. 暂时看不懂
	if (gs1 != gs2 ||
	    ULONG_CMP_LT(j, js) ||
	    ULONG_CMP_GE(gps, js) ||
	    !rcu_seq_state(gs2))
		return; /* No stall or GP completed since entering function. */
	rnp = rdp->mynode;
	jn = jiffies + ULONG_MAX / 2;
	self_detected = READ_ONCE(rnp->qsmask) & rdp->grpmask;

  // 如果到达这里，就说明由于 gp 迟迟没有完成，导致 rcu_state.jiffies_stall 没有被刷新
  // 机器的 jiffies 已经赶上 rcu_state.jiffies_stall 了，所以系统中的
  // cmpxchg(&rcu_state.jiffies_stall, js, jn) == js 应该是为了防止同时更新的
	if (rcu_gp_in_progress() &&
	    (self_detected || ULONG_CMP_GE(j, js + RCU_STALL_RAT_DELAY)) &&
	    cmpxchg(&rcu_state.jiffies_stall, js, jn) == js) {
```


可以的触发原因
1. CPU 被中断禁用过久
2. 调度延迟
3. 虚拟机时间跳变，例如修改
4. 虚拟机暂停分为两个类型
    1. 通过 qemu 的 hmp 的 stop / cont (这个不会导致问题，qemu 会自动的修复时间的变化)
    2. 使用 gdb 来暂停 qemu

配套测试
```sh
for _ in $(seq 1 1000); do
	sleep 1
	date
done
```
如果 hmp 暂停，date 输出是连续的，也就说，时间和现实世界落后的。

### 时钟中断中, invoke_rcu_core 来驱动 rcu softirq

代码在:
```c
#include "tree_stall.h"
```

在 6.14 的可以观测到:
```txt
@[
    rcu_sched_clock_irq+5
    update_process_times+116
    tick_sched_handle+33
    tick_nohz_highres_handler+111
    __hrtimer_run_queues+134
    hrtimer_interrupt+248
    __sysvec_apic_timer_interrupt+77
    sysvec_apic_timer_interrupt+111
    asm_sysvec_apic_timer_interrupt+26
    cpuidle_enter_state+220
    cpuidle_enter+45
    do_idle+500
    cpu_startup_entry+42
    start_secondary+291
    secondary_startup_64_no_verify+399
]: 360
@[]: 1388
```

3.10 中的机制到现在这个机制都是这样的，例如这样的效果:
```txt
dump_cpu_task+0x39/0x70
rcu_dump_cpu_stacks+0x90/0xd0
rcu_check_callbacks+0x442/0x730
? tcp_v4_rcv+0x87/0x990
? tick_sched_do_timer+0x50/0x50
update_process_times+0x46/0x80
tick_sched_handle+0x30/0x70
tick_sched_timer+0x39/0x80
__hrtimer_run_queues+0xd6/0x260
hrtimer_interrupt+0xaf/0x1d0
local_apic_timer_interrupt+0x3b/0x60
smp_apic_timer_interrupt+0x43/0x60
apic_timer_interrupt+0x162/0x170
? __getnstimeofday64+0x90/0xd0
? __getnstimeofday64+0x3f/0xd0
getnstimeofday64+0xe/0x30
ktime_get_real+0x25/0x70
netif_receive_skb_internal+0x76/0xc0
napi_gro_receive+0xd8/0x100
virtnet_poll+0x265/0x750 [virtio_net]
net_rx_action+0x26f/0x390
__do_softirq+0xf5/0x280
call_softirq+0x1c/0x30
do_softirq+0x65/0xa0
irq_exit+0x105/0x110
do_IRQ+0x56/0xf0
```

```txt
@[
    invoke_rcu_core+0
    update_process_times+132
    tick_nohz_handler+168
    __hrtimer_run_queues+980
    hrtimer_interrupt+236
    arch_timer_handler_virt+52
    handle_percpu_devid_irq+176
    handle_irq_desc+60
    generic_handle_domain_irq+36
    gic_handle_irq+84
    call_on_irq_stack+48
    do_interrupt_handler+136
    el1_interrupt+52
    el1h_64_irq_handler+24
    el1h_64_irq+108
    default_idle_call+180
    do_idle+540
    cpu_startup_entry+60
    secondary_start_kernel+312
    __secondary_switched+192
]: 475
```

```txt
@[
    invoke_rcu_core+0
    rcu_core_si+24
    handle_softirqs+304
    run_ksoftirqd+80
    smpboot_thread_fn+516
    kthread+340
    ret_from_fork+16
]: 18
```

居然也可以放到 thread 中执行
```c
/*
 * Wake up this CPU's rcuc kthread to do RCU core processing.
 */
static void invoke_rcu_core(void)
{
	if (!cpu_online(smp_processor_id()))
		return;
	if (use_softirq)
		raise_softirq(RCU_SOFTIRQ);
	else
		invoke_rcu_core_kthread();
}
```

最后就是为了执行 rcu_core ，在 rcu_core ，会去执行各种
之前挂载的 callback



## 具体配套测试在 concurrent/rcustall.c 中

### 为什么 4.19 使用暂停的时候，无法触发 rcustall
<!-- 3bdf1ab2-75a8-4dfd-90c6-3c9d52f53e27 -->

修改为:

   kvmclock_vm_state_change 中
      -        data.clock = s->clock;
 	    +        data.clock = s->clock + 600 *  NANOSECONDS_PER_SECOND;


```txt
[ 2027.284294] rcu: INFO: rcu_sched detected stalls on CPUs/tasks:
```

似乎原因在这里，按道理，但是这里无法触发
```c
void pvclock_touch_watchdogs(void)
{
	touch_softlockup_watchdog_sync();
	clocksource_touch_watchdog();
	rcu_cpu_stall_reset();
	reset_hung_task_detector();
}
```

那么在 workqueue 中应该也有一个这样的检测才可以，
想不到 这个东西这么晚才加入的。似乎之前 4.19 代码通过 pvclock_touch_watchdogs 就可以了
这里是没有必要的。但是，我们的确测试到了，如果 6.14.2 中去掉这个 patch ，
那么时间跳变还是可能的。

似乎现在的情况是这样的，如果添加了
kvm_check_and_clear_guest_paused ，
那么一定可以屏蔽掉问题，如果不添加，那么有可能。

```txt
History:        #0
Commit:         ccfc9dd6914feaa9a81f10f9cce56eb0f7712264
Author:         Sergey Senozhatsky <senozhatsky@chromium.org>
Committer:      Paul E. McKenney <paulmck@kernel.org>
Author Date:    Fri 21 May 2021 11:56:23 PM CST
Committer Date: Sat 07 Aug 2021 04:41:48 AM CST

rcu/tree: Handle VM stoppage in stall detection

The soft watchdog timer function checks if a virtual machine
was suspended and hence what looks like a lockup in fact
is a false positive.

This is what kvm_check_and_clear_guest_paused() does: it
tests guest PVCLOCK_GUEST_STOPPED (which is set by the host)
and if it's set then we need to touch all watchdogs and bail
out.

Watchdog timer function runs from IRQ, so PVCLOCK_GUEST_STOPPED
check works fine.

There is, however, one more watchdog that runs from IRQ, so
watchdog timer fn races with it, and that watchdog is not aware
of PVCLOCK_GUEST_STOPPED - RCU stall detector.

apic_timer_interrupt()
 smp_apic_timer_interrupt()
  hrtimer_interrupt()
   __hrtimer_run_queues()
    tick_sched_timer()
     tick_sched_handle()
      update_process_times()
       rcu_sched_clock_irq()

This triggers RCU stalls on our devices during VM resume.

If tick_sched_handle()->rcu_sched_clock_irq() runs on a VCPU
before watchdog_timer_fn()->kvm_check_and_clear_guest_paused()
then there is nothing on this VCPU that touches watchdogs and
RCU reads stale gp stall timestamp and new jiffies value, which
makes it think that RCU has stalled.

Make RCU stall watchdog aware of PVCLOCK_GUEST_STOPPED and
don't report RCU stalls when we resume the VM.

Signed-off-by: Sergey Senozhatsky <senozhatsky@chromium.org>
Signed-off-by: Signed-off-by: Paul E. McKenney <paulmck@kernel.org>
```



## 其他的记录
### 使用 QEMU 调试的过程中，Guest 首先一致卡在 idle 中，然后触发这个 bug

```txt
[ 4192.186591] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
[ 4192.187264]  (detected by 7, t=42141 jiffies, g=10393, q=61 ncpus=8)
[ 4192.187264] rcu: All QSes seen, last rcu_preempt kthread activity 42025 (4298858205-4298816180), jiffies_till_next_fqs=3, root ->qsmask 0x0
[ 4192.187264] rcu: rcu_preempt kthread timer wakeup didn't happen for 42031 jiffies! g10393 f0x2 RCU_GP_WAIT_FQS(5) ->state=0x200
[ 4192.187264] rcu:     Possible timer handling issue on cpu=1 timer-softirq=2124
[ 4192.187264] rcu: rcu_preempt kthread starved for 42048 jiffies! g10393 f0x2 RCU_GP_WAIT_FQS(5) ->state=0x200 ->cpu=1
[ 4192.187264] rcu:     Unless rcu_preempt kthread gets sufficient CPU time, OOM is now expected behavior.
[ 4192.187264] rcu: RCU grace-period kthread stack dump:
[ 4192.187264] task:rcu_preempt     state:R stack:14976 pid:   14 ppid:     2 flags:0x00004000
[ 4192.187264] Call Trace:
[ 4192.187264]  <TASK>
[ 4192.187264]  __schedule+0x2a4/0x7a0
[ 4192.187264]  ? rcu_gp_cleanup+0x4f0/0x4f0
[ 4192.187264]  schedule+0x55/0xa0
[ 4192.187264]  schedule_timeout+0x83/0x150
[ 4192.187264]  ? _raw_spin_unlock_irqrestore+0x16/0x30
[ 4192.187264]  ? timer_migration_handler+0x90/0x90
[ 4192.187264]  rcu_gp_fqs_loop+0x129/0x5d0
[ 4192.187264]  rcu_gp_kthread+0x19b/0x240
[ 4192.187264]  kthread+0xe0/0x110
[ 4192.187264]  ? kthread_complete_and_exit+0x20/0x20
[ 4192.187264]  ret_from_fork+0x1f/0x30
[ 4192.187264]  </TASK>
[ 4192.187264] rcu: Stack dump where RCU GP kthread last ran:
[ 4192.187264] Sending NMI from CPU 7 to CPUs 1:
[ 4150.290203] NMI backtrace for cpu 1 skipped: idling at default_idle+0xb/0x10
[ 4213.292264] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
[ 4213.293260]  (detected by 0, t=63241 jiffies, g=10393, q=151 ncpus=8)
[ 4213.293260] rcu: All QSes seen, last rcu_preempt kthread activity 63126 (4298879306-4298816180), jiffies_till_next_fqs=3, root ->qsmask 0x0
[ 4213.293260] rcu: rcu_preempt kthread timer wakeup didn't happen for 63133 jiffies! g10393 f0x2 RCU_GP_WAIT_FQS(5) ->state=0x200
[ 4213.293260] rcu:     Possible timer handling issue on cpu=1 timer-softirq=2124
[ 4213.293260] rcu: rcu_preempt kthread starved for 63151 jiffies! g10393 f0x2 RCU_GP_WAIT_FQS(5) ->state=0x200 ->cpu=1
[ 4213.293260] rcu:     Unless rcu_preempt kthread gets sufficient CPU time, OOM is now expected behavior.
[ 4213.293260] rcu: RCU grace-period kthread stack dump:
[ 4213.293260] task:rcu_preempt     state:R stack:14976 pid:   14 ppid:     2 flags:0x00004000
[ 4213.293260] Call Trace:
[ 4213.293260]  <TASK>
[ 4213.293260]  __schedule+0x2a4/0x7a0
[ 4213.293260]  ? rcu_gp_cleanup+0x4f0/0x4f0
[ 4213.293260]  schedule+0x55/0xa0
[ 4213.293260]  schedule_timeout+0x83/0x150
[ 4213.293260]  ? _raw_spin_unlock_irqrestore+0x16/0x30
[ 4213.293260]  ? timer_migration_handler+0x90/0x90
[ 4213.293260]  rcu_gp_fqs_loop+0x129/0x5d0
[ 4213.293260]  rcu_gp_kthread+0x19b/0x240
[ 4213.293260]  kthread+0xe0/0x110
[ 4213.293260]  ? kthread_complete_and_exit+0x20/0x20
[ 4213.293260]  ret_from_fork+0x1f/0x30
[ 4213.293260]  </TASK>
[ 4213.293260] rcu: Stack dump where RCU GP kthread last ran:
[ 4213.293260] Sending NMI from CPU 0 to CPUs 1:
[ 4192.297288] NMI backtrace for cpu 1 skipped: idling at default_idle+0xb/0x10
```

在 mount 的时候暂停了特别久之后，出现了这个 bug:

```txt
[ 5973.259566] rcu: INFO: rcu_preempt self-detected stall on CPU
[ 5973.259566] rcu:     3-...!: (4 ticks this GP) idle=f824/0/0x1 softirq=4614/4614 fqs=0
[ 5973.259566]  (t=39404443 jiffies g=18581 q=33 ncpus=8)
[ 5973.259566] NMI backtrace for cpu 3
[ 5973.259566] CPU: 3 PID: 0 Comm: swapper/3 Not tainted 6.0.0-rc1-00399-g15b3f48a4339 #23
[ 5973.259566] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.16.0-0-gd239552ce722-prebuilt.qemu.org 04/01/2014

[ 5973.259566] Call Trace:
[ 5973.259566]  <IRQ>
[ 5973.259566]  dump_stack_lvl+0x34/0x48
[ 5973.259566]  nmi_cpu_backtrace.cold+0x30/0x70
[ 5973.259566]  ? lapic_can_unplug_cpu+0x80/0x80
[ 5973.259566]  nmi_trigger_cpumask_backtrace+0xc6/0xe0
[ 5973.259566]  rcu_dump_cpu_stacks+0x106/0x150
[ 5973.259566]  rcu_sched_clock_irq.cold+0x63/0x2f2
[ 5973.259566]  ? raw_notifier_call_chain+0x3c/0x60
[ 5973.259566]  ? timekeeping_update+0xab/0x280
[ 5973.259566]  ? _raw_spin_unlock_irqrestore+0x16/0x30
[ 5973.259566]  ? timekeeping_advance+0x373/0x550
[ 5973.259566]  update_process_times+0x5d/0x90
[ 5973.259566]  tick_sched_handle+0x2f/0x40
[ 5973.259566]  tick_sched_timer+0x6c/0x80
[ 5973.259566]  ? tick_sched_do_timer+0x90/0x90
[ 5973.259566]  __hrtimer_run_queues+0x122/0x2c0
[ 5973.259566]  hrtimer_interrupt+0x101/0x220
[ 5973.259566]  __sysvec_apic_timer_interrupt+0x77/0x160
[ 5973.259566]  sysvec_apic_timer_interrupt+0x9d/0xd0
[ 5973.259566]  </IRQ>
[ 5973.259566]  <TASK>
[ 5973.259566]  asm_sysvec_apic_timer_interrupt+0x16/0x20
[ 5973.259566] RIP: 0010:default_idle+0xb/0x10
[ 5973.259566] Code: 0a 00 00 eb 81 4c 89 ef e8 d2 b3 84 ff eb c4 e8 bb 65 ff ff cc cc cc cc cc cc cc cc cc cc cc 66 90 0f 00 2d 77 68 56 00 fb f4 <c3> cc cc cc c
[ 5973.259566] RSP: 0018:ffffc9000009bee8 EFLAGS: 00000256
[ 5973.259566] RAX: ffffffff81eb71d0 RBX: 0000000000000003 RCX: 0000000000000000
[ 5973.259566] RDX: 0000000000000001 RSI: ffffffff82813bd9 RDI: 00000000000ef81c
[ 5973.259566] RBP: ffff8883002b2e00 R08: 0000039833046055 R09: 0000000000000115
[ 5973.259566] R10: 0000000000000001 R11: 0000000000000001 R12: 0000000000000000
[ 5973.259566] R13: 0000000000000000 R14: 0000000000000000 R15: 0000000000000000
[ 5973.259566]  ? __cpuidle_text_start+0x8/0x8
[ 5973.259566]  default_idle_call+0x2c/0xe0
[ 5973.259566]  do_idle+0x1ed/0x260
[ 5973.259566]  cpu_startup_entry+0x14/0x20
[ 5973.259566]  start_secondary+0xe8/0xf0
[ 5973.259566]  secondary_startup_64_no_verify+0xe0/0xeb
[ 5973.259566]  </TASK>
```

## 有办法只是触发 stallwarn ，但是不去触发 softlock watchdog 吗？

一种办法是在 rcustall 中去掉 kvm_check_and_clear_guest_paused 的检测。


## rcustall 从来不会导致 panic
<!-- 3db5c574-8b37-443f-a865-cdb1eb0bf0b2 -->

具体看函数: check_cpu_stall

没有 panic 的选项。

## rcu stall 报错的每一个字段的含义是什么
<!-- 470edd71-db33-4232-96a8-f38b8e9876cb -->

```c
static void print_cpu_stall_info(int cpu)
{
	unsigned long delta;
	bool falsepositive;
	struct rcu_data *rdp = per_cpu_ptr(&rcu_data, cpu);
	char *ticks_title;
	unsigned long ticks_value;
	bool rcuc_starved;
	unsigned long j;
	char buf[32];

	/*
	 * We could be printing a lot while holding a spinlock.  Avoid
	 * triggering hard lockup.
	 */
	touch_nmi_watchdog();

	ticks_value = rcu_seq_ctr(rcu_state.gp_seq - rdp->gp_seq);
	if (ticks_value) {
		ticks_title = "GPs behind";
	} else {
		ticks_title = "ticks this GP";
		ticks_value = rdp->ticks_this_gp;
	}
	delta = rcu_seq_ctr(rdp->mynode->gp_seq - rdp->rcu_iw_gp_seq);
	falsepositive = rcu_is_gp_kthread_starving(NULL) &&
			rcu_watching_snap_in_eqs(ct_rcu_watching_cpu(cpu));
	rcuc_starved = rcu_is_rcuc_kthread_starving(rdp, &j);
	if (rcuc_starved)
		// Print signed value, as negative values indicate a probable bug.
		snprintf(buf, sizeof(buf), " rcuc=%ld jiffies(starved)", j);
	pr_err("\t%d-%c%c%c%c: (%lu %s) idle=%04x/%ld/%#lx softirq=%u/%u fqs=%ld%s%s\n",
	       cpu,
	       "O."[!!cpu_online(cpu)],
	       "o."[!!(rdp->grpmask & rdp->mynode->qsmaskinit)],
	       "N."[!!(rdp->grpmask & rdp->mynode->qsmaskinitnext)],
	       !IS_ENABLED(CONFIG_IRQ_WORK) ? '?' :
			rdp->rcu_iw_pending ? (int)min(delta, 9UL) + '0' :
				"!."[!delta],
	       ticks_value, ticks_title,
	       ct_rcu_watching_cpu(cpu) & 0xffff,
	       ct_nesting_cpu(cpu), ct_nmi_nesting_cpu(cpu),
	       rdp->softirq_snap, kstat_softirqs_cpu(RCU_SOFTIRQ, cpu),
	       data_race(rcu_state.n_force_qs) - rcu_state.n_force_qs_gpstart,
	       rcuc_starved ? buf : "",
	       falsepositive ? " (false positive?)" : "");

	print_cpu_stat_info(cpu);
}
```

1. 从这里的 backtrace 看，rcustall 也是使用中断来判断的:
```txt
[ 1009.537062] rcu: INFO: rcu_preempt self-detected stall on CPU
[ 1009.537721] rcu:     2-....: (1 GPs behind) idle=21c4/1/0x4000000000000000 softirq=9503/9503 fqs=95261
[ 1009.538734] rcu:     (t=416031 jiffies g=14801 q=19834 ncpus=3)
[ 1009.539310] CPU: 2 UID: 0 PID: 28 Comm: ksoftirqd/2 Tainted: G           OEL      6.16.4 #14 PREEMPT(full)
[ 1009.539313] Tainted: [O]=OOT_MODULE, [E]=UNSIGNED_MODULE, [L]=SOFTLOCKUP
[ 1009.539315] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[ 1009.539316] RIP: 0010:delay_tsc+0x37/0xa0
[ 1009.539332] Code: ff 05 5d fb 40 01 65 44 8b 0d 59 fb 40 01 0f 01 f9 66 90 48 c1 e2 20 48 89 d7 48 09 c7 eb 21 65 ff 0d 3d fb 40 01 74 57 f3 90 <65> ff 05 32 fb 40 01 65 8b 35 2f fb 40 01 41 39 f1 75 28 41 89 f1
[ 1009.539333] RSP: 0018:ffffc900002ebd00 EFLAGS: 00000207
[ 1009.539336] RAX: 0000021b9cc3def2 RBX: 3ffffffffffffffe RCX: 0000000000000002
[ 1009.539337] RDX: 0000000000564d22 RSI: 0000000000000002 RDI: 0000021b9c6d91d0
[ 1009.539338] RBP: 0000000000000100 R08: 0000000000690a91 R09: 0000000000000002
[ 1009.539339] R10: 0000000000000001 R11: 0000000000000000 R12: ffffffffc05fc330
[ 1009.539340] R13: ffffc900002ebd20 R14: 00000001000ad21c R15: ffffffffc0ee7a40
[ 1009.539342] FS:  0000000000000000(0000) GS:ffff8882b3e3f000(0000) knlGS:0000000000000000
[ 1009.539345] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[ 1009.539346] CR2: 000055a6b90cd320 CR3: 0000000110d0a002 CR4: 0000000000772ef0
[ 1009.539347] PKRU: 55555554
[ 1009.539348] Call Trace:
[ 1009.539350]  <TASK>
[ 1009.539354]  endless_timer_fn+0x6e/0x80 [martins3]
[ 1009.539365]  call_timer_fn+0xa5/0x260
[ 1009.539374]  ? __pfx_endless_timer_fn+0x10/0x10 [martins3]
[ 1009.539377]  __run_timers+0x220/0x2f0
[ 1009.539387]  run_timer_softirq+0x49/0xf0
[ 1009.539391]  handle_softirqs+0xdd/0x420
[ 1009.539397]  ? smpboot_thread_fn+0x25/0x280
[ 1009.539402]  ? __pfx_smpboot_thread_fn+0x10/0x10
[ 1009.539405]  run_ksoftirqd+0x31/0x60
[ 1009.539413]  smpboot_thread_fn+0x125/0x280
[ 1009.539419]  kthread+0x10c/0x230
[ 1009.539424]  ? __pfx_kthread+0x10/0x10
[ 1009.539429]  ? __pfx_kthread+0x10/0x10
[ 1009.539435]  ret_from_fork+0x1b8/0x220
[ 1009.539441]  ? __pfx_kthread+0x10/0x10
[ 1009.539444]  ret_from_fork_asm+0x1a/0x30
[ 1009.539463]  </TASK>
```

> [!NOTE]
> 参考 Deepseeek ，有待验证

第二行的含义:

CPU: The stall was detected on CPU 2.
GPs behind: This means there is 1 grace period (GP) behind in the RCU mechanism, which typically happens when an RCU read-side critical section isn't being finalized in time.
Idle: 21c4/1/0x4000000000000000 represents the CPU’s idle state (in hex). The first value indicates the CPU state, the second one the state of the system, and the third one represents the mask for IRQ activity.
SoftIRQ: 9503/9503 shows the number of soft IRQs pending and handled. It appears there are no pending softirqs, as the number of pending equals the number processed.
Fqs (Forced Quiescent States): 95261 represents how many forced quiescent states have been triggered in the system.

第三行的含义:

t: This is the time in jiffies (416031), which is a kernel time unit.
g: Grace period time, 14801 jiffies.
q: Number of quiescent states, 19834.
ncpus: Number of CPUs in the system (3).

<!-- ds 结束 -->

### softirq
使用 time/timer.c 中的 case 3 测试

```txt
	       rdp->softirq_snap, kstat_softirqs_cpu(RCU_SOFTIRQ, cpu),
```

其中的 kstat_softirqs_cpu(RCU_SOFTIRQ, cpu) 就是当前 softirq

kstat_incr_softirqs_this_cpu

6.14 内核中测试:
```txt
[   62.888148] rcu:     2-....: (1 GPs behind) idle=21bc/0/0x3 softirq=9502/9502 fqs=6292
[  140.891857] rcu:     2-....: (1 GPs behind) idle=21bc/0/0x3 softirq=9502/9502 fqs=24922
[  218.894848] rcu:     2-....: (1 GPs behind) idle=21bc/0/0x3 softirq=9502/9502 fqs=43550
[  296.898608] rcu:     2-....: (1 GPs behind) idle=21bc/0/0x3 softirq=9502/9502 fqs=61986
[  374.903395] rcu:     2-....: (1 GPs behind) idle=21bc/0/0x3 softirq=9502/9502 fqs=80260
[  452.908201] rcu:     2-....: (1 GPs behind) idle=21bc/0/0x3 softirq=9502/9502 fqs=98379
[  530.913012] rcu:     2-....: (1 GPs behind) idle=21bc/0/0x3 softirq=9502/9502 fqs=116358

[  619.514674] rcu:     2-....: (1 GPs behind) idle=21c4/1/0x4000000000000000 softirq=9503/9503 fqs=6020
[  697.519461] rcu:     2-....: (1 GPs behind) idle=21c4/1/0x4000000000000000 softirq=9503/9503 fqs=24055
[  775.524258] rcu:     2-....: (1 GPs behind) idle=21c4/1/0x4000000000000000 softirq=9503/9503 fqs=41857
[  853.528112] rcu:     2-....: (1 GPs behind) idle=21c4/1/0x4000000000000000 softirq=9503/9503 fqs=59398
[  931.532926] rcu:     2-....: (1 GPs behind) idle=21c4/1/0x4000000000000000 softirq=9503/9503 fqs=77271
[ 1009.537721] rcu:     2-....: (1 GPs behind) idle=21c4/1/0x4000000000000000 softirq=9503/9503 fqs=95261
```

4.19 内核中测试:
```txt
[  219.912622] rcu:     2-....: (59913 ticks this GP) idle=dc6/1/0x4000000000000004 softirq=17214/17214 fqs=14167
[  399.912912] rcu:     2-....: (239527 ticks this GP) idle=dc6/1/0x4000000000000004 softirq=17214/17214 fqs=56486
[  579.913161] rcu:     2-....: (419138 ticks this GP) idle=dc6/1/0x4000000000000004 softirq=17214/17214 fqs=99244
[  759.913442] rcu:     2-....: (598754 ticks this GP) idle=dc6/1/0x4000000000000004 softirq=17214/17214 fqs=142348
[  939.915006] rcu:     2-....: (778364 ticks this GP) idle=dc6/1/0x4000000000000004 softirq=17214/17214 fqs=184663
[ 1119.915232] rcu:     2-....: (957968 ticks this GP) idle=dc6/1/0x4000000000000004 softirq=17214/17214 fqs=227694
```

还算符合预期，也就是 timer 的 softirq 卡主之后，导致 rcu 卡主了。


## 经典 ipi 不响应导致的 softlock up
<!-- 6ae34228-1948-4977-974b-3ac010a5ac15 -->

复现方法 : time/hrtimer.c:dead_lock_timer_fn ，也就是，
也就是，首先让 hrtimer 进入到死循环，然后其他的 CPU 想要给这个 CPU 触发 ipi 就会无法等到
返回。

当然，首先会观察到 rcu stall 的检测，这个检测也是很经典的
```txt
[  173.584703] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
[  173.584884] rcu:     15-...0: (1 GPs behind) idle=aa24/1/0x4000000000000000 softirq=1113/1114 fqs=6014
[  173.585294] rcu:     (detected by 4, t=26002 jiffies, g=20189, q=340 ncpus=32)
[  173.585478] Sending NMI from CPU 4 to CPUs 15:
[  173.585483] NMI backtrace for cpu 15
[  173.585486] CPU: 15 UID: 0 PID: 2177 Comm: tee Tainted: G           O        6.18.1-martins3-00001-g344d23e3a12a #32 PREEMPT(full)
[  173.585488] Tainted: [O]=OOT_MODULE
[  173.585488] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[  173.585489] RIP: 0010:__lock_acquire+0x594/0x2200
[  173.585492] Code: 1a e9 75 07 00 00 41 83 c6 01 49 83 c7 28 44 3b b5 f8 0b 00 00 0f 8d 60 07 00 00 4c 89 ee 4c 89 ff e8 00 b5 92 00 85 c0 74 dc <44> 8b 1d f5 e6 4d 01 45 85 db 75 3d 45 85 e4 0f 85 14 08 00 00 0f
[  173.585492] RSP: 0018:ffffc90000438da8 EFLAGS: 00000046
[  173.585494] RAX: 3ab6d40026a7fe15 RBX: ffff888105e34378 RCX: 0000000000000100
[  173.585494] RDX: 0000000026a7fe15 RSI: 0000000059f47e7c RDI: 00000000ffe5b241
[  173.585495] RBP: ffff888105e33700 R08: 00000000a5f133c5 R09: 0000000000000068
[  173.585495] R10: 0000000000000003 R11: 0000000000000003 R12: 0000000000000003
[  173.585496] R13: 0000000000000000 R14: 0000000000000068 R15: 0000000000000001
[  173.585496] FS:  00007eff87ab8740(0000) GS:ffff8882b4b4c000(0000) knlGS:0000000000000000
[  173.585498] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[  173.585499] CR2: 00007ffdb3f08878 CR3: 000000016168b000 CR4: 00000000000006f0
[  173.585499] Call Trace:
[  173.585500]  <IRQ>
[  173.585502]  lock_acquire+0xbe/0x2e0
[  173.585504]  ? hrtimer_try_to_cancel.part.0+0x24/0x130
[  173.585506]  ? hrtimer_try_to_cancel.part.0+0x50/0x130
[  173.585508]  ? __pfx_dead_lock_timer_fn+0x10/0x10 [martins3]
[  173.585511]  _raw_spin_lock_irqsave+0x3c/0x60
[  173.585513]  ? hrtimer_try_to_cancel.part.0+0x24/0x130
[  173.585514]  hrtimer_try_to_cancel.part.0+0x24/0x130
[  173.585516]  ? __pfx_dead_lock_timer_fn+0x10/0x10 [martins3]
[  173.585516]  hrtimer_cancel+0x21/0x40
[  173.585518]  dead_lock_timer_fn+0x15/0x20 [martins3]
[  173.585519]  __hrtimer_run_queues+0x20b/0x420
[  173.585521]  hrtimer_interrupt+0x118/0x260
[  173.585523]  __sysvec_apic_timer_interrupt+0x6a/0x190
[  173.585524]  sysvec_apic_timer_interrupt+0x6c/0x90
[  173.585527]  </IRQ>
[  173.585527]  <TASK>
[  173.585527]  asm_sysvec_apic_timer_interrupt+0x1a/0x20
[  173.585528] RIP: 0010:_raw_spin_unlock_irqrestore+0x36/0x70
[  173.585529] Code: f5 53 48 8b 74 24 10 48 89 fb 48 83 c7 18 e8 01 4d 6c ff 48 89 df e8 19 7c 6c ff f7 c5 00 02 00 00 75 17 9c 58 f6 c4 02 75 2b <65> ff 0d f3 6e 1c 01 74 16 5b 5d c3 cc cc cc cc e8 a5 10 7b ff 9c
[  173.585530] RSP: 0018:ffffc9000bdb7dd0 EFLAGS: 00000246
[  173.585531] RAX: 0000000000000002 RBX: ffff8882379dc540 RCX: 0000000000000080
[  173.585531] RDX: 0000000000000000 RSI: 0000000000000000 RDI: ffffffff81cc114b
[  173.585531] RBP: 0000000000000287 R08: 0000000000000001 R09: 0000000000000000
[  173.585532] R10: 0000000000000001 R11: 0000000000000000 R12: 0000000000000002
[  173.585532] R13: ffff888102683600 R14: ffffc9000bdb7e78 R15: ffff88810616b220
[  173.585534]  ? _raw_spin_unlock_irqrestore+0x4b/0x70
[  173.585535]  test_hrtimer+0x53/0xa0 [martins3]
[  173.585536]  hrtimer_store.cold+0x4e/0x93 [martins3]
[  173.585537]  kernfs_fop_write_iter+0x166/0x240
[  173.585540]  vfs_write+0x266/0x590
[  173.585542]  ksys_write+0x71/0xf0
[  173.585543]  do_syscall_64+0x74/0xfa0
[  173.585544]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
[  173.585545] RIP: 0033:0x7eff87b2977e
[  173.585546] Code: 4d 89 d8 e8 d4 bc 00 00 4c 8b 5d f8 41 8b 93 08 03 00 00 59 5e 48 83 f8 fc 74 11 c9 c3 0f 1f 80 00 00 00 00 48 8b 45 10 0f 05 <c9> c3 83 e2 39 83 fa 08 75 e7 e8 13 ff ff ff 0f 1f 00 f3 0f 1e fa
[  173.585546] RSP: 002b:00007ffdb3f086b0 EFLAGS: 00000202 ORIG_RAX: 0000000000000001
[  173.585547] RAX: ffffffffffffffda RBX: 0000000000000002 RCX: 00007eff87b2977e
[  173.585548] RDX: 0000000000000002 RSI: 00007ffdb3f08860 RDI: 0000000000000003
[  173.585548] RBP: 00007ffdb3f086c0 R08: 0000000000000000 R09: 0000000000000000
[  173.585548] R10: 0000000000000000 R11: 0000000000000202 R12: 0000000000000002
[  173.585549] R13: 00007ffdb3f08860 R14: 00005637316d12c0 R15: 00005637316d12c0
[  173.585551]  </TASK>
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```text
rcu:     15-...0: (1 GPs behind) idle=aa24/1/0x4000000000000000 softirq=1113/1114 fqs=6014
```

- `15-...0`
	* **CPU 15**
	* `...0` 表示该 CPU 上没有可识别的普通进程（通常是 idle 或内核上下文）
- `(1 GPs behind)`
	* **GPs = Grace Periods**
	* 该 CPU **落后了 1 个 RCU 宽限期**
	* 表示其他 CPU 已经推进了 RCU 状态，而 CPU 15 仍未确认 quiescent state
- `idle=aa24/1/0x4000000000000000`
	- 这是 RCU 打印的 **CPU idle 状态位掩码**，通常只对内核开发者有直接意义：
- `softirq=1113/1114`
	* softirq 执行计数器：
	  * **1113：当前 softirq 执行次数**
	  * **1114：期望的执行次数**
- `fqs=6014`
	* **FQS = Forced Quiescent States**
	* RCU 已经尝试 **6014 次强制 quiescent state**

```text
rcu:     (detected by 4, t=26002 jiffies, g=20189, q=340 ncpus=32)
```

- `detected by 4`
	- CPU 4 检测到的

- `t=26002 jiffies`
	* stall 持续时间：**26002 jiffies**
	* 假设 HZ=1000 → 约 **26 秒**
	* 假设 HZ=250 → 约 **104 秒**
- `g=20189`
	* 当前 **RCU grace period 编号**
	* 主要用于内部调试和比对
- `q=340`
	* RCU 等待的回调/状态队列大小
	* 数值不大，但因为 GP 卡住，无法释放
- `ncpus=32`
	* 系统共有 **32 个 CPU**


```txt
[  252.208794] watchdog: BUG: soft lockup - CPU#1 stuck for 96s! [kworker/1:1:293]
[  252.208820] irq event stamp: 529306
[  252.208820] hardirqs last  enabled at (529305): [<ffffffff8100148a>] asm_sysvec_apic_timer_interrupt+0x1a/0x20
[  252.208822] hardirqs last disabled at (529306): [<ffffffff81cae42e>] sysvec_apic_timer_interrupt+0xe/0x90
[  252.208825] softirqs last  enabled at (528212): [<ffffffff812d9ae6>] __irq_exit_rcu+0xa6/0xd0
[  252.208825] softirqs last disabled at (528207): [<ffffffff812d9ae6>] __irq_exit_rcu+0xa6/0xd0
[  252.208827] CPU: 1 UID: 0 PID: 293 Comm: kworker/1:1 Tainted: G           O L      6.18.1-martins3-00001-g344d23e3a12a #32 PREEMPT(full)
[  252.208828] Tainted: [O]=OOT_MODULE, [L]=SOFTLOCKUP
[  252.208829] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[  252.208829] Workqueue: events netstamp_clear
[  252.208831] RIP: 0010:smp_call_function_many_cond+0x1b2/0x750
[  252.208832] Code: d0 48 89 df e8 5f f8 3e 00 3b 05 69 47 45 01 73 26 48 63 d0 49 8b 34 24 48 03 34 d5 c0 de 1f 82 8b 56 08 83 e2 01 74 0a f3 90 <8b> 4e 08 83 e1 01 75 f6 83 c0 01 eb c1 48 83 c4 50 5b 5d 41 5c 41
[  252.208833] RSP: 0018:ffffc9000160fcc8 EFLAGS: 00000202
[  252.208834] RAX: 000000000000000f RBX: ffff88823766de48 RCX: 0000000000000001
[  252.208834] RDX: 0000000000000001 RSI: ffff8882379f3e00 RDI: 000000000000000f
[  252.208835] RBP: 0000000000000000 R08: ffff88823766de48 R09: 0000000000000000
[  252.208835] R10: 0000000000000001 R11: 0000000000000000 R12: ffff88823766de40
[  252.208835] R13: 0000000000000001 R14: 0000000000000001 R15: 0000000000000001
[  252.208836] FS:  0000000000000000(0000) GS:ffff8882b47cc000(0000) knlGS:0000000000000000
[  252.208837] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[  252.208838] CR2: 000056515f8a4098 CR3: 000000000263c000 CR4: 00000000000006f0
[  252.208839] Call Trace:
[  252.208840]  <TASK>
[  252.208841]  ? __pfx_do_sync_core+0x10/0x10
[  252.208845]  ? tpacket_rcv+0x3ff/0xce0 [af_packet]
[  252.208847]  on_each_cpu_cond_mask+0x24/0x40
[  252.208848]  smp_text_poke_batch_finish+0x1b8/0x4b0
[  252.208851]  arch_jump_label_transform_apply+0x1c/0x30
[  252.208852]  jump_label_update+0x4b/0x1f0
[  252.208855]  static_key_enable_cpuslocked+0x65/0xa0
[  252.208856]  static_key_enable+0x1a/0x20
[  252.208857]  process_one_work+0x1f8/0x580
[  252.208861]  worker_thread+0x1ce/0x3c0
[  252.208862]  ? __pfx_worker_thread+0x10/0x10
[  252.208863]  kthread+0x10f/0x230
[  252.208864]  ? __pfx_kthread+0x10/0x10
[  252.208866]  ? __pfx_kthread+0x10/0x10
[  252.208867]  ret_from_fork+0x21e/0x280
[  252.208867]  ? __pfx_kthread+0x10/0x10
[  252.208868]  ret_from_fork_asm+0x1a/0x30
[  252.208873]  </TASK>
```

1. 但是，我不理解的是，为什么这个场景没有触发 hard lockup ，其实这个是典型的 hard lockup 场景。
2. 更重要的是:
	- 那么，一个 CPU 执行了什么，就会刷新自己的 grace period ，然后不去做什么，又会被检查到
	- rcu:     15-...0: (1 GPs behind) idle=aa24/1/0x4000000000000000 softirq=1113/1114 fqs=6014 是什么意思?

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
