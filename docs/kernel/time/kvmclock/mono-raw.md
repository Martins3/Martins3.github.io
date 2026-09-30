## 问题是，为什么同步 host 的 CLOCK_MONOTONIC_RAW 到 guest 中会导致问题

参考 commit c52ffadc65e28ab461fd055e9991e8d8106a0056 中:

>    The unexpected time jumps are due to differences in the TSC=>nanoseconds
>    conversion algorithms between kvmclock and the host's CLOCK_MONOTONIC_RAW
>    (the pvclock algorithm is inherently lossy).  When updating the
>    masterclock, KVM refreshes the "base", i.e. moves the elapsed time since
>    the last update from the kvmclock/pvclock algorithm to the
>    CLOCK_MONOTONIC_RAW algorithm.  Synchronizing kvmclock with
>    CLOCK_MONOTONIC_RAW is the lesser of evils when the TSC is unstable, but
>    adds no real value when the TSC is stable.

kvmclock 获取使用的是 CLOCK_MONOTONIC_RAW 的:

arch/x86/kvm/x86.c 中还是使用了 do_monotonic 和 do_realtime 的:

对于 do_monotonic 的使用场景:
- kvm_get_monotonic_and_clockread : 被 xen 使用的
  - do_monotonic

对于 do_realtime 的使用场景:
- kvm_pv_clock_pairing : ptp driver 相关，暂时不考虑了
- kvm_set_msr_common -> kvm_write_wall_clock -> kvm_get_wall_clock_epoch : 用于刷新 msr wallclock 的时候
- get_kvmclock -> __get_kvmclock
  - kvm_get_walltime_and_clockread
    - do_realtime : 这个获取到的是真正的时间，其基准是 wall_time_sec

- kvm_vm_ioctl_get_clock
- get_kvmclock_ns
  - get_kvmclock

```c
/*
 * The pvclock_wall_clock ABI tells the guest the wall clock time at
 * which it started (i.e. its epoch, when its kvmclock was zero).
 *
 * In fact those clocks are subtly different; wall clock frequency is
 * adjusted by NTP and has leap seconds, while the kvmclock is a
 * simple function of the TSC without any such adjustment.
 *
 * Perhaps the ABI should have exposed CLOCK_TAI and a ratio between
 * that and kvmclock, but even that would be subject to change over
 * time.
 *
 * Attempt to calculate the epoch at a given moment using the *same*
 * TSC reading via kvm_get_walltime_and_clockread() to obtain both
 * wallclock and kvmclock times, and subtracting one from the other.
 *
 * Fall back to using their values at slightly different moments by
 * calling ktime_get_real_ns() and get_kvmclock_ns() separately.
 */
uint64_t kvm_get_wall_clock_epoch(struct kvm *kvm)
```

这个注释道出了关键:
1. wall clock frequency is adjusted by NTP and has leap seconds
2. while the kvmclock is a simple function of the TSC without any such adjustment.

### 难道会出现，虽然 host 的时间是正确的，只是因为被 ntp 调整过，所以会出现将 CLOCK_MONOTONIC_RAW 同步到 guest 的时候，导致 guest 时间错误吗？

```c
		host_tsc = rdtsc();
		kernel_ns = get_kvmclock_base_ns();
```

因为 host 的时间实际上可以正确的，因为存在 ntp 的校准，而且 guest 也是被 ntp 校准的，即便如此，当将 guest 的时间同步给 host 的时间后，
还是可以出现问题，因为:

## 备忘
- get_kvmclock_base_ns : boot ns

1. 如何理解 gtod_is_based_on_tsc ，这里判断到底是为了什么?

```c
static inline bool gtod_is_based_on_tsc(int mode)
{
	return mode == VDSO_CLOCKMODE_TSC || mode == VDSO_CLOCKMODE_HVCLOCK;
}
```

```c
enum vdso_clock_mode {
	VDSO_CLOCKMODE_NONE,
	VDSO_CLOCKMODE_TSC,
	VDSO_CLOCKMODE_PVCLOCK, // <--- 为什么这个不行
	VDSO_CLOCKMODE_HVCLOCK
	VDSO_CLOCKMODE_MAX,

	/* Indicator for time namespace VDSO */
	VDSO_CLOCKMODE_TIMENS = INT_MAX
};
```

为什么 VDSO_CLOCKMODE_HVCLOCK 又是可以的?

```c
static inline u64 __arch_get_hw_counter(s32 clock_mode,
					const struct vdso_data *vd)
{
	if (likely(clock_mode == VDSO_CLOCKMODE_TSC))
		return (u64)rdtsc_ordered() & S64_MAX;
	/*
	 * For any memory-mapped vclock type, we need to make sure that gcc
	 * doesn't cleverly hoist a load before the mode check.  Otherwise we
	 * might end up touching the memory-mapped page even if the vclock in
	 * question isn't enabled, which will segfault.  Hence the barriers.
	 */
#ifdef CONFIG_PARAVIRT_CLOCK
	if (clock_mode == VDSO_CLOCKMODE_PVCLOCK) {
		barrier();
		return vread_pvclock();
	}
#endif
#ifdef CONFIG_HYPERV_TIMER
	if (clock_mode == VDSO_CLOCKMODE_HVCLOCK) {
		barrier();
		return vread_hvclock();
	}
#endif
	return U64_MAX;
}
```


2. PVCLOCK_GUEST_STOPPED 按道理可以解决问题才对啊

```c
	if (dowd && unlikely((flags & PVCLOCK_GUEST_STOPPED) != 0)) {
		src->flags &= ~PVCLOCK_GUEST_STOPPED;
		pvclock_touch_watchdogs();
	}
```

3. 在 vgettsc 中做区分，不懂 hyper-v 的结果，那么可以简化为:

vgettsc 中为:
```c
case VDSO_CLOCKMODE_TSC:
		*mode = VDSO_CLOCKMODE_TSC;
		*tsc_timestamp = read_tsc();
		v = (*tsc_timestamp - clock->cycle_last) &
			clock->mask;
		break;
```

- kvm_get_time_and_clockread : 仅仅用于更新 masterclock
  - do_kvmclock_base

- kvm_get_monotonic_and_clockread : 只有 xen 使用
  - do_monotonic

- kvm_get_walltime_and_clockread : wallclock 或者当 host 用的不是 tsc 的时候
  - do_realtime

- get_kvmclock_base_ns 和 do_kvmclock_base 有什么区别?

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
