## 问题
- 为什么 v2 将 cpuacct 取消了

1. 如何确定将哪一个 thread 加入到哪一个 group ?
2. 创建 thread group 的创建的时机是什么 ?
3. thread group 让整个 reb tree 如何构建 ?
4. 一个 thread group 会不会对于另一个 thread group 含有优先级 ?
5. 是不是一旦配置了 tg 那么就所有的 thread 都必须属于某一个 group 中间 ?

- [ ] bandwidth 和 share 如何协同工作

## 结论
- task_group 持有 sched_entity 和 cfs_rq，通过 cfs_rq 可以重新构建该 group 的红黑树，而这个 rbtree 将会是 parent 的一个 node

### 基本操作
- list_add_leaf_cfs_rq
- list_add_leaf_cfs_rq
- [ ] init_tg_cfs_entry

### CONFIG_CFS_BANDWIDTH

参考资料:
- https://www.kernel.org/doc/Documentation/scheduler/sched-bwc.txt

> CFS bandwidth control is a `CONFIG_FAIR_GROUP_SCHED` extension which allows the
> specification of the maximum CPU bandwidth available to a group or hierarchy.
>
> The bandwidth allowed for a group is specified using a quota and period. Within
> each given "period" (microseconds), a group is allowed to consume only up to
> "quota" microseconds of CPU time.  When the CPU bandwidth consumption of a
> group exceeds this limit (for that period), the tasks belonging to its
> hierarchy will be throttled and are not allowed to run again until the next
> period.

总结到位。

## 创建
- sudo cgcreate -g cpu:A

```txt
#0  alloc_fair_sched_group (tg=tg@entry=0xffff888142e18000, parent=parent@entry=0xffffffff834a2000 <root_task_group>) at include/linux/slab.h:640
#1  0xffffffff8113e15a in sched_create_group (parent=0xffffffff834a2000 <root_task_group>) at kernel/sched/core.c:10097
#2  0xffffffff8113e1ca in cpu_cgroup_css_alloc (parent_css=<optimized out>) at kernel/sched/core.c:10246
#3  0xffffffff811ba029 in css_create (ss=0xffffffff82a63c00 <cpu_cgrp_subsys>, cgrp=0xffff888109618800) at kernel/cgroup/cgroup.c:5384
#4  cgroup_apply_control_enable (cgrp=cgrp@entry=0xffff888109618800) at kernel/cgroup/cgroup.c:3204
#5  0xffffffff811bc1ef in cgroup_mkdir (parent_kn=0xffff888141a0c980, name=<optimized out>, mode=<optimized out>) at kernel/cgroup/cgroup.c:5602
#6  0xffffffff813e2d29 in kernfs_iop_mkdir (mnt_userns=<optimized out>, dir=<optimized out>, dentry=<optimized out>, mode=<optimized out>) at fs/kernfs/dir.c:1185
#7  0xffffffff81359dbf in vfs_mkdir (mnt_userns=0xffffffff82a61a80 <init_user_ns>, dir=0xffff8881408c2490, dentry=dentry@entry=0xffff888148895c00, mode=<optimized out>, mode@entry=509) at fs/namei.c:4013
#8  0xffffffff8135ebc1 in do_mkdirat (dfd=dfd@entry=-100, name=0xffff888004103000, mode=mode@entry=509) at fs/namei.c:4038
#9  0xffffffff8135edb3 in __do_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4058
#10 __se_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4056
#11 __x64_sys_mkdir (regs=<optimized out>) at fs/namei.c:4056
#12 0xffffffff81f3356b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900015d3f58) at arch/x86/entry/common.c:50
#13 do_syscall_64 (regs=0xffffc900015d3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#14 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

在 sudo su 的时候，调用 setsid 的时候切换:

```txt
#0  alloc_fair_sched_group (tg=tg@entry=0xffff8880040c3c00, parent=parent@entry=0xffffffff834af000 <root_task_group>) at include/linux/slab.h:640
#1  0xffffffff811410de in sched_create_group (parent=0xffffffff834af000 <root_task_group>) at kernel/sched/core.c:10097
#2  0xffffffff811596b6 in autogroup_create () at kernel/sched/build_utility.c:10575
#3  sched_autogroup_create_attach (p=p@entry=0xffff888144d0bd00) at kernel/sched/build_utility.c:10676
#4  0xffffffff8111ee71 in ksys_setsid () at kernel/sys.c:1234
#5  0xffffffff8111eea5 in __do_sys_setsid (__unused=<optimized out>) at kernel/sys.c:1241
#6  0xffffffff81f3d6eb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90002abff58) at arch/x86/entry/common.c:50
#7  do_syscall_64 (regs=0xffffc90002abff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#8  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

- sched_online_group
- sched_offline_group

## 设置，以 cpu.max 为例

当 echo "10000 100000"  > cpu.max 之后，

```sh
sudo cgexec -g cpu:C dd if=/dev/zero of=/dev/null &
```

然后使用 top 去观测，发现 CPU 使用量就是 10%

- tg_set_cfs_bandwidth 进一步调用 `__cfs_schedulable`

- tg_cfs_scheduable_down　// 放到 walk_tg_tree_from 的参数吧!


```txt
#0  tg_set_cfs_bandwidth (tg=tg@entry=0xffff888104f1fc00, period=1000000, quota=18446744073709551615, burst=0, burst@entry=<error reading variable: That operation is not available on integers of more than 8 bytes.>) at kernel/sched/core.c:10558
#1  0xffffffff81138def in cpu_max_write (of=<optimized out>, buf=0xffff8881491cd180 "max 1000\n", nbytes=9, off=<optimized out>) at kernel/sched/core.c:11110
#2  0xffffffff813ee51e in kernfs_fop_write_iter (iocb=0xffffc90000fbbea0, iter=<optimized out>) at fs/kernfs/file.c:354
#3  0xffffffff813559cc in call_write_iter (iter=0xffffc90000fbbe78, kio=0xffffc90000fbbea0, file=0xffff88814b5c0c00) at include/linux/fs.h:2187
#4  new_sync_write (ppos=0xffffc90000fbbf08, len=9, buf=0x55ded7ad60c0 "max 1000\n", filp=0xffff88814b5c0c00) at fs/read_write.c:491
#5  vfs_write (file=file@entry=0xffff88814b5c0c00, buf=buf@entry=0x55ded7ad60c0 "max 1000\n", count=count@entry=9, pos=pos@entry=0xffffc90000fbbf08) at fs/read_write.c:578
#6  0xffffffff81355d9a in ksys_write (fd=<optimized out>, buf=0x55ded7ad60c0 "max 1000\n", count=9) at fs/read_write.c:631
#7  0xffffffff81f3d6eb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000fbbf58) at arch/x86/entry/common.c:50
#8  do_syscall_64 (regs=0xffffc90000fbbf58, nr=<optimized out>) at arch/x86/entry/common.c:80
```

这里在同时分析两个数值，period 和 quota

```c
struct cfs_bandwidth {
    // ...
    ktime_t         period;
    u64         quota;
```

关于 period

- start_cfs_bandwidth : 重置 period 的时钟

```txt
#0  start_cfs_bandwidth (cfs_b=0xffff888141e9a4c8) at kernel/sched/fair.c:5510
#1  __assign_cfs_rq_runtime (target_runtime=5000000, cfs_rq=0xffff8881439c5600, cfs_b=0xffff888141e9a4c8) at kernel/sched/fair.c:4856
#2  assign_cfs_rq_runtime (cfs_rq=0xffff8881439c5600) at kernel/sched/fair.c:4877
#3  __account_cfs_rq_runtime (cfs_rq=0xffff8881439c5600, delta_exec=<optimized out>) at kernel/sched/fair.c:4897
#4  0xffffffff81145737 in entity_tick (queued=0, curr=0xffff888151b6af00, cfs_rq=0xffff8881439c5600) at kernel/sched/fair.c:4735
#5  task_tick_fair (rq=0xffff888333c2b2c0, curr=0xffff888151b6ae80, queued=0) at kernel/sched/fair.c:11416
#6  0xffffffff8113f392 in scheduler_tick () at kernel/sched/core.c:5453
#7  0xffffffff8119b2b1 in update_process_times (user_tick=0) at kernel/time/timer.c:1844
#8  0xffffffff811ad85f in tick_sched_handle (ts=ts@entry=0xffff888333c1e5c0, regs=regs@entry=0xffffc9000098be88) at kernel/time/tick-sched.c:243
#9  0xffffffff811ada3c in tick_sched_timer (timer=0xffff888333c1e5c0) at kernel/time/tick-sched.c:1480
#10 0xffffffff8119bde2 in __run_hrtimer (flags=6, now=0xffffc90000003f48, timer=0xffff888333c1e5c0, base=0xffff888333c1e0c0, cpu_base=0xffff888333c1e080) at kernel/time/hrtimer.c:1685
#11 __hrtimer_run_queues (cpu_base=cpu_base@entry=0xffff888333c1e080, now=3236401072342, flags=flags@entry=6, active_mask=active_mask@entry=15) at kernel/time/hrtimer.c:1749
#12 0xffffffff8119ca71 in hrtimer_interrupt (dev=<optimized out>) at kernel/time/hrtimer.c:1811
#13 0xffffffff810e25d7 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1095
#14 __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1112
#15 0xffffffff81f4137d in sysvec_apic_timer_interrupt (regs=0xffffc9000098be88) at arch/x86/kernel/apic/apic.c:1106
```

period 时钟注册的 hook : sched_cfs_period_timer -> do_sched_cfs_period_timer -> `__refill_cfs_bandwidth_runtime` / distribute_cfs_runtime

如何形成约束:
- `update_curr` -> `account_cfs_rq_runtime` 中需要更新统计时间。
- enqueue_entity -> check_enqueue_throttle => throttle_cfs_rq 在其中的检查是层次的性的。

### CONFIG_FAIR_GROUP_SCHED

## 设置

```txt
#0  __sched_group_set_shares (tg=0xffff888141e9a300, shares=104448) at kernel/sched/fair.c:11845
#1  0xffffffff8114be9a in sched_group_set_shares (tg=0xffff888141e9a300, shares=104448) at kernel/sched/fair.c:11880
#2  0xffffffff811bb84b in cgroup_file_write (of=<optimized out>, buf=0xffff8881547244a0 "10\n", nbytes=3, off=<optimized out>) at kernel/cgroup/cgroup.c:3983
#3  0xffffffff813ee51b in kernfs_fop_write_iter (iocb=0xffffc9000099bea0, iter=<optimized out>) at fs/kernfs/file.c:354
#4  0xffffffff813559c9 in call_write_iter (iter=0x19800 <bts_ctx+10240>, kio=0xffff888141e9a300, file=0xffff888155f80e00) at include/linux/fs.h:2187
#5  new_sync_write (ppos=0xffffc9000099bf08, len=3, buf=0x5588fa0d60c0 "10\n", filp=0xffff888155f80e00) at fs/read_write.c:491
#6  vfs_write (file=file@entry=0xffff888155f80e00, buf=buf@entry=0x5588fa0d60c0 "10\n", count=count@entry=3, pos=pos@entry=0xffffc9000099bf08) at fs/read_write.c:578
#7  0xffffffff81355d9a in ksys_write (fd=<optimized out>, buf=0x5588fa0d60c0 "10\n", count=3) at fs/read_write.c:631
#8  0xffffffff81f3d6e8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9000099bf58) at arch/x86/entry/common.c:50
#9  do_syscall_64 (regs=0xffffc9000099bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
```

应该是将 weight 更新，从而导致让 weight 小的使用量更小。

## cfs_bandwidth::period_timer 和 cfs_bandwidth::slack_timer

start_cfs_bandwidth : 似乎是开始计时的函数。

slack_timer 的作用似乎是: 用于计算没有用完的时间的。
```c
static void dequeue_entity(struct cfs_rq *cfs_rq, struct sched_entity *se, int flags)
    static __always_inline void return_cfs_rq_runtime(struct cfs_rq *cfs_rq)
        /* we know any runtime found here is valid as update_curr() precedes return */
        static void __return_cfs_rq_runtime(struct cfs_rq *cfs_rq)
            static void start_cfs_slack_bandwidth(struct cfs_bandwidth *cfs_b)
```

## unthrottle_cfs_rq 和 throttle_cfs_rq 利用上 timer 机制

```c
// 这就是两个检查是否超过 bandwidth 的时机:
check_enqueue_throttle : 被 enqueue_entity 唯一处理
check_cfs_rq_runtime
  static void throttle_cfs_rq(struct cfs_rq *cfs_rq) // todo 观察其中，就可以知道到底如何实现控制 bandwidth


void unthrottle_cfs_rq(struct cfs_rq *cfs_rq)
```

![](../../img/source/check_cfs_rq_runtime.png)

> put_prev_entity 为什么需要 check_cfs_rq_runtime ? 都已经离开队列了，为什么还是需要处理 ?

![](../../img/source/unthrottle_cfs_rq.png)


分析一下:
```c
/*
 * Responsible for refilling a task_group's bandwidth and unthrottling its
 * cfs_rqs as appropriate. If there has been no activity within the last
 * period the timer is deactivated until scheduling resumes; cfs_b->idle is
 * used to track this state.
 */
static int do_sched_cfs_period_timer(struct cfs_bandwidth *cfs_b, int overrun)


/*
 * This is done with a timer (instead of inline with bandwidth return) since
 * it's necessary to juggle rq->locks to unthrottle their respective cfs_rqs.
 */
static void do_sched_cfs_slack_timer(struct cfs_bandwidth *cfs_b)


static enum hrtimer_restart sched_cfs_period_timer(struct hrtimer *timer)
static enum hrtimer_restart sched_cfs_slack_timer(struct hrtimer *timer)

void init_cfs_bandwidth(struct cfs_bandwidth *cfs_b) // 上述函数注册的位置
```

## 需要分析的一些细节

cfs_bandwith_used() 简单的辅助函数，用于开关 bandwidth 机制。
和 `void cfs_bandwidth_usage_inc(void)` 和 `void cfs_bandwidth_usage_dec(void)` 配合使用。

## tg_set_cfs_

调用者 : 都是来自于 cgroup 机制的
- tg_set_cfs_cpu
- tg_set_cfs_period
- cpu_max_write

## rq_offline_fair 和 rq_online_fair 的作用是什么

online 和 offline 表示 cpu 的添加和去除。

```c
static void rq_online_fair(struct rq *rq)
{
    update_sysctl();

    update_runtime_enabled(rq);
}
```

利用 此处的 git blame 可以找到当时添加此函数的原因是什么东西 ?

# Reading notes from : http://www.wowotech.net/

## CFS 调度器（3）-组调度 : http://www.wowotech.net/process_management/449.html

> task_group.shares : 调度实体有权重的概念，以权重的比例分配 CPU 时间。用户组同样有权重的概念，share 就是 task_group 的权重。
> task_group.load_avg : 整个用户组的负载贡献总和。


![](http://www-x-wowotech-x-net.img.abc188.com/content/uploadfile/201811/1f8a1541854243.png)

在结合 sched_init 中间的代码，可以完全的确定:
```c
    for_each_possible_cpu(i) {
        struct rq *rq;

        rq = cpu_rq(i);
        raw_spin_lock_init(&rq->lock);
        rq->nr_running = 0;
        rq->calc_load_active = 0;
        rq->calc_load_update = jiffies + LOAD_FREQ;
        init_cfs_rq(&rq->cfs);
        init_rt_rq(&rq->rt);
        init_dl_rq(&rq->dl);
#ifdef CONFIG_FAIR_GROUP_SCHED
        root_task_group.shares = ROOT_TASK_GROUP_LOAD;
        INIT_LIST_HEAD(&rq->leaf_cfs_rq_list);
        rq->tmp_alone_branch = &rq->leaf_cfs_rq_list;
        /*
         * How much CPU bandwidth does root_task_group get?
         *
         * In case of task-groups formed thr' the cgroup filesystem, it
         * gets 100% of the CPU resources in the system. This overall
         * system CPU resource is divided among the tasks of
         * root_task_group and its child task-groups in a fair manner,
         * based on each entity's (task or task-group's) weight
         * (se->load.weight).
         *
         * In other words, if root_task_group has 10 tasks of weight
         * 1024) and two child groups A0 and A1 (of weight 1024 each),
         * then A0's share of the CPU resource is:
         *
         *  A0's bandwidth = 1024 / (10*1024 + 1024 + 1024) = 8.33%
         *
         * We achieve this by letting root_task_group's tasks sit
         * directly in rq->cfs (i.e root_task_group->se[] = NULL).
         */
        init_cfs_bandwidth(&root_task_group.cfs_bandwidth);
        init_tg_cfs_entry(&root_task_group, &rq->cfs, NULL, i, NULL);
#endif /* CONFIG_FAIR_GROUP_SCHED */
```

1. root_task_group 中间就是和 `rq->cfs_rq` 相对应的
2. `rq->cfs_rq` 也可以放 普通的 entities
3. `void init_tg_cfs_entry(struct task_group *tg, struct cfs_rq *cfs_rq, struct sched_entity *se, int cpu, struct sched_entity *parent)` 两个使用的位置是很清晰的。



o
```c
/*
 * scheduler tick hitting a task of our scheduling class.
 *
 * NOTE: This function can be called remotely by the tick offload that
 * goes along full dynticks. Therefore no local assumption can be made
 * and everything must be accessed through the @rq and @curr passed in
 * parameters.
 */
static void task_tick_fair(struct rq *rq, struct task_struct *curr, int queued)
{
    struct cfs_rq *cfs_rq;
    struct sched_entity *se = &curr->se;

    for_each_sched_entity(se) {
        cfs_rq = cfs_rq_of(se);
        entity_tick(cfs_rq, se, queued);
    }

    if (static_branch_unlikely(&sched_numa_balancing))
        task_tick_numa(rq, curr);
}

static void
entity_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr, int queued)
{
    /*
     * Update run-time statistics of the 'current'.
     */
    update_curr(cfs_rq);

    /*
     * Ensure that runnable average is periodically updated.
     */
    update_load_avg(cfs_rq, curr, UPDATE_TG);
    update_cfs_group(curr);

#ifdef CONFIG_SCHED_HRTICK
    /*
     * queued ticks are scheduled to match the slice, so don't bother
     * validating it and just reschedule.
     */
    if (queued) {
        resched_curr(rq_of(cfs_rq));
        return;
    }
    /*
     * don't let the period tick interfere with the hrtick preemption
     */
    if (!sched_feat(DOUBLE_TICK) &&
            hrtimer_active(&rq_of(cfs_rq)->hrtick_timer))
        return;
#endif

    if (cfs_rq->nr_running > 1)
        check_preempt_tick(cfs_rq, curr);
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void
check_preempt_tick(struct cfs_rq *cfs_rq, struct sched_entity *curr)
{
    unsigned long ideal_runtime, delta_exec;
    struct sched_entity *se;
    s64 delta;

    ideal_runtime = sched_slice(cfs_rq, curr); // sched_slice 分析可以运行的时间
    delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
    if (delta_exec > ideal_runtime) {
        resched_curr(rq_of(cfs_rq));
        /*
         * The current task ran long enough, ensure it doesn't get
         * re-elected due to buddy favours.
         */
        clear_buddies(cfs_rq, curr);
        return;
    }

    /*
     * Ensure that a task that missed wakeup preemption by a
     * narrow margin doesn't have to wait for a full slice.
     * This also mitigates buddy induced latencies under load.
     */
    if (delta_exec < sysctl_sched_min_granularity)
        return;

    se = __pick_first_entity(cfs_rq);
    delta = curr->vruntime - se->vruntime;

    if (delta < 0)
        return;

    if (delta > ideal_runtime)
        resched_curr(rq_of(cfs_rq));
}
```

所有的计算 share load_weight 之类的各种蛇皮，
一是为了确定 vruntime 时间是不是超过，
二是进行 CPU 迁移。

@todo 但是实际上，sched_slice 的计算仅仅利用了 cfs_rq.load 以及 se.load 进行比例计算而已。
和 avg 之类的没有任何关系，也没有处理其中的，而且和 task_group 没有任何关系 ?

> entity_tick()函数继续调用 check_preempt_tick()函数，*这部分在之前的文章已经说过了。*
> check_preempt_tick()函数会根据满足抢占当前进程的条件下设置 TIF_NEED_RESCHED 标志位。
> 满足抢占条件也很简单，只要顺着`se->parent`这条链表遍历下去，如果有一个 se 运行时间超过分配限额时间就需要重新调度。

只要自己所在的任何一个级别中间出现了超过时间，那么就将自己自己发清理。

![](http://www-x-wowotech-x-net.img.abc188.com/content/uploadfile/201811/a0011541854246.png)

利用 share 确定其运行的时间量，通过其 share 进而得到其中的 group 的 se 的权重. share 并不是一个动态刷新的过程，而是当 task group 发生变化的时候才会发生变化的。

所以 reweight_entity 需要刷新整个所有的 subtree 中间的内容吗 ?

最后分析一波 calc_group_shares 中间:
1. calc_group_shares 使用位置是唯一的，就是在 update_cfs_group 中间的
2. 然后在 reweight_entity 中间实现直接赋值给 se.load (此处的 se 为 group se)
3. calc_group_shares 根本不是计算 share 的，而是计算 group se 对应的 load
  1. group se 的 load.weight 可以用于分析计算 time_slice
  2. 所以 share 是如何计算出来的 ? (除了，core.c 中间的来自于 cgroup 的设置，就是)

```c
// 就是简单的对于 tg.shares 进行赋值
// 然后传播一下影响:
int sched_group_set_shares(struct task_group *tg, unsigned long shares)
{
    int i;

    /*
     * We can't change the weight of the root cgroup.
     */
    if (!tg->se[0])
        return -EINVAL;

    shares = clamp(shares, scale_load(MIN_SHARES), scale_load(MAX_SHARES));

    mutex_lock(&shares_mutex);
    if (tg->shares == shares)
        goto done;

    tg->shares = shares;
    for_each_possible_cpu(i) {
        struct rq *rq = cpu_rq(i);
        struct sched_entity *se = tg->se[i];
        struct rq_flags rf;

        /* Propagate contribution to hierarchy */
        rq_lock_irqsave(rq, &rf);
        update_rq_clock(rq);
        for_each_sched_entity(se) {
            update_load_avg(cfs_rq_of(se), se, UPDATE_TG);
            update_cfs_group(se);
        }
        rq_unlock_irqrestore(rq, &rf);
    }

done:
    mutex_unlock(&shares_mutex);
    return 0;
}
```

所以，调用的来源在于何处 ?
1. core.c 中间的来源是: cgroup
2. `int proc_sched_autogroup_set_nice(struct task_struct *p, int nice)` 利用 proc 设置
    1. 从 nice 得到 share 的方法: `shares = scale_load(sched_prio_to_weight[idx]);`
    2. scale_load 只是在 64bit 的机器上提升精度

综合以上内容，share 就是 task group 的权重，而且除非管理员干预，整个就是静态的
  1. 但是 se.load.weight 并不是静止的

所以，为什么 task 需要含有三个 prio 而，这里就这么简单了，但是这一个这么复杂 ?

让我们分析一下，计算整个空间:


```plain
                    tg->shares * grq->load.weight
ge->load.weight = -------------------------------               (1)
                        \Sum grq->load.weight

注: grq := group cfs_rq
```
1. `tg->shares` : tg 的权重，static 的，和 其中的 task 的执行状态无关。
2. `grq->load.weight` : cfs_rq 中间所有的 entity 的 weight 求和，实时发生变化
3. \Sum 求和的对象，对于其中所有的 task_group 控制的所有的 `grq->load.weight` 得到其结果。
  1. 含义是什么: 当前的 cfs_rq 持有的 share 权重，在一个 cfs_rq 中间的

2. 感觉和 pelt 渐行渐远了!


分析一下 cfs_rq.load 的修改的内容，非常简单: account_entity_enqueue 添加进去的 sched_entity 的 se 就加进去。
> 所以，当在队列中间 se 的 weight 发生修改的时候，是不是首先需要 dequeued 然后 enqueue


```c
static void
account_entity_enqueue(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
    update_load_add(&cfs_rq->load, se->load.weight);
  // 如果到达顶层了，那么就刷新一下 rq
    if (!parent_entity(se))
        update_load_add(&rq_of(cfs_rq)->load, se->load.weight);

  // CONFIG_SMP 内的内容还是不知道呀!
#ifdef CONFIG_SMP
    if (entity_is_task(se)) {
        struct rq *rq = rq_of(cfs_rq);

        account_numa_enqueue(rq, task_of(se));
        list_add(&se->group_node, &rq->cfs_tasks);
    }
#endif
    cfs_rq->nr_running++;
}
```

两个调用位置:
1. enqueue_entity
2. reweight_entity

```c
void set_user_nice(struct task_struct *p, long nice)
    if (queued)
        dequeue_task(rq, p, DEQUEUE_SAVE | DEQUEUE_NOCLOCK);

    if (queued) {
        enqueue_task(rq, p, ENQUEUE_RESTORE | ENQUEUE_NOCLOCK);
        /*
         * If the task increased its priority or is running and
         * lowered its priority, then reschedule its CPU:
         */
        if (delta < 0 || (delta > 0 && task_running(rq, p)))
            resched_curr(rq);
    }

// 如果当前的 task_struct 在队列上，
// 想要修改其 weight 首先将其 dequeue 然后 enqueue
```

## bandwidth
利用接口
- /sys/fs/cgroup/cpu/cpu.cfs_quota_us
- /sys/fs/cgroup/cpu/cpu.cfs_period_us

`period`表示周期，`quota`表示限额，也就是在 period 期间内，用户组的 CPU 限额为 quota 值，当超过这个值的时候，用户组将会被限制运行（throttle），等到下一个周期开始被解除限制（unthrottle）；
- [ ] 还是一个 task_group 作为对象来限制吗 ?
  - [ ] 是一个 cpu 还是总的 cpu ?

- [ ] 那么我之前一致说，保证至少运行一点时间的机制在哪里啊 ?

```c
struct cfs_bandwidth {
#ifdef CONFIG_CFS_BANDWIDTH
    raw_spinlock_t      lock;
    ktime_t         period;
    u64         quota;
    u64         runtime; // 记录限额剩余时间，会使用quota值来周期性赋值；
    s64         hierarchical_quota;

    u8          idle;
    u8          period_active; // 周期性计时已经启动；
    u8          slack_started;
    struct hrtimer      period_timer;
    struct hrtimer      slack_timer; // 延迟定时器，在任务出列时，将剩余的运行时间返回到全局池里；
    struct list_head    throttled_cfs_rq;

    /* Statistics: */
    int         nr_periods;
    int         nr_throttled;
    u64         throttled_time;
#endif
};


/* CFS-related fields in a runqueue */
struct cfs_rq {
// ...
#ifdef CONFIG_CFS_BANDWIDTH
    int         runtime_enabled;
    s64         runtime_remaining; // 剩余的运行时间；

    u64         throttled_clock;
    u64         throttled_clock_task;
    u64         throttled_clock_task_time;
    int         throttled;
    int         throttle_count;
    struct list_head    throttled_list;
#endif /* CONFIG_CFS_BANDWIDTH */
```
- [ ] cfs_rq::runtime_remaining 和 cfs_bandwidth::runtime 描述感觉是同一个东西啊
- [x] cfs_bandwidth 会被多个 cfs_rq，是的，注意 bandwidth 的概念一致都是作用于 task_group 的，而不是 se 的
  - [x] 所以，cfs_bandwidth 就是一个全局概念

- `tg_set_cfs_bandwidth` 会从 `root_task_group` 根节点开始，遍历组调度树，并逐个设置限额比率 ；
- 由于 task_group 是层级的，如果顶层的被限制，下面的所有节点都是需要被限制，所以 quota 需要需要累计所有的子节点

注入时间, 或者称之为 runtime_remaining++ :
1. update_curr
2. check_enqueue_throttle :
3. set_next_task_fair : This routine is mostly called to set `cfs_rq->curr` field when a task migrates between groups/classes.

- slack_timer 定时器，slack_period 周期默认为 5ms，在该定时器函数中也会调用 distribute_cfs_runtime 从全局运行时间中分配 runtime；
- slack_timer : 一个用于将未用完的时间再返回到时间池中


A group’s unassigned quota is globally tracked, being refreshed back to cfs_quota units at each period boundary.

```c
/*
 * Amount of runtime to allocate from global (tg) to local (per-cfs_rq) pool
 * each time a cfs_rq requests quota.
 *
 * Note: in the case that the slice exceeds the runtime remaining (either due
 * to consumption or the quota being specified to be smaller than the slice)
 * we will always only issue the remaining available time.
 *
 * (default: 5 msec, units: microseconds)
 */
unsigned int sysctl_sched_cfs_bandwidth_slice       = 5000UL;
```

- bandwidth_slice : 时间是 cpu 时间，而不是 wall clock

- throttle 的两条路径:
  - check_enqueue_throttle : 如果从 runtime pool 中间都借不到资源，那么就只能 throttle
    - account_cfs_rq_runtime
      - `__account_cfs_rq_runtime` : 如果 `cfs_rq->runtime_remaining > 0`，那么就不需要继续了，有钱还借钱，贱不贱啊!
        - assign_cfs_rq_runtime : 借钱开始
          - `__assign_cfs_rq_runtime` : 从 runtime pool 中间尽量取出来给其
            - start_cfs_bandwidth : 如果 period 过期了，那么顺便将 period timer 移动一下
  - check_cfs_rq_runtime
    - throttle_cfs_rq

![](https://img2020.cnblogs.com/blog/1771657/202003/1771657-20200310214423221-158953219.png)


- [x] 可是，我还是无法理解 slack_timer
  - slack_timer：延迟定时器，在任务出列时，将**剩余的运行时间**返回到全局池里；
  - slack_timer 定时器，slack_period 周期默认为 5ms，在该定时器函数中也会调用 distribute_cfs_runtime 从全局运行时间中分配 runtime；
  - 好吧，还是理解的，当存在 task 将自己的时间返回给 runtime pool 的时候，不要立刻进行 distribute, 因为还有可能其他的 task 也在返回，所以等

- dequeue_entity
  - return_cfs_rq_runtime : return excess runtime on last dequeue
    - `__return_cfs_rq_runtime` :  we know any runtime found here is valid as update_curr() precedes return
      - 将自己持有的时间 `cfs_rq->runtime_remaining` 返回给 runtime pool `cfs_b->runtime`，如果此时有人被 unthrottle, 那么 `start_cfs_slack_bandwidth`
      - runtime_refresh_within : Are we near the end of the current quota period?

```c
/* a cfs_rq won't donate quota below this amount */
static const u64 min_cfs_rq_runtime = 1 * NSEC_PER_MSEC;
/* minimum remaining period time to redistribute slack quota */
static const u64 min_bandwidth_expiration = 2 * NSEC_PER_MSEC;
/* how long we wait to gather additional slack before distributing */
static const u64 cfs_bandwidth_slack_period = 5 * NSEC_PER_MSEC;
```
- unthrottle_cfs_rq 的时候，似乎操作就是 enqueue_task 就可以了，再次之前，runtime pool 的数值必然得到补充了

- [ ] update_curr

## cpu
[Linux Cgroup 系列（05）：限制 cgroup 的 CPU 使用（subsystem 之 cpu）](https://segmentfault.com/a/1190000008323952)
- [ ] Only worked on cgroup v1, can we redo the experiement on v2 ?
