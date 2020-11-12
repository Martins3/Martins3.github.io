# 中断
<!-- vim-markdown-toc GitLab -->

  - [Context](#context)
  - [TODO](#todo)
  - [code distribution](#code-distribution)
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
  - [irqaction](#irqaction)
  - [softirq](#softirq)
  - [tasklet](#tasklet)
  - [apic](#apic)
  - [chained irq](#chained-irq)
  - [irq domain](#irq-domain)
  - [irq domain hierarchy](#irq-domain-hierarchy)
  - [ir](#ir)
  - [request irq](#request-irq)
  - [irq desc](#irq-desc)
  - [affinity](#affinity)
  - [nmi](#nmi)
  - [apic](#apic-1)
- [gpio](#gpio)
  - [idt](#idt)
  - [handle irq](#handle-irq)
  - [x86 vector](#x86-vector)
  - [isa and pci](#isa-and-pci)
  - [eoi](#eoi)
  - [ref](#ref)

<!-- vim-markdown-toc -->

## Context
- [ ] 术道经纬 : https://zhuanlan.zhihu.com/p/93289632
- [ ] fwnode 是做什么的 ?
- [ ] 如何实现 nmi 中断的

## TODO
不要害怕开始：
1. 总结 从 ics 的中断 和 ucore 的中断的实现，然后再去分析

这篇文章
A Hardware Architecture for Implementing Protection Rings
让我怀疑人生:
1. 这里描述的 gates 和 syscall 是什么关系 ?
2. syscall 可以使用 int 模拟实现吗 ?
3. interupt 和 exception 在架构实现上存在什么区别吗 ?


1. 什么是软中断 ？
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

## code distribution
arch/x86/kernel/apic
| File             | blank | comment | code | explanation |
|------------------|-------|---------|------|-------------|
| io_apic.c        | 449   | 560     | 2063 |             |
| apic.c           | 413   | 758     | 1711 |             |
| x2apic_uv_x.c    | 274   | 107     | 1193 |             |
| vector.c         | 189   | 218     | 848  |             |
| msi.c            | 75    | 79      | 361  |             |
| apic_numachip.c  | 70    | 18      | 250  |             |
| ipi.c            | 55    | 68      | 208  |             |
| x2apic_cluster.c | 44    | 9       | 173  |             |
| apic_flat_64.c   | 48    | 35      | 168  |             |
| probe_32.c       | 38    | 25      | 152  |             |
| x2apic_phys.c    | 41    | 2       | 147  |             |
| bigsmp_32.c      | 41    | 13      | 138  |             |
| apic_noop.c      | 28    | 22      | 95   |             |
| local.h          | 12    | 16      | 41   |             |
| hw_nmi.c         | 7     | 11      | 41   |             |
| probe_64.c       | 7     | 13      | 35   |             |
| apic_common.c    | 7     | 5       | 34   |             |
| Makefile         | 6     | 9       | 15   |             |

kernel/irq
| file           | blank | comment | code | explanation |
|----------------|-------|---------|------|-------------|
| manage.c       | 309   | 678     | 1271 |             |
| irqdomain.c    | 242   | 412     | 1107 |             |
| chip.c         | 217   | 390     | 843  |             |
| irqdesc.c      | 157   | 127     | 661  |             |
| generic-chip.c | 92    | 144     | 412  |             |
| proc.c         | 92    | 52      | 385  |             |
| msi.c          | 76    | 92      | 357  |             |
| internals.h    | 73    | 60      | 348  |             |
| matrix.c       | 55    | 121     | 280  |             |
| spurious.c     | 58    | 160     | 247  |             |
| debugfs.c      | 45    | 8       | 220  |             |
| ipi.c          | 45    | 112     | 182  |             |
| affinity.c     | 44    | 44      | 182  |             |
| devres.c       | 39    | 98      | 150  |             |
| settings.h     | 27    | 5       | 137  |             |
| pm.c           | 32    | 56      | 124  |             |
| timings.c      | 41    | 197     | 124  |             |
| cpuhotplug.c   | 24    | 86      | 106  |             |
| handle.c       | 35    | 86      | 101  |             |
| irq_sim.c      | 25    | 43      | 95   |             |
| autoprobe.c    | 21    | 71      | 92   |             |
| migration.c    | 18    | 45      | 56   |             |
| resend.c       | 11    | 41      | 48   |             |
| debug.h        | 8     | 5       | 36   |             |
| dummychip.c    | 7     | 21      | 36   |             |
| Makefile       | 1     | 1       | 15   |             |


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

## irqaction
```c
/**
 * struct irqaction - per interrupt action descriptor
 * @handler:	interrupt handler function
 * @name:	name of the device
 * @dev_id:	cookie to identify the device
 * @percpu_dev_id:	cookie to identify the device
 * @next:	pointer to the next irqaction for shared interrupts
 * @irq:	interrupt number
 * @flags:	flags (see IRQF_* above)
 * @thread_fn:	interrupt handler function for threaded interrupts
 * @thread:	thread pointer for threaded interrupts
 * @secondary:	pointer to secondary irqaction (force threading)
 * @thread_flags:	flags related to @thread
 * @thread_mask:	bitmask for keeping track of @thread activity
 * @dir:	pointer to the proc/irq/NN/name entry
 */
struct irqaction {
	irq_handler_t		handler;
	void			*dev_id;
	void __percpu		*percpu_dev_id;
	struct irqaction	*next;
	irq_handler_t		thread_fn;
	struct task_struct	*thread;
	struct irqaction	*secondary;
	unsigned int		irq;
	unsigned int		flags;
	unsigned long		thread_flags;
	unsigned long		thread_mask;
	const char		*name;
	struct proc_dir_entry	*dir;
} ____cacheline_internodealigned_in_smp;
```

- [ ] /proc/interrupts : proc.c:show_interrupts() 的最后的描述，其实就是相关的 chip
```
            CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7       
   0:          8          0          0          0          0          0          0          0  IR-IO-APIC    2-edge      timer
   1:      19619          0          0          0          0          0          0       3444  IR-IO-APIC    1-edge      i8042
   8:          0          1          0          0          0          0          0          0  IR-IO-APIC    8-edge      rtc0
   9:         46         68          0          0          0          0          0          0  IR-IO-APIC    9-fasteoi   acpi
  12:        384          0          0          0          0          0        143          0  IR-IO-APIC   12-edge      i8042
  14:     187427          0      28041          0          0          0          0          0  IR-IO-APIC   14-fasteoi   INT344B:00
```




## softirq
- [ ] what's happending in kernel/softirq.c ?

![loading](https://img2020.cnblogs.com/blog/1771657/202006/1771657-20200614143354812-1093740244.png)


- [ ] how kernel transfer from hardirq to softirq ?

- [ ] /proc/stat 关于 softirq 的统计是什么 ？

## tasklet
- [ ] https://lwn.net/Articles/830964/
- [ ] https://www.cnblogs.com/LoyenWang/p/13124803.html

## apic
```c
// global apic variable
struct apic *apic __ro_after_init = &apic_flat;
```
- [ ] [^2]p117 的 IO APIC 的 24 个引脚的寄存器配置


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

## irq domain
- [x] [What are linux irq domains, why are they needed?](https://stackoverflow.com/questions/34371352/what-are-linux-irq-domains-why-are-they-needed)
  - 基本的思路是，信号是逐级的传递到 CPU 中间的
  - CPU 收到中断，知道是哪一个 interrupt line , 以及注册到该 driver 的 handler
    - 如果一个 interrupt line 上注册了多个，可以依次执行一下, 直到找到该 device / driver
      - 如果恰巧 driver 是一个 irq chip, 那么该芯片可以知道是来自于哪一个引脚，并且知道注册到该引脚的 device / driver，直到找到真正的 driver

- [ ] 虽然的确是这一个道理，那么为什么需要 irq domain 的概念啊 ?
  - [ ] 是不是因为在每一个 chip 自己引脚编号 和 对应的 action 的映射建立关系

[kernel doc](https://www.kernel.org/doc/html/latest/core-api/irq/irq-domain.html)

we need a mechanism to separate controller-local interrupt numbers, called hardware irq's, from Linux IRQ numbers.



// --------------------- trace the functions -------------------------------------------

1. alloc_desc <=== alloc_descs <=== `__irq_alloc_descs` <=== irq_domain_alloc_descs <=== `__irq_domain_alloc_irqs` <==== alloc_irq_from_domain <==== mp_map_pin_to_irq <==== pin_2_irq , mp_map_gsi_to_irq <== pin_2_irq <== setup_IO_APIC_irqs <== setup_IO_APIC <== apic_bsp_setup <== apic_intr_mode_init
  - maybe this how initial irq set up

2. default_get_smp_config ==> check_physptr ==> smp_read_mpc ==> mp_register_ioapic ==> mp_irqdomain_create


- [x] apic_intr_mode_select : choose intr mode which can be determined by krenel parameter
- [ ] apic_intr_mode_init --> *default_setup_apic_routing* , apic_bsp_setup
- [x] setup_local_APIC : as name suggests, doing something like this:
- [ ] enable_IO_APIC : why we need i8259, maybe just legacy. it seems io apic has a different mmio space
- [ ] mp_pin_to_gsi
  - [ ] mpprase
  - [ ] what's gsi
  - [ ] why we have set up relation with ioapic and pin, and what's pin, it's pin of cpu ?


- [ ] mp_map_pin_to_irq 
  - if irq domain already set up, return `irq_find_mapping`
  - otherwise, `alloc_irq_from_domain` firstly

// --------------------- read the functions -------------------------------------------






## irq domain hierarchy
[kernel doc](https://www.kernel.org/doc/html/latest/core-api/irq/irq-domain.html)

There are four major interfaces to use hierarchy `irq_domain`:
- `irq_domain_alloc_irqs()`: allocate IRQ descriptors and interrupt controller related resources to deliver these interrupts.
- `irq_domain_free_irqs()`: free IRQ descriptors and interrupt controller related resources associated with these interrupts.
- `irq_domain_activate_irq()`: activate interrupt controller hardware to deliver the interrupt.
- `irq_domain_deactivate_irq()`: deactivate interrupt controller hardware to stop delivering the interrupt.


```c
static const struct irq_domain_ops x86_vector_domain_ops = {
	.alloc		= x86_vector_alloc_irqs,
	.free		= x86_vector_free_irqs,
	.activate	= x86_vector_activate,
	.deactivate	= x86_vector_deactivate,
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
	.debug_show	= x86_vector_debug_show,
#endif
};

const struct irq_domain_ops mp_ioapic_irqdomain_ops = {
	.alloc		= mp_irqdomain_alloc,
	.free		= mp_irqdomain_free,
	.activate	= mp_irqdomain_activate,
	.deactivate	= mp_irqdomain_deactivate,
};

static const struct irq_domain_ops msi_domain_ops = {
	.alloc		= msi_domain_alloc,
	.free		= msi_domain_free,
	.activate	= msi_domain_activate,
	.deactivate	= msi_domain_deactivate,
};
```

:cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat:


To support such a hardware topology and make software architecture match hardware architecture,
an `irq_domain` data structure is built for each interrupt controller and those `irq_domains` are organized into hierarchy.
When building `irq_domain` hierarchy, the `irq_domain` near to the device is **child** and the `irq_domain` near to CPU is parent.
So a hierarchy structure as below will be built for the example above:
```
CPU Vector irq_domain (root irq_domain to manage CPU vectors)
        ^
        |
Interrupt Remapping irq_domain (manage irq_remapping entries)
        ^
        |
IOAPIC irq_domain (manage IOAPIC delivery entries/pins)
```
:cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat::cat:


- [ ] mp_irqdomain_alloc : setup mapping from child chip irq to parent irq which is IR or vector
- [ ] x86_vector_alloc_irqs : it will update vector at last

## ir
```c
static struct irq_chip ioapic_ir_chip __read_mostly = {
	.name			= "IR-IO-APIC",
	.irq_startup		= startup_ioapic_irq,
	.irq_mask		= mask_ioapic_irq,
	.irq_unmask		= unmask_ioapic_irq,
	.irq_ack		= irq_chip_ack_parent,
	.irq_eoi		= ioapic_ir_ack_level,
	.irq_set_affinity	= ioapic_set_affinity,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_get_irqchip_state	= ioapic_irq_get_chip_state,
	.flags			= IRQCHIP_SKIP_SET_WAKE,
};
```

## request irq
devm_request_threaded_irq ==> request_threaded_irq

![loading](https://img2020.cnblogs.com/blog/1771657/202006/1771657-20200605223042609-247616444.png)


## irq desc
- [ ] /sys/kernel/irq
  - chiq_name_show type_show hwirq_show type_show wakeup_show name_show action_show
  - irq_sysfs_add

[answer this question](https://unix.stackexchange.com/questions/579513/what-is-meaning-of-empty-action-respect-to-sys-kernel-irq)

- [ ] 显然，CPU 是没有超过 256 个引脚来作为 中断线的，那么 CPU 读取到是什么 ? 最后如何装换为 irq line ?

## affinity

```c
static int __init irq_affinity_setup(char *str)
static void __init init_irq_default_affinity(void)


static int alloc_masks(struct irq_desc *desc, int node)
static void free_masks(struct irq_desc *desc)
```
> cpu mask 的功能好神奇 ? 还可以实现什么功能

## nmi

## apic
https://habr.com/en/post/446312/


# gpio
https://github.com/Manawyrm/pata-gpio


## idt
- arch/x86/include/asm/idtentry.h
- arch/x86/kernel/irq.c

- [x] DEFINE_IDTENTRY_IRQ : interesting, wrap interrupt handler with `irq_enter_rcu`, `irq_exit_rcu` and `irqentry_exit`



```c
/*
 * common_interrupt() handles all normal device IRQ's (the special SMP
 * cross-CPU interrupts have their own entry points).
 */
DEFINE_IDTENTRY_IRQ(common_interrupt)
```




- [x] How exception are set
```c
/*
 * Dummy trap number so the low level ASM macro vector number checks do not
 * match which results in emitting plain IDTENTRY stubs without bells and
 * whistels.
 */
#define X86_TRAP_OTHER		0xFFFF

/* Simple exception entry points. No hardware error code */
DECLARE_IDTENTRY(X86_TRAP_DE,		exc_divide_error);
DECLARE_IDTENTRY(X86_TRAP_OF,		exc_overflow);
DECLARE_IDTENTRY(X86_TRAP_BR,		exc_bounds);
DECLARE_IDTENTRY(X86_TRAP_NM,		exc_device_not_available);
DECLARE_IDTENTRY(X86_TRAP_OLD_MF,	exc_coproc_segment_overrun);
DECLARE_IDTENTRY(X86_TRAP_SPURIOUS,	exc_spurious_interrupt_bug);
DECLARE_IDTENTRY(X86_TRAP_MF,		exc_coprocessor_error);
DECLARE_IDTENTRY(X86_TRAP_XF,		exc_simd_coprocessor_error);

/* 32bit software IRET trap. Do not emit ASM code */
DECLARE_IDTENTRY_SW(X86_TRAP_IRET,	iret_error);

/* Simple exception entries with error code pushed by hardware */
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_TS,	exc_invalid_tss);
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_NP,	exc_segment_not_present);
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_SS,	exc_stack_segment);
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_GP,	exc_general_protection);
DECLARE_IDTENTRY_ERRORCODE(X86_TRAP_AC,	exc_alignment_check);

/* Raw exception entries which need extra work */
DECLARE_IDTENTRY_RAW(X86_TRAP_UD,		exc_invalid_op);
DECLARE_IDTENTRY_RAW(X86_TRAP_BP,		exc_int3);
DECLARE_IDTENTRY_RAW_ERRORCODE(X86_TRAP_PF,	exc_page_fault);
```

```c
/*
 * The default IDT entries which are set up in trap_init() before
 * cpu_init() is invoked. Interrupt stacks cannot be used at that point and
 * the traps which use them are reinitialized with IST after cpu_init() has
 * set up TSS.
 */
static const __initconst struct idt_data def_idts[] = {
	INTG(X86_TRAP_DE,		asm_exc_divide_error),
	INTG(X86_TRAP_NMI,		asm_exc_nmi),
	INTG(X86_TRAP_BR,		asm_exc_bounds),
	INTG(X86_TRAP_UD,		asm_exc_invalid_op),
	INTG(X86_TRAP_NM,		asm_exc_device_not_available),
	INTG(X86_TRAP_OLD_MF,		asm_exc_coproc_segment_overrun),
	INTG(X86_TRAP_TS,		asm_exc_invalid_tss),
	INTG(X86_TRAP_NP,		asm_exc_segment_not_present),
	INTG(X86_TRAP_SS,		asm_exc_stack_segment),
	INTG(X86_TRAP_GP,		asm_exc_general_protection),
	INTG(X86_TRAP_SPURIOUS,		asm_exc_spurious_interrupt_bug),
	INTG(X86_TRAP_MF,		asm_exc_coprocessor_error),
	INTG(X86_TRAP_AC,		asm_exc_alignment_check),
	INTG(X86_TRAP_XF,		asm_exc_simd_coprocessor_error),

#ifdef CONFIG_X86_32
	TSKG(X86_TRAP_DF,		GDT_ENTRY_DOUBLEFAULT_TSS),
#else
	INTG(X86_TRAP_DF,		asm_exc_double_fault),
#endif
	INTG(X86_TRAP_DB,		asm_exc_debug),

#ifdef CONFIG_X86_MCE
	INTG(X86_TRAP_MC,		asm_exc_machine_check),
#endif

	SYSG(X86_TRAP_OF,		asm_exc_overflow),
#if defined(CONFIG_IA32_EMULATION)
	SYSG(IA32_SYSCALL_VECTOR,	entry_INT80_compat),
#elif defined(CONFIG_X86_32)
	SYSG(IA32_SYSCALL_VECTOR,	entry_INT80_32),
#endif
};
```
## handle irq
asm_common_interrupt => handle_irq => run_irq_on_irqstack_cond 

```c
static __always_inline void
run_irq_on_irqstack_cond(void (*func)(struct irq_desc *desc), struct irq_desc *desc,
			 struct pt_regs *regs)
{
	lockdep_assert_irqs_disabled();

	if (irq_needs_irq_stack(regs))
		__run_irq_on_irqstack(func, desc);
	else
		func(desc);
}
```

- [ ] irq_desc::handler
    - [ ] `__irq_do_set_handler` : currently this is only function where irq_desc::handler is set


```c
const struct irq_domain_ops mp_ioapic_irqdomain_ops = {
	.alloc		= mp_irqdomain_alloc,
	.free		= mp_irqdomain_free,
	.activate	= mp_irqdomain_activate,
	.deactivate	= mp_irqdomain_deactivate,
};
```
mp_irqdomain_alloc ==> mp_register_handler ==> `__irq_set_handler` ==> `__irq_do_set_handler`


`__irq_domain_alloc_irqs` ==> irq_domain_alloc_irqs_hierarchy



## x86 vector
arch/x86/kernel/irqinit.c

with irq domain, we can map *linux irq* and *hw irq*, 
`vector_irq_t` maps interrupt number ==> irq_desc, used in `common_interrupt`

```c
DEFINE_PER_CPU(vector_irq_t, vector_irq) = {
	[0 ... NR_VECTORS - 1] = VECTOR_UNUSED,
};
```

- [x] how interrupt are seted in idt
    - **idt_setup_apic_and_irq_gates**
- [ ] how interrupt are set in vector_irq
  - [ ] lapic_online : irq and desc relation already setup, *find* out when they are setup.

assign_irq_vector ==> assign_vector_locked


## isa and pci
[^1]:
ISA and PCI handle interrupts very differently. ISA expansion cards are configured manually for IRQ, usually by setting a jumper, but sometimes by running a setup program. All ISA slots have all IRQ lines present, so it doesn’t matter which card is placed in which slot. ISA cards use edge-sensitive interrupts, which means that an ISA device asserts a voltage on one of the interrupt lines to generate an interrupt. That in turn means that ISA devices cannot share interrupts because when the processor senses voltage on a particular interrupt line, it has no way to determine which of multiple devices might be asserting that interrupt. For ISA slots and devices, the rule is simple: two devices cannot share an IRQ if there is any possibility that those two devices may be used simultaneously. In practice that means that you cannot assign the same IRQ to more than one ISA device.

PCI cards use level-sensitive interrupts, which means that different PCI devices can assert different voltages on the same physical interrupt line, allowing the processor to determine which device generated the interrupt. PCI cards and slots manage interrupts internally. A PCI bus normally supports a maximum of four PCI slots, numbered 1 through 4. Each PCI slot can access four interrupts, labeled INT#1 through INT#4 (or INT#A through INT#D). Ordinarily, INT#1/A is used by PCI Slot 1, INT#2/B by Slot 2, and so on.


## eoi
https://stackoverflow.com/questions/7005331/difference-between-io-apic-fasteoi-and-io-apic-edge




## ref
[^1]: https://www.oreilly.com/library/view/pc-hardware-in/059600513X/ch01s03s01s01.html
[^2]: [深度探索Linux系统虚拟化](https://book.douban.com/subject/35238691/)
