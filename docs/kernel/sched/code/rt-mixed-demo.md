# 混合 RT / 普通线程测试

这个实验用于比较单 CPU 上两个线程分别使用不同调度策略时的效果。

它支持两类输入:

- `fifo` / `rr`: 参数值表示 RT priority
- `other`: 参数值表示 `nice`

也就是说，这个程序里第二和第四个参数统一叫 `value`，但含义按 policy 解释。

## 编译

```sh
cd /home/martins3/data/vn/docs/kernel/sched/code
make rt-mixed-demo.out
```

## 例子

### 1. RT 压制关系

```sh
sudo ./rt-mixed-demo.out fifo 80 rr 70 2 0
```

这表示:

- 线程 0: `SCHED_FIFO`, RT priority 80
- 线程 1: `SCHED_RR`, RT priority 70

单 CPU 上，只要 `FIFO:80` runnable，`RR:70` 基本没有运行机会。

### 2. 普通线程 nice 对比

```sh
./rt-mixed-demo.out other 0 other 5 2 0
```

这表示:

- 线程 0: `SCHED_OTHER`, nice 0
- 线程 1: `SCHED_OTHER`, nice 5

两者都属于普通调度类，都会运行，但 `nice 0` 一般会拿到更多 CPU 时间。

如果你想测试“更高权重”的普通线程，可以把 nice 调成负数，但这通常需要特权:

```sh
sudo ./rt-mixed-demo.out other -5 other 5 2 0
```

## 为什么之前 `other 10` 会失败

旧版本把 `other 10` 错当成:

- `SCHED_OTHER`
- `sched_priority = 10`

这是非法的，因为 `SCHED_OTHER` 的 `sched_priority` 必须是 0。

现在程序改成了统一走 `sched_setattr()`:

- `fifo/rr` 填 `sched_policy + sched_priority`
- `other` 填 `sched_policy + sched_nice`

所以现在:

```sh
./rt-mixed-demo.out other 10 other 0 2 0
```

表示的是:

- 线程 0: nice 10
- 线程 1: nice 0

这是合法的。

## 为什么之前失败后会卡住

旧版本用了 `pthread_barrier_t`。

如果一个线程在初始化阶段失败直接退出，另一个线程还会永久等在 barrier 上，所以整个程序看起来像卡死。

现在改成了:

- 两个线程先分别完成初始化
- 主线程确认两个线程都 ready 后，再统一放行
- 只要任一线程初始化失败，就直接让整个程序退出，不会再死等

## 观察方法

另开一个终端:

```sh
ps -T -p <pid> -o pid,tid,cls,rtprio,pri,ni,comm
```

可以看到:

- `FF` 表示 `SCHED_FIFO`
- `RR` 表示 `SCHED_RR`
- `TS` 一般表示 `SCHED_OTHER`
- `rtprio` 只对 RT 线程有意义
- `ni` 反映普通线程的 nice

程序自己的输出里也会打印:

- `policy`
- `sched_prio`
- `nice`

## 最想说明的点

- RT 类里先看 RT priority
- `RR` 的时间片不能帮助它抢过更高优先级 RT 线程
- `SCHED_OTHER` 不看 `sched_priority`，看的是 nice/权重
- 这个 demo 统一用 `sched_setattr()`，把 RT priority 和 nice 都放进同一套接口里

## 展示 SCHED_OTHER 的时候，不同的 nice 的差别

```txt
🤒  ./rt-mixed-demo.out other 20 other 0 2 1
pid=2315858 cpu=1 runtime=2s
thread[0]: SCHED_OTHER value=20
thread[1]: SCHED_OTHER value=0
thread[0] ready tid=2315859 cpu=1 policy=SCHED_OTHER sched_prio=0 nice=19
thread[1] ready tid=2315860 cpu=1 policy=SCHED_OTHER sched_prio=0 nice=0
thread[1] tid=2315860 policy=SCHED_OTHER sched_prio=0 nice=0 progress=250ms counter=13815733
thread[0] tid=2315859 policy=SCHED_OTHER sched_prio=0 nice=19 progress=411ms counter=217022
thread[1] tid=2315860 policy=SCHED_OTHER sched_prio=0 nice=0 progress=500ms counter=28701013
thread[0] tid=2315859 policy=SCHED_OTHER sched_prio=0 nice=19 progress=620ms counter=396174
thread[1] tid=2315860 policy=SCHED_OTHER sched_prio=0 nice=0 progress=750ms counter=43572681
thread[0] tid=2315859 policy=SCHED_OTHER sched_prio=0 nice=19 progress=828ms counter=575451
thread[1] tid=2315860 policy=SCHED_OTHER sched_prio=0 nice=0 progress=1000ms counter=58491181
thread[0] tid=2315859 policy=SCHED_OTHER sched_prio=0 nice=19 progress=1034ms counter=754852
thread[1] tid=2315860 policy=SCHED_OTHER sched_prio=0 nice=0 progress=1250ms counter=73292288
thread[0] tid=2315859 policy=SCHED_OTHER sched_prio=0 nice=19 progress=1450ms counter=1113578
thread[1] tid=2315860 policy=SCHED_OTHER sched_prio=0 nice=0 progress=1500ms counter=88232507
thread[0] tid=2315859 policy=SCHED_OTHER sched_prio=0 nice=19 progress=1659ms counter=1292540
thread[1] tid=2315860 policy=SCHED_OTHER sched_prio=0 nice=0 progress=1750ms counter=103110687
thread[0] tid=2315859 policy=SCHED_OTHER sched_prio=0 nice=19 progress=1867ms counter=1471881
thread[1] tid=2315860 policy=SCHED_OTHER sched_prio=0 nice=0 progress=2000ms counter=118049508
thread[1] done tid=2315860
thread[0] tid=2315859 policy=SCHED_OTHER sched_prio=0 nice=19 progress=2000ms counter=1858609
thread[0] done tid=2315859
```

## 其实 struct sched_attr 就可以看到其实实时 sched 类型和普通的完全不同的

/usr/include/linux/sched/types.h 中

```c
/*
 * Extended scheduling parameters data structure.
 *
 * This is needed because the original struct sched_param can not be
 * altered without introducing ABI issues with legacy applications
 * (e.g., in sched_getparam()).
 *
 * However, the possibility of specifying more than just a priority for
 * the tasks may be useful for a wide variety of application fields, e.g.,
 * multimedia, streaming, automation and control, and many others.
 *
 * This variant (sched_attr) allows to define additional attributes to
 * improve the scheduler knowledge about task requirements.
 *
 * Scheduling Class Attributes
 * ===========================
 *
 * A subset of sched_attr attributes specifies the
 * scheduling policy and relative POSIX attributes:
 *
 *  @size		size of the structure, for fwd/bwd compat.
 *
 *  @sched_policy	task's scheduling policy
 *  @sched_nice		task's nice value      (SCHED_NORMAL/BATCH)
 *  @sched_priority	task's static priority (SCHED_FIFO/RR)
 *
 * Certain more advanced scheduling features can be controlled by a
 * predefined set of flags via the attribute:
 *
 *  @sched_flags	for customizing the scheduler behaviour
 *
 * Sporadic Time-Constrained Task Attributes
 * =========================================
 *
 * A subset of sched_attr attributes allows to describe a so-called
 * sporadic time-constrained task.
 *
 * In such a model a task is specified by:
 *  - the activation period or minimum instance inter-arrival time;
 *  - the maximum (or average, depending on the actual scheduling
 *    discipline) computation time of all instances, a.k.a. runtime;
 *  - the deadline (relative to the actual activation time) of each
 *    instance.
 * Very briefly, a periodic (sporadic) task asks for the execution of
 * some specific computation --which is typically called an instance--
 * (at most) every period. Moreover, each instance typically lasts no more
 * than the runtime and must be completed by time instant t equal to
 * the instance activation time + the deadline.
 *
 * This is reflected by the following fields of the sched_attr structure:
 *
 *  @sched_deadline	representative of the task's deadline in nanoseconds
 *  @sched_runtime	representative of the task's runtime in nanoseconds
 *  @sched_period	representative of the task's period in nanoseconds
 *
 * Given this task model, there are a multiplicity of scheduling algorithms
 * and policies, that can be used to ensure all the tasks will make their
 * timing constraints.
 *
 * As of now, the SCHED_DEADLINE policy (sched_dl scheduling class) is the
 * only user of this new interface. More information about the algorithm
 * available in the scheduling class file or in Documentation/.
 *
 * Task Utilization Attributes
 * ===========================
 *
 * A subset of sched_attr attributes allows to specify the utilization
 * expected for a task. These attributes allow to inform the scheduler about
 * the utilization boundaries within which it should schedule the task. These
 * boundaries are valuable hints to support scheduler decisions on both task
 * placement and frequency selection.
 *
 *  @sched_util_min	represents the minimum utilization
 *  @sched_util_max	represents the maximum utilization
 *
 * Utilization is a value in the range [0..SCHED_CAPACITY_SCALE]. It
 * represents the percentage of CPU time used by a task when running at the
 * maximum frequency on the highest capacity CPU of the system. For example, a
 * 20% utilization task is a task running for 2ms every 10ms at maximum
 * frequency.
 *
 * A task with a min utilization value bigger than 0 is more likely scheduled
 * on a CPU with a capacity big enough to fit the specified value.
 * A task with a max utilization value smaller than 1024 is more likely
 * scheduled on a CPU with no more capacity than the specified value.
 *
 * A task utilization boundary can be reset by setting the attribute to -1.
 */
struct sched_attr {
	__u32 size;

	__u32 sched_policy;
	__u64 sched_flags;

	/* SCHED_NORMAL, SCHED_BATCH */
	__s32 sched_nice;

	/* SCHED_FIFO, SCHED_RR */
	__u32 sched_priority;

	/* SCHED_DEADLINE */
	__u64 sched_runtime;
	__u64 sched_deadline;
	__u64 sched_period;

	/* Utilization hints */
	__u32 sched_util_min;
	__u32 sched_util_max;

};
```

## rt 的优先级规则是什么?
<!-- 5103b2ed-f595-43ac-b31a-cea0f7a02d3c -->

原来 rt 的优先级规则怎么强，当然也这么简单:

| 场景 | 谁跑 |
|---|---|
| FIFO:80 vs RR:70 | FIFO:80 |
| RR:80 vs FIFO:70 | RR:80 |
| FIFO:50 vs RR:50，FIFO 已在跑 | FIFO:50 持续跑 |
| RR:50 vs FIFO:50，RR 已在跑 | 先 RR:50，时间片到后通常切到 FIFO:50，然后 FIFO 持续跑 |
| RR:50 vs RR:50 | 两者轮转 |
| FIFO:50 vs FIFO:50 | 谁先跑谁一直跑，直到阻塞/退出/yield |
| FIFO/RR:any vs OTHER | FIFO/RR |
| OTHER vs OTHER | 公平调度，轮流跑 |

一句话版本

- 不同 RT 优先级：高的绝对压低的
- 相同 RT 优先级：FIFO 比 RR 更霸道
- OTHER 遇到任何 runnable 的 RT 线程，基本都让路

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
