
## 这个项目做什么的
- https://github.com/ikwzm/udmabuf : user dma driver

## dma-buf 的用户

Documentation/userspace-api/dma-buf-alloc-exchange.rst
Documentation/userspace-api/dma-buf-heaps.rst

```txt
config DMA_SHARED_BUFFER
	bool
	default n
	select IRQ_WORK
	help
	  This option enables the framework for buffer-sharing between
	  multiple drivers. A buffer is associated with a file using driver
	  APIs extension; the file's descriptor can then be passed on to other
	  driver.

config VIRTIO_DMA_SHARED_BUFFER
	tristate
	depends on DMA_SHARED_BUFFER
	help
	 This option adds a flavor of dma buffers that are backed by
	 virtio resources.

```

> [!NOTE]
> 参考 Deepseeek ，有待验证

通过找 DMA_SHARED_BUFFER 可以找到:

  使用 DMA-BUF API 的特定模块：
   - Virtio DMA 共享缓冲区 - CONFIG_VIRTIO_DMA_SHARED_BUFFER (提供基于 virtio 的 dma 缓冲区的特定模块)
   - 同步文件 - 使用 DMA_SHARED_BUFFER
   - UDMABUF - 用户空间 DMA 缓冲区驱动程序
   - DMA-BUF 堆 - 提供用户空间内存堆
   - SW Sync - 同步验证框架
   - DMA-BUF 自测 - 测试框架
   - DMA-BUF Sysfs 统计 - 统计框架

  使用 DMA-BUF 的 GPU 驱动程序：
   - AMDGPU
   - Intel XE/i915
   - NVIDIA Tegra DRM
   - NVIDIA DRM
   - Mali GPU
   - 虚拟 GPU (Virtio)
   - 以及许多其他 DRM 子系统驱动程序


## dma-buf 是做什么的
<!-- b080316f-bde5-4f62-b99e-9b8ed5a08d3d -->

https://mp.weixin.qq.com/s/i9gSR0muX2_h7sivl8a0Dg

(可以找到一个环境测试再说吧，而且问题是，dma buf 中的东西和 GPU 关系很大，先搞搞 GPU 吧)

DMA-BUF 子系统是 Linux 内核中用于**在多个设备或子系统之间安全、高效地共享缓冲区（buffer）**的核心机制。它的主要目标是解决“**如何在不同驱动、不同硬件之间共享同一块物理内存，而无需拷贝数据**”的问题，尤其在多媒体、图形、GPU、摄像头、视频编解码等高性能场景中至关重要。

### 一、核心功能

#### 1. **缓冲区共享（Zero-copy Sharing）**
- DMA-BUF 允许一个设备（exporter，导出者）分配一块内存（例如 GPU 显存、CMA 内存、系统内存等），然后将其暴露为一个 **`dma_buf` 对象**。
- 其他设备或驱动（importer，导入者）可以通过文件描述符（fd）访问这块内存，进行 DMA 操作、映射（mmap）、CPU 访问等，**无需拷贝数据**。

#### 2. **跨设备/跨驱动同步**
- 通过集成 **`dma_fence`**（DMA fence）机制，DMA-BUF 支持在多个使用者之间进行**隐式同步（implicit synchronization）**。
  - 例如：GPU 渲染完一帧后，显示控制器才能读取该帧。
  - 同步信息存储在 **`dma_resv`（reservation object）** 中。
- 用户空间也可以通过 **`sync_file`** 显式控制同步（explicit fencing）。

#### 3. **统一的用户空间接口**
- 用户空间通过 **`/dev/dma_heap/xxx`** 或类似接口（如 `udmabuf`）创建 DMA-BUF。
- 通过 `ioctl(DMA_BUF_FD)` 获取文件描述符（fd）。
- 可传递该 fd 给其他进程或驱动（如 DRM、V4L2、ION 替代方案等）。
- 支持：
  - `mmap()`：映射到用户空间进行 CPU 访问。
  - `poll()`：等待渲染/处理完成。
  - `ioctl(DMA_BUF_IOCTL_SYNC)`：标记 CPU 访问开始/结束，确保 cache 一致性。
  - `/proc/<pid>/fdinfo/<fd>`：查看 buffer 信息（大小、导出者名等）。
  - `/sys/kernel/dmabuf/buffers/<inode>/`（若启用 `CONFIG_DMABUF_SYSFS_STATS`）：全局统计。

### 二、关键组件

| 组件 | 作用 |
|------|------|
| `struct dma_buf` | 核心对象，代表一个可共享的缓冲区 |
| `struct dma_buf_ops` | 导出者实现的回调函数（`map_dma_buf`, `mmap`, `vmap`, `release` 等） |
| `struct dma_resv` | 用于管理 fence 同步对象（读/写 fence） |
| `struct dma_fence` | 表示一个异步操作的完成信号（如 GPU 任务完成） |
| `dma_heap` | 用户空间可分配 DMA-BUF 的堆（如 system heap、CMA heap） |
| `udmabuf` / `dma-buf heaps` | 用户态创建 DMA-BUF 的方式 |

### 三、典型使用流程（内核驱动视角）

1. **Exporter（导出者）**：
   - 调用 `dma_buf_export()` 创建 `dma_buf`。
   - 实现 `dma_buf_ops` 回调。
   - 调用 `dma_buf_fd()` 生成 fd 并传给用户空间。

2. **Importer（导入者）**：
   - 从用户空间接收 fd。
   - 调用 `dma_buf_get(fd)` 获取 `dma_buf`。
   - 调用 `dma_buf_attach()` 绑定到自己的设备。
   - 调用 `dma_buf_map_attachment()` 获取 `sg_table` 用于 DMA。
   - 使用完毕后 `dma_buf_detach()` 和 `dma_buf_put()`。

3. **同步**：
   - 使用 `dma_resv` 添加/等待 fence。
   - 或通过 `poll()` 等待 buffer 可读/可写。

### 四、应用场景

- **GPU + Display**：GPU 渲染帧 → 显示控制器扫描输出。
- **Camera + GPU + Video Encoder**：摄像头采集 → GPU 处理 → 编码器压缩。
- **Android HAL 层**：替代旧的 ION，作为统一内存共享机制。
- **虚拟化**：在 Guest 和 Host 之间共享图形/视频 buffer。
- **用户态零拷贝管道**：如 GStreamer、Vulkan、Wayland。

### 五、优势

- **零拷贝**：避免昂贵的内存复制。
- **硬件无关**：抽象了底层内存类型（CMA、系统页、IOMMU 映射等）。
- **同步安全**：通过 fence 机制防止数据竞争。
- **用户空间友好**：基于 fd，易于跨进程传递。

## 基本的测试

```txt
sudo cat /sys/kernel/debug/dma_buf/bufinfo
[sudo] password for martins3:

Dma-buf Objects:
size            flags           mode            count           exp_name        ino             name

Total 0 objects, 0 bytes
```

但是在虚拟机的确观察 virtio_dma_buf 被 virtio-gpu 所依赖:
```txt
lsmod | grep virtio
virtio_net            118784  0
net_failover           28672  1 virtio_net
virtio_input           20480  0
virtio_balloon         32768  0
virtio_console         45056  0
virtio_scsi            28672  2
virtio_gpu            106496  0
drm_client_lib         12288  1 virtio_gpu
virtio_dma_buf         12288  1 virtio_gpu
drm_shmem_helper       32768  2 virtio_gpu
drm_kms_helper        237568  3 drm_shmem_helper,virtio_gpu,drm_client_lib
drm                   765952  5 drm_kms_helper,drm_shmem_helper,virtio_gpu,drm_client_lib
virtio_pci             53248  0
virtio_pci_legacy_dev    16384  1 virtio_pci
virtio_pci_modern_dev    16384  1 virtio_pci
```

相关的测试代码 code/src/drm/virtio-gpu-dma/


## 结合具体源码说说

也就是 dma buf 可以用于多种用途。

> [!NOTE]
> 参考神奇海螺的意见，有待验证

  drivers/dma-buf/ 不是某个具体硬件驱动，而是 Linux 里“跨设备共享 buffer + 同步”的通用基础设施。实际用它最多的是 DRM/GPU、
  视频、多媒体、摄像头这些子系统；这个目录本身主要提供框架、同步原语和用户态接口。

  这套代码可以概括成 3 个核心对象：

  - dma_buf：共享内存对象本身，由 exporter 导出、importer 导入。drivers/dma-buf/dma-buf.c:708
  - dma_fence：异步完成事件，表示“某次 DMA/GPU 工作还没结束”。drivers/dma-buf/dma-fence.c:486
  - dma_resv：挂在 buffer 上的 fence 容器，承载 implicit sync。 drivers/dma-buf/dma-resv.c:287

  一张图理解它

  大致流程是：

  1. exporter 分配真实内存，然后调用 dma_buf_export() 把自己的私有对象包装成 struct dma_buf。drivers/dma-buf/dma-buf.c:708
  2. 再调用 dma_buf_fd() 把它变成一个 fd 给用户态或别的驱动传递。drivers/dma-buf/dma-buf.c:788
  3. importer 用 dma_buf_get() 拿到对象，dma_buf_attach()/dma_buf_dynamic_attach() 绑定设备。drivers/dma-buf/dma-buf.c:810
     drivers/dma-buf/dma-buf.c:1009
  4. importer 在真正发 DMA 前，通过 dma_buf_map_attachment() 拿到 sg_table，也就是给本设备可用的 DMA 映射。drivers/dma-
     buf/dma-buf.c:1169
  5. 同步不是靠 buffer 本体做，而是靠 dma_resv 里挂的 dma_fence；CPU 访问前也会先等 fence。drivers/dma-buf/dma-buf.c:1433

  最关键的几个文件

  - drivers/dma-buf/dma-buf.c:708：核心对象生命周期、attach/map/unmap、mmap/vmap、CPU 访问同步。
  - drivers/dma-buf/dma-fence.c:523：通用 fence 机制，wait/callback/signal 都在这。
  - drivers/dma-buf/dma-resv.c:182：把多个 fence 挂到一个 buffer 上，并支持无锁读 + RCU。
  - drivers/dma-buf/sync_file.c:65：把 fence 暴露成用户态 fd，属于 explicit sync。
  - drivers/dma-buf/sw_sync.c:88：软件时间线，主要用于测试/调试，不是正常生产同步后端。
  - drivers/dma-buf/dma-heap.c:230：/dev/dma_heap/* 用户态分配入口。
  - drivers/dma-buf/heaps/system_heap.c:341：系统页分配的 heap。
  - drivers/dma-buf/heaps/cma_heap.c:295：CMA 连续内存 heap。
  - drivers/dma-buf/udmabuf.c:369：把 memfd 的页 pin 住后导出成 dma-buf，QEMU 之类会用。

  设计上最值得注意的点

  - dma_buf 只定义“共享 buffer 的协议”，真实内存怎么分配、怎么迁移、怎么 map，都交给 exporter 的 dma_buf_ops。include/
    linux/dma-buf.h:24
  - dma_resv 很核心。它把 fence 和 usage 绑在一起，usage 有 KERNEL/WRITE/READ/BOOKKEEP 四级，而且查询某一级时会自动包含更
    低级别的 fence。include/linux/dma-resv.h:47
  - dma_resv 的实现很“内核味”：把 usage 塞进 fence 指针低位，读路径走 RCU，无锁迭代可能重启。drivers/dma-buf/dma-resv.c:60
    drivers/dma-buf/dma-resv.c:366
  - dma_fence 是跨驱动 contract，不只是一个 completion。代码里专门强调：任何会阻塞 dma_fence_signal() 的锁依赖都可能死锁，
    所以 signaling 路径有很严格的约束。drivers/dma-buf/dma-fence.c:32
  - dma-buf.c 里写得最重要的一段其实是锁规则文档：哪些 API 调用时必须持有 dma_resv 锁，哪些回调在锁内执行，哪些在锁外执
    行。drivers/dma-buf/dma-buf.c:923
  - debug 模式下，dma_buf_map_attachment() 会把 exporter 返回的 sg_table 包一层，刻意把 struct page 藏掉，防止 importer 偷
    看底层页结构。drivers/dma-buf/dma-buf.c:854

  显式同步和隐式同步

  这里同时支持两套同步模型：

  - implicit sync：fence 挂在 dma_buf.resv 上，访问同一个 buffer 的驱动自动彼此等待。这是传统 DRM/图形栈里很常见的方式。
  - explicit sync：用 sync_file 把 fence 变成 fd，用户态自己传来传去。drivers/dma-buf/sync_file.c:65

  sync_file_merge() 本质上就是把多个 fence 展开、去重、必要时组合成 dma_fence_array。drivers/dma-buf/sync_file.c:165
  drivers/dma-buf/dma-fence-unwrap.c:117
  所以 dma-fence-array.c、dma-fence-chain.c、dma-fence-unwrap.c 这几个文件，本质上是 fence 的“组合器”和“扁平化工具”。

  dma-heap 和 udmabuf 的作用

  这两块更偏用户态接口：

  - dma-heap 给用户态一个标准分配入口。system heap 用普通页拼 sg；cma heap 用连续内存，适合对物理连续性有要求的设备。
    drivers/dma-buf/dma-heap.c:321 drivers/dma-buf/heaps/system_heap.c:341 drivers/dma-buf/heaps/cma_heap.c:295
  - udmabuf 不是“分配新内存”，而是把现有 memfd 页 pin 住，再导出成 dma-buf；它要求 memfd seal 合法，防止 backing storage
    在共享期间被缩小或写坏。drivers/dma-buf/udmabuf.c:291 drivers/dma-buf/udmabuf.c:369

  如果你是从“看代码”角度切入，我建议的阅读顺序

  1. 先读 include/linux/dma-buf.h:24 和 include/linux/dma-resv.h:47，先把 API 语义立住。
  2. 再读 drivers/dma-buf/dma-buf.c:708 和 drivers/dma-buf/dma-resv.c:182。
  3. 然后读 drivers/dma-buf/dma-fence.c:523，理解 wait/signal/callback contract。
  4. 最后按需求看 sync_file、dma-heap、udmabuf。

  如果你愿意，我下一步可以继续按下面任一方向深入：

  - 逐函数带你精读 dma-buf.c
  - 重点讲清 dma_resv + dma_fence 的同步语义
  - 结合 DRM/GPU 的典型调用链解释这套框架怎么用

## vfio + dma-buf
https://lore.kernel.org/dri-devel/cd6f8c6b-6950-4b06-8f2d-bb4ead660ead@app.fastmail.com/
https://lore.kernel.org/dri-devel/20241126031643.3490496-1-vivek.kasireddy@intel.com/#r
https://patchew.org/QEMU/20251122064936.2948632-1-vivek.kasireddy@intel.com/

qemu / virtio-gpu / dma buf 和 vfio 都可以放到一起。

根据这些补丁（patch）和讨论，他们想达到的主要目的（motivations / goals）可以总结为以下几点：

1. **支持通过 VFIO 导出 dmabuf**

   * 他们新增了 `VFIO_DEVICE_FEATURE_DMA_BUF`，这是 VFIO 设备的一种特性。([Patchew][1])
   * 通过这个特性，可以把 VFIO-PCI 设备的一部分内存区域（region）导出为 dmabuf（即零拷贝缓冲区）。([Patchew][1])
   * 具体来说，这里他们能 **把显卡（尤其是具有本地内存 / VRAM 的 dGPU VF）上的内存区域**，通过 VFIO 暴露出来，以 dmabuf 的形式被其他组件（host 或 guest）使用。([LWN.net][2])

2. **让 virtio-gpu（QEMU / 虚拟机里面的 GPU 驱动）可以使用这种 dmabuf 后端**

   * 在 QEMU 中，virtio-gpu 驱动可以创建 “blob 资源”。这些 blob 原来可以是 backed（支持） memfd（系统 RAM 后端） + udmabuf 驱动。([Patchew][3])
   * 但对于某些情形（比如 GPU 有本地显存 / device 区域），他们希望这些 blob 能用 VFIO-PCI 区域来 backing。这样 virtuio-gpu 就不只是操作系统 RAM 的 blob，而是可以直接利用显卡设备本身的内存。([Patchwork][4])
   * 于是，在这些补丁里，如果检测到一个内存 region 是 VFIO backed（而不是纯 RAM），QEMU 就会通过 VFIO 来创建 dmabuf，而不是走传统的 udmabuf 路径。([Patchwork][4])

3. **实现零拷贝 / 高效共享**

   * 通过这种方式，可以减少拷贝开销 —— 如果 guest 想用 GPU 的某块显存，它不必把那块显存先拷贝到系统 RAM 再传给 guest，而是可以直接通过 dmabuf 机制共享。
   * 这对性能敏感的场景尤其重要，比如图形重度应用、3D 渲染、GPU-GPU 共享内存、P2P DMA 等。实际上 LWN 的一篇文章指出，这套机制可以让 virtio-gpu 驱动在 guest 中导入来自设备（例如 dGPU VF）的扫描缓冲区 (scanout buffer)，以一种 “零拷贝” 的方式。([LWN.net][2])
   * 这样还可能允许 **不同 GPU 设备之间**（只要支持 P2P DMA）共享内存，比如一个是宿主机 GPU，一个是 guest GPU。LWN 上就提到了这种可能性。([LWN.net][2])

4. **增强可移植性 / 通用性**

   * 他们用更通用的方法来标识内存区域类型（比如引入 `ram_block_is_memfd_backed()` helper），不再仅仅依赖旧的方法 (比如通过 `res->blob`)。这是为了更灵活地支持多种类型的后端。([邮件归档][5])
   * 通过标准化 VFIO 和 dmabuf 的接口（feature ioctls, region translation 等），能让 QEMU 对于 VFIO 设备 region 的处理更加健壮、可扩展。

5. **安全与生命周期管理**

   * dmabuf 本身可以提供对缓冲区生命周期的管理。通过 VFIO 导出 dmabuf，可以让 host 控制导出的缓冲区（何时创建，何时释放）。这比某些简单共享机制要更安全 / 灵活。
   * 由于 VFIO 是一种面向设备访问和隔离的机制，这种导出在安全上下文里是非常重要的：host 可以有更细粒度地控制读取 / 写入，guest 通过 dmabuf 拿到句柄，而不必暴露整个设备不受控制。

[1]: https://patchew.org/QEMU/20251109053801.2267149-1-vivek.kasireddy%40intel.com/20251109053801.2267149-7-vivek.kasireddy%40intel.com/?utm_source=chatgpt.com "[v2] vfio: Implement VFIO_DEVICE_FEATURE_DMA_BUF and use it in virtio-gpu | Patchew"
[2]: https://lwn.net/Articles/998774/?utm_source=chatgpt.com "drm/virtio: Import scanout buffers from other devices [LWN.net]"
[3]: https://patchew.org/QEMU/20250903054438.1179384-1-vivek.kasireddy%40intel.com/?utm_source=chatgpt.com "[v1] vfio: Implement VFIO_DEVICE_FEATURE_DMA_BUF and use it in virtio-gpu | Patchew"
[4]: https://patchwork.ozlabs.org/project/qemu-devel/cover/20251003234138.85820-1-vivek.kasireddy%40intel.com/?utm_source=chatgpt.com "[v1,0/7] vfio: Implement VFIO_DEVICE_FEATURE_DMA_BUF and use it in virtio-gpu - Patchwork"
[5]: https://www.mail-archive.com/qemu-devel%40nongnu.org/msg1154484.html?utm_source=chatgpt.com "[PATCH v3 0/9] vfio: Implement VFIO_DEVICE_FEATURE_DMA_BUF and use it in virtio-gpu"

## vfio 现在也支持了
drivers/vfio/pci/vfio_pci_dmabuf.c
