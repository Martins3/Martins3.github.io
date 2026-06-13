# RCU

rcu 无法处理 writer 多的问题。

## 基本想法

The principle of RCU is simple: The mechanism keeps track of all users of the pointer to the shared
data structure. When the structure is supposed to change, a copy (or a new instance that is filled in
appropriately, this does not make any difference) is first created and the change is performed there. After
all previous readers have finished their reading work on the old copy, the pointer can be replaced by a
pointer to the new, modified copy. **Notice that this allows read access to happen concurrently with write
updates!**


- quiescent state : 当 reader 离开 critical region 的时候
- grace period : 等待所有的引用被释放的时间

- 当所有的 CPU 进入过 quiescent state 之后，可以知道一定可以开始回收了

参考 : https://doc.dpdk.org/guides/prog_guide/rcu_lib.html


在 Linux 的设计中，当立刻 critical region 的时候，会通知等待的 CPU 。

当调用 synchronize_rcu 或者 call_rcu 之后，
需要保证所有的 CPU 都离开了 read-side critical region ，
这样就可以开始进行回收了。

### 在 context switch 的通知机制

第一个小问题:

如何保证 preempt kernel 中 rcu critical region 中:

[How RCU reader section is protected from preemption](https://stackoverflow.com/questions/32260422/how-rcu-reader-section-is-protected-from-preemption?rq=3)

证据在: __schedule -> rcu_note_context_switch -> rcu_preempt_ctxt_queue

```c
/*
 * We have entered the scheduler, and the current task might soon be
 * context-switched away from.  If this task is in an RCU read-side
 * critical section, we will no longer be able to rely on the CPU to
 * record that fact, so we enqueue the task on the blkd_tasks list.
 * The task will dequeue itself when it exits the outermost enclosing
 * RCU read-side critical section.  Therefore, the current grace period
 * cannot be permitted to complete until the blkd_tasks list entries
 * predating the current grace period drain, in other words, until
 * rnp->gp_tasks becomes NULL.
 *
 * Caller must disable interrupts.
 */
void rcu_note_context_switch(bool preempt)
```

### 基于 tree 的刷新模式
```txt
@[
    rcu_start_this_gp+1
    rcu_accelerate_cbs+407
    __note_gp_changes+596
    note_gp_changes+95
    rcu_core+156
    handle_softirqs+228
    __irq_exit_rcu+152
    sysvec_apic_timer_interrupt+115
    asm_sysvec_apic_timer_interrupt+26
    cpuidle_enter_state+205
    cpuidle_enter+45
    do_idle+436
    cpu_startup_entry+41
    start_secondary+284
    common_startup_64+318
]: 14
```

在 rcu_do_batch 中实现调用 rcu_callback
```txt
@[
    rcu_do_batch+1
    rcu_core+441
    handle_softirqs+228
    __irq_exit_rcu+152
    sysvec_apic_timer_interrupt+115
    asm_sysvec_apic_timer_interrupt+26
    cpuidle_enter_state+205
    cpuidle_enter+45
    do_idle+436
    cpu_startup_entry+41
    start_secondary+284
    common_startup_64+318
]: 33
```

synchronize_rcu 的基本执行流程:

```sh
sudo perf ftrace -C0 -G synchronize_rcu -g 'smp_*'
```

```txt
# tracer: function_graph
#
# CPU  DURATION                  FUNCTION CALLS
# |     |   |                     |   |   |   |
  0)               |  synchronize_rcu() {
  0)               |    irq_enter_rcu() {
  0)   0.190 us    |      preempt_count_add();
  0)   0.501 us    |    }
  0)   0.141 us    |    __cond_resched();
  0)   0.090 us    |    rcu_gp_is_expedited();
  0)               |    __wait_rcu_gp() {
  0)   0.100 us    |      __init_swait_queue_head();
  0)               |      call_rcu_hurry() {
  0)               |        __call_rcu_common() {
  0)   0.090 us    |          rcu_segcblist_pend_cbs();
  0)   0.100 us    |          rcu_segcblist_enqueue();
  0)   0.461 us    |        }
  0)   0.631 us    |      }
  0)               |      wait_for_completion() {
  0) * 12969.91 us |    }
  0) * 12980.32 us |  }
```


似乎 synchronize_rcu 中的 wait_for_completion 是这样被唤醒的:

```txt
- sysvec_apic_timer_interrupt
  - __irq_exit_rcu
    - invoke_softirq
      - __do_softirq
        - rcu_core
          - rcu_check_quiescent_state
            - rcu_report_qs_rdp
              - swake_up_one
                - swake_up_locked
                  - swake_up_locked
                    - wake_up_process
                      - try_to_wake_up
                        - set_task_cpu
                          - migrate_task_rq_fair
```

## rcu 基本代码基本分析

tree.c 包含的三个文件，这就是全部的内容了:

```c
#include "tree_stall.h"  // 处理 stall detection 的
#include "tree_exp.h"    // 处理 expedited 的情况 ，主要是为了给实时系统
#include "tree_nocb.h"   // rcu isolation
#include "tree_plugin.h" // 处理 preemption 的
```


## expedited

- synchronize_rcu_expedited ，和 synchronize_rcu 相同，但是会加速

rcu_read_unlock 需要为 synchronize_rcu_expedited 提供的帮助 : 一旦结束，立刻告知当前任务已经结束。

```c
/**
 * synchronize_rcu_expedited - Brute-force RCU grace period
 *
 * Wait for an RCU grace period, but expedite it.  The basic idea is to
 * IPI all non-idle non-nohz online CPUs.  The IPI handler checks whether
 * the CPU is in an RCU critical section, and if so, it sets a flag that
 * causes the outermost rcu_read_unlock() to report the quiescent state
 * for RCU-preempt or asks the scheduler for help for RCU-sched.  On the
 * other hand, if the CPU is not in an RCU read-side critical section,
 * the IPI handler reports the quiescent state immediately.
 *
 * Although this is a great improvement over previous expedited
 * implementations, it is still unfriendly to real-time workloads, so is
 * thus not recommended for any sort of common-case code.  In fact, if
 * you are using synchronize_rcu_expedited() in a loop, please restructure
 * your code to batch your updates, and then use a single synchronize_rcu()
 * instead.
 *
 * This has the same semantics as (but is more brutal than) synchronize_rcu().
 */
void synchronize_rcu_expedited(void)
```

## kthread && softirq

似乎总是存在 softirq 和 kthread 总是成对出现的:
```c
/*
 * Wake up this CPU's rcuc kthread to do RCU core processing.
 */
static void invoke_rcu_core(void)
{
	if (!cpu_online(smp_processor_id()))
		return;
	if (use_softirq)
		raise_softirq(RCU_SOFTIRQ);
	else
		invoke_rcu_core_kthread();
}
```

```txt
1 I root           4       2  0  60 -20 -     0 -      Jun15 ?        00:00:00 [kworker/R-rcu_g]
1 I root          13       2  0  80   0 -     0 -      Jun15 ?        00:00:00 [rcu_tasks_kthread]
1 I root          14       2  0  80   0 -     0 -      Jun15 ?        00:00:00 [rcu_tasks_rude_kthread]
1 I root          15       2  0  80   0 -     0 -      Jun15 ?        00:00:00 [rcu_tasks_trace_kthread]
1 I root          17       2  0  80   0 -     0 -      Jun15 ?        00:06:10 [rcu_preempt]
1 S root          18       2  0  80   0 -     0 -      Jun15 ?        00:00:00 [rcu_exp_par_gp_kthread_worker/1]
1 S root          19       2  0  80   0 -     0 -      Jun15 ?        00:00:00 [rcu_exp_gp_kthread_worker]
1 S root          71       2  0  80   0 -     0 -      Jun15 ?        00:00:00 [rcu_exp_par_gp_kthread_worker/2]
```


```txt
invoke_rcu_core+1
rcu_sched_clock_irq+497
update_process_times+147
tick_sched_handle+34
tick_sched_timer+109
__hrtimer_run_queues+298
hrtimer_interrupt+262
__sysvec_apic_timer_interrupt+127
sysvec_apic_timer_interrupt+157
asm_sysvec_apic_timer_interrupt+18
native_safe_halt+11
default_idle+10
default_idle_call+50
do_idle+478
cpu_startup_entry+25
start_secondary+278
secondary_startup_64_no_verify+213
```

```txt
rcu_core_si+1
__softirqentry_text_start+238
__irq_exit_rcu+181
sysvec_apic_timer_interrupt+162
asm_sysvec_apic_timer_interrupt+18
native_safe_halt+11
default_idle+10
default_idle_call+50
do_idle+478
cpu_startup_entry+25
start_secondary+278
secondary_startup_64_no_verify+213
```

## 问题

- [ ] 思考一下，RCU 在用户态和内核态中实现的差异
  - [ ] 为什么 kernel 的实现比 userspace 的复杂那么多


## 为什么 list rcu 需要多存在一个操作

```c
/**
 * list_replace_rcu - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * The @old entry will be replaced with the @new entry atomically.
 * Note: @old should not be empty.
 */
static inline void list_replace_rcu(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->prev = old->prev;
	rcu_assign_pointer(list_next_rcu(new->prev), new);
	new->next->prev = new;
	old->prev = LIST_POISON2;
}


/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace(struct list_head *old,
				struct list_head *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}
```

- [x] 如果存在多个 updater ，也是需要上锁的吧
  - 必须上锁，例如两个
  - 作为额外的问题 : 其实多个 updater 是可以不用把整个链表锁起来的，并行编程艺术似乎分析过 🎋
- 但是更新可以 reader 和 updater

只是允许单向遍历。

## 为什么需要如此
https://github.com/WireGuard/wireguard-linux/commit/79105ecd70479b71a990463c8da343d3b4281ce9


## rcu 的第一个问题
似乎很多代码都是增加 READ_ONCE 和 WRITE_ONCE 就可以了

例如 include/linux/rculist.h
```c
/**
 * list_entry_rcu - get the struct for this entry
 * @ptr:        the &struct list_head pointer.
 * @type:       the type of the struct this is embedded in.
 * @member:     the name of the list_head within the struct.
 *
 * This primitive may safely run concurrently with the _rcu list-mutation
 * primitives such as list_add_rcu() as long as it's guarded by rcu_read_lock().
 */
#define list_entry_rcu(ptr, type, member) \
	container_of(READ_ONCE(ptr), type, member)
```

和 include/linux/list.h 这里的


## no place to store the SRCU cookie ?


如何理解 SRCU cookie ?
```c
static void *c_start(struct seq_file *m, loff_t *pos)
{
	struct console *con;
	loff_t off = 0;

	/*
	 * Hold the console_list_lock to guarantee safe traversal of the
	 * console list. SRCU cannot be used because there is no
	 * place to store the SRCU cookie.
	 */
	console_list_lock();
	for_each_console(con)
		if (off++ == *pos)
			break;

	return con;
}
```


每一个 thread 创建的时候需要一个 id ，这个 id 需要不重不漏。

似乎 atomic 的 get and inc 的操作才是最好的。

## 继续收集下这个报错
```txt
[ 1093.969122] rcu: INFO: rcu_preempt self-detected stall on CPU
[ 1093.969306] rcu:     1-...!: (8 ticks this GP) idle=01e4/1/0x4000000000000000 softirq=58209/58209 fqs=2
[ 1093.969557] rcu:     (t=197177 jiffies g=151925 q=3 ncpus=2)
[ 1093.969710] CPU: 1 PID: 1536 Comm: fio Not tainted 6.7.0-rc2-00147-gf1a09972a45a #15
[ 1093.970010] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[ 1093.970193] RIP: 0010:syscall_enter_from_user_mode+0x75/0x100
[ 1093.970362] Code: 48 05 58 3f 00 00 48 39 c3 0f 85 91 00 00 00 66 90 66 90 90 e8 3c 06 f0 fe 90 90 e8 45 07 f0 fe fb 65 48 8b 04 25 c0 dc 02 00 <48> 8b 70 08 40 f6 c6 3f 75 56 90 48 89 e8 5b 5d c3 cc cc cc cc b8
[ 1093.970867] RSP: 0018:ffffc900014cff28 EFLAGS: 00000246
[ 1093.971013] RAX: ffff888107d96a80 RBX: ffffc900014cff58 RCX: 0000000000000000
[ 1093.971212] RDX: 0000000080000000 RSI: 0000000000000000 RDI: ffffffff8239ebfb
[ 1093.971411] RBP: 00000000000000d0 R08: 0000000000000000 R09: 0000000000000000
[ 1093.971603] R10: 0000000000000000 R11: 0000000000000000 R12: 0000000000000000
[ 1093.971798] R13: 0000000000000000 R14: 0000000000000000 R15: 0000000000000000
[ 1093.971993] FS:  00007fc48bd96040(0000) GS:ffff888277d00000(0000) knlGS:0000000000000000
[ 1093.972212] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[ 1093.972375] CR2: 00007f4e54be4638 CR3: 0000000115d8a000 CR4: 0000000000750ef0
[ 1093.972575] PKRU: 55555554
[ 1093.972650] Call Trace:
[ 1093.972726]  <IRQ>
[ 1093.972786]  ? rcu_dump_cpu_stacks+0xea/0x170
[ 1093.972910]  ? rcu_sched_clock_irq+0x383/0x10f0
[ 1093.973039]  ? task_tick_fair+0x40/0x3f0
[ 1093.973164]  ? __cgroup_account_cputime_field+0x55/0x90
[ 1093.973310]  ? update_process_times+0x74/0xb0
[ 1093.973443]  ? tick_sched_handle+0x21/0x60
[ 1093.973558]  ? tick_nohz_highres_handler+0x6f/0x90
[ 1093.973690]  ? __pfx_tick_nohz_highres_handler+0x10/0x10
[ 1093.973833]  ? __hrtimer_run_queues+0x86/0x2e0
[ 1093.973955]  ? hrtimer_interrupt+0xf8/0x230
[ 1093.974072]  ? __sysvec_apic_timer_interrupt+0x4d/0x140
[ 1093.974217]  ? sysvec_apic_timer_interrupt+0x6f/0x80
[ 1093.974354]  </IRQ>
[ 1093.974416]  <TASK>
[ 1093.974478]  ? asm_sysvec_apic_timer_interrupt+0x1a/0x20
[ 1093.974629]  ? syscall_enter_from_user_mode+0x6b/0x100
[ 1093.974770]  ? syscall_enter_from_user_mode+0x75/0x100
[ 1093.974910]  ? syscall_enter_from_user_mode+0x6b/0x100
[ 1093.975050]  do_syscall_64+0x1c/0xf0
[ 1093.975152]  entry_SYSCALL_64_after_hwframe+0x6f/0x77
[ 1093.975294] RIP: 0033:0x7fc48be9548d
[ 1093.975396] Code: 5b 41 5c c3 66 0f 1f 84 00 00 00 00 00 f3 0f 1e fa 48 89 f8 48 89 f7 48 89 d6 48 89 ca 4d 89 c2 4d 89 c8 4c 8b 4c 24 08 0f 05 <48> 3d 01 f0 ff ff 73 01 c3 48 8b 0d 73 f9 0c 00 f7 d8 64 89 01 48
[ 1093.975893] RSP: 002b:00007ffdd0d6d428 EFLAGS: 00000246 ORIG_RAX: 00000000000000d0
[ 1093.976097] RAX: ffffffffffffffda RBX: 00007fc48bd95fb8 RCX: 00007fc48be9548d
[ 1093.976289] RDX: 0000000000000001 RSI: 0000000000000001 RDI: 00007fc483a78000
[ 1093.976482] RBP: 00007fc483a78000 R08: 0000000000000000 R09: 0000000000000000
[ 1093.976672] R10: 000055d661efd700 R11: 0000000000000246 R12: 0000000000000001
[ 1093.976861] R13: 0000000000000000 R14: 0000000000000001 R15: 000055d661efd700
[ 1093.977052]  </TASK>
```

## 先看看这些选项有什么作用吧

```txt
#
# RCU Debugging
#
# CONFIG_RCU_SCALE_TEST is not set
# CONFIG_RCU_TORTURE_TEST is not set
# CONFIG_RCU_REF_SCALE_TEST is not set
CONFIG_RCU_CPU_STALL_TIMEOUT=21
CONFIG_RCU_EXP_CPU_STALL_TIMEOUT=0
# CONFIG_RCU_CPU_STALL_CPUTIME is not set
CONFIG_RCU_TRACE=y
# CONFIG_RCU_EQS_DEBUG is not set
# end of RCU Debugging
```


## rcu 基本原理
http://www.wowotech.net/kernel_synchronization/223.html

写的极好，建议反复阅读。

## rcu_note_context_switch

通过上下文切换的时候，驱动 rcu :

```c
/*
 * Note a PREEMPTION=n context switch. The caller must have disabled interrupts.
 */
void rcu_note_context_switch(bool preempt)
{
	trace_rcu_utilization(TPS("Start context switch"));
	rcu_qs();
	/* Load rcu_urgent_qs before other flags. */
	if (!smp_load_acquire(this_cpu_ptr(&rcu_data.rcu_urgent_qs)))
		goto out;
	this_cpu_write(rcu_data.rcu_urgent_qs, false);
	if (unlikely(raw_cpu_read(rcu_data.rcu_need_heavy_qs)))
		rcu_momentary_eqs();
out:
	rcu_tasks_qs(current, preempt);
	trace_rcu_utilization(TPS("End context switch"));
}
```

这个是 PREEMPTION=n 的时候，如果 PREEMPTION=y 的时候，走这个函数，会复杂很多

```c
/*
 * We have entered the scheduler, and the current task might soon be
 * context-switched away from.  If this task is in an RCU read-side
 * critical section, we will no longer be able to rely on the CPU to
 * record that fact, so we enqueue the task on the blkd_tasks list.
 * The task will dequeue itself when it exits the outermost enclosing
 * RCU read-side critical section.  Therefore, the current grace period
 * cannot be permitted to complete until the blkd_tasks list entries
 * predating the current grace period drain, in other words, until
 * rnp->gp_tasks becomes NULL.
 *
 * Caller must disable interrupts.
 */
void rcu_note_context_switch(bool preempt)
{
	struct task_struct *t = current;
	struct rcu_data *rdp = this_cpu_ptr(&rcu_data);
	struct rcu_node *rnp;

	trace_rcu_utilization(TPS("Start context switch"));
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
