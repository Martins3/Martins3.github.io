## https://github.com/sched-ext/scx

- 这个也是其中一个作者写的: https://news.ycombinator.com/item?id=39442400

### 先试试这个
https://github.com/parttimenerd/minimal-scheduler

```txt
[ 9178.179103] sched_ext: "minimal_scheduler" does not implement cgroup cpu.weight
[ 9178.180274] sched_ext: scx_bpf_consume() renamed to scx_bpf_dsq_move_to_local()
[ 9178.180275] sched_ext: scx_bpf_consume() renamed to scx_bpf_dsq_move_to_local()
[ 9178.180275] sched_ext: scx_bpf_consume() renamed to scx_bpf_dsq_move_to_local()
[ 9178.180275] sched_ext: scx_bpf_consume() renamed to scx_bpf_dsq_move_to_local()
[ 9178.180275] sched_ext: scx_bpf_consume() renamed to scx_bpf_dsq_move_to_local()
[ 9178.180275] sched_ext: scx_bpf_consume() renamed to scx_bpf_dsq_move_to_local()
[ 9178.180275] sched_ext: scx_bpf_consume() renamed to scx_bpf_dsq_move_to_local()
[ 9178.180275] sched_ext: scx_bpf_consume() renamed to scx_bpf_dsq_move_to_local()
[ 9178.180450] sched_ext: BPF scheduler "minimal_scheduler" enabled
```
似乎有两个警告，这个代码更新一下:

### 是不是使用了

#### 测试

的确，在 start.sh 之后，可以观察到很多这个:
```txt
@[
    scx_pick_idle_cpu+5
    scx_select_cpu_dfl+445
    select_task_rq_scx+300
    try_to_wake_up+445
    wake_up_q+78
    futex_wake+345
    do_futex+293
    __x64_sys_futex+297
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 42973
@[
    scx_select_cpu_dfl+5
    select_task_rq_scx+300
    try_to_wake_up+445
    wake_up_q+78
    futex_wake+345
    do_futex+293
    __x64_sys_futex+297
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1618732
@[
    scx_bpf_dsq_move_to_local+9
    bpf_prog_3768393fc527e957_sched_dispatch+27
    bpf__sched_ext_ops_dispatch+75
    balance_one+334
    balance_scx+53
    prev_balance+67
    __pick_next_task+107
    __schedule+362
    schedule+65
    futex_wait_queue+101
    __futex_wait+334
    futex_wait+121
    do_futex+203
    __x64_sys_futex+297
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 1618967
```

关闭之后，就仅仅可以看到这个:
```txt
@[
    scx_tick+9
    sched_tick+221
    update_process_times+150
    tick_nohz_handler+143
    __hrtimer_run_queues+133
    hrtimer_interrupt+255
    __sysvec_apic_timer_interrupt+82
    sysvec_apic_timer_interrupt+110
    asm_sysvec_apic_timer_interrupt+26
    default_idle+15
    default_idle_call+63
    do_idle+463
    cpu_startup_entry+41
    start_secondary+286
    common_startup_64+318
]: 7932
```
#### 具体代码

- sched_fork

难道真的是这个影响的吗？
```c
struct task_struct {
  // ...
	const struct sched_class	*sched_class;
```

似乎是的，例如看这个例子:

例如 enqueue_task 中
```c
void enqueue_task(struct rq *rq, struct task_struct *p, int flags)
{
	if (!(flags & ENQUEUE_NOCLOCK))
		update_rq_clock(rq);

	p->sched_class->enqueue_task(rq, p, flags);
	/*
	 * Must be after ->enqueue_task() because ENQUEUE_DELAYED can clear
	 * ->sched_delayed.
	 */
	uclamp_rq_inc(rq, p);

	psi_enqueue(p, flags);

	if (!(flags & ENQUEUE_RESTORE))
		sched_info_enqueue(rq, p);

	if (sched_core_enabled(rq))
		sched_core_enqueue(rq, p);
}
```
当被 wake up 的时候，总是需要先加入到队列中，然后从队列中离开:

```txt
@[
    dequeue_entity+1
    dequeue_entities+289
    dequeue_task_fair+151
    __schedule+1940
    schedule+65
    futex_wait_queue+101
    __futex_wait+334
    futex_wait+121
    do_futex+203
    __x64_sys_futex+297
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 279
```

在中断的时候加入到队列中:
```txt
@[
    enqueue_task_fair+5
    enqueue_task+55
    ttwu_do_activate+111
    sched_ttwu_pending+245
    __flush_smp_call_function_queue+320
    __sysvec_call_function_single+28
    sysvec_call_function_single+110
    asm_sysvec_call_function_single+26
    default_idle+15
@[
    enqueue_task_fair+5
    enqueue_task+55
    ttwu_do_activate+111
    sched_ttwu_pending+245
    __flush_smp_call_function_queue+320
    __sysvec_call_function_single+28
    sysvec_call_function_single+110
    asm_sysvec_call_function_single+26
    default_idle+15
    default_idle_call+63
    do_idle+463
    cpu_startup_entry+41
    start_secondary+286
    common_startup_64+318
]: 1878
```

## 看看 sysfs 的结果

```txt
/sys/kernel/sched_ext🔒 🐕
🧀  tree
.
├── enable_seq
├── hotplug_seq
├── nr_rejected
├── root
│   └── ops
├── state
└── switch_all

2 directories, 6 files
```


### tools/sched_ext/ 是做什么的?
kernel 中的 tools/sched_ext/ 做什么的?

## 当时使用了 ebpf 吗?
https://github.com/google/ghost-userspace

## SCHED_EXT
https://mp.weixin.qq.com/s/89PuLJDE4aE1c3cWG6ZL8g

- include/linux/sched/ext.h  // ext 调度类的ops接口声明以及调度类核心结构体实现
- kernel/sched/ext.h         // ext 调度类的普通功能接口及标志位声明
- kernel/sched/ext.c         // ext 调度类的核心接口实现

- tools/sched_ext/scx_central.c   // central scheduler 的加载注册及监控部分实现
- tools/sched_ext/scx_central.bpf.c // central scheduler 的具体策略实现

## 获取一点代码的基本感觉

基本的感觉是在这里的:

### DEFINE_SCHED_CLASS : 所有的都是有的
```c
DEFINE_SCHED_CLASS(ext) = {
	.enqueue_task		= enqueue_task_scx,
	.dequeue_task		= dequeue_task_scx,
	.yield_task		= yield_task_scx,
	.yield_to_task		= yield_to_task_scx,

	.wakeup_preempt		= wakeup_preempt_scx,

	.balance		= balance_scx,
	.pick_task		= pick_task_scx,

	.put_prev_task		= put_prev_task_scx,
	.set_next_task		= set_next_task_scx,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_scx,
	.task_woken		= task_woken_scx,
	.set_cpus_allowed	= set_cpus_allowed_scx,

	.rq_online		= rq_online_scx,
	.rq_offline		= rq_offline_scx,
#endif

	.task_tick		= task_tick_scx,

	.switching_to		= switching_to_scx,
	.switched_from		= switched_from_scx,
	.switched_to		= switched_to_scx,
	.reweight_task		= reweight_task_scx,
	.prio_changed		= prio_changed_scx,

	.update_curr		= update_curr_scx,

#ifdef CONFIG_UCLAMP_TASK
	.uclamp_enabled		= 1,
#endif
};
```

### sched_ext_ops : 对于用户提供的 scheduler
```c
static struct sched_ext_ops __bpf_ops_sched_ext_ops = {
	.select_cpu		= sched_ext_ops__select_cpu,
	.enqueue		= sched_ext_ops__enqueue,
	.dequeue		= sched_ext_ops__dequeue,
	.dispatch		= sched_ext_ops__dispatch,
	.tick			= sched_ext_ops__tick,
	.runnable		= sched_ext_ops__runnable,
	.running		= sched_ext_ops__running,
	.stopping		= sched_ext_ops__stopping,
	.quiescent		= sched_ext_ops__quiescent,
	.yield			= sched_ext_ops__yield,
	.core_sched_before	= sched_ext_ops__core_sched_before,
	.set_weight		= sched_ext_ops__set_weight,
	.set_cpumask		= sched_ext_ops__set_cpumask,
	.update_idle		= sched_ext_ops__update_idle,
	.cpu_acquire		= sched_ext_ops__cpu_acquire,
	.cpu_release		= sched_ext_ops__cpu_release,
	.init_task		= sched_ext_ops__init_task,
	.exit_task		= sched_ext_ops__exit_task,
	.enable			= sched_ext_ops__enable,
	.disable		= sched_ext_ops__disable,
#ifdef CONFIG_EXT_GROUP_SCHED
	.cgroup_init		= sched_ext_ops__cgroup_init,
	.cgroup_exit		= sched_ext_ops__cgroup_exit,
	.cgroup_prep_move	= sched_ext_ops__cgroup_prep_move,
	.cgroup_move		= sched_ext_ops__cgroup_move,
	.cgroup_cancel_move	= sched_ext_ops__cgroup_cancel_move,
	.cgroup_set_weight	= sched_ext_ops__cgroup_set_weight,
#endif
	.cpu_online		= sched_ext_ops__cpu_online,
	.cpu_offline		= sched_ext_ops__cpu_offline,
	.init			= sched_ext_ops__init,
	.exit			= sched_ext_ops__exit,
	.dump			= sched_ext_ops__dump,
	.dump_cpu		= sched_ext_ops__dump_cpu,
	.dump_task		= sched_ext_ops__dump_task,
};
```

## 为什么有 kthread 程序在不断的调整自己的 level
```txt
#0  __sched_setscheduler (p=p@entry=0xffff888004393280,
    attr=0xffffc9000013beb8, user=user@entry=false, pi=pi@entry=true)
    at kernel/sched/syscalls.c:529
#1  0xffffffff8112d92b in _sched_setscheduler (
    param=0xffffffff8220eb00 <param>, check=false,
    policy=<optimized out>, p=0xffff888004393280)
    at kernel/sched/syscalls.c:788
#2  sched_setscheduler_nocheck (p=p@entry=0xffff888004393280,
    policy=policy@entry=0,
    param=param@entry=0xffffffff8220eb00 <param>)
    at kernel/sched/syscalls.c:835
#3  0xffffffff810dacb1 in kthread (_create=0xffff888004255f80)
    at ./arch/x86/include/asm/current.h:47
#4  0xffffffff810487b1 in ret_from_fork (prev=<optimized out>,
    regs=0xffffc9000013bf58, fn=0xffffffff810dac50 <kthread>,
    fn_arg=0xffff888004255f80) at arch/x86/kernel/process.c:147
#5  0xffffffff810036ea in ret_from_fork_asm ()
    at arch/x86/entry/entry_64.S:244
```

__sched_setscheduler 中在不断在判断 user 这个变量。

此外，如何理解: __sched_setscheduler 中
man sched_setscheduler(2)
```txt
	/*
	 * Valid priorities for SCHED_FIFO and SCHED_RR are
	 * 1..MAX_RT_PRIO-1, valid priority for SCHED_NORMAL,
	 * SCHED_BATCH and SCHED_IDLE is 0.
	 */
	if (attr->sched_priority > MAX_RT_PRIO-1)
		return -EINVAL;
	if ((dl_policy(policy) && !__checkparam_dl(attr)) ||
	    (rt_policy(policy) != (attr->sched_priority != 0)))
		return -EINVAL;
```

```txt
Currently,  Linux  supports  the following "normal" (i.e., non-
real-time) scheduling policies as values that may be  specified
in policy:

SCHED_OTHER   the standard round-robin time-sharing policy;

SCHED_BATCH   for "batch" style execution of processes; and

SCHED_IDLE    for running very low priority background jobs.

For  each  of the above policies, param->sched_priority must be 0.
```

## 看看
https://mp.weixin.qq.com/s/d043Be7vfXaq3HFNxUdfLw

https://mp.weixin.qq.com/s/B6zeBR-h-vLgTBe5CwGTSg


https://mp.weixin.qq.com/s/SE1W3IbYP8jeNX5rUDCaGg

https://mp.weixin.qq.com/s/WAPL5h4pigZ3m0-QMi-c0A

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
