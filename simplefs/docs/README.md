# SimpleFS 文档

这里仅保存“以当前代码为准”的文档。按用途分为三组：

- [tutorial/](tutorial/README.md)：连续阅读的文件系统教程；
- [reference/](reference/README.md)：磁盘格式、架构不变量和源码索引；
- [development/](development/README.md)：构建、测试、调试和维护流程。

按日期产生的测试结论、已结束的设计调查、早期问答草稿均在
[record/](../record/README.md)。正文和历史记录分开后，判断当前行为时应按以下
优先级取证：

1. 当前源码和磁盘格式定义；
2. 当前教程与 reference；
3. 最新的完整测试记录；
4. 历史草稿和旧阶段记录。

旧记录可以解释“为什么这样写”，但不能反过来覆盖当前代码。
