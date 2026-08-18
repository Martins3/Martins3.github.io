这个小程序现在不再只盯着 `virtio-gpu`，而是把问题拆成两层：

1. 先在任意支持 dumb buffer 的 DRM 节点上分配 BO。
2. 再把这个 BO 导出成 `dma-buf`，观察它在 `fdinfo` 和 `debugfs` 里的样子。

核心结论：

- `dma-buf` 不是“virtio-gpu 专属”，它只是一个跨驱动共享 buffer 的内核抽象。
- `/sys/kernel/debug/dma_buf/bufinfo` 里的 `exp_name` 常常只是 `drm`，表示导出者属于 DRM 子系统。
- `exp_name=drm` 不能告诉你具体是 `virtio_gpu`、`xe` 还是 `nvidia-drm`。
- 真正好用的关联键是 inode：
  - `/proc/self/fdinfo/<dma-buf-fd>` 里有 `ino`
  - `bufinfo` 里也有同一个 `ino`
  - 把这两个对上，就能定位“当前程序创建的就是这一条 dma-buf”

构建：

```bash
make
```

## udmabuf examples

`udmabuf_demo.out` 演示的是：

- 先创建一个 `memfd`
- 再通过 `/dev/udmabuf` 把这组页导出成 `dma-buf fd`
- 最后证明 `memfd` 映射和 `dma-buf` 映射看到的是同一份数据

```bash
./udmabuf_demo.out
echo a | sudo -S ./udmabuf_demo.out -t 5
```

`udmabuf_drm_import_demo.out` 则更进一步：

- 先做 `memfd -> udmabuf -> dma-buf fd`
- 再把这个 `dma-buf fd` 用 `DRM_IOCTL_PRIME_FD_TO_HANDLE` 导入到某个 DRM 节点
- 然后观察 `bufinfo` 里的 `Attached Devices` 变化

```bash
echo a | sudo -S ./udmabuf_drm_import_demo.out --device /dev/dri/renderD128
```

这台机器上的实际现象是：

- import 之前 `Attached Devices` 是 `0`
- PRIME import 之后变成 `1`
- 单独 `DRM_IOCTL_GEM_CLOSE` 之后 attachment 还可能存在
- 关闭 DRM fd 之后才真正掉回 `0`

这说明 attachment 的生命周期不一定只跟一个 GEM handle 绑定，还可能受 DRM file-private PRIME cache 影响。

列出当前机器上的 DRM 节点：

```bash
./virtio_gpu_dma_test.out --list
```

默认测试：

```bash
echo a | sudo -S ./virtio_gpu_dma_test.out
```

指定设备并保留一段时间，方便外部观察：

```bash
./virtio_gpu_dma_test.out \
  --device /dev/dri/card1 \
  --width 4096 \
  --height 4096 \
  --hold-seconds 20
```

观察点：

```bash
nvidia-smi --query-gpu=memory.used --format=csv,noheader
echo a | sudo -S cat /sys/kernel/debug/dma_buf/bufinfo
```

在我这台机器上的一次实际结果：

- `card0` 是 `xe`
- `card1` 是 `nvidia-drm`
- `card2` 是 `vkms`
- 在 `card1` 上分配 `4096x4096x32bpp` 的 dumb buffer 后，`nvidia-smi` 的 `memory.used` 从 `5 MiB` 上升到 `69 MiB`
- 但它没有出现在 `nvidia-smi --query-compute-apps` 里，也没有在 `nvidia-smi pmon` 里单独显示成这个测试进程

所以这里更准确的理解是：

- 这个程序分配的是某个 DRM driver 管理的 buffer object
- 它是否真落在“显存”以及是否会被 `nvidia-smi` 记账，取决于具体驱动
- 在当前这台机器的 `nvidia-drm` 上，至少全局 `memory.used` 会反映这笔分配

## 简单的 bpftrace 观察

```txt
@[
        __nv_drm_gem_nvkms_map+1
        __nv_drm_gem_nvkms_mmap+26
        nv_drm_mmap+222
        __mmap_new_vma+251
        __mmap_region+2837
        mmap_region+130
        do_mmap+1159
        vm_mmap_pgoff+291
        ksys_mmap_pgoff+354
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1
@[
        __nv_drm_gem_nvkms_map+1
        nv_drm_dumb_create+353
        drm_mode_create_dumb_ioctl+18
        drm_ioctl_kernel+174
        drm_ioctl+680
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1
```

## 为什么可以让 CPU 直接访问显存的

nvidia-smi 看到的 FB Memory 是显卡板载 VRAM，总量这张卡是 3072 MiB；而 /proc/iomem / lspci 看到的是这块 PCI 设备暴露给 CPU 的 BAR 资源窗口，不是“整块显存都挂进了 CPU 物理地址空间”。

你这台机器上我刚核对过 01:00.0 的 BAR：

```txt
sudo cat /proc/iomem | grep 01:00.0
[sudo] password for martins3:
    81000000-81ffffff : 0000:01:00.0
    82000000-8207ffff : 0000:01:00.0
    6000000000-600fffffff : 0000:01:00.0
    6010000000-6011ffffff : 0000:01:00.0
```

```txt
  - Region 0: 81000000 [size=16M]
  - Region 1: 6000000000 [size=256M]
  - Region 3: 6010000000 [size=32M]
  - ROM: 82000000 [size=512K]
```


  这加起来大概只有 304.5 MiB，确实远小于 3 GiB VRAM。这里最关键的是 Region 1，也就是 NVIDIA 常说的 BAR1。nvidia-smi -q -d
  MEMORY 也直接印证了这一点：

  - FB Memory Total: 3072 MiB
  - BAR1 Memory Total: 256 MiB

  所以关系应该这样理解：

  - FB Memory：真正的显存容量，在 GPU 本地。
  - BAR1：CPU 访问显存用的一个 PCI aperture/window，只是个“窗口”，不是整块显存本体。
  - mmap()：给进程建立的是用户虚拟地址映射，不要求底下对应一整块等大的 CPU 物理连续地址空间。

  为什么 mmap 后能直接读写？

  - 不是因为“3G 显存全都映射进了 /proc/iomem”。
  - 而是驱动把这块 BO 通过 BAR1 这样的窗口映到 CPU 可访问空间，再把它挂到你的进程 VMA 上。
  - 对用户态来说是连续虚拟地址；对驱动来说，它可以用 BAR1 去覆盖那段 VRAM。
  - 你的这次分配只有 64 MiB，小于 BAR1=256 MiB，所以完全能放进这个窗口里。

  这次实测也说明了这一点。分配并 mmap 一个 64 MiB buffer 期间，nvidia-smi -q -d MEMORY 变成：

```txt
  - FB Memory Used: 69 MiB
  - BAR1 Memory Used: 69 MiB
```

  这很像“这块 BO 在 VRAM 里分配了，同时也占了 BAR1 映射窗口来给 CPU 访问”。

  所以你的疑惑可以收敛成一句话：

  - mmap 映到的是进程虚拟地址。
  - 下面靠的是驱动用 BAR1 这样的 PCI 窗口去接通 VRAM。
  - /proc/iomem 里不需要出现整块 3 GiB 显存。

### 如果创建的空间大于 256M ，会如何?

在这台机器上，结论很直接：走现在这条 DRM_IOCTL_MODE_CREATE_DUMB + DRM_IOCTL_MODE_MAP_DUMB 路径时，还没到“大于 256 MiB 的
  mmap 会怎样”，就已经在创建阶段失败了。

  我刚做了边界测试：

  - 7936x7936x4 = 240 MiB：成功
  - 8000x8000x4 = 244 MiB：成功
  - 8064x8064x4 = 248 MiB：成功
  - 8128x8128x4 = 252 MiB：CREATE_DUMB 失败，Cannot allocate memory
  - 8192x8192x4 = 256 MiB：CREATE_DUMB 失败，Cannot allocate memory

  再看 8064x8064 持有期间的 nvidia-smi -q -d MEMORY：

  - FB Memory Used: 255 MiB
  - BAR1 Memory Used: 253 MiB
  - BAR1 Total: 256 MiB

  这基本说明了两点：

  1. 这条 CPU 可 mmap 的 dumb-buffer 路径，实际受 BAR1 窗口约束。
  2. 不是正好到 256 MiB 才失败，因为 BAR1 本来就有基础占用、对齐和驱动自身开销，所以大约到 252 MiB 就过不去了。

  所以对你这个问题，更准确的回答是：

  - 在这台 GTX 1060 3GB 上，如果你想 mmap 的那块 nvidia-drm dumb buffer 大于 BAR1 可用空间，通常不是“mmap 成功但只能访问一
    部分”，而是更早就在分配/创建阶段失败。
  - 这里失败的根因不是 VRAM 不够。VRAM 还有 3 GiB，卡住的是“CPU 直连可映射窗口”只有 256 MiB 左右。

  但要补一句一般性结论：

  - 这不是所有 GPU/驱动的统一规律。
  - 如果机器开了更大的 Resizable BAR，或者驱动支持按页迁移/按需 remap，更大的对象也可能存在，只是 CPU 映射方式会不同。
  - 但就你现在这个 nvidia-drm dumb buffer + mmap 实验，实测上限就在 248 MiB 左右。

## dmabuf info 中的输出含义
其实是有 device 用的


• 关键点先说结论：/sys/kernel/debug/dma_buf/bufinfo 不是“当前谁正在用 GPU”的视图，它只是“当前系统里还活着的 dma-buf 对象列
  表”。“活着”只表示这个 struct dma_buf 还没走到最终 release，不表示它正在提交 GPU 工作，也不表示一定有 importer 设备挂在上
  面。

  源码上是这样实现的：

  - dma_buf_export() 创建 struct dma_buf 后，会把对象挂进全局链表 dmabuf_list，见 /home/martins3/data/kernel/linux-build/
    drivers/dma-buf/dma-buf.c:67 和 /home/martins3/data/kernel/linux-build/drivers/dma-buf/dma-buf.c:767。
  - bufinfo 就是遍历这个全局链表打印出来，见 /home/martins3/data/kernel/linux-build/drivers/dma-buf/dma-buf.c:1699。
  - 对象只有在 dma-buf file 真正 release 时才从链表删掉，见 /home/martins3/data/kernel/linux-build/drivers/dma-buf/dma-
    buf.c:194。

  所以它回答的是：

  - 这个 dma-buf 还在不在
  - 大小是多少
  - 文件引用计数大概多少
  - 当前 dma_resv 上挂了哪些 fence
  - 当前有没有 dma_buf_attach() 形成的 importer device

  它不直接回答：

  - 现在 GPU 是否正在读写它
  - 它是不是“显存里热数据”
  - 它是不是 compositor/browser 缓存池里的闲置 buffer

  你这份输出每列的来源也很直接，见 /home/martins3/data/kernel/linux-build/drivers/dma-buf/dma-buf.c:1710：

  - size: buf_obj->size
  - flags: buf_obj->file->f_flags
  - mode: buf_obj->file->f_mode
  - count: file_count(buf_obj->file)
  - exp_name: buf_obj->exp_name
  - ino: file_inode(buf_obj->file)->i_ino
  - 后面的 write fence: ... signalled 来自 dma_resv_describe()
  - Attached Devices 来自 buf_obj->attachments

  这里最容易误解的有 4 个点。

  1. exp_name=drm 不等于“都是同一个驱动”

     这个字段在 generic DRM PRIME helper 里就是故意写成 KBUILD_MODNAME，还注释了 white lie for debug，见 /home/martins3/
     data/kernel/linux-build/drivers/gpu/drm/drm_prime.c:916。在这个文件里 KBUILD_MODNAME 就是 drm，所以很多不同 DRM 驱动
     导出的 dma-buf 最后都会显示成 drm，它没法区分 xe、virtio_gpu、nvidia-drm。
  2. Attached Devices: 0 不等于“完全没用”

     这里只统计 dma_buf_attach() 产生的 importer device，源码说明也写了，attach 是“把 dma-buf attach 到某个 device 上供
     DMA 访问”的那一步，见 /home/martins3/data/kernel/linux-build/drivers/dma-buf/dma-buf.c:662。

     但下面这些情况都可能让它看起来是 0：
      - 用户态只是持有这个 dma-buf fd
      - 用户态只是 mmap 了它
      - exporter 自己还在用原生 GEM/BO
      - PRIME handle/fd cache 还在给它保留引用
  3. count 不是“进程数”也不是“设备数”

     这里打印的是 file_count(buf_obj->file)，就是 file refcount，见 /home/martins3/data/kernel/linux-build/drivers/dma-
     buf/dma-buf.c:1711。它会包含：
      - 进程 fd 持有的引用
      - 内核内部额外拿的引用
      - DRM PRIME cache 持有的引用

     PRIME cache 这点在 /home/martins3/data/kernel/linux-build/drivers/gpu/drm/drm_prime.c:97 很明显，
     drm_prime_add_buf_handle() 里直接 get_dma_buf(dma_buf) 了。
  4. write fence ... signalled 不等于“当前在忙”

     它恰好相反，signalled 表示那条 fence 对应的写入已经完成了。它只能说明“这个 buffer 曾经有过同步关系”，不能说明“现在正
     在使用”。

  我还查了几条你输出里的 inode，对应到真实进程了，所以这些对象不是“莫名其妙创建了但没人要”，而是桌面栈里确实有人持有：

  - ino 5474，size 24576：被 Xwayland 持有
  - ino 5471，size 6291456：也被 Xwayland 持有
  - ino 4824/4826/4828，都是 32 MiB：同时被 gnome-shell 和 msedge --type=gpu-process 持有
  - ino 4799/4800/4801/4802，都是 ~20 MiB：被 gnome-shell 和 gnome-software 持有
  - ino 43：udmabuf

  这说明你的 bufinfo 里大部分对象其实就是 Wayland/Xwayland/浏览器 GPU 进程之间共享的图形缓冲。它们“看起来没在用”，只是因
  为：

  - fence 已经 signalled 了
  - 当前没有 active importer attachment
  - 但 fd/ref/cache 还在，所以对象还活着

  如果你想把这个再看得更透，下一步最有价值的是按 ino 去反查持有者。直接用这条就够了：

```txt
  echo a | sudo -S bash -lc 'grep -sl "^ino:[[:space:]]*4828$" /proc/[0-9]*/fdinfo/* 2>/dev/null'
```
  然后再看对应 PID 的 cmdline。这样比盯着 bufinfo 本身猜要准得多。
