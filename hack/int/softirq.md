## softirq

## TODO
- [ ] /proc/stat 关于 softirq 的统计是什么 ？

## Notes
![loading](https://img2020.cnblogs.com/blog/1771657/202006/1771657-20200614143354812-1093740244.png)

softirq_action 可能是紧跟着 hardirq 执行的，也可能是在 softirqd 中间执行的。


```c
static __init int spawn_ksoftirqd(void)
{
	cpuhp_setup_state_nocalls(CPUHP_SOFTIRQ_DEAD, "softirq:dead", NULL,
				  takeover_tasklets);
	BUG_ON(smpboot_register_percpu_thread(&softirq_threads));

	return 0;
}
```
一个小证据，从 nvme 到 softirq : 在 queue_request_irq 注册 irq handler 为 nvme_irq

- nvme_irq
  - nvme_process_cq
    - nvme_handle_cqe
      - nvme_try_complete_req
        - blk_mq_complete_request_remote
          - blk_mq_raise_softirq

- raise_softirq_irqoff : 比 raise_softirq 的用户更多，比如网络, 其注释也印证了想法，那就是 softirq 可以直接在上下文中间执行，也可以在 ksoftirq 中间执行


```c
/*
 * This function must run with irqs disabled!
 */
inline void raise_softirq_irqoff(unsigned int nr)
{
	__raise_softirq_irqoff(nr);

	/*
	 * If we're in an interrupt or softirq, we're done
	 * (this also catches softirq-disabled code). We will
	 * actually run the softirq once we return from
	 * the irq or softirq.
	 *
	 * Otherwise we wake up ksoftirqd to make sure we
	 * schedule the softirq soon.
	 */
	if (!in_interrupt())
		wakeup_softirqd();
}
```

- raise_softirq 对于 in_interrupt 的判断还隐藏一个重要的内容 : 如果一个代码被 spin_lock_bh 保护，那么在代码中间，可以发生 hardirq，但是无法进一步的发生 softirq 操作，而 spin_unlock_bh 会调用 do_softirq
  - raise_softirq 自带屏蔽 preempt 功能，对于 softirqd 显然也是无法切入进来的
  - 所以，无论是，hardirq 携带的 softirq，还是 ksoftirqd 携带的 softirq 都是无法进入的

- 到底 softirq 和 hardirq 放到一起执行的，还是 softirq 在 ksoftirqd 中间执行:
  - 在 invoke_softirq 中间对于内核参数 force_irqthreads 进行判断，如果是，那么所有的 softirq 都是在 ksoftirqd 中间执行的
  - 似乎存在一些 softirq 无法立刻被执行(防止 starve 其他的代码), 这些可能被之后 wakeup_softirqd 的时候执行

> Pending softirqs are checked for and executed in the following places:
> - In the return from hardware interrupt code path
> - In the ksoftirqd kernel thread
> - In any code that explicitly checks for and executes pending softirqs, such as the networking subsystem
>
> LKD chapter 8

- Most important, work queues are schedulable and can therefore sleep
    - softirq 和 tasklet 都是不能 sleep
      - 虽然在 ksoftirqd 中间是可以睡眠的，但是无法保证所有的 softirq_action 都是在其中执行的
