# XFS 与 ext4 的 journal 设计对比

核心区别是：**ext4 的传统 JBD2 journal 以 metadata block 为中心；XFS 的 log 以 metadata object 及其修改为中心，并围绕它设计了 transaction aggregation、rolling transaction 和 recovery 机制。**

两者都采用 write-ahead logging（WAL）：相关 log record 必须先持久化，修改后的 metadata 才能写回 home location，即 metadata 在文件系统中的最终存储位置。

本文主要比较 ext4 的传统 full commit 路径与 XFS delayed logging；ext4 fast commit 单独说明。

## 设计概览

| 维度             | ext4 / JBD2                                                      | XFS                                                                       |
| ---------------- | ---------------------------------------------------------------- | ------------------------------------------------------------------------- |
| 日志层           | 通用 JBD2，围绕 `buffer_head` / `journal_head` 管理 journal 状态 | 专用 log，通过不同类型的 log item 对接 metadata                           |
| 记录内容         | 通常记录修改后的完整 metadata block                              | inode 等使用对象专用格式，buffer 记录 dirty region，另有 intent/done item |
| Transaction 组织 | 多个操作的 `handle` 加入同一个 running transaction               | 操作使用自己的 `xfs_trans`，commit 后在 CIL 中聚合                        |
| 批量提交         | commit 一个聚合了多个操作的 JBD2 transaction                     | push 一个聚合了多个 transaction 修改的 CIL checkpoint                     |
| Log space 回收   | 主要通过 transaction 的 checkpoint 状态推进 journal tail         | 通过 AIL 跟踪仍需 log 保护的 item，推进 log tail                          |
| File data        | 支持 `data=ordered`、`data=writeback`、`data=journal`            | 只记录 metadata，没有对应的 `data=journal` 模式                           |

这里的 buffer 指承载 metadata block 内容的内存 buffer。JBD2 使用 `buffer_head`，并通过关联的 `journal_head` 管理其 journal 状态；XFS 则使用自己的 `xfs_buf` 等结构。

## 1. Log record 的粒度不同

假设只修改一个 inode 的部分字段：

- **ext4 full commit**：通常将包含该 inode 的整个 inode-table block 写入 journal，即使 block 内的其他 inode 没有变化。
- **XFS**：通过 inode log item 记录需要的 inode core、fork 等内容，不必记录整个 inode 所在的 disk block。对于 directory block、B-tree block 等 buffer，则记录被标记为 dirty 的 region。

因此，XFS 混合使用 object-specific logging、physical region logging 和 intent logging，不能简单归类为“只记录操作”的 logical journal。它记录的也不一定是精确到单个字段的最小 delta。

这种设计能减少某些 metadata 修改的 log volume，但实际收益取决于修改范围、aggregation 效果以及 workload，不能直接推导出 XFS 在所有场景下都更快。

## 2. 两者都会合并修改，但 aggregation 的位置不同

### ext4 / JBD2

```text
多个操作的 handle
    → 同一个 running transaction
    → 关闭该 transaction 的新 handle 入口，等待已有 handle 结束
    → 写 journal block 和 commit record
    → checkpoint：metadata 写回 home location
    → 回收 journal space
```

`handle` 表示一个操作在 JBD2 中的更新上下文，不等于一个独立的 on-disk transaction。多个操作可以共享一个 transaction。

JBD2 可以让新的 running transaction 与旧的 committing transaction 重叠执行；它并不要求每次 commit 都阻塞整个文件系统中的所有新操作。共享 metadata buffer 等依赖仍可能引入等待。

### XFS

```text
多个独立的 xfs_trans
    → xfs_trans_commit()：将修改合并进 CIL
    → CIL push：写 log record 和 commit record
    → AIL 跟踪仍需 log 保护的 item
    → metadata writeback 等工作使 log tail 向前移动
```

CIL 是 **Committed Item List**。这里的 committed 首先表示 transaction 已在内存中完成 commit，并不表示 log 已经持久化。

同一个 object 在一个 CIL checkpoint 内被反复修改，可以合并为包含累计修改的最新 log item 内容，避免每次 `xfs_trans_commit()` 都生成一份独立的 on-disk log record。

但不能据此认为“ext4 修改十次就写十次 journal”：**JBD2 在同一个 transaction 内也会合并同一 block 的多次修改。**

XFS 的多个 `xfs_trans` 也不意味着每个 transaction 都拥有独立的持久化路径；它们最终仍通过共享的 log，并遵守 commit ordering。

### 两套代码中的 checkpoint 含义不同

- **XFS CIL checkpoint**：一批聚合的 metadata 修改提交到 log。
- **JBD2 checkpoint**：已 commit 的 metadata 写回 home location，使相关 journal space 可以回收。

读代码时不能将这两个 checkpoint 当成同一个阶段。

## 3. XFS 用 rolling transaction 与 intent 支持复杂操作

例如释放大量 extent，无法假定一个 transaction 就能完成。XFS 使用以下机制：

- **Rolling transaction**：完成一部分工作并 commit，再继续下一部分，避免要求单个 transaction 具有无界的 log reservation。
- **Intent/done item**：记录待完成的工作和完成状态。Recovery 可以继续处理已经持久化、但尚未完成的 intent。
- **Relogging**：再次记录 object 时，携带仍需 log 保护的累计修改，将它对 log 的依赖移向较新的位置，避免长期占住旧的 log space。

**一串 rolling transactions 本身不构成一个 crash-atomic transaction。** 每个已经持久化的 transaction 可以单独恢复；需要跨 transaction 保证完成的工作依赖 intent/deferred-op 协议。

ext4 也会拆分长操作，并使用 orphan tracking 等机制处理特定 recovery 问题。但传统 JBD2 主要提供 block-level transaction，不理解“继续完成某个 extent 操作”这样的 filesystem-specific 工作。

## 4. Log space 回收的组织方式不同

JBD2 通过 checkpoint list 跟踪已 commit、但尚未完成 checkpoint 的 buffer，并根据 transaction 的完成状态回收 journal space。

XFS 使用 **Active Item List（AIL）** 跟踪仍需 log 保护的 item，按 LSN 组织。Metadata writeback 可以解除相应的 log 依赖；relogging 也可以将 item 的依赖移到更新的 LSN。

因此，XFS log tail 的推进不仅涉及 metadata writeback，还与 relogging、intent 的生命周期有关。AIL 中的 item 也不全是可以直接 writeback 的 metadata buffer。

两者都需要在修改开始前进行空间控制：JBD2 使用 journal credits，XFS 使用 log reservation。XFS 的 permanent reservation、rolling transaction 和 relogging 共同支持长操作的 forward progress。

## 5. File data 的一致性模式不同

ext4 提供三种主要模式：

- **`data=ordered`**：默认模式，file data 不进入 journal；与 transaction 关联的 data write 需要满足先于 metadata commit 的 ordering 要求。
- **`data=writeback`**：file data 不进入 journal，也不提供上述 data-before-metadata-commit ordering。
- **`data=journal`**：file data 和 metadata 都进入 journal。

XFS 不提供 file data journaling。它依赖 data I/O、unwritten extent conversion 以及相关 metadata update 的 ordering 来保证一致性，不能仅凭“都是 metadata journaling”就认为它与 ext4 `data=writeback` 的具体行为完全相同。

对应用而言，普通 write 返回不等于数据已经持久化，仍需使用 `fsync()` 等接口满足 durability 要求。对内核代码而言，`xfs_trans_commit()` 和 `jbd2_journal_stop()` 也不能直接理解成“journal 已经落盘”。

## 6. ext4 fast commit 是传统 block logging 之外的路径

启用 ext4 fast commit 后，“ext4 总是记录整个 metadata block”就不成立了。

Fast commit 在支持的 `data=ordered` 场景下，记录重建相关 metadata 所需的精简信息，而不是每次都写完整 metadata block。它仍与 JBD2 full commit 协作：遇到不支持的操作、fast commit space 不足或 full commit 定时触发等情况，会执行 full commit。

所以准确的比较是：

- **ext4 traditional full commit**：以 metadata block image 为中心。
- **ext4 fast commit**：为支持的操作提供精简的 recovery record。
- **XFS**：整个 logging 架构围绕 log item、CIL、AIL，以及 rolling transaction / intent 机制组织。

## 参考资料与源码入口

- [XFS Logging Design](https://docs.kernel.org/filesystems/xfs/xfs-delayed-logging-design.html)
- [ext4 Journal (JBD2)](https://www.kernel.org/doc/html/latest/filesystems/ext4/journal.html)
- [The Linux Journalling API](https://www.kernel.org/doc/html/latest/filesystems/journalling.html)
- XFS CIL：[`fs/xfs/xfs_log_cil.c`](fs/xfs/xfs_log_cil.c)
- XFS AIL：[`fs/xfs/xfs_trans_ail.c`](fs/xfs/xfs_trans_ail.c)
- XFS deferred operations：[`fs/xfs/libxfs/xfs_defer.c`](fs/xfs/libxfs/xfs_defer.c)
- JBD2 transaction：[`fs/jbd2/transaction.c`](fs/jbd2/transaction.c)
- JBD2 commit：[`fs/jbd2/commit.c`](fs/jbd2/commit.c)
- JBD2 checkpoint：[`fs/jbd2/checkpoint.c`](fs/jbd2/checkpoint.c)
- ext4 fast commit：[`fs/ext4/fast_commit.c`](fs/ext4/fast_commit.c)

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
