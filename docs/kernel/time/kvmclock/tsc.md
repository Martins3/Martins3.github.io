# 为什么不去直接使用 tsc 来解决问题

其实，配置一下参数就可以了:

-cpu host,migratable=no,+invtsc,tsc-frequency=1000000000

## migratable 的含义是什么

```c
struct ArchCPU {
    // ...
    bool migratable;
```

其唯一使用的地方在于:

- x86_cpu_get_supported_feature_word -> x86_cpu_get_migratable_flags 中

所以，这个就是一个 legacy 东西了，有 tsc-freq 其实设置不设置都无所谓。

但是如何理解，即便是使用这个方法，依旧可以实现效果
model="-cpu host,migratable=no,tsc-frequency=1000000000"

应该是这个函数实现的:
```c
static bool tsc_is_stable_and_known(CPUX86State *env)
{
    if (!env->tsc_khz) {
        return false;
    }
    return (env->features[FEAT_8000_0007_EDX] & CPUID_APM_INVTSC)
        || env->user_tsc_khz;
}
```


## +invtsc 是什么意思?

一个新添加的 cpuid :
`FEAT_8000_0007_EDX`

## 所以，为什么一定需要设置 tsc-freq ?

tsc-freq 和 invtsc 这个变量:

一个完全不耦合的东西，在 kvm_arch_init_vcpu 中检查的东西:
```c
    if (!env->user_tsc_khz) {
        if ((env->features[FEAT_8000_0007_EDX] & CPUID_APM_INVTSC) &&
            invtsc_mig_blocker == NULL) {
            error_setg(&invtsc_mig_blocker,
                       "State blocked by non-migratable CPU device"
                       " (invtsc flag)");
            r = migrate_add_blocker(&invtsc_mig_blocker, &local_err);
            if (r < 0) {
                error_report_err(local_err);
                return r;
            }
        }
    }
```
## kernel 中测试

```txt
cat /proc/cpuinfo | grep -m 1 "flags.*:" | grep -o -E '\S*tsc\S*'
```

```c
	/*
	 * X86_FEATURE_NONSTOP_TSC is TSC runs at constant rate
	 * with P/T states and does not stop in deep C-states.
	 *
	 * Invariant TSC exposed by host means kvmclock is not necessary:
	 * can use TSC as clocksource.
	 *
	 */
	if (boot_cpu_has(X86_FEATURE_CONSTANT_TSC) &&
	    boot_cpu_has(X86_FEATURE_NONSTOP_TSC) &&
	    !check_tsc_unstable())
		kvm_clock.rating = 299;
```

物理机中可以看到的:
```txt
tsc
rdtscp
constant_tsc
nonstop_tsc
tsc_known_freq
tsc_deadline_timer
tsc_adjust
```

虚拟机中看到的:
```txt
tsc
rdtscp
constant_tsc
tsc_known_freq
tsc_deadline_timer
tsc_adjust
```

可以检查下，在哪里屏蔽的。

# 为什么当使用 kvm-clock 的时候，本地迁移有警告啊

```txt
[   35.166702] clocksource: timekeeping watchdog on CPU5: Marking clocksource 'tsc' as unstable because the skew is too large:
[   35.167718] clocksource:                       'kvm-clock' wd_nsec: 504113713 wd_now: 838861f0c wd_last: 81a79f4db mask: ffffffffffffffff
[   35.168490] clocksource:                       'tsc' cs_nsec: 489600636 cs_now: 18a01d7d86 cs_last: 1848b53691 mask: ffffffffffffffff
[   35.169172] clocksource:                       Clocksource 'tsc' skewed -14513077 ns (-14 ms) over watchdog 'kvm-clock' interval of 504113713 ns (504 ms)
[   35.169952] clocksource:                       'kvm-clock' (not 'tsc') is current clocksource.
[   35.170432] tsc: Marking TSC unstable due to clocksource watchdog
```

## 但是，为什么平时热迁移的时候，没有看到这个错误
似乎是我们的虚拟机 kernel 太新了。做了算法的修正的?

之前的检测标准是 WATCHDOG_THRESHOLD，也就是 250ms

现在检测标准是
```txt
		md = cs->uncertainty_margin + watchdog->uncertainty_margin;
```
250000ns + 250000ns

很容易找到修正的地方:
commit 2e27e793e280 ("clocksource: Reduce clocksource-skew threshold")

## 为什么当使用 kvm-clock 作为 clocksource 的时候
clocksource_watchdog 会不停的调用，但是当使用 tsc 作为 clock source 的时候，
clocksource_watchdog 就完全不会被调用。

原因 : 如果使用 tsc 作为 clocksource ，
```c
static inline void clocksource_start_watchdog(void)
{
	pr_info("[martins3:%s:%d] %d %d %d\n", __FUNCTION__, __LINE__,
		list_empty(&watchdog_list), watchdog_running, !watchdog);
	if (watchdog_running || !watchdog || list_empty(&watchdog_list))
		return;
	timer_setup(&watchdog_timer, clocksource_watchdog, 0);
	watchdog_timer.expires = jiffies + WATCHDOG_INTERVAL;
	add_timer_on(&watchdog_timer, cpumask_first(cpu_online_mask));
	watchdog_running = 1;
}
```

对应的输出是:

```txt
[    0.000003] clocksource: [martins3:clocksource_start_watchdog:616] 1 0 0
[    0.091622] clocksource: [martins3:clocksource_start_watchdog:616] 1 0 0
[    0.221111] clocksource: [martins3:clocksource_start_watchdog:616] 1 0 0
[    0.224682] clocksource: [martins3:clocksource_start_watchdog:616] 1 0 0
[    0.245581] clocksource: [martins3:clocksource_start_watchdog:616] 1 0 0
[    0.299557] clocksource: [martins3:clocksource_start_watchdog:616] 1 0 0
[    0.312434] clocksource: [martins3:clocksource_start_watchdog:616] 1 0 0
```
所以，就是这样的了

# 即便是使用 kvm-clock ，在热迁移的时候也会设置 tsc-khz 的
是的，跟踪 kvm_set_tsc_khz 即可

# 切换 clocksource 不会导致问题，因为都是使用 timer keeper 中内容的
是的。

# [ ]  当 host 的 tsc 被 mark unstable 之后，虚拟机又是使用的 tsc 作为 clocksource
那么虚拟机该如何办?


# 研究一下当物理机是 hpet 的时候

虚拟机中的观察:
```txt
🤒  cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource
kvm-clock
kvm-clock tsc hpet acpi_pm
```

如果正确配置一下之后，那么就有:
```txt
tsc
tsc kvm-clock hpet acpi_pm
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
