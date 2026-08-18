## udmabuf 原理

  - memfd 解决“进程之间共享”
  - udmabuf 解决“把这段共享内存伪装成 dma-buf，交给 GPU/DRM/V4L2 之类的 importer”

  所以什么时候 udmabuf 有意义？

  - 你的数据先在用户态生成
  - 但下游接口要求你给它一个 dma-buf fd
  - 这时就可以 memfd -> udmabuf -> dma-buf fd

  什么时候没意义？

  - 只是两个用户进程共享数据
  - 或者你本来就能直接从 DRM/dma-heap 拿到 dma-buf

  你现在这个 demo证明的是：

  - udmabuf 不是新内存
  - 它只是把 memfd 那组页变成了 dma-buf 语义

  如果你愿意，下一步我可以再补一个“只有 memfd 做不到，但 udmabuf 能做到”的小例子，比如：

  - 用 udmabuf 导出后喂给某个 DRM import 路径
  - 或者至少做一个“memfd 没法 DMA_BUF_IOCTL_SYNC，udmabuf fd 可以”的对照 demo

## udmabuf demo

`udmabuf` 不是去分配 GPU buffer。
它做的事情是：

1. 用户先准备一个 `memfd`
2. `udmabuf` 把这个 `memfd` 对应的页 pin 住
3. 再把这些页包装成一个新的 `dma-buf fd`

所以它更像“把一段用户可见的匿名内存，导出成 dma-buf”，而不是“向 GPU 申请显存”。

演示程序：

```bash
./udmabuf_demo.out
echo a | sudo -S ./udmabuf_demo.out -t 5
```

这个 demo 会做几件事：

- 创建一个带 `F_SEAL_SHRINK` 的 `memfd`
- 把它映射到用户态并写入测试模式
- 通过 `/dev/udmabuf` 的 `UDMABUF_CREATE` ioctl 创建一个新的 `dma-buf fd`
- 再对这个 `dma-buf fd` 执行 `mmap`
- 双向验证：
  - `memfd` 里写的数据，`dma-buf` 映射能看到
  - `dma-buf` 映射里写的数据，原来的 `memfd` 映射也能看到

一次实际输出里，`/proc/self/fdinfo/<fd>` 会显示：

```txt
exp_name:	udmabuf
```

而 `bufinfo` 里对应项类似：

```txt
04194304	00000002	02080007	00000002	udmabuf	00005537	<none>
```

这说明这个对象的 exporter 是 `udmabuf`，和前面 `drm` 导出的对象是两类东西。


## 使用场景
• udmabuf 的典型场景，不是“显卡先分配一块 buffer，用户态去填”，而是反过来：

  - 数据一开始就在用户态 CPU 内存里产生
  - 但后面的内核/设备接口只接受 dma-buf fd
  - 所以需要一个桥，把“用户态提供的页”包装成 dma-buf

  可以把它理解成：

  - memfd 适合“进程 <-> 进程”
  - udmabuf 适合“进程 -> 设备/驱动”

  为什么数据会先由用户态提供？

  因为很多数据源天生就在 CPU 这边，不是设备先产出的。常见几类：

  1. 软件生成的图像/视频帧
     比如测试图、字幕叠加、CPU 软件渲染结果、桌面截图后处理。这些数据先在用户态算法里生成，之后才想交给显示、编码或别的硬
     件模块。
  2. 用户态做了预处理
     比如解压、颜色转换、拼接、裁剪、协议解析、加密前后缓冲。这时原始数据已经在普通 RAM 里了，后面想零拷贝喂给 GPU/V4L2/别
     的 DMA 设备。
  3. 测试和原型验证
     驱动或多媒体链路调试时，经常需要“我先手工造一块数据，再看看 importer 能不能吃”。这时 udmabuf 很方便，因为不需要先搞一
     套 DRM/heap 分配器。
  4. 用户态本来就持有共享内存
     比如某个 IPC/共享内存协议先给了你 memfd，但下游接口要求 dma-buf fd。这时 udmabuf 就是转换器。

  所以它真正解决的是这个问题：

  - “我已经有一段普通用户内存了”
  - “但下游 API 不认普通内存，只认 dma-buf”
  - “那我把这段内存变成 dma-buf 再交过去”

  它的价值主要是：

  - 避免再拷贝一份到别的专用 buffer
  - 接上只支持 dma-buf 的 kernel/device 接口
  - 做零拷贝或少拷贝的数据流

  但也要注意它不是万能的：

  - udmabuf 背后还是系统内存，不是 GPU 专用显存
  - importer 能不能高效使用，取决于那个设备/驱动是否愿意 import 这种页列表
  - 有些设备即使能 import，内部也可能再搬一次

  所以最准确的一句话是：

  - udmabuf 的用途不是“共享内存”
  - 而是“把用户态已有的共享内存，变成设备世界能接受的 dma-buf”

  如果你愿意，我下一步可以直接给你写一个更像真实场景的 demo：
  memfd -> udmabuf -> DRM PRIME import
  这样就不是只证明“两个映射看到同一块内存”，而是证明“用户态造的数据，真的能变成 DRM 侧能 import 的 dma-buf”。

## qemu 中就是支持 udmabuf 的
hw/display/virtio-gpu-udmabuf.c
contrib/vhost-user-gpu/vugbm.c
hw/display/virtio-dmabuf.c

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
# 传统路径
用户态 malloc() → 拷贝 → 内核 buffer → DMA → 设备

# udmabuf 的路径
用户态 malloc() → udmabuf → dma-buf → 设备 DMA
                ↑          ↑
            无拷贝     跨子系统共享
```
