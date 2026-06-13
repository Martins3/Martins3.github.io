# sched PELT
<!-- 714ad195-deb4-4794-8be8-5424ae486235 -->

这组实验不是为了“证明调度器怎么选进程”，而是为了直接观察 PELT 的 3 类核心信号:

最直接的观察入口是:

- `/proc/<pid>/sched`
- /sys/kernel/debug/sched/debug

## 通用观察命令

程序跑起来后，先记下输出中的 `pid` 和 `tid`。


```sh
watch -n 0.2 "grep -E 'se.avg|load_avg|runnable_avg|util_avg|util_est' /proc/<tid>/sched"
```

(不过的确，是需要理解这个输出的)

```txt
🧀  grep -E 'se.avg|load_avg|runnable_avg|util_avg|util_est' /proc/self/sched
se.avg.load_sum                              :                47355
se.avg.runnable_sum                          :             24223524
se.avg.util_sum                              :             24223524
se.avg.load_avg                              :                 1024
se.avg.runnable_avg                          :                  510
se.avg.util_avg                              :                  510
se.avg.last_update_time                      :      452700891902976
se.avg.util_est                              :                    0
```

## 实验 1: 一直跑

```sh
./pelt-busy-demo.out 10 1
```

这个程序会把自己绑到一个 CPU 上持续忙跑 10 秒。

预期现象:

- `util_avg` 持续升高并接近高值
- `runnable_avg` 也接近高值
- `load_avg` 也会上升

要点:

- 这是最接近“稳定 CPU-bound”任务的样子
- 对单线程满载来说，`runnable_avg` 和 `util_avg` 会比较接近

## 实验 2: 跑一会，睡一会

```sh
./pelt-burst-demo.out 10 1 5 20
```

参数含义:

- `10`: 运行 10 秒
- `1`: 绑到 CPU1
- `5`: 每轮忙跑 5ms
- `20`: 每轮睡眠 20ms

预期现象:

- `util_avg` 比 busy-demo 明显低
- 任务睡眠后，`util_avg` 不会瞬间掉到 0，而是平滑衰减
- `runnable_avg` 和 `util_avg` 都会体现出“bursty”特征

要点:

- 这个实验最适合感受 PELT 的指数衰减
- 如果把 `sleep_ms` 改大，信号会降得更明显
- 如果把 `work_ms` 改大，信号会更像 CPU-bound

## 实验 3: 两个线程抢一个 CPU

```sh
./pelt-contention-demo.out 10 1
```

这个程序会创建两个 `SCHED_OTHER` 线程，绑到同一个 CPU，同时忙跑。

预期现象:

- 两个线程的 `runnable_avg` 都会比较高，因为它们几乎一直想跑
- 但每个线程的 `util_avg` 会低于单线程 busy-demo，因为它们只能轮流拿 CPU
- 从 runqueue 角度看，`cfs_rq` 的 `runnable_avg` 往往比单个 task 更能体现 contention

这个实验最想说明的是:

- runnable 不等于 running
- “经常想跑” 和 “真的跑到了多少” 是两回事
- 这正是 `runnable_avg` 和 `util_avg` 要分开的原因

## 一个好用的对照顺序

按下面顺序跑，最容易建立直觉:

1. `./pelt-busy-demo.out 10 1`
2. `./pelt-burst-demo.out 10 1 5 20`
3. `./pelt-contention-demo.out 10 1`

对照关系:

- 实验 1 看“持续满载”
- 实验 2 看“睡眠后的平滑衰减”
- 实验 3 看“runnable 和 running 的分离”

## 几个容易混的点

- `/proc/loadavg` 不是 PELT，它是另一套系统 load average 统计
- `load_avg` 受权重影响，`util_avg` 更接近真实 CPU 使用强度
- 看 task 的时候优先看 `/proc/<tid>/sched`，因为线程级 PELT 更直观
- 新创建的任务信号不会瞬间到位，PELT 本来就是“慢慢收敛”的

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
