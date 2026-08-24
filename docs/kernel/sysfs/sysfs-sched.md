# sysfs sched
## 分析下 linux/kernel/sched/debug.c 中的内容

## /proc/schedstat && /proc/pid/schedstat
<!-- 990e9df0-b06f-4129-aa45-7d4a27984694 -->

实现文件 : sched/stat.c ，一共也就 200 行
- https://docs.kernel.org/scheduler/sched-stats.html 解释 /proc/schedstat 和 /proc/pid/schedstat
- /proc/schedstat 对应函数 show_schedstat ，关联的 config 为 CONFIG_SCHEDSTATS
- /proc/pid/schedstat 对应函数 proc_pid_schedstat ，关联 config 为 CONFIG_SCHED_INFO
- schedstat 可以通过内核启动参数关闭或者 /proc/sys/kernel/sched_schedstats，参考 setup_schedstats 和 sysctl_schedstats

### /proc/pid/schedstat
/proc/pid/schedstat 的含义:
```txt
time spent on the cpu (in nanoseconds)
time spent waiting on a runqueue (in nanoseconds)
# of timeslices run on this cpu
```

```c
struct sched_info {
#ifdef CONFIG_SCHED_INFO
	/* Cumulative counters: */

	/* # of times we have run on this CPU: */
	unsigned long			pcount;

	/* Time spent waiting on a runqueue: */
	unsigned long long		run_delay;

	/* Timestamps: */

	/* When did we last run on a CPU? */
	unsigned long long		last_arrival;

	/* When were we last queued to run? */
	unsigned long long		last_queued;

#endif /* CONFIG_SCHED_INFO */
};
```
- [ ] 无法理解什么叫做 of times we have run on this CPU ，这个参数和 cpu 运行是没有关系的吧


### [ ] 密切关联的
```txt
config TASK_DELAY_ACCT
	bool "Enable per-task delay accounting"
	depends on TASKSTATS
	select SCHED_INFO
	help
	  Collect information on time spent by a task waiting for system
	  resources like cpu, synchronous block I/O completion and swapping
	  in pages. Such statistics can help in setting a task's priorities
	  relative to other tasks for cpu, io, rss limits etc.

	  Say N if unsure.
```
### [ ] 回答这个问题
https://stackoverflow.com/questions/52736873/understanding-scheduling-and-proc-pid-sched

## /proc/pid/sched
<!-- 0758822c-36f4-424d-b63d-f31c9f137686 -->

fs/proc/base.c 中 sched_show -> proc_sched_show_task
关联的 config 为 CONFIG_SCHED_DEBUG 。其中的内容是受到 CONFIG_SCHEDSTATS 影响的。

实现的函数在:
- proc_sched_show_task
  - print_cfs_stats
  - 以及 dump struct sched_statistics 中内容
  - show_numa_stats

```txt
qemu (11176, #threads: 124)
-------------------------------------------------------------------
se.exec_start                                :     140935799.916922
se.vruntime                                  :         10268.049254
se.sum_exec_runtime                          :         43129.162737
se.nr_migrations                             :                 3645
nr_switches                                  :               218495
nr_voluntary_switches                        :               218380
nr_involuntary_switches                      :                  115
se.load.weight                               :              1048576
se.runnable_weight                           :              1048576
se.avg.load_sum                              :                   19
se.avg.runnable_load_sum                     :                   19
se.avg.util_sum                              :                19456
se.avg.load_avg                              :                    0
se.avg.runnable_load_avg                     :                    0
se.avg.util_avg                              :                    0
se.avg.last_update_time                      :      140935799916544
se.avg.util_est.ewma                         :                   10
se.avg.util_est.enqueued                     :                    0
policy                                       :                    0
prio                                         :                  120
clock-delta                                  :                   30
mm->numa_scan_seq                            :                   61
numa_pages_migrated                          :                  724
numa_preferred_nid                           :                    5
total_numa_faults                            :                 3440
current_node=5, numa_group_id=11176
numa_faults node=0 task_private=1 task_shared=1 group_private=1 group_shared=1
numa_faults node=1 task_private=1 task_shared=1 group_private=1 group_shared=1
numa_faults node=2 task_private=1 task_shared=1 group_private=1 group_shared=1
numa_faults node=3 task_private=1 task_shared=0 group_private=1 group_shared=0
numa_faults node=4 task_private=1 task_shared=1 group_private=1 group_shared=1
numa_faults node=5 task_private=3427 task_shared=1 group_private=3427 group_shared=1
numa_faults node=6 task_private=1 task_shared=1 group_private=1 group_shared=1
numa_faults node=7 task_private=1 task_shared=0 group_private=1 group_shared=0
```
## /proc/pid/status

实现函数 : proc_pid_status

```txt
voluntary_ctxt_switches:        0
nonvoluntary_ctxt_switches:     0
x86_Thread_features:
x86_Thread_features_locked:
```

## /proc/stat

文档: https://man7.org/linux/man-pages/man5/proc_stat.5.html

可以通过 fs/proc/stat.c:show_stat 将各种时间的统计开始结束时间确定下

```c
		/* Copy values here to work around gcc-2.95.3, gcc-2.96 */
		user		= cpustat[CPUTIME_USER];
		nice		= cpustat[CPUTIME_NICE];
		system		= cpustat[CPUTIME_SYSTEM];
		idle		= get_idle_time(&kcpustat, i);
		iowait		= get_iowait_time(&kcpustat, i);
		irq		= cpustat[CPUTIME_IRQ];
		softirq		= cpustat[CPUTIME_SOFTIRQ];
		steal		= cpustat[CPUTIME_STEAL];
		guest		= cpustat[CPUTIME_GUEST];
		guest_nice	= cpustat[CPUTIME_GUEST_NICE];
```

通过 /proc/stat 来实现 top 1 ，展示各个 CPU 在各种任务上话费了多少时间。

## debugfs

/sys/kernel/debug/sched

### debug

展示每一个 cfs_rq 的信息，也就是 cgroup 层级 * cpu 数量
```txt
cfs_rq[31]:/test
  .exec_clock                    : 0.000000
  .MIN_vruntime                  : 0.000001
  .min_vruntime                  : 69643.566731
  .max_vruntime                  : 0.000001
  .spread                        : 0.000000
  .spread0                       : -854710191.753634
  .nr_spread_over                : 0
  .nr_running                    : 1
  .h_nr_running                  : 1
  .idle_nr_running               : 0
  .idle_h_nr_running             : 0
  .load                          : 1048576
  .load_avg                      : 1024
  .runnable_avg                  : 1024
  .util_avg                      : 1019
  .util_est_enqueued             : 0
  .removed.load_avg              : 0
  .removed.util_avg              : 0
  .removed.runnable_avg          : 0
  .tg_load_avg_contrib           : 1024
  .tg_load_avg                   : 32763
  .throttled                     : 0
  .throttle_count                : 0
  .se->exec_start                : 1083661332.978182
  .se->vruntime                  : 1142880506.076297
  .se->sum_exec_runtime          : 69682.607166
  .se->load.weight               : 32773
  .se->avg.load_avg              : 32
  .se->avg.util_avg              : 1019
  .se->avg.runnable_avg          : 1024
```

### features
```txt
GENTLE_FAIR_SLEEPERS START_DEBIT NO_NEXT_BUDDY LAST_BUDDY CACHE_HOT_BUDDY WAKEUP_PREEMPTION NO_HRTICK NO_HRTICK_DL NO_DOUBLE_TICK NONTASK_CAPACITY TTWU_QUEUE NO_SIS_PROP SIS_UTIL NO_WARN_DOUBLE_CLOCK RT_PUSH_IPI NO_RT_RUNTIME_SHARE NO_LB_MIN ATTACH_AGE_LOAD WA_IDLE WA_WEIGHT WA_BIAS UTIL_EST UTIL_EST_FASTUP NO_LATENCY_WARN ALT_PERIOD BASE_SLICE
```

### idle_min_granularity_ns
750000

### latency_ns
### latency_warn_ms
### latency_warn_once
### migration_cost_ns
### min_granularity_ns
### nr_migrate
### preempt
### tunable_scaling
### verbose
### wakeup_granularity_ns


## echo w | sudo tee /proc/sysrq-trigger
- sysrq_sched_debug_show : echo t 进去
- sysrq_showstate_blocked_op : echo w 进去

## /proc 创建目录和 /proc/pid/task 下的目录都是动态生成的

很显然，就是 thread group 组成 /proc :
```c
/* for the /proc/TGID/task/ directories */
static int proc_task_readdir(struct file *file, struct dir_context *ctx)
```

忽然意识到，这个目录是查询的过程中动态展示出来的，参考
- proc_pid_readdir
  - 只是有点奇怪的是，这个遍历效率很低啊

`proc_pid_make_inode` ，

```txt
#0  proc_pid_make_inode (sb=0xffff888103c28000, task=task@entry=0xffff8881109b8000, mode=33188) at fs/proc/base.c:1890
#1  0xffffffff814c8f15 in proc_pident_instantiate (dentry=dentry@entry=0xffff88811da25680, task=task@entry=0xffff8881109b8000, ptr=ptr@entry=0xffffffff8264c0e8 <tgid_base_stuff+520>) at fs/proc/base.c:2643
#2  0xffffffff814c908e in proc_pident_lookup (end=0xffffffff8264c6b0, p=0xffffffff8264c0e8 <tgid_base_stuff+520>, dentry=0xffff88811da25680, dir=<optimized out>) at fs/proc/base.c:2679
#3  proc_tgid_base_lookup (dir=<optimized out>, dentry=0xffff88811da25680, flags=<optimized out>) at fs/proc/base.c:3378
#4  0xffffffff8143f15a in lookup_open (op=0xffffc90000017edc, op=0xffffc90000017edc, got_write=false, file=0xffff888106eceb00, nd=0xffffc90000017dc0) at fs/namei.c:3394
#5  open_last_lookups (op=0xffffc90000017edc, file=0xffff888106eceb00, nd=0xffffc90000017dc0) at fs/namei.c:3484
#6  path_openat (nd=nd@entry=0xffffc90000017dc0, op=op@entry=0xffffc90000017edc, flags=flags@entry=65) at fs/namei.c:3712
#7  0xffffffff81440d06 in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff888110a72000, op=op@entry=0xffffc90000017edc) at fs/namei.c:3742
#8  0xffffffff81427aea in do_sys_openat2 (dfd=-100, filename=<optimized out>, how=how@entry=0xffffc90000017f18) at fs/open.c:1356
#9  0xffffffff81427fe7 in do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1372
#10 __do_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1388
#11 __se_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1383
#12 __x64_sys_openat (regs=<optimized out>) at fs/open.c:1383
#13 0xffffffff822a0f0c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000017f58) at arch/x86/entry/common.c:50
#14 do_syscall_64 (regs=0xffffc90000017f58, nr=<optimized out>) at arch/x86/entry/common.c:80
```

## /proc/pid

主要看看其中的软链接是如何实现的?

### /proc/pid/stat

获取一个 process 上次运行所在的 CPU :
```sh
awk '{print "PID: " $1, "Last CPU: " $39}' /proc/self/stat
```
### /proc/pid/cmdline
proc_pid_cmdline_ops

实现的非常复杂，不是一行的事

### /proc/pid/pwd

- proc_cwd_link
  - get_fs_pwd
    - path_get

然后获取:

```c
struct task_struct {
  // ...
	/* Filesystem information: */
	struct fs_struct		*fs;
  // ...
```

如果一个进程的 pwd 被删除了，在 /proc/10204/cwd 将是:

```txt
cwd -> '/root/gg (deleted)'
```

如果重新创建 /root/gg ，输出还是如此。

这个现象可以继续跟踪一下。

## /proc/sys/kernel 和 sched 相关控制项目
<!-- 15942c54-d989-47ad-8b05-2094861b7d4f -->

https://docs.kernel.org/admin-guide/sysctl/kernel.html

- threads-max : 系统中到底可以有多少个 thread
- pid_max : 当 pid 到多少的时候，那么 pid 重新从最小的开始分配
- sched_cfs_bandwidth_slice_us
- sched_deadline_period_max_us
- sched_deadline_period_min_us
- sched_energy_aware : ???
- sched_rr_timeslice_ms
- sched_rt_period_us
- sched_rt_runtime_us
- sched_schedstats : 默认为 0
- sched_autogroup_enabled
- sched_util_clamp_max
- sched_util_clamp_min
- sched_util_clamp_min_rt_default

| 项目 | 作用 | 默认值 / 范围 | 注意点 |
| --- | --- | --- | --- |
| `threads-max` | 全系统可由 `fork()` 创建的 task 上限；进程和每个线程均计数。 | 启动时按内存估算，使线程结构最多约占可用页的 1/8；可写范围 `1`～`0x3fffffff`。 | 不是每用户限制；还受 `RLIMIT_NPROC`、cgroup `pids.max` 等约束。 |
| `pid_max` | PID 分配的回绕上界；下一个 PID 到该值时从较小 PID 重新查找。`pid_max` 本身及更大值不会分配。 | 64 位通常最大 `4194304`；最小值随 CPU 数动态提高。 | 它限制 PID/TID 编号空间，不等同于当前可创建线程数。 |
| `sched_cfs_bandwidth_slice_us` | CFS cgroup 带宽配额从全局池分给每 CPU 本地运行队列时，每次分配的批量大小。 | `5000` µs；至少 `1` µs；需 `CONFIG_CFS_BANDWIDTH`。 | 不是普通 CFS 任务的调度时间片。增大可降低全局配额锁竞争；减小可让配额消费更细粒度。仅在 cgroup 设置 CPU 配额时有实际影响。 |
| `sched_deadline_period_min_us` | `SCHED_DEADLINE` 任务的 `sched_period` 允许的最小值。 | 默认 `100` µs；与 max 保持 `min ≤ max`。 | 过小会提高定时器和调度开销；`runtime`、`deadline`、`period` 仍须满足 `runtime ≤ deadline ≤ period`。 |
| `sched_deadline_period_max_us` | `SCHED_DEADLINE` 任务的 `sched_period` 允许的最大值。 | 默认 `4194304` µs，约 4.19 秒。 | 是任务准入参数的边界，不是全局 DL 带宽本身。 |
| `sched_energy_aware` | 开关 EAS（Energy Aware Scheduling）：在满足性能需求的候选 CPU 中，结合能耗模型选择更省电的放置位置。 | EAS 可用时为 `0/1`，通常默认启用。 | 需要非对称 CPU 容量拓扑、Energy Model 和相关 cpufreq 支持。非 EAS 平台读为空，写入失败；它不是直接的 CPU 频率上限。 |
| `sched_rr_timeslice_ms` | `SCHED_RR` 同优先级任务轮转的 quantum。 | 默认 `100` ms；写入 `0` 或负数会复位到默认值。 | 仅影响 `SCHED_RR`，不影响 `SCHED_FIFO` 或 CFS；实际精度受 `HZ` 的 jiffy 粒度限制。 |
| `sched_rt_period_us` | RT 带宽控制的统计周期。每个周期内，FIFO/RR 任务最多使用 `sched_rt_runtime_us` 的 CPU 时间。 | 默认 `1000000` µs（1 秒），范围 `1`～`INT_MAX`。 | 也用于 `SCHED_DEADLINE` 的全局准入带宽比例。 |
| `sched_rt_runtime_us` | 每个 RT 周期允许 FIFO/RR 使用的总运行预算；耗尽时 RT 任务被 throttled 到下一周期。 | 默认 `950000` µs；范围 `-1`～`sched_rt_period_us`。 | 默认预留约 5% CPU 给非 RT 任务。`-1` 表示不限制，失控 RT 任务可能令机器无法交互。 |
| `sched_schedstats` | 启用调度器统计，例如 `/proc/schedstat` 及相关调度统计。 | `0/1`，默认 `0`；需 `CONFIG_SCHEDSTATS`。 | 打开有调度热路径开销，适合调试或性能分析；写入需 `CAP_SYS_ADMIN`。 |
| `sched_autogroup_enabled` | 按 session 自动把普通任务归入 CFS task group，使终端或桌面工作负载不易被另一 session 的大量 CPU 任务压制。 | `0/1`；上游通常默认 `1`，`noautogroup` 启动参数会关闭；需 `CONFIG_SCHED_AUTOGROUP`。 | 主要面向桌面交互性；服务器或已有精细 cgroup CPU 分组时通常不需要它。 |
| `sched_util_clamp_min` | 系统允许的最大 `UCLAMP_MIN` 请求值，即全局“性能下限/boost”上限。 | `0`～`1024`，默认 `1024`。 | 设为 `512` 后，任务请求更高的最低性能点不会生效到 `512` 以上。`1024` 是容量刻度，不是百分比 CPU 时间。 |
| `sched_util_clamp_max` | 系统允许的最大 `UCLAMP_MAX` 值，即全局性能上限。 | `0`～`1024`，默认 `1024`，且不得小于 `sched_util_clamp_min`。 | 例如设为 `512` 会将整个系统的有效性能点限制在约 50% 容量刻度，常用于省电或温控策略。 |
| `sched_util_clamp_min_rt_default` | 未通过 `sched_setattr()` 显式指定 uclamp 的 RT 任务，其默认 `UCLAMP_MIN`。 | `0`～`1024`，默认 `1024`；有效值受 `sched_util_clamp_min` 约束。 | 默认让 RT 任务倾向最高性能点和高容量 CPU；移动或电池设备可下调以换取续航。 |

几个容易混淆的点：

- `threads-max` 限制 task 数，`pid_max` 限制 PID/TID 编号回绕空间，两者不是同一个限制。
- `sched_cfs_bandwidth_slice_us` 是 cgroup 配额的“补给批量”，不是 CFS 的时间片。
- `sched_rt_*` 管的是 RT 带宽预算；`sched_rr_timeslice_ms` 管的是 RR 任务间轮转。
- uclamp 是调度放置和 `schedutil` 频率选择的性能提示或约束，并不等同于 cgroup CPU quota。

参考：

- [Documentation for /proc/sys/kernel/](https://docs.kernel.org/admin-guide/sysctl/kernel.html)
- [CFS Bandwidth Control](https://docs.kernel.org/scheduler/sched-bwc.html)
- [Real-Time group scheduling](https://docs.kernel.org/scheduler/sched-rt-group.html)
- [Deadline Task Scheduling](https://docs.kernel.org/scheduler/sched-deadline.html)
- [Utilization Clamping](https://docs.kernel.org/scheduler/sched-util-clamp.html)
- [`sched(7)`](https://man7.org/linux/man-pages/man7/sched.7.html)

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
