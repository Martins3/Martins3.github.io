之前学习王爽的汇编，感觉比学校的好，但是现在发现，其实也不好

1. 他在使用 real mode ，protected mode 之类的，那个东西我一直没搞懂，我个人认为那个东西
是搞 x86 固件的人才需要处理这个，而对于 x86 固件的人，这些东西不够深入。
2. 应该用用户态的，而不是系统态，对于入门会更加方便

难道不是一直在分析: https://blogsystem5.substack.com/p/dos-memory-models

这个会更好一些: https://mariokartwii.com/armv8/

还需要用模拟器，为什么不去用 gdb 模拟，还可以熟悉 gdb


汇编，从实用性来说
1. 配合 gdb 使用， 分析 kernel crash ，用户态 crash
2. CPU 设计的前置内容

## 配套代码
code/module/asm/README.md

https://news.ycombinator.com/item?id=43140614

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
