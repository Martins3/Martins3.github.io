# Investigations

- [format 2 修复与验证](2026-10-14-format2-fixes.md)：同步事务、可扩展目录、
  名称索引和空间优化；完整拷贝中位数为 ext4 的 1.295 倍，两个有效回归区段覆盖
  001--787：440 PASS / 347 NOTRUN / 0 FAIL / 0 TIMEOUT；
- [NFS 实际工作树拷贝与容量](2026-10-14-nfs-worktree-comparison.md)：nix-replace-kernel
  双盘对照、目录扫描 CPU 热点、单目录上限及大卷 sync credits 失败；
- [与 ext4 的基本对比](2026-10-14-ext4-comparison.md)：三组负载、元数据优化、
  基本正确性验证及设备移除错误处理；
- [Linux 源码树拷贝性能](2026-09-24-linux-copy-performance.md)：virtme 内存后端
  基准、预分配热点与按需首次分配优化；
- [generic/286 unwritten extent 设计调查](generic-286-unwritten-design.md)：历史问题与
  最小修复方向；当前实现需重新对照源码确认；
- [XFS lockdep 独立复现](xfs-lockdep-repro.md)：测试宿主 XFS 的问题，与 SimpleFS
  无关。

调查文档记录某一时点的证据，不自动成为当前架构说明。
