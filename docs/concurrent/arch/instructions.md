# CPU 微架构

## 总结一下常见的原子指令实现，希望可以理解原子执行设计有什么考虑

参考 [OSTEP](https://pages.cs.wisc.edu/~remzi/OSTEP/threads-locks.pdf)

1. Test-And-Set
1. Fetch-And-Add
2. Load-Linked and Store-Conditional
3. Compare-And-Swap

这里对比了下 ARM 从 ll-sc 到 fetch-and-add 的之后，似乎性能有较大的提升:
https://cpufun.substack.com/p/atomics-in-aarch64

## 硬件支持上也可以分析一下
- [CPU 多核指令 —— WFE 原理](http://www.wowotech.net/armv8a_arch/499.html)
  - http://www.wowotech.net/sort/armv8a_arch : 其实 wowotech 关于 ARM 的 atomic 分析了不少内容

- umwait 指令: https://lwn.net/Articles/790920/

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
