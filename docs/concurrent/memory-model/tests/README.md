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
(dekker 相反, 故意让两个 flag 共享一条 line 来加剧 ownership 争抢,
否则 Apple Silicon 上几乎测不到, 见 dekker.c 注释。)

## 测试列表

| 测试 | litmus                         | 检测的乱序结果            | x86 (TSO) | ARM    |
|------|--------------------------------|---------------------------|-----------|--------|
| sb   | `x=1; r1=y` ∥ `y=1; r2=x`      | r1=0 且 r2=0              | 允许      | 允许   |
| dekker | `flag[i]=1; if (!flag[j]) 进临界区` | 双方同时在临界区    | 允许      | 允许   |
| mp   | `x=i; y=i` 递增 ∥ `r1=y; r2=x` | r1 > r2                   | 禁止      | 允许   |
| lb   | `r1=x; y=1` ∥ `r2=y; x=1`      | r1=1 且 r2=1              | 禁止      | 允许   |
| ll   | `x=1; y=1` ∥ `r1=x; r2=y`      | r1=1 且 r2=0 (假象, 见下) | 不适用    | 不适用 |
| wr   | `x=1; y=1; y=0; x=0` 循环      | 看到 y=1 且 x=0           | 禁止      | 允许   |
| ss   | `a=t; b=t` ∥ `d=b; c=a`        | d > c                     | 禁止      | 允许   |
| xor  | `v ^= bit0` ∥ `v ^= bit1`      | 最终值非 0 (lost update)  | 发生      | 发生   |

- xor: 两个线程对同一个 long 的**不同 bit** 做 `v ^= BIT`。
  单次对齐的 load/store 确实是原子的 (不会撕裂), 但 XOR 是
  load -> modify -> store 三步, 三步之间另一个线程的完整 RMW 可以插入,
  后写覆盖先写 (lost update)。每个线程把自己的 bit 翻转偶数次,
  正确结果必须是 0, 非 0 即为 lost update。
  atomic 版本 (`-DUSE_FENCE` 编译出 `xor-fence.out`) 用 `__atomic_fetch_xor`
  (x86 `lock xor` / ARM `ldaxr/stlxr`), 结果恒为 0。
  这个测试和乱序无关, 说明的是 "访问原子性 ≠ RMW 原子性"。
- sb: Store Buffering, 双方都先写后读, 写滞留在 store buffer 导致互相读到旧值。 x86 上最容易观察到的乱序。 **这正是 StoreLoad 乱序**: x86 TSO 允许的唯一一种乱序。
- dekker: StoreLoad 乱序的软件后果。去掉屏障的 Dekker 互斥锁,
  双方升起自己 flag 的 store 滞留在 store buffer, 都读到对方 flag 为 0,
  于是同时进入临界区。sb 展示的是微架构现象, dekker 展示的是它造成的
  真实 bug: 互斥失效。fence 版本修复。
  实现细节: 检测双方同时在临界区的记账代码 (in_cs) 两侧必须用无条件
  dmb, 否则 ARM 上记账本身的 store 乱序会造成 80% 的误报;
  两个 flag 故意共享一条 cache line 加剧 ownership 争抢,
  否则 Apple Silicon 上真实的乱序率太低测不到。
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
[dekker-nofence] arch=x86: mutual-exclusion broken 74737578 / 74739837 enters
[dekker-fence]   arch=x86: mutual-exclusion broken 0 / 46449249 enters
[xor-plain]  arch=x86: lost-update in 16 / 30 trials (each thread 500000 flips)
[xor-atomic] arch=x86: lost-update in 0 / 30 trials (each thread 500000 flips)
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
[dekker-nofence] arch=arm: mutual-exclusion broken 261679600 / 310918434 enters
[dekker-fence]   arch=arm: mutual-exclusion broken 0 / 207811091 enters
[xor-plain]  arch=arm: lost-update in 21 / 30 trials (each thread 500000 flips)
[xor-atomic] arch=arm: lost-update in 0 / 30 trials (each thread 500000 flips)
```

### 结果解读

- sb: x86 上 23% 命中率 (cache line 分离后更容易触发), ARM 上反而很少,
  fence 版本都归零, 是教科书式的 store buffer (StoreLoad) 效应。
- dekker: StoreLoad 的直接后果 —— 不加 fence 的 Dekker 锁在 x86 上几乎
  100% 的进入都发生了双方同时在临界区, ARM 上 84%; 加 fence 后两边
  都归零 (x86 4600 万次, ARM 2 亿次进入, 0 次失效)。
- mp: **最戏剧性的对比** —— x86 上 1.7 亿次检查 0 命中, ARM 上 40% 命中,
  fence 版本都归零。这就是 "ARM 需要 smp_wmb(), x86 不需要" 的直接证据。
- ss: 同样清晰, x86 0 命中, ARM 1.6% 命中, fence 后归零。
- lb: 两个平台都没测到。ARM 理论上允许 load buffering,
  但 Apple Silicon 上在这个 harness 里很难触发, 保留作对照。
- ll / wr: 两个时间窗口测试, 命中与否和 fence 无关 (fence 版本不归零),
  说明它们测到的不是乱序, 见上面的分析。
- xor: 两个平台 plain 版本都大量出错 (x86 16/30, ARM 21/30),
  atomic 版本都恒为 0。证明 "对 long 的单次读写是原子的" 推不出
  "两个线程 xor 不同 bit 互不影响" —— RMW 不是单次读写。

## 整理一下这个笔记 https://research.swtch.com/mm

用 sb.c 中的内容来测试的:

```c
// Thread 1           // Thread 2
x = 1;                while(done == 0) { /* loop */ }
done = 1;             print(x);
```
It depends. It depends on the hardware, and it depends on the compiler. A direct line-for-line translation to assembly run on an x86 multiprocessor will always print 1. But a direct line-for-line translation to assembly run on an ARM or POWER multiprocessor can print 0.
- [ ] 为什么 x86 不会
- [ ] 如果从 high level 的语言的角度处理，为什么会存在更加高级的


<p align="center">
  <img src="https://research.swtch.com/mem-sc.png" alt="drawing" align="center"/>
</p>
<p align="center">
https://research.swtch.com/hwmm
</p>

<p align="center">
  <img src="https://research.swtch.com/mem-tso.png" alt="drawing" align="center"/>
</p>
<p align="center">
https://research.swtch.com/hwmm
</p>


Litmus Test: Write Queue (also called Store Buffer) Can this program see r1 = 0, r2 = 0?
```txt
// Thread 1           // Thread 2
x = 1                 y = 1
r1 = y                r2 = x
```

- On sequentially consistent hardware: no.
- On x86 (or other TSO): yes!

This example may seem artificial, but using two synchronization variables does happen in well-known synchronization algorithms, such as `Dekker's algorithm` or `Peterson's algorithm`, as well as ad hoc schemes. They break if one thread isn’t seeing all the writes from another.

### kimi 的回忆

完成。新增 dekker.c —— 不加屏障的 Dekker 互斥锁，StoreLoad 乱序直接导致双方同时进入临界区：

┌─────────┬─────────────────────────────────┬─────────────────────────┐
│ 版本    │ x86                             │ ARM                     │
├─────────┼─────────────────────────────────┼─────────────────────────┤
│ nofence │ 7474 万/7474 万进入失效（~100%) │ 2.6 亿/3.1 亿失效（84%) │
├─────────┼─────────────────────────────────┼─────────────────────────┤
│ fence   │ 0/4645 万                       │ 0/2.1 亿                │
└─────────┴─────────────────────────────────┴─────────────────────────┘

你听到的没错：StoreLoad 是 x86 TSO 唯一允许的乱序，前面的 sb.c 测的就是它；dekker 展示的是它的软件后果——互斥锁失效，效果更直观。

调试这个测试时踩了两个坑，都写进了 dekker.c 注释和 README:

• 检测代码自身也怕乱序：最初用 in_cs 计数判断双方同时在临界区，ARM fence 版本误报率 80%——退出方 in_cs=0 的 store 被排到 flag=0 之后可见，互斥
  明明成立也会误报。记账代码两侧必须用无条件 dmb（不能跟着 FENCE() 宏走）。这本身就是"ARM 上共享变量通信都要考虑序"的活教材
• Apple Silicon 上真实乱序率太低：flag 分 cache line 时 ARM nofence 一次都测不到（和 sb 分线后只有 95/2909 万一致）；让两个 flag 共享一条 line
  加剧 ownership 争抢后才到 84%。这也解释了为什么这类 bug 在 ARM 上难复现但不代表不存在

README 的测试表格、两个架构的实测结果和解读都已更新，ARM 机器上的 ~/memory-model-tests/ 也同步了。

## 这个也可以整理了
https://zhuanlan.zhihu.com/p/41872203

## TODO

我总是感觉这里的例子有点奇怪，每次启动一个 thread，真的就可以运气那么好，然后卡到一起吗?

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
