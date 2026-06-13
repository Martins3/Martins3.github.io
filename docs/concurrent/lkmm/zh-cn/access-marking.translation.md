# access-marking.txt 中文译解

源文件：`tools/memory-model/Documentation/access-marking.txt`

## 文档定位

这篇文档讨论的不是“如何建立排序”，而是“你打算如何把共享内存访问标注出来，以及如何向人和 KCSAN 解释这是有意为之”。它尤其适合处理“明知有数据竞争，但这是设计的一部分”这种情况。

## 可用的访问标注手段

原文列出六类选择：

1. 普通 C 访问，不做标注。
2. `data_race(...)`
3. `READ_ONCE()`
4. `WRITE_ONCE()`
5. `__data_racy`
6. KCSAN 的断言类标注，如 `ASSERT_EXCLUSIVE_ACCESS()`、`ASSERT_EXCLUSIVE_WRITER()`

文档强调一个排序偏好：

- 若一个访问确实参与了有意的数据竞争，通常优先考虑 `READ_ONCE()` / `WRITE_ONCE()`。
- 如果只想告诉 KCSAN “这是故意竞争，不要报”，而不想约束编译器，则可以用 `data_race()`。
- plain C 访问通常应留给“本来就不该有并发冲突”的场景。

## `data_race()` 适合什么情况

### 近似诊断

例如 `/proc`、`/sys`、统计、调试输出、非关键 `WARN` 检查等。这里读到近似值通常没问题，反而如果你把这些诊断性访问都升级成 `READ_ONCE()`，可能会让 KCSAN 更难暴露真正的核心竞争。

### 读出来还会做二次校验

例如先用一个可能竞争的旧值去喂给 `cmpxchg()`，失败后再重试。因为最终有一个带标记的重读或 RMW 校验，所以偶尔读到奇怪值是可恢复的。

### 喂给容错启发式

如果读到偶发错误值不会破坏系统，只会让启发式判断稍微偏一点，可以用 `data_race()`。但前提是启发式对所有可能错值都安全。

### 供启发式读取的写入

如果读侧是容错启发式，那么写侧很多时候也可以接受 `data_race()` 或普通写，只要最终稳定值会落到位。

## plain C 访问适合什么情况

原文给出几类典型场景：

- 被互斥机制完全保护的访问。
- 初始化和清理阶段。
- 不会被别的 CPU 访问的 per-CPU 数据。
- 私有任务数据和栈变量。
- 设计上不应该存在并发写的读。
- 设计上不应该存在并发读写或并发写写的写。

文档特别强调：这些场景故意保留 plain C，是为了让 KCSAN 在同步规则被破坏时能报出来。如果你把所有访问都包成 `READ_ONCE()`，反而可能掩盖 bug。

## `__data_racy`

`__data_racy` 是声明级别的“默认按 `data_race()` 处理”。它告诉 KCSAN 这个变量的所有访问都可能是有意竞争的。但它不自动约束编译器，所以不能拿它替代 `READ_ONCE()` / `WRITE_ONCE()` 的编译器层面语义。

## 访问文档化

除了代码标记，文档还强调要写注释说明同步设计。因为以后读代码的人首先要理解“为什么这里可以竞争”，其次才是“这里用了什么宏”。

KCSAN 断言的作用则是把你的同步假设明确交给工具：

- `ASSERT_EXCLUSIVE_ACCESS(x)`：任何并发访问都算错。
- `ASSERT_EXCLUSIVE_WRITER(x)`：并发读可以，但并发写不行。

## 这篇文档的真正重点

访问标记不是“消音器”。它应该服务于两个目标：

- 限制编译器做危险优化。
- 把你的同步设计明确告诉读代码的人和 KCSAN。

如果只是看到 KCSAN 报告就机械地加 `data_race()` 或 `READ_ONCE()`，那通常是在把问题藏起来。

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
