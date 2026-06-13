## api

### rcu_read_lock

```c
/**
 * rcu_read_lock() - mark the beginning of an RCU read-side critical section
 *
 * When synchronize_rcu() is invoked on one CPU while other CPUs
 * are within RCU read-side critical sections, then the
 * synchronize_rcu() is guaranteed to block until after all the other
 * CPUs exit their critical sections.  Similarly, if call_rcu() is invoked
 * on one CPU while other CPUs are within RCU read-side critical
 * sections, invocation of the corresponding RCU callback is deferred
 * until after the all the other CPUs exit their critical sections.
 *
 * In v5.0 and later kernels, synchronize_rcu() and call_rcu() also
 * wait for regions of code with preemption disabled, including regions of <---------------- ?????
 * code with interrupts or softirqs disabled.  In pre-v5.0 kernels, which
 * define synchronize_sched(), only code enclosed within rcu_read_lock()
 * and rcu_read_unlock() are guaranteed to be waited for.
 *
 * Note, however, that RCU callbacks are permitted to run concurrently
 * with new RCU read-side critical sections.  One way that this can happen
 * is via the following sequence of events: (1) CPU 0 enters an RCU
 * read-side critical section, (2) CPU 1 invokes call_rcu() to register
 * an RCU callback, (3) CPU 0 exits the RCU read-side critical section,
 * (4) CPU 2 enters a RCU read-side critical section, (5) the RCU
 * callback is invoked.  This is legal, because the RCU read-side critical
 * section that was running concurrently with the call_rcu() (and which
 * therefore might be referencing something that the corresponding RCU
 * callback would free up) has completed before the corresponding
 * RCU callback is invoked.
 *
 * RCU read-side critical sections may be nested.  Any deferred actions
 * will be deferred until the outermost RCU read-side critical section
 * completes.
 *
 * You can avoid reading and understanding the next paragraph by
 * following this rule: don't put anything in an rcu_read_lock() RCU
 * read-side critical section that would block in a !PREEMPTION kernel.
 * But if you want the full story, read on!
 *
 * In non-preemptible RCU implementations (pure TREE_RCU and TINY_RCU),
 * it is illegal to block while in an RCU read-side critical section.
 * In preemptible RCU implementations (PREEMPT_RCU) in CONFIG_PREEMPTION
 * kernel builds, RCU read-side critical sections may be preempted,
 * but explicit blocking is illegal.  Finally, in preemptible RCU
 * implementations in real-time (with -rt patchset) kernel builds, RCU
 * read-side critical sections may be preempted and they may also block, but
 * only when acquiring spinlocks that are subject to priority inheritance.
 */
static __always_inline void rcu_read_lock(void)
```

如果将 debug 选项全部关闭，那么可以简化为:

```c
	WRITE_ONCE(current->rcu_read_lock_nesting, READ_ONCE(current->rcu_read_lock_nesting) + 1);
	barrier();
```

- [ ] tdp_mmu_iter_cond_resched 中为什么先 rcu_read_unlock ，
然后 rcu_read_lock ，这个东西到底在保护什么?


- 当使用不可抢占的 RCU 时，`rcu_read_lock`/`rcu_read_unlock`之间不能使用可以睡眠的代码
  - 这样写代码，最后会被检测出来的
  - [ ] 我猜测是，synchronize_rcu 用 cpu 的 当上下文切换来作为 cpu 离开缓冲区的指示，如果 rcu_read_lock 可以执行了，
  那么 synchronize_rcu 会接受到一个假的信号。

### synchronize_rcu

```c
/**
 * synchronize_rcu - wait until a grace period has elapsed.
 *
 * Control will return to the caller some time after a full grace
 * period has elapsed, in other words after all currently executing RCU
 * read-side critical sections have completed.  Note, however, that
 * upon return from synchronize_rcu(), the caller might well be executing
 * concurrently with new RCU read-side critical sections that began while
 * synchronize_rcu() was waiting.
 *
 * RCU read-side critical sections are delimited by rcu_read_lock()
 * and rcu_read_unlock(), and may be nested.  In addition, but only in
 * v5.0 and later, regions of code across which interrupts, preemption,
 * or softirqs have been disabled also serve as RCU read-side critical
 * sections.  This includes hardware interrupt handlers, softirq handlers,
 * and NMI handlers.
 *
 * Note that this guarantee implies further memory-ordering guarantees.
 * On systems with more than one CPU, when synchronize_rcu() returns,
 * each CPU is guaranteed to have executed a full memory barrier since
 * the end of its last RCU read-side critical section whose beginning
 * preceded the call to synchronize_rcu().  In addition, each CPU having
 * an RCU read-side critical section that extends beyond the return from
 * synchronize_rcu() is guaranteed to have executed a full memory barrier
 * after the beginning of synchronize_rcu() and before the beginning of
 * that RCU read-side critical section.  Note that these guarantees include
 * CPUs that are offline, idle, or executing in user mode, as well as CPUs
 * that are executing in the kernel.
 *
 * Furthermore, if CPU A invoked synchronize_rcu(), which returned
 * to its caller on CPU B, then both CPU A and CPU B are guaranteed
 * to have executed a full memory barrier during the execution of
 * synchronize_rcu() -- even if CPU A and CPU B are the same CPU (but
 * again only if the system has more than one CPU).
 *
 * Implementation of these memory-ordering guarantees is described here:
 * Documentation/RCU/Design/Memory-Ordering/Tree-RCU-Memory-Ordering.rst.
 */
```

```txt
          read lock
┌───────────────────────┐
│                       │   mb
│       ┌───────────────┼────────────┐synchronize_rcu
│       └───────────────┼────────────┘
└───────────────────────┘
```
对于正好被管理的 read-side critical region ，不仅仅要等待 read lock 结束之后，做的操作至少为 mb 才可以。


> In addition, each CPU having an RCU read-side critical section that extends beyond the return from synchronize_rcu() is guaranteed to have
> executed a full memory barrier after the beginning of synchronize_rcu() and before the beginning of that RCU read-side critical section.

```txt
    synchronize_rcu
┌────--───────────┐
│                 │    read lock
│  mb  ┌──────────┼────────────────────┐
│      └──────────┼────────────────────┘
└───--────────────┘
```
如果一个 read-side critical 不是这个 synchronize_rcu 来管理的。执行了 mb 之后，read lock 才可以开始。

有点乱，继续分析。

具体案例:  `__zswap_pool_release` 中存在 `synchronize_rcu` 开始分析

synchronize_rcu 并不是一个频繁被调用的函数，一个触发方法启动 qemu

### rcu_read_unlock

```c
/**
 * rcu_read_unlock() - marks the end of an RCU read-side critical section.
 *
 * In almost all situations, rcu_read_unlock() is immune from deadlock.
 * In recent kernels that have consolidated synchronize_sched() and
 * synchronize_rcu_bh() into synchronize_rcu(), this deadlock immunity
 * also extends to the scheduler's runqueue and priority-inheritance
 * spinlocks, courtesy of the quiescent-state deferral that is carried
 * out when rcu_read_unlock() is invoked with interrupts disabled.
 *
 * See rcu_read_lock() for more information.
 */
```

这个不容易简化了。

### rcu_barrier

等待 rcu callback 返回，但是不等待 grace period

```c
/**
 * rcu_barrier - Wait until all in-flight call_rcu() callbacks complete.
 *
 * Note that this primitive does not necessarily wait for an RCU grace period
 * to complete.  For example, if there are no RCU callbacks queued anywhere
 * in the system, then rcu_barrier() is within its rights to return
 * immediately, without waiting for anything, much less an RCU grace period.
 */
void rcu_barrier(void)
```

而 synchronize_rcu 是需要等待整个 grace period 的。

- 理解下这个 : commit ec084de929e4 ("fs/writeback.c: use rcu_barrier() to wait for inflight wb switches going into workqueue when umount")

https://www.kernel.org/doc/Documentation/RCU/rcubarrier.txt 分析很清楚，很短，似乎是 module 使用的

#### [ ] 什么时候使用 rcu_barrier ，什么时候使用 synchronize_rcu

### call_rcu

```c
/**
 * call_rcu() - Queue an RCU callback for invocation after a grace period.
 * By default the callbacks are 'lazy' and are kept hidden from the main
 * ->cblist to prevent starting of grace periods too soon.
 * If you desire grace periods to start very soon, use call_rcu_hurry().
 *
 * @head: structure to be used for queueing the RCU updates.
 * @func: actual callback function to be invoked after the grace period
 *
 * The callback function will be invoked some time after a full grace
 * period elapses, in other words after all pre-existing RCU read-side
 * critical sections have completed.  However, the callback function
 * might well execute concurrently with RCU read-side critical sections
 * that started after call_rcu() was invoked.
 *
 * RCU read-side critical sections are delimited by rcu_read_lock()
 * and rcu_read_unlock(), and may be nested.  In addition, but only in
 * v5.0 and later, regions of code across which interrupts, preemption,
 * or softirqs have been disabled also serve as RCU read-side critical
 * sections.  This includes hardware interrupt handlers, softirq handlers,
 * and NMI handlers.
 *
 * Note that all CPUs must agree that the grace period extended beyond
 * all pre-existing RCU read-side critical section.  On systems with more
 * than one CPU, this means that when "func()" is invoked, each CPU is
 * guaranteed to have executed a full memory barrier since the end of its
 * last RCU read-side critical section whose beginning preceded the call
 * to call_rcu().  It also means that each CPU executing an RCU read-side
 * critical section that continues beyond the start of "func()" must have
 * executed a memory barrier after the call_rcu() but before the beginning
 * of that RCU read-side critical section.  Note that these guarantees
 * include CPUs that are offline, idle, or executing in user mode, as
 * well as CPUs that are executing in the kernel.
 *
 * Furthermore, if CPU A invoked call_rcu() and CPU B invoked the
 * resulting RCU callback function "func()", then both CPU A and CPU B are
 * guaranteed to execute a full memory barrier during the time interval
 * between the call to call_rcu() and the invocation of "func()" -- even
 * if CPU A and CPU B are the same CPU (but again only if the system has
 * more than one CPU).
 *
 * Implementation of these memory-ordering guarantees is described here:
 * Documentation/RCU/Design/Memory-Ordering/Tree-RCU-Memory-Ordering.rst.
 */
void call_rcu(struct rcu_head *head, rcu_callback_t func)
```

TODO 一个详细的注释，调用位置很多，有 200 个 (仅仅是当前的配置)

其作用使用一个例子来说明一下吧:

```txt
@[
    call_rcu+5
    __dentry_kill+220
    __fput+230
    task_work_run+90
    exit_to_user_mode_prepare+511
    syscall_exit_to_user_mode+27
    do_syscall_64+83
    entry_SYSCALL_64_after_hwframe+111
]: 5
```

- \_\_dentry_kill
  - dentry_free
    - call_rcu(&dentry->d_u.d_rcu, \_\_d_free);

```txt
@[
    __d_free+5
    rcu_do_batch+502
    rcu_core+421
    __do_softirq+260
    __irq_exit_rcu+167
    irq_exit_rcu+14
    sysvec_apic_timer_interrupt+63
    asm_sysvec_apic_timer_interrupt+26
]: 18
```

### nocb_cb_wait

每一个

```txt
#0  rcu_spawn_cpu_nocb_kthread (cpu=cpu@entry=1) at kernel/rcu/tree_nocb.h:1488
#1  0xffffffff811ffa43 in rcutree_prepare_cpu (cpu=1) at kernel/rcu/tree.c:4332
#2  0xffffffff81142bf5 in cpuhp_invoke_callback (cpu=cpu@entry=1, state=CPUHP_RCUTREE_PREP, bringup=bringup@entry=true, node=node@entry=0x0 <fixed_percpu_data>,
    lastp=lastp@entry=0x0 <fixed_percpu_data>) at kernel/cpu.c:195
#3  0xffffffff81142ff0 in __cpuhp_invoke_callback_range (bringup=bringup@entry=true, cpu=cpu@entry=1, st=st@entry=0xffff88fe3f65c3e0, target=target@entry=CPUHP_BP_KICK_AP,
    nofail=nofail@entry=false) at kernel/cpu.c:928
#4  0xffffffff811442d8 in cpuhp_invoke_callback_range (target=CPUHP_BP_KICK_AP, st=0xffff88fe3f65c3e0, cpu=1, bringup=true) at kernel/cpu.c:952
#5  cpuhp_up_callbacks (target=CPUHP_BP_KICK_AP, st=0xffff88fe3f65c3e0, cpu=1) at kernel/cpu.c:983
#6  _cpu_up (cpu=cpu@entry=1, tasks_frozen=tasks_frozen@entry=0, target=target@entry=CPUHP_BP_KICK_AP) at kernel/cpu.c:1671
#7  0xffffffff811444a0 in cpu_up (target=CPUHP_BP_KICK_AP, cpu=1) at kernel/cpu.c:1707
#8  cpu_up (cpu=<optimized out>, target=CPUHP_BP_KICK_AP) at kernel/cpu.c:1679
#9  0xffffffff835e8319 in cpuhp_bringup_mask (mask=mask@entry=0xffffffff82f67ec8 <__cpu_present_mask>, ncpus=63, ncpus@entry=64, target=target@entry=CPUHP_BP_KICK_AP) at kernel/cpu.c:1773
#10 0xffffffff835e84ff in cpuhp_bringup_cpus_parallel (ncpus=64) at kernel/cpu.c:1837
#11 bringup_nonboot_cpus (setup_max_cpus=<optimized out>) at kernel/cpu.c:1848
#12 0xffffffff835f095a in smp_init () at kernel/smp.c:960
#13 0xffffffff835bb646 in kernel_init_freeable () at init/main.c:1540
#14 0xffffffff821b601a in kernel_init (unused=<optimized out>) at init/main.c:1437
#15 0xffffffff810e3a51 in ret_from_fork (prev=<optimized out>, regs=0xffffc90000017f58, fn=0xffffffff821b6000 <kernel_init>, fn_arg=0x0 <fixed_percpu_data>)
    at arch/x86/kernel/process.c:145
```

没看到哪里体现出来的 nocb 的设置的

```c
/*
 * Per-rcu_data kthread, but only for no-CBs CPUs.  Repeatedly invoke
 * nocb_cb_wait() to do the dirty work.
 */
static int rcu_nocb_cb_kthread(void *arg)
```

### [ ] call_rcu_hurry
需要首先理解 CONFIG_RCU_LAZY 才可以

## reference
配合 Documentation/RCU/rcu_dereference.rst 和注释阅读吧

### RCU_INITIALIZER
RCU_POINTER_INITIALIZER

只是长的和 RCU_INIT_POINTER 类似，但是实际上，就是一个类型转换而已。

### [ ] RCU_INIT_POINTER
有待测试效果。



### rcu_assign_pointer (writer)
RCU_INIT_POINTER 和 rcu_assign_pointer 区别就在于 memory barrier ，两者都有很长的注释

rcu_assign_pointer 中需要包含 smp_store_release 是为了确保在“发布”一个新指针之前，
这个新指针所指向的数据已经完全准备就绪，并且对系统中的所有其他 CPU 可见。

```txt
#define rcu_assign_pointer(p, v)					      \
do {									      \
	uintptr_t _r_a_p__v = (uintptr_t)(v);				      \
	rcu_check_sparse(p, __rcu);					      \
									      \
	if (__builtin_constant_p(v) && (_r_a_p__v) == (uintptr_t)NULL)	      \
		WRITE_ONCE((p), (typeof(p))(_r_a_p__v));		      \
	else								      \
		smp_store_release(&p, RCU_INITIALIZER((typeof(p))_r_a_p__v)); \
} while (0)
```


### rcu_dereference (reader)
果然，rcu_dereference 需要先获取地址，才能读取其中的内容。

```txt
worker = rcu_dereference(vq->worker)
if (worker->stop) {
  // ...
}
```
除了 Alpha 架构外，其他的 CPU 都是会认为这个有地址依赖的。
就目前而言，这里无需额外的 memory barrier

### [ ] rcu_dereference_check (reader)

### [ ] rcu_dereference_protected (writer)

rcu_dereference_protected 和 rcu_dereference_check 为什么关系?

- https://stackoverflow.com/questions/39251287/rcu-dereference-vs-rcu-dereference-protected

```c
#define __rcu_dereference_check(p, local, c, space) \
({ \
	/* Dependency order vs. p above. */ \
	typeof(*p) *local = (typeof(*p) *__force)READ_ONCE(p); \
	RCU_LOCKDEP_WARN(!(c), "suspicious rcu_dereference_check() usage"); \
	rcu_check_sparse(p, space); \
	((typeof(*p) __force __kernel *)(local)); \
})
#define __rcu_dereference_protected(p, local, c, space) \
({ \
	RCU_LOCKDEP_WARN(!(c), "suspicious rcu_dereference_protected() usage"); \
	rcu_check_sparse(p, space); \
	((typeof(*p) __force __kernel *)(p)); \
})
```

所以，他们的本质区别在于 rcu_dereference_check 多了一行:
```txt
	/* Dependency order vs. p above. */
	typeof(*p) *local = (typeof(*p) *__force)READ_ONCE(p);
```

rcu_dereference_check 中的注释提到了为什么这里需要 READ_ONCE

> Inserts memory barriers on architectures that require them
> (currently only the Alpha), prevents the compiler from refetching
> (and from merging fetches), and, more importantly, documents exactly
> which pointers are protected by RCU and checks that the pointer is
> annotated as __rcu.

1. 防止出现 refetch
```txt
bar = rcu_dereference_check(foo->bar, lockdep_is_held(&foo->lock));
m += bar->a
m += bar->b
```
refetch 指的是这个现象 : bar->a 和 bar->b 被重新指令，重新从 foo->bar 中加载一次
那么就是从两个结构体中发现的。

2. 防止 merge fetch : 这个容易理解，如果没有 READ_ONCE ，那么可能从 rcu_read_lock 外读取到的指针，
而不是重新读的。


### rcu_access_pointer (writer)

```c
/**
 * rcu_access_pointer() - fetch RCU pointer with no dereferencing
 * @p: The pointer to read
 *
 * Return the value of the specified RCU-protected pointer, but omit the
 * lockdep checks for being in an RCU read-side critical section.  This is
 * useful when the value of this pointer is accessed, but the pointer is
 * not dereferenced, for example, when testing an RCU-protected pointer
 * against NULL.  Although rcu_access_pointer() may also be used in cases
 * where update-side locks prevent the value of the pointer from changing,
 * you should instead use rcu_dereference_protected() for this use case.
 * Within an RCU read-side critical section, there is little reason to
 * use rcu_access_pointer().
 *
 * It is usually best to test the rcu_access_pointer() return value
 * directly in order to avoid accidental dereferences being introduced
 * by later inattentive changes.  In other words, assigning the
 * rcu_access_pointer() return value to a local variable results in an
 * accident waiting to happen.
 *
 * It is also permissible to use rcu_access_pointer() when read-side
 * access to the pointer was removed at least one grace period ago, as is
 * the case in the context of the RCU callback that is freeing up the data,
 * or after a synchronize_rcu() returns.  This can be useful when tearing
 * down multi-linked structures after a grace period has elapsed.  However,
 * rcu_dereference_protected() is normally preferred for this use case.
 */
#define rcu_access_pointer(p) __rcu_access_pointer((p), __UNIQUE_ID(rcu), __rcu)
```
经典的使用场景是:
1. 获取指针，然后判断是不是 NULL
2. Within an RCU read-side critical section, there is little reason to use rcu_access_pointer().
3. In other words, assigning the rcu_access_pointer() return value to a local variable results in an
 accident waiting to happen.

4. It is also permissible to use rcu_access_pointer() when read-side
access to the pointer was removed at least one grace period ago, as is
the case in the context of the RCU callback that is freeing up the data,
or after a synchronize_rcu() returns.  This can be useful when tearing
down multi-linked structures after a grace period has elapsed.  However,
rcu_dereference_protected() is normally preferred for this use case.

虽然注释很长，但是 rcu_access_pointer 和 rcu_dereference_check 的区别只是在于
没有 lockdep 的检查
```txt
#define __rcu_access_pointer(p, local, space) \
({ \
	typeof(*p) *local = (typeof(*p) *__force)READ_ONCE(p); \
	rcu_check_sparse(p, space); \
	((typeof(*p) __force __kernel *)(local)); \
})

#define __rcu_dereference_check(p, local, c, space) \
({ \
	/* Dependency order vs. p above. */ \
	typeof(*p) *local = (typeof(*p) *__force)READ_ONCE(p); \
	RCU_LOCKDEP_WARN(!(c), "suspicious rcu_dereference_check() usage"); \
	rcu_check_sparse(p, space); \
	((typeof(*p) __force __kernel *)(local)); \
})
```

到底是 read 中使用，还是 write 中使用的:
```c
static int veth_xdp_xmit(struct net_device *dev, int n,
			 struct xdp_frame **frames,
			 u32 flags, bool ndo_xmit)
{
	struct veth_priv *rcv_priv, *priv = netdev_priv(dev);
	int i, ret = -ENXIO, nxmit = 0;
	struct net_device *rcv;
	unsigned int max_len;
	struct veth_rq *rq;

	if (unlikely(flags & ~XDP_XMIT_FLAGS_MASK))
		return -EINVAL;

	rcu_read_lock();
	rcv = rcu_dereference(priv->peer);
	if (unlikely(!rcv))
		goto out;

	rcv_priv = netdev_priv(rcv);
	rq = &rcv_priv->rq[veth_select_rxq(rcv)];
	/* The napi pointer is set if NAPI is enabled, which ensures that
	 * xdp_ring is initialized on receive side and the peer device is up.
	 */
	if (!rcu_access_pointer(rq->napi))
		goto out;

```


### [ ] rcu_dereference_bh
rcu_dereference_bh_check

### rcu_replace_pointer

理解下 scsi_device_dev_release 中的内容:

```txt
	mutex_lock(&sdev->inquiry_mutex);
	vpd_pg0 = rcu_replace_pointer(sdev->vpd_pg0, vpd_pg0,
				       lockdep_is_held(&sdev->inquiry_mutex));
	vpd_pg80 = rcu_replace_pointer(sdev->vpd_pg80, vpd_pg80,
				       lockdep_is_held(&sdev->inquiry_mutex));
	vpd_pg83 = rcu_replace_pointer(sdev->vpd_pg83, vpd_pg83,
				       lockdep_is_held(&sdev->inquiry_mutex));
	vpd_pg89 = rcu_replace_pointer(sdev->vpd_pg89, vpd_pg89,
				       lockdep_is_held(&sdev->inquiry_mutex));
	vpd_pgb0 = rcu_replace_pointer(sdev->vpd_pgb0, vpd_pgb0,
				       lockdep_is_held(&sdev->inquiry_mutex));
	vpd_pgb1 = rcu_replace_pointer(sdev->vpd_pgb1, vpd_pgb1,
				       lockdep_is_held(&sdev->inquiry_mutex));
	vpd_pgb2 = rcu_replace_pointer(sdev->vpd_pgb2, vpd_pgb2,
				       lockdep_is_held(&sdev->inquiry_mutex));
	mutex_unlock(&sdev->inquiry_mutex);
```

## synchronize_rcu_expedited 的作用
<!-- 2547aa81-89a9-43e7-9631-6a2c4251f04d -->

例如为什么这里需要使用 synchronize_rcu_expedited
```c
/*
 * Remove bdi from bdi_list, and ensure that it is no longer visible
 */
static void bdi_remove_from_list(struct backing_dev_info *bdi)
{
	spin_lock_bh(&bdi_lock);
	rb_erase(&bdi->rb_node, &bdi_tree);
	list_del_rcu(&bdi->bdi_list);
	spin_unlock_bh(&bdi_lock);

	synchronize_rcu_expedited();
}
```

`synchronize_rcu_expedited()` 是 Linux 内核 RCU（Read-Copy Update）机制中一个**强制、加速完成 RCU 宽限期（grace period）**的同步接口。它的核心作用是：

> **以尽可能快的方式，等待一个 RCU 宽限期结束，从而保证在该调用返回后，所有在调用之前进入 RCU 读临界区的读者都已经退出。**

### 1. 语义层面（它“保证”什么）

```c
synchronize_rcu_expedited();
```

在返回时，内核保证：

* 在调用开始之前已经进入的 **所有 RCU 读临界区**

  * `rcu_read_lock() / rcu_read_unlock()`
  * `rcu_read_lock_sched()`
  * `rcu_read_lock_bh()`
* 都已经全部退出

因此，调用之后可以安全地：

* 释放 RCU 保护的对象
* 重用内存
* 做“不可逆”的全局状态修改

从**正确性语义**上看，它与 `synchronize_rcu()` 是**等价的**，区别只在于“速度 vs 代价”。

### 2. “expedited” 的真正含义：如何加速？

普通的：

```c
synchronize_rcu();
```

依赖 **自然调度行为** 来推进宽限期：

* CPU 自愿调度
* 上下文切换
* tick
* quiescent state（QS）自然出现
(这对吗?)

而 `synchronize_rcu_expedited()` 的策略是：

#### 核心思路：**强制所有 CPU 尽快进入 quiescent state**

典型手段包括（取决于内核版本与配置）：

1. **向所有在线 CPU 发送 IPI**

   * 要求它们立即执行回调
2. 在 IPI 处理函数中：

   * 强制触发调度点
   * 或显式报告 quiescent state
3. 快速标记该 CPU 已经“过宽限期”

结果是：

* 不再等待自然调度
* 即使 CPU 在忙等、跑内核循环，也会被打断

因此，宽限期通常在 **毫秒级甚至更快**完成。

---

### 3. 代价与副作用（为什么不能滥用）

`expedited` 的代价是**全局性和侵入性**的：

* 🚨 向所有 CPU 发送 IPI（跨 NUMA 代价更高）
* 🚨 打断正在运行的内核路径
* 🚨 在大核数系统（几十 / 上百 CPU）上非常昂贵
* 🚨 会明显影响系统实时性和吞吐量

因此内核文档和维护者的共识是：

> **`synchronize_rcu_expedited()` 是“紧急刹车”，不是日常工具。**

### 4. 典型使用场景（什么时候“必须”用）

#### 4.1 进程退出 / 错误回滚路径

例如：

* 模块加载失败后的清理
* 初始化中途出错，需要立即回收 RCU 保护对象

此时：

* 路径执行频率极低
* 更关心“尽快完成”而不是系统吞吐

#### 4.2 卸载模块（rmmod）

模块卸载往往使用 expedited 版本：

* 避免用户态看到 rmmod 卡住很久
* 提升可预期性

#### 4.3 挂死风险路径

在以下情况下普通 `synchronize_rcu()` 可能**长时间不返回**：

* CPU 被 pin 住（`nohz_full`、RT 场景）
* 长时间关闭抢占
* 自旋在内核态

`expedited` 可以打破这种僵局。

### 5. 与相关接口的对比

| 接口                            | 特点              |
| ----------------------------- | --------------- |
| `synchronize_rcu()`           | 等待自然宽限期，低开销，可能慢 |
| `synchronize_rcu_expedited()` | 强制推进，快但代价极高     |
| `call_rcu()`                  | 异步回调，不阻塞        |
| `call_rcu_expedited()`        | 异步 + 加速         |
| `kfree_rcu()`                 | 延迟释放内存（首选）      |

> **经验法则**
>
> * 能用 `call_rcu()` / `kfree_rcu()`：不要同步等待
> * 能用 `synchronize_rcu()`：不要用 expedited
> * 只有在“必须尽快完成 + 调用极少”时才用 expedited


## 这是错误吗?

这个不就是相当于是将 rcu 直接访问了，放到了 local variable 中去吗?
```c
static void root_free_rcu(struct rcu_head *rcu)
{
	struct allowedips_node *node, *stack[MAX_ALLOWEDIPS_DEPTH] = {
		container_of(rcu, struct allowedips_node, rcu) };
	unsigned int len = 1;

	while (len > 0 && (node = stack[--len])) {
		push_rcu(stack, node->bit[0], &len);
		push_rcu(stack, node->bit[1], &len);
		kmem_cache_free(node_cache, node);
	}
}

static void push_rcu(struct allowedips_node **stack,
		     struct allowedips_node __rcu *p, unsigned int *len)
{
	if (rcu_access_pointer(p)) {
		if (WARN_ON(IS_ENABLED(DEBUG) && *len >= MAX_ALLOWEDIPS_DEPTH))
			return;
		stack[(*len)++] = rcu_dereference_raw(p);
	}
}
```

## 理解一下 rcu 的 api 为什么这么设计来着

前面的 rcu 判断都是如此吧

```c
/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add_rcu(struct list_head *new,
		struct list_head *prev, struct list_head *next)
{
	if (!__list_add_valid(new, prev, next))
		return;

	new->next = next;
	new->prev = prev;
	rcu_assign_pointer(list_next_rcu(prev), new);
	next->prev = new;
}
```

```c
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	if (!__list_add_valid(new, prev, next))
		return;

	next->prev = new;
	new->next = next;
	new->prev = prev;
	WRITE_ONCE(prev->next, new);
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
