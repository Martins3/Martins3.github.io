## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/environment-variables.html>


CUDA Environment Variables 章节系统梳理了 NVIDIA CUDA 运行时及驱动层面提供的全部环境变量。这些变量允许开发者和系统管理员在不重新编译程序的前提下，直接干预 GPU 设备枚举顺序、JIT 编译缓存策略、内核启动同步行为、CUDA 模块加载时机以及错误日志输出路径等关键环节。根据 CUDA Programming Guide Release 13.2 的划分，这些变量被归为五大类：设备枚举与属性控制（Device Enumeration and Properties）、JIT 编译（JIT Compilation）、执行行为（Execution）、模块加载（Module Loading）以及 CUDA 错误日志管理（CUDA Error Log Management）。理解每一个变量的语义、取值范围、副作用及彼此之间的覆盖关系，是在多 GPU 集群调试性能问题、排查兼容性故障以及优化启动延迟时不可或缺的技能。

## 背景与要解决的问题

在 GPU 计算日趋普及的生产环境中，同一个 CUDA 可执行文件往往需要在截然不同的硬件配置和软件约束下运行：有的服务器装配了 8 张甚至 16 张 GPU，用户却只希望进程看到其中特定几张；有的场景要求为了调试而强制同步执行，有的场景则希望最大化并发；驱动升级后 JIT 编译出的二进制可能存在细微差异，需要验证 PTX 路径是否工作正常；程序启动时一次性加载全部内核会拖慢初始化，延迟加载又可能在首次调用时引入不可预测的停顿。如果所有这些行为都必须通过修改源码或重新编译来控制，运维和调试成本将极高。因此，CUDA 提供了一套以环境变量为接口的运行时调控机制，使得"同一份二进制，多种运行行为"成为可能。

## 核心概念与术语

- **设备枚举（Device Enumeration）**：CUDA 运行时在程序启动时为系统中所有可用的 GPU 分配逻辑序号（ordinal）的过程。`cudaGetDeviceCount()` 返回的正是枚举后可见设备的数量。
- **PCI Bus ID**：每张 GPU 在 PCI 总线上的唯一标识，可通过 `nvidia-smi --query-gpu=name,pci.bus_id` 获取。
- **GPU UUID**：每张 GPU 的全局唯一标识符，格式如 `GPU-8932f937-d72c-4106-c12f-20bd9faed9f6`，可通过 `nvidia-smi -L` 查看。
- **MIG（Multi-Instance GPU）**：Ampere 架构引入的 GPU 虚拟化技术，允许将一张物理 GPU 划分为多个独立的 GPU 实例。CUDA 环境变量支持通过 `MIG-<GPU-UUID>/<GPU instance ID>/<compute instance ID>` 的形式枚举单个 MIG 实例。
- **PTX（Parallel Thread Execution）**：CUDA 的中间指令集架构，具有前向兼容性。驱动可在运行时将 PTX 即时编译（JIT）为对应架构的 CUBIN 二进制。
- **CUBIN**：CUDA 设备二进制文件，包含针对特定计算能力（compute capability）的机器码。
- **JIT 编译缓存**：驱动将 PTX 编译为 CUBIN 后，默认会把结果缓存到磁盘，避免每次运行都重复编译。缓存路径和大小均可通过环境变量调整。
- **工作队列 / 连接（Work Queues / Connections）**：GPU 内部的硬件队列，用于调度内核和拷贝操作。若多个流映射到同一队列，可能产生虚假依赖（false dependency），导致串行化。
- **统一内存（Unified Memory）**：在 multi-GPU 系统中，统一内存的物理存储位置默认由驱动自动管理；`CUDA_MANAGED_FORCE_DEVICE_ALLOC` 可强制将其放在设备内存中。
- **持久 L2 缓存（Persisting L2 Cache）**：从 compute capability 8.0 开始支持的特性，允许为特定访存预留一部分 L2 缓存，以降低延迟。

## API / 机制详解

### 设备枚举与属性控制

#### CUDA_VISIBLE_DEVICES
该变量直接决定 CUDA 应用能看到哪些 GPU，以及它们的枚举顺序。
- **未设置**：所有 GPU 均可见。
- **设为空字符串**：没有任何 GPU 可见，`cudaGetDeviceCount()` 返回 0。
- **整数索引**：以逗号分隔，对应 `nvidia-smi` 显示的序号，从 0 开始。例如 `CUDA_VISIBLE_DEVICES=2,1` 会使得设备 0 不可见，而原设备 2 被赋予 ordinal 0，原设备 1 被赋予 ordinal 1。
- **无效索引的截断语义**：若列表中出现无效索引，则仅保留该无效索引之前出现的设备。例如 `CUDA_VISIBLE_DEVICES=0,2,-1,1` 中，-1 是无效的，因此只有 0 和 2 可见，设备 1 被忽略。这个细节在多机多卡脚本中极易踩坑。
- **GPU UUID 字符串**：可接受完整 UUID，也可接受足够长度的前缀（只要能唯一标识目标 GPU）。这在设备频繁热插拔或序号不稳定的集群环境中尤为重要。
- **MIG 支持**：格式为 `MIG-<GPU-UUID>/<GPU instance ID>/<compute instance ID>`，例如 `MIG-GPU-8932f937-d72c-4106-c12f-20bd9faed9f6/1/2`。注意仅支持枚举单个 MIG 实例。

#### CUDA_DEVICE_ORDER
控制 CUDA 枚举设备时采用的排序策略。
- `FASTEST_FIRST`（默认）：使用简单启发式规则按性能从高到低枚举。
- `PCI_BUS_ID`：按 PCI Bus ID 升序枚举。该变量常与 `CUDA_VISIBLE_DEVICES` 配合使用，以确保跨节点作业获得稳定、可预测的设备映射。

#### CUDA_MANAGED_FORCE_DEVICE_ALLOC
改变 multi-GPU 环境下统一内存（Unified Memory）的物理存储位置。
- **非零值**：强制驱动使用设备内存进行物理存储。此时，进程内所有使用统一内存的设备必须支持 peer-to-peer（P2P）访问，否则将返回 `cudaErrorInvalidDevice`。
- **0**：默认行为，由驱动自行决定数据驻留位置。

### JIT 编译控制

#### CUDA_CACHE_DISABLE
控制磁盘上的 JIT 编译缓存。
- **1**：禁用缓存。每次执行时，若二进制中不含当前架构的 CUBIN，则强制将 PTX 重新编译为 CUBIN。这会增加启动时间，但有助于排查驱动版本或编译标志差异导致的问题，也能节省磁盘空间。
- **0**（默认）：启用缓存。

#### CUDA_CACHE_PATH
指定 JIT 缓存目录的绝对路径。
- **Linux 默认**：`~/.nv/ComputeCache`
- **Windows 默认**：`%APPDATA%\NVIDIA\ComputeCache`
在集群或容器环境中，将该路径指向节点本地的高速存储（如 NVMe 临时盘），可显著减少多用户竞争主目录 NFS 带来的性能瓶颈。

#### CUDA_CACHE_MAXSIZE
指定 JIT 缓存的最大字节数。超出此大小的二进制不会被缓存；若空间不足，旧的缓存条目会被逐出。
- **桌面/服务器默认**：1073741824（1 GiB）
- **嵌入式平台默认**：268435456（256 MiB）
- **上限**：4294967296（4 GiB）

#### CUDA_FORCE_PTX_JIT 与 CUDA_FORCE_JIT
强制驱动忽略应用内嵌的 CUBIN，改为对嵌入的 PTX 执行 JIT 编译。这对验证 PTX 是否已正确嵌入、以及测试未来架构的前向兼容性非常有用。
- `CUDA_FORCE_PTX_JIT` 的优先级高于 `CUDA_FORCE_JIT`：若两者同时设置，以前者为准。
- **1**：强制 PTX JIT；**0**：默认行为。

#### CUDA_DISABLE_PTX_JIT 与 CUDA_DISABLE_JIT
与上一对变量相反，它们禁用 PTX 的 JIT 编译，强制使用嵌入的 CUBIN。
- 若某内核没有嵌入兼容当前架构的 CUBIN，则加载会失败。这常用于验证应用是否已为所有目标架构生成了兼容的二进制。
- `CUDA_DISABLE_PTX_JIT` 覆盖 `CUDA_DISABLE_JIT`。

#### CUDA_FORCE_PRELOAD_LIBRARIES
影响 NVVM 和 JIT 编译所需库的预加载时机。
- **1**：在 CUDA 驱动初始化阶段就预加载相关库。这会增加内存占用和初始化时间，但能避免多线程场景下的特定死锁（deadlock）问题。
- **0**（默认）：按需加载。

### 执行行为控制

#### CUDA_LAUNCH_BLOCKING
控制内核启动是否异步。
- **1**：禁用异步执行。所有 GPU 工作从 CPU 视角看都是同步完成的。CUDA API 错误会在触发它们的 API 调用处立即返回，而不是延迟到后续同步点才暴露。这对调试极有价值。
- **0**（默认）：异步执行。
需要特别注意的是，开启此选项通常会显著降低整体性能，因为 CPU 无法通过下发队列掩盖内核执行延迟。

#### CUDA_DEVICE_MAX_CONNECTIONS
控制单个上下文中并发计算引擎连接（work queues）与拷贝引擎连接的数量。不同 CUDA 流中的独立任务若被映射到同一 work queue，会因共享底层资源而产生虚假依赖，导致 GPU 工作串行化。为降低此类风险，建议将此值设置为不小于每个上下文中活跃 CUDA 流的数量。
- **取值范围**：1 到 32
- **默认值**：8（假设未使用 MPS）
若同时设置了 `CUDA_DEVICE_MAX_COPY_CONNECTIONS`，则拷贝连接数以该专门变量为准。

#### CUDA_DEVICE_MAX_COPY_CONNECTIONS
仅影响 compute capability 8.0 及以上设备的并发拷贝连接数。若与 `CUDA_DEVICE_MAX_CONNECTIONS` 同时设置，此变量生效，覆盖前者对拷贝连接的设定。
- **取值范围**：1 到 32
- **默认值**：8（假设未使用 MPS）

#### CUDA_SCALE_LAUNCH_QUEUES
调整命令缓冲区（command buffer）队列大小的缩放因子，即设备上可排队等待执行的最多内核或拷贝操作数。
- **合法取值**：`0.25x`、`0.5x`、`2x`、`4x`
- 其他值均被视为 `1x`（默认）。
在提交大量小内核的应用中，放大队列可减少 CPU 侧阻塞；缩小队列则有助于降低延迟敏感任务的尾延迟。

#### CUDA_GRAPHS_USE_NODE_PRIORITY
控制 CUDA Graph 执行时是否尊重节点级启动优先级，还是简单地继承其启动流的优先级。
- **0**（默认）：Graph 继承启动流的优先级。
- **1**：尊重每个节点在创建时指定的优先级。运行时将其视为对就绪节点的调度提示。
此变量会覆盖通过 `cudaGraphInstantiateFlagUseNodePriority` 标志在图实例化时指定的行为。

#### CUDA_DEVICE_WAITS_ON_EXCEPTION
调试辅助变量。启用后，当设备端发生异常（如非法内存访问）时，CUDA 应用会暂停等待，而不是立即退出。
- **1**：发生异常时挂起，允许开发者 attach `cuda-gdb` 等调试器检查 GPU 现场状态。
- **0**（默认）：正常报错并退出。

#### CUDA_DEVICE_DEFAULT_PERSISTING_L2_CACHE_PERCENTAGE_LIMIT
设置 GPU L2 缓存中用于持久访问（persisting accesses）的默认保留比例，以百分比表示。
- 仅对 compute capability 8.0 及以上、且使用 CUDA MPS 的场景有意义。
- 必须在启动 `nvidia-cuda-mps-control -d` 之前设置。
- **默认值**：0；**合法范围**：0 到 100。

#### CUDA_DISABLE_PERF_BOOST
仅限 Linux 主机。
- **1**：阻止驱动提升设备性能状态（performance state），而是由启发式规则隐式选择 pstate。可用于降低功耗，但可能因动态调频而增加延迟。
- **0**（默认）。

#### CUDA_AUTO_BOOST（已弃用）
影响 GPU 的动态时钟提升（auto boost）行为，会覆盖 `nvidia-smi --auto-boost-default` 的设置。官方强烈建议使用 `nvidia-smi --applications-clocks=<memory,graphics>` 或 NVML API 替代。

### 模块加载控制

#### CUDA_MODULE_LOADING
控制 CUDA 运行时初始化设备代码的方式。
- `DEFAULT`：默认行为，等价于 `LAZY`。
- `LAZY`：延迟加载。只有在通过 `cuModuleGetFunction()` 或 `cuKernelGetFunction()` 提取函数句柄时，才会加载特定内核；CUBIN 中的数据则在第一个内核加载或第一个变量被访问时才加载。首次调用后，后续调用无额外开销。优点：减少启动时间和 GPU 内存占用。
- `EAGER`：在程序初始化时完全加载模块和所有内核。调用 `cuModuleLoad*` 或 `cuLibraryLoad*` 时，整个 CUBIN/FATBIN/PTX 的内容都会被加载。优点：内核启动开销可预测；缺点：启动慢、显存占用高。

#### CUDA_MODULE_DATA_LOADING
作为 `CUDA_MODULE_LOADING` 的补充，专门控制与模块相关的**数据**加载时机，不影响内核的延迟或立即加载。
- 若未设置，数据加载行为继承自 `CUDA_MODULE_LOADING`。
- `LAZY`：延迟加载模块数据。注意懒加载数据可能需要上下文同步，从而拖慢并发执行。
- `EAGER`：在 `cuModuleLoad*` / `cuLibraryLoad*` 时完全加载所有数据。

#### CUDA_BINARY_LOADER_THREAD_COUNT
设置加载设备二进制时使用的 CPU 线程数。
- **0**（默认）：实际使用 1 个线程。
- 设为更大的整数可利用多核 CPU 加速包含大量内核的 fatbinary 加载过程。

### 错误日志管理

#### CUDA_LOG_FILE
指定一个文件路径，CUDA 会在支持的错误发生时，将描述性错误日志写入该文件。
- 例如，若用非法维度 `dim3(1,1,128)` 启动内核，`cudaGetLastError()` 只会返回一个泛泛的 "invalid configuration argument"；而开启此日志后，文件中会出现类似 `[CUDA][E] Block Dimensions (1,1,128) include one or more values that exceed the device limit of ...` 的详细说明。这对快速定位参数越界问题非常有帮助。

## 典型工作流程 / 调用顺序

1. **环境准备阶段**：在 shell 或作业调度系统（Slurm、K8s）中导出所需环境变量，例如 `export CUDA_VISIBLE_DEVICES=2,3` 和 `export CUDA_LAUNCH_BLOCKING=1`。
2. **程序启动**：操作系统将环境变量注入进程地址空间。CUDA 驱动和运行时库在初始化时读取这些变量。
3. **设备枚举**：驱动根据 `CUDA_DEVICE_ORDER` 决定排序策略，再根据 `CUDA_VISIBLE_DEVICES` 过滤可见设备。`cudaGetDeviceCount()` 返回过滤后的数量。
4. **上下文建立与模块加载**：若设置了 `CUDA_MODULE_LOADING=EAGER`，此时会将全部 CUBIN/PTX 载入设备内存；若为 `LAZY`，则仅做簿记，真正的加载推迟到首次获取函数句柄或访问变量时。
5. **JIT 编译阶段**：若当前架构的 CUBIN 缺失且未禁用 JIT（`CUDA_DISABLE_PTX_JIT=0`），驱动将编译嵌入的 PTX。若缓存启用且命中，则直接读取磁盘缓存；否则现场编译并视缓存策略决定是否写入缓存。
6. **内核执行**：根据 `CUDA_LAUNCH_BLOCKING` 决定启动是同步还是异步；根据 `CUDA_DEVICE_MAX_CONNECTIONS` 分配 work queue；根据 `CUDA_SCALE_LAUNCH_QUEUES` 分配命令缓冲区。
7. **异常与日志**：若发生错误且 `CUDA_LOG_FILE` 已设置，运行时向指定文件输出详细诊断信息；若 `CUDA_DEVICE_WAITS_ON_EXCEPTION=1`，进程挂起等待调试器 attach。
8. **资源释放**：程序退出时，驱动清理上下文和队列。若使用 MPS，其守护进程继续驻留，等待后续客户端连接。

## 关键限制、边界条件与兼容性

- **无效索引截断**：`CUDA_VISIBLE_DEVICES` 遇到非法索引时，不会报错，而是静默截断列表。这在自动化脚本中可能导致部分 GPU 未被使用而无人察觉。
- **MIG 枚举限制**：仅支持枚举单个 MIG 实例，不能一次性列出多个 MIG slice。
- **覆盖优先级**：`CUDA_FORCE_PTX_JIT` > `CUDA_FORCE_JIT`；`CUDA_DISABLE_PTX_JIT` > `CUDA_DISABLE_JIT`。若同时设置强制和禁用 JIT，行为取决于具体变量组合，但通常不建议同时启用互斥标志。
- **计算能力门槛**：`CUDA_DEVICE_MAX_COPY_CONNECTIONS` 和 `CUDA_DEVICE_DEFAULT_PERSISTING_L2_CACHE_PERCENTAGE_LIMIT` 均要求 compute capability 8.0（Ampere）及以上。在 Pascal（sm_61）等旧架构上设置这些变量无效。
- **平台限制**：`CUDA_DISABLE_PERF_BOOST` 仅对 Linux 有效；`CUDA_AUTO_BOOST` 已弃用。
- **MPS 前置条件**：持久 L2 缓存比例必须在启动 `nvidia-cuda-mps-control -d` 之前设定，事后修改不会生效。
- **缓存大小硬上限**：`CUDA_CACHE_MAXSIZE` 最大只允许 4 GiB，超出会被截断或拒绝。
- **阻塞启动的性能代价**：`CUDA_LAUNCH_BLOCKING=1` 会消除 CPU-GPU 并行性，仅建议用于调试。
- **队列缩放离散值**：`CUDA_SCALE_LAUNCH_QUEUES` 只接受 0.25x、0.5x、2x、4x 四个离散档位，其他值 fallback 到 1x。

## 常见陷阱与调试建议

- **设备序号混淆**：`CUDA_VISIBLE_DEVICES=2,1` 后，调用 `cudaSetDevice(0)` 实际上操作的是原物理设备 2。日志和监控中应同时记录逻辑 ordinal 与物理 UUID/Bus ID，避免排查时张冠李戴。
- **JIT 缓存污染**：在升级驱动或修改编译器版本后，旧的 JIT 缓存可能导致难以复现的二进制差异。调试兼容性问题时，建议先 `export CUDA_CACHE_DISABLE=1` 排除缓存干扰。
- **延迟加载的首次调用延迟**：`CUDA_MODULE_LOADING=LAZY` 能缩短启动时间，但首次调用某个内核时可能触发显著停顿。对延迟敏感的服务，建议在正式 Serving 前做一次"热身"（warm-up）调用。
- **多线程死锁**：若应用在多线程环境下同时触发首次 JIT 编译或库加载，可能遇到驱动内部的锁竞争甚至死锁。此时可尝试设置 `CUDA_FORCE_PRELOAD_LIBRARIES=1`，以初始化时间为代价换取稳定性。
- **Graph 优先级被覆盖**：如果代码里显式使用了 `cudaGraphInstantiateFlagUseNodePriority`，但运行环境设置了 `CUDA_GRAPHS_USE_NODE_PRIORITY=0`，则节点级优先级不会生效。需确认两者一致。
- **日志文件权限**：`CUDA_LOG_FILE` 指向的路径必须对运行用户可写，否则运行时可能静默失败，不会报错。
- **空字符串与未设置的区别**：`CUDA_VISIBLE_DEVICES=""` 会隐藏所有 GPU，而不设置则显示全部。某些脚本在条件判断时容易将空值视为未设置，导致逻辑错误。

## 一个最小可运行示例的说明

本示例 `chapter_demo.cu` 的设计目标是：在单张 GTX 1060（Pascal, sm_61, CUDA 12.8）上，以最小代码量展示多个环境变量对运行时行为的影响，并对 sm_61 不支持的功能做出运行时检测与友好提示。

### 示例结构

1. **环境变量自报告**：程序启动后，通过标准 C 库的 `getenv()` 主动扫描并打印 18 个常见 CUDA 环境变量的当前值。这能让用户直观地看到"环境变量是否真正传入了进程"。
2. **设备枚举演示**：调用 `cudaGetDeviceCount()` 获取可见设备数。若用户在运行前设置了 `CUDA_VISIBLE_DEVICES`，此处返回的数量会随之变化，直观验证过滤效果。随后循环打印每张可见设备的名称、计算能力（sm_xy）和 PCI Bus ID，并对比 `CUDA_DEVICE_ORDER=PCI_BUS_ID` 与默认排序的差异。
3. **计算能力兼容性检查**：在设备属性查询循环中，若检测到 `major < 8`（即 Pascal 或更早架构），程序会打印提示信息，说明 `CUDA_DEVICE_MAX_COPY_CONNECTIONS` 和 `CUDA_DEVICE_DEFAULT_PERSISTING_L2_CACHE_PERCENTAGE_LIMIT` 在该硬件上不可用。这避免了用户对这些变量"设置后无效"产生困惑。
4. **内核启动与同步行为演示**：示例包含一个简单的 `vector_add` 内核。程序记录从启动到 `cudaGetLastError()` 返回的时间（反映 launch latency），再记录到 `cudaDeviceSynchronize()` 完成的时间（反映总执行时间）。如果用户设置了 `CUDA_LAUNCH_BLOCKING=1`，launch latency 会显著增大，因为此时 launch 已经包含了整个内核的执行；程序会在终端打印相应提示，帮助用户理解阻塞与非阻塞模式的区别。
5. **错误检查宏**：所有 CUDA API 调用均包裹在 `CUDA_CHECK` 宏中。若失败，宏会输出文件名、行号、错误码及错误描述字符串，并立即终止进程。这是生产级 CUDA 代码的基本规范。

### Makefile 与编译参数

`Makefile` 中指定的编译参数严格遵循用户要求：
- `nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2`
- 默认目标生成 `chapter_demo.out`
- 提供 `make clean` 和 `make run` 辅助目标

### sm_61 上的降级与兼容处理

由于 Pascal（sm_61）不支持 Ampere 引入的部分特性，示例未在代码中硬编码调用这些 API，而是通过设备属性查询做**运行时检测**并打印说明。例如：
- 不调用与持久 L2 缓存相关的 API（该特性 8.0+ 才存在）。
- 不依赖 `CUDA_DEVICE_MAX_COPY_CONNECTIONS` 的行为差异（拷贝连接数控制对 sm_61 无效，由驱动忽略）。
- 内核本身仅使用最基础的线程索引计算和全局内存访问，确保在 sm_61 上无需 PTX JIT 即可直接运行嵌入的 CUBIN。

用户可以通过以下命令组合来体验不同环境变量的效果：

```bash
# 基础运行
make run

# 只让程序看到设备 0（若存在多张卡）
CUDA_VISIBLE_DEVICES=0 ./chapter_demo.out

# 同步模式调试用法
CUDA_LAUNCH_BLOCKING=1 ./chapter_demo.out

# 禁用 JIT 缓存，观察启动差异（若有 PTX 路径）
CUDA_CACHE_DISABLE=1 ./chapter_demo.out
```

该示例虽然功能极简，但覆盖了"设备枚举可见性""环境变量自报告""阻塞/非阻塞启动差异"以及"旧架构兼容性提示"四个章节核心要点，足以作为学习和验证 CUDA 环境变量机制的起点。
