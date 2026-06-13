# trigger
不同的中断会走不同的 idt 入口，但是那些常规中断最后到达 `common_interrupt`, 在 idt 不同入口体现在其调用 `common_interrupt` 的参数 vector 不同。

- `common_interrupt` : 查看 `DEFINE_IDTENTRY_IRQ` 的定义，`common_interrupt` 接受两个参数 `struct pt_regs *regs, u32 vector`
  - 从 `percpu irq_desc` 数组也就是 `vector_irq` 中找到获取 `irq_desc`
  - `handle_irq`
    - `generic_handle_irq_desc` : 调用 `irq_desc::handle_irq` 来选择 edge 还是 level 的处理
      - `handle_edge_irq`
        - `handle_irq_event`
          - `handle_irq_event_percpu`
            - `__handle_irq_event_percpu`
              - `for_each_action_of_desc(desc, action)`
              - `action->handler(irq, action->dev_id)`

`irq_desc` 同时存在 `handle_irq` 和 action，前者来注册 `handle_edge_irq` ，后者注册 `nvme_irq`
在 Professional Linux Kerne Architecture 的 14.1.5 Interrupt Flow Handling 的分析是很有道理的，通过 `irq_desc::handle_irq` 来处理 flow 的，
通过 `irq_desc::action` 实现具体 irq 需要执行的动作。

```c
struct irq_desc {
  irq_flow_handler_t  handle_irq;
  struct irqaction  *action;  /* IRQ action list */
}
```

至于 level 和 edge 的关系，可以参考 [stackoverflow](https://stackoverflow.com/questions/7005331/difference-between-io-apic-fasteoi-and-io-apic-edge) 和 [IBM](https://www.ibm.com/docs/en/aix/7.2?topic=interrupts-interrupt-trigger) 的解释。

在 x86 中，使用 irq_desc::handle_irq 可以注册为:
1. handle_edge_irq
2. handle_fasteoi_irq
3. handle_level_irq

handle_level_irq 只有在 setup_default_timer_irq 中注册 timer_interrupt 是时候使用，使用这个 hook 主要是处理 legacy 的设备的，处理 level flow 的情况更多是
handle_fasteoi_irq
```txt
#0  timer_interrupt (irq=0, dev_id=0x0 <fixed_percpu_data>) at arch/x86/kernel/time.c:57
#1  0xffffffff81132995 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100051800, flags=flags@entry=0xffffc90000003f84) at kernel/irq/handle.c:156
#2  0xffffffff81132acc in handle_irq_event_percpu (desc=desc@entry=0xffff888100051800) at kernel/irq/handle.c:196
#3  0xffffffff81132b33 in handle_irq_event (desc=desc@entry=0xffff888100051800) at kernel/irq/handle.c:213
#4  0xffffffff8113686f in handle_level_irq (desc=0xffff888100051800) at kernel/irq/chip.c:650
#5  0xffffffff81098419 in generic_handle_irq_desc (desc=0xffff888100051800) at ./include/linux/irqdesc.h:158
#6  handle_irq (regs=<optimized out>, desc=0xffff888100051800) at arch/x86/kernel/irq.c:231
#7  __common_interrupt (regs=<optimized out>, vector=48) at arch/x86/kernel/irq.c:250
#8  0xffffffff81c366ee in common_interrupt (regs=0xffffffff82603dc8, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
但是随着系统启动，很快就切换为 lapic 来提供时钟中断，其 flow 类型为 interrupt
```txt
/*
#0  task_tick_fair (rq=0xffff8881b9c29700, curr=0xffff888100208000, queued=0) at kernel/sched/fair.c:10992
#1  0xffffffff8110d5d8 in scheduler_tick () at kernel/sched/core.c:4954
#2  0xffffffff81152cfb in update_process_times (user_tick=0) at kernel/time/timer.c:1801
#3  0xffffffff81161752 in tick_periodic (cpu=cpu@entry=0) at ./arch/x86/include/asm/ptrace.h:136
#4  0xffffffff811617bb in tick_handle_periodic (dev=0xffff8881b9c16f80) at kernel/time/tick-common.c:112
#5  0xffffffff810bc0f7 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1089
#6  __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1106
#7  0xffffffff81c3810d in sysvec_apic_timer_interrupt (regs=0xffffc90000013c98) at arch/x86/kernel/apic/apic.c:1100
```
因为 pci interrupt line 是共享的，而只有 level 类型才可以用于共享，可以看到 e1000 的就是经过 handle_fasteoi_irq 的。
```txt
/*
#0  e1000_intr (irq=11, data=0xffff888100232000) at drivers/net/ethernet/intel/e1000/e1000_main.c:3749
#1  0xffffffff81132995 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100100e00, flags=flags@entry=0xffffc90000003f7c) at kernel/irq/handle.c:156
#2  0xffffffff81132acc in handle_irq_event_percpu (desc=desc@entry=0xffff888100100e00) at kernel/irq/handle.c:196
#3  0xffffffff81132b33 in handle_irq_event (desc=desc@entry=0xffff888100100e00) at kernel/irq/handle.c:213
#4  0xffffffff81136751 in handle_fasteoi_irq (desc=0xffff888100100e00) at kernel/irq/chip.c:714
#5  0xffffffff81098419 in generic_handle_irq_desc (desc=0xffff888100100e00) at ./include/linux/irqdesc.h:158
#6  handle_irq (regs=<optimized out>, desc=0xffff888100100e00) at arch/x86/kernel/irq.c:231
#7  __common_interrupt (regs=<optimized out>, vector=40) at arch/x86/kernel/irq.c:250
#8  0xffffffff81c3670e in common_interrupt (regs=0xffffc900008b3b98, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
下面分析一下，通过 elcr[^12] 是如何操控 pic 的中断类型的

首先，提供在 io 空间中注册 elcr 的两个端口

- pic_common_realize : 将 PICCommonState::elcr_io 这个地址空间注册到 isa 上去
  - `isa_register_ioport(isa, &s->elcr_io, s->elcr_addr);`
- pic_realize : 注册 handler, 作用就是修改 PICCommonState::elcr 的数值，最后的作用体现在 pic_set_irq 上的
  - `memory_region_init_io(&s->elcr_io, OBJECT(s), &pic_elcr_ioport_ops, s, "elcr", 1);`

```txt
address-space: I/O
  0000000000000000-000000000000ffff (prio 0, i/o): io
    ...
    00000000000004d0-00000000000004d0 (prio 0, i/o): elcr
    00000000000004d1-00000000000004d1 (prio 0, i/o): elcr
```
然后在 seabios 中的 piix_isa_bridge_setup 会调用 pic_elcr_ioport_ops 更新 PICCommonState::elcr
```c
struct PICCommonState {
    uint8_t elcr; /* PIIX edge/trigger selection*/
}
```

在 `pic_set_irq` 中可以看到 PICCommonState::elcr 如何影响中断的
```c
static void pic_set_irq(void *opaque, int irq, int level)
{
    // ...
    if (s->elcr & mask) {
        /* level triggered */
        if (level) {
            s->irr |= mask;
            s->last_irr |= mask;
        } else {
            s->irr &= ~mask;
            s->last_irr &= ~mask;
        }
    } else {
        /* edge triggered */
        if (level) {
            if ((s->last_irr & mask) == 0) {
                s->irr |= mask;
            }
            s->last_irr |= mask;
        } else {
            s->last_irr &= ~mask;
        }
    }
    pic_update_irq(s);
}
```

## references
[^11]: https://cloud.tencent.com/developer/article/1087271
[^12]: https://en.wikipedia.org/wiki/Intel_8259

## 原来 nmi 也会讲这个东西

```c
static int nmi_handle(unsigned int type, struct pt_regs *regs)
{
	struct nmi_desc *desc = nmi_to_desc(type);
	struct nmiaction *a;
	int handled=0;

	rcu_read_lock();

	/*
	 * NMIs are edge-triggered, which means if you have enough
	 * of them concurrently, you can lose some because only one
	 * can be latched at any given time.  Walk the whole list
	 * to handle those situations.
	 */
	list_for_each_entry_rcu(a, &desc->head, list) {
		int thishandled;
		u64 delta;

		delta = sched_clock();
		thishandled = a->handler(type, regs);
		handled += thishandled;
		delta = sched_clock() - delta;
		trace_nmi_handler(a->handler, (int)delta, thishandled);

		nmi_check_duration(a, delta);
	}

	rcu_read_unlock();

	/* return total number of NMI events handled */
	return handled;
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
