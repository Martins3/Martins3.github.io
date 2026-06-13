# mlock

- [ ] 那么 mlock 可以自动 让原来没有建立映射映射的虚拟地址建立映射。mlock 保证其对应的页面没有换出，如果本身就是不存在，不换出的意义在于什么地方啊!

// 首先了解 mlock 的内容
// @todo read this https://lwn.net/Articles/286485/ 完全解释了 vm_flags 是 VM_LOCKED 以及 unevictable 的含义


mlock 施加影响的位置:
1. page reclaim 和 swap 模块
2. mlock 可以施加于 hugemem 吗 ?

- [ ] mlock is more complex than expected, because it has to handle isolation ?

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
