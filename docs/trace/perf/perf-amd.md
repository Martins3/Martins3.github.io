## 为什么在 Guest 无法使用 perf 了

### 第一个修复
意思是，如果没有启用 SVM ，那么就不要去设置 HO 或者 GO bit ，逻辑很简单，默认设置

考虑的问题在还是物理机的情况，这个是 CPU 有 bug 的情况:

检查 sdm 中的 v2 13.2 中的 Table 13-4

| Bits Host Mode (Bit 41) | Guest Mode (Bit 40) | Events Counted                                 |
|-------------------------|---------------------|------------------------------------------------|
| 0                       | 0                   | All events, irrespective of guest or host mode |
| 0                       | 1                   | Guest events, if `EFER[SVME]` = 1              |
| 1                       | 0                   | Host events, if `EFER[SVME]` = 1               |
| 1                       | 1                   | Guest and host events, if `EFER[SVME]` = 1     |

这个应该是主线也有的 bug 。
```txt
History:        #0
Commit:         1018faa6cf23b256bf25919ef203cd7c129f06f2
Author:         Joerg Roedel <joerg.roedel@amd.com>
Committer:      Ingo Molnar <mingo@elte.hu>
Author Date:    Wed 29 Feb 2012 09:57:32 PM CST
Committer Date: Fri 02 Mar 2012 07:16:39 PM CST

perf/x86/kvm: Fix Host-Only/Guest-Only counting with SVM disabled

It turned out that a performance counter on AMD does not
count at all when the GO or HO bit is set in the control
register and SVM is disabled in EFER.

This patch works around this issue by masking out the HO bit
in the performance counter control register when SVM is not
enabled.

The GO bit is not touched because it is only set when the
user wants to count in guest-mode only. So when SVM is
disabled the counter should not run at all and the
not-counting is the intended behaviour.

Signed-off-by: Joerg Roedel <joerg.roedel@amd.com>
Signed-off-by: Peter Zijlstra <a.p.zijlstra@chello.nl>
Cc: Avi Kivity <avi@redhat.com>
Cc: Stephane Eranian <eranian@google.com>
Cc: David Ahern <dsahern@gmail.com>
Cc: Gleb Natapov <gleb@redhat.com>
Cc: Robert Richter <robert.richter@amd.com>
Cc: stable@vger.kernel.org # v3.2
Link: http://lkml.kernel.org/r/1330523852-19566-1-git-send-email-joerg.roedel@amd.com
Signed-off-by: Ingo Molnar <mingo@elte.hu>
```

### 第二个修复

如果没有这个 patch ，
perf/x86/amd: Don't touch the AMD64_EVENTSEL_HOSTONLY bit inside the guest
https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/commit/?id=c797b8872bb996f703ef0d68e763caa3770f8a5f

在 Milan 架构上会触发这个错误，也就是在 Guest 中，无论是否开启虚拟化，如果含有 HO bit ，那么都有问题:
```txt
[ 3169.783059] unchecked MSR access error: WRMSR to 0xc0010200 (tried to write 0x0000020000130076) at rIP: 0xffffffffbdc5de24 (native_write_msr+0x4/0x20)
[ 3169.785365] Call Trace:
[ 3169.785782]  <IRQ>
[ 3169.786132]  amd_pmu_disable_event+0x22/0x90
[ 3169.786809]  x86_pmu_stop+0x3b/0xa0
[ 3169.787375]  perf_event_task_tick+0x1f0/0x380
[ 3169.788072]  ? tick_sched_do_timer+0x80/0x80
[ 3169.788745]  scheduler_tick+0x7f/0xd0
[ 3169.789337]  update_process_times+0x40/0x50
[ 3169.789995]  tick_sched_handle+0x21/0x70
[ 3169.790624]  ? tick_sched_do_timer+0x55/0x80
[ 3169.791304]  tick_sched_timer+0x37/0x70
[ 3169.791910]  __hrtimer_run_queues+0x108/0x290
[ 3169.792607]  hrtimer_interrupt+0xe5/0x240
[ 3169.793256]  smp_apic_timer_interrupt+0x6a/0x130
[ 3169.793981]  apic_timer_interrupt+0xf/0x20
[ 3169.794633]  </IRQ>
[ 3169.794977] RIP: 0010:smp_call_function_single+0xdc/0x100
[ 3169.795837] Code: 00 00 00 75 40 48 83 c4 48 41 5a 5d 49 8d 62 f8 c3 48 89 d1 48 89 f2 48 8d 75 b0 e8 2e fe ff ff 8b 55 c8 83 e2 01 74 0a f3 90 <8b> 55 c8 83 e2 01 75 f6 eb c2 8b 05 64 3a 96 01 85 c0 75 81 0f 0b
[ 3169.798732] RSP: 0018:ffffa36a83183ca0 EFLAGS: 00000202 ORIG_RAX: ffffffffffffff13
[ 3169.799911] RAX: 0000000000000000 RBX: ffff91f5ec2a9400 RCX: 0000000000000830
[ 3169.801025] RDX: 0000000000000001 RSI: 00000000000000fb RDI: 0000000000000830
[ 3169.802146] RBP: ffffa36a83183cf0 R08: 0000000000000022 R09: 0000000000000001
[ 3169.803264] R10: ffffa36a83183d08 R11: 0000000000000001 R12: ffffffffbddfe6b0
[ 3169.804377] R13: ffff91f2e737aa38 R14: 0000000000000000 R15: 0000000000000000
[ 3169.805495]  ? perf_event_disable+0x30/0x30
[ 3169.806164]  ? bpf_fd_reuseport_array_update_elem+0x1a0/0x1a0
[ 3169.807066]  ? event_function_call+0x117/0x120
[ 3169.807764]  event_function_call+0x117/0x120
[ 3169.808444]  ? ctx_resched+0xd0/0xd0
[ 3169.809015]  ? cpu_clock_event_read+0x30/0x30
[ 3169.809706]  ? perf_event_disable+0x30/0x30
[ 3169.810371]  perf_event_for_each_child+0x34/0x80
[ 3169.811104]  ? perf_event_disable+0x30/0x30
[ 3169.811771]  _perf_ioctl+0x120/0x6a0
[ 3169.812349]  ? tlb_finish_mmu+0x1f/0x30
[ 3169.812959]  ? unmap_region+0xdd/0x110
[ 3169.813561]  ? _cond_resched+0x15/0x40
[ 3169.814165]  perf_ioctl+0x2c/0x50
[ 3169.814696]  do_vfs_ioctl+0xa4/0x640
[ 3169.815272]  ? dput+0xfc/0x210
[ 3169.815761]  ? syscall_trace_enter+0x1df/0x2e0
[ 3169.817448]  ksys_ioctl+0x70/0x80
[ 3169.818984]  __x64_sys_ioctl+0x16/0x20
[ 3169.820729]  do_syscall_64+0x61/0x250
[ 3169.822398]  entry_SYSCALL_64_after_hwframe+0x44/0xa9
[ 3169.824227] RIP: 0033:0x7f88a2012107
[ 3169.825731] Code: 44 00 00 48 8b 05 81 cd 2c 00 64 c7 00 26 00 00 00 48 c7 c0 ff ff ff ff c3 66 2e 0f 1f 84 00 00 00 00 00 b8 10 00 00 00 0f 05 <48> 3d 01 f0 ff ff 73 01 c3 48 8b 0d 51 cd 2c 00 f7 d8 64 89 01 48
[ 3169.830735] RSP: 002b:00007ffe7380e1e8 EFLAGS: 00000246 ORIG_RAX: 0000000000000010
[ 3169.833005] RAX: ffffffffffffffda RBX: 0000000000000000 RCX: 00007f88a2012107
[ 3169.835268] RDX: 0000000000000000 RSI: 0000000000002400 RDI: 000000000000000e
[ 3169.837477] RBP: 00007ffe7380e220 R08: 0000000000000001 R09: 000055a83830c7c0
[ 3169.839686] R10: 0000000000000022 R11: 0000000000000246 R12: 000000000000000a
```

但是在 Milan 上包含了这个 patch ，在 4.19 kernel 中启动了虚拟机之后，使用  perf 有如下的 trace :

```txt
unchecked MSR access error: WRMSR to 0xc0010200 (tried to write 0x0000020000530076) at rIP: 0xffffffff852603a4 (native_write_msr+0x4/0x20)
Call Trace:
 x86_pmu_enable_all+0xbb/0x120
 ? x86_pmu_enable+0x105/0x310
 __perf_event_task_sched_in+0x17b/0x1b0
 finish_task_switch+0x139/0x2a0
 __schedule+0x298/0x760
 ? hrtimer_start_range_ns+0x139/0x310
 schedule+0x29/0xc0
 schedule_hrtimeout_range_clock+0xbb/0x190
 ? __hrtimer_init+0xb0/0xb0
 poll_schedule_timeout.constprop.12+0x42/0x70
 do_sys_poll+0x3d6/0x590
 ? update_load_avg+0x89/0x5d0
 ? update_load_avg+0x89/0x5d0
 ? account_entity_enqueue+0xc5/0xf0
 ? reweight_entity+0x1d2/0x230
 ? check_preempt_wakeup+0x102/0x230
 ? poll_select_finish+0x220/0x220
 ? poll_select_finish+0x220/0x220
 ? poll_select_finish+0x220/0x220
 ? ep_item_poll.isra.15+0x3f/0xb0
 ? ep_send_events_proc+0x8a/0x1e0
 ? ep_read_events_proc+0xe0/0xe0
 ? ep_scan_ready_list.constprop.20+0x220/0x240
 ? ep_poll+0x1eb/0x430
 ? kvm_clock_get_cycles+0xd/0x10
 ? ktime_get_ts64+0x4c/0xf0
 ? __x64_sys_poll+0x9e/0x120
 __x64_sys_poll+0x9e/0x120
 do_syscall_64+0x5f/0x240
 entry_SYSCALL_64_after_hwframe+0x5c/0xc1
kvm: SMP vm created on host with unstable TSC; guest TSC will not be reliable
```
这是预期的，因为启动 qemu 的时候 svm_enable_virtualization_cpu -> amd_pmu_enable_virt
```c
cpuc->perf_ctr_virt_mask = 0;
```

## 7945hx 中这个问题已经没有了
这里的输出为:
```c
	pr_info("%s : %s : %llx %llx %llx", __FUNCTION__ , current->comm, disable_mask, hwc->config, enable_mask);

  pr_info("%s : %s : %llx %llx", __FUNCTION__ , current->comm, disable_mask, hwc->config);
```
```txt
                                                     <!-- 20000130076 -->
[  499.944619] __x86_pmu_enable_event : .perf-wrapped : 0 20000130076 400000
[  499.944619] __x86_pmu_enable_event : .perf-wrapped : 0 200001300a9 400000
[  499.944619] __x86_pmu_enable_event : .perf-wrapped : 0 200001300c0 400000
[  499.944619] __x86_pmu_enable_event : .perf-wrapped : 0 200001300c2 400000
[  499.944619] __x86_pmu_enable_event : .perf-wrapped : 0 200001300c3 400000

[  507.093809] x86_pmu_disable_event : .perf-wrapped : 0 20000130076
[  507.094602] x86_pmu_disable_event : .perf-wrapped : 0 200001300a9
[  507.094949] x86_pmu_disable_event : .perf-wrapped : 0 200001300c0
[  507.095436] x86_pmu_disable_event : .perf-wrapped : 0 200001300c2
```
这里的 disable_mask 为什么总是配置的为 0 ，这是由于新的 kerel 当启动之后，会立刻执行，就会执行 amd_pmu_enable_virt 的:
```txt
[    1.817637] [martins3:amd_pmu_enable_virt:1568]
```

从上面中看，写入到 hwc->config 中也是添加了 AMD64_EVENTSEL_HOSTONLY 的，但是新的 CPU 中是没有这个 bug 的。

> It turned out that a performance counter on AMD does not
> count at all when the GO or HO bit is set in the control
> register and SVM is disabled in EFER.

这个 bug 是 SVM 如果在 EFER 中 disable 了，GO 和 HO bit 不可以打开。

## 如果继续调查，可以调查如下问题
1. 测试 AMD64_EVENTSEL_HOSTONLY 打开和关闭的效果是什么?
2. 为什么 perf 默认启动之后，kernel 默认打开 AMD64_EVENTSEL_HOSTONLY 的 bit ?

```txt
  - entry_SYSCALL_64
    - do_syscall_64
      - do_syscall_x64
        - __do_sys_perf_event_open
          - perf_event_alloc
            - perf_init_event
              - perf_try_init_event
                - x86_pmu_event_init
                  - __x86_pmu_event_init
                    - amd_pmu_hw_config
                      - amd_core_hw_config
```

```c
static int amd_core_hw_config(struct perf_event *event)
{
	if (event->attr.exclude_host && event->attr.exclude_guest)
		/*
		 * When HO == GO == 1 the hardware treats that as GO == HO == 0
		 * and will count in both modes. We don't want to count in that
		 * case so we emulate no-counting by setting US = OS = 0.
		 */
		event->hw.config &= ~(ARCH_PERFMON_EVENTSEL_USR |
				      ARCH_PERFMON_EVENTSEL_OS);
	else if (event->attr.exclude_host)
		event->hw.config |= AMD64_EVENTSEL_GUESTONLY;
	else if (event->attr.exclude_guest)
		event->hw.config |= AMD64_EVENTSEL_HOSTONLY;

	if ((x86_pmu.flags & PMU_FL_PAIR) && amd_is_pair_event_code(&event->hw))
		event->hw.flags |= PERF_X86_EVENT_PAIR;

	if (has_branch_stack(event))
		return static_call(amd_pmu_branch_hw_config)(event);

	return 0;
}
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
