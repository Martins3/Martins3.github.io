# SimpleFS 历史记录

`record/` 保存带时间背景的证据，不承担当前教程职责：

- [xfstests/](xfstests/README.md)：基线、阶段推进、最终全量结果与 NOTRUN 分类；
- [investigations/](investigations/README.md)：独立问题调查和设计记录；
- [archive/](archive/README.md)：早期问答、待办、环境片段和已取代脚本。

最新一次统一全量验收是 2026-07-26：generic/001--787 连续完整运行，PASS 432 /
NOTRUN 355 / FAIL 0 / TIMEOUT 0；787 份日志齐全，NOTRUN 均有原因，dmesg 无非预期
warning/Oops/panic。完整环境和哈希见
[JBD2/Phase 2 记录](xfstests/2026-07-20-phase2-jbd2-progress.md)。

判定规则：

- PASS 只表示用例实际执行、输出匹配且内核健康；
- NOTRUN 必须保留真实的 capability、格式或环境原因；
- TIMEOUT、外部中断、VM reset 不计为 PASS；
- 内核 warning/Oops/panic 即使用例输出匹配也必须调查和重跑；
- 历史 PASS 只覆盖当时的源码、内核、模块、mkfs 和 runner，当前变化要重新回归。

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
