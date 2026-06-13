# locking.txt 中文译解

源文件：`tools/memory-model/Documentation/locking.txt`

## 文档定位

这篇文档专门讨论一个常见陷阱：共享变量通常受某把锁保护，但你又想在某些路径上不持锁地访问它们。原文的核心观点是：锁本身能提供很多排序，但一旦脱离锁语境，常常还需要额外同步。

## 逐节译解

### Locking

最重要的一句话是：持有某把锁的 CPU，能够看到任何 CPU 在上一次释放同一把锁之前已经看到或已经做出的变化。这比“看到临界区内部的写”更强，因为它还把锁前后的可见性链起来了。

### Locking and Prior Accesses

如果 CPU0 在加锁前写了 `x`，在锁内写了 `y`，CPU1 在锁内读到 `y=1`，那么 CPU1 在锁外随后读 `x` 时也必须能读到 `1`。也就是说，锁不只保护锁内访问，还把某些锁前访问带进了可见性链。

### Locking and Subsequent Accesses

反过来也成立。如果 CPU0 在锁内读 `x` 时仍然看到旧值，那么它在加锁前读到的某些相关状态也必须仍是旧值。锁不仅能“带入过去”，也能“排除未来”。

### Double-Checked Locking

双重检查锁定不是只加一把锁就自然正确。问题有两个：

- 外层第一次读 `flag` 与后面读 `data` 之间没有排序。
- 初始化时写 `data` 与写 `flag` 之间没有排序。

修法是用 `smp_load_acquire()` 和 `smp_store_release()` 给外层无锁路径补同步边。

### Ordering Provided by a Lock to CPUs Not Holding That Lock

锁建立的顺序不一定会自动扩展给从未持有该锁的第三个 CPU。也就是说，锁序本身不是无条件全局广播。如果要把锁中的顺序延伸给锁外观察者，可能需要 `smp_mb__after_spinlock()` 之类的补强。

### No Roach-Motel Locking!

所谓 “roach motel locking” 指把原本在临界区外的访问偷偷拉进临界区，以为这样只会更强。文档强调这种变换在一般情况下不成立，尤其是当锁前有自旋等待或观察逻辑时。原因是：把访问搬进临界区会改变哪些执行结果允许出现。

## 这篇文档的工程含义

锁是强同步原语，但“受锁保护的数据可以随便无锁看一眼”是错误直觉。只要离开锁，就重新回到 LKMM 需要显式论证的世界。

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
