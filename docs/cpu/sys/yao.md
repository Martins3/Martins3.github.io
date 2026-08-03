# 超标量处理器设计 : 姚永斌

1. 为什么半导体的工艺提升可以带来频率的提升?
  - 是距离的缩短，还是供电，还是散热带来的

2. 超长指令字和 SIMD 可以算是一个东西吗?
  - 答曰，完全不相关的东西，VLIW 是靠编译器将多个指令放到一起，构建出来一个例子，而 SIMD 是一个指令
  - https://news.ycombinator.com/item?id=10872973
  - https://stackoverflow.com/questions/70400206/are-simd-and-vliw-instructions-the-same-thing

一般来说，fetch decode 是顺序，issue exec wb 是乱序，而 commit 是顺序

# 2 Cache

## 2.1.2

I-Cache 不会直接写入，即使代码自修改，也会是借助 D-Cache 实现。

- 忽然发现，即便是单核的 CPU 的 cache 设计，也有 L1 L2 L3 cache 的同步问题

在 D-Cache 中，Write Through 总是和 Non-Write Allocate 配合使用，将数据直接更新到下级 cache
而 Write Back 和 Write Allocate 配合使用。

在全相连的路线中，需要使用 LRU 来确定谁来

数字电路实现 LRU 的基本方法: 使用一个二叉树方式，但是这个只是一个近似的方法，二叉树中的一个 leaf 描述
node left 还是 right 中最近是否被访问过，但是这个方法不好，因为


## 2.2 提高 Cache 的性能

- 2.2.1 写缓存
- 2.2.2 流水线 : 如果读取 D-Cache ，可以将 Tag SRAM 和 Data SRAM 可以同时进行，
但是对于写，Tag SRAM 的读取和写 Data SRAM 有依赖关系，需要先访问了 Tag SRAM 之后，才可以知道
将 Tag SRAM 的读取和比较放到一个周期，写 Data SRAM 放到一个周期
- 2.2.3 多级 Cache
- 2.2.4 Victim Cache
- 2.2.5 预取

## 2.3 多端口 Cache

基本思想，使用 multiple bank 的方式

## 2.4 超标量的取指令

TODO

## 总结
似乎 cache 比想象的简单，但是我们关心的 cache cohenrence 没有分析。

## TODO
- https://www.zhihu.com/question/605427422/answer/3068622311
  - 看完之后，感觉不是在说一个东西啊

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
