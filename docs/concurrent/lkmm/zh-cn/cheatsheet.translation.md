# cheatsheet.txt 中文译解

源文件：`tools/memory-model/Documentation/cheatsheet.txt`

## 文档定位

这是一张“排序能力速查表”。横向看“前面的操作是什么”，纵向看“后面的操作是什么”，中间的 `Y` 表示该原语能保证这种前后顺序。

## 表中各行的中文理解

- `Relaxed store`：如 `WRITE_ONCE()`。它只把“自己这次 store”固定下来，不负责把前后其他访问排序好。
- `Relaxed load`：如 `READ_ONCE()`。它也只是一次受控 load，不提供通用跨访问排序。
- `Relaxed RMW operation`：如 `_relaxed` 变体，或一些不带返回值的 RMW，排序能力有限。
- `rcu_dereference()`：主要通过依赖和 RCU 语义约束后续基于该指针的访问。
- `Successful *_acquire()`：对“自己之后”的访问生效，保证 load 之后的读写不会越过它。
- `Successful *_release()`：对“自己之前”的访问生效，保证之前的读写不会越过它。
- `smp_rmb()`：约束前面的读和后面的读。
- `smp_wmb()`：约束前面的写和后面的写。
- `smp_mb()` 与 `synchronize_rcu()`：最强，几乎把前后所有受关注访问都隔开。
- `Successful full non-void RMW`：成功的全序返回值 RMW，本身就相当强。
- `smp_mb__before_atomic()` / `smp_mb__after_atomic()`：给弱排序 RMW 补边界。

## 关键术语译解

- `C`：cumulative，排序具有累积传播能力。
- `P`：propagates，强调传播上的强保证。
- `R`：read。
- `W`：write。
- `DR`：dependent read，依赖读。
- `DW`：dependent write，依赖写。
- `RMW`：read-modify-write。
- `SV`：same variable，约束同一变量上的后续访问。

## 怎么实际使用这张表

这张表最适合做“第二步确认”，不适合做“第一步设计”。先根据并发模式选 acquire/release、barrier 或锁，再回到这张表核对你需要的顺序是否真的被覆盖。

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
