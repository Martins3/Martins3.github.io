 ## 测试 vector_add.cu

  TID      名称                                                         说明
 ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  主 PID   vector_add.out / vector_add_3d.out / vector_add_stream.out   主线程
  +1       cuda00001400006                                              CUDA 内部工作线程
  +1       cuda-EvtHandlr                                               CUDA 事件处理线程
  +1       与主线程同名                                                 pthread 创建的辅助线程
 结论：即使是使用了 stream 和 cudaMallocHost 的 vector_add_stream，CUDA 运行时创建的线程数与最基础的 vector_add 完全相同。

 打开的文件描述符
 三个程序完全一致，都是 32 个 fd。关键 fd 分类如下：
  fd                    类型          路径              说明
 ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  0                     字符设备      /dev/null         stdin
  1/2                   日志文件      /tmp/...log       stdout/stderr
  3                     eventfd       [eventfd]         内部事件通知
  4/5, 6/7, 30/31       pipe          pipe              CUDA 内部 IPC
  8                     字符设备      /dev/nvidiactl    NVIDIA 控制设备
  9                     字符设备      /dev/nvidia-uvm   统一虚拟内存
  10-12, 15-20, 22-29   字符设备      /dev/nvidia0      GPU 设备（共 16 个 fd）
  13                    unix socket   @cuda-uvmfd-...   UVM fd 传递
  14, 21                eventfd       [eventfd]         额外事件通知

启动有点意思的是这两个:

• /dev/nvidia-uvm — 统一虚拟内存设备
  这是 nvidia-uvm.ko 内核模块暴露给用户态的字符设备，实现 Unified Virtual Memory (UVM) 的核心功能：
  • 统一地址空间：让 CPU 和 GPU 共享同一个虚拟地址空间。你用 cudaMallocManaged() 分配的内存，CPU 和 GPU 都可以用同一个指针直接访问
    不需要手动 cudaMemcpy()。
  • 按需页迁移：当 CPU 访问一块当前驻留在 GPU 显存中的页时，会触发 CPU page fault；UVM 驱动捕获这个 fault，把数据从 GPU 显存复制到
    机内存，更新 CPU 页表，然后让访问继续。反之亦然。
  • 对上层透明：对程序员来说就像普通的 malloc 内存，但底层由 UVM 驱动自动处理数据的物理位迁移。
  简单说，没有这个设备，cudaMallocManaged 和统一内存相关的功能就无法工作。

  @cuda-uvmfd-... — UVM fd 传递套接字
  这是一个 unix domain socket（类型为 SEQPACKET），命名格式大概是： `@cuda-uvmfd-<nsid>-<pid>@`
  它的作用是 文件描述符传递 (fd passing)：
  • CUDA 运行时需要与 UVM 内核驱动频繁交互，涉及到共享某些内部 fd（比如内存区域、事件通知等）。
  • 这个 socket 用于 CUDA 运行时内部组件之间（或运行时与驱动辅助线程之间）传递这些 fd，而不是通过全局路径名访问。
  • 名字里的 @ 开头表示它是一个 abstract socket（不绑定到文件系统路径，只存在于内核中）。

  简单说，它是 CUDA 运行时和 UVM 驱动之间的一条内部 IPC 通道，专门用来"交接"文件描述符。

(这里没想太明白，为什么 UVM 需要一个 /dev/nvidia-uvm 和 uds ，而且 uds 居然是和内核中通信 !)
