## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/extended-gpu-memory.html>


本章节介绍 CUDA 的 Extended GPU Memory（EGM）特性。EGM 利用高带宽 NVLink-C2C 互连技术，使 GPU 能够高效访问整个系统中的所有内存资源，既包括单节点内的 CPU 附加内存和 GPU HBM，也覆盖多节点环境下通过 NVSwitch  fabrics 连接的远端内存。EGM 的核心价值在于打破传统 CUDA 编程中 GPU 只能直接访问自身显存（或通过统一寻址访问本机 pinned host memory）的限制，让 GPU 线程可以以接近本地带宽的速度访问任意 NUMA 节点上的物理内存。

EGM 主要面向集成式 CPU-GPU NVIDIA 系统（例如 Grace Hopper 架构平台），其物理内存分配可被同一拓扑内的任意 GPU 线程访问。本地访问走 NVLink-C2C，远端访问走 GPU-GPU NVLink 或 NVLink-C2C，整个路由由硬件保证，不依赖传统 PCIe 路径。

## 背景与要解决的问题

在传统的 CUDA 系统中，GPU 访问内存存在以下层级和瓶颈：

1. **设备内存（HBM/GDDR）**：带宽最高，但容量受限于显卡物理容量。
2. **零拷贝内存（Zero-Copy）**：通过 `cudaHostAlloc`/`cudaMallocHost` 分配的 pinned 内存，GPU 可通过 PCIe 直接访问，但带宽受 PCIe 限制，且通常只能访问与当前 GPU 处于同一 NUMA 节点或同一物理机上的 host memory。
3. **统一内存（Unified Memory）**：由 CUDA 运行时自动迁移页，虽然编程模型简单，但在大规模数据共享和多 GPU 场景下，页迁移开销和策略不可控，难以满足极致性能需求。

在多处理器 NUMA 架构中，CPU 内存被划分为多个节点，每个节点有自己的处理器和本地内存。传统 CUDA 无法让 GPU 直接以高带宽访问远端 NUMA 节点的内存，跨节点或跨机的数据交换往往依赖显式拷贝（`cudaMemcpy`）或 NVLink P2P（仅显存之间），缺乏对主机侧远端内存的直接高带宽访问能力。

EGM 的出现正是为了解决这一问题：
- 允许 GPU 线程直接访问所有系统内存资源（包括 CPU attached memory 和 HBM3）。
- 利用 NVLink-C2C 和 NVSwitch fabric 保证跨 socket、跨节点的内存访问带宽。
- 提供显式的物理内存分配接口（VMM 和 Memory Pool），让用户精确控制内存的物理位置和访问权限。

## 核心概念与术语

### 2.1 NUMA 与 NUMA ID

NUMA（Non-Uniform Memory Access）是一种多处理器系统的内存架构，内存被划分为多个节点（node），每个节点拥有独立的处理器和本地内存。在 EGM 中，内存的物理位置通过 NUMA 节点标识符（`numaID`）来指定。这个 ID 由操作系统分配，与 CUDA 设备的 ordinal（设备序号）不同，它表示的是与某 GPU 物理上最接近的主机 NUMA 节点。

用户可通过 `cuDeviceGetAttribute` 配合 `CU_DEVICE_ATTRIBUTE_HOST_NUMA_ID` 属性查询某 CUDA 设备对应的 NUMA 节点 ID。这是后续所有 EGM 内存分配的前置信息。

### 2.2 支持的系统拓扑

EGM 目前支持三种平台拓扑：

1. **单节点单 GPU（Single-Node, Single-GPU）**
   - 包含 ARM 架构 CPU、CPU 附加内存和一个 GPU。
   - CPU 与 GPU 之间通过高带宽 C2C（Chip-to-Chip）互连。
   - 对开发者而言，本地 host 内存访问的语义与传统 host allocator 几乎一致，但带宽由 NVLink-C2C 保证。

2. **单节点多 GPU（Single-Node, Multi-GPU）**
   - 包含多个 ARM 架构 CPU（各带本地内存）和多个 GPU。
   - GPU 之间通过 NVLink 网络互联。
   - 用户必须显式指定内存应分配到哪个 NUMA 节点，并管理各 GPU 的访问权限。

3. **多节点多 GPU（Multi-Node, Multi-GPU）**
   - 两个或多个单节点系统通过 NVLink 网络互联。
   - 除了内存分配外，还需要使用 CUDA IPC（Inter-Process Communication）机制进行跨进程句柄交换。

### 2.3 分配器类型与 EGM 的映射粒度

EGM 内存的分配目前支持两种方式：

- **虚拟内存管理（VMM）API**：基于 `cuMemCreate` 的显式物理内存分配和虚拟地址映射。
- **流序内存池（Stream Ordered Memory Pool）**：基于 `cudaMemPoolCreate` 的异步分配器。

无论哪种方式，EGM 内存都使用 2MB 大页映射。这意味着在访问超大内存块时，可能会遇到比传统 4KB 页更多的 TLB miss，需要在性能调优时予以关注。

### 2.4 Fabric Handle 与跨节点共享

在多节点场景下，EGM 内存需要通过 `CU_MEM_HANDLE_TYPE_FABRIC` 类型的 handle 进行导出和导入。`cuMemExportToShareableHandle` 将本地分配句柄转换为可通过网络（如 TCP/IP）传输的 fabric handle；接收方通过 `cuMemImportFromShareableHandle` 还原句柄，并在本地完成地址空间映射和访问授权。

## API / 机制详解

### 4.1 查询 NUMA ID：`cuDeviceGetAttribute`

```cpp
int numaId;
cuDeviceGetAttribute(&numaId, CU_DEVICE_ATTRIBUTE_HOST_NUMA_ID, deviceOrdinal);
```

- **作用**：获取指定 CUDA 设备所关联的最近主机 NUMA 节点的 ID。
- **调用时机**：在任何 EGM 内存分配之前，必须先确定目标 NUMA 节点。
- **返回值**：如果设备不支持 EGM（如传统 x86 + PCIe GPU 平台），此调用可能返回错误码或无效值。
- **与传统接口的区别**：传统 CUDA 编程几乎不需要关心 NUMA 拓扑，`cudaSetDevice` 之后即可工作；EGM 要求开发者显式处理 NUMA 亲和性。

### 4.2 VMM API 方式分配 EGM 内存

#### 步骤 1：配置分配属性（`CUmemAllocationProp`）

```cpp
CUmemAllocationProp prop{};
prop.type = CU_MEM_ALLOCATION_TYPE_PINNED;
prop.location.type = CU_MEM_LOCATION_TYPE_HOST_NUMA;
prop.location.id = numaId;
```

- `CU_MEM_ALLOCATION_TYPE_PINNED` 表示分配 pinned 物理内存。
- `CU_MEM_LOCATION_TYPE_HOST_NUMA` 是关键标志，表明内存应分配到指定 NUMA 节点的 host 内存上。
- `prop.location.id` 必须填之前查询到的有效 `numaId`。

#### 步骤 2：获取分配粒度并创建物理内存（`cuMemGetAllocationGranularity` / `cuMemCreate`）

```cpp
size_t granularity = 0;
cuMemGetAllocationGranularity(&granularity, &prop, CU_MEM_ALLOC_GRANULARITY_MINIMUM);
size_t padded_size = ROUND_UP(size, granularity);
CUmemGenericAllocationHandle allocHandle;
cuMemCreate(&allocHandle, padded_size, &prop, 0);
```

- **为什么要对齐**：EGM 分配必须对齐到平台特定的粒度（通常是 2MB 大页）。
- `cuMemGetAllocationGranularity` 查询最小对齐要求。
- `cuMemCreate` 创建物理内存块，返回一个 `CUmemGenericAllocationHandle`。
- **失败风险**：如果指定的 NUMA ID 无效或当前平台不支持 `CU_MEM_LOCATION_TYPE_HOST_NUMA`，`cuMemCreate` 会返回错误。

#### 步骤 3：保留虚拟地址并映射（`cuMemAddressReserve` / `cuMemMap`）

```cpp
CUdeviceptr dptr;
cuMemAddressReserve(&dptr, padded_size, 0, 0, 0);
cuMemMap(dptr, padded_size, 0, allocHandle, 0);
```

- `cuMemAddressReserve` 在进程虚拟地址空间中预留一段连续区域。
- `cuMemMap` 将物理内存句柄映射到预留的虚拟地址上。
- 这两步在 EGM 中没有平台特定的变化，遵循标准 VMM 流程。

#### 步骤 4：设置访问权限（`cuMemSetAccess`）

```cpp
CUmemAccessDesc accessDesc[2]{{}};
accessDesc[0].location.type = CU_MEM_LOCATION_TYPE_HOST_NUMA;
accessDesc[0].location.id = numaId;
accessDesc[0].flags = CU_MEM_ACCESS_FLAGS_PROT_READWRITE;
accessDesc[1].location.type = CU_MEM_LOCATION_TYPE_DEVICE;
accessDesc[1].location.id = currentDev;
accessDesc[1].flags = CU_MEM_ACCESS_FLAGS_PROT_READWRITE;
cuMemSetAccess(dptr, size, accessDesc, 2);
```

- **关键点**：如果不显式设置访问权限，任何对映射地址的读写都会导致进程崩溃（segfault）。
- 必须分别为 host NUMA 节点和访问该内存的 GPU 设备授予读写权限。
- `CU_MEM_ACCESS_FLAGS_PROT_READWRITE` 授予完全访问；也可根据需求设为只读。

### 4.3 CUDA Memory Pool 方式分配 EGM 内存

#### 创建内存池（`cudaMemPoolCreate`）

```cpp
cudaMemPoolProps props{};
props.allocType = cudaMemAllocationTypePinned;
props.location.type = cudaMemLocationTypeHostNuma;
props.location.id = numaId;
cudaMemPoolCreate(&memPool, &props);
```

- `cudaMemLocationTypeHostNuma` 是运行时 API 中对应 `CU_MEM_LOCATION_TYPE_HOST_NUMA` 的枚举值。
- 内存池创建后，后续从该池分配的内存都位于指定的 NUMA 节点。

#### 设置池访问权限（`cudaMemPoolSetAccess`）

```cpp
cudaMemAccessDesc desc{};
desc.flags = cudaMemAccessFlagsProtReadWrite;
desc.location.type = cudaMemLocationTypeDevice;
desc.location.id = accessingDevice;
cudaMemPoolSetAccess(memPool, &desc, 1);
```

- 与 VMM 类似，需要显式授予 peer GPU 对内存池的访问权。
- `cudaMemPoolSetAccess` 的语义与 `cuMemSetAccess` 对应，但操作对象是整个内存池。

#### 分配内存（`cudaDeviceSetMemPool` + `cudaMallocAsync`）

```cpp
cudaDeviceSetMemPool(residentDevice, memPool);
cudaMallocAsync(&ptr, size, memPool, stream);
```

- `cudaDeviceSetMemPool` 将指定内存池设为某设备的默认内存池。
- `cudaMallocAsync` 从该池异步分配内存，内存物理上位于之前指定的 NUMA 节点。
- 释放时使用 `cudaFreeAsync`。

### 4.4 多节点多 GPU 的 Fabric Handle 机制

多节点场景需要额外的跨进程句柄交换：

#### 分配时指定 Fabric Handle 类型

```cpp
prop.requestedHandleTypes = CU_MEM_HANDLE_TYPE_FABRIC;
```

- 在调用 `cuMemCreate` 时，必须显式请求 `CU_MEM_HANDLE_TYPE_FABRIC`。
- 创建后的句柄才能被导出到其它节点。

#### 导出与导入

```cpp
// 节点 A 导出
cuMemExportToShareableHandle(&fabricHandle, allocHandle, CU_MEM_HANDLE_TYPE_FABRIC, 0);
// 通过网络发送 fabricHandle 到节点 B

// 节点 B 导入
cuMemImportFromShareableHandle(&allocHandle, &fabricHandle, CU_MEM_HANDLE_TYPE_FABRIC);
```

- `cuMemExportToShareableHandle` 将本地句柄序列化为可传输的 fabric handle。
- `cuMemImportFromShareableHandle` 在远端节点还原句柄。
- 导入后，远端节点仍需执行 `cuMemAddressReserve`、`cuMemMap` 和 `cuMemSetAccess`，并为本地 GPU 授予访问权限。

## 典型工作流程 / 调用顺序

### 单节点单 GPU 流程

1. 初始化 CUDA 上下文并设置设备。
2. 使用传统 host allocator（如 `cudaMallocHost`、`cudaHostAlloc`）或系统分配器分配内存。
3. 直接启动 kernel 访问该内存，NVLink-C2C 保证高带宽。
4. 无需额外 EGM API，语义与传统零拷贝内存一致。

### 单节点多 GPU 流程（VMM 方式）

1. 对每个 GPU，调用 `cuDeviceGetAttribute` 获取 `CU_DEVICE_ATTRIBUTE_HOST_NUMA_ID`。
2. 根据目标 NUMA 节点，填充 `CUmemAllocationProp`，指定 `CU_MEM_LOCATION_TYPE_HOST_NUMA` 和 `numaId`。
3. 调用 `cuMemGetAllocationGranularity` 获取对齐粒度，并对请求大小向上取整。
4. 调用 `cuMemCreate` 创建物理内存句柄。
5. 调用 `cuMemAddressReserve` 预留虚拟地址空间。
6. 调用 `cuMemMap` 完成物理到虚拟的映射。
7. 构造 `CUmemAccessDesc` 数组，为 host NUMA 节点和需要使用该内存的 GPU 分别设置 `CU_MEM_ACCESS_FLAGS_PROT_READWRITE`。
8. 调用 `cuMemSetAccess` 激活访问权限。
9. 启动 kernel，通过映射后的设备指针 `dptr` 访问 EGM 内存。
10. 使用完毕后，按 `cuMemUnmap` -> `cuMemAddressFree` -> `cuMemRelease` 的顺序释放资源。

### 单节点多 GPU 流程（Memory Pool 方式）

1. 查询目标 GPU 对应的 NUMA ID。
2. 填充 `cudaMemPoolProps`，指定 `cudaMemLocationTypeHostNuma`。
3. 调用 `cudaMemPoolCreate` 创建内存池。
4. 对需要访问该池的每个 peer GPU，调用 `cudaMemPoolSetAccess` 授予读写权限。
5. 调用 `cudaDeviceSetMemPool` 将该池设为某设备的默认池。
6. 在 stream 上调用 `cudaMallocAsync` 分配内存。
7. kernel 访问指针后，调用 `cudaFreeAsync` 释放。

### 多节点多 GPU 流程

1. 在源节点（Node A）上按 VMM 流程创建 EGM 内存，但额外设置 `CU_MEM_HANDLE_TYPE_FABRIC`。
2. 调用 `cuMemExportToShareableHandle` 获取 fabric handle。
3. 通过 TCP/IP 或其他网络机制将 fabric handle 发送到目标节点（Node B）。
4. Node B 接收后调用 `cuMemImportFromShareableHandle` 导入句柄。
5. Node B 执行本地虚拟地址预留和映射。
6. Node B 为本地 GPU 设置 `cuMemSetAccess`。
7. Node B 的 GPU 即可访问 Node A 上的 EGM 内存。

## 关键限制、边界条件与兼容性

1. **硬件平台限制**
   - EGM 仅支持集成式 CPU-GPU 的 NVIDIA 系统（如 Grace Hopper），需要 NVLink-C2C 硬件支持。
   - 传统 x86 平台 + PCIe GPU（如 GTX 1060、RTX 3090）不支持 EGM，`CU_DEVICE_ATTRIBUTE_HOST_NUMA_ID` 查询会失败，`cuMemCreate` 配合 `CU_MEM_LOCATION_TYPE_HOST_NUMA` 也会返回错误。

2. **cgroups 与设备可见性**
   - 使用 Linux cgroups 限制可用设备会阻断 EGM 的路由，并导致性能问题。
   - 若需限制 GPU 可见性，应使用 `CUDA_VISIBLE_DEVICES` 环境变量，而非 cgroups。

3. **页大小与 TLB**
   - EGM 使用 2MB 大页映射。对于超大分配（几十 GB 级别），相比 4KB 页可能产生更多的 TLB miss，导致随机访问性能下降。
   - 需要参考 CUDA Tuning Guide 关于内存分配器和页大小的建议。

4. **多节点 IPC 依赖**
   - 多节点场景必须使用 CUDA IPC 协议交换 fabric handle。
   - 开发者需自行处理网络传输层（TCP/IP 或 RDMA）的序列化和反序列化。

5. **粒度对齐**
   - 所有 EGM 物理分配必须对齐到 `cuMemGetAllocationGranularity` 返回的最小粒度。
   - 未对齐的请求会失败。

6. **访问权限是显式的**
   - 映射完成后若不调用 `cuMemSetAccess` 或 `cudaMemPoolSetAccess`，任何访问都会触发 segfault。
   - 这与传统 `cudaMalloc` 分配的内存默认可被当前设备访问不同。

7. **NUMA ID 与设备 Ordinal 的区别**
   - `numaID` 是操作系统分配的 NUMA 节点标识，与 CUDA 设备序号无关。
   - 同一 NUMA 节点可能关联多个 GPU，也可能一个 GPU 只关联一个 NUMA 节点。

## 常见陷阱与调试建议

1. **未检查 NUMA ID 有效性**
   - 很多开发者会假设设备 0 对应 NUMA 节点 0。实际上操作系统分配的 NUMA ID 可能是任意整数。
   - 建议：始终调用 `cuDeviceGetAttribute` 查询，并检查返回码。

2. **忽略分配粒度对齐**
   - 直接向 `cuMemCreate` 传入用户请求的大小（如 1024 字节）通常会失败。
   - 建议：使用 `cuMemGetAllocationGranularity` 查询并对齐到最小粒度。

3. **忘记设置访问权限**
   - VMM 映射后忘记 `cuMemSetAccess` 是最常见的崩溃原因。
   - 建议：将映射和设权作为原子步骤封装在辅助函数中，确保永远不会只 map 不 set access。

4. **在 Memory Pool 中混淆 location type**
   - VMM 使用 `CU_MEM_LOCATION_TYPE_HOST_NUMA`，Memory Pool 使用 `cudaMemLocationTypeHostNuma`，两者命名空间不同，但语义相同。
   - 建议：仔细阅读编译错误，避免混用 driver API 和 runtime API 的类型。

5. **cgroups 导致的路由异常**
   - 如果在容器或 systemd slice 中通过 cgroups 限制了 GPU 设备，EGM 的路由可能异常，表现为带宽骤降或分配失败。
   - 建议：容器环境下优先使用 `CUDA_VISIBLE_DEVICES`。

6. **2MB 页的 TLB 压力**
   - 当分配容量极大且访问模式高度随机时，2MB 大页的 TLB 覆盖范围不如 4KB 页密集。
   - 建议：对超大 EGM 分配采用顺序访问或分块（tiling）策略，减少 TLB miss。

7. **多节点 fabric handle 传输安全**
   - Fabric handle 本身不包含访问控制，任何获取该 handle 的进程都可以导入并映射。
   - 建议：在传输层使用 TLS 或 VPN 加密，避免 fabric handle 被中间人截获。

## 一个最小可运行示例的说明

`chapter_demo.cu` 是一个为 GTX 1060（sm_61, CUDA 12.8）设计的兼容示例，其设计目标是在不支持 EGM 的消费级硬件上也能**编译通过、运行不崩溃**，同时完整展示 EGM 的 VMM 和 Memory Pool 两条技术路径。

### 为什么这样设计

EGM 需要 NVLink-C2C、ARM CPU 和 NUMA 拓扑支持，而 sm_61 平台完全不满足这些条件。因此示例不能假设 API 调用一定成功，而是采取"探测式"策略：

1. **运行时检测**：代码首先尝试调用 `cuDeviceGetAttribute` 查询 `CU_DEVICE_ATTRIBUTE_HOST_NUMA_ID`。如果返回错误（如 `CUDA_ERROR_INVALID_VALUE`），说明当前设备不支持 EGM 相关的 NUMA 查询。
2. **优雅降级**：检测到不支持后，程序打印明确的提示信息，说明当前平台缺少哪些硬件/驱动条件，然后正常退出（返回 0），不会触发 segfault 或未定义行为。
3. **API 展示完整**：即使运行时会提前退出，代码中仍然保留了完整的 VMM 和 Memory Pool 调用序列（被条件分支保护），读者可以直接阅读代码了解 EGM 的标准用法。

### 演示了哪些能力

- **Driver API 的 NUMA 查询**：展示如何引入 `cuda.h` 并使用 driver API 获取 NUMA ID。
- **VMM 分配属性配置**：展示 `CUmemAllocationProp` 如何设置 `CU_MEM_LOCATION_TYPE_HOST_NUMA`。
- **Memory Pool 属性配置**：展示 `cudaMemPoolProps` 如何设置 `cudaMemLocationTypeHostNuma`。
- **完整的错误检查链**：每个 CUDA 调用都包裹在 `CUDA_CHECK` 宏中，失败时打印文件名、行号和错误描述。

### 在 sm_61 上的降级处理

- 使用 `cuDeviceGetAttribute` 作为能力探测点。这是 EGM 支持的最基础前提；如果连 NUMA ID 都查不到，后续 `cuMemCreate`/`cudaMemPoolCreate` 必然失败。
- 不使用任何 sm_61 不支持的 PTX 指令或 warp-level 原语。
- kernel 本身是一个简单的向量加法，仅用于验证内存是否可读可写；当 EGM 不支持时，kernel 不会被执行。
- Makefile 使用 `-arch=sm_61` 确保生成的二进制完全兼容 GTX 1060。

如果需要在一台真正的 Grace Hopper 机器上测试此示例，只需注释掉早期的兼容性检查退出逻辑，代码即可完整执行 VMM 分配、映射、设权、kernel 读写和释放的全过程。
