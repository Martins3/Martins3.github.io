## blk_mq_raise_softirq 中操作 percpu 变量需要 preempt_disable

这里简单看上去这个函数是中断上下文中，但是实际上这个函数不一定只是在中断中执行，
例如 virtblk_poll ，所以还是需要:

```c
static void blk_mq_raise_softirq(struct request *rq)
{
	struct llist_head *list;

	preempt_disable();
	list = this_cpu_ptr(&blk_cpu_done);
	if (llist_add(&rq->ipi_list, list))
		raise_softirq(BLOCK_SOFTIRQ);
	preempt_enable();
}
```

```txt
@[
        blk_mq_complete_request_remote+0
        nvme_irq+60
        __handle_irq_event_percpu+84
        handle_irq_event+76
        handle_fasteoi_irq+180
        handle_irq_desc+60
        generic_handle_domain_irq+36
        __gic_handle_irq_from_irqson.isra.0+340
        gic_handle_irq+40
        call_on_irq_stack+36
        do_interrupt_handler+136
        el1_interrupt+68
        el1h_64_irq_handler+24
        el1h_64_irq+128
        finish_task_switch.isra.0+128
        __schedule+720
        schedule_idle+48
        do_idle+196
        cpu_startup_entry+64
        secondary_start_kernel+224
        __secondary_switched+192
]: 84
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
