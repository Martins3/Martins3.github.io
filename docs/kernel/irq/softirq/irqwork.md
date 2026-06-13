# irq work
## 原来 irq work 的调用来自于这里

 当 isolcpus=$pcpu_list rcu_nocbs=$pcpu_list nohz_full=$pcpu_list 的时候似乎会导致这个:

```txt
  arch_irq_work_raise
  irq_work_queue
  tick_nohz_dep_set_cpu
  enqueue_task_fair
  ttwu_do_activate
  try_to_wake_up
  hrtimer_wakeup
  __hrtimer_run_queues
  hrtimer_interrupt
  smp_apic_timer_interrupt
  apic_timer_interrupt
    94446
```

## 没太理解有啥作用

如果理解了，需要回答这个问题 : https://stackoverflow.com/questions/71618940/why-does-schedutil-use-irq-work-queue-instead-of-directly-calling-kthread-queue

## 需要理解下 CONFIG_PREEMPT_RT 才可以理解 irq work
```c
static __init int irq_work_init_threads(void)
{
	if (IS_ENABLED(CONFIG_PREEMPT_RT))
		BUG_ON(smpboot_register_percpu_thread(&irqwork_threads));
	return 0;
}
```

## tsc_refine_calibration_work 就是采用 irq work 的，似乎是为了保证，在指定时间，CPU 必须响应

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
