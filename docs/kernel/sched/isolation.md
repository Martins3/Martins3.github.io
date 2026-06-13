# cpu isolation

- https://stackoverflow.com/questions/38112585/tickless-kernel-isolcpus-nohz-full-and-rcu-nocbs

- https://www.suse.com/c/cpu-isolation-housekeeping-and-tradeoffs-part-4/ : 这个连续好几篇文章
- https://www.suse.com/c/cpu-isolation-full-dynticks-part2/
  - cputime accounting 可以理解，但是 RCU 的不能理解
    - rcu writer 需要知道什么可以开始回收了，这个需要有人通知。

使用场景:
- virtualization hosts that want to maximize CPU resources for the guest
- CPU bound benchmarks for stable results
- specific real time needs, etc……

## 这些问题都是什么方法
```c
enum hk_type {
	HK_TYPE_TIMER,
	HK_TYPE_RCU,
	HK_TYPE_MISC,
	HK_TYPE_SCHED,
	HK_TYPE_TICK,
	HK_TYPE_DOMAIN,
	HK_TYPE_WQ,
	HK_TYPE_MANAGED_IRQ,
	HK_TYPE_KTHREAD,
	HK_TYPE_MAX
};
```

## 使用
如果打开了
```c
	if (housekeeping_cpu(cpu, HK_TYPE_TICK))
		arch_scale_freq_tick();
```

## 据说，当采用 full dynticks 的时候，会让一些上下文切换开销变大
如下的上下文。
- Syscalls
- Exceptions (page faults, traps, ……)
- IRQs
的开销变大。

开销来源是 cputime accounting 和 RCU tracking and ordering

- [ ] 应该存在一个统一的入口吧!
- 简单找了下，没有找到对应位置的证据。

## 分析一下 sched_isolation.c
- [ ] housekeeping_affine

## 提供了两个 kernel 参数
1. `nohz_full=` 自动包含了 `rcu_nocbs`
2. `isolcpus=` : 控制到底 isolate 啥
3. `rcu_nocbs=`

## 看看文档吧
### isolcpus
```txt
        isolcpus=       [KNL,SMP,ISOL] Isolate a given set of CPUs from disturbance.
                        [Deprecated - use cpusets instead]
                        Format: [flag-list,]<cpu-list>

                        Specify one or more CPUs to isolate from disturbances
                        specified in the flag list (default: domain):

                        nohz
                          Disable the tick when a single task runs.

                          A residual 1Hz tick is offloaded to workqueues, which you
                          need to affine to housekeeping through the global
                          workqueue's affinity configured via the
                          /sys/devices/virtual/workqueue/cpumask sysfs file, or
                          by using the 'domain' flag described below.

                          NOTE: by default the global workqueue runs on all CPUs,
                          so to protect individual CPUs the 'cpumask' file has to
                          be configured manually after bootup.

                        domain
                          Isolate from the general SMP balancing and scheduling
                          algorithms. Note that performing domain isolation this way
                          is irreversible: it's not possible to bring back a CPU to
                          the domains once isolated through isolcpus. It's strongly
                          advised to use cpusets instead to disable scheduler load
                          balancing through the "cpuset.sched_load_balance" file.
                          It offers a much more flexible interface where CPUs can
                          move in and out of an isolated set anytime.

                          You can move a process onto or off an "isolated" CPU via
                          the CPU affinity syscalls or cpuset.
                          <cpu number> begins at 0 and the maximum value is
                          "number of CPUs in system - 1".

                        managed_irq

                          Isolate from being targeted by managed interrupts
                          which have an interrupt mask containing isolated
                          CPUs. The affinity of managed interrupts is
                          handled by the kernel and cannot be changed via
                          the /proc/irq/* interfaces.

                          This isolation is best effort and only effective
                          if the automatically assigned interrupt mask of a
                          device queue contains isolated and housekeeping
                          CPUs. If housekeeping CPUs are online then such
                          interrupts are directed to the housekeeping CPU
                          so that IO submitted on the housekeeping CPU
                          cannot disturb the isolated CPU.

                          If a queue's affinity mask contains only isolated
                          CPUs then this parameter has no effect on the
                          interrupt routing decision, though interrupts are
                          only delivered when tasks running on those
                          isolated CPUs submit IO. IO submitted on
                          housekeeping CPUs has no influence on those
                          queues.
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
