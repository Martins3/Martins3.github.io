## perf book
https://mirrors.edge.kernel.org/pub/linux/kernel/people/paulmck/perfbook/perfbook.html

git clone https://github.com/paulmckrcu/perfbook
所以，现在就可以拿着这个文档直接问问题了

# 检查这些原子语句都是如何实现的

## lock 是靠什么保证这些的

> No need to worry about which CPU
did or did not execute a memory barrier, no need to worry
about the CPU or compiler reordering operations—life is
simple.

是靠 lock 的代码实现，还是靠硬件实现的?


## 形式化验证指的是什么?

形式化验证似乎除了可以处理并行，还是可以处理其他的 (Formally Verified)
- 形式化验证 kvm :  https://alastairreid.github.io/RelatedWork/papers/li:sandp:2021/
  - http://nieh.net/pubs/ieeesp2021_kvm.pdf

## if so 我们的核心挑战是什么? (2023/10/16)

rcu 和 memory barrier 的问题，都是两章，这是重点，其他的慢慢感悟吧。
分别存在两章

## 工具上
1. 反汇编
2. 找到 cpp 的标准库

## 代办

- [ ] 将 x86, ARM, RISC-V 的 memory model 整理为表格
- [ ] 构建一个 memory model 测试程序
  - [ ] 所有的想法验证都是需要一个测试程序

## 实际上 perfbook (2024/2/25) 真正不知道在搞什么的

其实也就是花费了几天时间，感觉 rcu 和 memory barrier 也不是不可逾越的。

- Partitioning and Synchronization Design
- Data Ownership

- Validation
- Formal Verification

## 重新思考 perfbook 该如何阅读 (2025-07-18)

我们让 gemini 2.5 pro

12 章的例子可以仔细看看

理解第 4 章的:

锁争用（Contention）对性能的严重影响，如缓存行伪共享（False Sharing）和缓存行弹跳（Cache-line Bouncing）；以及锁本身的开销。

关于 Cache Coherency，仔细阅读 chapter 2 就可以了。

然后看看 Data Structure 的功能发a

## 源码
https://git.kernel.org/pub/scm/linux/kernel/git/paulmck/perfbook.git

CodeSamples/formal/herd/Makefile 这里如何使用来着?

## is perfbook hard, if so, what can you do about it ?

首先看看 /home/martins3/core/linux/Documentation/kernel-hacking/locking.rst
内核的经典入门材料。

- memory model 的难点，没有找到齐全的测试，这个我们也许直接找下，但是自己写也是很锻炼人的
- RCU : kernel 的 RCU 和各种东西耦合很强

## 有人做了一个翻译
https://github.com/pengdonglin137/perfbook_cn

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
