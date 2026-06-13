## latency-collector
感觉就是配合哪些 scheduler 来运行的

tools/tracing/latency/latency-collector.c


```txt
History:        #0
Commit:         e23db805da2dfc39e5281b5efd3e36d132aa83af
Author:         Viktor Rosendahl <Viktor.Rosendahl@bmw.de>
Committer:      Steven Rostedt (VMware) <rostedt@goodmis.org>
Author Date:    Fri 12 Feb 2021 09:44:21 PM CST
Committer Date: Sat 13 Feb 2021 12:52:59 AM CST

tracing/tools: Add the latency-collector to tools directory

This is a tool that is intended to work around the fact that the
preemptoff, irqsoff, and preemptirqsoff tracers only work in
overwrite mode. The idea is to act randomly in such a way that we
do not systematically lose any latencies, so that if enough testing
is done, all latencies will be captured. If the same burst of
latencies is repeated, then sooner or later we will have captured all
the latencies.

It also works with the wakeup_dl, wakeup_rt, and wakeup tracers.
However, in that case it is probably not useful to use the random
sleep functionality.

The reason why it may be desirable to catch all latencies with a long
test campaign is that for some organizations, it's necessary to test
the kernel in the field and not practical for developers to work
iteratively with field testers. Because of cost and project schedules
it is not possible to start a new test campaign every time a latency
problem has been fixed.

It uses inotify to detect changes to /sys/kernel/tracing/trace.
When a latency is detected, it will either sleep or print
immediately, depending on a function that act as an unfair coin
toss.

If immediate print is chosen, it means that we open
/sys/kernel/tracing/trace and thereby cause a blackout period
that will hide any subsequent latencies.

If sleep is chosen, it means that we wait before opening
/sys/kernel/tracing/trace, by default for 1000 ms, to see if
there is another latency during this period. If there is, then we will
lose the previous latency. The coin will be tossed again with a
different probability, and we will either print the new latency, or
possibly a subsequent one.

The probability for the unfair coin toss is chosen so that there
is equal probability to obtain any of the latencies in a burst.
However, this assumes that we make an assumption of how many
latencies there can be. By default  the program assumes that there
are no more than 2 latencies in a burst, the probability of immediate
printout will be:

1/2 and 1

Thus, the probability of getting each of the two latencies will be 1/2.

If we ever find that there is more than one latency in a series,
meaning that we reach the probability of 1, then the table will be
expanded to:

1/3, 1/2, and 1

Thus, we assume that there are no more than three latencies and each
with a probability of 1/3 of being captured. If the probability of 1
is reached in the new table, that is we see more than two closely
occurring latencies, then the table will again be extended, and so
on.

On my systems, it seems like this scheme works fairly well, as
long as the latencies we trace are long enough, 300 us seems to be
enough. This userspace program receive the inotify event at the end
of a latency, and it has time until the end of the next latency
to react, that is to open /sys/kernel/tracing/trace. Thus,
if we trace latencies that are >300 us, then we have at least 300 us
to react.

The minimum latency will of course not be 300 us on all systems, it
will depend on the hardware, kernel version, workload and
configuration.

Example usage:

In one shell, give the following command:
sudo latency-collector -rvv -t preemptirqsoff -s 2000 -a 3

This will trace latencies > 2000us with the preemptirqsoff tracer,
using random sleep with maximum verbosity, with a probability
table initialized to a size of 3.

In another shell, generate a few bursts of latencies:

root@host:~# modprobe preemptirq_delay_test delay=3000 test_mode=alternate
burst_size=3
root@host:~# echo 1 > /sys/kernel/preemptirq_delay_test/trigger
root@host:~# echo 1 > /sys/kernel/preemptirq_delay_test/trigger
root@host:~# echo 1 > /sys/kernel/preemptirq_delay_test/trigger
root@host:~# echo 1 > /sys/kernel/preemptirq_delay_test/trigger

If all goes well, you should be getting stack traces that shows
all the different latencies, i.e. you should see all the three
functions preemptirqtest_0, preemptirqtest_1, preemptirqtest_2 in the
stack traces.

Link: https://lkml.kernel.org/r/20210212134421.172750-2-Viktor.Rosendahl@bmw.de

Signed-off-by: Viktor Rosendahl <Viktor.Rosendahl@bmw.de>
Signed-off-by: Steven Rostedt (VMware) <rostedt@goodmis.org>
```


> [!NOTE]
> 参考神奇海螺的意见，有待验证

# latency-collector 使用指南

## 功能

监控内核 ftrace 延迟 tracer，通过 inotify 监听 `tracing_max_latency` 文件变化，每当检测到新的最大延迟时，自动读取并打印 trace 内容。

程序退出时自动还原所有 ftrace 设置（tracer、thresh、options）。

---

## 编译

依赖 `libtracefs` 和 `libtraceevent`。

**使用 nix 临时提供依赖：**

```bash
nix-shell -p libtraceevent libtracefs pkg-config --run \
  "make -C tools/tracing/latency/"
```

**使用系统包（Ubuntu/Debian）：**

```bash
apt install libtracefs-dev libtraceevent-dev
make -C tools/tracing/latency/
```

---

## 支持的 Tracer

```bash
sudo latency-collector --list
```

常见输出（取决于内核配置）：

```
wakeup_dl
wakeup_rt
wakeup
preemptirqsoff   # 需要 CONFIG_PREEMPT_TRACER + CONFIG_IRQSOFF_TRACER
preemptoff       # 需要 CONFIG_PREEMPT_TRACER
irqsoff          # 需要 CONFIG_IRQSOFF_TRACER
```

各 tracer 含义：

| Tracer | 追踪对象 |
|--------|---------|
| `wakeup` | 最高优先级任务从被唤醒到实际运行的延迟 |
| `wakeup_rt` | RT 任务从被唤醒到实际运行的延迟 |
| `wakeup_dl` | DEADLINE 任务的唤醒延迟 |
| `preemptirqsoff` | 关中断或关抢占的最长持续时间 |
| `preemptoff` | 关抢占的最长持续时间 |
| `irqsoff` | 关中断的最长持续时间 |

---

## 参数说明

```
Usage: latency-collector [OPTION]...
```

| 参数 | 说明 |
|------|------|
| `-l, --list` | 列出当前内核支持的延迟 tracer |
| `-t TR, --tracer TR` | 指定使用的 tracer，默认按优先级自动选择 |
| `-s TH, --threshold TH` | 触发阈值（微秒），`0` 表示用 `tracing_max_latency`（每次新 max 都触发） |
| `-f, --function` | 启用 `function-trace`，记录延迟期间的函数调用 |
| `-g, --graph` | 启用 `display-graph`，以调用图形式展示函数 |
| `-r, --random` | 随机延迟后再读 trace（避免读 trace 时的 blackout 遮盖后续事件） |
| `-a N, --nrlat N` | 预设最多 N 个聚集延迟（配合 `-r` 使用），默认 2 |
| `-u MS, --time MS` | 随机延迟的睡眠时间（毫秒），默认 1000ms，隐含 `-r` |
| `-n, --notrace` | 检测到延迟时不打印 trace 内容（仅统计） |
| `-e N, --threads N` | 打印线程数，默认 3 |
| `-v, --verbose` | 增加详细输出（`-vv` 输出更多） |
| `-F, --force` | 强制覆盖已有 tracer（默认若 tracer 非 nop 则拒绝启动） |
| `-c POL, --policy POL` | 调度策略：`other/batch/idle/rr/fifo`，默认 `rr` |
| `-p PRI, --priority PRI` | 调度优先级，默认 99 |
| `-x, --no-ftrace` | 不自动配置 ftrace，由用户手动配置 |
| `-i FILE, --tracefile FILE` | 手动指定 trace 文件路径，隐含 `-x` |
| `-m FILE, --max-lat FILE` | 手动指定 `tracing_max_latency` 文件路径，隐含 `-x` |

---

## 典型用法

### 基础使用

自动选择 tracer，阈值为 0（每次新 max 都触发）：

```bash
sudo latency-collector
```

### 指定 tracer + 阈值 + 函数追踪

捕获 wakeup_rt 延迟超过 100µs 的事件，并记录函数调用栈：

```bash
sudo latency-collector -t wakeup_rt -s 100 -f -v
```

### 随机采样模式

适合延迟事件密集出现的场景（如 irqsoff），随机选取其中一个记录，避免总是记录第一个：

```bash
sudo latency-collector -t preemptirqsoff -r -v
```

### 仅统计不打印 trace

只在退出时显示最大延迟值：

```bash
sudo latency-collector -t wakeup -s 50 -n
```

### 手动配置 ftrace，仅借用打印功能

```bash
# 手动设置好 ftrace 后
sudo latency-collector -x -i /sys/kernel/tracing/trace \
  -m /sys/kernel/tracing/tracing_max_latency
```

---

## 实测结果

**环境：** Linux 6.18，虚拟机，24 CPU

**命令：**
```bash
sudo latency-collector -t wakeup_rt -s 0 -f -v
```

**捕获输出（158µs，来自 latency-collector 自身 RT 线程的唤醒延迟）：**

```
# tracer: wakeup_rt
# latency: 158 us, #335/335, CPU#1 | (M:PREEMPT(lazy) VP:0, KP:0, SP:0 HP:0 #P:24)
# task: latency-collect-96404 (uid:0 nice:5 policy:2 rt_prio:99)

  <idle>-0    1dNh3.    0us+: 0:120:R + [001] 96404:0:R latency-collect
  <idle>-0    1dNh3.   14us : <stack trace>
 => probe_wakeup
 => ttwu_do_activate
 => sched_ttwu_pending
 => __flush_smp_call_function_queue
 => __sysvec_call_function_single
 => sysvec_call_function_single
 => pv_native_safe_halt
 => default_idle
```

**退出时输出：**
```
The maximum detected latency was: 158us
```

---

## 注意事项

- 需要 **root 权限**（访问 `/sys/kernel/tracing/`）
- 程序以 `SCHED_RR priority=99` 运行，避免自身被调度延迟影响
- 若发现当前 tracer 不是 `nop`，默认**拒绝启动**（防止冲突），用 `-F` 强制覆盖
- `preemptirqsoff` / `preemptoff` / `irqsoff` 需要内核开启对应配置项，普通发行版内核通常不包含
- 读取 trace 文件本身会造成数百毫秒的 ftrace blackout，`-r` 参数通过随机延迟来分散采样点

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
