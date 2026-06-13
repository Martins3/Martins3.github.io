## qemu rcu
<!-- 09eae8f9-105a-4702-b839-a5c7fdfa39cf -->

- [QEMU RCU 文档](https://github.com/qemu/qemu/blob/master/docs/devel/rcu.txt)
	- https://qemu.readthedocs.io/en/v10.0.3/devel/rcu.html
- [terenceli 的 blog : QEMU RCU implementation](https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2021/03/14/qemu-rcu)

## 继续看看文档再说吧
- qatomic_rcu_set 什么作用


## 经典案例

### RAMList::dirty_memory

```c
typedef struct RAMList {
    // ...
    DirtyMemoryBlocks *dirty_memory[DIRTY_MEMORY_NUM];
    // ...
}
```
从 writer 的角度分析，做了两件事情
- 让 RAMList::dirty_memory 存储新的 DirtyMemoryBlocks 地址
- 释放老的 DirtyMemoryBlocks

```c
static void dirty_memory_extend(ram_addr_t old_ram_size,
                                ram_addr_t new_ram_size){
        new_blocks = g_malloc(sizeof(*new_blocks) +
                              sizeof(new_blocks->blocks[0]) * new_num_blocks);
        qatomic_rcu_set(&ram_list.dirty_memory[i], new_blocks);

        g_free_rcu(old_blocks, rcu);
}
```
- 如果一个 reader 从 RAMList::dirty_memory 中获取的就是新的 DirtyMemoryBlocks 地址，之后一切访问正常。
- 如果一个 reader 在更新 RAMList::dirty_memory 之前访问，获取的是旧的的 DirtyMemoryBlocks，现在是不能立刻将其释放掉的。需要等待 reader 都结束了才可以释放。
- 无论上面的哪一个情况，reader 通过 RAMList::dirty_memory 获取的 DirtyMemoryBlocks 总是 atomic 状态的，而不是一部分修改了，一部分没有修改，这是正确性的保证。
- g_free_rcu 是对于 call_rcu1 的简单包装，将需要调用释放函数推迟操作。

```c
void call_rcu1(struct rcu_head *node, void (*func)(struct rcu_head *node))
{
    node->func = func;
    enqueue(node);
    qatomic_inc(&rcu_call_count);
    qemu_event_set(&rcu_call_ready_event);
}
```
推迟的时间当然是等待所有的 reader 都结束才可以。

再看 reader 这一侧，使用 cpu_physical_memory_get_dirty 作为例子:

```c
static inline bool cpu_physical_memory_get_dirty(ram_addr_t start,
                                                 ram_addr_t length,
                                                 unsigned client)
{
    WITH_RCU_READ_LOCK_GUARD() {
      // 访问
    }
    return dirty;
}
```

这里还使用了 QLIST_NEXT_RCU ，也是经典位置了

### virtioqueue
```c
void virtqueue_push(VirtQueue *vq, const VirtQueueElement *elem,
                    unsigned int len)
{
    RCU_READ_LOCK_GUARD();
    virtqueue_fill(vq, elem, len, 0);
    virtqueue_flush(vq, 1);
}
```

应该配合使用的地方为:

virtio_init_region_cache
```c
    qatomic_rcu_set(&vq->vring.caches, new);
    if (old) {
        call_rcu(old, virtio_free_region_cache, rcu);
    }
```

## 问题

在 `call_rcu_thread` 中，需要持有 lock 才可以释放资源，这很奇怪。既然都是可以开始来执行 hook 函数了，
说明这些资源已经是没有人使用的，
那么为什么还需要使用 BQL 保护。
其原因在: https://lists.gnu.org/archive/html/qemu-devel/2015-02/msg03170.html

## 原理

WITH_RCU_READ_LOCK_GUARD 会展开为:

```txt
- rcu_read_auto_lock
  - rcu_read_lock
    - `rcu_reader->ctr = rcu_gp_ctr->ctr` : 在进入的时候同步 global 的 ctr 到本地，这样如果 global 的发生变动了，那么就可以检测出来

// 中间进行访问

- rcu_read_auto_unlock
  - rcu_read_unlock
    - 如果检测到 rcu_reader::waiting 的话，`qemu_event_set(&rcu_gp_event);`，通知 call_rcu thread 有 reader 结束了
```

先总结一下关联到几个主要结构体:

| 名称                 | 作用                                                                               |
|----------------------|------------------------------------------------------------------------------------|
| rcu_gp_ctr           | 全局变量，用于标记当前的 period                                                    |
| rcu_reader           | 每一个线程的局部变量，当 reader 进入 critical reagion 的时候，会和 rcu_gp_ctr 同步 |
| rcu_call_ready_event | 在 call_rcu1 中用于通知 `call_rcu` thread 有垃圾可以回收了                         |
| rcu_gp_event         | 在 rcu_read_unlock 中用于通知 `call_rcu` thread 有 reader 结束了                   |

reader 和 writer 都是和 call_rcu thread 来交互的:

- call_rcu_thread : 一个死循环，用户回收
  - 第一个 while 循环: 需要等待 writer 调用 call_rcu1 才可以, 然后等待一段时间
  - synchronize_rcu
    - 修改 rcu_gp_ctr, 表示进入回收的 period 了，如果一些 reader 正好在 critical region 中，那么因为 `rcu_reader->ctr` 和 `rcu_gp_ctr->ctr` 不相等而可以识别出来
    - wait_for_readers
      1. `static ThreadList registry = QLIST_HEAD_INITIALIZER(registry);` : 在 rcu_register_thread 的时候，将 thread local 的 rcu_reader 挂到上面去
      2. 对于 register 上挂载的 rcu_reader 调用 rcu_gp_ongoing 查询 local 的 `rcu_reader->ctr` 和 global 的 `rcu_gp_ctr->ctr` 是否存在差别，如果有，那么设置 rcu_reader_data::waiting 为 true, 如果版本相同，那么从 registry 中移除掉
      3. QLIST_EMPTY(&registry) : 这表示所有的 reader 都离开 critical region 了
  - try_dequeue && `node->func(node)` : 从队列中间取出需要执行的函数来, 这些执行函数就是进行垃圾回收



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
