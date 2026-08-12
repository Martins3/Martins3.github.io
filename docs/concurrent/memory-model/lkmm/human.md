https://lwn.net/Articles/799218/

## rcu 的邮件，每一个都需要阅读下
- https://lore.kernel.org/rcu/E73E4593-DFA7-4A46-924C-3867CC0B4807@gmail.com/T/#m74d0caa2ea47bfc8fe369d18f772ca1244616d9a
  - 这里的 LKMM 是什么含义?
  - 里面的小测试都搞一下

也许最终可以来帮助 Paul 来 review patch 吧

## 为什么内核有定义了一个 LKMM ?
https://mp.weixin.qq.com/s/ooNq32HCF4PmKoirsczDtg

## 资源
- 工具链：`herdtools7` — 包含 `herd7` (模拟器) 和 `klitmus7` (硬件测试生成器)

https://pauillac.inria.fr/~maranget/papers/asplos2018.pdf

全序关系 如何理解?
Happens-Before

**From-Reads (fr)**
**Reads-From (rf)**
有区别？

cat 语言?

- [herdtools7 GitHub](https://github.com/herd/herdtools7)
- [diy7 文档](https://diy.inria.fr/doc/index.html)

- [ASPLOS 2017 Memory Model Verification](https://research.nvidia.com/sites/default/files/pubs/2017-04_Automated-Synthesis-of/ASPLOS_2017_Memory_Model_Verification.pdf)


- [What every systems programmer should know about concurrency](https://www.cl.cam.ac.uk/~pes20/weakmemory/cacm.pdf)
- [Weak vs Strong Memory Models](https://preshing.com/20120930/weak-vs-strong-memory-models/)
- [Memory Barriers Are Like Source Control Operations](https://preshing.com/20120710/memory-barriers-are-like-source-control-operations/)


- [LWN: Who ordered memory fences on an x86?](https://lwn.net/Articles/720550/)
- [LWN: Memory models](https://lwn.net/Articles/470681/)
- [Write once, herd everywhere (LPC)](https://lpc.events/event/7/contributions/653/attachments/605/1087/Write_once_herd_everywhere.pdf)
- [Memory Barriers: a Hardware View for Software Hackers](http://www.rdrop.com/users/paulmck/scalability/paper/whymb.2010.07.23a.pdf)
- [perfbook](https://git.kernel.org/pub/scm/linux/kernel/git/paulmck/perfbook.git)


- [内核 tools/memory-model/README](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/tools/memory-model/README)
- [内核 Documentation/litmus-tests.txt](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/litmus-tests.txt)


linux-drm/tools/testing/selftests/membarrier/ 这是很小的测试，按道理可以吸收进来

还有几个工具:
**MemAlloy**: 用于验证和比较内存模型的 Alloy 模型
**Nemos**: 另一个内存模型工具
**rmem**: ARM 的内存模型探索工具

## 关键目录

/home/martins3/data/kernel/linux-drm/tools/memory-model/
/home/martins3/data/kernel/linux-drm/Documentation/dev-tools/lkmm/

## README 内容总结

- 新手从 `simple.txt` 开始。
- 想先认识原语分类，从 `ordering.txt` 开始。
- 想直接写 litmus test，从 `litmus-tests.txt` 开始。
- 想系统理解 LKMM，本体文档是 `explanation.txt`。

如果你的目标是“会写代码并能解释为什么对”，`README -> simple -> ordering -> recipes -> locking -> litmus-tests` 是最稳的路线。


tools/memory-model/Documentation/cheatsheet.txt
先设计同步模式，再用这张表核对，不要反过来只靠查表拼代码。


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
