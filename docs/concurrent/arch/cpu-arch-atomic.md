# 反汇编常见指令

### why
```c
typedef struct {
	int counter;
} atomic_t;

#define ATOMIC_INIT(i) { (i) }
```

### `_raw_spin_lock_irqsave`

## atomic_add_return
```txt
void martins3(void)
{
	atomic_t combined_event_count = ATOMIC_INIT(0);
	int x = 12;
	x = atomic_add_return(x, &combined_event_count);
	pr_info("%d", x);
}
```

```txt
dis$ disass martins3
Dump of assembler code for function martins3:
   0xffffffff81d7a510 <+0>:     endbr64
   0xffffffff81d7a514 <+4>:     sub    $0x10,%rsp
   0xffffffff81d7a518 <+8>:     mov    $0xc,%esi               # 初始化 x
   0xffffffff81d7a51d <+13>:    mov    %gs:0x28,%rax
   0xffffffff81d7a526 <+22>:    mov    %rax,0x8(%rsp)
   0xffffffff81d7a52b <+27>:    xor    %eax,%eax
   0xffffffff81d7a52d <+29>:    movl   $0x0,0x4(%rsp)           # 初始化 combined_event_count
   0xffffffff81d7a535 <+37>:    lock xadd %esi,0x4(%rsp)        # esi 中持有两者之后，而 0x(%rsp) 也就是 atomic_t 被更新上两者之和。

   0xffffffff81d7a53b <+43>:    mov    $0xffffffff82a06bc1,%rdi # 调用 pr_info
   0xffffffff81d7a542 <+50>:    add    $0xc,%esi
   0xffffffff81d7a545 <+53>:    call   0xffffffff82133af8 <_printk>
   0xffffffff81d7a54a <+58>:    mov    0x8(%rsp),%rax
   0xffffffff81d7a54f <+63>:    sub    %gs:0x28,%rax
   0xffffffff81d7a558 <+72>:    jne    0xffffffff81d7a563 <martins3+83>
   0xffffffff81d7a55a <+74>:    add    $0x10,%rsp
   0xffffffff81d7a55e <+78>:    jmp    0xffffffff821b86c4 <__x86_return_thunk>
   0xffffffff81d7a563 <+83>:    call   0xffffffff821a50d0 <__stack_chk_fail>
```
实际上就是 xadd(&v->counter, i); 而已。

对比普通的 add :
https://stackoverflow.com/questions/30130752/assembly-does-xadd-instruction-need-lock/65055576#65055576

## spin_lock_irq

```c
static __always_inline void spin_lock_irq(spinlock_t *lock)
{
	raw_spin_lock_irq(&lock->rlock);
}
```

```c
static inline __attribute__((__gnu_inline__)) __attribute__((__unused__)) __attribute__((no_instrument_function)) __attribute__((__always_inline__)) void queued_spin_lock(struct qspinlock *lock)
{
 int val = 0;

 if (__builtin_expect(!!(atomic_try_cmpxchg_acquire(&lock->val, &val, (1U << 0))), 1))
  return;

 queued_spin_lock_slowpath(lock, val);
}
```

## qatomic_add
```c
void *trythis(void *arg) {
  unsigned long i = 10000000;
  printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
  while (i--) {
    // qatomic_set(&counter, counter + 1);
    qatomic_add(&counter, 1);
  }
  return NULL;
}
```

$ disass trythis
Dump of assembler code for function trythis:
   0x00000000004011e6 <+0>:     push   %rbp
   0x00000000004011e7 <+1>:     mov    %rsp,%rbp
   0x00000000004011ea <+4>:     sub    $0x20,%rsp
   0x00000000004011ee <+8>:     call   0x401060 <mcount@plt>
   0x00000000004011f3 <+13>:    mov    %rdi,-0x18(%rbp)
   0x00000000004011f7 <+17>:    movq   $0x989680,-0x8(%rbp)
   0x00000000004011ff <+25>:    mov    $0x21,%edx
   0x0000000000401204 <+30>:    mov    $0x402058,%esi
   0x0000000000401209 <+35>:    mov    $0x402008,%edi
   0x000000000040120e <+40>:    mov    $0x0,%eax
   0x0000000000401213 <+45>:    call   0x401040 <printf@plt>
   0x0000000000401218 <+50>:    jmp    0x401222 <trythis+60>
   0x000000000040121a <+52>:    lock addl $0x1,0x2e4e(%rip)        # 0x404070 <counter>
   0x0000000000401222 <+60>:    mov    -0x8(%rbp),%rax
   0x0000000000401226 <+64>:    lea    -0x1(%rax),%rdx
   0x000000000040122a <+68>:    mov    %rdx,-0x8(%rbp)
   0x000000000040122e <+72>:    test   %rax,%rax
   0x0000000000401231 <+75>:    jne    0x40121a <trythis+52>
   0x0000000000401233 <+77>:    mov    $0x0,%eax
   0x0000000000401238 <+82>:    leave
   0x0000000000401239 <+83>:    ret
End of assembler dump.

# CPU 微架构

## 总结一下常见的原子指令实现，希望可以理解原子执行设计有什么考虑

参考 [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf)

1. Test-And-Set
1. Fetch-And-Add
2. Load-Linked and Store-Conditional
3. Compare-And-Swap

这里对比了下 ARM 从 ll-sc 到 fetch-and-add 的之后，似乎性能有较大的提升:
https://cpufun.substack.com/p/atomics-in-aarch64

atomic 和 CAS

## 硬件支持上也可以分析一下
- [CPU 多核指令 —— WFE 原理](http://www.wowotech.net/armv8a_arch/499.html)
  - http://www.wowotech.net/sort/armv8a_arch : 其实 wowotech 关于 ARM 的 atomic 分析了不少内容

- umwait 指令: https://lwn.net/Articles/790920/

## [x] atomic 指令如何实现的?

### [x] 为什么 atomic 指令还有 memory model ，从 CPU 设计的角度如何理解?

### 如何理解这些架构细节?

* store buffer、load queue 和 cache pipeline 可能交错处理其他请求；
* coherence 请求可能要求该 line 被降级或转移；

是从哪里知道 CPUID 是完全清空流水线的
locked instruction 不是 `CPUID` 那种“完全序列化整个流水线”的 serializing instruction。它主要对内存访问和一致性事务施加强约束。

### [ ] 为什么 arm 用了两套实现机制
后者是更加容易的

### 如果 mutex 命中了，那么就采用

### 这个机制我是不太理解的
/home/martins3/data/vn/docs/concurrent/kernel/api/gcc-atomic.md
中提到了，为什么会存在 weak 的写法?

## 看看这个
https://zhuanlan.zhihu.com/p/191660613

## 无竞争时 atomic inc 和 plain i++ 的性能差距
<!-- 2c82aeb8-4c5d-44ea-98e3-ec3dd88a2fcb -->

测试代码 [atomic-inc-bench.c](./atomic-inc-bench.c)，单线程 10 亿次自增，
plain i++ 用 volatile 防止编译器把整个循环折叠成一次加法（模拟真实代码中每次都写内存的形态）。

x86_64 (gcc -O2):

| 方式 | ns/op | 相对倍数 |
|---|---|---|
| volatile plain i++ | 1.37 | 1x |
| atomic relaxed | 3.29 | 2.4x |
| atomic seq_cst | 3.29 | 2.4x |

Apple Silicon (Asahi Linux, aarch64):

| 方式 | LSE (armv8.5, ldadd) | LL/SC (armv8-a, ldxr/stxr) |
|---|---|---|
| volatile plain i++ | 0.32 | 0.34 |
| atomic relaxed | 2.10 (~6.5x) | 2.19 |
| atomic seq_cst | 4.12 (~13x) | 5.31 |

结论：

- x86 上 relaxed 和 seq_cst 没区别，因为两者都编译成 `lock xadd`，
  无竞争时代价就是独占总线锁本身，大约 2.4 倍于普通 load+add+store。
- aarch64 的 seq_cst 明显比 relaxed 贵：relaxed 是 `ldadd`，seq_cst 是 `ldaddal`，
  多出来的 acquire/release 语义在 Apple Silicon 上代价不小。
- LL/SC (`ldxr/stxr` 循环) 在无竞争时和 LSE 差不多，差距要到有竞争时才拉开。
  (参考上面 cpufun 那篇 ARM ll-sc vs fetch-and-add 的分析)

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
