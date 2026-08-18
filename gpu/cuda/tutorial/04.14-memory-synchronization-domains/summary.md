## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/memory-sync-domains.html>


本章 "Memory Synchronization Domains" 出自 NVIDIA CUDA Programming Guide Release 13.2，针对的是从计算能力 9.0（Hopper 架构）和 CUDA 12.0 开始引入的一项新特性。该特性的核心目标是缓解某些 CUDA 应用因内存栅栏（memory fence）或刷写（flush）操作等待过多非必要事务而导致的性能下降问题。通过引入 "同步域（domain）" 的概念，GPU 能够在代码显式协助的前提下，缩小 fence 操作需要捕捞的在途内存操作范围，从而降低并发计算内核与通信内核之间的相互干扰。本章围绕三个层次展开：首先解释问题的根源——内存栅栏干扰（fence interference）与累积性（cumulativity）；其次提出通过域隔离流量的方案；最后详细介绍在 CUDA 中实际使用域的 API、语义变化以及典型使用模式。

需要强调的是，虽然域功能在 Hopper 上才真正发挥硬件隔离作用，但 CUDA 为了保证可移植性，允许在所有设备上调用相关 API。对于计算能力低于 9.0 的设备，CUDA 会报告域数量为 1，并将所有逻辑域映射到物理域 0，从而保持向后兼容。因此，理解这一特性不仅有助于在最新硬件上优化多内核并发场景，也有助于编写能在不同代际 GPU 上平滑迁移的代码。

## 背景与要解决的问题

### 内存栅栏干扰（Memory Fence Interference）

在深入域机制之前，必须先理解它试图解决的具体问题。CUDA 内存一致性模型要求某些同步操作（如原子变量的 release/acquire 语义、显式 fence intrinsic、或任务边界处的隐式同步）必须保证特定顺序关系。然而，GPU 在执行时无法精确区分「哪些写操作在源代码层面已经被保证可见」与「哪些写操作只是因时序巧合而当前恰好可见」。为了安全起见，GPU 的 fence/flush 会保守地等待一大范围的在途（in-flight）内存操作完成，这就导致了「干扰」：fence 等待了它本不需要等待的操作，从而耗时过长。

### 累积性（Cumulativity）
TODO 从这个例子就没有看懂，所以后面的东西就不用看了

本章用一个具体代码示例说明了问题的本质。示例中包含三个线程：Thread 1 在 GPU SM 上执行，Thread 2 在另一个 SM 上执行，Thread 3 在 CPU 上执行。变量 `x` 是 `__managed__` 整型，`a` 是 device-scope 原子变量，`b` 是 system-scope 原子变量。

Thread 1 执行：
```
x = 1;
a = 1;  // release, device-scope
```

Thread 2 执行：
```
while (a != 1);  // acquire, device-scope
assert(x == 1);
b = 1;  // release, system-scope
```

Thread 3 执行：
```
while (b != 1);  // acquire, system-scope
assert(x == 1);
```

CUDA 内存模型保证两个 assert 都不会失败。这意味着 Thread 1 对 x 的写必须对 Thread 3 可见。这里的关键在于：a 的 release/acquire 只保证 device-scope 的顺序，即足以让 x=1 对 Thread 2 可见，但不足以直接保证对 Thread 3（CPU）可见。真正让 x=1 传播到 CPU 的是 b 的 system-scope release/acquire。因此，b 的 acquire 不仅要保证 Thread 2 自己之前的写对 Thread 3 可见，还必须保证「Thread 2 已经看到的其他线程的写」也对 Thread 3 可见。这种「间接传递可见性」的特性就称为累积性（cumulativity）。

由于累积性的存在，当 GPU 执行 system-scope fence 时，它不仅要刷写当前线程/内核的内存操作，还要把「当前内核可能已经看到的所有其他线程的写」也纳入等待范围。在不知道源代码层面精确依赖关系的情况下，GPU 只能撒一张大网，捕获所有在途操作。这在大规模并发场景下尤其痛苦。

### 典型干扰场景

本章举了一个非常常见的实际案例：一个内核在本地 GPU 内存中进行计算，另一个并行的内核（例如来自 NCCL 的通信内核）通过 NVLink 或 PCIe 与对端设备进行通信。当本地计算内核完成时，它会隐式刷写自己的写操作，以满足与下游任务的 synchronizes-with 关系。但由于 fence 的保守性，这个刷写可能会不必要地等待（完全或部分等待）来自通信内核的较慢的 NVLink/PCIe 写操作完成。换句话说，两个在逻辑上几乎无关的内核，因为共享了同一张 fence 大网，而产生了性能耦合。这种耦合在通信延迟较高或通信量较大时会显著拖尾计算内核的完成时间。

## 核心概念与术语

### 同步域（Memory Synchronization Domain）
TODO 这里也没有理解为什么可以通过 domain 来解决

同步域是 Hopper 架构引入的一种硬件级别的内存操作分区机制。每个内核启动（kernel launch）可以被赋予一个域 ID（domain ID）。GPU 在跟踪在途写操作和 fence 时，会根据域 ID 打标签。当某个内核执行 fence 时，该 fence 只会等待与其自身域 ID 相匹配的写操作，而不会无差别地等待所有在途操作。

### 逻辑域与物理域（Logical vs. Physical Domains）

CUDA 没有直接暴露物理域编号给内核启动接口，而是引入了两层抽象：

- **逻辑域（Logical Domain）**：当前定义了两个枚举值，`cudaLaunchMemSyncDomainDefault`（默认域）和 `cudaLaunchMemSyncDomainRemote`（远程域）。远程域的预期语义是「该内核主要执行远程内存访问」，以便将其流量与本地计算内核隔离。
- **物理域（Physical Domain）**：真正的硬件域编号。设备属性 `cudaDevAttrMemSyncDomainCount` 报告物理域的总数。Hopper 设备有 4 个物理域。逻辑域通过域映射表（domain map）转换到物理域。

这种两层设计的好处是应用组合性：底层库（如 NCCL）可以直接选择逻辑域 Remote，无需关心上层应用是否也在使用域；而上层应用可以通过映射表灵活地决定逻辑域对应哪个物理域，避免冲突。

### 域映射表（Domain Map）

`cudaLaunchMemSyncDomainMap` 结构体定义了逻辑域到物理域的映射关系。它包含两个字段：`default_` 和 `remote`，分别表示默认逻辑域和远程逻辑域对应的物理域编号。默认情况下，若用户不修改映射，则 `default_` 映射到 0，`remote` 映射到 1（前提是设备支持多于 1 个域）。应用程序也可以根据流或任务类型自定义映射，例如将流 A 的两个逻辑域都映射到物理域 0，流 B 映射到物理域 1，以此实现更细粒度的隔离。

### 累积性在域模型下的变化

使用域之后，`thread_scope_device` 的定义被微妙地修改了：device-scope fence 只保证同一物理域内的顺序和可见性。如果两个内核位于不同物理域，那么它们之间的同步或顺序保证需要显式使用 system-scope fence。换句话说，跨域的累积性不再由单一 device-scope 操作隐式涵盖，而必须通过 system-scope 刷写来提前完成。这是使用域功能时最容易被忽视却又最关键的语义变化。

## API / 机制详解

### cudaDevAttrMemSyncDomainCount（设备属性查询）

**作用**：查询当前 GPU 支持的物理内存同步域数量。

**何时调用**：在应用初始化阶段，或在使用任何域相关功能之前调用，以判断设备是否真正支持域隔离。

**调用方式**：
```cpp
int domainCount = 0;
cudaDeviceGetAttribute(&domainCount, cudaDevAttrMemSyncDomainCount, device);
```

**调用后得到什么**：Hopper（sm_90+）设备返回 4；更早的设备返回 1。返回值 1 意味着虽然 API 层面可以设置逻辑域，但硬件层面只有一个物理域，所有流量都会落到同一个域里，不存在真正的隔离。

**常见错误**：将返回值大于 0 等同于「支持隔离」。实际上只有返回值大于 1 时，才具备隔离不同流量的硬件能力。返回值 1 时，逻辑域设置可以被 API 接受，但不会产生任何实际的性能收益。

### cudaLaunchAttributeMemSyncDomain（内核启动属性）

**作用**：为一次具体的内核启动指定逻辑域。有效取值为 `cudaLaunchMemSyncDomainDefault` 或 `cudaLaunchMemSyncDomainRemote`。

**何时调用**：在调用 `cudaLaunchKernelEx` 之前，填充 `cudaLaunchAttribute` 结构体。

**关键参数**：
```cpp
cudaLaunchAttribute domainAttr;
domainAttr.id = cudaLaunchAttributeMemSyncDomain;
domainAttr.val.memSyncDomain = cudaLaunchMemSyncDomainRemote;
```

**前置条件**：CUDA 版本需 12.0 或更高。该属性仅对使用 `cudaLaunchKernelEx` 启动的内核有效，不兼容传统的 `<<<...>>>` 语法或旧的 `cudaLaunchKernel`。

**调用后得到什么**：内核启动时，GPU 会将该内核产生的写操作标记为对应逻辑域（再经映射表转为物理域）。后续同域的 device-scope fence 只需等待这些带标签的写操作。

**常见错误**：
1. 试图通过 `<<<...>>>` 语法设置域属性——这是不可能的，必须使用 `cudaLaunchKernelEx`。
2. 在计算能力低于 9.0 的设备上期望获得性能提升——API 不会报错，但硬件不执行隔离。
3. 混淆逻辑域和物理域，直接给 `memSyncDomain` 赋物理域编号——这会编译失败或导致未定义行为，因为 `cudaLaunchMemSyncDomain` 枚举目前只定义了 Default 和 Remote 两个逻辑值。

### cudaLaunchAttributeMemSyncDomainMap（域映射属性）

**作用**：提供逻辑域到物理域的映射，使得上层应用可以控制 Default 和 Remote 分别落到哪个物理域上。

**何时调用**：同样在内核启动前，通过 `cudaLaunchKernelEx` 的配置结构传递。通常在创建流或初始化上下文时设置一次，后续多次启动复用。

**关键参数**：
```cpp
cudaLaunchAttribute mapAttr;
mapAttr.id = cudaLaunchAttributeMemSyncDomainMap;
mapAttr.val.memSyncDomainMap.default_ = 0;
mapAttr.val.memSyncDomainMap.remote  = 1;
```

**与传统接口的区别**：在域功能出现之前，CUDA 没有任何机制能让用户把内存同步流量按内核类型分类；所有 fence 都是全局的。DomainMap 的引入相当于在「全局 fence」和「按内核分别 fence」之间增加了一层可配置的中间层。

**失败风险**：如果映射表将两个逻辑域映射到同一个物理域，则这两个逻辑域内的内核不会获得隔离收益，但语义上仍然是正确的（只是退化为传统行为）。如果映射到的物理域编号超出设备支持的 domainCount，则可能导致启动失败或静默回退行为，文档中未明确说明，因此建议始终确保映射值小于 domainCount。

### cudaLaunchKernelEx（扩展启动接口）

**作用**：这是传递启动属性的通用接口。传统的 `cudaLaunchKernel` 不支持 `cudaLaunchConfig_t` 中的属性数组。

**调用方式**：
```cpp
cudaLaunchConfig_t config = {};
config.gridDim = dim3(...);
config.blockDim = dim3(...);
config.attrs = &domainAttr;
config.numAttrs = 1;
cudaLaunchKernelEx(&config, myKernel, arg1, arg2, ...);
```

**前置条件**：CUDA 12.0+，并且内核函数签名需与调用一致。

**常见错误**：忘记设置 `config.numAttrs`，或 `config.attrs` 指针指向的数组生命周期在 `cudaLaunchKernelEx` 执行前已结束。由于 `config` 是按值传递的指针，但 `attrs` 指向的是调用者栈上的数据，如果 `domainAttr` 是局部变量，需确保它在 `cudaLaunchKernelEx` 调用期间仍然有效（通常在同一作用域内即可）。

## 典型工作流程 / 调用顺序

一个完整的、使用内存同步域的工作流程通常包含以下步骤：

**步骤 1：查询设备能力**
在程序初始化时，调用 `cudaDeviceGetAttribute` 查询 `cudaDevAttrMemSyncDomainCount`。若返回值大于 1，则可以利用域隔离；若等于 1，则所有后续域设置仅为兼容占位，不会产生性能差异。

**步骤 2：设计逻辑域分工**
根据应用架构，决定哪些内核放入 `Default` 域，哪些放入 `Remote` 域。典型分工是：本地计算内核使用 `Default`，涉及 NVLink/PCIe 通信的内核（如 NCCL 集合通信）使用 `Remote`。如果应用内部有多种独立的工作流（例如 NVSHMEM 应用中没有明显的本地/远程二分法），也可以按流分区：流 A 的两个逻辑域都映射到物理域 0，流 B 映射到物理域 1，以此类推。

**步骤 3：配置域映射表（可选）**
如果默认映射（Default->0, Remote->1）已经满足需求，这一步可以跳过。否则，创建 `cudaLaunchMemSyncDomainMap` 结构体，设置 `default_` 和 `remote` 字段，构造 `cudaLaunchAttribute`，id 设为 `cudaLaunchAttributeMemSyncDomainMap`。

**步骤 4：为每次内核启动设置域属性**
构造 `cudaLaunchAttribute`，id 设为 `cudaLaunchAttributeMemSyncDomain`，val 设为 `cudaLaunchMemSyncDomainDefault` 或 `Remote`。将其放入 `cudaLaunchConfig_t` 的 `attrs` 数组中，调用 `cudaLaunchKernelEx` 启动内核。

**步骤 5：处理跨域同步**
如果两个不同域的内核之间存在数据依赖或同步需求，必须使用 system-scope 的 fence 或原子操作（例如 `cuda::memory_order_release/acquire` 配合 `cuda::thread_scope_system`）。device-scope 的同步仅在同一物理域内有效。若忽视这一点，可能导致数据竞争或未定义行为。

**步骤 6：验证与调试**
通过 Nsight Systems 或 Nsight Compute 观察内核执行时间，比较开启域映射前后的 fence 等待时间。在 Hopper 设备上，正确配置后应能看到通信内核的慢速写操作不再阻塞计算内核的隐式 flush。

## 关键限制、边界条件与兼容性

### 硬件限制

内存同步域是真正的硬件特性，仅计算能力 9.0（Hopper）及以上 GPU 拥有多个物理域。Hopper 设备有 4 个物理域。更早的设备（Ampere、Turing、Pascal 等）均只有 1 个物理域。

### 软件版本限制

需要 CUDA 12.0 或更高版本才能使用域相关 API。`cudaLaunchKernelEx` 和相应的 launch attribute 枚举都在 CUDA 12.0 中引入。NCCL 2.16 及更高版本会自动为通信内核打上 `Remote` 逻辑域标签。

### 向后兼容性

为了便于移植，CUDA 在所有设备上都允许调用域 API。对于不支持多域的设备，`cudaDevAttrMemSyncDomainCount` 返回 1。默认映射将 Default 和 Remote 都指向物理域 0。这意味着旧代码完全不修改即可在新版本 CUDA 上编译运行，且行为不变；反过来，为 Hopper 编写的域相关代码也能在老 GPU 上运行，只是不会获得性能收益。

### thread_scope_device 的语义变化

启用域之后，`thread_scope_device` 的可见范围被限制在「同一物理域」内，而不再是「整个 GPU」。这是一个不向前兼容的语义变化，但由于内核默认属于域 0，现有代码只要不显式设置域，其语义就与之前完全一致。只有在显式将不同内核放入不同物理域时，才需要额外关注这一变化。

### 跨域同步必须使用 system-scope

这是使用域时最重要的规则。如果在物理域 A 的内核写入了一块内存，然后在物理域 B 的内核中通过 device-scope 原子或 fence 读取该内存，这是不安全的。必须至少使用一次 system-scope 操作将数据从域 A 刷出到系统级别，再由域 B 读取。本质上，累积性被「推」到了 system-scope 边界。

### 域不影响内存访问合法性

选择一个域（如 Remote）并不会限制该内核只能访问远程内存，也不会赋予它新的访问权限。域只影响 fence/flush 的等待范围，不影响地址翻译、P2P 能力或统一内存迁移策略。

## 常见陷阱与调试建议

### 陷阱 1：误认为设置逻辑域就自动获得隔离

仅设置 `cudaLaunchAttributeMemSyncDomain` 为 Remote 并不足以在硬件上隔离流量。还必须满足两个条件：设备支持多于 1 个物理域（domainCount > 1），且域映射表将 Remote 映射到了一个与 Default 不同的物理域。如果映射表将两者都映射到 0，则所有内核仍在同一个物理域内竞争。

### 陷阱 2：跨域使用 device-scope 同步

如前所述，不同物理域之间的同步不能依赖 device-scope 操作。一个常见错误是：开发者习惯性地在核间使用 `__threadfence()` 或 `cuda::atomic`（默认 device-scope）进行同步，在引入域之后没有升级为 system-scope，从而导致偶发性数据不一致。这种 bug 很难复现，因为它取决于两个域的内核是否恰好同时在执行。

### 陷阱 3：忽略隐式 fence

内核完成时的隐式 flush（例如流同步、事件记录、下一个内核启动时的依赖解析）也会受域影响。如果计算流和通信流共享同一个 CUDA 流，那么即使把它们放在不同域，流内部的顺序保证仍可能导致隐式同步。最佳实践是让计算内核和通信内核运行在不同的流上，各自绑定不同的域映射。

### 陷阱 4：混淆物理域编号与逻辑域枚举

`cudaLaunchMemSyncDomain` 枚举只有两个值：Default 和 Remote。不要试图将物理域编号（如 2 或 3）直接赋给 `memSyncDomain` 字段。如果需要将内核分配到物理域 2，应通过 DomainMap 把某个逻辑域映射到 2，然后在启动时选择对应的逻辑域。

### 调试建议

1. **始终先查询 domainCount**：在任何使用域的代码路径前加入断言或日志，记录 `cudaDevAttrMemSyncDomainCount`。这可以帮助快速判断性能问题是否源于「设备不支持域」。
2. **使用 Nsight Systems 时间线**：观察不同内核的启动和结束时间线。如果通信内核的 NVLink 事务在计算内核结束后仍然阻塞了下游任务，说明域隔离没有生效。
3. **逐步验证映射表**：先使用默认映射，确认功能正确；再尝试自定义映射，观察性能变化。不要同时修改逻辑域选择和映射表，否则出现问题时难以定位。
4. **检查 NCCL 版本**：如果使用 NCCL，确认其版本是否 >= 2.16。较早的 NCCL 不会自动标记 Remote 域，需要手动包装 NCCL 内核启动（如果 NCCL 暴露底层启动接口的话）。

## 一个最小可运行示例的说明

本目录下的 `chapter_demo.cu` 是一个面向 sm_61（Pascal, GTX 1060）编译运行的最小完整示例。由于 Memory Synchronization Domains 要求 sm_90+，该示例无法在硬件上真正演示域隔离效果，但它完成了以下设计目标：

**1. 运行时检测与降级说明**
示例首先查询 `cudaDevAttrMemSyncDomainCount` 和设备的计算能力。在 GTX 1060（sm_61）上，domainCount 返回 1。程序会明确打印：该 GPU 不支持 Memory Synchronization Domains，CUDA 返回 domainCount=1 仅为向后兼容。这满足了「若涉及 sm_61 不支持的特性，运行时检测并打印说明」的要求。

**2. 演示 API 的正确调用方式**
示例展示了如何构造 `cudaLaunchAttribute`、设置 `cudaLaunchAttributeMemSyncDomain`、以及使用 `cudaLaunchKernelEx` 启动内核。在 sm_61 设备上，代码选择 `cudaLaunchMemSyncDomainDefault`，并打印提示：如果强行指定 Remote，在旧 GPU 上会被驱动忽略或回退。这让读者看到 API 的「调用前需要什么」和「调用后得到什么」——API 层面成功，但硬件层面无隔离。

**3. 演示域映射表的填充**
示例中的 Demo 4 展示了如何填充 `cudaLaunchMemSyncDomainMap`。在 sm_61 上，由于只有一个物理域，代码将 Default 和 Remote 都映射到 0，并打印说明。如果迁移到 Hopper，只需修改条件分支，将 Remote 映射到 1 或其他物理域即可。

**4. 用简单内核演示累积性概念**
Demo 3 使用两个顺序执行的简单内核模拟「生产者-消费者」模式。虽然这不是严格意义上的 acquire/release 示例，但它说明了：在 sm_61 上，内核结束时的隐式 device-scope flush 会等待所有先前的在途操作，包括如果存在的 PCIe/NVLink 通信。这对应了原文中「本地计算内核隐式 flush 可能等待通信内核慢速写」的场景。

**5. 兼容处理总结**
- 编译目标：`-arch=sm_61`，确保可在 GTX 1060 上运行。
- 所有域相关 API 都在 CUDA 12.8 中可用，因此编译通过。
- 运行时通过 `prop.major >= 9` 判断是否为 Hopper+，从而选择不同的逻辑域和映射值。
- 若未来在 Hopper 上运行同一二进制（需重新编译为 sm_90），代码会自动走 `domainCount > 1` 分支，启用 Remote 域和 nonzero 映射。

**6. 错误处理**
代码全程使用 `CUDA_CHECK` 宏包装 CUDA API 调用。一旦遇到错误，会打印文件名、行号和错误字符串，并立即 `exit(EXIT_FAILURE)`。这符合生产级 CUDA 代码的最低健壮性要求。

通过这个示例，读者可以在没有 Hopper 硬件的情况下，完整走通域功能的 API 调用流程，理解每个结构体字段的含义，同时清楚地看到旧设备上的行为边界。当获得 Hopper 硬件后，同一套代码逻辑只需调整运行时分支判断，即可真正发挥域隔离的性能优势。
