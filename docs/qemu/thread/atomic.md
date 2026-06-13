## qemu 中的 atomic 使用
<!-- 76b7b4c1-8616-4ee9-b151-9d9243320730 -->
https://qemu.readthedocs.io/en/v10.0.3/devel/atomics.html

The semantics of concurrent memory accesses are governed by the C11 memory model.
(c11 和 c++11 memory model 实际上是一个东西，参考 https://stackoverflow.com/questions/12800255/c11-c11-memory-model)

总体来说，难度不大，还是那些东西，不过是粗略读的东西。

1. Compiler memory barrier 全部都是问题
2. smp_read_barrier_depends 需要一些特殊的关注，这里又讲到了不熟悉的 The DEC Alpha 的问题

### qemu 为什么没有使用 stdatomic.h 吗?
<!-- f2b3ec86-fcb6-4053-8568-20ed1c35295d -->

这个头文件是 gcc 自带的
```txt
🧀  sudo rpm -qf /usr/lib/gcc/x86_64-redhat-linux/15/include/stdatomic.h
gcc-15.2.1-4.fc42.x86_64
```

我的感觉主要是考虑到兼容性的问题吧

这个问题中的内容也很简单，对于 gcc 内置的各种函数提供了 macro 的封装

不过这两个文件可以继续看看:
```txt
extern void atomic_thread_fence (memory_order);
extern void atomic_signal_fence (memory_order);
```
2026-04-09 又看了一次，的确不容易

找到如下内容:
1. https://en.cppreference.com/w/cpp/atomic/atomic_thread_fence.html
2. https://stackoverflow.com/questions/14581090/how-to-correctly-use-stdatomic-signal-fence

### qatomic_read 和 READ_ONCE 的区别
<!-- 109e4a02-e5f8-4b6c-bae5-52c5e230053c -->

首先，文档中指出来的这个:
```txt
- Originally, ``atomic_read`` and ``atomic_set`` in Linux gave no guarantee
  at all. Linux 4.1 updated them to implement volatile
  semantics via ``ACCESS_ONCE`` (or the more recent ``READ``/``WRITE_ONCE``).

  QEMU's ``qatomic_read`` and ``qatomic_set`` implement C11 atomic relaxed
  semantics if the compiler supports it, and volatile semantics otherwise.
  Both semantics prevent the compiler from doing certain transformations;
  the difference is that atomic accesses are guaranteed to be atomic,
  while volatile accesses aren't. Thus, in the volatile case we just cross
  our fingers hoping that the compiler will generate atomic accesses,
  since we assume the variables passed are machine-word sized and
  properly aligned.

  No barriers are implied by ``qatomic_read`` and ``qatomic_set`` in either
  Linux or QEMU.
```

```c
static inline void atomic_load(atomic_t *counter, unsigned long *value)
{
	__atomic_load(&counter->counter, value, __ATOMIC_RELAXED);
}
```

- include/qemu/atomic.h 中实现和内核中等价吗?
	- 不等价，最后分析了，其中的差别是 (TODO)


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
