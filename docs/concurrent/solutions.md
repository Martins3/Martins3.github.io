## 并发编程中 lockless 是什么意思
<!-- 7a70407f-aa20-4fb1-bdaf-36733ee7889d -->

codex 解答:

- “lockless 算法”这个词，严格说通常 不是一个特别精确的学术术语。更精确的术语一般是这些：

1. blocking
2. obstruction-free
3. lock-free
4. wait-free

很多人平时说 lockless，往往只是想表达：

- 没有使用 mutex/spinlock/rwlock 这类传统锁
- 线程之间主要靠 atomic、CAS、内存序来协调
- 希望避免“拿锁睡眠”或“锁竞争串行化”

但这还不够精确，因为“没写 lock()/unlock()”不等于“有严格的非阻塞进度保证”。

如果你问“什么样的算法算 lockless”，常见有两种口径：

### 口径 1：工程上的“无锁”

只要算法不依赖传统锁，而是依赖原子操作实现同步，很多工程师就会叫它 lockless 或 lock-free style。

比如：

- Treiber stack
- Michael-Scott queue
- RCU 读侧
- 基于原子变量的 ring buffer

但这里面有些算法其实只是“看起来没锁”，并不一定有严格 progress guarantee。

### 口径 2：理论上的“非阻塞”

在并发算法理论里，更精确的是按“进度保证”分类：

- obstruction-free
  线程单独运行时能完成；一旦有竞争，不保证谁能前进
- lock-free
  整个系统总会有某个线程在有限步内前进；但某个具体线程可能永远饿死
- wait-free
  每个线程都保证在有限步内完成；这是最强保证

这里真正精确的是 lock-free、wait-free，而不是 lockless。

### 举几个例子

1. 自旋 CAS 循环

do {
    old = atomic_load(&x);
    new = f(old);
} while (!atomic_compare_exchange_weak(&x, &old, new));

这通常会被叫做 lockless。

但它是否 lock-free，要看整体算法设计。
单看这段代码，只能说明它“没显式加锁”，不能自动推出完整的 progress guarantee。

2. RCU

RCU 读侧常被称为 “lockless read path”。

这在工程上很合理，因为读者几乎不加锁。
但 RCU 整体并不等价于“所有操作都是 lock-free”；宽限期推进、回收机制有自己的一套同步模型。

3. Seqlock

读者不拿传统锁，只是读 sequence 重试。
很多人会说“读路径 lockless”。但它并不是对所有线程都提供严格的非阻塞完成保证，尤其读者可能一直重试。

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
