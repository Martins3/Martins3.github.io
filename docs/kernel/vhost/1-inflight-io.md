# vhost-user Inflight I/O Tracking 详解
<!-- 00c7d91c-28a4-4ec9-a9cd-14de06c2942c -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

感觉的确是和乱序提交有关，但是细节我没太想清楚。

## 背景

`Inflight I/O tracking` 解决的是这样一个问题：

1. Guest 已经把请求放进 virtqueue。
2. vhost-user 后端已经取走请求，并开始处理。
3. 但后端还没有把这个请求正式提交到 used ring，或者提交过程只做了一半。
4. 此时后端进程崩溃、重启，或者需要重新连接。

如果没有额外状态，新的后端通常无法准确区分下面几类请求：

- 还在 avail ring 中、尚未被旧后端取走的请求
- 已经被旧后端取走、但尚未完成的请求
- 已经完成到一半、guest 侧可见状态与后端内部状态不一致的请求

这个问题在后端支持乱序处理、批量提交、packed virtqueue 时尤其明显。因为仅依赖 virtqueue 本身的 ring 内容，很多中间状态可能已经被覆盖，恢复时无法可靠重建。

所以 vhost-user 协议引入了一个共享内存缓冲区，用来记录“后端已经接手但尚未完全提交”的请求信息。这个共享区在前后端之间传递，并能在后端崩溃后继续保留，供新后端恢复使用。

docs/interop/vhost-user.rst
```txt
Inflight I/O tracking
---------------------
To support reconnecting after restart or crash, back-end may need to
resubmit inflight I/Os. If virtqueue is processed in order, we can
easily achieve that by getting the inflight descriptors from
descriptor table (split virtqueue) or descriptor ring (packed
virtqueue). However, it can't work when we process descriptors
out-of-order because some entries which store the information of
inflight descriptors in available ring (split virtqueue) or descriptor
ring (packed virtqueue) might be overridden by new entries. To solve
this problem, the back-end need to allocate an extra buffer to store this
information of inflight descriptors and share it with front-end for
persistent. ``VHOST_USER_GET_INFLIGHT_FD`` and
``VHOST_USER_SET_INFLIGHT_FD`` are used to transfer this buffer
```

## 协议目标

这个机制记录的不是数据内容本身，而是 virtqueue 层面的请求状态，目标是让后端在重启后做到：

- 找出哪些请求在崩溃前处于 inflight 状态
- 判定最后一批提交是否已经对 guest 生效
- 按合适顺序重新提交这些请求

它不能单独保证下面这些更强的语义：

- 底层设备状态一定能完整回滚
- 所有 I/O 一定严格 exactly-once
- 后端内部事务一定无副作用

它提供的是 virtqueue 层面的恢复基础，而不是后端设备层面的完整日志系统。

## 共享区的传递方式

这个能力依赖协议特性 `VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD`。

协商成功后：

1. 前端通过 `VHOST_USER_GET_INFLIGHT_FD` 向后端要一块共享缓冲区。
2. 后端返回一个 fd，以及 `mmap_size` 和 `mmap_offset`。
3. 前端把这块区域映射到自己的地址空间，并持有这个 fd。
4. 如果后端后续崩溃并重启，前端再通过 `VHOST_USER_SET_INFLIGHT_FD` 把同一块共享区传回给新后端。

对 `QEMU + SPDK` 这类典型链路，可以这样理解：

- 后端负责创建或维护 inflight 共享区
- QEMU 在连接期间持有这个共享区 fd
- 后端死掉后，QEMU 仍然保有这块共享区
- 新后端上线后，QEMU 把这块共享区重新交给它

因此，这个机制的“持久性”不是把状态写到磁盘，而是通过前端继续持有共享内存对象来保住状态。

## Inflight 描述头

消息里传递的是一个 `VhostUserInflight` 描述头：

```c
typedef struct VhostUserInflight {
    uint64_t mmap_size;
    uint64_t mmap_offset;
    uint16_t num_queues;
    uint16_t queue_size;
} VhostUserInflight;
```

字段含义：

- `mmap_size`：整块 inflight 区域大小
- `mmap_offset`：在共享 fd 中的偏移
- `num_queues`：virtqueue 数量
- `queue_size`：每个 virtqueue 的大小

逻辑布局上，这块区域会被分成多个 queue region：

```text
+---------------+---------------+-----+---------------+
| queue0 region | queue1 region | ... | queueN region |
+---------------+---------------+-----+---------------+
```

每个 virtqueue 都有自己对应的一段 inflight 跟踪区。

## 为什么仅靠 virtqueue 本身不够

协议文档指出，如果 virtqueue 总是严格按顺序处理，那么重启后可以尝试从 ring 本身恢复状态；但一旦后端支持乱序处理，就会有问题：

- split ring 下，avail ring 或 used ring 中的信息可能被后续请求覆盖
- packed ring 下，avail 和 used 都复用同一组 descriptor，更容易丢掉中间状态
- 批量提交时，最后一批请求可能已经部分写到 guest 可见状态，但 inflight 标志还没来得及更新

所以协议要求后端维护一份额外的 inflight 账本，而不是依赖 ring 当前内容做推测。

## Split Virtqueue 的设计

split virtqueue 的每个 descriptor 对应一个 `DescStateSplit` 条目：

```c
typedef struct DescStateSplit {
    uint8_t inflight;
    uint8_t padding[5];
    uint16_t next;
    uint64_t counter;
} DescStateSplit;
```

它所在的 queue region 结构大致是：

```c
typedef struct QueueRegionSplit {
    uint64_t features;
    uint16_t version;
    uint16_t desc_num;
    uint16_t last_batch_head;
    uint16_t used_idx;
    DescStateSplit desc[];
} QueueRegionSplit;
```

这些字段的关键作用如下。

### `inflight`

只对 head descriptor 有意义，表示这个请求是否仍在处理中。

### `counter`

记录后端从 avail ring 取走请求的顺序。恢复后需要按 `counter` 顺序重新提交，以尽量保留原始取队顺序。

这点很重要，因为后端虽然可以乱序执行，但恢复时仍然需要一个稳定顺序来重放 inflight 请求。

### `next` 和 `last_batch_head`

这两个字段用于把“最后一批已经写到 used ring 的请求”串成一个链表。

原因是 split ring 下有一个危险窗口：

1. 后端已经把一批请求写入 used ring。
2. `used->idx` 可能也已经增加。
3. 但共享区里的 `inflight` 标志还没来得及清零。
4. 这时后端崩溃。

如果恢复时仅看 `inflight=1`，会把已经完成的最后一批请求误认为还没完成。

因此协议要求在批量提交 used buffer 时：

- 先把本批次请求通过 `next` 串起来
- 通过 `last_batch_head` 记住链表头
- 用 `used_idx` 记录共享区视角下已经提交到哪一个 used ring 索引

恢复时如果发现：

- 共享区里的 `used_idx`
- 与 guest 实际 used ring 的 `idx`

不一致，就说明最后一批提交可能出现了“guest 已看见，但 inflight 标志还没清”这种部分完成状态。此时恢复逻辑会沿着 `last_batch_head` 链表修正这批请求，把它们从 inflight 集合里去掉，再对真正未完成的请求做 resubmit。

### Split ring 的处理流程

简化后可以理解成下面几步。

当后端从 avail ring 取到一个请求时：

1. 找到 head descriptor 索引 `i`
2. 给 `desc[i].counter` 赋当前全局计数器
3. 全局计数器加一
4. 把 `desc[i].inflight` 置为 1

当后端把请求提交到 used ring 时：

1. 把对应 head descriptor 挂到“最后一批”链表上
2. 增加 used ring 的 `idx`
3. 把本批次相关条目的 `inflight` 清零
4. 更新共享区里的 `used_idx`

恢复时：

1. 先检查共享区 `used_idx` 和真实 used ring `idx` 是否一致
2. 若不一致，修正最后一批完成请求的 inflight 状态
3. 按 `counter` 顺序重提剩余 inflight 请求

## Packed Virtqueue 的设计

packed ring 更复杂，因为 avail 和 used 共用同一组 descriptor ring，状态更新不是简单地推进两个独立 ring 指针，而是：

- 更新 descriptor flags
- 更新 used index
- 维护 wrap counter
- 处理一个请求对应的 descriptor chain

所以 packed ring 记录的信息明显更多：

```c
typedef struct DescStatePacked {
    uint8_t inflight;
    uint8_t padding;
    uint16_t next;
    uint16_t last;
    uint16_t num;
    uint64_t counter;
    uint16_t id;
    uint16_t flags;
    uint32_t len;
    uint64_t addr;
} DescStatePacked;
```

对应 queue region 还带有额外的恢复辅助状态：

```c
typedef struct QueueRegionPacked {
    uint64_t features;
    uint16_t version;
    uint16_t desc_num;
    uint16_t free_head;
    uint16_t old_free_head;
    uint16_t used_idx;
    uint16_t old_used_idx;
    uint8_t used_wrap_counter;
    uint8_t old_used_wrap_counter;
    uint8_t padding[7];
    DescStatePacked desc[];
} QueueRegionPacked;
```

### 为什么 packed ring 需要更多字段

因为恢复时不只是要知道“某个请求还在 inflight”，还要知道：

- 这个请求对应哪个 descriptor chain
- chain 的末尾在哪里
- chain 长度是多少
- descriptor 的 `id/flags/len/addr` 是什么
- 更新 used 状态时是否已经对 guest 生效

换句话说，packed ring 的 inflight 共享区不只是“标记表”，还承担了部分 descriptor 元信息镜像的职责。

### `old_*` 状态的作用

packed ring 恢复最核心的是这些字段：

- `old_free_head`
- `old_used_idx`
- `old_used_wrap_counter`

它们相当于“正在提交这一批 used 更新之前的快照”。

这样恢复时可以判断：

- 如果 guest 已经看到了本批次 used 更新，就把这次更新视为已提交
- 如果 guest 还没看到，就回滚到 `old_*` 快照

这就是 packed ring 文档里 commit-or-rollback 的核心思想。

### Packed ring 的处理流程

简化后可以理解成：

当后端取到一个新的 descriptor chain 时：

1. 用 `old_free_head` 代表这次请求对应的 inflight 头条目
2. 记录 head 的 `counter`
3. 置 `inflight=1`
4. 把 chain 中每个 descriptor 的关键信息复制到 `DescStatePacked`
5. 更新 `free_head`

当后端完成并提交 used 状态时：

1. 找到这次请求对应的 head 条目
2. 把它占用的条目重新串回 free list
3. 更新 `used_idx` 与 `used_wrap_counter`
4. 更新 packed descriptor 的 used/avail flags
5. 清理 head 的 `inflight`
6. 把 `old_*` 更新到新状态

恢复时：

1. 若 `used_idx != old_used_idx`，说明上次 used 提交可能做了一半
2. 再去看 descriptor flags 是否已经对 guest 可见
3. 若已可见，则提交到新状态
4. 若未可见，则回滚到 `old_*` 快照
5. 把 free list 中的条目视为非 inflight
6. 按 `counter` 顺序重提真正仍在 inflight 的请求

## QEMU 侧如何处理共享区

在 `QEMU` 里，前端会在协商了 `VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD` 后请求这块共享区。

大致流程如下：

1. 发 `VHOST_USER_GET_INFLIGHT_FD`
2. 读取返回的 `VhostUserInflight`
3. 从消息 fd 中取出 memfd/shmfd
4. `mmap` 到本地地址空间
5. 保存 `addr/fd/size/offset`

后续如果后端重启，QEMU 再通过 `VHOST_USER_SET_INFLIGHT_FD` 把这块共享区交给新的后端实例。

这说明 QEMU 在这里扮演的是“共享区保管者”的角色，而不是 inflight 内容的主要维护者。真正往共享区里写 descriptor 状态的通常还是后端及其运行库。

## SPDK 中的实现方式

`SPDK` 自己并没有手写整套 inflight 协议细节，而是主要依赖 `DPDK rte_vhost`。

从源码看：

- `vhost-blk` 支持 `VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD`
- `vhost-scsi` 也支持 `VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD`
- 队列初始化时，SPDK 通过 DPDK 取回 inflight ring 状态
- 请求开始处理时，SPDK 调用 DPDK helper 标记 inflight
- 请求完成后，SPDK 调用 DPDK helper 更新 used 状态并清除 inflight
- 重连后，SPDK 从 `resubmit_inflight` 列表中重新提交请求

### 初始化和恢复

SPDK 初始化 virtqueue 时会调用：

- `rte_vhost_get_vhost_ring_inflight()`
- `rte_vhost_get_vring_base_from_inflight()`

其中 packed ring 下尤其依赖 `rte_vhost_get_vring_base_from_inflight()` 来恢复 `last_avail_idx` 和 `last_used_idx`。

### 标记 inflight

请求被取出开始处理时：

- split ring 调用 `rte_vhost_set_inflight_desc_split()`
- packed ring 调用 `rte_vhost_set_inflight_desc_packed()`

### 完成请求

请求完成写回时：

- split ring 先 `rte_vhost_set_last_inflight_io_split()`，再 `rte_vhost_clr_inflight_desc_split()`
- packed ring 先 `rte_vhost_set_last_inflight_io_packed()`，再 `rte_vhost_clr_inflight_desc_packed()`

这说明“写 used ring”和“清 inflight 状态”之间是有明确顺序的，正是为了在崩溃时能判断最后一批请求到底算已提交还是未提交。

### 重新提交 inflight 请求

恢复时，SPDK 会从：

- `vq->vring_inflight.resubmit_inflight`

里拿到一份 `resubmit_list`，然后遍历这些请求重新进入正常处理流程。

这表示真正的“找出哪些请求需要重跑”的逻辑，底层由 DPDK inflight 恢复逻辑准备好，SPDK 负责把这些请求重新送回自己的请求处理路径。

## 一个典型恢复场景

以 `QEMU + SPDK vhost-blk` 为例，可以把一次崩溃恢复抽象成下面的过程：

1. Guest 把若干 block 请求放入 virtqueue。
2. SPDK 从 avail ring 中取走这些请求，并把对应条目标记为 inflight。
3. 某些请求已经提交到底层 bdev，但还没来得及完全写回 used ring；或者已经写回了一半。
4. SPDK 进程崩溃。
5. QEMU 仍然持有 inflight shmfd。
6. 新的 SPDK 进程启动并重新连接。
7. QEMU 通过 `SET_INFLIGHT_FD` 把旧共享区交给新 SPDK。
8. DPDK 根据共享区状态，恢复每个 virtqueue 的 inflight 信息，并构造 `resubmit_list`。
9. SPDK 遍历 `resubmit_list`，按恢复出的顺序重新提交这些尚未真正完成的请求。

如果最后一批请求在旧后端崩溃前其实已经对 guest 生效，那么 inflight 恢复逻辑应当把它们识别为“已提交”，而不是再次重放。

## 和普通 ring 状态恢复的区别

普通的 ring 恢复更像是恢复：

- 当前 avail 指针在哪里
- 当前 used 指针在哪里
- 下一个要处理的索引是什么

而 inflight tracking 恢复的是：

- 哪些请求已经被后端接手
- 哪些请求仍在处理中
- 哪些请求处于“最后一次提交的中间状态”
- 如果需要重跑，应按什么顺序重跑

所以它不是简单的 `last_avail_idx/last_used_idx` 备份，而是一套面向“未决请求重建”的机制。

## 和 `GET_VRING_BASE_INFLIGHT` 的关系

协议还规定，在停止 vring 时，后端默认应该先完成所有 inflight I/O，再停止队列。

但如果协商了 `VHOST_USER_PROTOCOL_F_GET_VRING_BASE_INFLIGHT`，后端可以不强制完成所有 inflight I/O，而是把这些请求挂起并记录到 inflight 区中，之后在恢复时继续处理。

这说明 inflight tracking 的用途不仅是“进程崩溃后的重连恢复”，也包括：

- 后端停止后再恢复
- 某些迁移/重建场景下的未决请求保存

## 一句话总结

`Inflight I/O tracking` 可以理解为：

前端替后端保管一份“已经接手但还没完全对 guest 提交”的 virtqueue 请求账本；后端重启后再把这本账拿回来，继续完成这些请求，而不是盲猜 ring 当前状态。

## 结合当前源码的观察

基于当前 `QEMU` 与 `SPDK` 源码，可以得出几点：

- QEMU 负责请求并保管 inflight shmfd，再在重连时回传
- SPDK 的 vhost-blk 和 vhost-scsi 都支持 `VHOST_USER_PROTOCOL_F_INFLIGHT_SHMFD`
- SPDK 主要借助 DPDK 的 `rte_vhost_*inflight*` 接口维护和恢复 inflight 状态
- packed ring 的恢复明显比 split ring 更复杂，因为需要处理 descriptor flags 和 wrap counter 的半完成状态

## 进一步可跟的源码方向

如果后续还要继续深入，建议重点跟这些逻辑：

- `QEMU` 中 `GET_INFLIGHT_FD` / `SET_INFLIGHT_FD` 的完整调用链
- `DPDK rte_vhost` 如何构造 `resubmit_inflight`
- `SPDK vhost-blk` 在 resubmit 后如何避免与新到达请求产生重复处理
- packed ring 场景下 `old_used_idx` 与 descriptor flags 的一致性判断

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
