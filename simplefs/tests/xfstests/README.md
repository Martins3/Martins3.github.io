# xfstests 适配层

这里保存 runner 之外、但运行 SimpleFS generic 测试必需的支持文件：

- `prepare.sh`：生成 private xfstests 配置、helper 和 wrapper；
- `patch.py`：幂等添加格式 capability 与少量合法 NOTRUN guard；
- `legacy_mount.c`：让 timestamp 测试走 legacy mount(2) 的小型 wrapper 后端。

标准入口始终是仓库根的 `xfstests-full.sh`。设计、用例选择和维护规则见
[docs/development/xfstests.md](../../docs/development/xfstests.md)。不要直接修改共享的
`/home/martins3/data/xfstests`；适配只作用于 `/var/tmp` private copy。

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
