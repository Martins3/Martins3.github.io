# 8. 一致性：事务、JBD2 与恢复

一次 create 会修改多块元数据，设备却只能逐次完成写入。crash 可以发生在任意两次
写之间。journal 的目标不是让磁盘永不出错，而是让一组元数据更新以可恢复的边界
发布：恢复后看到旧状态或新状态，不能看到随意拼接的半状态。

## 数据一致性与元数据一致性

目录项指向 inode、inode 指向 extent、extent 指向数据、bitmap 记录所有权。元数据
自洽不自动保证用户数据已经稳定。SimpleFS 默认采用类似 ordered 的原则：发布
written extent 或完成 fsync transaction 前，相关文件数据必须先落盘。

unwritten extent 也是一致性工具：已经分配但尚未成功写入的数据块，在任何时刻读都
返回零。只有 I/O 成功后才把状态转换为 written。

## SimpleFS 与 JBD2 的边界

JBD2 上游客户端 API 以 `buffer_head` 为中心，但 SimpleFS 核心元数据访问使用 bdev
folio。`simplefs_journal.c` 是唯一适配层：核心传入 block number、folio snapshot 和
transaction handle，适配层再满足 JBD2 API。

```text
inode/dir/extent/xattr/bitmap code (folio-native)
                  |
        simplefs_journal_* API
                  |
simplefs_journal.c: folio snapshot <-> buffer_head/JBD2
                  |
                 JBD2
```

把 buffer_head 扩散到其他文件会产生两套元数据缓存模型和 dirty 规则，使块所有权与
缓存身份更难证明。这里的限制是架构边界，不只是代码风格。

## Transaction 生命周期

典型修改路径：

1. `simplefs_journal_start()` 申请 handle 和 credits；
2. 修改前让 JBD2 获得对应 metadata block 的 write access；
3. 修改 folio 中的格式字段；
4. `simplefs_journal_dirty_metadata()` 把新 snapshot 交给 transaction；
5. `simplefs_journal_stop()` 结束当前操作对 handle 的使用；
6. commit thread 按 descriptor/data/barrier/commit 顺序发布 transaction；
7. checkpoint 最终把 journal 中的 metadata 写回 home location。

credits 是一个 transaction 最多修改多少 metadata buffer 的预算。估少会在持锁路径
中扩展失败，估得无限大又掩盖真实更新边界。教程阅读时先数“可能脏几个不同 block”，
再看代码给的 credits。

## fsync 和 commit record

普通 write 返回只表示数据进入内核；fsync 才要求相应数据和元数据到达稳定边界。
SimpleFS 的顺序是：写回文件数据，等待 unwritten conversion，把 inode/extent
snapshot 加入 transaction，force commit，等待 commit record 和必要 flush。

commit record 之前掉电，recovery 忽略不完整 transaction；之后掉电，recovery 可以
重放完整 metadata。barrier/flush 是为了让设备不能把 commit 持久化顺序反转到数据
之前。

## Mount recovery

可写挂载期间 `s_needs_recovery=1`，干净卸载或成功恢复后清零。mount 加载 JBD2 后
可能重放 inode table、目录和 bitmap，因此先前读入内存的 bitmap 必须重载。根 inode
也必须在 replay 后实例化。

`norecovery` 只允许只读，供检查损坏现场；`nojournal` 是显式关闭一致性保护的测试/
教学选项，不应被误认为默认生产语义。

## Revoke、复用与 discard

块在 transaction 中被释放后，旧日志记录仍可能在 replay 时写回这个 block。JBD2
revoke 告诉 recovery 不再应用旧版本，但 allocator 还要避免在 freeing transaction
真正 commit 前过早复用。否则新对象刚写入的数据会被旧 transaction 覆盖。

在线 discard 同样要延迟到释放达到 durability boundary 后下发；若块在此之前重新
分配，必须取消 pending discard。discard 是设备提示，不是所有权变更本身。

## 错误策略

I/O error 不能被 `|| true` 式地吞掉。journal abort、writeback error 或关键 metadata
读错后，SimpleFS 进入 shutdown，使后续操作返回 EIO，避免继续在未知状态上修改。
故障注入用例期望看到受控错误，而不是 silent corruption、warning 或 hang。

## 对照源码

- `Documentation/filesystems/journalling.rst`；
- `fs/jbd2/` 与 `fs/ext4/ext4_jbd2.c`：JBD2 客户端模型；
- `fs/xfs/xfs_log*`：不同实现、相同事务/恢复问题；
- [JBD2 迁移历史](../../record/xfstests/2026-07-20-phase2-jbd2-progress.md)：
  SimpleFS 从不可靠自建 journal 迁移时暴露的具体问题。

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
