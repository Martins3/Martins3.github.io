# 中断
<!-- vim-markdown-toc GitLab -->

- [TODO](#todo)
- [unsorted](#unsorted)
- [workqueue](#workqueue)
    - [struct work_struct](#struct-work_struct)
    - [struct workqueue](#struct-workqueue)
    - [struct worker](#struct-worker)
    - [struct worker_pool](#struct-worker_pool)
    - [struct pool_workqueue](#struct-pool_workqueue)
- [ipi](#ipi)
- [https://elinux.org/images/8/8c/Zyngier.pdf](#httpselinuxorgimages88czyngierpdf)
- [https://linux-kernel-labs.github.io/refs/heads/master/lectures/interrupts.html](#httpslinux-kernel-labsgithubiorefsheadsmasterlecturesinterruptshtml)
- [timer](#timer)
- [irq](#irq)
- [softirq](#softirq)
- [tasklet](#tasklet)
- [apic](#apic)
- [chained irq](#chained-irq)

<!-- vim-markdown-toc -->

不要害怕开始：
1. 总结 从 ics 的中断 和 ucore 的中断的实现，然后再去分析


这篇文章
A Hardware Architecture for Implementing Protection Rings
让我怀疑人生:
1. 这里描述的 gates 和 syscall 是什么关系 ?
2. syscall 可以使用 int 模拟实现吗 ?
3. interupt 和 exception 在架构实现上存在什么区别吗 ?

## TODO


- [ ] https://www.kernel.org/doc/html/latest/core-api/genericirq.html

- [ ] --------------- unsorted
1. https://stackoverflow.com/questions/1053572/why-kernel-code-thread-executing-in-interrupt-context-cannot-sleep
2. spin lock 为什么需要提供 spin_lock_irqsave : 因为 spin_lock 不可以 recursive 的，一个 process 持有 lock, 然后 interrupt handler 被执行，持有这个锁，那么就进入死锁了

[^4] 的质量很好，记录放在 insides 中间:

ideentry.h : 定义了一些常见的 idt, 比如 GP(general protection), PF(page fault) 的问题

<<<<<<< HEAD
http://www.cs.columbia.edu/~krj/os/lectures/L07-LinuxEvents.pdf

[^5] 的质量很高，虽然是个 ppt:
- I/O	devices	hae	(unique	or	shared)	Interrupt	Request Lines	(IRQs)	
- IRQs	are	mapped	by	special	hardware	to	interrupt	vectors, and	passed	to	the	CPU	
- This	hardware	is	called	a	Programmable	Interrupt Controller	(PIC)	v

[^4]: https://0xax.gitbooks.io/linux-insides/content/Interrupts/
[^5]: http://www.cs.columbia.edu/~krj/os/lectures/L07-LinuxEvents.pdf

为什么设备是复杂的 ?
1. 利用构建 vfs 提供访问设备的接口，同时block 设备又是 fs 的载体
2. module 系统
3. 中断 系统
- [ ] so, it's time unravel the mess relationships between them ?


- [answer this question](https://unix.stackexchange.com/questions/491437/how-does-linux-kernel-switches-from-kernel-stack-to-interrupt-stack?rq=1)


- [ ] http://wiki.0xffffff.org/posts/hurlex-8.html : used for understand 8259APIC

## unsorted
- [ ][How does the Linux kernel handle shared IRQs?](https://unix.stackexchange.com/questions/47306/how-does-the-linux-kernel-handle-shared-irqs)
  - [ ] multiple device driver for one interrupt line ?

- [ ] --------------- unsorted

## workqueue
- [ ] wowotech 中间的东西可以看看:
- [ ] https://zhuanlan.zhihu.com/p/91106844

With this article,[LoyenWang](https://www.cnblogs.com/LoyenWang/p/13185451.html), I feel like that I understand how workqueue works, some questions
  - [ ] workqueue attr

  

#### struct work_struct
`struct work_struct`用来描述`work`，初始化一个`work`并添加到工作队列后，将会将其传递到合适的内核线程来进行处理，它是用于调度的最小单位。

```c
struct work_struct {
	atomic_long_t data;     //低比特存放状态位，高比特存放worker_pool的ID或者pool_workqueue的指针
	struct list_head entry; //用于添加到其他队列上
	work_func_t func;       //工作任务的处理函数，在内核线程中回调
#ifdef CONFIG_LOCKDEP
	struct lockdep_map lockdep_map;
#endif
};
```

![](https://img2020.cnblogs.com/blog/1771657/202006/1771657-20200623234322425-1856230121.png)

check code get_work_pool(), graph above is kind of misleading.

#### struct workqueue
- [ ] why we need `struct workqueue`, I think `struct worker_pool` is enough.

#### struct worker

- [x] worker can be create and destroied dynamically.

#### struct worker_pool


#### struct pool_workqueue
`pool_workqueue`充当纽带的作用，用于将`workqueue`和`worker_pool`关联起来；

```c
struct pool_workqueue {
	struct worker_pool	*pool;		/* I: the associated pool */    //指向worker_pool
	struct workqueue_struct *wq;		/* I: the owning workqueue */   //指向所属的workqueue

	int			nr_active;	/* L: nr of active works */     //活跃的work数量
	int			max_active;	/* L: max active works */   //活跃的最大work数量
	struct list_head	delayed_works;	/* L: delayed works */      //延迟执行的work挂入本链表
	struct list_head	pwqs_node;	/* WR: node on wq->pwqs */      //用于添加到workqueue链表中
	struct list_head	mayday_node;	/* MD: node on wq->maydays */   //用于添加到workqueue链表中
    ...
} __aligned(1 << WORK_STRUCT_FLAG_BITS);
```

- [ ] I think `struct pool_workqueue` is child of `struct worker_pool` and `struct workqueue_struct`


## ipi
https://stackoverflow.com/questions/62068750/kinds-of-ipi-for-x86-architecture-in-linux


## https://elinux.org/images/8/8c/Zyngier.pdf

Chained interrupt controllers : 参考ldd 以及 https://stackoverflow.com/questions/34377846/what-is-chained-irq-in-linux-when-are-they-need-to-used

Generic MSIs : https://en.wikipedia.org/wiki/Message_Signaled_Interrupts

Most systems have tens, hundreds of interrupt signal, an interrupt controller allows them to be multiplexed.
> device need attension : cpu check some pin after finished one instruction
> exception : this instruction worked abnormally
> int n : go to idt , execute  number n slot : 如何这样，似乎只有int 0x80 有意义啊!

> 仲裁器 还是　multiplexed , 如何体现的 multiplexed 的形式的 ?

IRQ line 到底是什么?


## https://linux-kernel-labs.github.io/refs/heads/master/lectures/interrupts.html#
- https://www.ibm.com/developerworks/cn/linux/l-cn-linuxkernelint/index.html
- https://0xax.gitbooks.io/linux-insides/content/Interrupts/linux-interrupts-7.html
- https://zhuanlan.zhihu.com/p/83709066

## timer


## irq

```c
struct irq_chip {
	struct device	*parent_device;     //指向父设备
	const char	*name;      //  /proc/interrupts中显示的名字
	unsigned int	(*irq_startup)(struct irq_data *data);  //启动中断，如果设置成NULL，则默认为enable
	void		(*irq_shutdown)(struct irq_data *data);     //关闭中断，如果设置成NULL，则默认为disable
	void		(*irq_enable)(struct irq_data *data);   //中断使能，如果设置成NULL，则默认为chip->unmask
	void		(*irq_disable)(struct irq_data *data);  //中断禁止

	void		(*irq_ack)(struct irq_data *data);  //开始新的中断
	void		(*irq_mask)(struct irq_data *data); //中断源屏蔽
	void		(*irq_mask_ack)(struct irq_data *data); //应答并屏蔽中断
	void		(*irq_unmask)(struct irq_data *data);   //解除中断屏蔽
	void		(*irq_eoi)(struct irq_data *data);  //中断处理结束后调用

	int		(*irq_set_affinity)(struct irq_data *data, const struct cpumask *dest, bool force); //在SMP中设置CPU亲和力
	int		(*irq_retrigger)(struct irq_data *data);    //重新发送中断到CPU
	int		(*irq_set_type)(struct irq_data *data, unsigned int flow_type); //设置中断触发类型
	int		(*irq_set_wake)(struct irq_data *data, unsigned int on);    //使能/禁止电源管理中的唤醒功能

	void		(*irq_bus_lock)(struct irq_data *data); //慢速芯片总线上的锁
	void		(*irq_bus_sync_unlock)(struct irq_data *data);  //同步释放慢速总线芯片的锁

	void		(*irq_cpu_online)(struct irq_data *data);
	void		(*irq_cpu_offline)(struct irq_data *data);

	void		(*irq_suspend)(struct irq_data *data);
	void		(*irq_resume)(struct irq_data *data);
	void		(*irq_pm_shutdown)(struct irq_data *data);

	void		(*irq_calc_mask)(struct irq_data *data);

	void		(*irq_print_chip)(struct irq_data *data, struct seq_file *p);
	int		(*irq_request_resources)(struct irq_data *data);
	void		(*irq_release_resources)(struct irq_data *data);

	void		(*irq_compose_msi_msg)(struct irq_data *data, struct msi_msg *msg);
	void		(*irq_write_msi_msg)(struct irq_data *data, struct msi_msg *msg);

	int		(*irq_get_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool *state);
	int		(*irq_set_irqchip_state)(struct irq_data *data, enum irqchip_irq_state which, bool state);

	int		(*irq_set_vcpu_affinity)(struct irq_data *data, void *vcpu_info);

	void		(*ipi_send_single)(struct irq_data *data, unsigned int cpu);
	void		(*ipi_send_mask)(struct irq_data *data, const struct cpumask *dest);

	unsigned long	flags;
};

struct irq_domain {
	struct list_head link;  //用于添加到全局链表irq_domain_list中
	const char *name;   //IRQ domain的名字
	const struct irq_domain_ops *ops;   //IRQ domain映射操作函数集
	void *host_data;    //在GIC驱动中，指向了irq_gic_data
	unsigned int flags; 
	unsigned int mapcount;  //映射中断的个数

	/* Optional data */
	struct fwnode_handle *fwnode;
	enum irq_domain_bus_token bus_token;
	struct irq_domain_chip_generic *gc;
#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	struct irq_domain *parent;  //支持级联的话，指向父设备
#endif
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
	struct dentry		*debugfs_file;
#endif

	/* reverse map data. The linear map gets appended to the irq_domain */
	irq_hw_number_t hwirq_max;  //IRQ domain支持中断数量的最大值
	unsigned int revmap_direct_max_irq;
	unsigned int revmap_size;   //线性映射的大小
	struct radix_tree_root revmap_tree; //Radix Tree映射的根节点
	unsigned int linear_revmap[];   //线性映射用到的查找表
};

struct irq_domain_ops {
	int (*match)(struct irq_domain *d, struct device_node *node,
		     enum irq_domain_bus_token bus_token);      // 用于中断控制器设备与IRQ domain的匹配
	int (*select)(struct irq_domain *d, struct irq_fwspec *fwspec,
		      enum irq_domain_bus_token bus_token);
	int (*map)(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw);    //用于硬件中断号与Linux中断号的映射
	void (*unmap)(struct irq_domain *d, unsigned int virq);
	int (*xlate)(struct irq_domain *d, struct device_node *node,
		     const u32 *intspec, unsigned int intsize,
		     unsigned long *out_hwirq, unsigned int *out_type);     //通过device_node，解析硬件中断号和触发方式

#ifdef	CONFIG_IRQ_DOMAIN_HIERARCHY
	/* extended V2 interfaces to support hierarchy irq_domains */
	int (*alloc)(struct irq_domain *d, unsigned int virq,
		     unsigned int nr_irqs, void *arg);
	void (*free)(struct irq_domain *d, unsigned int virq,
		     unsigned int nr_irqs);
	void (*activate)(struct irq_domain *d, struct irq_data *irq_data);
	void (*deactivate)(struct irq_domain *d, struct irq_data *irq_data);
	int (*translate)(struct irq_domain *d, struct irq_fwspec *fwspec,
			 unsigned long *out_hwirq, unsigned int *out_type);
#endif
};
```

![](https://img2020.cnblogs.com/blog/1771657/202005/1771657-20200531111554895-528341955.png)
- `struct irq_chip`结构，描述的是中断控制器的底层操作函数集，这些函数集最终完成对控制器硬件的操作；
- `struct irq_domain`结构，用于硬件中断号和Linux IRQ中断号（virq，虚拟中断号）之间的映射；


![](https://img2020.cnblogs.com/blog/1771657/202005/1771657-20200531111647851-1005315068.png)

- 每个中断控制器都对应一个IRQ Domain；
- 中断控制器驱动通过`irq_domain_add_*()`接口来创建IRQ Domain；
- IRQ Domain支持三种映射方式：linear map（线性映射），tree map（树映射），no map（不映射）；
  - linear map：维护固定大小的表，索引是硬件中断号，如果硬件中断最大数量固定，并且数值不大，可以选择线性映射；
  - tree map：硬件中断号可能很大，可以选择树映射；
  - no map：硬件中断号直接就是Linux的中断号；

![](https://img2020.cnblogs.com/blog/1771657/202005/1771657-20200531111718514-879227841.png)

![](https://img2020.cnblogs.com/blog/1771657/202005/1771657-20200531111755704-1231972965.png)


## softirq
- [ ] what's happending in kernel/softirq.c ?

![loading](https://img2020.cnblogs.com/blog/1771657/202006/1771657-20200614143354812-1093740244.png)


## tasklet
- [ ] https://lwn.net/Articles/830964/
- [ ] https://www.cnblogs.com/LoyenWang/p/13124803.html

## apic
```c
// global apic variable
struct apic *apic __ro_after_init = &apic_flat;
```

## chained irq
[IRQs: the Hard, the Soft, the Threaded and the Preemptible](https://elinux.org/images/8/8c/Zyngier.pdf)

> An interrupt controller allows them to be multiplexed

我想知道 controller 如何实现 multiplexed 的，或者 multiplexed 到底指什么东西 ? interrupt 队列 ?

> Offers specific facilities
> - Masking/unmasking individual interrupts
> - Setting priorities
> - SMP affinity
> - Exotic things like wake-up interrupts

> Interrupt triggers
> - Level triggered (high or low)
>   - Indicates a persistent condition
>   - An action has to be performed on the device to clear the interrupt
> - Edge triggered (rising or falling)
>   - Indicates an event
>   - May have happened once or more...
> - Some systems do not expose the trigger type to software
>   - Either the interrupt is abstracted (virtualization)
>   - Or this is more an exception than an interrupt...

> How does Linux deal with interrupts
> - `struct irq_chip`
>     - A set of methods describing how to drive the interrupt controller
>     - Directly called by core IRQ code
> - `struct irqdomain`
>     - A pointer to the firmware node for a given interrupt controller (`fwnode`)
>     - A method to convert a firmware description of an IRQ into an ID local to this interrupt controller (`hwirq`)
>     - A way to retrieve the Linux view of an IRQ from the `hwirq`
> - `struct irq_desc`
>     - Linux’s view of an interrupt
>     - Contains all the core stuff
>     - 1:1 mapping to the Linux interrupt number
> - `struct irq_data`
>     - Contains the data that is relevant to the `irq_chip` managing this interrupt
>     - Both the Linux IRQ number and the hwirq
>     - A pointer to the `irq_chip`
>     - Embedded in `irq_desc` (for now)

> - CPU gets an interrupt
> - Find out the `hwirq` from the interrupt controller
>    - Usually involves reading some HW register
> - Look-up the `irq_desc` into the `irqdomain` using the `hwirq`
>    - Actually returns an IRQ number, which is equivalent to the `irq_desc`
> - The core kernel then handles the interrupt
> 
> ![](../../../img/misc/irqdomain.png)

那么，hwirq 在 irqdomain 中间被翻译为 irq_desc ，IRQ number 在 kernel 看来等价于 irq_desc 


> - Not enough interrupts lines?
>   - Dedicate a single line for a secondary interrupt controller
>   - And add more stuff to it!
> - Requires two level handling
>   - First handle the interrupt on the primary interrupt controller
>   - Then at the secondary one to find out which device has caused the interrupt
>   - See `irq_set_chained_handler_and_data`, `chained_irq_enter`, `chained_irq_exit`
>   - `Never` treat this as a normal interrupt handler
> - Used in each and every x86 system
>   - The infamous i8259 cascade
> - You can also share a single interrupt between devices
>   - And that really stinks. Please avoid doing it if possible.



```c
/*
 * Entry/exit functions for chained handlers where the primary IRQ chip
 * may implement either fasteoi or level-trigger flow control.
 */
static inline void chained_irq_enter(struct irq_chip *chip,
				     struct irq_desc *desc)
{
	/* FastEOI controllers require no action on entry. */
	if (chip->irq_eoi)
		return;

	if (chip->irq_mask_ack) {
		chip->irq_mask_ack(&desc->irq_data);
	} else {
		chip->irq_mask(&desc->irq_data);
		if (chip->irq_ack)
			chip->irq_ack(&desc->irq_data);
	}
}

static inline void chained_irq_exit(struct irq_chip *chip,
				    struct irq_desc *desc)
{
	if (chip->irq_eoi)
		chip->irq_eoi(&desc->irq_data);
	else
		chip->irq_unmask(&desc->irq_data);
}
```


> - Each interrupt controller has its own `irqdomain`
> - The kernel deals with two interrupts
>   - and two interrupt handlers
>   - the first one being a chained handler
>   - *convention is to stash a pointer to the secondary domain inside the top-level `irq_desc`*
> - We walk the interrupt chain in reverse order
> - Once we reach the last level irq_desc, we can process the actual interrupt handler
> ![](../../../img/misc/irqdomain2.png)

interrupt controller 和 irqdomain 一一对应的

top-level irq_desc 中间哪里 TMD 有 stash a pointer，只有 action chain 吧 ? 应该是 irq_domain 是含有层次架构的，
但是对于 irq_desc 和 irq_domain 如何联系起来，并不清楚 ?

> When multiplexing doesn’t fit
> - There is more than just cascading irqchips
> - Some setups have a 1:1 mapping between input and output
>   - *Interrupt routers*
>   - *Wake-up controllers*
>   - Programmable line inverters
> - Most of them are not interrupt controllers
>   - Still, they do impact the interrupt delivery
>   - We choose to represent them as `irq_chip`
> - This is a hierarchical/stacked configuration
> - *The chained irqchip paradigm doesn’t match it*

第 15 16 页是在看不懂了
