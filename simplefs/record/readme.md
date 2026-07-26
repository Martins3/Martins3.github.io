# simplefs xfstests 记录索引

- [2026-07-16-final-xfstests.md](2026-07-16-final-xfstests.md)：基线结论（307 PASS / 480 NOTRUN）。
- [2026-07-18-phase1-xfstests.md](2026-07-18-phase1-xfstests.md)：Phase 0+1 全量结论。**787 个 generic case，PASS 345 / NOTRUN 442 / FAIL 0 / TIMEOUT 0**，净解锁 38 个且未引入 FAIL。
- [2026-07-19-phase2-journal-progress.md](2026-07-19-phase2-journal-progress.md)：Phase 2 旧自建 journal 和 JBD2 迁移初期的历史调查记录。
- [2026-07-20-phase2-jbd2-progress.md](2026-07-20-phase2-jbd2-progress.md)：当前最新全量结论和 Phase 2 完整调查记录。2026-07-26 在统一的 `7.1.2-00001-gfb512e2a3eed #26` 内核、模块和 mkfs 环境中连续覆盖 001--787，最终为 **PASS 432 / NOTRUN 355 / FAIL 0 / TIMEOUT 0**；787 份日志齐全，所有 NOTRUN 都有明确原因。SimpleFS 核心保持 folio/iomap，只有 JBD2 适配层可依赖 buffer_head。
- [XFS_LOCKDEP_REPRO.md](XFS_LOCKDEP_REPRO.md)：测试期间在 VM 的 rootfs XFS 上复现的 reclaim lockdep 问题，与 simplefs 无关，含独立复现方法。
- [unimplemented-plan.md](unimplemented-plan.md)：NOTRUN 的逐原因分布，以及后续可实现功能的难度/收益分析。
- [roadmap.md](roadmap.md)：推进计划（Phase 0 环境补齐 → Phase 1 易实现功能 → Phase 2 journal 补全）。

2026-03 及更早的中间状态记录、原始日志和已修复 bug 的调查文档已删除，需要时从 git 历史找回。

判定规则：

- `PASS` 只表示用例实际执行且输出匹配；
- `NOTRUN` 必须保留具体原因，不计为 PASS；
- `TIMEOUT`、外部中断、VM reset 不计为 PASS；
- 内核 warning、Oops 或 panic 即使用例输出匹配，也必须单独调查和重跑。
- 目标未完成而本轮阶段性结束时，也必须更新当日进展记录和 roadmap，不能只在对话中报告。

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
