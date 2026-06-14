# deadline scheduler
## 看看文档

https://documentation.suse.com/sles/12-SP4/html/SLES-all/cha-tuning-io.html#sec-tuning-io-schedulers-deadline

1. 参数 : scheduler 是可以配置参数的
```bash
ls /sys/devices/pci0000:00/0000:00:17.0/ata7/host6/target6:0:0/6:0:0:0/block/sda/queue/iosched
#  async_depth   fifo_batch   front_merges   prio_aging_expire   read_expire   write_expire   writes_starved
```
2. The DEADLINE algorithm maintains two additional queues (read and write) in which requests are sorted by deadline


## 设备注册 scheduler 的规则是什么?

### qemu 中的两个 nvme 盘的默认 scheduler 不同

```txt
➜  ~ cat /sys/block/nvme*n1/queue/scheduler
none [mq-deadline]
[none] mq-deadline
```

```txt
➜  ~ cat /sys/block/nvme*n1/queue/nr_requests
256
1023
```

```sh
-device nvme,drive=nvme1,serial=foo,bus=mybridge,addr=0x1,id=nvme1 \
-drive file=/home/martins3/hack/vm/oe23/img/1,format=qcow2,if=none,id=nvme1 \

-device nvme,drive=nvme2,max_ioqpairs=1,serial=foo2,id=nvme2  \
-drive file=/home/martins3/hack/vm/oe23/img/2,format=qcow2,if=none,id=nvme2    \
```

具体实现在 elevator_init_mq 中，如果指定了，那么用指定的，
如果没有，用 elevator_get_default 中根据队列数量来确定

## 做点有趣的测试

**测试 nvme 的性能**
4k read iodepth=128
```txt
bs: 1 (f=1): [r(1)][2.7%][r=3298MiB/s][r=844k IOPS][eta 04m:52s]
```

将 none 修改为 mq-deadline 之后:

在 600K 到 700K 中间波动:
```txt
Jobs: 1 (f=1): [r(1)][4.7%][r=2660MiB/s][r=651k IOPS][eta 04m:46s]0s]
```

**测试 sata 的性能**

```txt
Jobs: 1 (f=1): [r(1)][2.0%][r=239MiB/s][r=61.2k IOPS][eta 04m:54s]
```

将 mq-deadline  修改为 none  之后:

```txt
Jobs: 1 (f=1): [r(1)][2.7%][r=237MiB/s][r=60.7k IOPS][eta 04m:52s]
```

几乎没有区别。


总结，就测试我们自己的机器上的性能，mq-deadline 对于 nvme 性能存在严重的影响，
对于 sata 影响可以忽略。

## codex mq deadline 分析

# 理解 mq-deadline 是如何工作的

理解 `mq-deadline`，最好的方式不是把它当成“一个很神秘的优化器”，而是先看没有它时 `blk-mq` 怎么跑。

## 没有 scheduler 时发生了什么

没有 block scheduler 时，请求大体上是：

1. `bio` 合并成 `request`
2. `request` 进入对应 `hctx` 的软件队列
3. `blk-mq` 按 CPU / ctx / hctx 的映射直接往 driver 派发
4. driver 拿到 tag 后下发给硬件

这条路径很短，开销低，但它基本不做“全局重排”。它更像“谁先到、哪个 CPU 先推到、哪个 hctx 当前能发，就先发哪个”。

所以它的问题也很直接：

- 可能错过跨 CPU、跨 ctx 的合并机会
- 可能让随机读写交错得很碎
- 可能让写请求长期压着读请求，或者反过来
- 多个 hctx 时，局部最优不等于设备全局最优

相关调度框架在 [block/blk-mq-sched.c](./block/blk-mq-sched.c)。

## mq-deadline 到底做了什么

`mq-deadline` 的核心不是“凭空提高设备能力”，而是：

`把 request 先收进一个队列级别的共享调度器里，再按更适合设备的顺序派发给各个 hctx。`

它的关键数据结构在 [block/mq-deadline.c](./block/mq-deadline.c)：

- 每个优先级一组队列
- 读和写分开维护
- 同时维护两种视图
- 一个按扇区位置排序的 `rbtree`
- 一个按到达时间排序的 `fifo list`

这两种视图分别解决两个目标：

- 按扇区排序：尽量让 I/O 更“顺”，减少来回跳
- 按到达时间排序：防止某些请求永远排不到，保证延迟上界

这就是 “deadline” 名字的来源。每个请求入队时会被赋一个过期时间：

- 读默认 `500ms`
- 写默认 `5s`

见 [block/mq-deadline.c](./block/mq-deadline.c) 中的默认参数定义。

### 它怎么选下一个 request

核心逻辑在 `__dd_dispatch_request()`。

可以把它理解成这几个规则叠加：

1. 平时优先按扇区位置连续派发
   这提高吞吐，尤其对顺序性敏感的设备更有利。

2. 如果某个请求等太久了，就不再坚持“位置最优”，而是直接从 FIFO 里把最早过期的发出去
   这保证延迟不会无限恶化。

3. 读通常优先于写
   如果有读在排队，默认先照顾读。

4. 但写不能一直被饿死
   `writes_starved=2`，意思是读连续占优几轮后，要让写也发一批。

5. 批量发同一方向的一小串请求
   `fifo_batch=16`，允许连续发一批“看起来顺”的请求，再重新评估。

所以它不是“严格 FIFO”，也不是“纯按 LBA 排序”，而是：

`大多数时候追求更顺的访问模式，必要时用 deadline 强行纠偏。`

### 它为什么能提升性能

提升一般来自三类收益。

1. 更好的合并和顺序性

`mq-deadline` 插入时会尝试 merge，请求也会进按 sector 排序的树。

这会让原本分散、交错的 I/O，变成更容易被设备高效处理的请求流。

对 HDD 最明显，因为磁头寻道是真成本。对 SATA SSD、部分 NVMe，也可能改善 device-side merge、FTL 写放大或队列抖动。

2. 降低尾延迟

如果只按“谁当前更好发”来派发，某些请求可能一直被更有利的请求压住。`mq-deadline` 的 FIFO 过期机制会强制这些老请求出队。

这类提升常体现在：

- 读延迟更稳定
- 混合负载下 P99/P999 更好
- 不容易出现“写流把读卡住很久”的现象

3. 在多队列环境里做“队列级”的全局观察

`mq-deadline` 虽然被 `hctx` 调用，但它维护的是 queue-wide shared state。也就是说：

- `blk-mq` 的底层是多队列
- 但 `mq-deadline` 在上层先做一次全局调度
- 然后再把选好的 request 分发回具体硬件队列

因此它能修正“每个 hctx 各看各的”带来的局部性问题。

### 它具体如何嵌进 blk-mq

大致流程是：

1. 请求到来时，`dd_insert_requests()` 把 request 放入 scheduler 内部结构
2. 运行硬件队列时，`blk_mq` 调用 scheduler 的 `dispatch_request()`
3. `mq-deadline` 从自己的全局结构里挑一个最合适的 request
4. `blk-mq` 再为这个 request 分配 driver tag，并送到对应 `hctx` / driver

还有一个很关键的点：

`mq-deadline` 会限制异步请求和写请求占用的 tag 深度，不让它们把同步读堵死。

这也是它改善读延迟的重要手段之一。直白说就是：

`不要让后台写和异步 I/O 把有限的并发槽位先占满。`

### 所以它的“提升”本质上是什么

可以压缩成一句话：

`mq-deadline 用少量调度开销，换取更好的请求形状、更稳定的延迟和更可控的读写公平性。`

不是所有场景都提高 IOPS 峰值，但它经常能提高：

- 混合读写下的稳定性
- 尾延迟
- HDD 或顺序性敏感设备的吞吐
- 高并发时的可预测性

### 什么时候收益大，什么时候不大

收益通常更大：

- HDD
- SATA SSD
- 混合读写明显
- 延迟抖动明显
- 有后台写回、journal、metadata 更新的场景
- 多进程/多 CPU 并发随机 I/O

收益可能不大甚至变差：

- 高端 NVMe，设备内部调度已经很强
- 纯顺序吞吐压测
- 只追求极限 IOPS，不关心尾延迟
- 调度器本身的锁和重排成本高于收益

所以很多 NVMe 默认会更偏向 `none`，而不是一定上 `mq-deadline`。

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
