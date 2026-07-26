volatile 修饰的是“访问方式”，需要先区分值传递和指针传递。

1. volatile 变量按值传递

volatile unsigned long addr;

void m(unsigned long value);

m(addr);

调用时大致发生：

unsigned long tmp = addr;  // 对 addr 进行一次 volatile 读取
m(tmp);                    // 将普通值传入

volatile 不会随着数值一起传递。进入 m() 后，参数 value 只是一个普通的局部副本。之后即使原来的 addr 改变，value 也不会改变。

即使写成：

void m(volatile unsigned long value);

也只是让 m() 内部的局部参数 value 成为 volatile，并没有让它和原来的 addr 建立联系。

如果函数需要反复访问原对象，必须传地址：

```txt
void m(volatile unsigned long *p)
{
    unsigned long a = *p;  // volatile 读取
    unsigned long b = *p;  // 再次 volatile 读取
}

m(&addr);
```

———

2. volatile 指针的准确含义

下面三个声明不同：

volatile unsigned long *p1;
unsigned long * volatile p2;
volatile unsigned long * volatile p3;

含义分别是：

volatile unsigned long *p1;

- p1 是普通指针
- *p1 是 volatile 对象
- 编译器不能随意合并、删除通过 p1 进行的访问

unsigned long * volatile p2;

- p2 这个指针变量本身是 volatile
- *p2 指向的对象不是 volatile
- 每次读取 p2 都是 volatile 访问，但访问 *p2 不是

volatile unsigned long * volatile p3;

- 指针本身是 volatile
- 指向的对象也是 volatile

所以：

volatile unsigned long *addr;

严格来说应称为“指向 volatile unsigned long 的指针”，而不是“volatile 指针”。

还要注意，你的代码：

void m(unsigned long value);
m(addr);

类型不匹配：addr 是指针，m() 要的是整数。编译器至少应该给出诊断。

可能想写的是：

m(*addr);

这会先进行一次 volatile 读取，再把读取到的普通数值传给 m()。

或者：

void m(volatile unsigned long *p);
m(addr);

这才是把访问原对象的能力传给函数。

———

3. volatile 不保证什么

volatile 主要约束编译器对访问的优化。它不保证：

- 操作是原子的
- 多线程访问不存在数据竞争
- CPU 不会进行内存重排
- 具有 acquire/release 语义
- 数据会绕过 CPU cache
- x++ 变成原子操作

例如：

volatile unsigned long x;
x++;

通常仍然是：

读取 x
加 1
写回 x

其他 CPU 可以在中间插入操作，因此它不是原子递增。Linux 内核也明确说明，volatile 不是共享数据同步机制，应当使用锁、原子操作和内存屏障。Linux 内核说明
(https://www.kernel.org/doc/html/latest/translations/zh_CN/process/volatile-considered-harmful.html)

———

4. arch_test_and_set_bit() 为什么接收 volatile 指针

arch_test_and_set_bit(long nr,
                      volatile unsigned long *addr)

首先，它必须接收指针，因为函数要直接修改调用者的位图，并返回修改前的 bit 值：

unsigned long flags = 0;

bool old = arch_test_and_set_bit(3, &flags);

参数中的 volatile 有两个主要作用。

### 接受 volatile 和非 volatile 对象

普通对象可以隐式增加限定符：

unsigned long normal;
volatile unsigned long vol;

arch_test_and_set_bit(1, &normal); // 可以
arch_test_and_set_bit(1, &vol);    // 也可以

如果参数声明成：

unsigned long *addr

那么传入 volatile unsigned long * 会丢弃 volatile 限定符，编译器应当给出诊断。

### 约束通过该指针进行的 C 语言访问

在函数内部，表达式：

*addr

具有 volatile 访问语义。对于某些使用 C 代码实现的架构，这可以避免相关内存访问被普通优化消除或合并。

但必须强调：

> arch_test_and_set_bit() 的原子性并不是由 volatile 提供的。

以 x86 当前实现为例，它使用带 LOCK_PREFIX 的位操作指令和 inline asm 内存约束；真正提供原子读改写的是硬件指令及内核的汇编/原子操作实现。x86 bitops 实现
(https://raw.githubusercontent.com/torvalds/linux/master/arch/x86/include/asm/bitops.h)

通用实现则最终调用：

raw_atomic_long_fetch_or(...)

原子性来自这个原子 primitive，而不是参数上的 volatile。asm-generic 实现 (https://raw.githubusercontent.com/torvalds/linux/master/include/asm-generic/bitops/atomic.h)

因此更准确的总结是：

volatile 参数：
    保留并表达“通过该指针访问时不能当作普通内存访问”的类型语义，
    同时兼容 volatile 对象。

LOCK 指令 / 原子 primitive：
    提供真正的原子读-修改-写。

内存屏障和具体 bitops API 契约：
    提供所需的编译器及 CPU 内存顺序。

另外，普通位图通常不需要声明为 volatile：

unsigned long bitmap;
arch_test_and_set_bit(3, &bitmap);

这是合法且常见的。函数形参中的 volatile 只影响通过该形参进行的访问，并不会把调用者的 bitmap 永久变成 volatile。

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
