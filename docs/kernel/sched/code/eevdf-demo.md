# EEVDF 用户态实验

这个实验不直接读取内核里的 `lag`、`eligible`、`virtual deadline`，而是从用户态去观察一个更容易看到的结果：

- 一个周期性短任务
- 一个持续满载的 CPU hog
- 两者绑到同一个 CPU

看短任务每次被唤醒后，实际要等多久才能开始继续执行。

这对应 EEVDF 在 [fair.c](/home/martins3/data/kernel/linux-build/kernel/sched/fair.c#L903) 里的两个核心条件:

1. task 必须是 eligible
2. 在 eligible 的任务里，选 virtual deadline 最早的

对于“经常睡眠、每次只做一点点事”的任务，用户态最容易观察到的效果通常是：

- 它醒来后通常不会被 CPU hog 长时间压住
- 它的 wake latency 往往比较小
- 它的短 burst 比较容易很快完成

## 运行

```sh
./eevdf-demo.out 10 1 20000 500 0 0
```

参数含义:

- `10`: 运行 10 秒
- `1`: 绑到 CPU1
- `20000`: 周期线程每 20ms 醒一次
- `500`: 每次醒来后忙跑 500us
- `0`: hog 线程 nice
- `0`: 周期线程 nice

程序会创建两个 `SCHED_OTHER` 线程:

- `thread[0]`: hog，持续忙跑
- `thread[1]`: periodic，周期性睡眠然后做一小段工作

## 观察

程序启动后会直接打印两个线程的真实 `tid` 和可复制的 `/proc/<tid>/sched` 命令，例如:

```sh
thread[1] observe: watch -n 0.2 "grep -E 'se\.|vruntime|deadline|slice|sum_exec_runtime|nr_switches|nr_voluntary_switches|nr_involuntary_switches' /proc/12345/sched"
```

你可以重点看:

- `se.vruntime`
- `se.slice`
- `sum_exec_runtime`
- `nr_switches`

## 输出怎么理解

周期线程会定期打印:

```txt
thread[1] iter=40 wake_latency=31us cpu_runtime=504us
```

意思是:

- 这次计划唤醒点之后，等了 `31us` 才真正拿到 CPU
- 它这轮实际消耗了约 `504us` 的线程 CPU 时间

最后会打印汇总:

```txt
summary:
  periodic thread: samples=500 period=20000us work=500us
  wake latency us: min=8 avg=24.3 p50=19 p95=61 p99=115 max=310
  wake latency buckets: >100us=3 >500us=0 >1000us=0
```

## 建议对照

### 1. 默认情况

```sh
./eevdf-demo.out 10 1 20000 500 0 0
```

先建立基线。

### 2. 周期任务更“交互式”

```sh
./eevdf-demo.out 10 1 10000 200 0 0
```

更短的工作、更频繁的周期，一般更像交互/短请求任务。

### 3. 提高 hog 权重

```sh
./eevdf-demo.out 10 1 20000 500 -5 0
```

这会让 hog 更重。短任务通常仍然能跑，但 wake latency 可能会变差。

### 4. 降低周期任务权重

```sh
./eevdf-demo.out 10 1 20000 500 0 5
```

这会让周期任务自己更轻，延迟往往会明显变差。

## 这个实验最想说明什么

- EEVDF 不是“谁先醒谁立刻独占 CPU”
- 它也不是 RT 调度
- 它是在 `SCHED_OTHER` 里，尽量让“该被服务的 task”按更合适的顺序拿到 CPU

用户态最容易看到的就是:

- 周期性短任务即使和 hog 同 CPU，通常也能比较快地被调度
- 而不是长时间饿着等 hog 自己“跑爽了”

## 局限

- 这个实验不能直接证明内核内部某次 pick 一定是因为哪个 `virtual deadline`
- wake latency 还会受 timer、irq、CPU idle、频率变化等因素影响
- 它更适合作为“EEVDF 用户态现象”实验，不是严格证明内核公式

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
