## 大部分 kvmclock3 的地方

kvmclock3.md
54:#### 这里的日志都是来自于 masterclock
56:而 masterclock 更新于这里的，当时想要解决的问题也是如何把不去轻易更新这里
182:            hv_clock.system_time = ka->master_kernel_ns + ka->kvmclock_offset;
261:- 显然两者不是等价的，src->tsc_timestamp + src->system_time 是 masterclock 刷新的时候修改的，
575:    Commit 0bc48bea36d1 ("KVM: x86: update master clock before computing
584:    However, it leaves any other masterclock update unsafe, which includes,
623:### 2023/10/8 KVM: x86: Don't unnecessarily force masterclock update on vCPU hotplug
630:    KVM: x86: Don't unnecessarily force masterclock update on vCPU hotplug
632:    Don't force a masterclock update when a vCPU synchronizes to the current
634:    VM.  Unnecessarily updating the masterclock is undesirable as it can cause
641:    masterclock, KVM refreshes the "base", i.e. moves the elapsed time since
647:    Prior to commit 7f187922ddf6 ("KVM: x86: update masterclock values on TSC
655:        KVM: x86: update masterclock values on TSC writes
657:        When the guest writes to the TSC, the masterclock TSC copy must be
661:        Once "if (!vcpus_matched && ka->use_master_clock)" is simplified to
662:        "if (ka->use_master_clock)", the corresponding "if (!ka->use_master_clock)"
666:    Before that, KVM only re-synced the masterclock if the masterclock was
733:                    if (!ka->use_master_clock)
736:            if (!vcpus_matched && ka->use_master_clock)
759:    kernel time, it's the masterclock snapshot!
762:            use_master_clock = ka->use_master_clock;
763:            if (use_master_clock) {
777:    And so when KVM skips the masterclock update after a TSC write, i.e. after
782:    Forcing a masterclock update essentially fudged around that problem, but
784:    unnecessarily forces a masterclock update when a new vCPU joins the party
787:    Note, KVM forces masterclock updates in other weird ways that are also
796:    Fixes: 7f187922ddf6 ("KVM: x86: update masterclock values on TSC writes")
822:+    * To use the masterclock, the host clocksource must be based on TSC
826:+   bool use_master_clock = (ka->nr_vcpus_matched_tsc + 1 ==
831:-    * Once the masterclock is enabled, always perform request in
834:-    * In order to enable masterclock, the host clocksource must be TSC
836:-    * perform request to enable masterclock.
837:+    * Request a masterclock update if the masterclock needs to be toggled
838:+    * on/off, or when starting a new generation and the masterclock is
839:+    * enabled (compute_guest_tsc() requires the masterclock snapshot to be
842:-   if (ka->use_master_clock ||
844:+   if ((ka->use_master_clock && new_generation) ||
845:+       (ka->use_master_clock != use_master_clock))
1027:       (ka->use_master_clock != use_master_clock))
1033:                       ka->use_master_clock, gtod->clock.vclock_mode);
1150:## 那么，之前为什么需要更新 masterclock


## 这个 unstable 到底是什么意思?

```c
static inline bool kvm_check_tsc_unstable(void)
{
#ifdef CONFIG_X86_64
	/*
	 * TSC is marked unstable when we're running on Hyper-V,
	 * 'TSC page' clocksource is good.
	 */
	if (pvclock_gtod_data.clock.vclock_mode == VDSO_CLOCKMODE_HVCLOCK)
		return false;
#endif
	return check_tsc_unstable(); // 这个其实是物理机的 unstable 的结果
}
```

## masterclock
pvclock_update_vm_gtod_copy 上的注释解释了为什么要 masterclock

而这个所谓的 master clock 就是 `pvclock_gtod_data` 吗?

看上去主要的影响在于:
- kvm_get_wall_clock_epoch
- kvm_vm_ioctl_set_clock
- __get_kvmclock
- kvm_guest_time_update : 这个是影响最大的了

### master_kernel_ns 和 master_cycle_now 的刷新位置

```c
	host_tsc_clocksource = kvm_get_time_and_clockread(
					&ka->master_kernel_ns,
					&ka->master_cycle_now);
```
这里获取的 :
- ka->master_kernel_ns : boot ns
- ka->master_cycle_now : rdtsc() 的返回值

所谓刷新 master clock ，就是刷新这两个数值而已。

do_kvmclock_base 的计算就是 host 开机了多少时间：

(base_cycles + ((read_tsc() - clock->cycle_last) & clock->mask) * clock->mult ) >> shift + gtod->raw_clock.offset + gtod->offs_boot

节选自 update_pvclock_gtod 中:
```c
	vdata->raw_clock.vclock_mode	= tk->tkr_raw.clock->vdso_clock_mode;
	vdata->raw_clock.cycle_last	= tk->tkr_raw.cycle_last;
	vdata->raw_clock.mask		= tk->tkr_raw.mask;
	vdata->raw_clock.mult		= tk->tkr_raw.mult;
	vdata->raw_clock.shift		= tk->tkr_raw.shift;
	vdata->raw_clock.base_cycles	= tk->tkr_raw.xtime_nsec;
	vdata->raw_clock.offset		= tk->tkr_raw.base;

	vdata->offs_boot		= tk->offs_boot;
  vdata->offs_boot		= tk->offs_boot;
  //  @offs_boot:			Offset clock monotonic -> clock boottime
```

这里获取的是 host 的开机时间，所以 host 时间开机时间越长，那么误差越大。

```txt
@[
    update_pvclock_gtod
    pvclock_gtod_notify+287
    pvclock_gtod_notify+287
    notifier_call_chain+90
    timekeeping_update+175
    timekeeping_advance+835
    update_wall_time+16
    tick_sched_do_timer+132
    tick_nohz_highres_handler+49
    __hrtimer_run_queues+271
    hrtimer_interrupt+248
    __sysvec_apic_timer_interrupt+77
    sysvec_apic_timer_interrupt+111
    asm_sysvec_apic_timer_interrupt+26
    cpuidle_enter_state+205
    cpuidle_enter+45
    do_idle+474
    cpu_startup_entry+42
    start_secondary+286
    secondary_startup_64_no_verify+388
]: 4888
```

在 update_pvclock_gtod


### kvm_guest_time_update

如果使用:
```c
			host_tsc = ka->master_cycle_now;
			kernel_ns = ka->master_kernel_ns;
```

如果不使用:
```c
		host_tsc = rdtsc();
		kernel_ns = get_kvmclock_base_ns();
```

最后刷新到:
```c
	tsc_timestamp = kvm_read_l1_tsc(v, host_tsc); // 因为 host tsc 和 guest tsc 的频率不同，所以需要进行换算。
	vcpu->hv_clock.tsc_timestamp = tsc_timestamp;
	vcpu->hv_clock.system_time = kernel_ns + v->kvm->arch.kvmclock_offset;
```

kvm_arch_init_vm 和 kvm_vm_ioctl_set_clock 中，默认情况，当虚拟机启动的时候，host 的启动时间:
```c
	kvm->arch.kvmclock_offset = -get_kvmclock_base_ns();
```

实际上，v->kvm->arch.kvmclock_offset; 只有在初始化，或者通过 ioctl 调整 clock 的时候，

### 难道 masterclock 不是天经地义的吗?

从 pvclock_update_vm_gtod_copy 看，就是两个考虑:
1. 所有的 vCPU 都是在一个 generation
2. host 的 tsc 本来是 stable 的，那么就让所有的 clock 公用一个体系

其实是更加简单的，就是所有的 vCPU 都不要独立的更新

### 如何理解 generation 的概念



## 因为 masterclock 的存在，所以实际上，即使是更新单独的每一个 kvm ，但是还是会更新所有的 vcpu 的 kvm
```c
		kvm_make_request(KVM_REQ_CLOCK_UPDATE, vcpu);
```

- __kvm_synchronize_tsc
  - kvm_track_tsc_matching : Sean Christopherson workaround CPU 的地方，在这里决定是否刷新 masterlock

通往 __kvm_synchronize_tsc :

- kvm_vcpu_ioctl_device_attr : 这个路径看上去不会走
  - kvm_arch_tsc_set_attr
    - __kvm_synchronize_tsc

- kvm_arch_vcpu_postcreate
- kvm_set_msr_common
  - kvm_synchronize_tsc
    - __kvm_synchronize_tsc


## kvm_synchronize_tsc

kvm_synchronize_tsc 的位置:
```txt
	/*
	 * For a reliable TSC, we can match TSC offsets, and for an unstable
	 * TSC, we add elapsed time in this computation.  We could let the
	 * compensation code attempt to catch up if we fall behind, but
	 * it's better to try to match offsets from the beginning.
   */
```

## 是不是没有 stable tsc 的时候，必然没有 master clock ?

## 按道理，虚拟机暂停不该出现时间回退的!

## PVCLOCK_TSC_STABLE_BIT

## kvm_vm_ioctl_set_clock 的语意是什么?

只是为了修改 	ka->kvmclock_offset = data.clock - now_raw_ns; ?

先不管，为什么不去直接修改 kvmclock_offset 了，但是最后的效果就是直接
修改 pvti 中的 system_timestamp 了

但是比较的内容居然是 ka->master_kernel_ns; 这个是物理机的开机时间啊

### 为什么 kvm_vm_ioctl_set_clock 和 kvm_update_masterclock 在结构上如此相似?

### kvm_vm_ioctl_get_clock

就是获取到 tsc 的:
```c
		data->clock = __pvclock_read_cycles(&hv_clock, data->host_tsc);
```


## wallclock

```txt
4.30 KVM_SET_CLOCK
------------------

:Capability: KVM_CAP_ADJUST_CLOCK
:Architectures: x86
:Type: vm ioctl
:Parameters: struct kvm_clock_data (in)
:Returns: 0 on success, -1 on error

Sets the current timestamp of kvmclock to the value specified in its parameter.
In conjunction with KVM_GET_CLOCK, it is used to ensure monotonicity on scenarios
such as migration.

The following flags can be passed:

KVM_CLOCK_REALTIME
  If set, KVM will compare the value of the `realtime` field
  with the value of the host's real time clocksource at the instant when
  KVM_SET_CLOCK was called. The difference in elapsed time is added to the final
  kvmclock value that will be provided to guests.

Other flags returned by ``KVM_GET_CLOCK`` are accepted but ignored.

::

  struct kvm_clock_data {
	__u64 clock;  /* kvmclock current value */
	__u32 flags;
	__u32 pad0;
	__u64 realtime;
	__u64 host_tsc;
	__u32 pad[4];
  };
```

## 如何快速知道当前是不是用的 masterclock

这两个函数都是谁在调用的:
- pvclock_update_vm_gtod_copy : 很难调用
- kvm_track_tsc_matching : synchronized tsc 调用

pvclock_gtod_notify : 中进行发现如果 gtod 不是基于 tsc ，但是系统又是使用 masterclock 的，
那么就让系统中所有的机器都不要使用 masterclock 。

```c
gtod_is_based_on_tsc(gtod->clock.vclock_mode)
```

```c
static int pvclock_gtod_notify(struct notifier_block *nb, unsigned long unused,
			       void *priv)
{
	struct pvclock_gtod_data *gtod = &pvclock_gtod_data;
	struct timekeeper *tk = priv;

	update_pvclock_gtod(tk);

	/*
	 * Disable master clock if host does not trust, or does not use,
	 * TSC based clocksource. Delegate queue_work() to irq_work as
	 * this is invoked with tk_core.seq write held.
	 */
	if (!gtod_is_based_on_tsc(gtod->clock.vclock_mode) &&
	    atomic_read(&kvm_guest_has_master_clock) != 0)
		irq_work_queue(&pvclock_irq_work);
	return 0;
}
```
默认取值:
1. gtod->clock.vclock_mode : 1 ，表示取值是 VDSO_CLOCKMODE_TSC
2. kvm_guest_has_master_clock : 1

gtod->clock.vclock_mode 是物理机的 tsc 状态

虚拟机中，这两个数值都是 0 。都是符合预期的


那么，为什么 vclock_mode 中来解释这些问题了:

```c
/*
 * Calculates the kvmclock_base_ns (CLOCK_MONOTONIC_RAW + boot time) and
 * reports the TSC value from which it do so. Returns true if host is
 * using TSC based clocksource.
 */
static bool kvm_get_time_and_clockread(s64 *kernel_ns, u64 *tsc_timestamp)
{
	/* checked again under seqlock below */
	if (!gtod_is_based_on_tsc(pvclock_gtod_data.clock.vclock_mode))
		return false;

	return gtod_is_based_on_tsc(do_kvmclock_base(kernel_ns,
						     tsc_timestamp));
}
```

称之为 kvm 的时钟源，看 vgettsc 中的各种判断:
arch/x86/kvm/x86.c

## 真的可以解决问题吗? pvclock_update_vm_gtod_copy


## 可以通过这个函数来彻底理解 kvm 中为什么需要 realtime 的

```c
/*
 * Calculates CLOCK_REALTIME and reports the TSC value from which it did
 * so. Returns true if host is using TSC based clocksource.
 *
 * DO NOT USE this for anything related to migration. You want CLOCK_TAI
 * for that.
 */
static bool kvm_get_walltime_and_clockread(struct timespec64 *ts,
					   u64 *tsc_timestamp)
{
	/* checked again under seqlock below */
	if (!gtod_is_based_on_tsc(pvclock_gtod_data.clock.vclock_mode))
		return false;

	return gtod_is_based_on_tsc(do_realtime(ts, tsc_timestamp));
}

```

## intel 中应该有这个警告才把
```txt
[  124.909396] kvm: SMP vm created on host with unstable TSC; guest TSC will not be reliable
```
但是在 hygon 的嵌套中发现了。

似乎不是一个 unstable 。

### qemu 中注释的含义是什么?

关键在 kvm_update_clock 的注释

## master clock 什么时候初始化的?

从什么时候，开始使用的 master clock :

利用 trace_kvm_update_master_clock 做下观察吧:

```txt
#0  kvmclock_vm_state_change (opaque=0x5555575328b0, running=true, state=RUN_STATE_RUNNING) at ../hw/i386/kvm/clock.c:168
#1  0x0000555555b1b664 in vm_state_notify (running=running@entry=true, state=state@entry=RUN_STATE_RUNNING) at ../system/runstate.c:396
#2  0x0000555555b12047 in vm_prepare_start (step_pending=step_pending@entry=false) at ../system/cpus.c:776
#3  0x0000555555b1209b in vm_start () at ../system/cpus.c:783
#4  0x0000555555b6d341 in qmp_cont (errp=0x0) at ../monitor/qmp-cmds.c:112
#5  0x0000555555b23a45 in qemu_init (argc=<optimized out>, argv=<optimized out>) at ../system/vl.c:3843
#6  0x00005555558903e9 in main (argc=<optimized out>, argv=<optimized out>) at ../system/main.c:68
```

## 这个数值会不变吗?
```diff
History:        #0
Commit:         3ebcbd2244f5a69e06e5f655bfbd8127c08201c7
Author:         Anton Romanov <romanton@google.com>
Committer:      Sean Christopherson <seanjc@google.com>
Author Date:    Thu 09 Jun 2022 02:35:26 AM CST
Committer Date: Thu 01 Dec 2022 08:31:27 AM CST

KVM: x86: Use current rather than snapshotted TSC frequency if it is constant

Don't snapshot tsc_khz into per-cpu cpu_tsc_khz if the host TSC is
constant, in which case the actual TSC frequency will never change and thus
capturing TSC during initialization is unnecessary, KVM can simply use
tsc_khz.  This value is snapshotted from
kvm_timer_init->kvmclock_cpu_online->tsc_khz_changed(NULL)

On CPUs with constant TSC, but not a hardware-specified TSC frequency,
snapshotting cpu_tsc_khz and using that to set a VM's target TSC frequency
can lead to VM to think its TSC frequency is not what it actually is if
refining the TSC completes after KVM snapshots tsc_khz.  The actual
frequency never changes, only the kernel's calculation of what that
frequency is changes.

Ideally, KVM would not be able to race with TSC refinement, or would have
a hook into tsc_refine_calibration_work() to get an alert when refinement
is complete.  Avoiding the race altogether isn't practical as refinement
takes a relative eternity; it's deliberately put on a work queue outside of
the normal boot sequence to avoid unnecessarily delaying boot.

Adding a hook is doable, but somewhat gross due to KVM's ability to be
built as a module.  And if the TSC is constant, which is likely the case
for every VMX/SVM-capable CPU produced in the last decade, the race can be
hit if and only if userspace is able to create a VM before TSC refinement
completes; refinement is slow, but not that slow.

For now, punt on a proper fix, as not taking a snapshot can help some uses
cases and not taking a snapshot is arguably correct irrespective of the
race with refinement.

Signed-off-by: Anton Romanov <romanton@google.com>
Reviewed-by: Sean Christopherson <seanjc@google.com>
Link: https://lore.kernel.org/r/20220608183525.1143682-1-romanton@google.com
Signed-off-by: Sean Christopherson <seanjc@google.com>
```

## 是否使用 master clock 的判定标准是什么?
<!-- 819de719-21e9-4adc-a693-18d3f7595a6f -->

不急，只是用来提醒该整理 time 机制而已

master clock 是所有的故事的核心了

## 到底
如果使用 master clock ，arch/x86/kvm/x86.c 中的 get_kvmclock_ns 和
qemu 中 kvmclock_current_nsec 含义相同，只是去执行 rdstc 时间不同。

如果不使用 master clock : 虚拟机的 pvti 在被在 pvclock_update_vm_gtod_copy 中每 5 分钟刷新一次，
刷新来源为 ktime_get_boot_ns() + rdtsc() 。KVM_GET_CLOCK 为:
ktime_get_boot_ns() + ka->kvmclock_offset
在 qemu 中 kvmclock_current_nsec() ，相当于帮 guest os 计算了一次。
由于 5 分钟刷新一次，两者差别极小。


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
