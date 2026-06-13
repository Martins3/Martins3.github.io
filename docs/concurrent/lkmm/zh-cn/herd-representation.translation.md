# herd-representation.txt 中文译解

源文件：`tools/memory-model/Documentation/herd-representation.txt`

## 文档定位

这份文件说明：LKMM 在 herd7 里不是直接按 C 代码执行，而是先把内核原语翻译成抽象事件和关系。你可以把它看成“内核同步 API 到形式化事件图”的对照表。

## 事件图例译解

- `R`：读事件
- `W`：写事件
- `F`：栅栏事件
- `LKR` / `LKW`：加锁相关事件
- `UL`：解锁
- `RL` / `RU`：锁状态检测的成功/失败语义
- `R*` / `W*`：RMW 中的读/写事件
- `SRCU`：SRCU 特殊事件
- `po`：程序顺序
- `rmw`：读改写关系，它本身也是一种 `po`

## 对照表怎么读

表左边是内核原语，右边是它在 herd 里的事件展开方式。例如：

- `READ_ONCE` -> `R[ONCE]`
- `WRITE_ONCE` -> `W[ONCE]`
- `smp_load_acquire` -> `R[ACQUIRE]`
- `smp_store_release` -> `W[RELEASE]`
- `smp_mb` -> `F[MB]`
- `smp_rmb` -> `F[rmb]`
- `smp_wmb` -> `F[wmb]`

这说明 herd7 分析 litmus test 时，并不是执行宏实现，而是直接给这些操作贴上抽象语义标签。

## RMW 的表示

无返回值 RMW 例如 `atomic_add()` 会被表示成 `R* ->rmw W*`，但默认不等于全序。返回值型全序 RMW 例如 `atomic_add_return()` 会被表示为带 `MB` 属性的读写对。`_relaxed`、`_acquire`、`_release` 只是把属性换成对应标签。

条件 RMW 如 `cmpxchg()` 更特殊：

- 成功时：有 `R* ->rmw W*`
- 失败时：只有读事件，没有写事件

## 锁与 RCU 的表示

- `spin_lock` 会展开成锁相关的读写事件
- `spin_unlock` 是 `UL`
- `rcu_read_lock` / `rcu_read_unlock` / `synchronize_rcu` 都被当作不同类别的 fence
- `rcu_dereference` 是读事件
- `rcu_assign_pointer` 是带 release 的写事件

这反映了 LKMM 的一个重要观念：很多高层同步 API 在形式化模型里最终都落成“读、写、fence 以及它们之间的关系”。

## 这篇文档最适合什么时候用

当你已经会读 litmus test，开始想看 `linux-kernel.cat`、`linux-kernel.bell` 或 herd7 输出时，这张映射表会很有用。它能帮你把“源码里的 API”翻译成“模型里的事件”。

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
