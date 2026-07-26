## kasan
- https://www.kernel.org/doc/html/latest/dev-tools/kasan.html
- https://github.com/google/kernel-sanitizers
  - KTSAN 是什么？

#### KASAN
Finding places where the kernel accesses memory that it shouldn't is the goal for the kernel address sanitizer (KASan).

分析下如下的 config 是做啥的
```txt
CONFIG_CONSTRUCTORS=y
CONFIG_GENERIC_CSUM=y
CONFIG_KASAN_SHADOW_OFFSET=0xdffffc0000000000
CONFIG_STACKDEPOT_ALWAYS_INIT=y
CONFIG_KASAN=y
CONFIG_KASAN_GENERIC=y
CONFIG_KASAN_OUTLINE=y
# CONFIG_KASAN_INLINE is not set
CONFIG_KASAN_STACK=y
# CONFIG_KASAN_VMALLOC is not set
# CONFIG_KASAN_MODULE_TEST is not set

# 分析下 CONFIG_VMAP_STACK=y 是做什么的
# 打开 KASAN 之后，这个选项就消失了
```
#### kmemleak
Kmemleak provides a way of detecting possible kernel memory leaks in a way similar to a tracing garbage collector, with the difference that the orphan objects are not freed but only reported via /sys/kernel/debug/kmemleak. [^18]

# 2. ASan 和 KASAN 的关系

## 用户态 ASan

用户态 ASan 通常由两部分组成：

```text
LLVM/GCC ASan pass
        ↓
compiler-rt/libasan runtime
```

运行时负责：

* 管理进程地址空间中的 shadow memory；
* 包装或拦截 `malloc/free/new/delete`；
* 在分配对象周围添加 redzone；
* 对释放对象进行 poison；
* 将部分释放对象放入 quarantine，延迟重新使用；
* 处理线程、动态库、libc 函数；
* 输出报告、符号化栈信息。

ASan 的典型机制是一个 shadow byte 描述 8 字节应用内存，并配合 redzone 与 quarantine 检测越界和 use-after-free。([谷歌研究][2])

## Generic KASAN

KASAN 并没有另外发明一种编译器 pass。Linux 构建系统直接传入：

```make
-fsanitize=kernel-address
-fasan-shadow-offset=...
```

或者 LLVM 参数：

```make
-mllvm -asan-mapping-offset=...
```

也就是说，它使用的是编译器中的 kernel ASan 模式。([GitHub][3])

编译器会生成类似这些 ABI 调用：

```c
__asan_load1(addr);
__asan_load4(addr);
__asan_store8(addr);
__asan_register_globals(...);
__asan_alloca_poison(...);
```

然后 Linux 在 `mm/kasan/` 中自己实现这些函数。例如 `mm/kasan/generic.c` 明确实现了：

```c
void __asan_load4(void *addr);
void __asan_store8(void *addr);
void __asan_loadN(void *addr, ssize_t size);
void __asan_register_globals(void *ptr, ssize_t size);
```

这些函数最终调用内核自己的 shadow 检查和 `kasan_report()`。([GitHub][4])

因此调用链大致是：

```text
kernel C code
    ↓
GCC/LLVM ASan instrumentation
    ↓
__asan_loadN / inline shadow check
    ↓
Linux mm/kasan runtime
    ↓
kasan_report()
```

## KASAN 和 ASan 真正不同的部分

### 1. 分配器不同

用户态 ASan主要接入：

```text
malloc/free
new/delete
mmap
libc interceptors
```

KASAN 必须接入：

```text
SLAB/SLUB
kmalloc/kfree
kmem_cache_alloc/free
page allocator
alloc_pages/free_pages
vmalloc/vfree
vmap
kernel stack
loadable modules
```

Linux 文档列出的 Generic KASAN 覆盖范围包括 slab、page allocator、vmap、vmalloc、内核栈和全局变量。([Linux内核档案][5])

例如，释放 slab 对象时，KASAN 不只是执行一个通用 `free()` hook，而是：

```text
找到所属 slab/cache
检查是否 invalid free / double free
poison 对象
记录 free stack
决定是否进入 KASAN quarantine
最后再允许 SLUB 回收
```

这些逻辑直接嵌入 SLAB/SLUB 生命周期中。([GitHub][6])

### 2. 地址空间不同

用户态 ASan 可以在一个普通进程的虚拟地址空间中保留大块 shadow 区域。

内核则需要处理：

* kernel direct map；
* `vmalloc` 区域；
* module 区域；
* 每种架构不同的 kernel VA layout；
* KASLR；
* 4 级/5 级页表；
* shadow 自己的页表；
* 内存管理器尚未启动时的 early shadow。

所以内核构建时必须向编译器提供架构相关的 `KASAN_SHADOW_OFFSET`。([GitHub][3])

### 3. 内核是自己的内存分配器

用户态 ASan runtime 位于被操作系统管理的进程里。它可以调用：

```text
mmap
pthread
malloc
write
signal handler
dynamic loader
```

KASAN 本身却运行在操作系统核心中。它不能依赖 libc、pthread、普通 `mmap()` 或用户态 signal handler；甚至在内核内存分配器初始化之前，KASAN 就可能需要开始工作。

因此不能简单把 `libasan.so` 链进 `vmlinux`。

### 4. 错误处理不同

用户态 ASan通常可以：

```text
打印 stderr
终止当前进程
设置 exit code
调用外部 symbolizer
```

内核不能简单“杀掉当前进程”，因为错误可能发生在：

* 中断上下文；
* softirq；
* 调度器；
* 内存分配器；
* RCU；
* idle task；
* early boot；
* 持有 spinlock 时。

KASAN 必须使用内核的：

```text
printk
stack unwinder
stack depot
task/PID/CPU 信息
slab/page 元数据
panic/oops 策略
```

所以 KASAN 报告可以显示对象所属的 slab cache、分配栈、释放栈以及对应的 `struct page` 信息。([Linux内核档案][5])

---

# 3. Generic KASAN 和 ASan 到底有多像

Generic KASAN 与用户态 ASan 的 shadow 编码基本属于同一家族：

```text
1 shadow byte → 8 bytes kernel memory
```

典型编码：

```text
00      8 字节全部可访问
01..07  前 N 字节可访问
负值    整个 granule 不可访问
```

不同的负值表示：

* slab redzone；
* freed slab object；
* page free；
* stack left/mid/right redzone；
* global redzone；
* alloca redzone。

Linux 源码甚至把这些值标注为：

```c
/* Compiler ABI, do not change. */
```

说明这部分必须与 GCC/LLVM 的 ASan 插桩 ABI 保持一致。([GitHub][7])

不过现在 KASAN 不只有 Generic 模式：

| KASAN 模式                 | 对应思路                        |
| ------------------------ | --------------------------- |
| Generic KASAN            | 类似用户态 ASan，1:8 shadow       |
| Software Tag-Based KASAN | 类似 HWASan，指针 tag 与内存 tag 比较 |
| Hardware Tag-Based KASAN | 使用 ARM64 MTE 硬件检查           |

Linux 文档也明确将 Generic KASAN 类比为 ASan，将 SW_TAGS KASAN 类比为 HWASan。([Linux内核档案][5])

## 对 KASAN 相对于 ASAN 的区别

```text
这个地址属于哪个 kmem_cache？
这个对象真实请求大小是多少？
SLUB 为对齐额外添加了多少空间？
对象是否是 SLAB_TYPESAFE_BY_RCU？
对象现在能否进入 quarantine？
这个 page 是否是 highmem？
vmalloc 区域的 shadow 是否已建立？
模块刚加载时如何建立 shadow？
```

这些信息只存在于 Linux 内存管理器中。

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
