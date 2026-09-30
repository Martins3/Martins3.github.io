## 这个问题难道不会影响到虚拟机的热迁移吗?
应该不会，热迁移的时候，会完全按照新机器的时间来算，
只要新机器的时间没问题，那么就没问题。


#### 这里的日志都是来自于 masterclock

pvclock_update_vm_gtod_copy

这里实际上调用到
```c
	/*
	 * If the host uses TSC clock, then passthrough TSC as stable
	 * to the guest.
	 */
	host_tsc_clocksource = kvm_get_time_and_clockread(
					&ka->master_kernel_ns,
					&ka->master_cycle_now);
```

使用 http://david.woodhou.se/tsdrift.c 测试，一天的误差其实也就是 7000ns 而已
需要 3 年才有 1s 的差距

### kvm_get_wall_clock_epoch 是做什么用的
注意，`tsc_timestamp` 计算到的是 guset os 的 timestamp

```c
	tsc_timestamp = kvm_read_l1_tsc(v, host_tsc);
```
https://lore.kernel.org/lkml/87o8dxf597.ffs@nanos.tec.linutronix.de/T/#r492be97a96e17b1f0a1d0197a8c41d5034cbb136

- kvm_write_wall_clock
    - kvm_get_wall_clock_epoch
        - get_kvmclock_ns

- hyperv 系统
- get_time_ref_counter
    - get_kvmclock_ns

- get_kvmclock_ns
- kvm_vm_ioctl_get_clock
    - get_kvmclock
        - __get_kvmclock

```txt
@[
    kvm_get_wall_clock_epoch+5
    kvm_write_wall_clock.constprop.0+173
    kvm_set_msr_common+1026
    vmx_set_msr+3234
    kvm_set_msr_ignored_check+162
    kvm_emulate_wrmsr+78
    vmx_handle_exit+1898
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+153
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 1
```

#### __get_kvmclock 这个函数好奇怪

这个函数是 kvm 模块内部调用的，似乎为了获取到 guest 中的时间当前是多少。
```txt
		hv_clock.tsc_timestamp = ka->master_cycle_now;
		hv_clock.system_time = ka->master_kernel_ns + ka->kvmclock_offset;
```

### [ ] 误差
3. 如果 host 运行了足够长的时间，host 更新 pvti 会导致 guest 时间发生跳变的原因:

计算是有误差的!

- https://lore.kernel.org/all/20230926230649.67852-1-dongli.zhang@oracle.com/#r

host
do_kvmclock_base

使用的是 kvm_get_time_and_clockread

计算这个时间:
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

host 的算法是:
do_kvmclock_base

kvmclock 算法:
```txt
(rdtsc() >> tsc_shift) * tsc_to_system_mul >> 32
```
tsc 算法:
```txt
elapsed_time = (tsc * mult) >> shift
```

### pvclock_vcpu_time_info 被叫做 hv_clock ?

```c
struct kvm_vcpu_arch {

	struct pvclock_vcpu_time_info hv_clock;
```
这个应该是习惯

### pvclock_vsyscall_time_info 直接分装

arch/x86/include/asm/pvclock-abi.h
```c
struct pvclock_vsyscall_time_info {
	struct pvclock_vcpu_time_info pvti;
} __attribute__((__aligned__(SMP_CACHE_BYTES)));
```

## 一点琐事

在函数开始的地方:
```txt
	pr_info("[martins3:%s:%d] %d %d %d\n", __FUNCTION__, __LINE__, from, to, maxsec);
```
```txt
[    0.000001] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 1000000000 600
[    0.000002] clocksource: [martins3:clocks_calc_mult_shift:88] 8388608 23
[    0.000005] clocksource: [martins3:clocks_calc_mult_shift:63] 2995200 1000000 0
[    0.000006] clocksource: [martins3:clocks_calc_mult_shift:88] 1433950085 32
[    0.306432] clocksource: [martins3:clocks_calc_mult_shift:63] 100000000 1000000000 42
[    0.306860] clocksource: [martins3:clocks_calc_mult_shift:88] 2684354560 28
[    0.307539] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 100000000 21
[    0.307861] clocksource: [martins3:clocks_calc_mult_shift:88] 429496730 32
[    0.314071] clocksource: [martins3:clocks_calc_mult_shift:63] 2995200 1000000 600000
[    0.314429] clocksource: [martins3:clocks_calc_mult_shift:88] 5601368 24
[    0.345968] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.346173] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[    0.051328] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
[    0.648697] clocksource: [martins3:clocks_calc_mult_shift:63] 3579545 1000000000 4
[    0.649041] clocksource: [martins3:clocks_calc_mult_shift:88] 2343484437 23
[    0.689508] clocksource: [martins3:clocks_calc_mult_shift:63] 2995200 1000000 600000
[    0.689845] clocksource: [martins3:clocks_calc_mult_shift:88] 5601368 24
```
### 为什么这么多调用的位置

### 为什么 maxsec ，有的是 600，有的是 600000

### maxsec 就是有更新规则的
```txt
#0  accumulate_nsecs_to_secs (tk=<optimized out>) at kernel/time/timekeeping.c:2194
#1  logarithmic_accumulation (tk=0xffffffff83117860 <tk_core+288>, clock_set=<synthetic pointer>, shift=5, offset=13751424) at kernel/time/timekeeping.c:2197
#2  timekeeping_advance (mode=mode@entry=TK_ADV_TICK) at kernel/time/timekeeping.c:2255
#3  0xffffffff81195bf0 in update_wall_time () at kernel/time/timekeeping.c:2280
#4  0xffffffff811a81cb in tick_nohz_update_jiffies (now=135971275350) at kernel/time/tick-sched.c:717
#5  tick_nohz_irq_enter () at kernel/time/tick-sched.c:1537
#6  tick_irq_enter () at kernel/time/tick-sched.c:1554
#7  0xffffffff810a236f in irq_enter_rcu () at kernel/softirq.c:607
#8  0xffffffff81a6e406 in instr_sysvec_apic_timer_interrupt (regs=0xffffc900022a7e38) at arch/x86/kernel/apic/apic.c:1049
#9  sysvec_apic_timer_interrupt (regs=0xffffc900022a7e38) at arch/x86/kernel/apic/apic.c:1049
```

## 问题的根因在这里吗?

timekeeping 都是把时间累计到一个位置的:

- sysvec_apic_timer_interrupt
  - instr_sysvec_apic_timer_interrupt
    - irq_enter_rcu
      - tick_irq_enter
        - tick_nohz_irq_enter
          - tick_nohz_update_jiffies
            - update_wall_time
              - timekeeping_advance
                - logarithmic_accumulation
                  - accumulate_nsecs_to_secs

- sysvec_apic_timer_interrupt
  - instr_sysvec_apic_timer_interrupt
    - irq_enter_rcu
      - tick_irq_enter
        - tick_nohz_irq_enter
          - ktime_get
            - timekeeping_get_ns

```c
ktime_t ktime_get(void)
{
	struct timekeeper *tk = &tk_core.timekeeper;
	unsigned int seq;
	ktime_t base;
	u64 nsecs;

	WARN_ON(timekeeping_suspended);

	do {
		seq = read_seqcount_begin(&tk_core.seq);
		base = tk->tkr_mono.base;
		nsecs = timekeeping_get_ns(&tk->tkr_mono);

	} while (read_seqcount_retry(&tk_core.seq, seq));

	return ktime_add_ns(base, nsecs);
}
```

time keeping 会来切换这个时间，使用这种方法，内核可以做到，换算
nanosecond 的时候，时间没有偏差的。
```c
static inline unsigned int accumulate_nsecs_to_secs(struct timekeeper *tk)
{
	u64 nsecps = (u64)NSEC_PER_SEC << tk->tkr_mono.shift;
	unsigned int clock_set = 0;

	while (tk->tkr_mono.xtime_nsec >= nsecps) {
		int leap;

		tk->tkr_mono.xtime_nsec -= nsecps;
		tk->xtime_sec++;
```

https://www.zhihu.com/question/638849154/answer/3402609558

## 最简单的方法就是模拟 kvm 的插入，然后直接计算出来物理机的开机的差别


### 可能的原因是 tsc 本来就是有误差的吗


### 问题是，为什么 pvti 需要记录 ns

而且为什么 multi shift 需要在物理机中计算，这个是告诉 guest ，
但是本来就是可以使用物理机 msr 来计算变化的，那个默认是不做装换吗？

使用 multi shift 可以让虚拟机看的 clock 的频率总是 1G Hz 的。


更新 kvmclock 的，

guest = (tsc , multi, shift) - timestamp + kernel_ns

为什么要提供的 kernel_ns ，因为热迁移的时候，tsc 会跳变


## windows 根本不去使用 kvmclock ，那么 windows 如何保证时间的同步的?

如果我来设计，我们根本不会采用这种方法，告诉 guest os TSC 的频率就可以了。

## 琐事

## 关键的 commit

### 2019/10/28 KVM: x86: switch KVMCLOCK base to monotonic raw clock

```txt
commit 53fafdbb8b21fa99dfd8376ca056bffde8cafc11
Author: Marcelo Tosatti <mtosatti@redhat.com>
Date:   Mon Oct 28 12:36:22 2019 -0200

    KVM: x86: switch KVMCLOCK base to monotonic raw clock

    Commit 0bc48bea36d1 ("KVM: x86: update master clock before computing
    kvmclock_offset")
    switches the order of operations to avoid the conversion

    TSC (without frequency correction) ->
    system_timestamp (with frequency correction),

    which might cause a time jump.

    However, it leaves any other masterclock update unsafe, which includes,
    at the moment:

            * HV_X64_MSR_REFERENCE_TSC MSR write.
            * TSC writes.
            * Host suspend/resume.

    Avoid the time jump issue by using frequency uncorrected
    CLOCK_MONOTONIC_RAW clock.

    Its the guests time keeping software responsability
    to track and correct a reference clock such as UTC.

    This fixes forward time jump (which can result in
    failure to bring up a vCPU) during vCPU hotplug:

    Oct 11 14:48:33 storage kernel: CPU2 has been hot-added
    Oct 11 14:48:34 storage kernel: CPU3 has been hot-added
    Oct 11 14:49:22 storage kernel: smpboot: Booting Node 0 Processor 2 APIC 0x2          <-- time jump of almost 1 minute
    Oct 11 14:49:22 storage kernel: smpboot: do_boot_cpu failed(-1) to wakeup CPU#2
    Oct 11 14:49:23 storage kernel: smpboot: Booting Node 0 Processor 3 APIC 0x3
    Oct 11 14:49:23 storage kernel: kvm-clock: cpu 3, msr 0:7ff640c1, secondary cpu clock

    Which happens because:

                    /*
                     * Wait 10s total for a response from AP
                     */
                    boot_error = -1;
                    timeout = jiffies + 10*HZ;
                    while (time_before(jiffies, timeout)) {
                             ...
                    }

    Analyzed-by: Igor Mammedov <imammedo@redhat.com>
    Signed-off-by: Marcelo Tosatti <mtosatti@redhat.com>
    Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

### 2023/10/8 KVM: x86: Don't unnecessarily force masterclock update on vCPU hotplug

```diff
commit c52ffadc65e28ab461fd055e9991e8d8106a0056
Author: Sean Christopherson <seanjc@google.com>
Date:   Wed Oct 18 12:56:38 2023 -0700

    KVM: x86: Don't unnecessarily force masterclock update on vCPU hotplug

    Don't force a masterclock update when a vCPU synchronizes to the current
    TSC generation, e.g. when userspace hotplugs a pre-created vCPU into the
    VM.  Unnecessarily updating the masterclock is undesirable as it can cause
    kvmclock's time to jump, which is particularly painful on systems with a
    stable TSC as kvmclock _should_ be fully reliable on such systems.

    The unexpected time jumps are due to differences in the TSC=>nanoseconds
    conversion algorithms between kvmclock and the host's CLOCK_MONOTONIC_RAW
    (the pvclock algorithm is inherently lossy).  When updating the
    masterclock, KVM refreshes the "base", i.e. moves the elapsed time since
    the last update from the kvmclock/pvclock algorithm to the
    CLOCK_MONOTONIC_RAW algorithm.  Synchronizing kvmclock with
    CLOCK_MONOTONIC_RAW is the lesser of evils when the TSC is unstable, but
    adds no real value when the TSC is stable.

    Prior to commit 7f187922ddf6 ("KVM: x86: update masterclock values on TSC
    writes"), KVM did NOT force an update when synchronizing a vCPU to the
    current generation.

      commit 7f187922ddf6b67f2999a76dcb71663097b75497
      Author: Marcelo Tosatti <mtosatti@redhat.com>
      Date:   Tue Nov 4 21:30:44 2014 -0200

        KVM: x86: update masterclock values on TSC writes

        When the guest writes to the TSC, the masterclock TSC copy must be
        updated as well along with the TSC_OFFSET update, otherwise a negative
        tsc_timestamp is calculated at kvm_guest_time_update.

        Once "if (!vcpus_matched && ka->use_master_clock)" is simplified to
        "if (ka->use_master_clock)", the corresponding "if (!ka->use_master_clock)"
        becomes redundant, so remove the do_request boolean and collapse
        everything into a single condition.

    Before that, KVM only re-synced the masterclock if the masterclock was
    enabled or disabled  Note, at the time of the above commit, VMX
    synchronized TSC on *guest* writes to MSR_IA32_TSC:

            case MSR_IA32_TSC:
                    kvm_write_tsc(vcpu, msr_info);
                    break;

    which is why the changelog specifically says "guest writes", but the bug
    that was being fixed wasn't unique to guest write, i.e. a TSC write from
    the host would suffer the same problem.

    So even though KVM stopped synchronizing on guest writes as of commit
    0c899c25d754 ("KVM: x86: do not attempt TSC synchronization on guest
    writes"), simply reverting commit 7f187922ddf6 is not an option.  Figuring
    out how a negative tsc_timestamp could be computed requires a bit more
    sleuthing.

    In kvm_write_tsc() (at the time), except for KVM's "less than 1 second"
    hack, KVM snapshotted the vCPU's current TSC *and* the current time in
    nanoseconds, where kvm->arch.cur_tsc_nsec is the current host kernel time
    in nanoseconds:

            ns = get_kernel_ns();

            ...

            if (usdiff < USEC_PER_SEC &&
                vcpu->arch.virtual_tsc_khz == kvm->arch.last_tsc_khz) {
                    ...
            } else {
                    /*
                     * We split periods of matched TSC writes into generations.
                     * For each generation, we track the original measured
                     * nanosecond time, offset, and write, so if TSCs are in
                     * sync, we can match exact offset, and if not, we can match
                     * exact software computation in compute_guest_tsc()
                     *
                     * These values are tracked in kvm->arch.cur_xxx variables.
                     */
                    kvm->arch.cur_tsc_generation++;
                    kvm->arch.cur_tsc_nsec = ns;
                    kvm->arch.cur_tsc_write = data;
                    kvm->arch.cur_tsc_offset = offset;
                    matched = false;
                    pr_debug("kvm: new tsc generation %llu, clock %llu\n",
                             kvm->arch.cur_tsc_generation, data);
            }

            ...

            /* Keep track of which generation this VCPU has synchronized to */
            vcpu->arch.this_tsc_generation = kvm->arch.cur_tsc_generation;
            vcpu->arch.this_tsc_nsec = kvm->arch.cur_tsc_nsec;
            vcpu->arch.this_tsc_write = kvm->arch.cur_tsc_write;

    Note that the above creates a new generation and sets "matched" to false!
    But because kvm_track_tsc_matching() looks for matched+1, i.e. doesn't
    require the vCPU that creates the new generation to match itself, KVM
    would immediately compute vcpus_matched as true for VMs with a single vCPU.
    As a result, KVM would skip the masterlock update, even though a new TSC
    generation was created:

            vcpus_matched = (ka->nr_vcpus_matched_tsc + 1 ==
                             atomic_read(&vcpu->kvm->online_vcpus));

            if (vcpus_matched && gtod->clock.vclock_mode == VCLOCK_TSC)
                    if (!ka->use_master_clock)
                            do_request = 1;

            if (!vcpus_matched && ka->use_master_clock)
                            do_request = 1;

            if (do_request)
                    kvm_make_request(KVM_REQ_MASTERCLOCK_UPDATE, vcpu);

    On hardware without TSC scaling support, vcpu->tsc_catchup is set to true
    if the guest TSC frequency is faster than the host TSC frequency, even if
    the TSC is otherwise stable.  And for that mode, kvm_guest_time_update(),
    by way of compute_guest_tsc(), uses vcpu->arch.this_tsc_nsec, a.k.a. the
    kernel time at the last TSC write, to compute the guest TSC relative to
    kernel time:

      static u64 compute_guest_tsc(struct kvm_vcpu *vcpu, s64 kernel_ns)
      {
            u64 tsc = pvclock_scale_delta(kernel_ns-vcpu->arch.this_tsc_nsec,
                                          vcpu->arch.virtual_tsc_mult,
                                          vcpu->arch.virtual_tsc_shift);
            tsc += vcpu->arch.this_tsc_write;
            return tsc;
      }

    Except the "kernel_ns" passed to compute_guest_tsc() isn't the current
    kernel time, it's the masterclock snapshot!

            spin_lock(&ka->pvclock_gtod_sync_lock);
            use_master_clock = ka->use_master_clock;
            if (use_master_clock) {
                    host_tsc = ka->master_cycle_now;
                    kernel_ns = ka->master_kernel_ns;
            }
            spin_unlock(&ka->pvclock_gtod_sync_lock);

            if (vcpu->tsc_catchup) {
                    u64 tsc = compute_guest_tsc(v, kernel_ns);
                    if (tsc > tsc_timestamp) {
                            adjust_tsc_offset_guest(v, tsc - tsc_timestamp);
                            tsc_timestamp = tsc;
                    }
            }

    And so when KVM skips the masterclock update after a TSC write, i.e. after
    a new TSC generation is started, the "kernel_ns-vcpu->arch.this_tsc_nsec"
    is *guaranteed* to generate a negative value, because this_tsc_nsec was
    captured after ka->master_kernel_ns.

    Forcing a masterclock update essentially fudged around that problem, but
    in a heavy handed way that introduced undesirable side effects, i.e.
    unnecessarily forces a masterclock update when a new vCPU joins the party
    via hotplug.

    Note, KVM forces masterclock updates in other weird ways that are also
    likely unnecessary, e.g. when establishing a new Xen shared info page and
    when userspace creates a brand new vCPU.  But the Xen thing is firmly a
    separate mess, and there are no known userspace VMMs that utilize kvmclock
    *and* create new vCPUs after the VM is up and running.  I.e. the other
    issues are future problems.

    Reported-by: Dongli Zhang <dongli.zhang@oracle.com>
    Closes: https://lore.kernel.org/all/20230926230649.67852-1-dongli.zhang@oracle.com
    Fixes: 7f187922ddf6 ("KVM: x86: update masterclock values on TSC writes")
    Cc: David Woodhouse <dwmw2@infradead.org>
    Reviewed-by: Dongli Zhang <dongli.zhang@oracle.com>
    Tested-by: Dongli Zhang <dongli.zhang@oracle.com>
    Link: https://lore.kernel.org/r/20231018195638.1898375-1-seanjc@google.com
    Signed-off-by: Sean Christopherson <seanjc@google.com>

diff --git a/arch/x86/kvm/x86.c b/arch/x86/kvm/x86.c
index 6d0772b47041..99ec48203667 100644
--- a/arch/x86/kvm/x86.c
+++ b/arch/x86/kvm/x86.c
@@ -2510,26 +2510,29 @@ static inline int gtod_is_based_on_tsc(int mode)
 }
 #endif

-static void kvm_track_tsc_matching(struct kvm_vcpu *vcpu)
+static void kvm_track_tsc_matching(struct kvm_vcpu *vcpu, bool new_generation)
 {
 #ifdef CONFIG_X86_64
-	bool vcpus_matched;
 	struct kvm_arch *ka = &vcpu->kvm->arch;
 	struct pvclock_gtod_data *gtod = &pvclock_gtod_data;

-	vcpus_matched = (ka->nr_vcpus_matched_tsc + 1 ==
-			 atomic_read(&vcpu->kvm->online_vcpus));
+	/*
+	 * To use the masterclock, the host clocksource must be based on TSC
+	 * and all vCPUs must have matching TSCs.  Note, the count for matching
+	 * vCPUs doesn't include the reference vCPU, hence "+1".
+	 */
+	bool use_master_clock = (ka->nr_vcpus_matched_tsc + 1 ==
+				 atomic_read(&vcpu->kvm->online_vcpus)) &&
+				gtod_is_based_on_tsc(gtod->clock.vclock_mode);

 	/*
-	 * Once the masterclock is enabled, always perform request in
-	 * order to update it.
-	 *
-	 * In order to enable masterclock, the host clocksource must be TSC
-	 * and the vcpus need to have matched TSCs.  When that happens,
-	 * perform request to enable masterclock.
+	 * Request a masterclock update if the masterclock needs to be toggled
+	 * on/off, or when starting a new generation and the masterclock is
+	 * enabled (compute_guest_tsc() requires the masterclock snapshot to be
+	 * taken _after_ the new generation is created).
 	 */
-	if (ka->use_master_clock ||
-	    (gtod_is_based_on_tsc(gtod->clock.vclock_mode) && vcpus_matched))
+	if ((ka->use_master_clock && new_generation) ||
+	    (ka->use_master_clock != use_master_clock))
 		kvm_make_request(KVM_REQ_MASTERCLOCK_UPDATE, vcpu);

 	trace_kvm_track_tsc(vcpu->vcpu_id, ka->nr_vcpus_matched_tsc,
@@ -2706,7 +2709,7 @@ static void __kvm_synchronize_tsc(struct kvm_vcpu *vcpu, u64 offset, u64 tsc,
 	vcpu->arch.this_tsc_nsec = kvm->arch.cur_tsc_nsec;
 	vcpu->arch.this_tsc_write = kvm->arch.cur_tsc_write;

-	kvm_track_tsc_matching(vcpu);
+	kvm_track_tsc_matching(vcpu, !matched);
 }

 static void kvm_synchronize_tsc(struct kvm_vcpu *vcpu, u64 *user_value)

```

###  2024/2/27 KVM: x86/xen: improve accuracy of Xen timers

```txt
commit 451a707813aee24b4a734f28d1d414be0360862b
Author: David Woodhouse <dwmw@amazon.co.uk>
Date:   Tue Feb 27 11:49:15 2024 +0000

    KVM: x86/xen: improve accuracy of Xen timers

    A test program such as http://david.woodhou.se/timerlat.c confirms user
    reports that timers are increasingly inaccurate as the lifetime of a
    guest increases. Reporting the actual delay observed when asking for
    100µs of sleep, it starts off OK on a newly-launched guest but gets
    worse over time, giving incorrect sleep times:

    root@ip-10-0-193-21:~# ./timerlat -c -n 5
    00000000 latency 103243/100000 (3.2430%)
    00000001 latency 103243/100000 (3.2430%)
    00000002 latency 103242/100000 (3.2420%)
    00000003 latency 103245/100000 (3.2450%)
    00000004 latency 103245/100000 (3.2450%)

    The biggest problem is that get_kvmclock_ns() returns inaccurate values
    when the guest TSC is scaled. The guest sees a TSC value scaled from the
    host TSC by a mul/shift conversion (hopefully done in hardware). The
    guest then converts that guest TSC value into nanoseconds using the
    mul/shift conversion given to it by the KVM pvclock information.

    But get_kvmclock_ns() performs only a single conversion directly from
    host TSC to nanoseconds, giving a different result. A test program at
    http://david.woodhou.se/tsdrift.c demonstrates the cumulative error
    over a day.

    It's non-trivial to fix get_kvmclock_ns(), although I'll come back to
    that. The actual guest hv_clock is per-CPU, and *theoretically* each
    vCPU could be running at a *different* frequency. But this patch is
    needed anyway because...

    The other issue with Xen timers was that the code would snapshot the
    host CLOCK_MONOTONIC at some point in time, and then... after a few
    interrupts may have occurred, some preemption perhaps... would also read
    the guest's kvmclock. Then it would proceed under the false assumption
    that those two happened at the *same* time. Any time which *actually*
    elapsed between reading the two clocks was introduced as inaccuracies
    in the time at which the timer fired.

    Fix it to use a variant of kvm_get_time_and_clockread(), which reads the
    host TSC just *once*, then use the returned TSC value to calculate the
    kvmclock (making sure to do that the way the guest would instead of
    making the same mistake get_kvmclock_ns() does).

    Sadly, hrtimers based on CLOCK_MONOTONIC_RAW are not supported, so Xen
    timers still have to use CLOCK_MONOTONIC. In practice the difference
    between the two won't matter over the timescales involved, as the
    *absolute* values don't matter; just the delta.

    This does mean a new variant of kvm_get_time_and_clockread() is needed;
    called kvm_get_monotonic_and_clockread() because that's what it does.

    Fixes: 536395260582 ("KVM: x86/xen: handle PV timers oneshot mode")
    Signed-off-by: David Woodhouse <dwmw@amazon.co.uk>
    Reviewed-by: Paul Durrant <paul@xen.org>
    Link: https://lore.kernel.org/r/20240227115648.3104-2-dwmw2@infradead.org
    [sean: massage moved comment, tweak if statement formatting]
    Signed-off-by: Sean Christopherson <seanjc@google.com>
```

## 5 分钟会从 master lock 刷新一次 pvti
```c
void kvm_arch_vcpu_postcreate(struct kvm_vcpu *vcpu)
{
	struct kvm *kvm = vcpu->kvm;

	if (mutex_lock_killable(&vcpu->mutex))
		return;
	vcpu_load(vcpu);
	kvm_synchronize_tsc(vcpu, NULL);
	vcpu_put(vcpu);

	/* poll control enabled by default */
	vcpu->arch.msr_kvm_poll_control = 1;

	mutex_unlock(&vcpu->mutex);

	if (kvmclock_periodic_sync && vcpu->vcpu_idx == 0)
		schedule_delayed_work(&kvm->arch.kvmclock_sync_work,
						KVMCLOCK_SYNC_PERIOD);
}
```

但是每次运行时间都是比 300s 要长的几十秒钟(这个不符合预期):
```txt
🧀  dmesg | grep martins3
[100340.748127] kvm: [martins3:kvmclock_sync_fn:3472]
[100668.432688] kvm: [martins3:kvmclock_sync_fn:3472]
[100996.112959] kvm: [martins3:kvmclock_sync_fn:3472]
[101323.791030] kvm: [martins3:kvmclock_sync_fn:3472]
[101651.467769] kvm: [martins3:kvmclock_sync_fn:3472]
[101979.143953] kvm: [martins3:kvmclock_sync_fn:3472]
[102306.830156] kvm: [martins3:kvmclock_sync_fn:3472]
```

这个只是周期性的刷新 workqueue 的时间，不是去刷新 workqueue 的时间

### 拿到更新 master clcok 之后，不会去立刻把 pvti 更新一下吗?
而是需要这个周期的 sync 做什么?

### 好好来理解下报错吧

```txt
localhost login: [ 2932.304608] clocksource: Long readout interval, skipping watchdog check: cs_nsec: 4432020902 wd_nsec: 496086735
```

```txt
[Tue Feb 18 14:46:38 2025] kvm: after  : 8794755721648 2932293385446 102995203823253
[Tue Feb 18 14:46:38 2025] kvm: before : 251341974 29941291 102999139814137
```

哎，原来都是在他们的计算之中，为了让虚拟机即便是暂停，时间也会是稳定的。

## 两个问题
```txt
[112365.449943]  dump_stack_lvl+0x77/0xb0
[112365.449947]  __kvm_synchronize_tsc+0xcb/0x1c0 [kvm]
[112365.450005]  kvm_synchronize_tsc+0xe8/0x230 [kvm]
[112365.450044]  ? vmx_vcpu_load+0x45/0xf0 [kvm_intel]
[112365.450052]  kvm_arch_vcpu_postcreate+0x3e/0x90 [kvm]
[112365.450093]  kvm_vm_ioctl+0x1784/0x18a0 [kvm]
[112365.450126]  ? __slab_free+0xdf/0x300
[112365.450129]  __x64_sys_ioctl+0x99/0xe0
[112365.450132]  do_syscall_64+0xc1/0x220
[112365.450134]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
```

```txt
[112365.450299] Hardware name: ASUS System Product Name/TUF GAMING B660-PLUS WIFID4, BIOS 1620 08/12/2022
[112365.450300] Call Trace:
[112365.450301]  <TASK>
[112365.450301]  dump_stack_lvl+0x77/0xb0
[112365.450303]  __kvm_synchronize_tsc+0xcb/0x1c0 [kvm]
[112365.450341]  kvm_synchronize_tsc+0xe8/0x230 [kvm]
[112365.450380]  kvm_set_msr_common+0x9dc/0x12a0 [kvm]
[112365.450420]  vmx_set_msr+0xc8b/0x1370 [kvm_intel]
[112365.450429]  ? __virt_addr_valid+0xff/0x180
[112365.450432]  ? preempt_count_sub+0x51/0x60
[112365.450435]  kvm_set_msr_ignored_check+0xa2/0x2f0 [kvm]
[112365.450474]  kvm_arch_vcpu_ioctl+0xd80/0x1920 [kvm]
[112365.450514]  ? vmx_vcpu_pi_put+0x69/0x90 [kvm_intel]
[112365.450521]  ? vmx_vcpu_put+0x2c/0x200 [kvm_intel]
[112365.450528]  ? vmx_vcpu_load+0x45/0xf0 [kvm_intel]
[112365.450534]  ? kvm_vcpu_ioctl+0x731/0x980 [kvm]
[112365.450564]  kvm_vcpu_ioctl+0x731/0x980 [kvm]
[112365.450595]  ? ioctl_has_perm.constprop.0.isra.0+0xd2/0x140
[112365.450598]  __x64_sys_ioctl+0x99/0xe0
[112365.450600]  do_syscall_64+0xc1/0x220
[112365.450602]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[112365.450603] RIP: 0033:0x7f2f37077aef
```
## 问题是，测试结果就是这样的
使用这个 diff ，结果为:
```diff
diff --git a/arch/x86/kvm/x86.c b/arch/x86/kvm/x86.c
index c79a8cc57ba4..c045af3484d7 100644
--- a/arch/x86/kvm/x86.c
+++ b/arch/x86/kvm/x86.c
@@ -2509,6 +2509,7 @@ static void kvm_track_tsc_matching(struct kvm_vcpu *vcpu, bool new_generation)
 	    (ka->use_master_clock != use_master_clock))
 		kvm_make_request(KVM_REQ_MASTERCLOCK_UPDATE, vcpu);

+	kvm_make_request(KVM_REQ_MASTERCLOCK_UPDATE, vcpu);
 	trace_kvm_track_tsc(vcpu->vcpu_id, ka->nr_vcpus_matched_tsc,
 			    atomic_read(&vcpu->kvm->online_vcpus),
 		            ka->use_master_clock, gtod->clock.vclock_mode);
@@ -3188,6 +3189,9 @@ static int kvm_guest_time_update(struct kvm_vcpu *v)
 {
 	unsigned long flags, tgt_tsc_khz;
 	unsigned seq;
+	u64 tsc;
+	u64 a;
+	u64 b;
 	struct kvm_vcpu_arch *vcpu = &v->arch;
 	struct kvm_arch *ka = &v->kvm->arch;
 	s64 kernel_ns;
@@ -3270,8 +3274,18 @@ static int kvm_guest_time_update(struct kvm_vcpu *v)
 		kvm_xen_update_tsc_info(v);
 	}

+	tsc = rdtsc();
+	a = __pvclock_read_cycles(&vcpu->hv_clock, tsc);
+	pr_info("before : %d %lld %lld %lld", v->vcpu_id, vcpu->hv_clock.tsc_timestamp,
+		vcpu->hv_clock.system_time, a);
 	vcpu->hv_clock.tsc_timestamp = tsc_timestamp;
 	vcpu->hv_clock.system_time = kernel_ns + v->kvm->arch.kvmclock_offset;
+
+	b = __pvclock_read_cycles(&vcpu->hv_clock, tsc);
+	pr_info("after  : %d %lld %lld %lld", v->vcpu_id, vcpu->hv_clock.tsc_timestamp,
+		vcpu->hv_clock.system_time, b);
+
+	pr_info("after  : %d %lld", v->vcpu_id, b - a);
 	vcpu->last_guest_tsc = tsc_timestamp;

 	/* If the host uses TSC clocksource, then it is stable */
```

截取其中部分日志:
```txt
[114981.577204] kvm: before : 8 204258062 29699201 115006207877777
[114981.577207] kvm: after  : 8 3742847527892 1249576837834 115006207984959
[114981.577208] kvm: after  : 8 107182

[114981.577262] kvm: before : 0 204258062 29699201 115006207935083
[114981.577264] kvm: after  : 0 3742847527892 1249576837834 115006208042265
[114981.577264] kvm: after  : 0 107182
```

大约 5 小时的结果:
```txt
kvm: after  : 27 54656195394162 18247891546961 132004522793223
kvm: before : 27 7908094115450 2640217510736 132004521454443
kvm: after  : 27 1338780
```

从 Guest 内核日志中看，-1338644 和里面对比外面计算的实际上一样
```txt
[18247.797300] clocksource: timekeeping watchdog on CPU28: Marking clocksource 'tsc' as unstable because the skew is too large:
[18247.797763] clocksource:                       'kvm-clock' wd_nsec: 504058793 wd_now: 1098b25ce802 wd_last: 109894519459 mask: ffffffffffffffff
[18247.798130] clocksource:                       'tsc' cs_nsec: 502720149 cs_now: 31b5b8e26b90 cs_last: 31b55f228a50 mask: ffffffffffffffff
[18247.798484] clocksource:                       Clocksource 'tsc' skewed -1338644 ns (-1 ms) over watchdog 'kvm-clock' interval of 504058793 ns (504 ms)
[18247.798898] clocksource:                       'kvm-clock' (not 'tsc') is current clocksource.
[18247.799142] tsc: Marking TSC unstable due to clocksource watchdog
```

不过为什么要 mark tsc unstable 啊
```txt
🧀  cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource
kvm-clock
kvm-clock hpet acpi_pm
```


```txt
[174025.504073] kvm: before : 30 192508862005 64235014617 174050135841104
[174025.504076] kvm: after  : 30 2323040359326 775550345604 174050135902119
[174025.504077] kvm: after  : 30 61015
```


虚拟机开机的时候，会计算出来一个日志
```txt
[Wed Feb 19 10:17:54 2025] kvm: before : 0 0 0 173274631358872
[Wed Feb 19 10:17:54 2025] kvm: after  : 0 137870224 8582920 173274593911402
[Wed Feb 19 10:17:54 2025] kvm: after  : 0 -37447470
```


```txt
[183823.273533] kvm: before : 0 96320805251 32112961887 367616285727176
[183823.273535] kvm: kvmclock_offset: 0 -183743600028900
[183823.273537] kvm: after  : 0 238280086662 79508559357 367616285731241
[183823.273538] kvm: after  : 0 4065
```

但是还是可以计算出来这个差值:
```txt
[Wed Feb 19 14:45:53 2025] kvm: before : 1 207668875453 69302402303 1755085244301 <5256925645464> [5049256770011]
[Wed Feb 19 14:45:53 2025] kvm: kvmclock_offset: 1 -187574044565522
[Wed Feb 19 14:45:53 2025] kvm: after  : 1 5256925644003 1755085388416 1755085388903
[Wed Feb 19 14:45:53 2025] kvm: after  : 1 144602
```

当时虚拟机中的 boottime 为：
```txt
[ 1959.510660] 1
```

## 好的，为什么物理机的算法有那么大的误差

```txt
[204703.657563] kvm: -> 8301162777387368
[204703.657567] kvm: -> 204703000000000

[204736.610510] kvm: -> 7511803395706864
[204736.610514] kvm: -> 204736000000000
```

仔细想想，时间的误差是什么?


## 那么，之前为什么需要更新 masterclock

里面的 generation 是做什么的?

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
