## sched_tick

本来是一个 CPU 来执行就可以了，但是并不是

```txt
sudo bpftrace -e 'kprobe:sched_tick { @[cpu] = count(); }'
```
同时打开，同时关闭

在虚拟机中可以观察到如下结果，虚拟机的内核自己构建的:
```txt
@[1]: 149
@[6]: 231
@[2]: 235
@[4]: 309
@[0]: 320
@[5]: 387
@[20]: 494
@[30]: 549
@[27]: 575
@[23]: 654
@[31]: 684
@[22]: 765
@[7]: 873
@[19]: 939
@[8]: 1078
@[28]: 1088
@[15]: 1104
@[25]: 1273
@[29]: 1313
@[13]: 1423
@[18]: 1500
@[11]: 1508
@[14]: 1596
@[24]: 1699
@[26]: 1784
@[10]: 1826
@[12]: 2049
@[17]: 2195
@[9]: 2318
@[21]: 2338
@[16]: 2545
@[3]: 3338
```

物理机中观测到的:
```txt
@[31]: 358
@[1]: 374
@[5]: 425
@[13]: 425
@[7]: 436
@[30]: 638
@[28]: 802
@[23]: 841
@[25: 926
@[15]: 1578
@[21]: 2102
@[22]: 2667
@[24]: 3509
@[20]: 3908
@[27]: 4751
@[26]: 4819
@[29]: 5910
@[9]: 7648
@[0]: 8294
@[3]: 8595
@[19]: 10171
@[18]: 13986
@[4]: 16888
@[17]: 17369
@[6]: 21513
@[16]: 24774
@[11]: 25023
@[2]: 25904
@[14]: 35335
@[8]: 44680
@[12]: 46134
@[10]: 55785
```

## 文摘
- Measuring mutexes, spinlocks and how bad the Linux scheduler is (probablydance.com)
  - https://news.ycombinator.com/item?id=21919988

## 看看这个的实现
https://serverfault.com/questions/235825/disable-hyperthreading-from-within-linux-no-access-to-bios

## 有必要一天到晚调用 put_task_stack 吗?
```c
void put_task_stack(struct task_struct *tsk)
{
	if (refcount_dec_and_test(&tsk->stack_refcount))
		release_task_stack(tsk);
}
```

```txt
@[
    put_task_stack+5
    finish_task_switch.isra.0+589
    __schedule+940
    schedule_idle+35
    cpu_startup_entry+41
    start_secondary+286
    common_startup_64+318
]: 152
```

例如如果打开一个中断，然后关闭，可以至少观察到这么多内容。

## enqueue_task_fair 的参数 rq 不是 cfs_rq 的

一直以为 struct rq 的参数:
```c
struct rq *rq
```

实际上获取的 cfs_rq 的方法是:

而 es 是 task_struct 中一部分
```c
/* runqueue on which this entity is (to be) queued */
static inline struct cfs_rq *cfs_rq_of(const struct sched_entity *se)
{
	return se->cfs_rq;
}
```

```c
/* CFS-related fields in a runqueue */
struct cfs_rq {
	struct rq		*rq;	/* CPU runqueue to which this cfs_rq is attached */
```

```txt
@[
    enqueue_task_fair+5
    enqueue_task+55
    ttwu_do_activate+111
    try_to_wake_up+708
    hrtimer_wakeup+34
    __hrtimer_run_queues+332
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
]: 1421
```

struct rq 中各个 cpu 的 scheduler :
```c
struct rq {
  // ...
	struct cfs_rq		cfs;
	struct rt_rq		rt;
	struct dl_rq		dl;
#ifdef CONFIG_SCHED_CLASS_EXT
	struct scx_rq		scx;
#endif
```
所以，其实简单，一个 CPU 一个 rq ，一个 rq 携带所有的信息，在

在 try_to_wake_up 中的关键问题，到底在哪一个 CPU 上 wake up ，
- cpu = select_task_rq(p, p->wake_cpu, &wake_flags);
- ttwu_queue(p, cpu, wake_flags);
  - ttwu_do_activate
    - activate_task
      - sched_mm_cid_migrate_to : 先需要迁移过去
      - enqueue_task

## CONFIG_SCHED_CORE 似乎是用于 hyper thread 的技术

```txt
config SCHED_CORE
	bool "Core Scheduling for SMT"
	depends on SCHED_SMT
	help
	  This option permits Core Scheduling, a means of coordinated task
	  selection across SMT siblings. When enabled -- see
	  prctl(PR_SCHED_CORE) -- task selection ensures that all SMT siblings
	  will execute a task from the same 'core group', forcing idle when no
	  matching task is found.

	  Use of this feature includes:
	   - mitigation of some (not all) SMT side channels;
	   - limiting SMT interference to improve determinism and/or performance.

	  SCHED_CORE is default disabled. When it is enabled and unused,
	  which is the likely usage by Linux distributions, there should
	  be no measurable impact on performance.
```


## sched_setattr 的参数 flags

```txt
       sched_flags
              This field contains zero or more of the following flags that are
              ORed together to control scheduling behavior:

              SCHED_FLAG_RESET_ON_FORK
                     Children  created  by  fork(2)  do not inherit privileged
                     scheduling policies.  See sched(7) for details.

              SCHED_FLAG_RECLAIM (since Linux 4.13)
                     This flag allows a SCHED_DEADLINE thread to reclaim band‐
                     width unused by other real-time threads.

              SCHED_FLAG_DL_OVERRUN (since Linux 4.16)
                     This flag allows an application  to  get  informed  about
                     run-time  overruns in SCHED_DEADLINE threads.  Such over‐
                     runs may be caused by (for example) coarse execution time
                     accounting or incorrect parameter assignment.   Notifica‐
                     tion  takes  the form of a SIGXCPU signal which is gener‐
                     ated on each overrun.

                     This SIGXCPU signal is process-directed  (see  signal(7))
                     rather than thread-directed.  This is probably a bug.  On
                     the one hand, sched_setattr() is being used to set a per-
                     thread  attribute.  On the other hand, if the process-di‐
                     rected signal is delivered to a thread inside the process
                     other than the one that had a run-time overrun,  the  ap‐
                     plication has no way of knowing which thread overran.

              SCHED_FLAG_UTIL_CLAMP_MIN
              SCHED_FLAG_UTIL_CLAMP_MAX (both since Linux 5.3)
                     These   flags   indicate   that   the  sched_util_min  or
                     sched_util_max fields, respectively, are present,  repre‐
                     senting  the  expected minimum and maximum utilization of
                     the thread.

                     The utilization attributes  provide  the  scheduler  with
                     boundaries  within  which  it should schedule the thread,
                     potentially informing its decisions regarding task place‐
                     ment and frequency selection.
```

## deadline server 是什么?
https://lore.kernel.org/all/4968601859d920335cf85822eb573a5f179f04b8.1699095159.git.bristot@kernel.org/T/#u

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
