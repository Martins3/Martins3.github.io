## hypervisor 的工作
基本在 kvm_get_wall_clock_epoch 中处理的:

get_cpu 是如何保证不会且到其他的内核中的:
```c
		/*
		 * The TSC read and the call to get_cpu_tsc_khz() must happen
		 * on the same CPU.
		 */
		get_cpu();

		local_tsc_khz = get_cpu_tsc_khz();

		if (local_tsc_khz &&
		    !kvm_get_walltime_and_clockread(&ts, &host_tsc))
			local_tsc_khz = 0; /* Fall back to old method */

		put_cpu();
```

- kvm_get_wall_clock_epoch 上的注释是什么意思?

- kvm_write_wall_clock : 读写 MSR 的地方
  - kvm_get_wall_clock_epoch

## 虚拟机中使用

kvm_get_wallclock 是 guest os 使用 wallclock 的地方，很显然，开机

- kvm_get_wallclock

```c
static void kvm_get_wallclock(struct timespec64 *now)
{
	wrmsrl(msr_kvm_wall_clock, slow_virt_to_phys(&wall_clock));
	preempt_disable();
	pvclock_read_wallclock(&wall_clock, this_cpu_pvti(), now);
	preempt_enable();
}
```
首先触发一个 kvm_write_wall_clock ，然后 host kvm 中会刷新数值。

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
