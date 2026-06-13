# Lockless 无锁设计收集

- [A lockless ring-buffer](https://lwn.net/Articles/340400/)
- [Lockless algorithms for mere mortals](https://lwn.net/Articles/827180/)
- [An introduction to lockless algorithms](https://lwn.net/Articles/844224/) :
- [Lockless patterns: relaxed access and partial memory barriers](https://lwn.net/Articles/846700/)

1：工程上的“无锁”
  比如：

- Treiber stack
- Michael-Scott queue
- RCU 读侧
- 基于原子变量的 ring buffer


## 收集一些内核中 lockless 的内容

- for_each_shadow_entry_lockless
  - [ ] lockless 的实现方法
  - [ ] 为什么需要 lock
  - [ ] fast_page_fault 是怎么回事

本来以为 lockless 实际上没有什么作用，但是实际上在 Linux 中是存在很多使用的

https://news.ycombinator.com/item?id=35684232

## 具体案例
- https://github.com/DNedic/lockfree

失望，这个 queue 的实现是 single writer

1. lockfree 不代表性能好，只是说 thread 的执行流而已
2. lockfree 可以 atomic

## 这里存在一整个系列
- https://lwn.net/Articles/844224/


11.7 验证什么

区分 parallel 和 concurrent :
1. 前者没有依赖，后者存在依赖。
2. 对于 schdeduler 的要求。

补充下 stackover flow 中的回答。

如何回答这个问题？


关于为什么 software 为什么会出现 bug 的分析，我认为是非常有道理的，人是靠不住的，至少
- 靠验证，靠编译器，靠工具


## 如果不使用 spinlock , mutex ，会有 dead link 吗?


## 是这样的吗?
3. 写 lockless 算法时最容易犯的几个错：ABA、memory ordering、reclamation

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
