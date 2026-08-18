## 章节概述

原文链接：<https://docs.nvidia.com/cuda/cuda-programming-guide/05-appendices/cpp-language-support.html>


本章系统阐述了 NVIDIA CUDA 编译器（nvcc）对 C++ 语言的支持范围与限制。内容横跨 C++03 到 C++20 的设备代码支持矩阵，涵盖 libcu++ 标准库实现、设备端 C 标准库函数（printf、malloc、clock 等）、Lambda 表达式（特别是扩展 Lambda）的完整语义与限制、多态函数包装器 `nvstd::function`，以及变量、函数、类、模板层面的语言级约束。此外，本章还针对 C++11/14/17/20 各版本在 CUDA 中的特殊限制进行了逐项说明。对于希望在 GPU 内核中运用现代 C++ 抽象的开发者而言，本章既是能力清单，也是禁区地图。

---

## 背景与要解决的问题

GPU 异构编程面临的核心矛盾在于：C++ 语言及其标准库最初为同构主机环境设计，而 CUDA 程序必须同时运行在主机（CPU）和设备（GPU）两个执行空间中。这两个空间拥有独立的地址空间、内存模型和 ABI，且设备端缺乏完整的操作系统运行时（如异常处理、动态链接、虚拟内存映射）。因此，CUDA 编译器需要解决以下问题：

1. **标准版本对齐**：主机编译器与设备编译器必须就 C++ 方言达成一致，否则模板实例化、名字查找和重载解析会出现分歧。
2. **执行空间隔离**：哪些函数和变量可以在 host 端调用、哪些可以在 device 端调用，需要显式或隐式的标注机制。
3. **跨空间抽象传递**：开发者希望将 lambda、函数对象、模板参数等高级抽象从 host 代码传入 device 内核，但这涉及闭包类型的布局、捕获变量的生命周期和地址可访问性。
4. **设备端运行时支持**：设备代码需要基本的输入输出（printf）、动态内存（malloc/free）、时间测量（clock）和字符串/内存操作（memcpy/memset），但这些操作的语义与主机端存在差异（例如 printf 使用主机端格式化、malloc 从设备堆分配）。
5. **架构兼容与降级**：不同计算能力（compute capability）的硬件对指令集和内存模型的支持不同，语言特性必须在编译期和运行期都有明确的兼容性边界。

---

## 核心概念与术语

- **Execution Space（执行空间）**：CUDA 通过 `__host__`、`__device__`、`__global__` 和 `__host__ __device__` 修饰符界定代码的运行位置。`__global__` 函数由 host 调用、在 device 上执行；`__device__` 函数仅在 device 上执行；`__host__ __device__` 函数则可在两侧编译和调用。
- **Extended Lambda（扩展 Lambda）**：标准 C++ lambda 的执行空间由最内层外围函数隐式决定（host 函数内的 lambda 为 `__host__`）。CUDA 的 `--extended-lambda` 标志允许在 lambda 引入符后显式标注 `__device__` 或 `__host__ __device__`，使其能作为 `__global__` 函数模板的类型参数传递。
- **Closure Type（闭包类型）**：lambda 表达式编译器生成的匿名类。扩展 lambda 的闭包类型在 host 编译阶段会被 CUDA 前端替换为一个占位类型（placeholder type），这会影响类型萃取和 ADL。
- **libcu++**：CUDA 提供的 C++ 标准库实现，位于 `<cuda/std/...>` 路径下。它在 host 和 device 两侧均可用，支持 C++17 向 C++20/23/26 的特性回移植，并针对设备代码高度优化。
- **Whole Program Compilation / Separate Compilation**：前者不允许 `extern __device__` 等外部链接变量定义（除动态 `__shared__` 外）；后者（`-rdc=true`）允许跨翻译单元的外部链接。
- **Device Heap**：设备端 `malloc`/`free`/`new`/`delete` 使用的内存池，默认大小为 8 MB，必须通过 `cudaDeviceSetLimit(cudaLimitMallocHeapSize)` 在首次使用前显式设置，且加载模块后不可动态调整。

---

## API / 机制详解

### 1. C++ 标准版本支持

nvcc 通过 `--std=c++03/11/14/17/20` 标志启用对应标准。该标志不仅作用于设备前端，也会将等价的 C++ 方言选项传递给主机预处理器、编译器和链接器。各版本的关键设备代码支持如下：

- **C++11**：引入了大量现代 C++ 基础能力，包括右值引用、变参模板、lambda、自动类型推导、`constexpr`、强类型枚举、`nullptr`、原子操作和线程本地存储等。CUDA Toolkit 7.x 起基本完整支持。
- **C++14**：增加了变量模板、泛型 lambda、二进制字面量、单引号数字分隔符、`[[deprecated]]` 属性、放宽的 `constexpr` 函数限制等。CUDA Toolkit 9.x 起支持。
- **C++17**：引入结构化绑定、`if constexpr`、折叠表达式、`inline` 变量、类模板参数推导、`std::byte`、`[[nodiscard]]`/`[[fallthrough]]`/`[[maybe_unused]]` 等属性。CUDA Toolkit 11.x 起支持。
- **C++20**：支持 Concepts、`consteval`、三路比较运算符 `<=>`、指定初始化、模块、协程等。CUDA Toolkit 12.x 起支持，但需要 GCC >= 10.0、Clang >= 10.0 或 MSVC >= 2022 等主机编译器配合。

### 2. libcu++（CUDA C++ Standard Library）

libcu++ 是 CUDA 对 C++ 标准库的重实现，核心价值在于：

- **跨空间可用**：同一套 API 在 host 和 device 均可调用，避免了在 device 代码中直接调用 `std::` 可能触发的未定义行为（因为主机 STL 实现可能使用异常、RTTI 或平台相关 ABI）。
- **扩展数据类型**：原生支持 `__int128`、半精度浮点 `__half`、Bfloat16 `__nv_bfloat16` 和四精度 `__float128`。
- **向后移植**：在 C++17 模式下即可使用部分 C++20/23/26 的库特性。
- **性能优化**：device 端的算法、原子操作、同步原语和容器扩展均针对 GPU 的 SIMT 架构做了专门优化。

建议将 device 代码中的 `std::memcpy`、`std::clock`、`std::chrono`、`std::move`、`std::forward` 替换为 `cuda::std::` 对应版本，以确保语义确定性和跨平台兼容性。

### 3. 设备端 C 标准库函数

#### 3.1 `clock()` 与 `clock64()`

```cpp
__host__ __device__ clock_t clock();
__device__ long long clock64();
```

在 device 代码中，这两个函数返回每多处理器（SM）的周期计数器，每个时钟周期递增一次。开发者可以在内核开始和结束时采样并相减，估算线程占用的时钟周期数。但需注意：由于 GPU 采用时间片调度线程，该数值大于线程实际执行指令所需的周期数。更精确的计时建议使用 `cuda::std::chrono` 提供的可移植接口。

#### 3.2 `printf()`

```cpp
int printf(const char* format[, arg, ...]);
```

device 端的 `printf` 将格式化后的参数和格式字符串通过固定大小的循环缓冲区传递到 host，由 host 的 C 库完成最终格式化输出。关键语义：

- **返回值**：返回成功解析的参数个数（不是输出字符数）。格式字符串为 NULL 返回 -1，内部错误返回 -2。
- **参数限制**：最多接受 32 个参数，超出部分被静默忽略，格式说明符原样输出。
- **格式说明符**：支持 `#`, `' '`, `0`, `+`, `-` 等标志；宽度 `*`, `0-9`；精度 `0-9`；长度修饰符 `h`, `l`, `ll`；类型 `%cdiouxXpeEfgGaAs`。
- **平台差异**：`long` 在 Windows 上为 32 位，在 Linux 上为 64 位，跨平台编译时 `%ld` 可能导致输出损坏。
- **缓冲区管理**：默认 1 MB 循环缓冲区。仅在以下时刻刷新：内核启动开始时、`cudaDeviceSynchronize()` 等同步点、阻塞内存拷贝、模块加载/卸载、上下文销毁、流回调执行前。程序退出时不会自动刷新，因此必须在退出前显式同步。
- **线程执行顺序**：`printf` 内部使用共享数据结构，可能改变线程执行路径长度，但 CUDA 除 `__syncthreads()` 外不保证线程执行顺序，因此输出顺序本身是不确定的。

#### 3.3 `memcpy()` 与 `memset()`

```cpp
__host__ __device__ void* memcpy(void* dest, const void* src, size_t size);
__host__ __device__ void* memset(void* ptr, int value, size_t size);
```

两者在 host 和 device 均可使用，但 `memset` 的 `value` 按 `unsigned char` 解释。建议使用 `cuda::std::memcpy` 和 `cuda::std::memset` 作为更安全、语义更明确的替代。

#### 3.4 `malloc()` 与 `free()`

```cpp
__host__ __device__ void* malloc(size_t size);
__device__ void* __nv_aligned_device_malloc(size_t size, size_t align);
__host__ __device__ void free(void* ptr);
```

- `malloc` 从设备堆分配至少 `size` 字节，返回的指针 16 字节对齐；失败返回 NULL。
- `__nv_aligned_device_malloc` / `cuda::std::aligned_alloc` 支持自定义对齐，对齐值必须是非零的 2 的幂。
- `free` 释放由上述分配函数或 `new` 返回的内存。重复释放同一指针导致未定义行为；释放 NULL 被忽略。
- **生命周期**：通过 device 端 `malloc` 分配的内存生命周期持续到 CUDA context 销毁，或显式 `free` 为止。它可以被同一 context 中的其他线程（包括后续内核启动的线程）访问。任何线程均可释放其他线程分配的内存。
- **互操作性**：device 端 `malloc`/`free` 分配的内存不能通过 `cudaMalloc`/`cudaMemcpy`/`cudaMemset` 等 host API 使用或释放，反之亦然。
- **堆大小设置**：必须在首次使用设备堆分配（包括 `new`/`delete`）之前，通过 `cudaDeviceSetLimit(cudaLimitMallocHeapSize, size)` 设置。若未显式设置，默认分配 8 MB。模块加载后堆大小不可更改，也不会按需动态扩展。

#### 3.5 `alloca()`

```cpp
__host__ __device__ void* alloca(size_t size);
```

在调用者栈帧内分配内存，返回 16 字节对齐的指针。函数返回时自动释放。Windows 平台需包含 `<malloc.h>`。需注意栈溢出风险，必要时调整栈大小。

### 4. Lambda 表达式

#### 4.1 标准 Lambda 的执行空间推导

lambda 的执行空间默认由最内层外围函数的执行空间决定：

- 在 `__host__` 函数内定义的 lambda 为 `__host__`。
- 在 `__device__` 函数内定义的 lambda 为 `__device__`。
- 在 `__global__` 函数内定义的 lambda 为 `__device__`。
- 在 `__host__ __device__` 函数内定义的 lambda 为 `__host__ __device__`。
- 全局/命名空间作用域的 lambda 为 `__host__`。

标准 lambda 不能用作 `__global__` 函数的模板类型参数。

#### 4.2 扩展 Lambda（Extended Lambda）

通过 `--extended-lambda` 标志启用。允许在 lambda 引入符后显式标注执行空间：

- `[] __device__ {}`：扩展设备 lambda。
- `[] __host__ __device__ {}`：扩展 host-device lambda。
- `[] __host__ {}`：不是扩展 lambda（显式标注必须包含 device 侧才被视为扩展）。

扩展 lambda 必须是“局部”的：定义在 `__host__` 或 `__host__ __device__` 函数的块作用域内（或嵌套块中）。全局作用域或纯 `__device__` 函数内的 lambda 即使有标注也不是扩展 lambda。

关键能力：扩展 lambda 可以作为类型参数传递给 `__global__` 函数模板。这是标准 lambda 无法实现的。

#### 4.3 扩展 Lambda 类型特征

nvcc 提供编译期类型特征用于检测闭包类型：

- `__nv_is_extended_device_lambda_closure_type(type)`：是否为扩展 `__device__` lambda 的闭包类型。
- `__nv_is_extended_host_device_lambda_closure_type(type)`：是否为扩展 `__host__ __device__` lambda 的闭包类型。
- `__nv_is_extended_device_lambda_with_preserved_return_type(type)`：对于扩展 `__device__` lambda，如果使用了尾随返回类型（trailing return type）且该返回类型不引用 lambda 参数名，则返回 true。此时 host 代码可以正确萃取 `operator()` 的返回类型。

#### 4.4 扩展 Lambda 的 18 条限制（核心摘要）

1. 不能嵌套在另一个扩展 lambda 内。
2. 不能定义在泛型 lambda（带 `auto` 参数的 lambda）内。
3. 若嵌套在多层 lambda 中，最外层 lambda 必须定义在函数作用域内（不能是纯 lambda 嵌套）。
4. 外围函数必须具名且地址可访问。若外围函数是类成员，则该类必须具名，且成员和外围类均不能是 private/protected。
5. 外围函数的地址必须能无歧义地被取到（不能被类型别名遮蔽模板参数名等）。
6. 不能定义在局部于函数的类中。
7. 外围函数不能是返回类型推断的（`auto` 返回类型）。
8. `__host__ __device__` 扩展 lambda 不能是泛型 lambda（不能有 `auto` 参数）。
9. 若外围函数是模板实例化，模板参数必须具名，最多一个变参包且必须位于最后；实例化参数不能涉及局部类型或 private/protected 类成员。
10. MSVC 主机编译器下，外围函数必须具有外部链接（external linkage）。
11. MSVC 主机编译器下，不能在 `if constexpr` 块内定义扩展 lambda。
12. 捕获限制：
    - 只能按值捕获（不能 `[&a]`）。
    - 数组类型变量可捕获，但维度不能超过 7；数组元素类型必须默认可构造且可复制赋值。
    - 不能捕获变参包中的参数。
    - 不能捕获局部类型变量（扩展 lambda 闭包类型除外）或 private/protected 类成员。
    - `__host__ __device__` 扩展 lambda 不支持 init-capture；`__device__` 扩展 lambda 支持 init-capture，但初始化器不能是数组或 `std::initializer_list`。
    - 扩展 lambda 的 `operator()` 不能是 `constexpr` 或 `consteval`；闭包类型不是字面类型。
    - `if constexpr` 块内不能隐式捕获变量，除非该变量已在块外被隐式捕获或出现在显式捕获列表中。
13. 函数内扩展 lambda 的数量和顺序不能依赖于 `__CUDA_ARCH__` 宏的定义或取值，因为 CUDA 前端用计数器为每个扩展 lambda 生成占位类型名。
14. 对于扩展 device-only lambda，host 代码中不能直接萃取 `operator()` 的返回类型或参数类型，除非 `__nv_is_extended_device_lambda_with_preserved_return_type()` 为 true。device 代码内萃取则不受限。
15. 同 14，针对参数类型和返回类型的 device/host 侧萃取限制。
16. 若扩展 lambda 从 host 传递到 device（如作为 `__global__` 参数），其体内捕获变量的表达式不能因 `__CUDA_ARCH__` 而异，否则闭包类布局在 host 编译和 device 编译中可能不一致，导致运行时错误。
17. 扩展 device-only lambda 在 host 代码中不提供到函数指针的转换运算符（device 代码中提供）。`__host__ __device__` 扩展 lambda 不受此限。
18. 由于占位类型可能定义特殊的成员函数，部分标准类型特征（`std::is_trivially_copyable`、`std::is_trivially_constructible` 等）在 CUDA 前端和主机编译器中的结果可能不同。不得将这些特征的结果用于实例化 `__global__`、`__device__`、`__constant__` 或 `__managed__` 的函数/变量模板。

#### 4.5 `*this` 按值捕获

C++11/C++14 中，lambda 在非静态成员函数内引用成员变量时，默认按值捕获 `this` 指针。若该 lambda 在 GPU 上执行而 `this` 指向 host 内存，将触发运行时段错误。CUDA 支持 C++17 的 `[=, *this]` 捕获模式：复制 `*this` 对象本身而非捕获指针。该模式适用于 `__device__` 和 `__global__` 函数内的 lambda，以及 host 代码中的扩展 device-only lambda。非标注 lambda和扩展 host-device lambda 仅在语言方言支持时才允许使用 `*this` 捕获。

#### 4.6 参数依赖查找（ADL）副作用

扩展 lambda 被替换为占位类型时，占位类型的模板参数包含外围函数的地址。这可能导致外围函数所在命名空间意外参与 ADL，从而在 host 编译阶段引发调用歧义。

### 5. 多态函数包装器 `nvstd::function`

定义于 `<nvfunctional>`，可在 host 和 device 代码中存储、复制和调用任意可调用目标（包括 lambda）。

- host 代码中的 `nvstd::function` 不能用 `__device__` 函数或仅含 `__device__ operator()` 的函数对象初始化。
- device 代码中的 `nvstd::function` 不能用 `__host__` 函数或仅含 `__host__ operator()` 的函数对象初始化。
- `nvstd::function` 实例不能在运行时在 host 和 device 之间传递。
- `nvstd::function` 不能作为从 host 代码启动的 `__global__` 函数的参数类型。

### 6. C/C++ 语言限制

#### 6.1 不支持特性

- **RTTI 与异常**：`typeid`、`dynamic_cast`、`try`/`catch`/`throw` 在 device 代码中不支持。
- **long double**：device 代码不支持。
- **Trigraphs 与 Digraphs**：Trigraphs 全平台不支持；Windows 上不支持 Digraphs。
- **用户自定义 operator new/delete**：不能替换编译器内置的全局内存分配函数，否则在 host 和 device 上均为未定义行为。

#### 6.2 命名空间保留

向顶级命名空间 `cuda::`、`nv::`、`cooperative_groups::` 或其任何嵌套命名空间添加定义属于未定义行为。但允许将 `cuda::` 等作为非保留命名空间的子命名空间使用。

#### 6.3 指针与内存地址

指针解引用（`*p`、`p->member`、`p[0]`）只允许在内存所在执行空间进行。以下行为未定义：

- 在 host 上解引用指向 global/shared/constant 内存的指针。
- 在 device 上解引用指向 host 内存的指针。

函数地址限制：

- host 代码中不能取 `__device__` 函数的地址。
- host 中取的 `__global__` 函数地址不能在 device 中使用，反之亦然。
- 通过 `cudaGetSymbolAddress()` 获取的 `__device__`/`__constant__` 变量地址只能在 host 代码中使用。

#### 6.4 变量规则

- **局部变量**：`__host__` 函数内不允许出现 non-extern 的 `__device__`/`__shared__`/`__managed__`/`__constant__` 声明。`__device__` 函数内不允许出现 non-extern 且 non-static 的 `__constant__`/`__managed__` 声明。
- **const 变量**：未加内存空间修饰符的 `const` 全局/命名空间/类作用域变量默认为 host 变量。device 代码不能直接取地址或引用它，但若其用常量表达式初始化、类型非 volatile、且为内置整型/浮点型（MSVC 除外），则可直接按值使用。C++14 以后建议使用 `constexpr` 或 C++17 的 `inline constexpr`。
- **volatile**：CUDA 保留 `volatile` 仅为了兼容 ISO C++，但**不适合**用于线程同步（应使用原子操作）或内存映射 I/O（应使用内联 PTX `st.mmio`/`ld.mmio`）。`volatile` 读写不保证原子性、内存操作顺序，也不保证硬件实际执行的内存访问次数与 PTX 指令数一致。
- **static 变量**：允许在 `__global__`/`__device__`-only 函数内使用。在 `__host__ __device__` 函数内，无显式内存空间的 static 变量隐式推导；显式带 `__device__`/`__constant__`/`__shared__`/`__managed__` 的 static 变量仅在 `__CUDA_ARCH__` 已定义时允许。同一 `__host__ __device__` 函数内的 static 变量在 host 和 device 侧持有不同实体。
- **extern 变量**：Whole Program Compilation 模式下，不能用 `extern` 定义 `__device__`/`__managed__`/`__constant__` 变量（动态 `__shared__` 除外）。Separate Compilation 模式下允许。

#### 6.5 函数规则

- **递归**：`__global__` 函数不支持递归；`__device__` 和 `__host__ __device__` 函数允许递归。
- **外部链接**：Separate Compilation 模式下，跨翻译单元引用的 `__device__`/`__global__` 函数的参数和返回类型必须是完整类型（满足 ODR-use）。
- **形参内存空间**：`__device__`/`__shared__`/`__managed__`/`__constant__` 不能用于函数形参。
- **`__global__` 参数限制**：
  - 不能用 C 风格变参（`...` 和 `va_list`）；C++11 变参模板允许，但只能有一个参数包且必须位于最后。
  - 参数总大小不超过 32,764 字节，通过 constant memory 传递。
  - 不能按引用或按右值引用传递。
  - 参数类型不能是 `std::initializer_list`。
  - 多态类（含虚函数）参数属于未定义行为。
  - Lambda/闭包类型允许，但受本章 Lambda 相关限制约束。
- **`__global__` 参数传递语义**：
  - 从 device 代码启动时，参数必须平凡可复制且平凡可析构。
  - 从 host 代码启动时，允许非平凡可复制/可析构类型，但 CUDA Runtime 使用原始内存拷贝（类似 `memcpy`）传递参数，**跳过**用户自定义拷贝构造函数的副作用。
  - 内核启动是异步的，若参数类型有非平凡析构函数，host 端可能在 `__global__` 函数执行完成前就调用析构函数，导致依赖于析构副作用的程序出错。

#### 6.6 类规则

- **类类型变量**：`__device__`/`__constant__`/`__managed__`/`__shared__` 变量不能有非空构造函数或非空析构函数的类类型。构造函数/析构函数被视为“空”的条件包括：函数已定义、无参数、初始化列表为空、函数体为空、类无虚函数/虚基类/非静态数据成员初始化器、基类和成员的默认构造/析构也是空的。
- **数据成员**：类/结构体/联合的数据成员不能用 `__device__` 等内存空间修饰符。仅支持编译期求值的静态数据成员（`const`、`constexpr`、`static inline constexpr`）。
- **成员函数**：`__global__` 函数不能是结构体/类/联合的成员，但可以在友元声明中出现（不能定义）。
- **隐式/显式默认成员函数**：编译器隐式声明或首次声明即 `= default` 的非虚函数，其执行空间是所有调用者的执行空间之并集。若该函数是隐式声明的虚函数，则被覆盖的虚函数的执行空间也会加入并集。
- **多态类**：
  - 将多态对象从 device 复制到 host 或反之（包括作为 `__global__` 参数）属于未定义行为。
  - 覆盖虚函数时，派生类函数的执行空间必须与基类函数一致。
- **MSVC 特定类布局**：CUDA 遵循 IA64 ABI，而 MSVC 不遵循。对于指向成员的类型、多态类、含多个空基类的多重继承类，或特定空基类布局的类，在 MSVC 下 host 和 device 的类布局/大小可能不同，跨空间复制属于未定义行为。

#### 6.7 模板限制

`__global__` 函数或 `__device__`/`__constant__` 变量（C++14）的模板参数不能是以下类型：

- 在 `__host__` 或 `__host__ __device__` 函数作用域内定义的类型。
- 匿名类型（匿名结构、非 device/global 函数内的 lambda）。
- private 或 protected 类成员（除非该类定义在 `__device__` 或 `__global__` 函数内）。
- 由上述类型复合而成的类型。

### 7. C++11/14/17/20 特定限制

- **C++11 inline namespace / inline unnamed namespace**：不能在其中定义 `__global__` 函数、设备/常量/管理/共享变量、纹理/表面对象变量。
- **C++11 constexpr 函数**：默认不能从不兼容执行空间的函数中调用。`--expt-relaxed-constexpr` 可放松此限制，允许在需要常量求值的上下文（如 `constexpr` 变量初始化器）中跨空间调用，并允许在 device 代码中生成 host-only `constexpr` 函数的代码。但 `__global__` 不能是 `constexpr`。
- **C++11 constexpr 变量**：可直接在 device 代码中使用的包括：标量类型（非指针/成员指针）、枚举、`nullptr_t`、含 `constexpr` 构造函数的类、以及上述类型的原始数组（仅在 `constexpr __device__`/`__host__ __device__` 函数内部）。不支持 `constexpr __managed__` 和 `constexpr __shared__`。
- **C++14 deduced return type**：`__global__` 函数不能有推导返回类型。`__device__` 函数的推导返回类型不能在 host 代码中萃取。
- **C++14 变量模板**：在 MSVC 上，`__device__`/`__constant__` 变量模板不能是 const-qualified 的。
- **C++17 inline 变量**：`__device__`/`__constant__`/`__managed__` 的 `inline` 变量仅在 Separate Compilation 模式或内部链接（`static`/匿名命名空间）时允许。GCC 下 `inline __managed__` 可能对调试器不可见。
- **C++17 结构化绑定**：不能带 `__device__`/`__shared__`/`__constant__`/`__managed__` 内存空间修饰符。
- **C++20 `<=>`**：device 代码支持，但隐式依赖 `<compare>` 中的 `std::strong_ordering` 等主机实现，可能需要 `--expt-relaxed-constexpr`。
- **C++20 `consteval`**：不受执行空间限制，host 和 device 代码均可调用。

---

## 典型工作流程 / 调用顺序

### 设备端 printf 诊断流程

1. 在 kernel 代码中需要诊断的位置插入 `printf("...", ...)`。
2. 确保参数不超过 32 个，格式字符串兼容主机 C 库。
3. 内核启动后，执行 `cudaDeviceSynchronize()`（或等效同步操作）强制刷新 printf 缓冲区。
4. 在主机终端查看输出。若输出缺失，检查是否忘记同步、缓冲区是否因循环写入而被覆盖。

### 设备端动态内存管理流程

1. **前置配置**：在首次使用任何设备堆分配（`malloc`、`new`）之前，调用 `cudaDeviceSetLimit(cudaLimitMallocHeapSize, size)` 设置堆大小。
2. **分配**：在内核中调用 `malloc`/`aligned_alloc`/`new`。检查返回值是否为 NULL。
3. **跨作用域使用**：分配的指针可写入 `__device__` 全局数组、通过共享内存广播，或供后续内核使用。
4. **释放**：确保同一指针仅被 `free`/`delete` 一次。建议仅由单个线程（如 `threadIdx.x == 0`）执行释放，避免重复释放。
5. **同步检查**：在释放前后插入 `cudaDeviceSynchronize()`，通过 `cudaGetLastError()` 捕获堆耗尽或非法释放错误。

### 扩展 Lambda 跨空间传递流程

1. 在 `__host__` 或 `__host__ __device__` 函数内定义扩展 lambda，标注 `__device__` 或 `__host__ __device__`。
2. 将该 lambda 作为模板参数传递给 `__global__` 函数模板（如 `kernel<<<...>>>(lambda)`）。
3. 在 kernel 内部通过参数调用 lambda（如 `c()`）。
4. 若需要萃取 lambda 返回类型，仅在 device 代码中进行；或在 host 代码中确认 `__nv_is_extended_device_lambda_with_preserved_return_type()` 为 true。
5. 若 lambda 捕获了局部变量，确保捕获列表不依赖于 `__CUDA_ARCH__`，且所有捕获均为按值捕获。

---

## 关键限制、边界条件与兼容性

- **计算能力兼容性**：本章所述的 C++17 特性在 sm_61（Pascal 架构）上完全可用。C++20 特性（如 Concepts、三路比较、协程）需要 CUDA Toolkit 12.x，但在 sm_61 上仍受限于主机编译器版本（GCC >= 10.0 等）。
- **printf 缓冲区**：固定 1 MB 循环缓冲区，没有动态扩容机制。高频、多线程 printf 会覆盖旧数据。
- **设备堆**：默认 8 MB，模块加载后不可调整。若内核中存在大规模并行 `malloc`（如每个线程均分配），极易耗尽堆空间，应优先使用块级共享分配或预分配策略。
- **Lambda 限制密度最高**：18 条限制中的大多数源于 CUDA 前端将扩展 lambda 替换为占位类型时需要取外围函数地址。这要求外围函数具名、地址可访问、模板参数无歧义、非推断返回类型。任何违反都会导致 host 编译失败或静默的运行时错误（如闭包布局不一致）。
- **非平凡类型的 kernel 参数**：从 host 启动 kernel 时，非平凡可复制/可析构类型的拷贝构造和析构语义被绕过。若程序依赖拷贝构造的副作用（如计数、日志）或假设参数在内核执行期间保持存活，则行为会出错。
- **volatile 的误用风险**：GPU 上 `volatile` 不提供线程间同步语义，也不保证 MMIO 访问次数。线程同步务必使用 `cuda::atomic_ref`、`cuda::atomic` 或 `atomicAdd`/`atomicExch` 配合 `__threadfence()`；MMIO 务必使用内联 PTX `ld.mmio`/`st.mmio`。
- **静态变量双实体**：`__host__ __device__` 函数内的 static 变量在 host 编译和设备编译中生成不同实体，各自独立演进。程序不应假设两者共享同一状态。
- **MSVC 特有陷阱**：MSVC 下的类布局差异、不支持局部类型模板参数、不支持 non-extern linkage 函数地址作为模板参数，以及 `__device__` const 变量模板限制，均需在跨平台代码中特别注意。

---

## 常见陷阱与调试建议

1. **printf 看不到输出**：最常见原因是未在 kernel 启动后调用 `cudaDeviceSynchronize()`。程序退出前也必须同步，因为缓冲区不会自动刷新。
2. **设备 malloc 返回 NULL**：检查是否忘记 `cudaDeviceSetLimit(cudaLimitMallocHeapSize, ...)`；检查已分配总量是否超过设定值；注意堆不会在运行中自动增长。
3. **扩展 lambda 编译失败**：优先检查是否定义在 `__host__` 或 `__host__ __device__` 函数内；是否使用了 `[&]` 引用捕获；是否是泛型 host-device lambda（`auto` 参数）；是否嵌套在另一个扩展 lambda 或泛型 lambda 内。
4. **lambda 运行时崩溃**：若 lambda 在成员函数内引用成员变量且使用默认 `[=]` 捕获，实际捕获的是 `this` 指针。当 lambda 在 device 执行时，`this` 指向 host 内存，导致段错误。应改用 `[=, *this]`（C++17）按值捕获对象副本。
5. **ADL 导致调用歧义**：若扩展 lambda 定义在某个命名空间内的函数中，该命名空间可能通过占位类型的模板参数意外参与 ADL，导致 host 编译阶段出现“ambiguous call”错误。解决方法是显式限定函数名或使用非 ADL 调用语法。
6. **类型特征结果不一致**：避免对扩展 lambda 的闭包类型使用 `std::is_trivially_copyable` 等特征的结果去实例化 `__global__` 模板。CUDA 前端和主机编译器对这些特征可能给出不同答案。
7. **kernel 参数拷贝构造被跳过**：若结构体的拷贝构造函数执行了资源分配或状态检查，不要期望它在 host-to-device 传递时被调用。应手动在 host 侧准备 POD 或 trivially-copyable 的传输表示。
8. **kernel 参数提前析构**：非平凡析构类型的 kernel 参数可能在 host 侧内核启动语句结束后就进入析构，而 kernel 仍在异步执行。若析构函数会释放内核仍在使用的资源（如 `__managed__` 内存的关联句柄），将导致数据竞争或访问无效内存。
9. **避免 device 代码中直接调用 `std::` 函数**：主机 STL 实现可能包含异常、RTTI 或平台相关依赖。应始终使用 `cuda::std::` 的对应功能。
10. **constexpr 跨空间调用**：默认情况下，`constexpr __device__` 函数不能在 host 代码中调用。若确有需要，使用 `--expt-relaxed-constexpr` 并确保调用点处于需要常量求值的上下文；否则将编译错误。

---

## 一个最小可运行示例的说明

本章配套的 `chapter_demo.cu` 是一个可在 sm_61（GTX 1060）与 CUDA 12.8 环境下编译运行的最小完整示例，其 `Makefile` 使用 `nvcc -ccbin /usr/bin/g++-14 -std=c++17 -arch=sm_61 -O2 --extended-lambda`。示例围绕本章最核心且易出错的三个主题进行设计：**设备端 C 库函数**、**扩展 Lambda** 和 **C++17 设备代码特性**。

### 示例结构与设计动机

1. **设备端 printf（`printf_demo_kernel`）**
   - 演示 `printf` 在内核中的基本用法，以及输出顺序的不确定性（多线程各自调用）。
   - 设计要点：kernel 启动后必须 `cudaDeviceSynchronize()` 才能看到输出，符合本章 5.3.6.2 节所述的缓冲区刷新规则。

2. **设备端 malloc/free（`malloc_alloc_kernel`、`malloc_verify_kernel`、`malloc_free_kernel`）**
   - 演示设备堆分配的生命周期：先 `cudaDeviceSetLimit` 设置堆大小，再在内核中 `malloc`，随后跨 kernel 使用（验证数据），最后由 `free` 释放。
   - 设计要点：展示本章 5.3.6.4 节强调的“分配可在上下文生命周期内跨线程/跨 kernel 使用”以及“同一块内存只能释放一次”的惯例（每个线程管理自己的指针，避免重复释放）。

3. **扩展 Lambda（`invoke_kernel` 模板 + 两个 lambda）**
   - `ext_device_lambda` 标注 `__device__`，`ext_hd_lambda` 标注 `__host__ __device__`。二者均被传递给模板 `__global__` 函数 `invoke_kernel`。
   - 设计要点：这是标准 lambda 无法实现的能力，直接对应本章 5.3.7.1 节“Lambda Expressions and __global__ Function Parameters”。`ext_hd_lambda` 内部还调用了 `__host__ __device__` 辅助函数，展示跨空间函数在 lambda 中的可组合性。

4. **扩展 Lambda 类型特征（编译期 `static_assert`）**
   - 使用 `__nv_is_extended_device_lambda_closure_type`、`__nv_is_extended_host_device_lambda_closure_type` 和 `__nv_is_extended_device_lambda_with_preserved_return_type` 对前述 lambda 进行分类验证。
   - 设计要点：展示本章 5.3.7.3 节提供的编译期 introspection 能力，帮助开发者理解不同标注 lambda 在前端编译器中的类型差异。

5. **C++17 `constexpr if`（`get_type_bits`）**
   - 模板函数使用 `if constexpr` 根据 `sizeof(T)` 在编译期选择分支。
   - 设计要点：展示本章 5.3.3 节所列 C++17 特性在 sm_61 设备代码中的完全可用性。`constexpr if` 不会产生运行时分支，是 device 端类型分发的零开销抽象。

### sm_61 兼容性与降级处理

- 本示例刻意选用 C++17 而非 C++20，因为 sm_61 在 CUDA 12.x 下对 C++17 的支持已完全成熟，无需额外降级。
- `--extended-lambda` 编译标志是必需的；缺少该标志时，扩展 lambda 语法（`[] __device__ {}`）将导致编译错误。
- 示例未使用 device 端递归、`long double`、RTTI、异常、或动态并行（Dynamic Parallelism），这些要么在 sm_61 上行为受限，要么本章明确列为不支持。
- 所有设备内存操作均通过 `CUDA_CHECK` 宏包裹，确保在 sm_61 或任何兼容架构上运行时，若出现堆耗尽、同步失败等错误都能立即暴露，而非静默失败。

通过该示例，开发者可以验证本地工具链的 nvcc + g++-14 组合是否能正确处理扩展 lambda 的占位类型变换、设备堆分配语义，以及 C++17 特性在设备代码中的零开销集成。
