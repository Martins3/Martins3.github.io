## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/graphics-interop.html>


本章（CUDA Interoperability with APIs）系统性地介绍了 CUDA 与外部图形/计算 API 之间的互操作机制，核心目标是让 CUDA 能够直接读写由其他 API（如 OpenGL、Direct3D、Vulkan）创建或管理的 GPU 资源，从而避免不必要的数据回传（device-to-host）和冗余拷贝。章节内容分为两大主线：一是传统的 **Graphics Interoperability**，即 CUDA 直接映射（register/map）OpenGL 与 Direct3D 资源；二是更为通用的 **External Resource Interoperability**，通过操作系统级句柄（file descriptor、NT handle 等）导入/导出内存对象与同步对象，支持 Vulkan、Direct3D 12 以及 NVIDIA 的 NVSCI（NvSciBuf/NvSciSync）接口。两条主线覆盖了从资源注册、设备指针/数组映射、到跨 API 同步的完整生命周期，并给出了 SLI 多 GPU 配置下的特殊处理建议。

## 背景与要解决的问题

在 GPU 计算与图形渲染并存的场景中（例如物理模拟后渲染、视频处理后显示、深度学习推理结果可视化），数据通常需要在 CUDA 与图形 API 之间流转。最原始的方案是：CUDA 将结果写回主机内存（Host），再由图形 API 从主机内存上传到 GPU 显存。这种方式不仅带宽利用率低，而且引入了显著的延迟。

CUDA 互操作机制旨在解决"零拷贝"共享 GPU 数据的问题。其设计哲学是：既然数据已经在 GPU 显存上，就应当允许不同 API 的上下文通过统一的地址空间或外部句柄直接访问同一份物理内存。本章描述的互操作方案可分为两类：

1. **直接映射型（Graphics Interoperability）**：CUDA 驱动原生支持将 OpenGL 的 Buffer/Texture/Renderbuffer 以及 Direct3D 的 Buffer/Texture/Surface 注册为 `cudaGraphicsResource`，随后映射为设备指针或 CUDA array。这种方式对应用透明，但通常要求 CUDA 与图形 API 运行在同一进程、同一 GPU、且满足特定的驱动与上下文约束。

2. **外部资源导入导出型（External Resource Interoperability）**：通过操作系统级句柄（Linux 上的 `fd`，Windows 上的 `NT handle` 或 `D3DKMT handle`）将 Vulkan、Direct3D 12、NVSCI 等资源显式导入 CUDA。这种方式更加灵活，支持跨进程甚至跨 API 的共享，并允许通过外部信号量（semaphore/fence）定义细粒度的执行顺序。

## 核心概念与术语

| 术语 | 说明 |
|------|------|
| `cudaGraphicsResource` | CUDA 对注册后的图形资源的抽象句柄。注册成功后，该资源可被多次 map/unmap，最终通过 `cudaGraphicsUnregisterResource` 释放。 |
| Register / Map / Unmap / Unregister | 图形互操作的四个核心生命周期操作。Register 代价较高，建议每个资源只执行一次；Map 将资源锁定供 CUDA 访问，期间其他 API 访问该资源会产生未定义行为。 |
| `cudaExternalMemory_t` | 外部内存对象的 CUDA 句柄。通过 `cudaImportExternalMemory()` 创建，可映射为设备指针或 mipmapped array。 |
| `cudaExternalSemaphore_t` | 外部同步对象的 CUDA 句柄。通过 `cudaImportExternalSemaphore()` 创建，可在 CUDA stream 上执行 signal/wait。 |
| Dedicated memory | 外部内存对象若为 API 独占分配（如 Vulkan dedicated allocation、D3D12 committed resource），导入时必须设置 `cudaExternalMemoryDedicated` 标志。 |
| Opaque handle / File descriptor / NT handle | 不同 OS 与 API 导出的资源句柄类型。Linux 上通常使用 `fd`（CUDA 导入后接管所有权）；Windows 上使用 `NT handle`（应用自行管理生命周期）。 |
| Timeline semaphore / Binary semaphore | Timeline semaphore 内部为 64-bit 计数器，允许同一线程内多次 signal/wait 并建立 happens-before 关系；Binary semaphore 仅有 0/1 状态，signal 与 wait 必须严格交替。 |
| NVSCI (NvSciBuf / NvSciSync) | NVIDIA 软件通信接口，主要用于嵌入式/车载场景（如 NVIDIA DRIVE），提供跨模块的 buffer 分配与同步能力。 |
| SLI (Scalable Link Interface) | 多 GPU 硬件配置。显式 SLI 下，应用需为每个 GPU 创建独立 CUDA context，并针对每帧渲染的 GPU 注册资源。 |

## API / 机制详解

### 4.19.1 Graphics Interoperability

Graphics Interoperability 是 CUDA 最早支持的互操作模式，其本质是在 CUDA 地址空间中为 OpenGL 或 Direct3D 资源建立临时或长期的别名。核心步骤包括：注册资源（register）、映射资源（map）、获取设备访问地址（get pointer/array）、解除映射（unmap）、注销资源（unregister）。

#### 4.19.1.1 OpenGL Interoperability

OpenGL 侧可被共享的资源包括：Buffer Object（如 VBO）、Texture、Renderbuffer。注册函数有两类：

- **`cudaGraphicsGLRegisterBuffer(cudaGraphicsResource **resource, GLuint buf, unsigned int flags)`**：将 OpenGL buffer object 注册为 CUDA 资源。注册后通过 `cudaGraphicsResourceGetMappedPointer()` 获取设备指针，CUDA kernel 可以像操作普通设备内存一样读写该 buffer。
- **`cudaGraphicsGLRegisterImage(cudaGraphicsResource **resource, GLuint image, GLenum target, unsigned int flags)`**：将 OpenGL texture 或 renderbuffer 注册为 CUDA array。注册后通过 `cudaGraphicsSubResourceGetMappedArray()` 获取 CUDA array，可用于 surface load/store 或 texture fetch。

`flags` 参数可指定使用提示，例如 `cudaGraphicsMapFlagsWriteDiscard` 表示 CUDA 将完全覆写该资源，驱动可据此优化缓存一致性。注意：当资源处于 mapped 状态时，OpenGL、Direct3D 或其他 CUDA context 若同时访问该资源，结果是未定义行为。

**支持的纹理格式**：`cudaGraphicsGLRegisterImage()` 支持含 1/2/4 个通道、内部类型为 float（如 `GL_RGBA_FLOAT32`）、normalized integer（如 `GL_RGBA8`）和 unnormalized integer（如 `GL_RGBA8UI`）的格式。如果注册时带有 `cudaGraphicsRegisterFlagsSurfaceLoadStore` 标志，则支持写入操作。

**OpenGL 互操作的典型代码路径**：
1. `glGenBuffers` / `glBufferData` 创建并初始化 VBO；
2. `cudaGraphicsGLRegisterBuffer` 注册；
3. 渲染循环内：`cudaGraphicsMapResources` -> `cudaGraphicsResourceGetMappedPointer` -> 启动 CUDA kernel -> `cudaGraphicsUnmapResources`；
4. OpenGL 侧绑定 VBO 并渲染；
5. 程序退出时：`cudaGraphicsUnregisterResource` -> `glDeleteBuffers`。

#### 4.19.1.2 Direct3D Interoperability (Direct3D 11)

Direct3D 互操作支持 D3D9、D3D10 和 D3D11（不支持 D3D12，后者归入 External Resource Interoperability）。以 D3D11 为例：

- 注册函数为 `cudaGraphicsD3D11RegisterResource()`。
- CUDA context 只能与 `DriverType = D3D_DRIVER_TYPE_HARDWARE` 创建的 D3D11 设备互操作。
- D3D11 的 buffer、texture、surface 均可注册。

原文给出的 2D Texture 互操作示例揭示了一个重要细节：CUDA 无法直接向 texture 写入数据（因为映射后看到的是 `cudaArray`，而 `cudaArray` 只能作为 texture/surface 访问）。因此示例中额外使用 `cudaMallocPitch` 分配了一块线性设备内存作为 CUDA 的写入缓冲区，kernel 修改线性内存后，再由后续步骤（未在摘录中完整展示）将数据同步到 texture。这提醒开发者：**texture 的互操作往往需要一个可写的线性中间缓冲区**。

#### 4.19.1.3 SLI 配置中的互操作性

在显式 SLI（Explicit SLI）系统中，多个物理 GPU 协同渲染同一帧或交替渲染帧。此时有以下几点必须注意：

1. **显存放大效应**：在一个 CUDA device 上分配内存，会在 SLI 组内的其他 GPU 上也占用内存，因此分配可能比单 GPU 系统更早失败。
2. **多 CUDA context**：建议为 SLI 配置中的每个 GPU 创建一个独立的 CUDA context，避免跨 device 的不必要数据传输。
3. **设备选择**：通过 `cudaD3D[9|10|11]GetDevices()` 或 `cudaGLGetDevices()` 查询当前帧和下一帧分别由哪个 GPU 渲染。将 `deviceList` 参数设为 `cudaD3D11DeviceListCurrentFrame` 或 `cudaGLDeviceListCurrentFrame`，获取当前负责渲染的 device handle，然后将资源注册到该 device 上。
4. **资源归属**：`cudaGraphicsD3D11RegisterResource` 和 `cudaGraphicsGLRegisterBuffer/Image` 返回的资源只能在注册时所在的 device 上使用。因此如果不同帧的数据在不同 CUDA device 上计算，每 device 都必须独立注册一次资源。

### 4.19.2 External Resource Interoperability

External Resource Interoperability 比传统 Graphics Interoperability 更底层、更通用。它允许 CUDA 导入显式导出的资源句柄，这些句柄通常是操作系统原生的（Linux 的 `fd`、Windows 的 `NT handle`）。该机制不依赖 CUDA 与图形 API 在同一进程内隐式协作，而是显式地共享内存与同步原语。

可导入的资源分为两类：
- **Memory objects**：导入后可映射为设备指针或 CUDA mipmapped array；
- **Synchronization objects**：导入后可在 CUDA stream 上异步 signal 或 wait，用于建立跨 API 的执行顺序。

#### 4.19.2.1 Vulkan Interoperability

Vulkan-CUDA 互操作是 External Resource Interoperability 中最复杂的场景之一，涉及内存对象与同步对象的双重共享。

**扩展要求**：
- Instance 级别：`VK_KHR_external_memory_capabilities`、`VK_KHR_external_semaphore_capabilities`。
- Device 级别：`VK_KHR_external_memory`、`VK_KHR_external_semaphore`、`VK_KHR_timeline_semaphore`。
- 平台特定：Linux 需要 `VK_KHR_external_memory_fd` 和 `VK_KHR_external_semaphore_fd`；Windows 需要 `VK_KHR_external_memory_win32` 和 `VK_KHR_external_semaphore_win32`。

**设备匹配**：Vulkan 创建的内存和同步对象只能在创建它们的物理设备上被 CUDA 导入。匹配方式是对比 UUID：`vkPhysicalDeviceIDProperties.deviceUUID` 与 CUDA 的 `cudaDeviceProp.uuid`。代码逻辑为遍历所有 CUDA device，用 `memcmp` 比对 UUID，一致则调用 `cudaSetDevice`。注意：该 Vulkan 物理设备不能属于包含多个物理设备的 device group。

**导出 Vulkan 内存对象**：创建 buffer 时，需要在 `VkBufferCreateInfo` 的 `pNext` 链上附加 `VkExternalMemoryBufferCreateInfo`，设置 `handleTypes`（如 `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT` 或 `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT`）。随后在内存分配时，在 `VkMemoryAllocateInfo` 的 `pNext` 链上附加 `VkExportMemoryAllocateInfoKHR`，同样指定句柄类型。Windows 下还需额外提供 `VkExportMemoryWin32HandleInfoKHR` 以设置访问权限。

**导出 Vulkan 同步对象**：创建 semaphore 时，在 `VkSemaphoreCreateInfo` 的 `pNext` 链上附加 `VkExportSemaphoreCreateInfoKHR`。对于 timeline semaphore，还需在此之前附加 `VkSemaphoreTypeCreateInfo`，将 `semaphoreType` 设为 `VK_SEMAPHORE_TYPE_TIMELINE`。Binary semaphore 和 Timeline semaphore 的导出方式不同，导入到 CUDA 时使用的 handle type 也不同（timeline 对应 `cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd/Win32`）。

**导入内存到 CUDA**：
- 填充 `cudaExternalMemoryHandleDesc`，根据 Vulkan 导出的句柄类型设置 `type`（`cudaExternalMemoryHandleTypeOpaqueFd` / `OpaqueWin32` / `OpaqueWin32Kmt`）。
- 设置 `size` 为内存对象的字节大小。
- Linux 下将 `fd` 填入 `handle.fd`；Windows 下将 `NT handle` 填入 `handle.win32.handle`。
- 调用 `cudaImportExternalMemory(&extMem, &desc)`。
- 如果是 dedicated memory，必须设置 `desc.flags |= cudaExternalMemoryDedicated`。

**映射为设备指针**：调用 `cudaExternalMemoryGetMappedBuffer(void **devPtr, cudaExternalMemory_t extMem, const cudaExternalMemoryBufferDesc *desc)`。`desc` 中的 `offset` 和 `size` 必须与 Vulkan 侧创建 buffer 时指定的范围一致。返回的 `devPtr` 必须通过 `cudaFree()` 释放，而 `extMem` 本身通过 `cudaDestroyExternalMemory()` 释放。

**映射为 mipmapped array**：调用 `cudaExternalMemoryGetMappedMipmappedArray()`，需要提供 `offset`、`formatDesc`、`extent`、`flags`、`numLevels`，且这些参数必须与 Vulkan 侧创建的 image 完全一致。若该 image 在 Vulkan 中可作为 color target 绑定，则 flags 中需包含 `cudaArrayColorAttachment`。

**导入同步对象到 CUDA**：填充 `cudaExternalSemaphoreHandleDesc`，根据 semaphore 类型选择 `cudaExternalSemaphoreHandleTypeOpaqueFd`、`TimelineSemaphoreFd`、`OpaqueWin32`、`TimelineSemaphoreWin32` 等。调用 `cudaImportExternalSemaphore(&sem, &desc)` 完成导入。

**Signal / Wait**：
- `cudaSignalExternalSemaphoresAsync()`：对导入的 semaphore 发起 signal。对于 timeline semaphore，需在 `cudaExternalSemaphoreSignalParams.params.fence.value` 中指定 signal 值。
- `cudaWaitExternalSemaphoresAsync()`：对导入的 semaphore 发起 wait。对于 timeline semaphore，需在 `cudaExternalSemaphoreWaitParams.params.fence.value` 中指定 wait 值。
- **顺序约束**：binary semaphore 的 wait 必须在对应的 signal 之后发起；timeline semaphore 的 wait 值必须大于等于已 signal 的值。对应的 signal/wait 必须在导出 API（Vulkan）侧存在配对的 wait/signal。

#### 4.19.2.2 Direct3D 12 Interoperability

D3D12 资源通过 LUID（Locally Unique Identifier）匹配 CUDA 设备。`ID3D12Device::GetAdapterLuid()` 返回 D3D12 的 LUID，而 `cudaDeviceProp.luid` 包含 CUDA 设备的 LUID。通过逐字节比对 `LowPart` 和 `HighPart` 即可找到对应的 CUDA device。注意 D3D12 设备不能创建在 linked node adapter 上（`GetNodeCount()` 必须为 1）。

**导入内存对象**：D3D12 可导入两类资源：
- **Heap**：通过 `D3D12_HEAP_FLAG_SHARED` 创建的共享 heap，使用 `cudaExternalMemoryHandleTypeD3D12Heap` 导入。
- **Committed Resource**：通过 `D3D12_HEAP_FLAG_SHARED` 创建的 committed resource，使用 `cudaExternalMemoryHandleTypeD3D12Resource` 导入，**必须**设置 `cudaExternalMemoryDedicated`。

导入时使用 `NT handle`（`handle.win32.handle`）或 named handle（`handle.win32.name`）。Windows 下 CUDA 不接管 NT handle 的所有权，应用必须显式 `CloseHandle`。

**映射 buffer 与 mipmapped array**：接口与 Vulkan 完全一致，使用 `cudaExternalMemoryGetMappedBuffer()` 和 `cudaExternalMemoryGetMappedMipmappedArray()`。对于 mipmapped array，如果 D3D12 资源允许作为 render target（`D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET`），需设置 `cudaArrayColorAttachment`。

**导入同步对象**：D3D12 的 fence（通过 `D3D12_FENCE_FLAG_SHARED` 创建）可导入为 CUDA semaphore，type 为 `cudaExternalSemaphoreHandleTypeD3D12Fence`。同样通过 NT handle 或 named handle 导入，应用负责关闭 handle。

**Signal / Wait**：与 Vulkan timeline semaphore 类似，通过 `params.params.fence.value` 指定 fence 值。D3D12 侧必须存在对应的 wait/signal。

#### 4.19.2.3 NVSCI Interoperability

NvSciBuf 和 NvSciSync 主要用于车载/嵌入式系统（NVIDIA DRIVE）。

**NvSciBuf**：
- 创建 `NvSciBufObj` 时，需在 attribute list 中设置 `NvSciBufGeneralAttrKey_GpuId`（填充 CUDA 设备的 UUID），并可选项设置 CPU 访问权限、对齐方式、cache/compression 策略、访问权限（read-only 可通过 `NvSciBufObjDupWithReducePerm` 创建副本）。
- 导入到 CUDA 时，使用 `cudaExternalMemoryHandleTypeNvSciBuf`，`handle.nvSciBufObject` 指向 `NvSciBufObj`。
- 导入前需要查询 `NvSciBufGeneralAttrKey_GpuSwNeedCacheCoherency` 等属性，若 cache coherency 为 false，需配合 NvSciSync 使用恰当的 barrier。
- 映射 buffer 和 mipmapped array 的方式与 Vulkan/D3D12 相同，但注意 **mipmapped array 的 numLevels 必须为 1**。

**NvSciSync**：
- 通过 `cudaDeviceGetNvSciSyncAttributes()` 生成 CUDA device 兼容的 attribute list。分别对 signaler 和 waiter 调用，flags 为 `CUDA_NVSCISYNC_ATTR_SIGNAL` 和 `CUDA_NVSCISYNC_ATTR_WAIT`。
- 使用 `NvSciSyncAttrListReconcile()` 调和 attribute list 后，分配 `NvSciSyncObj`。
- 导入到 CUDA 时 type 为 `cudaExternalSemaphoreHandleTypeNvSciSync`，`handle.nvSciSyncObj` 指向 `NvSciSyncObj`。注意：导入后 `NvSciSyncObj` 的所有权仍属于应用，不可提前释放。
- Signal 时通过 `params.params.nvSciSync.fence` 传入 fence 参数，初始化后的 fence 会被对应的 wait 操作等待。若 `NvSciBufGeneralAttrKey_GpuSwNeedCacheCoherency` 为 false，可在 `signalParams.flags` 中设置 `cudaExternalSemaphoreSignalSkipNvSciBufMemSync` 以跳过默认的内存同步。

## 典型工作流程 / 调用顺序

### OpenGL VBO 互操作流程
1. **初始化 OpenGL 上下文**：确保 OpenGL context 在当前线程上处于 current 状态。  
2. **注册资源**：调用 `cudaGraphicsGLRegisterBuffer` 将 VBO 注册为 `cudaGraphicsResource`。  
3. **映射资源**：调用 `cudaGraphicsMapResources` 锁定资源供 CUDA 使用。  
4. **获取指针**：调用 `cudaGraphicsResourceGetMappedPointer` 获取设备指针。  
5. **执行 CUDA Kernel**：使用获取到的指针启动 kernel。  
6. **解除映射**：调用 `cudaGraphicsUnmapResources`，解除锁定，使 OpenGL 可以安全访问。  
7. **渲染**：OpenGL 绑定并渲染该 VBO。  
8. **注销资源**：程序退出时调用 `cudaGraphicsUnregisterResource`，随后删除 OpenGL buffer。

### Vulkan-CUDA 互操作流程
1. **初始化 Vulkan**：启用 `VK_KHR_external_memory` 等扩展，创建 instance 与 device。  
2. **匹配设备**：通过 UUID 对比找到与 Vulkan physical device 对应的 CUDA device，并 `cudaSetDevice`。  
3. **创建并导出外部资源**：在 Vulkan 侧创建带有 export flag 的 buffer/image 和 semaphore，获取 OS 句柄（fd 或 NT handle）。  
4. **导入内存到 CUDA**：调用 `cudaImportExternalMemory`。  
5. **映射为可用形式**：调用 `cudaExternalMemoryGetMappedBuffer` 或 `cudaExternalMemoryGetMappedMipmappedArray`。  
6. **导入同步对象到 CUDA**：调用 `cudaImportExternalSemaphore`。  
7. **交替执行与同步**：Vulkan 渲染后 signal semaphore -> CUDA wait -> CUDA kernel -> CUDA signal -> Vulkan wait -> Vulkan 渲染。Timeline semaphore 可通过单个 semaphore 的递增计数完成同步。

### Direct3D 12 - CUDA 互操作流程
1. **匹配设备**：通过 LUID 对比找到对应 CUDA device。  
2. **创建共享资源**：D3D12 侧创建 `D3D12_HEAP_FLAG_SHARED` heap 或 committed resource，以及 `D3D12_FENCE_FLAG_SHARED` fence。  
3. **获取 NT handle**：通过 D3D12 API 导出资源的 NT handle。  
4. **导入内存与同步对象**：分别调用 `cudaImportExternalMemory`（dedicated resource 需加 flag）和 `cudaImportExternalSemaphore`。  
5. **映射与执行**：与 Vulkan 流程一致，通过 signal/wait 建立执行顺序。

## 关键限制、边界条件与兼容性

1. **OpenGL 上下文约束**：调用任何 OpenGL 互操作 API 时，资源所属的 OpenGL context 必须在当前线程上 current。否则注册或映射可能失败或产生未定义行为。

2. **Bindless Texture 限制**：如果 OpenGL texture 已被设为 bindless（例如通过 `glGetTextureHandle` 或 `glGetImageHandle` 获取了 handle），则**不能**再被注册到 CUDA。应用必须先注册，再请求 bindless handle。

3. **D3D11 驱动类型限制**：CUDA 只能与 `D3D_DRIVER_TYPE_HARDWARE` 创建的 D3D11 设备互操作，软件/参考设备（Reference/WARP）不受支持。

4. **Map 期间的排他性访问**：当资源被 `cudaGraphicsMapResources` 映射后，OpenGL、Direct3D 或其他 CUDA context 若同时访问该资源，结果未定义。这意味着 map/unmap 的粒度和时机直接影响正确性。

5. **外部内存映射的严格匹配**：通过 `cudaExternalMemoryGetMappedBuffer` 或 `cudaExternalMemoryGetMappedMipmappedArray` 建立的映射，其 offset、size、format、extent、mip levels 必须与导出 API（Vulkan/D3D12）侧完全一致。任何不匹配都会导致未定义行为。

6. **句柄所有权语义差异**：
   - Linux `fd`：CUDA 导入成功后**接管**该 fd 的所有权，后续应用不应再使用或关闭该 fd。
   - Windows `NT handle`：CUDA**不接管**所有权，应用必须自行 `CloseHandle`。NT handle 持有资源的引用，必须先关闭 handle 才能释放底层内存。
   - `D3DKMT handle`（Win32 KMT）：不持有引用，当所有其他引用销毁后自动失效。

7. **SLI 显存放大**：在 SLI 系统中，单 GPU 上的分配会在 SLI 组内所有 GPU 上占用内存，因此可用显存显著减少。

8. **NvSciBuf 的 mip level 限制**：通过 NvSciBuf 导入的内存对象若映射为 mipmapped array，**numLevels 必须为 1**。

9. **NvSciSync 的内存同步**：默认情况下，signal NvSciSync 时会执行所有已导入 NvSciBuf 的内存同步。若应用已确认 `GpuSwNeedCacheCoherency` 为 false，应设置 `cudaExternalSemaphoreSignalSkipNvSciBufMemSync` 以避免不必要的同步开销。

## 常见陷阱与调试建议

1. **注册代价高，避免重复注册**：`cudaGraphicsGLRegisterBuffer` 和 `cudaGraphicsD3D11RegisterResource` 涉及驱动内部的资源追踪和元数据建立，调用开销较大。建议每个资源在整个生命周期内只注册一次，而不是每帧都注册/注销。

2. **每 context 独立注册**：如果应用创建了多个 CUDA context（例如 SLI 多 GPU 场景），每个 context 都必须独立注册同一图形资源，注册结果不能跨 context 复用。

3. **Map/Unmap 必须成对**：忘记 unmap 会导致 OpenGL/Direct3D 无法正常访问资源；在已 map 状态下重复 map 通常也会报错。

4. **格式和 pitch 匹配**：在 D3D11 texture 互操作中，CUDA 侧的 `cudaMallocPitch` 分配的 pitch 应与 D3D11 texture 的 row pitch 对齐并匹配。建议在初始化阶段打印两边的 pitch 值进行比对。

5. **设备匹配是硬性要求**：UUID（Vulkan）或 LUID（D3D12）不匹配时，`cudaImportExternalMemory` 或 `cudaImportExternalSemaphore` 可能静默失败，也可能在后续访问时产生未定义行为。务必在初始化阶段严格比对并断言匹配成功。

6. **注意 binary semaphore 的配对顺序**：binary semaphore 的 signal 与 wait 必须严格交替。若 CUDA signal 后 Vulkan 忘记 wait，或 Vulkan signal 后 CUDA 忘记 wait，都会导致死锁或资源泄漏。

7. **Timeline semaphore 的初始值**：创建 Vulkan timeline semaphore 时，`initialValue` 通常设为 0。后续的 signal 和 wait 值必须单调递增，否则行为未定义。

8. **检查 `cudaExternalMemoryDedicated`**：导入 dedicated allocation（Vulkan dedicated memory、D3D12 committed resource）时忘记设置此标志，是外部资源导入失败的常见原因之一。

9. **NT handle 生命周期**：Windows 开发者容易遗漏 `CloseHandle`，导致句柄泄漏和资源无法释放。建议将 handle 封装在 RAII 对象中，确保异常路径也能关闭。

## 一个最小可运行示例的说明

本章提供的示例 `chapter_demo.cu` 聚焦于 **OpenGL Graphics Interoperability** 中最经典的 VBO（Vertex Buffer Object）场景，对应原文 4.19.1.1 节 "OpenGL Interoperability" 的 `simpleGL` 示例思想，但进行了极度简化，使其成为一个无需复杂窗口事件循环、可在命令行直接验证的最小可运行单元。

**示例设计动机**：
- OpenGL-CUDA 互操作是 CUDA 互操作能力的入门场景，概念最少（仅需 register/map/unmap），却完整覆盖了 Graphics Interoperability 的生命周期。
- 选择 VBO 而非 texture/array，是因为 buffer 的映射结果是一个普通设备指针，kernel 编写最直观，验证逻辑最简单（读回 CPU 对比数值即可）。
- 使用 GLX（X11）直接创建 OpenGL 上下文，避免了对 GLFW、GLUT、GLEW 等第三方库的依赖。只要系统安装了 Mesa 或 NVIDIA 驱动提供的 `libGL.so`，即可编译链接。

**示例核心流程**：
1. 通过 X11 和 GLX 创建一个最小化的窗口（实际尺寸 1x1 即可）并建立 OpenGL 上下文。
2. 创建一个包含 `N * sizeof(float4)` 字节的 VBO，初始数据全零。
3. 调用 `cudaGraphicsGLRegisterBuffer` 将 VBO 注册为 `cudaGraphicsResource`，flags 设为 `cudaGraphicsMapFlagsWriteDiscard`，向驱动声明 CUDA 将完全覆写该 buffer。
4. 在循环中模拟 "Map -> Kernel -> Unmap" 的帧更新逻辑：CUDA kernel 将每个顶点计算为 `(index, index * 2.0f, index * 3.0f, 1.0f)` 的固定模式。
5. Unmap 后通过 `glGetBufferSubData` 将数据读回 CPU，验证 kernel 写入是否正确。
6. 清理阶段依次执行 `cudaGraphicsUnregisterResource`、`glDeleteBuffers`、销毁 GLX 上下文和 X11 窗口。

**sm_61 兼容性与降级处理**：
- 示例中 kernel 仅使用基本的浮点运算和全局内存访问，不依赖任何 Pascal 架构之后的特性（如 Tensor Core、WMMA、异步拷贝等），因此天然兼容 sm_61（GTX 1060）。
- 代码在 `main` 函数入口处通过 `cudaGetDeviceCount` 和 `cudaGetDeviceProperties` 检查运行环境。如果系统中没有任何 CUDA-capable device，程序会打印说明信息并优雅退出，而不是崩溃。
- 如果检测到设备存在但计算能力低于 6.1，程序同样会打印警告并退出。由于 kernel 非常简单，实际上 sm_50 及以上均可运行，但 Makefile 中固定使用 `-arch=sm_61` 以符合题目要求。
- 若 X11 显示环境不可用（`XOpenDisplay` 返回 NULL），程序会打印诊断信息并返回错误码，避免在 headless 服务器上产生难以理解的段错误。
- 由于 sm_61 不支持某些较新的 CUDA 特性（如某些版本的 cooperative groups 高级特性或特定异步 barrier），示例完全规避了这些 API，仅使用 CUDA Runtime 的基础功能。

**为什么这样设计**：
- 由于当前编译环境可能没有安装 CUDA Toolkit 和 X11 显示环境，示例的重点是**代码正确性**和**接口使用范式**，而非在此物理机上实际运行。Makefile 中指定的 `nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2` 参数确保在目标环境（CUDA 12.8 + sm_61）上可直接编译。
- 通过 `CUDA_CHECK` 宏对所有 CUDA API 进行错误检查，便于在目标设备上运行出问题时的快速定位。
