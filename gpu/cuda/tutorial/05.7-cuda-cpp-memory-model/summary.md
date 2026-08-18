## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cuda-cpp-memory-model.html>


本章（CUDA Programming Guide Release 13.2，第 5.7 节）系统阐述了 **CUDA C++ Memory Model**，即 CUDA 对标准 C++ 内存模型与并发设施的扩展。NVIDIA 引入这套机制的根本动机在于：标准 C++ 假设线程同步的代价是均匀且低廉的，而 CUDA 程序的执行环境横跨同一块 GPU 内的线程块、同一设备内的多个线程块、多 GPU 乃至 CPU 线程，同步代价随线程"距离"增大而显著上升。为此，CUDA 在 `cuda::` 命名空间中扩展了线程作用域（thread scopes）概念，并在默认情况下保持与标准 C++ 一致的语法和语义，使开发者既能利用熟悉的原子操作、内存序和栅栏，又能精确控制同步的可见范围，从而在正确性与性能之间取得平衡。

除了核心内存模型，本章还简要涉及了嵌套启动（CDP）中的 ECC 错误报告机制（5.6.4.9），以及一个典型的消息传递示例（5.7.5），用于说明 `memory_order_release` 与 `memory_order_acquire` 在设备作用域下的实际用法。

---

## 背景与要解决的问题

### 标准 C++ 内存模型的假设

ISO C++ 的内存模型建立在"对称多处理"（SMP）抽象之上：所有线程在逻辑上平等，同步原语（如 `std::atomic`、`std::mutex`）的代价被假定为均匀且可接受。在这一模型下，原子操作的默认作用域隐含为"全系统可见"。

### CUDA 的异构与层级并行特性

CUDA 程序的线程组织具有严格的层级结构：

- **线程（Thread）**：最基本的执行单元，拥有私有寄存器和局部内存。
- **线程块（Block）**：由若干线程组成，共享一块高速的 Shared Memory，可通过 `__syncthreads()` 进行低成本同步。
- **网格（Grid）**：由若干线程块组成，共同完成一个核函数启动。
- **设备（Device）**：一个独立的 GPU，包含多个 SM（Streaming Multiprocessor）。
- **系统（System）**：可能包含多个 GPU、多个 CPU 插槽以及复杂的 PCIe/NVLink 互联拓扑。

在这一架构下，让块内两个线程同步的代价极低（只需一条屏障指令），但让不同 GPU 上的线程同步则可能需要涉及 PCIe 事务、系统总线仲裁甚至操作系统调度，代价高出数个数量级。如果强制所有原子操作都使用"系统级"语义，不仅会造成严重的性能浪费，还可能在不支持系统级原子性的硬件上导致未定义行为。

### CUDA 的解决方案

CUDA C++ 的解决方案是**保留标准 C++ 的语法和默认语义**，同时引入**显式的线程作用域参数**。开发者可以：

1. 继续使用 `std::` 或 `cuda::std::` 的同步原语，它们默认表现为 `cuda::thread_scope_system`。
2. 在性能敏感场景下，显式使用 `cuda::atomic<T, Scope>` 或 `cuda::atomic_ref<T, Scope>`，将同步范围缩小到 `thread_scope_device`、`thread_scope_block` 甚至 `thread_scope_thread`。

这种设计既兼容现有代码，又为底层优化提供了精确控制手段。

---

## 核心概念与术语

### 线程作用域（Thread Scopes）

CUDA 在 `cuda::` 命名空间中定义了枚举 `thread_scope`，包含四个层级：

```cpp
namespace cuda {
enum thread_scope {
    thread_scope_system,   // 系统级：所有 CPU 和 GPU 线程
    thread_scope_device,   // 设备级：同一 GPU 内、同一内存同步域中的所有线程
    thread_scope_block,    // 块级：同一线程块内的所有线程
    thread_scope_thread    // 线程级：仅线程自身
};
}
```

这四个作用域构成了一个由宽到窄的层级体系。选择更窄的作用域意味着：编译器和硬件可以采用更轻量的同步机制，减少不必要的缓存一致性流量和内存屏障开销。

### 作用域关系（Scope Relationships）

每个程序线程与其他线程之间通过一种或多种作用域关系相关联：

- **系统作用域关系**：系统中任意两个线程之间都存在此关系，对应 `cuda::thread_scope_system`。
- **设备作用域关系**：同一 CUDA 设备内、且处于同一内存同步域（memory synchronization domain）的任意两个 GPU 线程之间存在此关系，对应 `cuda::thread_scope_device`。
- **块作用域关系**：同一线程块内的任意两个 GPU 线程之间存在此关系，对应 `cuda::thread_scope_block`。
- **线程作用域关系**：每个线程与自身之间存在此关系，对应 `cuda::thread_scope_thread`。

这里的"内存同步域"是一个容易被忽略的细节。在现代 GPU 中，特别是支持多实例 GPU（MIG）或具有复杂缓存层次结构的设备上，并非设备内所有线程都默认处于同一个同步域。某些系统配置可能将设备划分为多个域，域内的同步代价低于跨域同步。开发者在编写假设"设备内所有线程可见"的代码时，应意识到这一潜在边界。

### 同步原语（Synchronization Primitives）

CUDA C++ 提供的同步原语分布在多个命名空间中：

- **`cuda::` 命名空间**：提供显式作用域版本的原语，如 `cuda::atomic<T, Scope>`、`cuda::atomic_ref<T, Scope>`、`cuda::barrier<Scope>` 等。
- **`std::` 与 `cuda::std::` 命名空间**：这些命名空间中的类型与 `cuda::` 中显式指定 `cuda::thread_scope_system` 的类型行为完全一致。

这种设计的好处是：如果你已经在主机端代码中使用了 `std::atomic`，将其替换为 `cuda::std::atomic` 后，在设备端代码中的语义保持一致；同时，你可以在需要时将作用域收紧为 `thread_scope_device` 或 `thread_scope_block`，以获得更好的性能。

### 原子性与可见性

原子性（Atomicity）在 CUDA C++ 中不是绝对的，而是**相对于所指定的作用域**而言的。一个原子操作在特定作用域下是原子的，当且仅当满足该作用域对应的硬件与内存条件。这一点与标准 C++ 有本质差异：标准 C++ 中的原子性通常默认是全系统可见的，而 CUDA 中的原子性可能仅在设备内或块内成立。

---

## API / 机制详解

TODO 细究
### `cuda::atomic_ref<T, Scope>` 与 `cuda::atomic<T, Scope>`

`cuda::atomic_ref` 是 CUDA C++ 中最重要的新增类型之一，它对标 C++20 的 `std::atomic_ref`，但增加了第二个模板参数用于指定线程作用域。

- **它做什么**：提供一个非拥有的原子操作视图，允许对现有内存位置进行原子读写，而不像 `cuda::atomic` 那样自己管理存储。
- **什么时候调用**：当你需要对已经通过 `cudaMalloc`、`new` 或全局变量分配的内存进行原子操作时。
- **调用前需要什么**：目标内存必须已分配且生命周期至少覆盖所有可能并发访问的线程；目标地址应对齐到类型自然边界（naturally-aligned）。
- **调用后得到什么**：对该内存位置的原子读写保证，保证范围由 `Scope` 参数决定。
- **常见错误**：
  - 对未对齐的地址创建 `atomic_ref`，可能导致非原子访问或总线错误。
  - 在设备代码中使用 `thread_scope_system`，但底层硬件或内存类型不支持系统级原子性（见下文原子性条件）。
  - 混淆 `atomic_ref` 与 `atomic` 的生命周期：`atomic_ref` 不管理内存，销毁 `atomic_ref` 不会释放底层内存。

### `cuda::barrier<Scope>` 与 `cuda::std::barrier`

`cuda::barrier` 提供了灵活的线程屏障机制，支持到达与等待的分离（arrive-and-wait）、到达与退出的分离（arrive-and-drop）等高级用法。与 `atomic_ref` 类似，显式的 `Scope` 模板参数允许将同步范围限制在块内或设备内。

### 内存序（Memory Orders）

CUDA C++ 完整支持标准 C++ 的内存序：

- `memory_order_relaxed`：仅保证原子性，不保证顺序。
- `memory_order_consume`：数据依赖顺序（实际使用中较少见）。
- `memory_order_acquire`：当前线程中，后续读操作不能重排到此操作之前。
- `memory_order_release`：当前线程中，前置写操作不能重排到此操作之后。
- `memory_order_acq_rel`：同时具有 acquire 和 release 语义。
- `memory_order_seq_cst`：顺序一致性，最强约束。

在 CUDA 的消息传递模式中，最常用的是 `memory_order_release`（写端）与 `memory_order_acquire`（读端）配对。写端在 `release` 之前对所有内存的写入，对读端在 `acquire` 成功之后的读操作可见。

### 原子性成立的条件（5.7.3 节）

TODO 这个需要细究
一个原子操作在指定作用域下具备原子性，需满足以下条件之一：

1. **作用域非 `thread_scope_system`**：即 `thread_scope_device`、`thread_scope_block` 或 `thread_scope_thread`。此时只要操作在 GPU 内存上执行，且满足对齐要求，通常都能保证原子性。

2. **作用域为 `thread_scope_system`**：此时需要进一步满足严格的硬件与内存条件：
   - **系统分配内存（System Allocated Memory）**：要求 `pageableMemoryAccess == 1`。若 `pageableMemoryAccessUsesHostPagetables == 0`，则对内存映射文件（mmap）或 hugetlbfs 分配的原子操作**不保证原子**。
   - **托管内存（Managed Memory / Unified Memory）**：要求 `concurrentManagedAccess == 1`。
   - **映射内存（Mapped Memory）**：要求 `hostNativeAtomicSupported == 1`。
   - **映射内存上的自然对齐 load/store**：对 1、2、4、8 或 16 字节大小的自然对齐对象进行 load 或 store，即使 `hostNativeAtomicSupported == 0` 也可能成立。但对于 16 字节对象，若 `hostNativeAtomicSupported == 0`，则需要系统支持；NVIDIA 表示目前未知存在不支持的系统，且没有 CUDA API 可查询此支持。
   - **GPU 内存**：仅 GPU 线程访问时，要求：
     - 通过 `cudaDeviceGetP2PAttribute(&val, cudaDevP2PAttrNativeAtomicSupported, srcDev, dstDev)` 查询每个访问源设备 `srcDev` 与对象所在目标设备 `dstDev` 之间的原生原子支持；若值为 1，则支持；或者
     - 仅来自**单一 GPU** 的线程并发访问该对象。

这些条件的复杂性意味着：在编写跨主机-设备或跨多 GPU 的代码时，**不能盲目假设 `thread_scope_system` 的原子操作一定有效**。必须在运行时通过 `cudaDeviceGetAttribute` 和 `cudaDeviceGetP2PAttribute` 进行能力查询，并根据查询结果回退到更保守的同步策略（如使用显式的 `cudaMemcpy` 或 CUDA 事件进行同步）。

### 数据竞争（Data Races）的扩展定义

CUDA C++ 对 ISO C++ 标准中多处涉及数据竞争的条款进行了修改，核心思想是：将"原子操作"替换为"在包含另一操作线程的作用域下具备原子性的操作"。

具体修改包括：

1. **intro.races 第 21 段**：程序包含数据竞争，当且仅当包含两个潜在的并发冲突动作，且至少有一个动作在"包含执行另一操作的线程"的作用域下**不是原子的**，且二者不存在 happens-before 关系。

2. **`barrier`、`latch`、`counting_semaphore`**：并发调用这些类型的成员函数时，不引入数据竞争，但前提是这些调用在相应作用域下表现为原子操作。

3. **`stop_token` 相关函数**：`request_stop`、`stop_requested`、`stop_possible` 的并发调用不引入数据竞争。
TODO 这是在 cuda 中使用，这也太奇怪了吧

4. **原子栅栏（Fences）**：释放栅栏（release fence）与获取栅栏（acquire fence）或获取原子操作之间的 synchronizes-with 关系，要求涉及的每个操作（A、B、X、Y）所指定的作用域都必须**包含执行其他操作的线程**。

这些修改的实质是：CUDA C++ 将标准 C++ 的全局原子性假设，替换为**参数化的、作用域受限的原子性假设**。这意味着即使两个线程对同一个变量使用了 `atomic_ref`，如果它们分别属于不同设备，而原子操作的作用域仅为 `thread_scope_device`，那么从系统级视角看，这仍然可能构成数据竞争。

---

## 典型工作流程 / 调用顺序

以本章给出的消息传递（Message Passing）模式为例，典型的跨块通信流程如下：

### 阶段 1：初始化

在主机端分配设备内存：

```cpp
int *x, *f;
cudaMalloc(&x, sizeof(int));
cudaMalloc(&f, sizeof(int));
cudaMemset(x, 0, sizeof(int));  // x = 0
cudaMemset(f, 0, sizeof(int));  // f = 0
```

此处 `x` 为消息载荷，`f` 为标志位。两者都位于设备全局内存中。

### 阶段 2：写端执行（Block 0）

写端线程（如 block 0 的 thread 0）执行：

```cpp
*x = 42;
cuda::atomic_ref<int, cuda::thread_scope_device> flag(*f);
flag.store(1, cuda::std::memory_order_release);
```

关键点：
- `*x = 42` 是一个普通的非原子写操作。
- `flag.store(1, memory_order_release)` 不仅原子地写入标志位，还建立了释放语义：在此 store 之前的所有内存写操作（包括 `*x = 42`）不能被重排到此 store 之后。

### 阶段 3：读端执行（Block 1）

读端线程（如 block 1 的 thread 0）执行：

```cpp
cuda::atomic_ref<int, cuda::thread_scope_device> flag(*f);
while (flag.load(cuda::std::memory_order_acquire) != 1);
assert(*x == 42);
```

关键点：
- `flag.load(memory_order_acquire)` 具有获取语义：一旦读到了值为 1 的写入，该 load 之后的所有内存读操作（包括 `*x`）不能被重排到此 load 之前。
- 由于 release-acquire 配对，写端在 release 之前的写入对读端在 acquire 之后的读取可见。因此 `assert(*x == 42)` 不会失败。

### 阶段 4：错误变体（数据竞争）

如果读端或写端中任意一方不使用原子操作，或者使用 `memory_order_relaxed`，则可能发生数据竞争。例如，若读端直接以非原子方式读取 `f`：

```cpp
while (*f != 1);  // 数据竞争！与写端的 atomic store 冲突
```

此时，`f` 上存在两个并发冲突动作（一个原子 store，一个非原子 load），且非原子 load 的动作在 `thread_scope_device` 下不具备原子性，因此构成数据竞争，导致未定义行为。

---

## 关键限制、边界条件与兼容性

### 1. 线程作用域与硬件能力的匹配

并非所有作用域在所有场景下都有效。`thread_scope_system` 对系统分配内存、托管内存和映射内存的原子性依赖于多个设备属性，必须在运行时查询：

- `cudaDevAttrPageableMemoryAccess`
- `cudaDevAttrPageableMemoryAccessUsesHostPageTables`
- `cudaDevAttrConcurrentManagedAccess`
- `cudaDevAttrHostNativeAtomicSupported`

若代码在运行时检测到目标能力不存在，应避免使用系统级原子操作，改用显式的内存拷贝或 CUDA 流同步。

### 2. 多 GPU 场景下的原子性

对于位于某个 GPU 内存中的对象，若其他 GPU 的线程需要原子访问，必须满足以下之一：

- 源设备与目标设备之间的 `cudaDevP2PAttrNativeAtomicSupported` 为 1。
- 仅由单一 GPU 的线程并发访问。

如果不满足这些条件，跨 GPU 的原子操作不具备原子性，可能导致数据竞争。注意，这里的"单一 GPU"指的是并发访问的线程来源，而非对象所在的 GPU。

### 3. 16 字节原子操作的平台依赖

对自然对齐的 16 字节对象进行 load/store 在大多数平台上是原子的，但文档指出：如果 `hostNativeAtomicSupported == 0`，且操作在系统分配内存或映射内存上，则可能需要系统级支持。NVIDIA 表示目前未知有不支持的系统，但**没有 API 可查询此能力**。因此，在对可移植性要求极高的代码中，应谨慎依赖 16 字节的系统级原子 load/store。

### 4. 自然对齐要求

无论是原子操作还是栅栏配对，所有涉及的对象都必须是"自然对齐"的（naturally-aligned）。即 4 字节对象地址对齐到 4 字节边界，8 字节对象对齐到 8 字节边界。未对齐访问在 GPU 上可能导致总线错误或非原子行为。

### 5. 编译器与架构支持

CUDA C++ 内存模型扩展需要较新的 CUDA 工具包（建议 CUDA 11.4 以上）以及支持相应指令集的 GPU 架构（sm_60 及以上对大部分特性支持较好）。在较老的架构（如 sm_52 及以下）上，某些作用域受限的原子操作可能会静默降级为更宽作用域的操作，或导致编译错误。

### 6. ECC 错误与内核内代码

在嵌套内核启动（CDP, CUDA Dynamic Parallelism）的上下文中，**内核代码无法收到 ECC 错误的通知**。ECC 错误只在整个启动树（launch tree）完成后，在主机端报告。这意味着内核内部的错误处理逻辑不能依赖 ECC 状态查询；对于需要高可靠性的应用，应在主机端检查 CUDA API 返回的错误码，并在必要时重置设备或重新分配内存。

---

## 常见陷阱与调试建议

### 陷阱 1：混淆作用域层级

初学者容易认为 `thread_scope_device` 就是"设备内所有线程都可见"。实际上，它指的是"同一内存同步域内的设备线程"。在启用 MIG 或某些虚拟化配置下，一个物理 GPU 可能被划分为多个逻辑域。若代码假设跨所有 SM 的可见性，可能在特定配置下出现偶发的同步失败。

**建议**：在需要跨整个设备同步时，如果硬件配置不确定，优先使用显式的 `cudaDeviceSynchronize()` 或 CUDA 事件，而非依赖原子操作的设备级语义。

### 陷阱 2：在设备代码中混用 `std::` 与 `cuda::`

主机端编译器通常无法识别设备端的 `cuda::atomic_ref`，而设备端编译器（NVCC 的设备编译阶段）对 `std::atomic` 的支持有限。若在 `.cu` 文件中不加区分地混用二者，可能遇到"模板未实例化"或"函数未定义"的编译错误。

**建议**：在设备代码（`__global__` 或 `__device__` 函数）中，始终使用 `cuda::atomic_ref` 或 `cuda::std::atomic_ref`；在主机代码中，如果需要与设备端保持语义一致，可使用 `cuda::std::atomic`。

### 陷阱 3：忽略 `memory_order_relaxed` 的可见性风险

为了性能，开发者可能倾向于在所有原子操作中使用 `memory_order_relaxed`。这在计数器自增等场景下是安全的，但在消息传递、标志位同步等场景下会导致数据竞争：因为 `relaxed` 不建立 happens-before 关系，写端对普通内存的修改可能对读端不可见。

**建议**：仅在以下情况使用 `relaxed`：
- 操作是独立的计数器或统计值，读取时不需要看到其他内存的更新。
- 已经有其他更强的同步机制（如 `cudaDeviceSynchronize`）保证了可见性。

### 陷阱 4：对映射内存盲目使用系统级原子

当使用 `cudaHostRegister` 将主机内存映射到设备地址空间时，若 `hostNativeAtomicSupported == 0`，系统级原子操作可能不具备原子性。这在 x86_64 主机配合较老 GPU 时尤其常见。

**建议**：在程序初始化阶段查询 `cudaDevAttrHostNativeAtomicSupported`，若不支持，避免在映射内存上使用 `atomic_ref` 进行跨主机-设备的原子操作，改用 CUDA 流或事件进行同步。

### 陷阱 5：未初始化的标志位导致死循环

在消息传递模式中，如果标志位 `f` 的初始值未清零，或者写端因某种原因未执行，读端的 `while (flag.load(acquire) != 1)` 将无限循环。

**建议**：
- 始终用 `cudaMemset` 或初始化内核将标志位置为 0。
- 在生产代码中，为自旋循环添加超时机制或退避策略（backoff），避免无限占用 SM 资源。
- 考虑使用 `cuda::barrier` 替代自旋等待，因为 barrier 通常由硬件提供更高效的睡眠/唤醒机制。

---

## 一个最小可运行示例的说明

本章配套的 `chapter_demo.cu` 实现了一个完整的消息传递演示程序，并包含设备能力查询与兼容性报告。以下是该示例的设计思路与实现细节。

### 设计目标

1. **演示核心机制**：复现 5.7.5 节的消息传递模式，展示 `cuda::atomic_ref` 配合 `memory_order_release` / `memory_order_acquire` 在跨块通信中的用法。
2. **设备能力自检**：在主机端查询并打印 `pageableMemoryAccess`、`concurrentManagedAccess`、`hostNativeAtomicSupported` 等关键属性，帮助用户理解当前硬件对系统级原子操作的支持程度。
3. **sm_61 兼容性**：GTX 1060（Pascal 架构，sm_61）完整支持 `thread_scope_block` 和 `thread_scope_device` 的原子操作，但对系统级原子操作（`thread_scope_system`）的支持取决于主机平台。因此示例中：
   - 核心演示使用 `thread_scope_device`，确保在所有 sm_61 设备上都能正确运行。
   - 可选地展示 `thread_scope_system` 的查询结果，但不将其作为运行必要条件。
4. **避免不支持的特性**：
   - 不使用 16 字节原子操作（避免潜在的 sm_61 / 平台兼容性风险）。
   - 不使用 Managed Memory（除非用户显式启用），因为 `concurrentManagedAccess` 在部分 Pascal 系统上可能为 0。
   - 使用标准 `cudaMalloc` 分配设备内存，这是兼容性最高的方案。

### 内核设计

程序包含两个核函数：

- **`message_passing_kernel`**：核心演示内核。启动 2 个 block，每个 block 1 个线程。
  - Block 0 的线程执行写操作：先写入消息 `x = 42`，再通过 `atomic_ref` 以 `release` 语义设置标志位。
  - Block 1 的线程执行读操作：通过 `atomic_ref` 以 `acquire` 语义自旋等待标志位，读取到 1 后将 `x` 的值写入结果缓冲区。
  - 通过 `assert` 在设备端验证读取值是否为 42（在 release build 中 assert 可能被优化掉，因此主机端会再次验证）。

- **`simple_block_atomic_kernel`**：辅助演示内核。展示 `thread_scope_block` 的用法。单个 block 内的多个线程对共享计数器进行原子累加，验证块内原子操作的正确性。

### 主机端流程

1. 查询并打印 GPU 名称、SM 数量、计算能力。
2. 查询并打印与原子性相关的关键属性。
3. 根据属性值给出兼容性判断（如"支持系统分配内存的原子访问"、"不支持托管内存并发访问"等）。
4. 分配设备内存，运行核函数。
5. 将结果拷贝回主机，打印验证结果。
6. 释放资源。

### 降级与兼容处理

- 若 `cudaDevAttrPageableMemoryAccess == 0`，程序会打印警告，提示系统分配内存上的系统级原子操作不可用，但不会影响核心演示（因为核心演示使用设备内存 + `thread_scope_device`）。
- 所有 CUDA 运行时 API 调用均通过 `CUDA_CHECK` 宏包装，遇到错误时立即打印文件名、行号和错误描述并退出。
- Makefile 中指定 `-arch=sm_61`，确保生成 Pascal 架构指令；同时 `-std=c++17` 保证 `cuda::atomic_ref` 所需的语言特性可用。

### 编译与运行

```bash
make
./chapter_demo.out
```

预期输出包括设备信息、属性查询结果，以及类似以下内容：

```
[Message Passing] Result: 42 (expected: 42) -> PASS
[Block Atomic] Sum: 256 (expected: 256) -> PASS
```

若看到 `PASS`，说明当前平台的 `thread_scope_device` 原子操作与 release-acquire 语义工作正常。
