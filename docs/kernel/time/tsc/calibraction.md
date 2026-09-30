# tsc calibraction
<!-- 335009c6-4078-4fef-8442-c5cf8be8909e -->

init_tsc_clocksource 中，如果存在 tsc_known_freq 这个 feature ，就不用校准了
否则才需要校准:

```c
	/*
	 * When TSC frequency is known (retrieved via MSR or CPUID), we skip
	 * the refined calibration and directly register it as a clocksource.
	 */
	if (boot_cpu_has(X86_FEATURE_TSC_KNOWN_FREQ)) {
		if (boot_cpu_has(X86_FEATURE_ART)) {
			have_art = true;
			clocksource_tsc.base = &art_base_clk;
		}
		clocksource_register_khz(&clocksource_tsc, tsc_khz);
		clocksource_unregister(&clocksource_tsc_early);

		if (!tsc_force_recalibrate)
			return 0;
	}

	schedule_delayed_work(&tsc_irqwork, 0);
```

核心工作都是在 tsc_refine_calibration_work 中:

```c
	/* Make sure we're within 1% */
	if (abs(tsc_khz - freq) > tsc_khz/100)
		goto out;

	tsc_khz = freq;
	pr_info("Refined TSC clocksource calibration: %lu.%03lu MHz\n",
		(unsigned long)tsc_khz / 1000,
		(unsigned long)tsc_khz % 1000);
```

tsc_khz 一般来说，从 CPUID 之类的地方获取

```c
static bool __init determine_cpu_tsc_frequencies(bool early)
{
	/* Make sure that cpu and tsc are not already calibrated */
	WARN_ON(cpu_khz || tsc_khz);

	if (early) {
		cpu_khz = x86_platform.calibrate_cpu();
		if (tsc_early_khz) {
			tsc_khz = tsc_early_khz;
		} else {
			tsc_khz = x86_platform.calibrate_tsc();
			clocksource_tsc.freq_khz = tsc_khz;
		}
	} else {
		/* We should not be here with non-native cpu calibration */
		WARN_ON(x86_platform.calibrate_cpu != native_calibrate_cpu);
		cpu_khz = pit_hpet_ptimer_calibrate_cpu();
	}
	// ...
```

freq 从 hpet 或者 acpi 中获取的结果
如果 freq 和 tsc_khz 差别在 1% 之内，
那么就输出
```txt
	pr_info("Refined TSC clocksource calibration: %lu.%03lu MHz\n",
		(unsigned long)tsc_khz / 1000,
		(unsigned long)tsc_khz % 1000);
```

否则，相当于不去校准了。 这往往意味着 tsc 会存在较大偏差，

## 为什么让 hpet 来校准 tsc

总体印象，hpet 似乎不是一个更好的时钟，而 tsc 更好。
那么使用 hpet 来校准 tsc 不是在用一个差的东西来校准好的?

我的理解是，hpet 是主板上的，精度更高，但是访问延迟很大，而 tsc 访问延迟低，开销小
但是精度其实是没有 hpet 高的

./hpet-tsc-compare.c 中直接来读取 hpet ，然后对比 tsc ，
其实两者都是准的。

## 虚拟机中会校准吗?

虚拟机中总是携带了 X86_FEATURE_TSC_KNOWN_FREQ ，需要特殊参数

tsc=recalibrate 实际上只做一件事：
即使 CPU 声称自己知道 TSC 频率，内核也要再用 HPET/PM_TIMER 这种硬件时钟源实测一遍，并把结果打印出来。

tsc_early_khz=2000000 在 determine_cpu_tsc_frequencies() 如果 tsc_early_khz 非零，early
阶段直接把 tsc_khz 设成这个值，跳过平台默认的 calibrate_tsc()。


## 参考
https://en.wikipedia.org/wiki/High_Precision_Event_Timer

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
