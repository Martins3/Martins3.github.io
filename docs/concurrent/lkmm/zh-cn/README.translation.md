# README 中文译解

源文件：`tools/memory-model/Documentation/README`

## 文档在说什么

这份 `README` 不是讲某个具体内存序原语，而是在说明整套 LKMM 文档应该怎么读。原文先强调一个现实问题：LKMM 的读者背景差异极大，有的人是并发新手，有的人已经熟悉内核同步原语，有的人只是想写 litmus test，还有的人想读形式化模型本身。

因此，这份文档给出的不是单一阅读顺序，而是“按目标选入口”的阅读地图。它提醒读者：越靠后的文档越假定你已经理解前面的材料，所以最好按自己的背景从合适的位置切入。

## 阅读路径译解

- 如果你刚接触 Linux 内核并发，先读 `simple.txt`。
- 如果你已经知道一些并发，但想快速了解内核提供了哪些低层内存序原语，读 `ordering.txt`。
- 如果你已经知道自己要用哪些原语，只想开始写 LKMM litmus test，读 `litmus-tests.txt`。
- 如果你要在不持锁的情况下访问通常受锁保护的共享变量，读 `locking.txt`。
- 如果你想建立对 LKMM 的直觉性理解，尤其是涉及两个以上线程的场景，读 `recipes.txt`。
- 如果你担心编译器破坏控制依赖，读 `control-dependencies.txt`。
- 如果你要处理 KCSAN 报告、标注共享内存访问、区分故意的数据竞争，读 `access-marking.txt`。
- 如果你已经在日常使用 LKMM，只想查表，读 `cheatsheet.txt`。
- 如果你想读 LKMM 的要求、动机和形式化实现，读 `explanation.txt` 与 `herd-representation.txt`。
- 如果你想追溯论文、硬件手册、LWN 文章和工具文献，读 `references.txt`。

## 各文件的角色

- `access-marking.txt`：解释如何标注共享内存访问，以及何时用 `READ_ONCE()`、`WRITE_ONCE()`、`data_race()`、`__data_racy` 等。
- `cheatsheet.txt`：一页速查表，告诉你每种原语大概能约束哪些前后访问。
- `control-dependencies.txt`：解释控制依赖为什么脆弱，以及如何不被编译器优化掉。
- `explanation.txt`：从概念、关系、约束到形式化语义，系统说明 LKMM。
- `glossary.txt`：术语表。
- `herd-representation.txt`：说明 herd7 中 LKMM 原语被抽象成哪些事件和关系。
- `litmus-tests.txt`：介绍 litmus test 的格式、语法、限制和调试方法。
- `locking.txt`：专门讲“锁保护数据的无锁访问”这个容易写错的话题。
- `ordering.txt`：按类别梳理 barrier、acquire/release、RCU、control dependency 等。
- `recipes.txt`：给出常见并发模式的“配方”和对照用例。
- `references.txt`：背景材料索引。
- `simple.txt`：给想先把问题简化的人看的实用建议。

## 这份 README 的使用方式

把它当成导航页，而不是知识点本身。真正学 LKMM 时，最常见的路径是：

1. `simple.txt`
2. `ordering.txt`
3. `recipes.txt`
4. `litmus-tests.txt`
5. `explanation.txt`

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
