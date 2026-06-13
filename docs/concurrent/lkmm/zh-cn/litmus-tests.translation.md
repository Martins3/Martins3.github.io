# litmus-tests.txt 中文译解

源文件：`tools/memory-model/Documentation/litmus-tests.txt`

## 文档定位

这篇文档讲的是 LKMM litmus test 的格式、调试方法和边界。它的目标不是教你某条 barrier 的语义，而是教你如何把一个并发问题压缩成一个可被 herd7 穷举分析的小模型。

## Copy-Pasta

原文一开始就给出非常实用的建议：不要从零写测试，优先改已有 litmus test。源码树中的 `tools/memory-model/litmus-tests/` 和 `Documentation/litmus-tests/`，以及外部仓库里已经有大量现成样例。

## Examples and Format

文档用一个消息传递 MP 示例说明 litmus test 的基本格式：

- 第一行 `C <name>`：声明这是 LKMM C 风格 litmus test，并给出测试名。
- 初始化块 `{}`：给全局变量赋初值，省略时默认全零。
- `P0`、`P1` 等函数：表示并发执行的进程。
- 形参列表里的指针名：就是共享全局变量名。
- 局部变量：通常命名为 `r0`、`r1` 等。
- 最后的 `exists (...)`：描述你关心的终态是否可能出现。

## Message-Passing Example

MP 例子测试的是：`smp_store_release()` 与 `smp_load_acquire()` 是否足以阻止“看到 flag=1 但 data 还是旧值”的坏结果。herd7 输出中的 `Never`、`Sometimes`、`Always` 是最重要的三种判定。

## herd7 输出怎么读

- `States N`：总共有多少种终态。
- 每一行状态：列出某个进程局部寄存器最终可能取值。
- `Observation ... Never/Sometimes/Always`：`exists` 条件是否成立。

调试时，完整状态列表往往比最终结论更有价值，因为它能告诉你自己到底漏掉了哪条同步边。

## Initialization

如果变量初始值不是零，就必须显式写在初始化块里。litmus test 的关键之一就是：把你依赖的初始状态说清楚，否则分析结果会偏。

## Control Structures

LKMM litmus test 支持的是一个很小的 C 子集。你不能指望把真实内核函数直接贴进去跑。它更像“面向并发语义的伪代码”，而不是完整 C 解释器。

## Tricks and Traps

文档提醒了若干常见坑：

- 忘记把共享变量写进进程形参列表，结果它被当成局部变量。
- 误以为 `exists` 用的是 C 表达式语法，实际上它有自己的布尔表达式语法。
- 过度依赖复杂控制流，导致测试不是在验证内存序，而是在验证你自己写的测试。

## Debug Output

当测试结果不符合预期时，应优先看状态枚举和 witness 信息，而不是只看最后的 `Never`/`Sometimes`。调试 litmus test 本身也是一项工作。

## Spin Loops / Linked Lists / Comments / Asynchronous RCU Grace Periods

这几节说明了一些高级技巧和局限：自旋循环需要特殊建模；链表和 RCU 场景可以表达，但要遵守 litmus 语言支持的边界；注释也受多层解析器影响，不完全像普通 C 文件那样随便写。

## Performance

测试越大，状态空间越容易爆炸。litmus test 的设计目标应该是“小而准”，只保留和同步结论直接相关的最少结构。

## LIMITATIONS

LKMM litmus test 不是完整内核，也不是完整 C。它适合验证“这个同步模式是否允许某个结果”，不适合直接模拟复杂实现细节。

## 这篇文档的真正价值

它教你一种工作流：

1. 把问题抽象成少量共享变量和少量线程。
2. 把想禁止的结果写进 `exists`。
3. 用 herd7 让模型告诉你：这个结果是 `Never` 还是 `Sometimes`。

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
