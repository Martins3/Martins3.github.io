## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cuda-cpp-execution-model.html>


本章（CUDA Programming Guide Release 13.2, Chapter 5.8 "CUDA C++ Execution model"）系统阐述了 CUDA C++ 对 C++ 标准中「执行与前进进度（execution and forward progress）」概念的扩展与修改。核心目标是为既有的 C++ 程序向 CUDA C++ 迁移时提供清晰的并行语义保证，使开发者能够明确判断：在何种条件下设备线程（device thread）一定会取得进展（make progress）、在何种条件下可能永远阻塞、以及 CUDA API 调用本身对设备线程启动和推进具有何种契约义务。

全章可分为五大板块：

1. **原子操作与内存作用域的引子**：通过一个 `thread_scope_block` 与 `thread_scope_device` 不匹配的典型错误，引出「作用域不足即构成数据竞争」的核心认知。
2. **前向进度（Forward Progress）的理论基础**：引用并解释 C++ 标准草案中的 `[intro.progress.7]`（并发前向进度）与 `[intro.progress.9]`（并行前向进度），为后续设备端保证奠定语义基础。
3. **Host 线程与 Device 线程的前向进度分级**：明确 host 线程的行为是 host 实现定义的；若 host 提供并发前向进度，则 CUDA C++ 为 device 线程提供并行前向进度。
4. **Device 线程的具体保证与限制**：这是本章最密集的部分，涵盖协作网格（Cooperative Grid）、线程块集群（thread-block cluster）中的进度传播规则，以及对 C++ 标准 `[intro.progress.1]` 的六项设备端修改。
5. **CUDA API 的前向进度义务与依赖关系**：规定 CUDA API 调用必须最终返回或确保至少一个设备线程取得进展；同时说明流（stream）依赖如何决定设备线程何时被允许启动。

---

## 背景与要解决的问题

### 为什么需要专门定义 CUDA C++ 执行模型？

传统的 C++ 标准假设程序运行在一组由操作系统调度的 CPU 线程上，这些线程具有较为成熟的前向进度保证（例如时间片轮转、优先级调度）。然而 GPU 的 SIMT 执行模型与 CPU 有本质差异：

- **线程数量悬殊**：一个 GPU kernel 可能同时启动数万个线程，硬件调度器不可能像 CPU 那样为每个线程独立提供细粒度抢占。
- **执行以 warp 为单位**：在旧架构中，一个 warp 内的线程以锁步（lock-step）方式执行，若其中某线程陷入无限循环，可能拖慢甚至阻塞整个 warp。
- **内存层次复杂**：设备线程可访问的内存包括寄存器、共享内存、L2、全局内存，以及通过 `cudaHostRegister` 映射的主机内存。不同作用域（scope）的原子操作决定了哪些线程能够合法地同步。
- **异构协同**：CUDA 程序通常由 host 线程（CPU）与 device 线程（GPU）共同完成计算，两者之间的同步语义必须精确约定，否则会出现「host 永远等不到 device 设置标志位」的死锁。

因此，NVIDIA 需要在 C++ 标准基础上做出明确扩展和修订，让开发者知道：

- 写一个 `while(!flag.load()) ;` 自旋在 GPU 上是否安全？
- 如果安全，需要满足哪些前提（作用域、存储期、API 调用方式）？
- `cudaDeviceSynchronize()` 到底对设备线程的启动和推进负有什么级别的义务？
- 为什么把数据放在共享内存（自动存储期）上做原子自旋可能触发未定义行为（UB）？

### 引子示例：作用域不匹配导致的数据竞争

章节开头给出的例子是理解后续所有规则的钥匙：

```cpp
int x = 0, f = 0;
// Thread 0 Block 0
x = 42;
cuda::atomic_ref<int, cuda::thread_scope_block> flag(f);
flag.store(1, memory_order_release); // UB: data race

// Thread 0 Block 1
cuda::atomic_ref<int, cuda::thread_scope_device> flag(f);
while(flag.load(memory_order_acquire) != 1); // UB: data race
assert(x == 42);
```

这里 `f` 上的 load/store 虽然分别通过 `atomic_ref` 完成，但 store 使用的是 `thread_scope_block`（块作用域）。由于执行 store 的线程属于 Block 0，该原子操作的「可见范围」仅覆盖 Block 0 内部；而执行 load 的线程位于 Block 1，不在 store 作用域之内。按照 PTX 内存一致性模型，这对操作不构成「原子地」可见的同步关系，从而引发数据竞争（data race），程序具有未定义行为。这个例子强调了：**原子性不仅取决于操作本身是否原子，还取决于作用域是否覆盖所有参与的执行代理。**

---

## 核心概念与术语

### 1. Forward Progress（前向进度）

C++ 标准将前向进度分为三个层次：

- **并发前向进度（concurrent forward progress）**：`[intro.progress.7]` — 实现必须保证该线程只要未终止，就最终会在有限时间内取得进展，无论其他线程是否在运行。
- **并行前向进度（parallel forward progress）**：`[intro.progress.9]` — 实现不保证线程在尚未执行任何步骤前一定会被启动；但一旦执行了至少一步，它就升级为并发前向进度保证。
- **弱并行前向进度**：标准未在 CUDA 文档中显式引用，但可理解为比并行更弱的保证（CUDA device 线程实际上得到的是并行前向进度，而非并发）。

CUDA C++ 的规则是：**如果 host 实现提供并发前向进度，则 CUDA C++ 为 device 线程提供并行前向进度。**

### 2. Cooperative Grid（协作网格）

当一个 kernel 以协作启动方式（cooperative launch）启动时，整个 grid 被显式标记为「协作的」。此时，device 线程的前向进度保证会扩散到整个 grid：只要 grid 内任何一个线程取得了进展，该 grid 内所有线程最终都会取得进展。

协作网格通常需要调用 `cudaLaunchCooperativeKernel` 或设置相应的启动配置，且对硬件和占用率（occupancy）有额外要求。

### 3. Thread-block Cluster（线程块集群）

这是从 Hopper 架构（sm_90）开始引入的概念。一个 cluster 包含多个 thread block，这些 block 可以被保证共同取得前向进度。对于非协作网格：

- 若架构支持 cluster，则一个线程取得进展可推导出「同 cluster 内所有线程最终都会进展」。
- 若架构不支持 cluster（如 sm_61 及更早），则 cluster 退化为单个 thread block。

本章明确规定：**非协作网格下，其他 cluster 中的线程不会被保证最终取得进展。**

### 4. Automatic Storage Duration（自动存储期）

在设备端，自动存储期指寄存器或栈上的变量（例如 kernel 内的局部变量、`__shared__` 变量是否属于自动存储期需要结合实现理解）。本章对 `[intro.progress.1]` 的修改中，设备线程被允许「对具有自动存储期的对象执行 volatile 访问或原子读操作」这一条被显式排除。换言之：

- 对自动存储期的对象做 `volatile` 访问，**不能**作为编译器判定「线程终将取得进展」的依据。
- 对自动存储期的对象做原子写操作，**也不能**让编译器假设线程不会永远自旋。

这是设备端与 host 端的一个关键语义差异，直接影响自旋锁应使用何种内存（全局内存或共享内存的静态存储期变量）。

### 5. Memory Scope（内存作用域）

CUDA 扩展了 C++ 原子操作的内存作用域：

- `cuda::thread_scope_thread`：仅对当前线程可见。
- `cuda::thread_scope_block`：对同一块内所有线程可见。
- `cuda::thread_scope_cluster`：对同 cluster 内所有线程可见。
- `cuda::thread_scope_device`：对同一 GPU 设备上所有线程可见。
- `cuda::thread_scope_system`：对整个系统（包括 CPU 和其他 GPU）可见。

本章引子示例已经说明：如果 store 与 load 的作用域不匹配，导致某个参与线程不在 store 的覆盖范围内，那么这对操作就不构成原子同步，从而产生数据竞争。

---

## API / 机制详解

### 5.8.1 Host 线程的前向进度

Host 线程的前向进度完全取决于 host 实现。对于运行 `main`、`std::thread`、`std::jthread` 的 host 线程：

- **通用 host 实现应当提供并发前向进度。** 这意味着在现代 Linux/Windows 系统上，可以合理假设 host 线程不会被无限期饿死。
- **device 线程的前向进度是 host 的下游保证**：只有当 host 提供并发前向进度时，CUDA C++ 才会进一步为 device 线程提供并行前向进度。

### 5.8.2 Device 线程的前向进度保证

#### 基本规则

一旦某个 device 线程取得了进展：

- **若它属于 Cooperative Grid**：该 grid 内所有 device 线程最终都必须取得进展。
- **否则**：该线程所在 thread-block cluster 内的所有 device 线程最终都必须取得进展。

注意：

- 这里「cluster」在非 Hopper 架构上等价于单个 thread block。
- 其他 cluster 中的线程**没有**这种保证。这意味着跨 cluster 或跨 block（非协作网格下）的自旋等待是不安全的，除非你通过 CUDA API 级别的同步（如 `cudaDeviceSynchronize`）来推进。

#### 对 `[intro.progress.1]` 的设备端修改

C++ 标准允许实现假设 host 线程最终会执行以下六种行为之一：终止、调用 `std::this_thread::yield`、调用库 I/O 函数、通过 volatile glvalue 访问、执行同步或原子操作、继续执行平凡无限循环。

CUDA 对 device 线程的假设做了以下修改（加粗部分）：

1. **终止** — 与 host 相同。
2. **调用库 I/O 函数** — 去掉了 `std::this_thread::yield`，device 线程不支持此调用。
3. **通过 volatile glvalue 访问** — 增加了限制：**除非所指对象具有自动存储期**。
4. **执行同步操作或原子读操作** — 增加了限制：**除非所指对象具有自动存储期**。同时明确排除了原子写操作和 fence 操作。

文档特别指出：部分限制是已知的实现缺陷，未来可能修复；但另一些是故意为之，例如允许原子写或 fence 作为进度依据会牺牲大量性能，而实际收益甚微。

#### 示例解读

本章给出了 5 个设备端示例（Execution.Model.Device.0 到 .4），它们精确展示了上述修改的含义：

- **Device.0**（安全）：全局/共享内存上的 `atomic_ref` 自旋。对象不具有自动存储期，因此 `device.threads.4` 适用，grid 最终会终止。
- **Device.1**（不安全）：`cuda::std::this_thread::yield()` 在无限循环中。Device 线程不支持 host.threads.2，因此允许没有任何线程取得进展。
- **Device.2**（不安全）：`volatile bool True = true; while(True);`。`True` 是自动存储期，因此 `device.threads.3` 的例外不适用，允许无进展。
- **Device.3**（不安全）：`cuda::atomic<bool, thread_scope_thread> True = true; while(True.load());`。同样是自动存储期对象上的原子读，允许无进展。
- **Device.4**（不安全）：`while(true) { /* empty */ }`。Device 线程不支持平凡无限循环假设（host.thread.6），允许无进展。

### 5.8.3 CUDA API 的前向进度保证

这是连接 host 与 device 的关键契约：

> **A CUDA API call shall eventually either return or ensure at least one device thread makes progress.**

也就是说，任何 CUDA API 调用都不能成为一个「黑洞」——它要么立即返回，要么在阻塞期间必须确保至少有一个设备线程在运行并取得进展。这条规则的推论包括：

- **查询函数**（如 `cudaStreamQuery`、`cudaEventQuery`）不能在没有设备线程推进的情况下永远返回 `cudaErrorNotReady`。它们必须「让步」于设备执行。
- **`cudaDeviceSynchronize`** 在设备为空时，也必须最终确保至少一个设备线程启动并取得进展（见 API.1 示例）。
- **API 调用所推进的线程不必与 API 参数相关**：操作 stream A 的 API 可能最终推进了 stream B 中的线程。

#### API 示例精析

- **API.1（安全终止）**：`hello_world<<<1,2>>>()` 后接 `cudaDeviceSynchronize()`。由于 device 为空时 `cudaDeviceSynchronize` 必须确保至少一个线程启动，而线程启动后同 block 内其他线程也会启动，最终所有线程到达 `__syncthreads()` 并退出，程序终止。
- **API.2（可能死锁）**：host 自旋等待 device 设置 flag，但 `cudaDeviceSynchronize()` 仅在 flag 被设置后才会调用。CUDA 只保证同步 API 被调用时才会推进线程，因此 host 可能永远等不到 flag。
- **API.3（单次查询不足）**：在自旋中仅调用一次 `cudaStreamQuery(0)`，之后进入纯 host 自旋。单次查询 API 不足以保证 device 线程启动，因此仍可能死锁。
- **API.4（安全）**：在自旋循环内**反复**调用 `cudaStreamQuery(0)`。重复的查询 API 调用满足「API 必须最终推进某个 device 线程」的义务，因此 device 线程最终会启动并设置 flag，程序终止。

### 5.8.3.1 依赖关系（Dependencies）

设备线程在**所有依赖完成前不得启动**。依赖通常通过 CUDA Stream 命令建立，例如：

- `cudaStreamWaitEvent`
- 同一 stream 中按序执行的 kernel 启动（隐式依赖前一个 kernel 完成）
- 显式图（CUDA Graph）中的依赖边

#### Stream 示例精析

- **Stream.0（可能死锁）**：`first` kernel 写入 flag，`second` kernel 自旋读 flag，两个 kernel 分别提交到 stream `s0` 和 `s1`。由于没有依赖关系，CUDA 调度器可能永远只推进 `second` 的线程，导致 `first` 被饿死，flag 永远为 0。
- **Stream.1（安全终止）**：两个 kernel 提交到**同一个** stream `s0`。由于 stream 内命令按 FIFO 顺序执行，`first` 一定在 `second` 之前完成。因此 flag 一定会在 `second` 开始自旋前被写入，程序终止。

---

## 典型工作流程 / 调用顺序

基于本章语义，一个正确利用前向进度保证的 host-device 协作流程应如下：

1. **分配内存**：对于需要 host-device 原子同步的场景，使用 `cudaMallocHost` 或 `cudaHostAlloc` 分配页锁定（page-locked）主机内存，并通过 `cudaHostGetDevicePointer` 获取设备端可访问的指针。
2. **初始化标志**：在 host 上将原子标志置为 0。
3. **启动 device kernel**：通过 `<<< >>>` 语法或 `cudaLaunchCooperativeKernel` 启动 kernel。
4. **Host 侧等待**：
   - 若使用忙等待（spin-loop），**必须在循环体内反复调用 CUDA 查询 API**（如 `cudaStreamQuery(0)`），不能是纯 CPU 自旋。
   - 若使用阻塞等待，直接调用 `cudaDeviceSynchronize()` 或 `cudaStreamSynchronize()`。
5. **Device 侧推进**：
   - 同 block 内的线程同步应使用 `__syncthreads()` 或 block-scope 原子操作。
   - 跨 block 同步必须使用协作启动（cooperative grid）或返回 host 后重新调度。
6. **清理与验证**：同步完成后读取结果，验证原子标志和数据一致性，释放内存。

---

## 关键限制、边界条件与兼容性

### 1. 架构差异（sm_61 vs sm_90+）

- **Thread-block cluster**：仅 sm_90（Hopper）及以上支持。在 sm_61 上，cluster 语义退化为单个 thread block。这意味着跨 block 的前向进度保证在非协作网格下**不存在**。
- **Cooperative Grid**：sm_61 支持协作启动，但需要满足最大占用率限制，且需使用 `cudaLaunchCooperativeKernel` API。普通 `<<< >>>` 启动的 grid 不属于 Cooperative Grid。
- **原子操作与系统内存**：sm_61 支持 `thread_scope_system`，但要求相关主机内存必须通过 `cudaMallocHost`/`cudaHostRegister` 页锁定并映射到设备地址空间。直接传递栈变量指针到 device 会导致非法内存访问。

### 2. 自动存储期限制

在 device 线程中：

- 不要对局部变量（自动存储期）执行 `volatile` 读取并期望它构成前向进度依据。
- 不要对局部变量执行原子读并期望它构成前向进度依据。
- 不要在局部变量上做原子写或 memory fence 并期望编译器或调度器将其视为「线程终将进展」的信号。

安全的做法是使用全局内存、`__device__` 静态变量，或 `__shared__` 变量（需注意具体编译器版本对其存储期的处理，但通常共享内存被视为块级共享，可用于进度依据）。

### 3. API 调用与忙等待

Host 侧对 device 结果的忙等待必须遵循以下规则之一：

- 使用 `cudaDeviceSynchronize()` 阻塞等待。
- 或者在自旋循环中**反复调用** `cudaStreamQuery()` / `cudaEventQuery()` 等查询 API。

单次查询后接纯 host 自旋（如 API.3 示例）仍然可能死锁，因为 CUDA 运行时没有义务在查询返回后持续推进 device 线程。

### 4. 作用域一致性

使用 `cuda::atomic_ref` 或 `cuda::atomic` 时：

- 所有参与同步的线程必须选择一个能够覆盖彼此的作用域。
- Block 0 用 `thread_scope_block` 写 flag，Block 1 用任何作用域读，都是 UB。
- 跨 grid 或跨设备同步必须使用 `thread_scope_device` 或 `thread_scope_system`。

---

## 常见陷阱与调试建议

### 陷阱 1：错误的作用域选择

很多开发者误以为「原子操作就是线程安全的」，而忽略了作用域。在共享内存上使用 `thread_scope_block` 是自然的，但如果后续需要在 host 或其他 block 中观测该值，必须改用 `thread_scope_device` 或 `thread_scope_system`，并将数据放在全局内存或映射的主机内存中。

### 陷阱 2：自动存储期自旋

在 kernel 内写如下代码：

```cpp
__global__ void bad() {
    cuda::atomic<bool, cuda::thread_scope_thread> done(false);
    if (threadIdx.x == 0) done.store(true);
    while (!done.load()); // 在自动存储期上自旋，UB
}
```

这在语义上是不安全的。应将 `done` 改为 `__shared__` 或全局内存变量。

### 陷阱 3：Host 纯自旋等待 Device

```cpp
kernel<<<1,1>>>();
while (!host_flag); // 没有 CUDA API 调用，可能死锁！
```

正确做法是在循环内加入 `(void)cudaStreamQuery(0);`，或者直接使用 `cudaDeviceSynchronize()`。

### 陷阱 4：不同 Stream 上的隐式依赖假设

```cpp
first<<<1,1,0,s0>>>();
second<<<1,1,0,s1>>>(); // second 依赖 first？不依赖！
```

如果需要执行顺序，必须使用同一个 stream 或显式 `cudaStreamWaitEvent`。

### 调试建议

NVIDIA 在文档中提供了一个**不充分但有用**的测试方法：

```bash
export CUDA_DEVICE_MAX_CONNECTIONS=1
export CUDA_LAUNCH_BLOCKING=1
```

在此环境下运行程序，如果程序仍不终止，则说明存在前向进度缺陷。此方法能捕获很多常见错误，但并非全部，不应作为唯一测试手段。

此外：

- 使用 `cuda-memcheck` / `compute-sanitizer` 检测数据竞争。
- 在 kernel 内使用 `printf` 或 Nsight 跟踪线程执行时间线，确认自旋线程是否被饿死。
- 对于系统内存原子，务必检查 `cudaHostGetDevicePointer` 返回值是否为有效设备指针。

---

## 一个最小可运行示例的说明

本项目提供了 `chapter_demo.cu` 及配套 `Makefile`，目标架构为 `sm_61`（GTX 1060，CUDA 12.8）。该示例分为三个子演示，分别对应本章不同侧面的核心概念。

### 示例设计动机

本示例没有选择 Hopper 特有的 thread-block cluster 或协作网格同步（因为 sm_61 不支持 cluster，且协作启动需要额外的占用率检查），而是聚焦于 sm_61 完全支持且在实际开发中最易出错的三个场景：

1. **同 block 内原子同步的正确写法**（对应 Execution.Model.Device.0）。
2. **`__syncthreads()` 的协作屏障语义**（对应 API.1 中的 block 内同步逻辑）。
3. **Host 自旋 + 查询 API 的前向进度模式**（对应 Execution.Model.API.4）。

### 编译与运行

```bash
make        # 编译生成 chapter_demo.out
make run    # 编译并运行
```

编译参数固定为：

```makefile
nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2
```

由于 sm_61 在 CUDA 12.8 中属于已弃用目标，Makefile 额外加入了 `-Wno-deprecated-gpu-targets` 以抑制警告。

### Demo 1：Block-scope 原子同步

```cpp
__global__ void block_forward_progress_kernel(int* result) {
    __shared__ int shared_flag;
    __shared__ int shared_value;
    // ...
    cuda::atomic_ref<int, cuda::thread_scope_block> flag(shared_flag);
    if (threadIdx.x == 0) {
        shared_value = 42;
        flag.store(1, cuda::memory_order_release);
    } else if (threadIdx.x == 1) {
        while (flag.load(cuda::memory_order_acquire) == 0);
        result[0] = shared_value;
    }
    __syncthreads();
}
```

**演示点**：

- `shared_flag` 和 `shared_value` 是 `__shared__` 变量，不属于自动存储期，因此 device 线程的前向进度保证适用于它们。
- Thread 0 与 Thread 1 处于同一块内，`thread_scope_block` 完全覆盖二者。
- 使用 `memory_order_release` / `memory_order_acquire` 配对，保证 `shared_value = 42` 对 Thread 1 可见。
- 由于 CUDA 保证同 block 内线程最终都会取得进展，Thread 0 一定会执行到 store，Thread 1 的自旋一定会终止。

### Demo 2：协作屏障 `__syncthreads()`

```cpp
__global__ void cooperative_barrier_kernel(int* result) {
    __shared__ int shared_buf[64];
    int tid = threadIdx.x;
    shared_buf[tid] = tid * 10;
    __syncthreads();
    int neighbor = shared_buf[(tid + 1) % blockDim.x];
    result[tid] = neighbor;
}
```

**演示点**：

- 每个线程先写入自己的共享内存槽位。
- `__syncthreads()` 作为 block 级屏障，确保所有写入在后续读取前已经完成。
- 每个线程读取邻线程的值并写回全局内存，用于 host 验证数据一致性。
- 这体现了 API.1 示例中「所有线程到达屏障后，等待线程被解封」的机制。

### Demo 3：API 前向进度与系统作用域原子

```cpp
__global__ void producer_kernel(int* flag_ptr) {
    cuda::atomic_ref<int, cuda::thread_scope_system> flag(*flag_ptr);
    flag.store(1, cuda::memory_order_relaxed);
}
```

Host 侧：

```cpp
cudaMallocHost(&h_flag, sizeof(int));
cudaHostGetDevicePointer(&d_flag, h_flag, 0);
producer_kernel<<<1, 1>>>(d_flag);
while (*h_flag == 0) {
    (void)cudaStreamQuery(0); // 关键：重复查询 API
}
```

**演示点**：

- `thread_scope_system` 允许 device kernel 直接写 host 可见的页锁定内存。
- sm_61 虽不支持最新统一内存的所有特性，但通过 `cudaMallocHost` + `cudaHostGetDevicePointer` 映射的内存可以被 device 原子访问。
- Host 自旋循环内**重复调用** `cudaStreamQuery(0)`，这正是 Execution.Model.API.4 所展示的正确模式。如果不加此调用，host 可能永远等不到 flag（对应 API.3 的错误模式）。

### sm_61 兼容性处理

- **Thread-block cluster**：代码在运行时会检测 `prop.major >= 9`，在 sm_61 上打印「not supported (sm_61)」，提示用户 cluster 语义退化为 block 级别。
- **协作网格**：未在示例中使用 `cudaLaunchCooperativeKernel`，因为演示的重点是 block 级和 host-device 同步，而非 grid 级同步。若需扩展，可使用条件编译或运行时检测。
- **系统内存原子**：使用 `cudaMallocHost` 而非 `cudaMallocManaged`，因为 sm_61 上的 Managed Memory 原子语义在旧驱动上行为不够稳定，而显式页锁定映射是最保守、最兼容的做法。

---

## 小结

CUDA C++ 执行模型并非简单地将 C++ 线程语义移植到 GPU，而是在硬件约束与性能需求之间做出了精确的折中。理解以下几条主线，即可避免绝大多数并发缺陷：

1. **作用域决定原子性**：原子操作的作用域必须覆盖所有参与线程，否则即使语法正确也是 UB。
2. **前向进度有边界**：非协作网格下，前向进度只保证到 thread-block cluster（sm_61 上即单个 block）。跨 block 忙等待必须使用 host 介入或协作启动。
3. **自动存储期是陷阱**：device 线程不能以自动存储期对象的 volatile/atomic 访问作为前向进度依据。
4. **API 调用是推进器**：host 自旋等待 device 时，要么用同步 API 阻塞，要么在循环中反复调用查询 API，切勿纯 CPU 空转。
5. **流依赖即执行序**：不同 stream 之间没有隐式顺序，必须通过 event 或同 stream 提交来建立依赖。

这些规则看似琐碎，但它们是 GPU 程序从「偶尔正确」走向「始终正确」的关键基石。
