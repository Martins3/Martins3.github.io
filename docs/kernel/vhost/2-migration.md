# vhost 热迁移
<!-- c30ceafd-32b2-467a-a55b-bbc5b3b369e8 -->

在 docs/interop/vhost-user.rst 的语境里，vhost-user 为热迁移要解决的核心其实有 3 类问题：guest 内存一致性、virtqueue
执行位置一致性、后端私有状态一致性。dirty page 跟踪就是第一类里最关键的一项，因为 vhost 后端会绕过 vCPU，直接改 guest 内存。

docs/interop/vhost-user.rst
```txt
Migration
---------
During live migration, the front-end may need to track the modifications
the back-end makes to the memory mapped regions. The front-end should mark
the dirty pages in a log. Once it complies to this logging, it may
declare the ``VHOST_F_LOG_ALL`` vhost feature.
To start/stop logging of data/used ring writes, the front-end may send
messages ``VHOST_USER_SET_FEATURES`` with ``VHOST_F_LOG_ALL`` and
``VHOST_USER_SET_VRING_ADDR`` with ``VHOST_VRING_F_LOG`` in ring's
flags set to 1/0, respectively.
...
In postcopy migration the back-end is started before all the memory has
been received from the source host, and care must be taken to avoid
accessing pages that have yet to be received.
```



为什么必须有 dirty page 跟踪

- 热迁移尤其是 precopy 时，QEMU 一边拷内存，一边 guest 和 vhost 后端还在继续运行。
- vhost 后端会直接写 guest 内存，比如把网卡收到的数据写进 guest buffer，或者把完成状态写进 used ring。这些写入不是 guest CPU 发起的，QEMU
  不能天然知道哪些页被后端改过。
- 如果不记脏，某一页在第一轮已经复制到目标端后，源端后端又改了它，但迁移层不知道它变了，就不会在下一轮重传。结果目标端拿到的是旧数据。
- 这会直接破坏设备语义。例如 virtio-net RX：包数据页没重传，或者 used ring
  的更新没重传，目标端驱动可能看不到完成、看到旧数据，轻则丢包，重则队列状态错乱。

可以把它理解成：vhost 后端像一个在 guest RAM 上做 DMA 的实体，热迁移必须把这类“设备写内存”的变化也纳入脏页集合。


这个文档里与热迁移直接相关的支持

• Dirty logging：VHOST_F_LOG_ALL、VHOST_USER_SET_LOG_BASE、VHOST_USER_SET_LOG_FD、VHOST_VRING_F_LOG。这是为了追踪后端对 guest buffer 和 used ring
  的写入。
• 队列冻结/静止点：通过 VHOST_USER_GET_VRING_BASE 停止 vring；当所有 vring 都停下后，设备进入 suspended 状态，此时后端不得再写 guest
  内存、不得再通知 guest。这给迁移切换提供了一个稳定时刻。
• 队列进度迁移：SET/GET_VRING_BASE 要把“处理到哪个 descriptor/idx”一起带到目标端，否则目标端可能重复处理请求，或者漏处理请求。
• 后端设备私有状态迁移：VHOST_USER_PROTOCOL_F_DEVICE_STATE、VHOST_USER_SET_DEVICE_STATE_FD、VHOST_USER_CHECK_DEVICE_STATE。因为仅迁移 RAM 和 vring
  idx 还不够，后端自己可能还有内部状态，例如控制面状态、设备上下文、未落到 guest RAM 的软件状态。
• 配置空间迁移：SET_CONFIG 的 live migration 标志允许在目标端恢复一些平时只读的配置字段。
• Postcopy 支持：VHOST_USER_PROTOCOL_F_PAGEFAULT 和 POSTCOPY_* 消息。目标端后端先启动时，内存可能还没全到，必须靠 userfaultfd
  在访问缺页时阻塞并等待页面到达。
• Inflight I/O tracking：GET/SET_INFLIGHT_FD，以及
  GET_VRING_BASE_INFLIGHT。它最初是为“后端重连/崩溃恢复”描述的，但对迁移也很重要，因为停止队列时可能还有未完成
  I/O；如果不跟踪，恢复后可能不知道哪些请求需要继续完成或重放。


 docs/interop/vhost-user.rst lines 483-491

```txt
Suspended device state
^^^^^^^^^^^^^^^^^^^^^^
While all vrings are stopped, the device is *suspended*.  In addition to
not processing any vring (because they are stopped), the device must:
* not write to any guest memory regions,
* not send any notifications to the guest,
* not send any messages to the front-end,
```


可以这样理解整套需求

• dirty page 跟踪 解决“哪些 guest RAM 被后端改过”。
• suspend + vring base 解决“切换瞬间队列停在什么位置”。
• device state 解决“后端内部还有哪些状态不在 guest RAM 里”。
• postcopy pagefault 解决“目标端还没拿到页面时，后端怎么安全访问内存”。
• inflight I/O 解决“切换时还有没做完的请求怎么办”。

## 是如何记录 dirty 结果的
<!-- 3fa5b64a-a421-4ec7-857a-a927751618f3 -->

(看上去的确是这么回事，也就是共享一个空间过来，然后让 spdk 每次修改内存的时候，自动标记一下
那些页面是 dirty 的，就是这样了)

> [!NOTE]
> 参考神奇海螺的意见，有待验证

在 vhost-user 里，不是后端自己“上报一串脏页列表”给 QEMU，而是 QEMU 先提供一块共享的 dirty log 位图内存，后端在写 guest 内存时直接把对应 bit
置脏。QEMU 之后去读这块位图，就知道哪些 guest page 被 vhost 后端改过了。

docs/interop/vhost-user.rst lines 537-553

```rst
To start/stop logging of data/used ring writes, the front-end may send
messages ``VHOST_USER_SET_FEATURES`` with ``VHOST_F_LOG_ALL`` and
``VHOST_USER_SET_VRING_ADDR`` with ``VHOST_VRING_F_LOG`` in ring's
flags set to 1/0, respectively.
...
The log memory fd is provided in the ancillary data of
``VHOST_USER_SET_LOG_BASE`` message when the back-end has
``VHOST_USER_PROTOCOL_F_LOG_SHMFD`` protocol feature.
...
to mark page at ``addr`` as dirty::
  page = addr / VHOST_LOG_PAGE
  log[page / 8] |= 1 << page % 8
Where ``addr`` is the guest physical address.
Use atomic operations, as the log may be concurrently manipulated.
```

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
• 脏页粒度是固定的 4KB，不是按字节或请求记录。
• 记录的是 后端写 guest RAM 的页，包括 descriptor 指向的数据页；如果队列启用了 VHOST_VRING_F_LOG，还要记录 used ring 的写入。

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

(看上去是不需要的)

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
