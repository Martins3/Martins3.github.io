# memory model litmus 测试

由内核模块 `m/concurrent/memory_model.c` 和 `m/concurrent/mm_ll.c` 转换而来的用户态测试,
并补充了经典的 SB / MP / LB litmus 测试。使用 pthread + C11 stdatomic,
Linux (gcc) 和 macOS / Asahi Linux (clang / gcc) 都可以直接编译。

## 构建和运行

// 参考: https://github.com/smcdef/memory-reordering

```sh
make            # 每个测试生成 xx-nofence.out 和 xx-fence.out 两个版本
./run-all.sh    # 每个测试跑 5 秒, 依次输出结果
./sb-nofence.out 10   # 也可以单独跑, 参数是秒数, 默认 10
```

- `nofence` 版本: `FENCE()` 只是编译器屏障, 允许 CPU 乱序, 用来观察现象
- `fence` 版本: `FENCE()` 是 seq_cst 全屏障, 作为对照, 乱序结果应当消失

测试变量一律是 `volatile` + `compiler_barrier()`, 禁止编译器重排,
因此观察到的都是 CPU 层面的乱序。
另外测试变量按 256B 对齐放到不同 cache line 上: 若共享一条 cache line,
两个 store 会随同一个 line 一起可见, mp / lb 几乎不可能触发。

## 测试列表

| 测试 | litmus | 检测的乱序结果 | x86 (TSO) | ARM |
|------|--------|----------------|-----------|-----|
| sb | `x=1; r1=y` ∥ `y=1; r2=x` | r1=0 且 r2=0 | 允许 | 允许 |
| mp | `x=i; y=i` 递增 ∥ `r1=y; r2=x` | r1 > r2 | 禁止 | 允许 |
| lb | `r1=x; y=1` ∥ `r2=y; x=1` | r1=1 且 r2=1 | 禁止 | 允许 |
| ll | `x=1; y=1` ∥ `r1=x; r2=y` | r1=1 且 r2=0 (假象, 见下) | 不适用 | 不适用 |
| wr | `x=1; y=1; y=0; x=0` 循环 | 看到 y=1 且 x=0 | 禁止 | 允许 |
| ss | `a=t; b=t` ∥ `d=b; c=a` | d > c | 禁止 | 允许 |

- sb: Store Buffering, 双方都先写后读, 写滞留在 store buffer 导致互相读到旧值。 x86 上最容易观察到的乱序。
- mp: Message Passing, 看到新 flag 却配上旧数据, x86 TSO 保证不允许, ARM 允许。 采用单调递增自由跑版本 (writer 持续 `x=i; y=i`, reader 检测 r1 > r2),
  不需要每轮重置, 持续制造 store 流量, 比 rendezvous 版本更容易触发。
- lb: Load Buffering, load 和随后的 store 乱序, ARM 理论上允许, 实测 Apple Silicon 上 5 秒内未触发, 保留作对照。
- ll: 对应内核 `mm_ll.c` 的忠实转换, 作为**反面教材**保留。
  内核检测条件 r1=1 且 r2=0 只是读者两次 load 跨过 writer 两次 store 的
  时间窗口, SC 也允许; 这个形状 (W: x;y / R: x;y) 不存在对乱序敏感的结果。
  实测它在 x86 任何版本 (含 fence) 都会 "测到", 且 fence 版本次数不归零,
  恰好证明测到的不是乱序。判断一个 litmus 是否有效的标准:
  fence 版本必须归零 (对照实验)。
- wr: 对应内核 `memory_model.c` 的 `test_failed_logic`, 参考
  <https://github.com/smcdef/memory-reordering>, 同样是时间窗口占主导的
  测试: 读者读到 y=1 后若被调度走, 回来时 writer 早已执行完 x=0,
  x86 和 fence 版本都会 "测到", fence 版本甚至更多 (fence 拉长了时间窗口)。
- ss: 对应内核 `memory_model.c` 的 `test_ss_logic`, a 和 b 写入同一个快照值,
  理论上任何时刻都相等, 观察到 d > c 说明 store 乱序。

sb / lb / ll 用 watcher 每轮重置变量并 rendezvous 两个 actor
(等价于内核版本里的 sem_x / sem_y / sem_end 信号量),
mp / wr / ss 自由跑 (mp 用单调递增避免重置, wr / ss 和内核版本一致)。

## 实测结果

### x86_64 (本机, Intel/AMD)

```
[sb-nofence] arch=x86: reorder(r1=0,r2=0) detected 4185618 / 18320364 iterations
[sb-fence]   arch=x86: reorder(r1=0,r2=0) detected 0 / 18426997 iterations
[mp-nofence] arch=x86: reorder(flag>data) detected 0 / 97452152 checks
[mp-fence]   arch=x86: reorder(flag>data) detected 0 / 170042961 checks
[lb-nofence] arch=x86: reorder(r1=1,r2=1) detected 0 / 15262854 iterations
[lb-fence]   arch=x86: reorder(r1=1,r2=1) detected 0 / 17369335 iterations
[ll-nofence] arch=x86: racy-timing(r1=1,r2=0) detected 124676 / 19134872 iterations (NOT a reorder proof)
[ll-fence]   arch=x86: racy-timing(r1=1,r2=0) detected 702165 / 17651800 iterations (NOT a reorder proof)
[wr-nofence] arch=x86: hits(y=1,x=0) 0 / 100433190 checks (fence 版本不归零说明是时间窗口假象)
[wr-fence]   arch=x86: hits(y=1,x=0) 8515 / 272122218 checks (fence 版本不归零说明是时间窗口假象)
[ss-nofence] arch=x86: reorder(d>c) detected 0 / 113936849 checks
[ss-fence]   arch=x86: reorder(d>c) detected 0 / 117962080 checks
```

### aarch64 (Asahi Linux @ Apple Silicon, 100.113.183.51)

```
[sb-nofence] arch=arm: reorder(r1=0,r2=0) detected 95 / 29092460 iterations
[sb-fence]   arch=arm: reorder(r1=0,r2=0) detected 0 / 24077549 iterations
[mp-nofence] arch=arm: reorder(flag>data) detected 136589057 / 342861784 checks
[mp-fence]   arch=arm: reorder(flag>data) detected 0 / 183557732 checks
[lb-nofence] arch=arm: reorder(r1=1,r2=1) detected 0 / 33399335 iterations
[lb-fence]   arch=arm: reorder(r1=1,r2=1) detected 0 / 24703486 iterations
[ll-nofence] arch=arm: racy-timing(r1=1,r2=0) detected 4 / 33475849 iterations (NOT a reorder proof)
[ll-fence]   arch=arm: racy-timing(r1=1,r2=0) detected 174 / 26362565 iterations (NOT a reorder proof)
[wr-nofence] arch=arm: hits(y=1,x=0) 285199 / 3117154268 checks (fence 版本不归零说明是时间窗口假象)
[wr-fence]   arch=arm: hits(y=1,x=0) 2 / 3429056110 checks (fence 版本不归零说明是时间窗口假象)
[ss-nofence] arch=arm: reorder(d>c) detected 23585059 / 1462506076 checks
[ss-fence]   arch=arm: reorder(d>c) detected 0 / 193916441 checks
```

### 结果解读

- sb: x86 上 23% 命中率 (cache line 分离后更容易触发), ARM 上反而很少,
  fence 版本都归零, 是教科书式的 store buffer 效应。
- mp: **最戏剧性的对比** —— x86 上 1.7 亿次检查 0 命中, ARM 上 40% 命中,
  fence 版本都归零。这就是 "ARM 需要 smp_wmb(), x86 不需要" 的直接证据。
- ss: 同样清晰, x86 0 命中, ARM 1.6% 命中, fence 后归零。
- lb: 两个平台都没测到。ARM 理论上允许 load buffering,
  但 Apple Silicon 上在这个 harness 里很难触发, 保留作对照。
- ll / wr: 两个时间窗口测试, 命中与否和 fence 无关 (fence 版本不归零),
  说明它们测到的不是乱序, 见上面的分析。

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
