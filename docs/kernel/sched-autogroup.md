2. autogroup 机制的原理

setsid

group scheduling 表示 CONFIG_FAIR_GROUP_SCHED

**A new autogroup is created when a new session is created via setsid(2);
this happens, for example, when a new terminal window is started.**
A new process created by fork(2) inherits its parent's autogroup membership.
Thus, all of the processes in a session are members of the same autogroup.
An autogroup is automatically destroyed when the last process in the group terminates.
> process group 和 task group 混为一团


```c
// 在配置了 autogroup 的 setsid 被调用
/* Allocates GFP_KERNEL, cannot be called under any spinlock: */
void sched_autogroup_create_attach(struct task_struct *p)
{
    struct autogroup *ag = autogroup_create();

    autogroup_move_group(p, ag);

    /* Drop extra reference added by autogroup_create(): */
    autogroup_kref_put(ag);
}
EXPORT_SYMBOL(sched_autogroup_create_attach);

static inline struct autogroup *autogroup_create(void)
    /* tg = sched_create_group(&root_task_group); */
  // 所有的总是默认使用的为 sched_create_group 中间的内容
```

CONFIG_FAIR_GROUP_SCHED 中间的 FAIR 是和 RT 对应的，所以其作用就是为了 group 的效果。


The use of the cgroups(7) CPU controller to place processes in cgroups other than the root CPU cgroup overrides the effect of autogrouping.
> 当 cgroup 没有被配置的时候，depth 没有任何意义，所有的 group 都是放在 root 下面
> 其中，代码分析也可以的出来该结果。

```txt
       cpu (since Linux 2.6.24; CONFIG_CGROUP_SCHED)
              Cgroups can be guaranteed a minimum number of "CPU shares"
              when a system is busy.  This does not limit a cgroup's CPU
              usage if the CPUs are not busy.  For further information, see
              Documentation/scheduler/sched-design-CFS.txt.

              In Linux 3.2, this controller was extended to provide CPU
              "bandwidth" control.  If the kernel is configured with CON‐
              FIG_CFS_BANDWIDTH, then within each scheduling period (defined
              via a file in the cgroup directory), it is possible to define
              an upper limit on the CPU time allocated to the processes in a
              cgroup.  This upper limit applies even if there is no other
              competition for the CPU.  Further information can be found in
              the kernel source file Documentation/scheduler/sched-bwc.txt.
```

> 两个文档读一下，结果发现都是垃圾

1. 目前的问题，cgroup 形成的多级结构现在无法理解 ?
> 其实并不难，因为 cgroup 可以利用文件的方法构建关系，
> 那么 entity 有的是文件夹，有的是文件一样，文件就是对应正常的 process
> 区分的标准就是 my_q 变量了

## https://lwn.net/Articles/639543/

```c
config SCHED_AUTOGROUP
    bool "Automatic process group scheduling"
    select CGROUPS
    select CGROUP_SCHED
    select FAIR_GROUP_SCHED
    help
      This option optimizes the scheduler for common desktop workloads by
      automatically creating and populating task groups.  This separation
      of workloads isolates aggressive CPU burners (like build jobs) from
      desktop applications.  Task group autogeneration is currently based
      upon task session.
```
> @todo 所以为什么通过 autogroup 将 CPU burner 和 desktop 之间划分开 ?

对于 cgroup 的文件夹的内容， task_group，entity 以及 cfs_rq 三者一组。

> So when a task belonging to tg migrates from CPUx to CPUy, it will be dequeued from tg->cfs_rq[x] and enqueued on tg->cfs_rq[y].


But the priority value by itself is not helpful to the scheduler,
which also needs to know the *load of the task* to estimate its *time slice*.
As mentioned above, the load must be the multiple of the capacity of a standard CPU that is required to make satisfactory progress on the task.
Hence this priority number must be mapped to such a value; this is done in the array `prio_to_weight[]`.

> 1. load of the taks 估计 time slice
> 2. time slice 是 The concept of a time slice was introduced above as the amount of time that a task is allowed to run on a CPU within a scheduling period.
> 3. @todo 既然存在 struct load_weight , load 和 weight 就是一个东西吗 ?

## auto group
man sched(7)
和 setsid 的关系

因为 task_group 是 cgroup 机制下，所以将 task_group 加入到机制中间:
- sched_online_group
- sched_offline_group

how and when groups of tasks are created:
1. Users may use the control group ("cgroup") infrastructure to partition system resources between tasks. Tasks belonging to a cgroup are associated with a group in the scheduler (if the scheduler controller is attached to the group).
2. When a new session is created through the `set_sid()` system call. All tasks belonging to a specific session also belong to the same scheduling group. This feature is enabled when `CONFIG_SCHED_AUTOGROUP` is set in the kernel configuration.
3. a single task becomes a scheduling entity on its own.
