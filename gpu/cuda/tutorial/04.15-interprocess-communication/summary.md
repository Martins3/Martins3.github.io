## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/inter-process-communication.html>

其实原文很短。

## 背景与要解决的问题

在单进程多线程的 CUDA 程序中，任何由主机线程创建的 device memory pointer 或 event handle 都可以被同一进程内的其他线程直接引用。然而，当程序模型从多线程转向多进程时，这一假设不再成立。操作系统为每个进程提供了独立的虚拟地址空间，CUDA Runtime 在进程内部维护的 device pointer 本质上是该进程 CUDA 上下文中的句柄，离开创建它的进程后即失效。因此，如果进程 A 通过 `cudaMalloc` 分配了一段设备内存，并将得到的 `float*` 直接发送给进程 B，进程 B 无法直接将其传递给 kernel 或 `cudaMemcpy`，否则会导致未定义行为甚至程序崩溃。

为了打破这一隔离，CUDA 提供了一套 IPC 抽象：允许进程 A 将本地的设备内存"句柄化"为一个进程可移植（process-portable）的 IPC handle，进程 B 再通过 CUDA IPC API 将该 handle "还原"为进程 B 自己 CUDA 上下文中的有效 device pointer。整个过程类似于操作系统中的文件描述符传递：真正的资源（GPU 显存）由 NVIDIA 内核驱动统一管理，IPC handle 只是一个跨进程边界的、可被验证和转换的令牌。这一机制的典型使用场景包括：一个主进程（primary process）生成大批量输入数据，然后通过 IPC 共享给多个辅助进程（secondary processes）进行并行计算，避免数据在主机内存中的重复生成或冗余拷贝。

除了单节点单 OS 实例内的 IPC，现代数据中心还面临多节点 NVLink 互联集群的通信需求。在这种场景下，通信双方不仅处于不同进程，还可能运行在不同节点的独立操作系统实例之上。为此，CUDA 引入了 Fabric Handle，作为跨节点、跨操作系统实例的 portable handle，使得多节点 rank 之间也能建立对等（peer-to-peer）显存访问。

## 核心概念与术语

**Process-portable Handle（进程可移植句柄）**
这是本章最核心的抽象。它指代一种经过 CUDA 驱动编码的数据结构（如 `cudaIpcMemHandle_t`），其内部包含了定位某段设备物理内存所需的全部信息，但又不依赖于任何特定进程的地址空间。不同进程可以通过标准 OS IPC 机制（如共享内存、Unix domain socket、管道或文件）交换这种句柄，然后在本地调用 CUDA API 将其"打开"为进程私有的 device pointer。

**Device Pointer（设备指针）**
在 CUDA 编程模型中，`cudaMalloc` 返回的指针属于当前进程的 CUDA 上下文。它直接对应 GPU 虚拟地址空间中的一个地址，仅在创建它的进程中有效。IPC 机制的最终目标就是让另一个进程也能获得指向同一段物理显存的、属于该进程上下文的 device pointer。

**Event Handle（事件句柄）**
CUDA Event 用于同步和计时。与设备指针类似，`cudaEvent_t` 句柄同样具有进程边界限制。CUDA IPC API 也提供了对应的事件共享入口点（如 `cudaIpcGetEventHandle` / `cudaIpcOpenEventHandle`），使得一个进程中记录的事件可以被另一个进程等待。

**Memory Sync Domain Map（内存同步域映射）**
虽然这不是 IPC 独占的概念，但章节开头以它作为引子。在 Hopper（计算能力 9.0）及更高版本的架构上，CUDA 引入了内存同步域（Memory Sync Domain）的概念，用于细化不同流之间的缓存一致性粒度。`cudaLaunchAttributeMemSyncDomainMap` 允许开发者将逻辑域（logical domain）映射到物理域（physical domain）。例如，可以将 `default_` 映射到物理域 0、`remote` 映射到物理域 1，也可以将不同流分别固定到不同的物理域以隔离同步开销。值得注意的是，这两个属性会在流捕获期间被复制到 CUDA Graph 的节点中；Graph 执行时使用的是节点自身的属性，而非启动 Graph 所在的流属性。

**Fabric Handle**
专为多节点 NVLink 集群设计的跨节点句柄。它与单节点的 IPC handle 处于同一抽象层次，但额外封装了跨节点路由所需的信息。参与通信的各 rank 需要交换 fabric handle，然后在本地转换为 process-local device pointer。

**VMM（Virtual Memory Management）API**
CUDA 驱动级 API 提供的虚拟内存管理接口。与 Legacy IPC API 相比，VMM 允许在分配时就精确控制内存的 peer accessibility 和 sharing 属性，但代价是必须使用相对底层的 Driver API，编程复杂度更高。

## API / 机制详解

### 内存同步域映射相关 API

在 Hopper 架构上，流默认会携带一个内存同步域映射。开发者可以通过 `cudaStreamSetAttribute` 显式设置该映射。示例中的代码模式如下：

```cpp
cudaLaunchAttributeValue mapAttr;
mapAttr.memSyncDomainMap.default_ = 0;  // 将逻辑 default 域映射到物理域 0
mapAttr.memSyncDomainMap.remote  = 1;  // 将逻辑 remote 域映射到物理域 1
cudaStreamSetAttribute(stream, cudaLaunchAttributeMemSyncDomainMap, &mapAttr);
```

如果需要让某条流完全忽略逻辑域设置、直接绑定到特定物理域，可以将 `default_` 和 `remote` 都设为同一个物理域 ID。与所有启动属性一样，该映射统一暴露于 CUDA 流、`cudaLaunchKernelEx` 的单个启动参数，以及 CUDA Graph 的 kernel 节点。典型用法是在流级别设置映射，在启动级别（或某段流使用的首尾）设置逻辑域。如果通过流捕获构建 Graph，这些属性会被复制到 Graph 节点；Graph 启动时，节点自身的域属性生效，承载 Graph 的流所附带的域属性则被忽略。

对于 sm_61（Pascal）这类较早的架构，内存同步域机制并不存在，因此上述 API 调用要么在旧驱动上编译失败，要么即使编译通过也无效。实际开发中需要根据计算能力做条件编译或运行时检测。

### 传统 Legacy IPC API

这是 CUDA Runtime 提供的最直接的 IPC 支持，也是大多数开发者首选的路径。其核心函数包括：

**`cudaIpcGetMemHandle(cudaIpcMemHandle_t* handle, void* devPtr)`**
- **作用**：将当前进程上下文中由 `devPtr` 指向的设备内存块转换为一个 IPC handle。
- **调用时机**：在分配了设备内存并初始化数据之后，且需要将该段内存共享给其他进程之前。
- **前置条件**：`devPtr` 必须是由 `cudaMalloc`（而非 `cudaMallocManaged` 或 `cudaMallocHost`）分配的有效设备指针；调用者必须处于有效的 CUDA 上下文中。
- **调用后得到**：一个可被序列化并通过 OS IPC 通道传输的 `cudaIpcMemHandle_t` 结构体。
- **常见错误**：传入 Unified Memory 指针将导致 API 失败；在未设置设备上下文的情况下调用可能返回上下文未初始化的错误。

**`cudaIpcOpenMemHandle(void** devPtr, cudaIpcMemHandle_t handle, unsigned int flags)`**
- **作用**：接收来自其他进程的 IPC handle，在当前进程的 CUDA 上下文中创建一个指向同一段物理显存的 device pointer。
- **调用时机**：在通过 OS IPC 机制（如共享内存、socket、管道）接收到对方进程发送的 handle 之后。
- **前置条件**：当前进程必须已经初始化 CUDA 上下文（例如通过 `cudaSetDevice`）；接收到的 handle 必须未被篡改且确实指向本节点内可访问的显存。
- **调用后得到**：一个仅对当前进程有效的 `devPtr`，可与普通 `cudaMalloc` 返回的指针一样用于 kernel 启动、`cudaMemcpy` 等操作。
- **flags**：常用值为 `cudaIpcMemLazyEnablePeerAccess`，表示按需启用对等访问。如果 GPU 之间已经通过 NVLink 或 PCIe 建立了 P2P 连接，该标志允许延迟建立映射直到首次访问。
- **常见错误**：在已经持有该 handle 对应内存的上下文中重复打开同一个 handle，或传入无效的 flags，都可能导致未定义行为或资源泄漏。

**`cudaIpcCloseMemHandle(void* devPtr)`**
- **作用**：关闭由 `cudaIpcOpenMemHandle` 打开的内存映射，释放当前进程中与该 IPC 内存关联的引用。
- **注意**：这不会释放原始物理显存，只有创建者进程调用 `cudaFree` 才会真正回收内存。

**事件 IPC API**
事件的共享遵循完全对称的模式：
- `cudaIpcGetEventHandle(cudaIpcEventHandle_t* handle, cudaEvent_t event)`：将本地事件编码为 IPC handle。
- `cudaIpcOpenEventHandle(cudaEvent_t* event, cudaIpcEventHandle_t handle)`：在另一个进程中解码为本地事件句柄。
- 解码后的事件可用于 `cudaStreamWaitEvent`，实现跨进程同步。

### VMM API 路径

VMM API 属于 CUDA Driver API 层面，提供了一组以 `cuMem*` 为前缀的函数。它的优势在于"细粒度控制"：开发者可以在分配阶段就通过 `cuMemCreate` 和 `cuMemMap` 的组合，显式指定某段物理内存是否允许被特定 Peer GPU 访问，以及是否支持 IPC 导出（通过 `cuMemExportToShareableHandle`）。与 Legacy API 的事后"获取 handle"不同，VMM 在内存创建时就定义了它的共享能力，因此在安全性和可预测性上更优。

然而，VMM 的代价是编程模型更复杂。开发者需要手动处理物理分配（`CUmemGenericAllocationHandle`）、虚拟地址映射、访问权限描述符（`CUmemAccessDesc`）等概念。对于习惯于 `cudaMalloc` 简单模型的开发者而言，切换到 VMM 意味着要重写内存管理层的相当一部分代码。此外，VMM 的 shareable handle 机制与 Legacy IPC 并不完全等价：它生成的 handle 通常需要配合 OS 特定的 handle 类型（如 POSIX file descriptor）进行传输。

### Fabric Handle 与多节点通信

在多节点 NVLink 集群中，跨节点的 GPU 之间无法直接使用单节点的 `cudaIpcMemHandle_t`，因为后者假设所有 GPU 受同一 OS 实例内的同一 NVIDIA 驱动管理。Fabric Handle 在更高层级抽象了这一问题：它允许在节点 A 上将设备内存导出为一个"fabric"句柄，通过网络层（通常由 NVSHMEM、NCCL 或应用自身的通信层）传输到节点 B，再由节点 B 导入为本地 device pointer。尽管章节正文未列出具体的 Fabric API 函数名，但它明确指出这是单节点 IPC 思想的直接扩展——核心仍然是"创建可移植句柄 -> 交换 -> 还原为本地指针"的三步模型。
