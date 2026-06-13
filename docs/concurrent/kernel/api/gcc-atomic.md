## gcc atomic

其实内容并不多:
https://gcc.gnu.org/onlinedocs/gcc/_005f_005fatomic-Builtins.html

此外，clang 会有不同吗? clang 的文档在哪里，我猜测应该都是一样的吧

## __atomic_compare_exchange_n
<!-- 9bbff50c-0a87-45d4-9e37-a3e426f1ec06 -->

一直在使用这个函数，但是没搞懂这里的几个参数都是做什么的，这里问了一下 codex ，回答应该是非常正确的:
还有一些疑问这个到底是如何实现的，但是暂时先这样了:

1. 是如何提供 success_memorder 和 failure_memorder 的，也许看看
2. 为什么会有 weak 这种失败的情况

```txt
Built-in Function: bool __atomic_compare_exchange_n (type *ptr, type *expected, type desired, bool weak, int success_memorder, int failure_memorder)
This built-in function implements an atomic compare and exchange operation. This compares the contents of *ptr with the contents of *expected. If equal, the operation is a read-modify-write operation that writes desired into *ptr. If they are not equal, the operation is a read and the current contents of *ptr are written into *expected. weak is true for weak compare_exchange, which may fail spuriously, and false for the strong variation, which never fails spuriously. Many targets only offer the strong variation and ignore the parameter. When in doubt, use the strong variation.

If desired is written into *ptr then true is returned and memory is affected according to the memory order specified by success_memorder. There are no restrictions on what memory order can be used here.
```


```c
  bool __atomic_compare_exchange_n(
      type *ptr,
      type *expected,
      type desired,
      bool weak,
      int success_memorder,
      int failure_memorder
  );
```
核心语义：
1. 比较 *ptr 和 *expected
2. 如果相等：
    - 把 desired 写入 *ptr
    - 返回 true
3. 如果不相等：
    - 不写 desired
    - 把当前 *ptr 的值回写到 *expected
    - 返回 false

**weak**

- weak = false：强 CAS
    - 只有在 *ptr != *expected 时才会失败
    - 不会“无缘无故失败”
- weak = true：弱 CAS
    - 即使 *ptr == *expected，也允许偶尔失败，叫“spurious failure（虚假失败）”
    - 常用于循环重试，因为某些硬件上实现更便宜

典型理解：

do {
    expected = old;
} while (!__atomic_compare_exchange_n(&x, &expected, new,
                                      true, ...));

这里用 weak=true 很常见，因为失败了就继续重试。


**success_memorder**

这个表示：

- 如果比较成功，并且把 desired 写进了 *ptr
- 那么这次“读-改-写”操作要遵守什么内存序

常见值：

- __ATOMIC_RELAXED：只保证原子性，不保证额外顺序
- __ATOMIC_ACQUIRE：成功后，后续读写不能跑到它前面
- __ATOMIC_RELEASE：成功前，之前的写不能跑到它后面
- __ATOMIC_ACQ_REL：同时有 acquire + release 效果
- __ATOMIC_SEQ_CST：最强，提供全局一致的顺序感

直觉上：

- “我成功抢到这个状态后，后面才能安全读数据” -> 常用 ACQUIRE
- “我先把数据写好，再发布状态给别人看” -> 常用 RELEASE
- 如果 CAS 同时承担“获取 + 发布” -> 用 ACQ_REL

**failure_memorder**

这个表示：

- 如果比较失败，没有写入 desired
- 那这次“失败的读取”要遵守什么内存序

因为失败时只是读，没有写，所以它不能带 release 语义。

所以 GCC 要求：

- 不能是 __ATOMIC_RELEASE
- 不能是 __ATOMIC_ACQ_REL
- 不能比 success_memorder 更强

常见搭配：

```txt
  - success = __ATOMIC_ACQUIRE, failure = __ATOMIC_RELAXED
  - success = __ATOMIC_ACQ_REL, failure = __ATOMIC_ACQUIRE
  - success = __ATOMIC_SEQ_CST, failure = __ATOMIC_SEQ_CST
```

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
