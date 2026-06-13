# tasklet

总体上，就是 在 softirq 上的封装

### [Linux 中断子系统: softirq 和 tasklet](https://www.cnblogs.com/LoyenWang/p/13124803.html)

> 从上文中分析可以看出，tasklet 是软中断的一种类型，那么两者有啥区别呢？先说结论吧：
>
> - 软中断类型内核中都是静态分配，不支持动态分配，而 tasklet 支持动态和静态分配，也就是驱动程序中能比较方便的进行扩展；
> - 软中断可以在多个 CPU 上并行运行，因此需要考虑可重入问题，而 tasklet 会绑定在某个 CPU 上运行，运行完后再解绑，不要求重入问题，当然它的性能也就会下降一些；

(绑定 CPU 确认一下)

参考 lkd chapter 8 ，不可重入指的是:
Two different tasklets can run concurrently on different processors,
but two of the same type of tasklet cannot run simultaneously.

### [The end of tasklets](https://lwn.net/Articles/960041/)
https://lkml.org/lkml/2024/1/29/1604

### 简单分析下代码

一共使用了两个入口:

1. TASKLET_SOFTIRQ
2. HI_SOFTIRQ

## tasklet 的使用很常见的

```txt
@[
    apple_dart_map_pages+0
    iommu_map+80
    __iommu_dma_map+164
    iommu_dma_map_page+336
    dma_map_page_attrs+84
    usb_hcd_map_urb_for_dma+832
    xhci_map_urb_for_dma+368
    usb_hcd_submit_urb+156
    usb_submit_urb+452
    rx_submit+328
    rx_complete+736
    __usb_hcd_giveback_urb+164
    usb_giveback_urb_bh+208
    tasklet_action_common.isra.0+236
    tasklet_action+48
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+260
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    secondary_start_kernel+224
    __secondary_switched+184
]: 12214
```

这个是在 13900k 的环境中抓到的，所以 tasklet 没有那么不堪，还是
```txt
- 3.72% __irq_exit_rcu
   - 3.70% handle_softirqs
      - 3.59% tasklet_hi_action
         - 3.57% bh_worker
            - 3.47% process_one_work
               - 3.42% usb_giveback_urb_bh
                  - 3.40% __usb_hcd_giveback_urb
                     - 3.21% btusb_intr_complete
                        - 1.64% btusb_recv_intr
                           - 0.58% queue_delayed_work_on
                              - 0.57% __queue_work
                                 - 0.52% kick_pool
                                      try_to_wake_up
                           - 0.57% btintel_recv_event
                              - hci_recv_frame
                                 - 0.53% queue_work_on
                                      0.53% __queue_work
                        - 1.49% usb_hcd_submit_urb
                           - 1.40% xhci_urb_enqueue
                              - 1.31% xhci_queue_bulk_tx
                                   1.21% xhci_ring_ep_doorbell
```

此外，测试两个物理机的 iperf3 ，使用 bcc 工具 softirqs 可以观察到：

```txt
SOFTIRQ          TOTAL_usecs
rcu                      250
timer                    478
sched                   1276
hi                      1579
tasklet                22394
net_tx                118328
net_rx                129335
```

## 经典例子

```c
void __init tcp_tasklet_init(void)
{
	int i;

	for_each_possible_cpu(i) {
		struct tsq_tasklet *tsq = &per_cpu(tsq_tasklet, i);

		INIT_LIST_HEAD(&tsq->head);
		tasklet_setup(&tsq->tasklet, tcp_tasklet_func);
	}
}
```

触发过程:
```txt
@[
        tcp_wfree+0       // 在其中调用 tasklet_schedule
        xmit_one.constprop.0+124
        dev_hard_start_xmit+104
        __dev_queue_xmit+412
        neigh_hh_output+148
        ip_finish_output2+940
        __ip_finish_output+172
        ip_finish_output+60
        ip_output+112
        __ip_queue_xmit+364
        ip_queue_xmit+28
        __tcp_transmit_skb+968
        tcp_write_xmit+836
        __tcp_push_pending_frames+68
        tcp_push+184
        tcp_sendmsg_locked+2332
        tcp_sendmsg+64
        inet_sendmsg+76
        __sock_sendmsg+100
        __sys_sendto+240
        __arm64_sys_sendto+48
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 389
```

处理过程:

```txt
@[
        tcp_tasklet_func+0
        tasklet_action+56
        handle_softirqs+300
        __do_softirq+28
        ____do_softirq+24
        call_on_irq_stack+36
        do_softirq_own_stack+36
        __irq_exit_rcu+316
        irq_exit_rcu+24
        el1_interrupt+72
        el1h_64_irq_handler+24
        el1h_64_irq+128
        default_idle_call+56
        cpuidle_idle_call+380
        do_idle+244
        cpu_startup_entry+64
        rest_init+196
        start_kernel+1104
        __primary_switched+136
]: 7
```
就是中断处理完之后，然后继续


## 看看到底是如何替换为
```c
struct workqueue_struct *system_bh_wq;
EXPORT_SYMBOL_GPL(system_bh_wq);
struct workqueue_struct *system_bh_highpri_wq;
EXPORT_SYMBOL_GPL(system_bh_highpri_wq);
```

哦，原来是要去彻底移除掉 tasklet 的:
```c
/*
 * TODO: Convert all tasklet users to workqueue and use softirq directly.
 *
 * This is currently called from tasklet[_hi]action() and thus is also called
 * whenever there are tasklets to run. Let's do an early exit if there's nothing
 * queued. Once conversion from tasklet is complete, the need_more_worker() test
 * can be dropped.
 *
 * After full conversion, we'll add worker->softirq_action, directly use the
 * softirq action and obtain the worker pointer from the softirq_action pointer.
 */
void workqueue_softirq_action(bool highpri)
{
	struct worker_pool *pool =
		&per_cpu(bh_worker_pools, smp_processor_id())[highpri];
	if (need_more_worker(pool))
		bh_worker(list_first_entry(&pool->workers, struct worker, node));
}
```

现在的调用模式为:

```txt
@[
        workqueue_softirq_action+0
        handle_softirqs+300
        __do_softirq+28
        ____do_softirq+24
        call_on_irq_stack+36
        do_softirq_own_stack+36
        __irq_exit_rcu+316
        irq_exit_rcu+24
        el1_interrupt+72
        el1h_64_irq_handler+24
        el1h_64_irq+128
        selinux_inode_permission+16
        inode_permission+116
        link_path_walk.part.0.constprop.0+180
        path_openat+132
        do_filp_open+152
        do_sys_openat2+140
        __arm64_sys_openat+108
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 2
```

还是有点不懂的，之前 tasklet 也是在 softirq 上下文中执行的啊

## tasklet
<!-- ee812adc-8cdc-45e1-9251-5e8eb6c1cfbf -->

如何理解这个 tasklet_schedule ?
```c
static void kbd_event(struct input_handle *handle, unsigned int event_type,
		      unsigned int event_code, int value)
{
	/* We are called with interrupts disabled, just take the lock */
	scoped_guard(spinlock, &kbd_event_lock) {
		if (event_type == EV_MSC && event_code == MSC_RAW &&
				kbd_is_hw_raw(handle->dev))
			kbd_rawcode(value);
		if (event_type == EV_KEY && event_code <= KEY_MAX)
			kbd_keycode(event_code, value, kbd_is_hw_raw(handle->dev));
	}

	tasklet_schedule(&keyboard_tasklet);
	do_poke_blanked_console = 1;
	schedule_console_callback();
}
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
