# kmemleak

由于 检查内存泄露

## 实现原理

分配器通过 `kmemleak_alloc()`
注册对象，记录地址范围、大小、分配调用栈、进程信息等；释放时通过
`kmemleak_free()`
删除记录。对象同时保存在链表和红黑树中：链表用于遍历，红黑树用于根据一个候选指针快速查找它落在哪个对象的地址范围内。

扫描过程类似三色标记：

1. 将所有受跟踪对象标成白色，表示尚未证明可达；
2. 扫描内核 `.data/.bss`、per-CPU
   数据、任务内核栈和其他根对象，把找到的对象加入灰色队列；
3. 继续扫描灰色对象中的指针，递归传播可达性；
4. 扫描结束后仍为白色且超过最小存活时间的对象，被报告为疑似泄漏。

扫描把对齐的机器字保守地当成候选指针，并接受指向对象内部的指针。
因此它可能把普通整数或残留旧指针误认为有效引用，导致真实泄漏漏报。对象内容发生变化时，kmemleak
还会通过 checksum 暂缓一轮报告，以减少并发更新造成的瞬时误报。

## 不解决的问题

kmemleak 不检测 use-after-free。对象执行 `kfree()` 后就会从 kmemleak
的跟踪结构中删除，后续非法访问不再属于它的检测范围。

| 问题                              | 适合的工具 |
| --------------------------------- | ---------- |
| 内存泄漏、对象引用丢失            | kmemleak   |
| use-after-free、越界、double free | KASAN      |
| 低开销抽样检测 UAF/越界           | KFENCE     |
| 未初始化内存读取                  | KMSAN      |

kmemleak 也不是严格证明工具：

- 指针经过编码或保存在未扫描区域时，可能误报；
- 普通整数、栈上残留值或 stale pointer 可能造成漏报；
- 自定义 allocator 没有调用 kmemleak hook 时不会被追踪；
- 普通 page allocation 和 `ioremap` 默认不在完整追踪范围内；
- 引用计数泄漏如果仍存在可达指针，通常不会被发现。

## 使用

需要启用 `CONFIG_DEBUG_KMEMLEAK` 并挂载 debugfs：

```bash
echo clear > /sys/kernel/debug/kmemleak
# 执行待测操作
echo scan > /sys/kernel/debug/kmemleak
# 间隔一段时间后再次扫描，排除瞬时状态
echo scan > /sys/kernel/debug/kmemleak
cat /sys/kernel/debug/kmemleak
```

`clear` 只是忽略当前已经报告的对象，用来建立新的测试基线，并不会释放泄漏内存。

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
