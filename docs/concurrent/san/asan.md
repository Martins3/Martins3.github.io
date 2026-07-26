# asan

本文只是 ChatGPT 的一个回答，人工审查了所有的内容，并且删掉一些很很容易理解的内容。

ASan（AddressSanitizer）的核心思路：
1. 给应用内存建立一份 shadow memory，记录每一小段内存当前是否允许访问；
2. 编译器在每次 load/store 前插入检查。

它主要检测：

* heap / stack / global 越界；
* use-after-free；
* use-after-return、use-after-scope；
* double free、invalid free；
* 部分内存分配接口使用错误。

## 1. 基本结构

### 对于所有的 load / store 操作插桩

```c
int value = *p;
```

编译器在 LLVM IR 阶段看到一个 4 字节 load，会插入近似这样的逻辑：

```c
shadow_addr = ((uintptr_t)p >> 3) + offset;
shadow_value = *(signed char *)shadow_addr;

if (shadow_value != 0) {
    if (access_crosses_poisoned_region(p, 4, shadow_value))
        __asan_report_load4(p);
}

value = *p;
```

常见访问大小：

```text
1 byte
2 bytes
4 bytes
8 bytes
16 bytes
```

都有专门的快速检查和报告函数，例如概念上的：

```txt
__asan_report_load1
__asan_report_load4
__asan_report_store8
```

编译器通常会做大量优化：

* 已经证明安全的访问不插桩；
* 相邻检查可能合并；
* 常量地址和固定偏移可能简化；
* 不同大小的访问走不同 fast path。


### 将所有可以访问和不可以访问的区间进行标记

通过跟踪 malloc / free 接口，可以标记出来
所有可以访问和不可以访问的区间，那么就很容易:
1. double free 检测: 如果 free 返回包含了不可以访问的区间
2. use-after-free 检测: 访问了被被标记不可访问的区间


也就是下面谈到的 shadow memory 和 red zone 机制

### 对于 `memcpy` 等访存接口进行劫持

用户代码中的普通 load/store 可以由编译器插桩，但 libc 中的：

```txt
memcpy
memmove
memset
strcpy
strlen
```

可能没有用 ASan 重新编译。

因此 ASan runtime 会 intercept 这些函数。

例如：

```c
memcpy(dst, src, n);
```

interceptor 会检查：

```text
[src, src+n) 是否可读
[dst, dst+n) 是否可写
```

然后调用真正的实现。

分配器函数也采用类似的 interception / replacement 机制。

## 2. Shadow memory

也就是 8:1 映射。

对于一个应用地址 `addr`，它对应的 shadow 地址大致是：

```c
shadow = (addr >> 3) + shadow_offset;
```

其中：

* `addr >> 3` 相当于除以 8；
* `shadow_offset` 是平台相关的固定偏移；
* shadow memory 位于进程虚拟地址空间的专用区域。

例如：

```text
Application memory:

0x1000 - 0x1007  ──> shadow byte S0
0x1008 - 0x100f  ──> shadow byte S1
0x1010 - 0x1017  ──> shadow byte S2
```

因此检查一个地址是否可访问，只需要读一个 shadow byte。


### Shadow byte

一个 shadow byte 描述对应的 8 字节应用内存。

最常见的编码：

```text
shadow = 0
对应的 8 字节全部可访问

shadow = 1
只有第 0 个字节可访问，其余 7 字节不可访问

shadow = 2
前 2 个字节可访问，其余不可访问

...

shadow = 7
前 7 个字节可访问，最后 1 字节不可访问

shadow < 0，或者特殊 magic value
整块不可访问，并且说明为什么不可访问
```

例如分配：

```c
malloc(10);
```

假设分配区域恰好从 8 字节边界开始，则 shadow 状态可能是：

```text
应用内存：

[0 1 2 3 4 5 6 7] [8 9 X X X X X X]

shadow：

       0                 2
```

第一个 shadow byte 是 `0`，表示 8 字节都合法。

第二个 shadow byte 是 `2`，表示只有前两个字节合法。

所以访问：

```c
p[9]   // 合法
p[10]  // 非法
```

访问 `p[10]` 时：

```text
offset within 8-byte block = 10 & 7 = 2
shadow value = 2
```

因为：

```text
offset >= shadow value
2 >= 2
```

所以访问非法。

### 为什么不是每个字节一个 shadow byte ，而是 8:1

因为正常对象通常满足：

* `malloc` 返回地址至少 8 字节对齐；
* 栈变量和全局变量可以由编译器控制对齐；
* 一个 8 字节区间中，常见状态是全部合法、全部非法，或者只有尾部部分非法。

因此一个 shadow byte 足够表示：

```text
8 字节全部可访问
前 1～7 字节可访问
8 字节全部不可访问
```

但这种编码也带来一个约束：

> 它最自然地表示“前 N 字节有效，后面无效”，不能任意表示同一个 8 字节块中间有一个洞。

ASan 通过对齐和对象布局，尽量保证这种表示方式足够。


## 5. Redzone：如何发现越界

假设有两个相邻对象：

```c
char *a = malloc(8);
char *b = malloc(8);
```

如果分配器将它们紧挨着放：

```text
地址空间：

|-------- a --------|-------- b --------|
0x1000             0x1008             0x1010
```

因为 `a` 和 `b` 都是合法对象，对应 shadow 必须全部标记为可访问：

```text
Application:  [ a: 8 bytes ] [ b: 8 bytes ]
Shadow:             0              0
```

现在执行：

```c
a[8] = 1;
```

从语言和程序逻辑看，这是越过 `a` 的边界，写进了 `b`。

但 ASan 的普通检查只问：

```text
地址 0x1008 当前是否允许访问？
```

答案是：允许，因为这是 `b[0]`。

所以没有 redzone 时，ASan 看不出来。


仅有 shadow memory 还不够，ASan 需要把对象附近的内存标记成不可访问。
这就是 redzone。


## 7. Use-after-free 如何检测

考虑：

```c
char *p = malloc(32);
free(p);
p[0] = 1;
```

`free(p)` 时，ASan 不只是把内存直接交还给 allocator。

它会：

```text
1. 把用户区域的 shadow 标记成 freed / poisoned
2. 记录 free 的调用栈
3. 通常把内存放进 quarantine
```

因此后续访问：

```c
p[0] = 1;
```

会看到 poisoned shadow，报告：

```text
heap-use-after-free
```

### 为什么需要 quarantine

如果 `free(p)` 后立即重新分配给另一个对象：

```c
free(p);
q = malloc(32);
```

并且 `q == p`，那么 shadow 又会被 unpoison。

这时旧指针 `p` 的访问看起来就可能合法：

```c
p[0] = 1;
```

为了提高 use-after-free 的检测概率，ASan 不会立刻复用刚释放的对象，而是把它放进 quarantine：

```text
free
  ↓
quarantine 队列
  ↓ 过一段时间
真正可复用
```

因此 freed 内存会在一段时间内保持 poisoned 状态。

在 AddressSanitizer (ASan) 的上下文中，**Quarantine（隔离区）** 是一个用于检测 **Use-After-Free (UAF，释放后使用)** 错误的核心机制。

你可以把它理解为一个 **“内存延迟回收站”**。当程序释放（`free` 或 `delete`）一块堆内存时，
ASan 不会立刻将其归还给内存分配器以供重用，而是将它放入一个名为 Quarantine 的队列中暂存。

Quarantine 机制的根本目的，是**提高检测 Use-After-Free 错误的概率**。

如果一块内存被释放后立即被回收并重新分配给程序的其他部分使用，那么之前指向它的“悬空指针”就可能无意中访问到新的、有效的数据，
导致程序“看似”正常运行，而错误被隐藏。

Quarantine 通过**延迟内存的重用**，使被释放的内存块在一段时间内保持“已释放”的 Poisoned 状态。
这样，任何通过悬空指针访问该内存的操作，都会立即触发 ASan 的报告，从而精准地定位 UAF 错误。

## 9. Stack use-after-scope

例如：

```cpp
int *p;

{
    int x = 1;
    p = &x;
}

printf("%d\n", *p);
```

变量 `x` 的生命周期在离开作用域时结束。

编译器可以在作用域结束的位置，把 `x` 对应的 shadow poison：

```text
进入作用域：unpoison x
离开作用域：poison x
```

所以后续访问可以报告：

```text
stack-use-after-scope
```

这依赖编译器知道 C/C++ 对象的词法生命周期。


## 10. Stack use-after-return

这个更复杂：

```c
int *f(void)
{
    int x = 1;
    return &x;
}

int *p = f();
printf("%d\n", *p);
```

普通栈帧在 `f()` 返回后，内存仍然可能暂时保留原值。

如果 ASan 只使用正常栈，后续另一个函数调用可能立即覆盖 shadow 状态，检测效果有限。

因此 ASan 可以使用 fake stack：

```text
普通函数栈帧
      ↓
ASan 将部分局部变量放在专门管理的 fake stack 中
      ↓
函数返回时不立即复用，并将其 poison
```

这样返回后的旧指针访问更容易被发现。

常见运行时选项：

```bash
ASAN_OPTIONS=detect_stack_use_after_return=1
```

部分平台或编译方式默认启用。

fake stack 和 heap quarantine 思路很像：

> 延迟复用已经结束生命周期的存储空间。


## 12. ASan 如何区分不同错误

Shadow memory 中不仅有“允许”和“不允许”，还会使用一些特殊 magic value表示 poison 原因。

概念上可能包括：

```text
heap left redzone
freed heap region
stack left redzone
stack mid redzone
stack right redzone
stack after return
stack use after scope
global redzone
allocator internal region
```

因此检查失败后，runtime 读取 shadow value，就能大致判断：

```text
这是越过 heap 对象边界
还是访问已经 free 的对象
还是访问已经返回的 stack frame
```

具体 magic number 是实现细节，但原理就是给 poison 分类。



## 15. 多字节访问如何检查

假设执行：

```c
uint32_t v = *(uint32_t *)p;
```

访问范围是：

```text
[p, p + 3]
```

仅检查起始字节不一定足够，因为访问可能跨越 8 字节 shadow block。

例如：

```text
p = 地址末三位为 7
```

4 字节访问会横跨两个 shadow block。

所以 ASan 对多字节访问要考虑：

```text
访问起点
访问终点
是否跨 shadow granule
部分可访问 shadow value
```

固定大小且对齐的访问通常能走非常短的 fast path；非对齐或大块访问可能调用 runtime helper，例如概念上的：

```c
__asan_loadN(addr, size);
__asan_storeN(addr, size);
```




## 17. ASan 运行时由哪些部分组成

可以粗略拆成：

```text
编译器插桩
├── 普通 load/store 检查
├── stack redzone 布局
├── global redzone 布局
└── 生成对象描述元数据

ASan runtime
├── shadow memory 初始化
├── malloc/free/new/delete 拦截
├── quarantine
├── libc 函数拦截
├── poison/unpoison
├── 错误报告
└── stack trace / symbolization
```

编译器负责“在哪检查、对象怎么摆”。

runtime 负责“shadow 怎么维护、错误怎么报告”。


## 18. ASan 哪些情况可能漏报

ASan 也不是完备的。

### 内存被重新分配

```c
p = malloc(16);
free(p);

q = malloc(16);  // q 恰好等于 p
p[0] = 1;
```

如果该区域已经重新分配并 unpoison，ASan 不知道你使用的是旧指针还是新指针。

它检查的是：

```text
地址现在是否合法
```

而不是完整追踪：

```text
这个指针是否仍属于原来的对象实例
```


### 对象内越界

```c
struct S {
    char a[8];
    char b[8];
};

struct S s;
s.a[8] = 1;
```

从 C 语义看，这是越过成员 `a` 的边界写入 `b`。

但从 ASan 的内存布局看，整个 `struct S` 都是合法对象：

```text
[a][b]
```

`a` 和 `b` 中间通常没有 redzone，因此可能不报告。

这叫 intra-object overflow。

某些编译器配置、字段 padding 或额外 sanitizer 可以覆盖部分场景，但普通 ASan 不保证检测。


### 任意未插桩访问

例如：

```text
未使用 ASan 编译的库
内联汇编直接访问内存
JIT 生成的代码
DMA 或设备直接写内存
内核修改用户内存
```

ASan 没有执行访问前检查，就无法直接报告。

即使分配器和 shadow 是正确的，未插桩的 store 也可以直接写进 redzone。


### 自定义 allocator 没有配合 ASan

如果自定义内存池把一大片内存整体申请下来，再自行切分：

```text
ASan 只看到一个 1 MB 大对象
用户 allocator 内部切成很多 32-byte 小对象
```

那么小对象之间通常没有 ASan redzone。

内部越界只要没有越过整个 1 MB 大对象，ASan 就可能认为合法。

自定义 allocator 可以主动使用接口：

```c
__asan_poison_memory_region
__asan_unpoison_memory_region
```

来维护对象边界。


### 野指针碰巧指向合法区域

```c
int *p = corrupted_pointer();
*p = 1;
```

如果损坏后的地址碰巧落在一个当前合法、未 poisoned 的对象中，ASan不会报告。

ASan检查地址有效性，不理解这个指针“本应该”指向哪里。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
