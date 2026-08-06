# vhost

## 为什么需要 vhost 参与热迁移

先来思考下，为什么需要 memory_region_set_dirty() 函数

1. kvm 的 dirty page tracking 是仅仅跟踪 ept page table 的

2. 此外通过正常 address_space_write() 写 RAM 时，QEMU 会自动执行 dirty 标记和 TCG TB 失效

但下面这些修改绕过了正常内存访问路径：

memcpy(memory_region_get_ram_ptr(mr) + offset, buf, len);

或者：

- vhost 内核后端完成了 DMA；
- Spice 线程直接修改显存；
- TPM backend 修改共享 command buffer；
- ACPI 表在 host 侧被重新生成；
- NVDIMM label 通过 host pointer 更新。

KVM/TCG 看不到这种 host-side 写入，所以设备必须补一次 memory_region_set_dirty()。

memory_region_set_dirty() 不是“给 MemoryRegion 设置一个 dirty 状态”，而是
某段 RAM 已被 QEMU、设备线程或外部后端直接修改，请把覆盖到的页通知给当前所有关心内存变化的消费者。

## vhost 热迁移基本流程
<!-- c30ceafd-32b2-467a-a55b-bbc5b3b369e8 -->

在 docs/interop/vhost-user.rst 的语境里，vhost-user 为热迁移要解决的核心其实有 3 类问题：
1. guest 内存一致性、
2. virtqueue 执行位置一致性
3. 后端私有状态一致性。

dirty page 跟踪就是第一类里最关键的一项，因为 vhost 后端会绕过 vCPU，直接改 guest 内存。

docs/interop/vhost-user.rst

这个文档里与热迁移直接相关的支持
- Dirty logging：VHOST_F_LOG_ALL、VHOST_USER_SET_LOG_BASE、VHOST_USER_SET_LOG_FD、VHOST_VRING_F_LOG。这是为了追踪后端对 guest buffer 和 used ring 的写入。
- 队列冻结/静止点：通过 VHOST_USER_GET_VRING_BASE 停止 vring；当所有 vring 都停下后，设备进入 suspended 状态，此时后端不得再写 guest 内存、不得再通知 guest。这给迁移切换提供了一个稳定时刻。
- 队列进度迁移：SET/GET_VRING_BASE 要把“处理到哪个 descriptor/idx”一起带到目标端，否则目标端可能重复处理请求，或者漏处理请求。
- 后端设备私有状态迁移：VHOST_USER_PROTOCOL_F_DEVICE_STATE、VHOST_USER_SET_DEVICE_STATE_FD、VHOST_USER_CHECK_DEVICE_STATE。因为仅迁移 RAM 和 vring idx 还不够，后端自己可能还有内部状态，例如控制面状态、设备上下文、未落到 guest RAM 的软件状态。
- 配置空间迁移：SET_CONFIG 的 live migration 标志允许在目标端恢复一些平时只读的配置字段。
- Postcopy 支持：VHOST_USER_PROTOCOL_F_PAGEFAULT 和 POSTCOPY_* 消息。目标端后端先启动时，内存可能还没全到，必须靠 userfaultfd 在访问缺页时阻塞并等待页面到达。
- Inflight I/O tracking：GET/SET_INFLIGHT_FD，以及 GET_VRING_BASE_INFLIGHT。它最初是为“后端重连/崩溃恢复”描述的，但对迁移也很重要，因为停止队列时可能还有未完成 I/O；如果不跟踪，恢复后可能不知道哪些请求需要继续完成或重放。

在 vhost-user 里，不是后端自己“上报一串脏页列表”给 QEMU，而是 QEMU 先提供一块共享的 dirty log 位图内存，
后端在写 guest 内存时直接把对应 bit
置脏。QEMU 之后去读这块位图，就知道哪些 guest page 被 vhost 后端改过了。

大致流程是这样：
1. QEMU 在开始迁移、需要记录设备写内存时，开启 VHOST_F_LOG_ALL，并按需给某个 vring 打开 VHOST_VRING_F_LOG。
	- VHOST_F_LOG_ALL 负责的是“后端对 guest 数据页的写需要记脏”。
	- VHOST_VRING_F_LOG 主要是额外控制“used ring 的写是否也要记脏”。
2. QEMU 通过 VHOST_USER_SET_LOG_BASE 把一块共享内存的 fd 传给后端。
3. 后端把这个 fd mmap 成一块 bitmap。
4. 之后后端每次写 guest 内存时，按 guest physical address 算出对应的 4K 页号，把 bitmap 里对应 bit 置 1。
5. 如果配置了 VHOST_USER_SET_LOG_FD，后端还可以通过 eventfd 通知 QEMU“log 变了”，让 QEMU 更快收集。
6. QEMU 读取这块 bitmap，把这些页并入迁移层的 dirty pages 集合，再安排下一轮重传。

关键点有两个：
- 脏页粒度是固定的 4KB，不是按字节或请求记录。
- 记录的是 后端写 guest RAM 的页，包括 descriptor 指向的数据页；如果队列启用了 VHOST_VRING_F_LOG，还要记录 used ring 的写入。

还有一个容易忽略的点：

 docs/interop/vhost-user.rst lines 567-570

```txt
Note that when logging modifications to the used ring (when
``VHOST_VRING_F_LOG`` is set for this ring), ``log_guest_addr`` should
be used to calculate the log offset: the write to first byte of the
used ring is logged at this offset from log start.
```

也就是说，used ring 的日志地址计算不一定直接用普通 guest RAM 映射，而是用 vring addr 里的 log_guest_addr 语义去算，这样 QEMU
才能正确知道队列状态页脏了。
所以本质上：
• QEMU 负责分配和读取 dirty log
• 后端 负责在写内存时原子置位
• 两者通过共享内存位图协作，而不是靠后端主动发送“脏页列表”消息

如果你想，我可以继续把这个流程和 QEMU 迁移里的 memory_global_dirty_log / used ring 一起对照讲一遍。

### 热迁移的访问的时候，需要 qemu 和 vhost 停下来吗?
<!-- e0c76777-e182-4f46-b65d-06352d835b2c -->

一般来说，不需要后端感知，也不需要后端停下来。
这里的设计本来就是为并发访问准备的：
• 后端一边继续处理 I/O，一边用原子操作把 bitmap 对应 bit 置 1
• QEMU 另一边周期性地去扫这块 shared memory，把已经置位的 bit 读出来并清掉
• 两边默认就是可以同时操作这块 bitmap 的

协议文档其实明确提示了这一点：log 可能被并发修改，所以后端置位时要用 atomic 操作。
QEMU 这边实现也是按这个思路做的，它不是简单读，而是原子地把一整个 log chunk 取出来并清零：

hw/virtio/vhost.c lines 86-106
```c
  static void vhost_dev_sync_region(struct vhost_dev *dev,
                                    MemoryRegionSection *section,
                                    uint64_t mfirst, uint64_t mlast,
                                    uint64_t rfirst, uint64_t rlast)
  {
      vhost_log_chunk_t *dev_log = dev->log->log;
      ...
      for (;from < to; ++from) {
          vhost_log_chunk_t log;
          if (!*from) {
              addr += VHOST_LOG_CHUNK;
              continue;
          }
          log = qatomic_xchg(from, 0);
          while (log) {
              ...
              memory_region_set_dirty(section->mr, mr_offset, VHOST_LOG_PAGE);
              ...
          }
      }
  }
```
后端置位也是原子的：

subprojects/libvhost-user/libvhost-user.c lines 721-737
```c
static void
vu_log_page(uint8_t *log_table, uint64_t page)
{
    qatomic_or(&log_table[page / 8], 1 << (page % 8));
}
static void
vu_log_write(VuDev *dev, uint64_t address, uint64_t length)
{
    ...
    while (page * VHOST_LOG_PAGE < address + length) {
        vu_log_page(dev->log_table, page);
        page += 1;
    }
    vu_log_kick(dev);
}
```

所以关键点是：
- pre-copy 期间：后端通常不停，QEMU 随时可以直接读这块 shared bitmap
- 不需要专门通知后端“我要读了”
- 不会因为并发访问就天然丢脏页，前提是双方按协议做原子更新
- 如果某页在 QEMU 清零之后又被后端写了，后端会再次把 bit 置 1，QEMU 下一轮还能看到

只有到最终 stop-and-copy / source 完成迁移那个阶段，设备/ring 才会被停住，此时才要求不再继续更新 log。

## TODO

1. 几种 virtio 设备都是如何处理热迁移的:
	1. vhost-net
	2. virtio-blk
	3. vhost user virtio-blk

2. 看看 vhost 对于热迁移的优化
	- https://patchew.org/QEMU/20250813164856.950363-1-vsementsov@yandex-team.ru/

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
