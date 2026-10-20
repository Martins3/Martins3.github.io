# pcie 硬件基础

是这样的吗?

因为 switch 和 bridge 描述的是不同层面的东西：
switch 是交换拓扑和硬件功能，bridge 是 PCI 软件模型中连接两条总线的逻辑结构。
一个 PCIe switch 在软件看来，就是多个 PCI-to-PCI bridge 的组合。

但是，为什么要这么设计？将 pcie switch 伪装为 PCI-to-PCI bridge

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
