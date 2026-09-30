## soft lockup
- https://www.kernel.org/doc/html/latest//admin-guide/lockup-watchdogs.html
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_monitoring_and_updating_the_kernel/keeping-kernel-panic-parameters-disabled-in-virtualized-environments_managing-monitoring-and-updating-the-kernel#doc-wrapper

- soft lockup : 如果一个进程 20s 独占 CPU，让其他的 task 不能使用
- hard lockup : 如果一个进程 20s 独占 CPU，连中断都不可以进入

If any CPU in the system does not receive any hrtimer interrupt during that time the
'hardlockup detector' (the handler for the NMI perf event) will generate a kernel warning or call panic, depending on the configuration.

## sysfs 和 kernel parameter 的配置

参数:
nmi_watchdog=panic
softlockup_panic

在 /proc/sys/kernel 下的 option

|------------|--------------------------------|
| hardlockup | nmi_watchdog hardlockup_panic  |
| softlockup | soft_watchdog softlockup_panic |

## 源码

kernel/watchdog.c
kernel/watchdog_hld.c
driver/watchdog

## softlockup 实现原理
<!-- d75f6d78-9e71-4d6d-8923-9efafef98d30 -->

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



第一步 ：周期性的 timer 触发
32 core 的机器观测 10s ，一共 64 个

```txt
@[
    watchdog_timer_fn+5
    __hrtimer_run_queues+133
    hrtimer_interrupt+250
    __sysvec_apic_timer_interrupt+85
    sysvec_apic_timer_interrupt+110
    asm_sysvec_apic_timer_interrupt+26
    default_idle+15
    default_idle_call+63
    do_idle+466
    cpu_startup_entry+41
  C  start_secondary+286
    common_startup_64+318
]: 64
```

第二步，timer 希望让这个 CPU 来执行一个 stop 任务 ，其中的逻辑是 softlockup_fn
timer 让 stop 来执行:
```txt
@[
    softlockup_fn+5
    cpu_stopper_thread+139
    smpboot_thread_fn+375
    kthread+220
    ret_from_fork+49
    ret_from_fork_asm+26
]: 64
```
在 softlockup_fn 中，调用 update_touch_ts() 也就是:
```c
__this_cpu_write(watchdog_report_ts, get_timestamp());
__this_cpu_write(watchdog_touch_ts, get_timestamp());
```

才知道 stop 是最高优先级:
[What is the use of stop_sched_class in linux kernel](https://stackoverflow.com/questions/15399782/what-is-the-use-of-stop-sched-class-in-linux-kernel)

实现的原理: 如果 stop 进程都无法被调度，那么认为是出现了 softlockup

## hardlockup
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

基本的原理: 注册时钟中断来证明系统是正常的，如果 20s 中无法接受到之前注册的时钟中断，那么报告错误。

- watchdog_timer_fn
 - watchdog_interrupt_count : 来刷新 hardlockup ，来说明时钟时钟是来的

## 想不到 reboot 的时候还存在这个 bug
https://patchwork.kernel.org/project/linux-watchdog/patch/1650874932-18407-3-git-send-email-liuxp11@chinatelecom.cn/

## nohz_full 会让 softlock 的检查跳过

https://sysctl-explorer.net/kernel/watchdog_cpumask/

毕竟，softlock 是通过 watchdog 触发的。


## 想不到还有参数来控制
```c
bool noirqdebug __read_mostly;

int noirqdebug_setup(char *str)
{
	noirqdebug = 1;
	printk(KERN_INFO "IRQ lockup detection disabled\n");

	return 1;
}

__setup("noirqdebug", noirqdebug_setup);
module_param(noirqdebug, bool, 0644);
MODULE_PARM_DESC(noirqdebug, "Disable irq lockup detection when true");
```
不过一般是关闭的

/sys/module/spurious/parameters/noirqdebug

想不到还有这个模块?

## 其他的 watchdog
### workqueue 
/sys/module/workqueue/parameters/panic_on_stall

### pip


```txt
        smp.panic_on_ipistall= [KNL]
                        If a csd_lock_timeout extends for more than
                        the specified number of milliseconds, panic the
                        system.  By default, let CSD-lock acquisition
                        take as long as they take.  Specifying 300,000
                        for this value provides a 5-minute timeout.
```

## 奇怪现象
### CPU 的 steal 会导致虚拟机 softlock

在 mac 中，如果两个去构建 make -j32 来构建 llvm-project ，结果发现 guest os 直接挂掉了。
```txt
Authorized users only. All activities may be monitored and reported.
localhost login: [233945.014685] capability: warning: `yum' uses 32-bit capabilities (legacy support in use)
[322140.914437] watchdog: Watchdog detected hard LOCKUP on cpu 4
[322151.133370] watchdog: BUG: soft lockup - CPU#4 stuck for 21s! [kworker/4:2:54901]
[322151.926479] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
[322151.926484] rcu:    4-...!: (24 ticks this GP) idle=5cd4/1/0x4000000000000000 softirq=868844/868845 fqs=1136
[322151.926487] rcu:    (detected by 0, t=5252 jiffies, g=1890621, q=429 ncpus=8)
[322151.926489] Sending NMI from CPU 0 to CPUs 4:
[322152.914493] watchdog: Watchdog detected hard LOCKUP on cpu 3
[322162.187416] rcu: rcu_preempt kthread timer wakeup didn't happen for 5316 jiffies! g1890621 f0x0 RCU_GP_WAIT_FQS(5) ->state=0x402
[322162.187423] rcu:    Possible timer handling issue on cpu=3 timer-softirq=588860
[322162.187425] rcu: rcu_preempt kthread starved for 5317 jiffies! g1890621 f0x0 RCU_GP_WAIT_FQS(5) ->state=0x402 ->cpu=3
[322162.187426] rcu:    Unless rcu_preempt kthread gets sufficient CPU time, OOM is now expected behavior.
[322162.187426] rcu: RCU grace-period kthread stack dump:
[322162.187427] task:rcu_preempt     state:I stack:0     pid:15    tgid:15    ppid:2      flags:0x00000008
[322162.187430] Call trace:
[322162.187431]  __switch_to+0x150/0x1fc
[322162.187524]  __schedule+0x528/0x126c
[322162.187525]  schedule+0x74/0x108
[322162.187526]  schedule_timeout+0x98/0x12c
[322162.187540]  rcu_gp_fqs_loop+0x184/0x738
[322162.187567]  rcu_gp_kthread+0x64/0x158
[322162.187569]  kthread+0x104/0x18c
[322162.187583]  ret_from_fork+0x10/0x20
[322162.187584] rcu: Stack dump where RCU GP kthread last ran:
[322162.187584] Sending NMI from CPU 0 to CPUs 3:
[322164.914541] watchdog: Watchdog detected hard LOCKUP on cpu 2
[322173.059034] BUG: workqueue lockup - pool cpus=4 node=0 flags=0x0 nice=0 stuck for 40s!
[322173.059045] Showing busy workqueues and worker pools:
[322173.059046] workqueue events: flags=0x0
[322173.059047]   pwq 2: cpus=0 node=0 flags=0x0 nice=0 active=1 refcnt=2
[322173.059049]     pending: vmstat_shepherd
[322173.059089]   pwq 18: cpus=4 node=0 flags=0x0 nice=0 active=1 refcnt=2
[322173.059090]     in-flight: 54901:drm_fb_helper_damage_work
[322173.059122] workqueue mm_percpu_wq: flags=0x8
[322173.059123]   pwq 18: cpus=4 node=0 flags=0x0 nice=0 active=1 refcnt=3
[322173.059124]     pending: lru_add_drain_per_cpu BAR(64)
[322173.105218] workqueue kblockd: flags=0x18
[322173.105226]   pwq 31: cpus=7 node=0 flags=0x0 nice=-20 active=1 refcnt=2
[322173.105235]     in-flight: 118:blk_mq_timeout_work
[322173.158176] pool 18: cpus=4 node=0 flags=0x0 nice=0 hung=40s workers=3 idle: 101240 149433
[322173.158191] pool 31: cpus=7 node=0 flags=0x0 nice=-20 hung=0s workers=2 idle: 54
[322173.158195] Showing backtraces of running workers in stalled CPU-bound worker pools:
[322173.158200] pool 18:
[322173.158201] task:kworker/4:2     state:R  running task     stack:0     pid:54901 tgid:54901 ppid:2      flags:0x0000000a
[322173.158207] Workqueue: events drm_fb_helper_damage_work
[322173.158215] Call trace:
[322173.158216]  __switch_to+0x150/0x1fc
[322173.158220]  drm_fb_helper_damage_work+0x84/0x160
[322173.158225]  process_scheduled_works+0x174/0x2bc
[322173.158229]  worker_thread+0x288/0x3c4
[322173.158232]  kthread+0x104/0x18c
[322173.158234]  ret_from_fork+0x10/0x20
[322180.218740] Modules linked in:
[322180.228871] CPU: 4 PID: 54901 Comm: kworker/4:2 Not tainted 6.9.7 #25
[322180.229614] Hardware name: linux,dummy-virt (DT)
[322180.230110] Workqueue: events drm_fb_helper_damage_work
[322180.230770] pstate: 41401805 (nZcv daif +PAN -UAO -TCO +DIT +SSBS BTYPE=-c)
[322180.231468] pc : net_rx_action+0x0/0x2b0
[322180.231990] lr : handle_softirqs+0xd4/0x1fc
[322180.232418] sp : ffff800081fbbf60
[322180.232813] x29: ffff800081fbbf80 x28: 0000000000000003 x27: 000000000000000a
[322180.233415] x26: ffff800081cefaa8 x25: 0000000000000008 x24: 0000000000000003
[322180.234147] x23: 0000000000000100 x22: ffff800081d160d8 x21: ffff800081d160c0
[322180.234731] x20: 0000000000000018 x19: ffff0000d7f03600 x18: 0000000040000000
[322180.235303] x17: ffff80037c197000 x16: ffff800081fb8000 x15: 0000000000000500
[322180.236049] x14: 7d1d8ea6a48fa1dd x13: 0000000000000096 x12: 000000061b4d7a21
[322180.236658] x11: 0000000000000015 x10: 0000000000000000 x9 : 00000000003c8c01
[322180.237266] x8 : ffff80008101035c x7 : 0000000000000820 x6 : 0000000000000000
[322180.237868] x5 : ffff0000c5952e58 x4 : ffff0003fde94b30 x3 : 0000000104cbc2dc
[322180.238478] x2 : 000000000000001c x1 : 0000000000000100 x0 : ffff800081d160d8
[322180.239348] Call trace:
[322180.239670]  net_rx_action+0x0/0x2b0
[322180.240059]  __do_softirq+0x14/0x20
[322180.240432]  ____do_softirq+0x10/0x1c
[322180.240814]  call_on_irq_stack+0x24/0x4c
[322180.241211]  do_softirq_own_stack+0x1c/0x28
[322180.241618]  __irq_exit_rcu+0x54/0xf0
[322180.241986]  irq_exit_rcu+0x10/0x1c
[322180.242345]  el1_interrupt+0x38/0x54
[322180.242743]  el1h_64_irq_handler+0x18/0x24
[322180.243151]  el1h_64_irq+0x64/0x68
[322180.243503]  vp_notify+0x14/0x1c
[322180.243891]  virtio_gpu_notify+0x58/0x6c
[322180.244279]  virtio_gpu_primary_plane_update+0x2ec/0x3c8
[322180.244747]  drm_atomic_helper_commit_planes+0x180/0x29c
[322180.245313]  drm_atomic_helper_commit_tail+0x3c/0x158
[322180.245843]  commit_tail+0xb4/0x16c
[322180.246166]  drm_atomic_helper_commit+0x244/0x25c
[322180.246541]  drm_atomic_commit+0xa8/0xd4
[322180.249257]  drm_atomic_helper_dirtyfb+0x190/0x26c
[322180.249681]  drm_fbdev_generic_helper_fb_dirty+0x1e4/0x2c0
[322180.250155]  drm_fb_helper_damage_work+0x84/0x160
[322180.250789]  process_scheduled_works+0x174/0x2bc
[322180.251252]  worker_thread+0x288/0x3c4
[322180.251672]  kthread+0x104/0x18c
[322180.251978]  ret_from_fork+0x10/0x20
[322180.252301] Modules linked in:
[322180.253132] watchdog: BUG: soft lockup - CPU#4 stuck for 48s! [kworker/4:2:54901]
[322180.253137] Sending NMI from CPU 3 to CPUs 4:
[322180.253611] Modules linked in:
[322180.253614] CPU: 4 PID: 54901 Comm: kworker/4:2 Tainted: G             L     6.9.7 #25
[322180.253618] Hardware name: linux,dummy-virt (DT)
[322180.253618] Workqueue: events drm_fb_helper_damage_work
[322180.256216] pstate: 41401805 (nZcv daif +PAN -UAO -TCO +DIT +SSBS BTYPE=-c)
[322180.256771] pc : net_rx_action+0x0/0x2b0
[322180.257157] lr : handle_softirqs+0xd4/0x1fc
[322180.257534] sp : ffff800081fbbf60
[322180.257902] x29: ffff800081fbbf80 x28: 0000000000000003 x27: 000000000000000a
[322180.258481] x26: ffff800081cefaa8 x25: 0000000000000008 x24: 0000000000000003
[322180.259091] x23: 0000000000000100 x22: ffff800081d160d8 x21: ffff800081d160c0
[322180.259689] x20: 0000000000000018 x19: ffff0000d7f03600 x18: 0000000040000000
[322180.260281] x17: ffff80037c197000 x16: ffff800081fb8000 x15: 0000000000000500
[322180.260898] x14: 7d1d8ea6a48fa1dd x13: 0000000000000096 x12: 000000061b4d7a21
[322180.261548] x11: 0000000000000015 x10: 0000000000000000 x9 : 00000000003c8c01
[322180.262115] x8 : ffff80008101035c x7 : 0000000000000820 x6 : 0000000000000000
[322180.262703] x5 : ffff0000c5952e58 x4 : ffff0003fde94b30 x3 : 0000000104cbc2dc
[322180.263309] x2 : 000000000000001c x1 : 0000000000000100 x0 : ffff800081d160d8
[322180.263929] Call trace:
[322180.264219]  net_rx_action+0x0/0x2b0
[322180.264561]  __do_softirq+0x14/0x20
[322180.264937]  ____do_softirq+0x10/0x1c
[322180.265283]  call_on_irq_stack+0x24/0x4c
[322180.265674]  do_softirq_own_stack+0x1c/0x28
[322180.266069]  __irq_exit_rcu+0x54/0xf0
[322180.266443]  irq_exit_rcu+0x10/0x1c
[322180.266829]  el1_interrupt+0x38/0x54
[322180.267194]  el1h_64_irq_handler+0x18/0x24
[322180.267603]  el1h_64_irq+0x64/0x68
[322180.267938]  vp_notify+0x14/0x1c
[322180.268423]  virtio_gpu_notify+0x58/0x6c
[322180.268764]  virtio_gpu_primary_plane_update+0x2ec/0x3c8
[322180.269228]  drm_atomic_helper_commit_planes+0x180/0x29c
[322180.269631]  drm_atomic_helper_commit_tail+0x3c/0x158
[322180.270069]  commit_tail+0xb4/0x16c
[322180.270386]  drm_atomic_helper_commit+0x244/0x25c
[322180.270924]  drm_atomic_commit+0xa8/0xd4
[322180.271343]  drm_atomic_helper_dirtyfb+0x190/0x26c
[322180.271929]  drm_fbdev_generic_helper_fb_dirty+0x1e4/0x2c0
[322180.273060]  drm_fb_helper_damage_work+0x84/0x160
[322180.273518]  process_scheduled_works+0x174/0x2bc
[322180.273904]  worker_thread+0x288/0x3c4
[322180.274229]  kthread+0x104/0x18c
[322180.274525]  ret_from_fork+0x10/0x20
[322180.274860] Modules linked in:
[322180.276973] Kernel panic - not syncing: Hard LOCKUP
[322180.277613] CPU: 1 PID: 0 Comm: swapper/1 Tainted: G             L     6.9.7 #25
[322180.278591] Hardware name: linux,dummy-virt (DT)
[322180.279145] Call trace:
[322180.279675]  dump_backtrace+0xf0/0x128
[322180.280091]  show_stack+0x18/0x24
[322180.280485]  dump_stack_lvl+0x40/0x84
[322180.280872]  dump_stack+0x18/0x24
[322180.282231]  panic+0x130/0x358
[322180.288865]  nmi_panic+0x44/0x90
[322180.290171]  watchdog_hardlockup_check+0x1c8/0x248
[322180.291381]  watchdog_buddy_check_hardlockup+0xa0/0xb4
[322180.292790]  watchdog_timer_fn+0x60/0x304
[322180.294497]  __hrtimer_run_queues+0xf0/0x1a8
[322180.294992]  hrtimer_interrupt+0xc4/0x37c
[322180.298349]  arch_timer_handler_virt+0x3c/0x4c
[322180.298927]  handle_percpu_devid_irq+0x80/0x128
[322180.299448]  generic_handle_domain_irq+0x2c/0x44
[322180.299899]  gic_handle_irq+0x4c/0x110
[322180.300284]  call_on_irq_stack+0x24/0x4c
[322180.300670]  do_interrupt_handler+0x4c/0x6c
[322180.301115]  el1_interrupt+0x34/0x54
[322180.302130]  el1h_64_irq_handler+0x18/0x24
[322180.302638]  el1h_64_irq+0x64/0x68
[322180.311586]  do_idle+0xf0/0x264
[322180.312409]  cpu_startup_entry+0x34/0x38
[322180.313915]  secondary_start_kernel+0x13c/0x160
[322180.316103]  __secondary_switched+0xb8/0xbc
[322180.316836] SMP: stopping secondary CPUs
[322180.318520] Kernel Offset: disabled
[322180.320545] CPU features: 0x0,00000002,80113528,674e7727
[322180.321960] Memory Limit: none
[322180.324964] ---[ end Kernel panic - not syncing: Hard LOCKUP ]---
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
