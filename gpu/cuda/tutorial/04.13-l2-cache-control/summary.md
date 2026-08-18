## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/l2-cache-control.html>


CUDA Programming Guide 的 "L2 Cache Control"（L2 缓存控制）章节系统性地介绍了从 Compute Capability 8.0（Ampere 架构）开始引入的 L2 缓存持久化机制。该机制允许开发者通过 CUDA Runtime API 或 libcu++ 的 `cuda::annotated_ptr` API，对全局内存的访问模式进行标注，从而影响数据在 L2 缓存中的留存策略。核心目标是：当 kernel 反复访问某一块全局内存区域时，通过硬件和软件协作，让这部分数据更有可能驻留在 L2 缓存中，从而获得更高的有效带宽和更低的访问延迟；而对于只访问一次的数据，则标记为 streaming，避免不必要地占用宝贵的 L2 缓存资源。

本章内容覆盖了 L2 缓存预留（set-aside）、访问策略窗口（access policy window）、命中比例（hitRatio）的语义、三种访问属性（streaming / persisting / normal）的区别、通过 CUDA Stream 和 CUDA Graph Kernel Node 两种粒度设置策略的方法、L2 重置的三种方式、以及多流并发时的资源竞争与管理策略。这些知识点共同构成了一套完整的、从配置到使用再到清理的 L2 缓存持久化工作流。

## 背景与要解决的问题

在现代 GPU 中，L2 缓存位于全局内存（DRAM）和 SM 的 L1 缓存 / 共享内存之间，承担着缓冲热点数据、降低访存延迟的关键职责。然而，传统的 L2 缓存管理对上层程序是透明的：硬件根据替换算法（通常是类 LRU 策略）自主决定哪些 cache line 保留、哪些被逐出。这种透明性在大多数情况下工作良好，但对于某些特定的 CUDA 工作负载，它会带来明显的效率损失。

第一类典型场景是"反复访问同一数据区域"。例如，一个 kernel 在循环中多次读取同一张权重表、同一个查找表或同一小块输入特征。如果这些数据在第一次加载后因为 L2 容量压力或其他访存的竞争而被逐出，后续每次访问都需要重新从全局内存加载，造成大量冗余的 DRAM 流量。

第二类典型场景是"单次访问的大数据流"。例如，一个 kernel 顺序扫描一个巨大的数组，每个元素只读一次。这些数据如果占据了 L2 缓存，会把其他更有复用价值的数据挤出去，导致缓存污染（cache pollution）。

L2 Cache Control 机制正是为了解决这两类问题而设计的。它引入了软件可控制的提示（hint）机制，让开发者可以告诉硬件：
- 哪些地址范围的数据应该被优先保留在 L2 中（persisting）；
- 哪些数据应该被优先逐出（streaming）；
- 如何为 persisting 访问专门预留一部分 L2 缓存空间，使其不受 normal 或 streaming 访问的干扰。

需要注意的是，这些控制属于"提示"而非"强制保证"。硬件在具体实现上仍然可能根据实际压力进行替换，但 persisting 数据获得的是"优先保留"的待遇，streaming 数据获得的是"优先逐出"的待遇，从而在统计意义上优化整体访存性能。

## 核心概念与术语

在深入 API 之前，必须准确理解本章涉及的若干核心概念，它们之间的区别和联系直接决定了能否正确使用该功能。

### Persisting Access（持久化访问）与 Streaming Access（流式访问）

这是两种对立的访问语义：
- **Persisting Access**：指 kernel 对全局内存某区域的访问是反复进行的，因此希望该数据尽可能长时间地保留在 L2 缓存中。被标记为 persisting 的内存访问会优先使用专门为 persisting 预留的 L2 缓存空间，并在空间不足时优先保留这些 cache line。
- **Streaming Access**：指数据只被访问一次或很少几次，因此不期望它在 L2 中长期驻留。被标记为 streaming 的访问在需要逐出 cache line 时会被优先淘汰，从而减少对 persisting 数据的干扰，也减少把一次性数据保留在缓存中的浪费。

### Access Policy Window（访问策略窗口）

这是 L2 缓存控制的基本配置单元。一个访问策略窗口定义了一段连续的全局内存区域，以及对该区域内访问的 L2 缓存策略属性。窗口由以下字段描述：
- `base_ptr`：全局内存起始地址；
- `num_bytes`：窗口大小（字节数），必须小于等于设备属性中的 `accessPolicyMaxWindowSize`；
- `hitRatio`：命中比例提示，范围 0.0 ~ 1.0；
- `hitProp`：当访问被归类为"命中"时应用的访问属性；
- `missProp`：当访问被归类为"未命中"时应用的访问属性。

访问策略窗口可以绑定到 CUDA Stream，也可以绑定到 CUDA Graph 的 Kernel Node。绑定到 Stream 意味着后续在该 Stream 中执行的 kernel 都会继承这一策略；绑定到 Graph Kernel Node 则只影响该节点。

### hitRatio 的随机语义

`hitRatio` 是本章最容易被误解的参数之一。它并不是精确指定"前 X% 的访问"或"某个特定子区域"使用 `hitProp`，而是一个概率提示。硬件会以近似 `hitRatio` 的概率，随机地将窗口内的内存访问标记为 persisting（即应用 `hitProp`），其余访问则应用 `missProp`。具体哪条访问被归类为 persisting 是由硬件根据概率分布和内存地址分布决定的，软件无法精确控制。

例如，若 L2 预留空间为 16KB，窗口大小为 32KB：
- 当 `hitRatio = 1.0` 时，硬件会尝试缓存整个 32KB 窗口，但由于预留空间只有 16KB，实际采用的是类似 LRU 的策略在 32KB 中保留最近使用的 16KB。
- 当 `hitRatio = 0.5` 时，硬件会随机挑选大约 16KB 的数据作为 persisting，恰好填满预留空间。

这一设计的目的在于：当多个并发 Stream 的窗口总大小超过 L2 预留容量时，通过设置 `hitRatio < 1.0`，可以降低各窗口之间的 cache line 相互驱逐（thrashing）的概率。

### L2 Cache Set-Aside（L2 缓存预留空间）

这是 persisting 访问的物理资源池。开发者可以通过 API 将 L2 缓存的一部分容量专门划分出来，供 persisting 访问优先使用。Normal 或 streaming 访问只有在该预留空间未被 persisting 访问占满时，才能使用这部分空间。一旦预留空间被 persisting 数据占满，normal/streaming 访问就只能使用 L2 的其余部分。

预留空间的大小可以通过 `cudaDeviceSetLimit` 调整，上限为 `cudaDeviceProp::persistingL2CacheMaxSize`。推荐的做法是设置为 L2 总容量的 75% 或最大允许值中的较小者，以在 persisting 需求和 normal 需求之间取得平衡。

### 三种 Access Property（访问属性）

CUDA 定义了三种具体的访问属性：
1. **`cudaAccessPropertyStreaming`**：流式属性。带有该属性的访问在 L2 中更不容易持久化，优先被逐出。适用于一次性扫描的大数据。
2. **`cudaAccessPropertyPersisting`**：持久化属性。带有该属性的访问更有可能保留在 L2 预留空间中，优先被保留。适用于反复访问的热点数据。
3. **`cudaAccessPropertyNormal`**：正常属性。用于显式清除之前设置的 persisting 属性，将访问恢复为默认状态。它的重要性在于：之前 kernel 留下的 persisting cache line 可能在 L2 中保留很长时间，如果不及时清理，会持续占用预留空间，影响后续不需要持久化特性的 kernel。将窗口的访问属性设为 normal，就可以取消先前访问的"优先保留"状态。

## API / 机制详解

本章涉及的 API 可以分为四大类：容量配置 API、Stream 级策略 API、Graph Node 级策略 API、以及重置/清理 API。下面逐一展开。

### 1. cudaDeviceSetLimit —— 配置 L2 预留空间大小

**作用**：设置当前设备上可用于 persisting 访问的 L2 缓存预留容量。

**调用时机**：通常在程序初始化阶段、启动任何使用 persisting 特性的 kernel 之前调用一次。也可以在运行过程中动态调整，但频繁调整可能带来额外的缓存刷新开销。

**关键参数**：
- `limit`：必须是 `cudaLimitPersistingL2CacheSize`。
- `value`：以字节为单位的预留大小，不能超过 `cudaDeviceProp::persistingL2CacheMaxSize`。

**前置条件**：设备 Compute Capability >= 8.0。在 MIG 模式下该功能被禁用，调用可能返回错误或无效。在 MPS 环境下，该 API 无法动态修改预留大小，必须在 MPS 服务器启动时通过环境变量 `CUDA_DEVICE_DEFAULT_PERSISTING_L2_CACHE_PERCENTAGE_LIMIT` 指定。

**调用后得到什么**：如果成功，后续 persisting 访问将可以使用最多 `value` 字节的 L2 缓存空间。

**常见错误**：
- 在 MIG 模式下调用，功能不可用；
- 在 MPS 运行时调用试图修改大小，可能被忽略或报错；
- 设置的值超过 `persistingL2CacheMaxSize`，可能导致错误；
- 未检查设备属性就盲目设置，在不支持的设备上浪费调试时间。

### 2. cudaStreamSetAttribute —— Stream 级访问策略窗口

**作用**：将一个 `accessPolicyWindow` 绑定到指定的 CUDA Stream。后续在该 Stream 中提交的 kernel，其对窗口内全局内存的访问将按照窗口定义的策略属性处理。

**调用时机**：在创建 Stream 后、提交 kernel 前设置。通常在初始化流程中完成。

**关键参数**：
- `stream`：目标 CUDA Stream；
- `attr`：必须是 `cudaStreamAttributeAccessPolicyWindow`；
- `value_ptr`：指向 `cudaStreamAttrValue` 结构体的指针，其中内嵌 `accessPolicyWindow`。

`accessPolicyWindow` 的字段说明：
- `base_ptr`：必须是设备可访问的全局内存指针（通过 `cudaMalloc` 等分配）；
- `num_bytes`：窗口字节数，必须 <= `accessPolicyMaxWindowSize`；
- `hitRatio`：概率提示，0.0 ~ 1.0；
- `hitProp`：命中时的属性，通常为 `cudaAccessPropertyPersisting`；
- `missProp`：未命中时的属性，通常为 `cudaAccessPropertyStreaming`。

**常见错误**：
- `num_bytes` 超过 `accessPolicyMaxWindowSize`；
- `base_ptr` 不是有效的全局内存地址（例如使用了 host pinned memory 的 host 侧指针）；
- 设置后误以为"整个窗口内的所有访问都必然是 persisting"，忽略了 `hitRatio` 的随机语义；
- 忘记在 kernel 执行完毕后清理窗口，导致后续无关 kernel 也受到影响。

### 3. cudaGraphKernelNodeSetAttribute —— Graph Kernel Node 级策略窗口

**作用**：与 Stream 级 API 类似，但粒度更细：只作用于 CUDA Graph 中的某个特定 Kernel Node，而不是整个 Stream 上的所有 kernel。

**调用时机**：在构建 CUDA Graph 时，对特定节点设置属性。

**关键参数**：
- `node`：目标 `cudaGraphNode_t`；
- `attr`：必须是 `cudaKernelNodeAttributeAccessPolicyWindow`；
- `value_ptr`：指向 `cudaKernelNodeAttrValue` 结构体的指针。

其内部 `accessPolicyWindow` 的语义与 Stream 版本完全一致。区别在于作用域：Stream 版本影响 Stream 上随后执行的所有 kernel；Graph Node 版本只影响 Graph 执行时的该节点。

**与传统 CUDA 接口的区别**：这是 CUDA Graph 编程模型下的专用接口。如果使用传统 Stream-based 提交，不需要也不应该使用此 API。

### 4. cudaCtxResetPersistingL2Cache —— 全局强制重置

**作用**：立即将所有当前驻留在 L2 缓存中的 persisting cache line 重置为 normal 状态。这是一个全局操作，影响当前 CUDA Context 下所有 persisting 数据，不区分 Stream 或 kernel。

**调用时机**：在完成了一批依赖 L2 持久化的 kernel 之后，希望彻底释放预留空间、让后续 kernel 能以 normal 优先级使用全部 L2 缓存时调用。

**前置条件**：无特殊前置条件，但应在确保所有相关 kernel 已完成后再调用，否则可能影响正在执行的 persisting 访问的性能。

**常见错误**：
- 过度依赖此 API 而不使用更细粒度的窗口清理；
- 在 kernel 仍在执行时调用，可能导致 persisting 效果被提前中断；
- 忘记调用，导致 persisting 数据长期占用 L2，影响后续工作负载。

### 5. 通过窗口自清理（设置 num_bytes = 0）

除了调用 `cudaCtxResetPersistingL2Cache`，还可以通过将 Stream 或 Graph Node 的 `accessPolicyWindow.num_bytes` 设为 0 并重新设置属性，来禁用该窗口。这种方式的作用域更精确，只影响特定的 Stream 或 Node，而不会波及整个 Context 中其他可能仍在利用 persisting 特性的 Stream。

## 典型工作流程 / 调用顺序

本章给出的完整工作流可以归纳为以下六个步骤，这是一个从配置到清理的闭环：

**步骤一：查询设备能力**
首先通过 `cudaGetDeviceProperties` 查询 `l2CacheSize`、`persistingL2CacheMaxSize` 和 `accessPolicyMaxWindowSize`。这一步是强制性的，因为不同 GPU 的 L2 容量和最大窗口大小差异很大，且 Compute Capability < 8.0 的设备根本不支持该特性。

**步骤二：设置 L2 预留空间**
根据查询结果计算合适的预留大小，通常取 `min(l2CacheSize * 0.75, persistingL2CacheMaxSize)`，然后调用 `cudaDeviceSetLimit(cudaLimitPersistingL2CacheSize, size)` 完成配置。

**步骤三：创建 Stream 并配置访问策略窗口**
创建 CUDA Stream，填充 `cudaStreamAttrValue` 结构体，指定 `base_ptr`、`num_bytes`、`hitRatio`（如 0.6）、`hitProp = cudaAccessPropertyPersisting`、`missProp = cudaAccessPropertyStreaming`，然后调用 `cudaStreamSetAttribute` 绑定到 Stream。

**步骤四：在 Stream 中反复提交 kernel**
启动需要重复访问热点数据的 kernel。由于这些 kernel 都在同一个已配置策略的 Stream 中执行，它们对窗口内数据的访问更有可能命中 L2 预留空间。同一个 Stream 中不同的 kernel 都可以共享这一持久化收益。

**步骤五：禁用访问策略窗口**
将 `accessPolicyWindow.num_bytes` 设为 0，再次调用 `cudaStreamSetAttribute`。这相当于告诉硬件：该 Stream 不再需要这个 persisting 窗口。

**步骤六：全局重置 L2 缓存**
调用 `cudaCtxResetPersistingL2Cache()`，强制清除所有残留的 persisting cache line，确保后续不使用持久化特性的 kernel 能够平等地使用全部 L2 缓存。

对于 CUDA Graph 场景，步骤三替换为对 `cudaGraphKernelNodeSetAttribute` 的调用，其余逻辑类似。

## 关键限制、边界条件与兼容性

### Compute Capability 限制
L2 缓存持久化功能仅限 Compute Capability 8.0 及以上的设备（Ampere、Ada Lovelace、Hopper 等）。在 sm_70 或更早的设备上，相关的设备属性字段（如 `persistingL2CacheMaxSize`）可能为零，调用 `cudaDeviceSetLimit` 也可能返回错误。

### MIG 模式禁用
当 GPU 配置为 Multi-Instance GPU（MIG）模式时，L2 缓存预留功能被完全禁用。任何试图设置 `cudaLimitPersistingL2CacheSize` 的操作都不会生效。

### MPS 环境限制
在 Multi-Process Service（MPS）环境下，`cudaDeviceSetLimit` 无法动态修改 L2 预留大小。预留大小只能在 MPS 服务器启动时通过环境变量 `CUDA_DEVICE_DEFAULT_PERSISTING_L2_CACHE_PERCENTAGE_LIMIT` 设定。这意味着在 MPS 下运行的程序必须在启动前就与系统管理员协调好预留比例，运行时无法根据 workload 自适应调整。

### 窗口大小上限
`num_bytes` 不能超过 `cudaDeviceProp::accessPolicyMaxWindowSize`。超过该值设置属性将失败。在实际编程中，应使用 `min(accessPolicyMaxWindowSize, actual_data_size)` 来确定窗口大小。

### 并发 kernel 的资源竞争
多个在不同 Stream 中并发执行的 kernel 可能各自拥有独立的访问策略窗口，但它们共享同一块 L2 预留空间。总的 persisting 数据量若超过预留容量，就会引发 cache thrashing，反而降低性能。此时应通过调整各窗口的 `hitRatio` 来协调，使它们预期占用的 persisting 空间之和不超过预留容量。

### hitRatio = 1.0 的陷阱
当窗口大小超过预留空间且 `hitRatio = 1.0` 时，硬件会尝试缓存整个窗口，但由于空间不足，只能保留最近使用的部分数据。这会导致频繁的 cache line 换入换出（thrashing）。因此，当多个并发窗口竞争空间时，将 `hitRatio` 从 1.0 降低（如两个窗口都设为 0.5）可以有效减少相互驱逐。

### 自动重置的不确定性
文档明确警告：不要依赖 persisting cache line 的"自动重置"。虽然未被继续访问的 persisting 数据最终会被硬件恢复为 normal 状态，但这一过程的时间是不确定的，可能长达数毫秒甚至更久。在此期间，预留空间被无用数据占据，影响后续 kernel 性能。因此，显式清理（normal 属性或 `cudaCtxResetPersistingL2Cache`）是必需的工程实践。

## 常见陷阱与调试建议

### 陷阱一：混淆 "hint" 与 "guarantee"
L2 缓存控制提供的是访问属性提示，不是强制的缓存锁定（cache locking）。即使设置了 `cudaAccessPropertyPersisting`，硬件在极端压力（如 L2 被大量 persisting 窗口挤爆）下仍可能逐出数据。不要把它当作 CPU 上的 `mlock` 或 GPU 上的 shared memory 来使用。它的收益体现在统计意义上——多次运行中平均访存延迟降低、带宽提升——而不是每一次访问都必然命中 L2。

### 陷阱二：忽略清理导致后续 kernel 性能下降
这是最隐蔽的错误。Persisting cache line 不会 kernel 一结束就消失。如果一个 Stream 先执行了 persisting kernel A，然后执行普通 kernel B，而 persisting 数据仍然占据着 L2 预留空间，那么 kernel B 的 normal 访问就只能竞争 L2 的非预留部分，可能导致性能意外下降。解决方案是严格执行清理流程：先设置 `num_bytes = 0` 禁用窗口，再调用 `cudaCtxResetPersistingL2Cache()`。

### 陷阱三：hitRatio 设置不当引发 thrashing
很多开发者初次使用时喜欢把 `hitRatio` 设为 1.0，认为这样"所有访问都是 persisting，效果最好"。但当窗口大小超过预留空间时，这反而会引发 thrashing——硬件不断把新的 cache line 加载到预留空间，又把旧的逐出，大量带宽浪费在无效的缓存替换上。正确的做法是：
- 单窗口场景：若窗口大小 <= 预留空间，`hitRatio = 1.0` 是合理的；若窗口更大，应降低 `hitRatio` 使预期 persisting 数据量匹配预留空间。
- 多窗口并发场景：计算各窗口预期占用（`num_bytes * hitRatio`）之和，确保不超过预留空间。

### 陷阱四：在不受支持的设备上静默失败
Compute Capability < 8.0 的设备不支持 L2 持久化。如果不检查 `prop.major` 就直接调用 `cudaDeviceSetLimit` 或 `cudaStreamSetAttribute`，可能得到错误码，也可能某些旧版本驱动下返回成功但实际无效。健壮的代码应在运行时检查 `prop.major >= 8`，并在不支持时打印明确提示或自动降级到普通执行路径。

### 陷阱五：base_ptr 指向错误的内存类型
`accessPolicyWindow.base_ptr` 必须是设备全局内存指针（`cudaMalloc` 分配的设备指针）。如果错误地传入了 host 指针、共享内存指针、常量内存指针或 managed memory 的 host 侧虚拟地址，`cudaStreamSetAttribute` 可能不报错，但 persisting 行为不会生效。

### 调试建议
1. **使用 Nsight Compute 验证**：在支持的设备上，Nsight Compute 可以报告 L2 缓存命中率和 persisting 访问的统计信息。如果启用了 L2 持久化但 L2 命中率没有明显提升，说明 persisting 没有生效，应检查窗口配置、hitRatio 和预留大小。
2. **对比实验**：在同样的 workload 下，分别测试"无 L2 控制"、"窗口 + hitRatio=1.0"、"窗口 + hitRatio=0.5" 三种配置的性能。如果 hitRatio=1.0 比不控制更慢，说明发生了 thrashing，需要降低 hitRatio 或缩小窗口。
3. **检查设备属性日志**：在程序启动时打印 `l2CacheSize`、`persistingL2CacheMaxSize`、`accessPolicyMaxWindowSize` 和 Compute Capability，确保程序行为和预期一致。
4. **关注 MPS/MIG 环境**：如果程序在数据中心 GPU（如 A100、H100）上运行，务必确认是否处于 MIG 或 MPS 模式，这两种模式对 L2 持久化有严格的额外限制。

## 一个最小可运行示例的说明

本目录下的 `chapter_demo.cu` 是一个围绕 L2 Cache Control 章节设计的最小可运行 CUDA 程序，配合 `Makefile` 可以一键编译运行。以下详细说明其设计思路、覆盖的章节内容、以及针对 sm_61 硬件的兼容性处理。

### 示例设计目标

该示例试图在单一代码文件中完整演示本章的"配置 -> 使用 -> 清理"闭环，同时保持代码最小化、可编译、可运行、可验证。具体包括：
1. 查询设备属性，打印 L2 缓存相关参数；
2. 根据 Compute Capability 判断是否支持 L2 持久化；
3. 若支持，完整执行预留空间设置、Stream 策略窗口绑定、kernel 反复启动、窗口禁用、全局重置的流程；
4. 若不支持，打印清晰的降级说明，解释当前硬件为什么不支持，并展示代码逻辑会在支持硬件上如何执行；
5. 包含 `CUDA_CHECK` 错误检查宏，确保任何 API 失败都能立即终止并输出错误信息；
6. 对 kernel 计算结果进行数值验证，保证程序逻辑正确性。

### 示例中的 Kernel 设计

Demo 包含两个 kernel：

1. `repeated_access_kernel`：让所有线程以 strided 方式反复扫描一块 4MB 的热点数据，累加后写回输出。这种访问模式正是 L2 持久化的理想用例——如果输入数据能驻留在 L2 中，多次迭代的 load 操作都能享受 L2 命中带来的低延迟。

2. `polluter_kernel`：在每次热点 kernel 测量结束后，扫描一块远大于 L2 容量（约 4 倍 L2 大小）的数组，以尽可能驱逐 L2 中的普通 cache line。这样在下一轮测量开始时，热点数据需要从 DRAM 重新加载，从而放大 baseline 与 streaming 之间的性能差异。

虽然在本机 sm_61 上无法真正触发 persisting 行为，但 kernel 的计算逻辑本身是正确的，可以通过数值校验验证。

### sm_61 兼容性处理

本示例的核心挑战在于：L2 Cache Control 是 Compute Capability 8.0+ 的特性，而编译目标和解码环境是 GTX 1060（sm_61, CUDA 12.8）。为了保证代码既能编译通过，又不会在不受支持的硬件上产生误导行为，做了以下兼容性设计：

1. **编译时无条件通过**：代码中使用了 `cudaLimitPersistingL2CacheSize`、`cudaStreamAttributeAccessPolicyWindow`、`cudaAccessPropertyPersisting` 等符号，它们都定义在 CUDA Runtime 头文件中。由于 CUDA 12.8 的 Runtime API 完整支持这些符号，即使编译目标为 `-arch=sm_61`，代码也能通过编译。这些 API 是 host 侧调用，不受 SM 架构代码生成的影响。

2. **运行时 Compute Capability 检测**：程序在 `main` 函数开头就通过 `cudaGetDeviceProperties` 获取 `prop.major` 和 `prop.minor`。如果 `prop.major < 8`，程序会打印详细的说明信息，包括当前 GPU 名称、Compute Capability、L2 缓存大小、以及为什么 L2 持久化不可用。然后程序会跳转到清理逻辑，正常退出，不会尝试调用任何可能在旧设备上行为未定义的 persisting API。

3. **数值验证作为功能正确性兜底**：即使 L2 持久化没有生效，kernel 本身会正确执行向量累加。程序最后会回读 GPU 结果并与 CPU 计算的期望值（`iterations * input_value`）比对。如果结果一致，说明 CUDA 执行链路（内存分配、数据传输、kernel 启动、同步、回读）全部正确，只是没有 L2 持久化加速。

4. **属性字段安全读取**：`cudaDeviceProp` 结构体在 CUDA 12.8 中包含 `persistingL2CacheMaxSize` 和 `accessPolicyMaxWindowSize` 字段，无论设备是否支持该特性。在 sm_61 上这些字段的值通常为 0，程序打印这些值可以作为"该设备不支持 L2 持久化"的佐证。

### 在 sm_61 上的预期输出

在 GTX 1060 上运行该程序，预期输出如下：
- 设备名称：NVIDIA GeForce GTX 1060 3GB
- Compute Capability：6.1
- L2 Cache Size：1536 KB（或其他实际值）
- Persisting L2 Cache Max Size：0 KB（或 0 bytes）
- Access Policy Max Window Size：0
- 提示信息：L2 Cache Persistence requires compute capability 8.0+ (sm_80). Current device is sm_61, so this feature is not supported.
- 后续列出若运行在 sm_80+ 设备上的完整 API 执行流程。

### 在 sm_80+ 上的预期执行流程

如果同一份代码在 Ampere 或更新架构的 GPU 上编译运行，程序会执行完整流程：
1. 设置 L2 预留空间为 L2 总容量的 75%（不超过上限）；
2. 创建 Stream，绑定 access policy 窗口到 `d_input`，窗口大小取 `min(accessPolicyMaxWindowSize, num_bytes)`，`hitRatio = 1.0`；
3. 分别测试三种场景，每次测量前先用 polluter kernel 冲刷 L2 cache：
   - **Baseline**：不设置任何窗口；
   - **Persisting**：`hitProp = cudaAccessPropertyPersisting`；
   - **Streaming**：`hitProp = cudaAccessPropertyStreaming`；
4. 禁用窗口（`num_bytes = 0`）；
5. 调用 `cudaCtxResetPersistingL2Cache()`；
6. 数值验证并打印结果。

### 编译与运行方式

在项目根目录执行：
```bash
make 04.13-l2-cache-control/chapter_demo.out
./04.13-l2-cache-control/chapter_demo.out
```

或在 `04.13-l2-cache-control` 目录下直接运行已编译好的二进制：
```bash
cd 04.13-l2-cache-control
./chapter_demo.out
```

项目根目录的 `Makefile` 会自动为所有 `chapter_demo.cu` 生成编译规则，使用以下参数：
- `nvcc -ccbin g++ -std=c++17 -arch=sm_120 -O2`

`CUDA_CHECK` 宏会在任何 CUDA API 调用失败时立即终止程序并输出错误位置和描述，方便快速定位问题。
