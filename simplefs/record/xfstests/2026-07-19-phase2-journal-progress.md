# 2026-07-19 Phase 2 journal 定向推进记录

> 本文件保留迁移初期的历史证据。当前状态见
> `2026-07-20-phase2-jbd2-progress.md`；其中已纠正本文提出的“把核心元数据
> 改成原生 buffer_head”方向，buffer_head 现在只允许存在于 JBD2 适配层。

## 结论

Phase 2 尚未完成，不能据此更新全量 787 case 的统计。当前定向测试已经
解锁大部分 metadata-journal case，但 `generic/475` 仍为 FAIL，`generic/050`
仍未通过，`generic/052/054/055` 仍因缺少 simplefs log-state probing 而
NOTRUN；dm-flakey 全批次和 dm-logwrites 批次尚未验收。

上述 PASS 来自迁移前的自建 folio/data-copy journal，不能代表当前 JBD2 WIP。
本轮已开始把后端替换为内核 JBD2，但最小元数据压力尚会触发 JBD2 断言，
因此当前代码仍是不可验收的开发状态，不能把定向 PASS 当成 journal 已完成。

## JBD2 迁移 WIP

已完成的结构性改动：

- mkfs 改写标准大端 JBD2 v2 superblock，并要求日志区至少 1024 blocks；
- mount 改用 `jbd2_journal_init_dev()`、`jbd2_journal_load()`，模块依赖已显示
  `depends: jbd2`；
- transaction wrapper 改用 `jbd2_journal_start/stop()`，阶段性采用串行事务和
  `jbd2_journal_flush()`，避免旧调用顺序跨 checkpoint 复用元数据；
- inode/extent/xattr 的部分 metadata dirty 路径已接到 JBD2 buffer_head adapter；
- `norecovery` 保留只读、不重放语义。

2026-07-19 晚间的最小回归结果：

- `generic/001` 首轮在 `jbd2_journal_commit_transaction()` 的
  `buffer_jbddirty` 断言处 panic；
- 把 commit 改成完整 checkpoint 后，曾完整跑完但 FAIL，随后日志进入 `-EIO`；
- 继续复现确认问题集中在 inode table home block（首先是 block 1）；
- 调试栈精确定位为
  `mkdir -> write_inode_now -> simplefs_write_inode -> simplefs_get_folio ->
  simplefs_journal_prepare_current`，随后 JBD2 报告 dirty metadata buffer 并在
  commit 断言；
- 20 个空文件的轻量烟雾测试曾完成，但扩大到 100 个文件即可稳定复现 panic，
  所以不能把轻量成功视为修复。

根因边界已经明确：当前 adapter 让调用者通过 folio 修改元数据，再临时为同一
bdev page 建立 buffer_head。即使提前 prepare、串行事务和清理 folio dirty 状态，
普通写回与 JBD2 的 buffer 生命周期仍可能并发，无法可靠维持
`BH_Dirty`/`BH_JBDDirty` 所有权。下一步不再扩大“清 dirty bit”的补丁，而是先把
inode table 写路径改成原生 buffer_head 序列：

1. `sb_getblk()`/`sb_bread()`；
2. `jbd2_journal_get_write_access()`；
3. 在 buffer lock/正确生命周期内修改 inode table；
4. `jbd2_journal_dirty_metadata()`；
5. 证明 generic/001 无 warning/Oops 后，再按同样模式迁移目录、extent、bitmap、
   xattr 和 superblock。

## 本轮修复

### 并发 handle UAF

当前 HEAD 重编后，首次运行 `generic/051` 在并发 fsstress 中触发 kernel panic：

```text
BUG: unable to handle page fault
RIP: __simplefs_journal_dirty_metadata [simplefs]
simplefs_journal_dirty_current
simplefs_file_sync_extents
```

根因是 `j_current_handle` 指向某个并发操作的 per-operation handle。该 handle
已经在 `simplefs_journal_stop()` 中释放，但共享 transaction 仍由其他操作持有，
后续 extent dirty 路径解引用悬空指针。

修复方式：删除 `j_current_handle`，在 `j_mutex` 保护下直接把无显式 handle 的
metadata block 加入仍存活的 `j_running` transaction；同时删除会在其他 handle
仍存活时强行 commit/free transaction 的危险轮转路径。修复后 `generic/051` PASS，
没有再次出现 Oops/panic。

### norecovery 语义

新增 `norecovery` mount option：

- 只允许只读挂载；
- 跳过 journal replay；
- 卸载时不 checkpoint、不写 home metadata；
- norecovery 模式拒绝 FITRIM，避免依据未重放的空闲位图 discard 数据；
- 日志需要恢复而底层 block device 只读时，普通挂载返回 `-EROFS`。

同时补齐 legacy mount helper 对只读块设备的只读重试，并把生成的 helper binary
改成 `legacy_mount.out`。`generic/537` 从 FAIL 转为 PASS；`generic/050` 仍需继续
修正并重新核对完整输出。

## 定向测试结果

测试环境：`yyds-fs`，kernel
`7.1.2-00001-gd41bd6abfe34`，当前 worktree 重新构建的 `simplefs.ko`。

| case | 当前结果 | 说明 |
| --- | --- | --- |
| 043-047, 049 | PASS | journal 崩溃恢复回归 |
| 048 | PASS | 默认 2 GiB scratch 会因要求 10 GiB NOTRUN；用 12 GiB sparse scratch 实跑 PASS |
| 051 | PASS | 首跑 panic；修复并发 handle UAF 后重跑 PASS |
| 388 | PASS | metadata journal 定向测试 |
| 392, 417, 468 | PASS | metadata journal 定向测试 |
| 475 | FAIL | 50 秒反复 dm-error/recovery 后出现 `p0: Not a directory`、mount 失败；仍有日志/目录元数据一致性问题 |
| 505, 530 | PASS | metadata journal 定向测试 |
| 537 | PASS | 实现 norecovery + 禁止 norecovery FITRIM 后 PASS |
| 753 | PASS | metadata journal 定向测试 |
| 050 | FAIL | read-only blockdev / dirty-log / norecovery mount 序列尚未完全匹配预期 |
| 052, 054, 055 | NOTRUN | `simplefs does not support log state probing` |

本轮编译和静态检查：

- `./build.sh` PASS；
- `git diff --check` PASS；
- `shellcheck -x xfstests-full.sh` 零警告；
- `shfmt -w -ci -s -bn xfstests-full.sh` 后无额外 diff。

## 已确认的根本缺口

`generic/475` 的失败不是应该通过 harness 跳过的问题。当前自建日志的 log block
写路径主要依赖 `folio_mark_dirty()`，随后执行 device flush；flush 本身不会保证尚在
page cache 中的 descriptor/data/commit folio 已按 WAL 顺序提交。这无法提供 JBD2
要求的 write-ahead ordering，在反复 dm-error 下会暴露目录/根 inode 状态不一致。

JBD2 后端迁移已经启动，下一步应完成原生 buffer_head 元数据生命周期，而不是
继续扩大自建 transaction/log/recovery 代码或在 adapter 中清理 dirty bit：

1. mkfs 写入标准 JBD2 v2 journal superblock；
2. mount 使用 `jbd2_journal_init_dev()` + `jbd2_journal_load()`；
3. transaction wrapper 改用 `jbd2_journal_start/stop()`；
4. metadata 缓存适配 JBD2 的 `buffer_head` 接口，并保证 home metadata 不绕过
   `jbd2_journal_get_write_access()` / `jbd2_journal_dirty_metadata()` 提前写回；
5. 用 475、050、052/054/055、dm-flakey 和 dm-logwrites 批次逐级验收；
6. 最后全量复跑 787 case，要求 FAIL 0 / TIMEOUT 0。

## 记录规则

只要目标尚未完成但本轮需要阶段性结束，就更新本文件对应日期的进展记录和
`roadmap.md`，明确当前证据、剩余 FAIL/NOTRUN、未验证批次和下一步；不得只在
对话中报告进度。

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
