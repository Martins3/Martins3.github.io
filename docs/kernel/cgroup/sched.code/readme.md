已经扩展好了。现在项目里有三个配套工具，都在 docs/kernel/cgroup/ 下：

┌────────────────────┬──────────────────────────────────────────────────────────────┐
│ 文件               │ 作用                                                         │
├────────────────────┼──────────────────────────────────────────────────────────────┤
│ dump-task-group.bt │ 遍历内核 task_groups 链表，dump 每个 task_group 的层级关系   │
├────────────────────┼──────────────────────────────────────────────────────────────┤
│ dump-task-group.py │ 把上面的原始数据格式化成树状图                               │
├────────────────────┼──────────────────────────────────────────────────────────────┤
│ dump-cfs-rq.bt     │ 接收一个 cfs_rq 地址，dump 它的基本信息 + rbtree 中的 entity │
└────────────────────┴──────────────────────────────────────────────────────────────┘

用法

1. 看 task_group 树

```bash
echo a | sudo -S bpftrace dump-task-group.bt | python3 dump-task-group.py
```

输出示例（已经在你的机器上跑过）：

```txt
  0xffffffff84a10200 shares=1048576 cfs_rq[0]=0xffff889ffe833140 se[0]=0 (ROOT)
  ├── 0xffff88811180c200 shares=1048576 cfs_rq[0]=0xffff8881035bd400 se[0]=0xffff88810052c000
  ├── 0xffff888114e2ee00 shares=1048576 cfs_rq[0]=0xffff888114deb800 se[0]=0xffff888112db1000
  │   ├── 0xffff888114e2f900 shares=1048576 cfs_rq[0]=0xffff888124b06c00 se[0]=0xffff888107ef8400
  │   ├── 0xffff888114e2d280 shares=1048576 cfs_rq[0]=0xffff88810bf22600 se[0]=0xffff8881082c4400
  │   └── ...
```

注意：
• cfs_rq[0]：这个 task_group 在 CPU 0 上的 cfs_rq 地址
• se[0]：这个 task_group 在 CPU 0 上的 group sched_entity 地址（在父级 cfs_rq 的 rbtree 里排队时用到）
• root 的 se[0]=0，因为 root 没有父级，不需要在父 cfs_rq 中表现为 group entity

2. 深入看某个 cfs_rq 的 rbtree

从上一步的输出里挑一个 cfs_rq[0] 地址，比如 0xffff888124b06c00：

```bash
  echo a | sudo -S bpftrace dump-cfs-rq.bt 0xffff888124b06c00
```

输出示例（当前系统空闲，rbtree 为空）：

```txt
  cfs_rq=0xffff888124b06c00 tg=0xffff888114e2f900 nr_queued=0 h_nr_queued=0
  curr=0 next=0 rb_root=0 rb_leftmost=0
  (empty rbtree)
```

如果系统有负载，你会看到类似这样的输出：

```txt
  cfs_rq=0xffff... tg=0xffff... nr_queued=2 h_nr_queued=2
  curr=0xffff... next=0 rb_root=0xffff... rb_leftmost=0xffff...
  --- rbtree entities ---
    [TASK ] se=0xffff... vruntime=12345678 parent=0
    [GROUP] se=0xffff... vruntime=23456789 my_q=0xffff... parent=0
  dumped 2 rbtree nodes
```

• [TASK]：普通进程 entity，my_q == 0
• [GROUP]：子 cgroup 的 group entity，my_q 指向该子 cgroup 自己的 cfs_rq
• curr：当前正在运行、已被 dequeue 出 rbtree 的 entity（如果有的话）

────────────────────────────────────────────────────────────────────────────────

限制说明

1. bpftrace 循环上限：dump-cfs-rq.bt 中 rbtree 遍历上限是 16 个节点。这是 BPF verifier 的限制，超过 16
   次循环展开就会拒绝加载。对于理解层级结构通常够用了。
2. 只 dump CPU 0：dump-task-group.bt 当前只打印 cfs_rq[0] 和 se[0]。如果你需要看其他 CPU，原理一样，解引用 cfs_rq[cpu] 和
   se[cpu] 即可。
3. 空闲系统可能为空：cfs_rq 的 rbtree 只在有 runnable 且排队等待的任务时才非空。当前运行中的任务会被 dequeue（curr
   指针指向它），所以 rb_root 可能为 0，但 curr 非 0。

────────────────────────────────────────────────────────────────────────────────

如果你想看一个有负载的 cfs_rq 的完整输出，可以跑你的 cgroup-sched-lab.sh 制造几个 busy worker，然后挑它们所在 cgroup 的
cfs_rq[0] 用 dump-cfs-rq.bt 查看。


## cgroup sched 在多核上的分布
<!-- 9127fe9c-6203-4543-817f-63eda87d171e -->

这个测试终于完成了我一直奇怪的东西:
1. 子 cgroup 其实是作为 se 放到 rbtree 的
2. 每一个 CPU 都有 cfs_rq ，cfs_rq 中有 rbtree 的

我在 /sys/fs/cgroup/demo-sched-test/ 下建了一个层级：

```txt
  demo-A (cpu.weight=100)
  ├── demo-B (cpu.weight=200)   ← a(PID 2646310, CPU0) + b(PID 2646311, CPU1)
  └── demo-C (cpu.weight=300)   ← c(PID 2646312, CPU0) + d(PID 2646313, CPU1) + e(PID 2646314, CPU2)
```

然后通过 bpftrace dump 出了内核里的 task_group、cfs_rq、sched_entity 和 rbtree 的实拍数据。

────────────────────────────────────────────────────────────────────────────────

实测数据：task_group 层级

```
  A  tg=0xffff889bf06039c0  shares=1048576  parent=demo-sched-test
  B  tg=0xffff888ab75d6e00  shares=2097152  parent=A
  C  tg=0xffff889fe0b1d280  shares=3145728  parent=A
```

shares 和 cpu.weight 的关系：shares = cpu.weight * 1048576 / 100
• weight=100 → shares=1048576
• weight=200 → shares=2097152
• weight=300 → shares=3145728

────────────────────────────────────────────────────────────────────────────────

核心结论：多核下每个 CPU 都有一整套 cfs_rq

一个 cgroup（task_group）不是只有一个 cfs_rq，而是每个 CPU 都有一个独立的 cfs_rq。

B 和 C 的 cfs_rq 分布：

┌────────┬───────────────┬───────────────┬────────────────────┐
│ cgroup │ CPU 0 cfs_rq  │ CPU 1 cfs_rq  │ CPU 2 cfs_rq       │
├────────┼───────────────┼───────────────┼────────────────────┤
│ B      │ 0xffff...a000 │ 0xffff...6c00 │ 0xffff...1600 (空) │
├────────┼───────────────┼───────────────┼────────────────────┤
│ C      │ 0xffff...7a00 │ 0xffff...b400 │ 0xffff...f600      │
└────────┴───────────────┴───────────────┴────────────────────┘

B 在 CPU 2 上没有进程，所以 cfs_rq[2] 是空的。C 在三个 CPU 上都有进程，所以三个 cfs_rq 都非空。

────────────────────────────────────────────────────────────────────────────────

A 的 cfs_rq：装的是 B 和 C 的 group entity

A 自己没有进程，它的 rbtree 里只有 group sched_entity（代表 B 和 C）：

```txt
  === A cfs_rq[0] (CPU 0) ===
  nr_queued=2  h_nr_queued=2
    [CURR:GROUP] se=0xffff888c52ae2400 vruntime=135926731147 my_q=0xffff888558eea000
    [GROUP]      se=0xffff888a61e0a400 vruntime=135926984813 my_q=0xffff888132a07a00

  === A cfs_rq[1] (CPU 1) ===
  nr_queued=2  h_nr_queued=2
    [CURR:GROUP] se=0xffff88850fe3bc00 vruntime=136142682456 my_q=0xffff88945211b400
    [GROUP]      se=0xffff888b7234b800 vruntime=136144474778 my_q=0xffff88830c856c00

  === A cfs_rq[2] (CPU 2) ===
  nr_queued=1  h_nr_queued=1
    [CURR:GROUP] se=0xffff8896933ff400 vruntime=272582939947 my_q=0xffff8891290ff600
```

关键点：
• A 的 rbtree 里没有 task，只有 group entity
• my_q 指向的是 子 cgroup 的 cfs_rq。比如 A 中代表 C 的 group entity，它的 my_q=0xffff888132a07a00，这正是 C 的 cfs_rq[0]
• CPU 0 上 A 的 curr 是 B 的 group entity（说明当前调度器正在运行 B 下面的任务 a）
• CPU 1 上 A 的 curr 是 C 的 group entity（说明当前正在运行 C 下面的任务 d）
• CPU 2 上只有 C 有进程（e），所以 A 的 cfs_rq[2] 里只有一个 C 的 group entity，没有 B

────────────────────────────────────────────────────────────────────────────────

B 和 C 的 cfs_rq：装的是真正的 task entity

```txt
  === B cfs_rq[0] (CPU 0, a 在跑) ===
  curr=TASK  se=0xffff8886c6538080  vruntime=108317230474

  === B cfs_rq[1] (CPU 1, b 在跑) ===
  curr=TASK  se=0xffff8888fcff2b40  vruntime=108532409529

  === C cfs_rq[0] (CPU 0, c 在跑) ===
  curr=TASK  se=0xffff888f3ebbab40  vruntime=108677868102

  === C cfs_rq[1] (CPU 1, d 在排队) ===
  curr=0
  rb_root=TASK  se=0xffff8898b06e2b40  vruntime=108894974287  parent=0xffff88850fe3bc00

  === C cfs_rq[2] (CPU 2, e 在跑) ===
  curr=TASK  se=0xffff8899990f0080  vruntime=218100392810
```

注意 parent=0xffff88850fe3bc00，这正是 C 在 A 的 cfs_rq[1] 中的 group sched_entity 地址。验证了层级关系：

```txt
  A cfs_rq[1]
    └─ group se for C  (parent=0xffff8893abbafc00, 这是 C 的 se[1])
         └─ C cfs_rq[1]
              └─ task se for d  (parent=0xffff88850fe3bc00, 指回 C 的 group se)
```

────────────────────────────────────────────────────────────────────────────────

一句话总结

│ 每个 cgroup 在每个 CPU 上都有一个独立的 cfs_rq（内含 rbtree）。父 cgroup 的 rbtree 里排的是子 cgroup 的 group
│ sched_entity，子 cgroup 的 rbtree 里排的是 task 的 sched_entity。调度器 pick_next_task 时，从 root cfs_rq 开始，遇到 group
│ entity 就通过 my_q 进入子 cfs_rq，递归向下，直到选到一个真正的 task。


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
