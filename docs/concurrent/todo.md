## [ ] atomic 指令是自动携带 memory barrier 的吗？

## [ ] x86 为什么又存在 lock prefix，又存在 cas 指令

## [ ] atomic 和 cas 似乎是两个东西啊

## 验证: 原子指令不是自带 memory barrier 的

因为原子指令的实现方法是:

## [ ] memory barrier 总是配对的吧

## qatomic_set 有意义吗？
set 本身就是 atomic 的

```c
/* Weak atomic operations prevent the compiler moving other
 * loads/stores past the atomic operation load/store. However there is
 * no explicit memory barrier for the processor.
 *
 * The C11 memory model says that variables that are accessed from
 * different threads should at least be done with __ATOMIC_RELAXED
 * primitives or the result is undefined. Generally this has little to
 * no effect on the generated code but not using the atomic primitives
 * will get flagged by sanitizers as a violation.
 */
#define qatomic_read__nocheck(ptr) \
    __atomic_load_n(ptr, __ATOMIC_RELAXED)
```

只是为了告诉 sanitizers 而已。

## 为什么 pause 指令可以拯救 memory order 啊？
- https://www.felixcloutier.com/x86/pause

## 看 smp_store_release 的展开最后就是 barrier

```c
# define barrier() __asm__ __volatile__("": : :"memory")
```
但是这条指令展开似乎没有没有指令，我们需要理解下

## 仔细阅读这个
- https://stackoverflow.com/questions/50323347/how-many-memory-barriers-instructions-does-an-x86-cpu-have

intel 指令的 ，lock 是自带 memory barrier 的

## 如何和外设进行同步
- cxl 似乎处理过这个

## 内核中的 readwrite lock 没有分析

## Linux programming interface 中间分析过 process 之间的同步方法
