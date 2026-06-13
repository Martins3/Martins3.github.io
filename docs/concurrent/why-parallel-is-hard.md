# 为什么并行编程如此困难

从量化中的每一个章节
- 指令集并行，
- multi CPU
- GPU
- 超级计算机
- 请求级并行

当从软件的角度来思考的时候

1. 体系结构设计各种指令
2. os 提供的系统调用 ( os 应该提供 thread 级别的并行 )
  - fork
  - futex
  - 这种系统调用的分析
  - 异步 io / io uring
  - 操作系统内部对于多核的支持
3. 硬件驱动通过 multiqueue 实现并行

4. 编译器提供的 builtin 语句

4. 语言提供的支持
  - java
  - c++
  - go
  - c : pthread
  - rust

## 工程实现
那么，可莉考考你，那哪一个环节实现并行最复杂?

难以实现，难以验证，难以调试

个人认为是 CPU 中的并行设计最难。

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
