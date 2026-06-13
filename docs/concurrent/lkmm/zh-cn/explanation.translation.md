# explanation.txt 中文译解

源文件：`tools/memory-model/Documentation/explanation.txt`

## 文档定位

`explanation.txt` 是 LKMM 的核心说明书。它不是速查表，也不是写代码时随手翻的 cookbook，而是在解释：

- 为什么需要 LKMM
- LKMM 在形式上到底描述什么
- 这些 `po`、`rf`、`co`、`fr`、`ppo`、`hb`、`pb`、`rcu-order` 等关系各自是什么意思
- 为什么某些并发结果允许，某些结果不允许

这份中文辅助材料采用“章节级译解”的方式帮助你抓住主线。

## 1. INTRODUCTION

原文开头先承认：LKMM 很复杂，直接看 `linux-kernel.bell` 与 `linux-kernel.cat` 会非常晦涩。本文件的作用，是用自然语言解释这些形式化定义在表达什么，而不是逐行注释 cat 文件本身。

## 2. BACKGROUND

内存模型的基本问题是：给定一段并发代码，load 可能读到哪些值。实践中大家常反过来用它：给定某个结果，问这个结果能不能发生。LKMM 要覆盖 Linux 支持的多种架构，因此必须足够宽松，允许任何某个支持架构可能允许的行为。

## 3. A SIMPLE EXAMPLE

文档用最经典的消息传递例子开场：

```c
P0: WRITE_ONCE(buf, 1); WRITE_ONCE(flag, 1);
P1: r1 = READ_ONCE(flag); if (r1) r2 = READ_ONCE(buf);
```

直觉上很多人会以为只要看到 `flag=1` 就一定能看到 `buf=1`。但 LKMM 明确允许 `r1=1 && r2=0`。这说明“源码顺序”不等于“跨 CPU 的可见顺序”。

## 4. A SELECTION OF MEMORY MODELS

这一节把 LKMM 放到更大背景里：

- 顺序一致性 `SC` 最容易理解，但现实硬件很少这么强。
- `TSO` 比 SC 弱，例如允许 Store Buffering 模式出现双零结果。
- ARM、PowerPC 等更弱，允许更多反直觉结果。

LKMM 借鉴了这些模型，但不完全等同于任何一个硬件模型。

## 5. ORDERING AND CYCLES

内存模型本质上是“限制哪些排序必须成立”。一旦若干必须成立的关系形成环，就意味着某个结果不可能发生。LKMM 的很多公理，最终都可以理解成“某种关系不允许成环”。

## 6. EVENTS

LKMM 不直接操作 C 语句，而是把并发相关动作抽象成事件：

- 读事件
- 写事件
- fence 事件

RMW 会被拆成读事件加写事件。共享内存以外的计算、分支、私有寄存器等不直接形成事件，但会通过依赖关系间接影响模型。

## 7. THE PROGRAM ORDER RELATION: `po` AND `po-loc`

`po` 是程序顺序，只在同一个 CPU 内成立。它可以近似理解成“进入执行单元的指令顺序”。`po-loc` 是同地址上的程序顺序子关系。

这一节非常重要，因为 LKMM 虽然以 C 源码为输入，但真正要描述的是“编译器和 CPU 最终保留下来的可观察顺序”。这也是为什么共享访问通常要用 `READ_ONCE()` / `WRITE_ONCE()` 去约束编译器。

## 8. A WARNING

这一节专门讲编译器如何破坏你以为存在的顺序。例如 if/else 两边写同一个值时，编译器可能把写提到条件分支外。还有函数实参求值顺序本身就不固定。结论很明确：LKMM 不是“只管 CPU 不管编译器”的模型。

## 9. DEPENDENCY RELATIONS: `data`, `addr`, AND `ctrl`

三种依赖关系的直觉是：

- `data`：后面的写值依赖前面的读值
- `addr`：后面的访问地址依赖前面的读值
- `ctrl`：后面的写是否执行依赖前面的读值

文档同时反复提醒：很多看上去像依赖的代码只有“语法上的依赖”，没有“语义上的依赖”，编译器可以把它优化掉。

## 10. THE READS-FROM RELATION: `rf`, `rfi`, AND `rfe`

`rf` 表示某个读读到了哪个写。跨 CPU 的 `rfe` 特别重要，因为它天然意味着时间必须从写推进到读，所以经常能成为建立 `hb` 的关键边。相比之下，同 CPU 的 `rfi` 可能只是 store forwarding，并不代表真正执行顺序。

## 11. CACHE COHERENCE AND THE COHERENCE ORDER RELATION: `co`, `coi`, AND `coe`

`co` 描述同一地址上的写覆盖顺序，也就是一致性顺序。它不是执行顺序，而是“内存系统最终裁定哪个写覆盖了哪个写”的顺序。因此 `co` 能帮助排除某些结果，但不能简单地当作“前一个写一定先执行”。

## 12. THE FROM-READS RELATION: `fr`, `fri`, AND `fre`

`fr` 连接“某次读到的那个值”与“后来把该值覆盖掉的写”。它和 `co`、`rf` 一起构成很多 litmus test 中的关键闭环。读一个值、然后另一个写来晚一步没赶上，这个关系正是 `fr` 想表达的事情。

## 13. AN OPERATIONAL MODEL

原文在这一段给出一个操作式视角：CPU 发出读写请求，内存系统传播、缓存接收、store forwarding 等。这个操作式模型不是 LKMM 最终暴露给用户的接口，但它帮助说明为什么某些 axiomatic 关系是合理的。

## 14. PROPAGATION ORDER RELATION: `cumul-fence`

这节开始进入 LKMM 的关键特色之一：不仅要关心“执行顺序”，还要关心“传播顺序”。`cumul-fence` 描述的是某些 barrier 不只约束本 CPU 本地顺序，还能把别处已经传播过来的效果继续往前带。

这是理解 MP、RCU、以及多 CPU 链式传播的基础。

## 15. DERIVATION OF THE LKMM FROM THE OPERATIONAL MODEL

这一节在做桥接：前面讲了操作式直觉，后面要进入公理化模型。原文解释了诸如 `hb`、`pb`、propagation 关系为何这样定义，以及这些定义怎样对应到可能的执行。

## 16. SEQUENTIAL CONSISTENCY PER VARIABLE

LKMM 不是全局 SC，但同一个变量上仍然保留了较强的一致性约束。也就是说，系统整体可以弱排序，但单变量上的读写不能完全胡来，否则连最基本的 cache coherence 都丢掉了。

## 17. ATOMIC UPDATES: `rmw`

RMW 是 LKMM 中的特殊访问，因为它兼具读和写。成功的条件 RMW 与失败的条件 RMW 在模型里不一样；返回值型与不返回值型的排序强度也不一样。不能把所有 atomic API 当成一种东西看待。

## 18. THE PRESERVED PROGRAM ORDER RELATION: `ppo`

`ppo` 可以理解为“即便在弱内存硬件上，也仍被保留下来的程序顺序片段”。它来自依赖、barrier、同地址限制等多种来源。很多时候你写下的 `po` 会被硬件打散，而 LKMM 真正依赖的是被保留下来的 `ppo`。

## 19. AND THEN THERE WAS ALPHA

这一节讲 Alpha 的特殊性：它连地址依赖到 load 都可能不保。正因为 Alpha 太弱，Linux 在 Alpha 上会自动在某些读后面补 fence。文档借此提醒你：内核模型必须向最弱支持架构看齐，而不是向你手头的 x86 机器看齐。

## 20. THE HAPPENS-BEFORE RELATION: `hb`

`hb` 是 LKMM 的核心关系之一。它收集了：

- `ppo`
- 跨 CPU 的 `rfe`
- 以及由 coherence/propagation 间接推导出来的一些顺序

如果 `hb` 成环，就意味着某个结果要求“一个事件先于自己”，因此被禁止。

## 21. THE PROPAGATES-BEFORE RELATION: `pb`

`pb` 比 `hb` 更偏向“传播到所有 CPU 之前”的语义。它依赖强 fence，描述某些写不仅先执行，而且必须在另一个事件发生前传播到所有 CPU 与 RAM。它是理解强 barrier、全局可见性和某些多 CPU 推理的关键。

## 22. RCU RELATIONS: `rcu-link`, `rcu-gp`, `rcu-rscsi`, `rcu-order`, `rcu-fence`, AND `rb`

这一节把 RCU 纳入 LKMM。重点不是“RCU 内部实现”，而是“grace period 与 read-side critical section 在模型里如何形成强排序约束”。

可以把 `rcu-order` 理解成一种跨 CPU 的“超强 fence 链”。它表达的是：某些写必须在 grace period/读侧边界的约束下传播完成，否则就会违反 RCU 保证。

## 23. SRCU READ-SIDE CRITICAL SECTIONS

SRCU 与普通 RCU 类似，但又多了 domain、index 和跨上下文匹配等特性。因此 LKMM 需要给 `srcu_read_lock()` / `srcu_read_unlock()` / `synchronize_srcu()` 引入不同于普通 RCU 的建模方式。

## 24. LOCKING

这一节把锁纳入形式化关系里。核心不是告诉你“锁能互斥”，而是形式化地说明锁如何与 `po`、`hb`、`pb` 等关系互动，以及为什么有些锁外观察者看不到你想象中的全局顺序。

## 25. PLAIN ACCESSES AND DATA RACES

这是和日常内核代码最相关的一节之一。它解释了 LKMM 为什么对 plain C 访问采取更保守、更特殊的处理：

- plain 访问可能被编译器拆分、融合、复制、删除或发明
- 大多数正式关系默认只作用于 marked access
- 为了分析数据竞争，LKMM 通过“pre-bound / post-bound”等思路，把 plain 访问用周边带标记访问和 compiler barrier 间接夹住

这一节的实质是在回答：为什么 plain access 能用，但证明它正确很麻烦。

## 26. ODDS AND ENDS

最后一节收尾补充若干零碎但重要的点，例如：

- 为什么某些关系只加到 `hb`，某些不加
- plain access 是否会间接参与 `ppo`
- RCU 与其他关系的交织边界

## 读完这篇你应该得到什么

不是把每个关系都背下来，而是建立三层心智模型：

1. 代码先被抽象成读、写、fence 事件。
2. 事件之间通过 `po`、依赖、`rf`、`co`、`fr`、`ppo`、`hb`、`pb`、RCU 关系等连接起来。
3. 某个结果是否允许，取决于这些必须成立的关系能否同时成立而不成环。

## 推荐的配套阅读顺序

如果你要真正消化 `explanation.txt`，建议配合这几份文档一起读：

1. `glossary.txt`
2. `ordering.txt`
3. `recipes.txt`
4. `litmus-tests.txt`
5. `herd-representation.txt`

这样读下来，再回看 `linux-kernel.cat` 和 litmus test 输出，会轻松很多。

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
