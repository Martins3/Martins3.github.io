# Development

- [构建与测试环境](testing.md)
- [xfstests runner 结构](xfstests.md)
- [调试方法与真实案例](debugging.md)

修改代码后的基本顺序是：主机构建、guest 定向回归、检查日志和 dmesg、邻接回归，
最后再做完整 generic 验收。

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
