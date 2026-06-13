## IRQ_TIME_ACCOUNTING

```c
config IRQ_TIME_ACCOUNTING
	bool "Fine granularity task level IRQ time accounting"
	depends on HAVE_IRQ_TIME_ACCOUNTING && !VIRT_CPU_ACCOUNTING_NATIVE
	help
	  Select this option to enable fine granularity task irq time
	  accounting. This is done by reading a timestamp on each
	  transitions between softirq and hardirq state, so there can be a
	  small performance impact.

	  If in doubt, say N here.
```

似乎同时有好几个地方来统计:
```txt
@[
    irqtime_account_irq+5
    __irq_exit_rcu+67
    irq_exit_rcu+14
    common_interrupt+136
    asm_common_interrupt+38
    default_idle+19
    default_idle_call+71
    do_idle+244
    cpu_startup_entry+42
    start_secondary+156
    common_startup_64+318
]: 204737
```

- https://unix.stackexchange.com/questions/242835/is-top-accounting-for-kernel-interrupts
  - https://lore.kernel.org/lkml/1286237003-12406-1-git-send-email-venki@google.com/
    - 这里指出来的原始的 patch ，大致的意思是，以前会把 irq 的时间计算到 task 中，这其实对于 scheduler 产生了噪音
    - 此外一个好处就是，现在通过 /proc/stat 可以看到 softirq 和 hardirq  的时间

关于 /proc/stat ，参考 sysfs-sched.md


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
