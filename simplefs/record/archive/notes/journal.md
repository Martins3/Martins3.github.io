## 如何理解?
Metadata-only Journaling
**决策**: 只日志元数据，不日志数据块

## ext4 的 journal 机制依赖 buffer head 的

ext4_jbd2.c 中的 buffer_head 依赖
// 所有核心 journal 操作都基于 buffer_head
```txt
int __ext4_journal_get_write_access(..., struct buffer_head *bh, ...)
{
    err = jbd2_journal_get_write_access(handle, bh);  // 直接传递 bh
    ...
}

int __ext4_forget(..., struct buffer_head *bh, ...)
{
    BUFFER_TRACE(bh, "enter");
    err = jbd2_journal_forget(handle, bh);  // 直接传递 bh
    err = jbd2_journal_revoke(handle, blocknr, bh);
    ...
}

int __ext4_handle_dirty_metadata(..., struct buffer_head *bh)
{
    err = jbd2_journal_dirty_metadata(handle, bh);  // 直接传递 bh
    ...
}
```

为什么依赖 buffer_head？

```txt
 原因         说明
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 历史继承     jbd/jbd2 设计于 2.4/2.6 内核时代，buffer_head 是块设备 I/O 的标准抽象
 事务追踪     jbd2 需要精确追踪每个块的修改状态，buffer_head 提供了 b_private 字段挂载 journal 信息
 回写控制     mark_buffer_dirty() / set_buffer_uptodate() 等操作是 jbd2 的核心机制
 原子性保证   buffer_head 的锁机制（bh->b_lock）保证事务的原子性
```

jbd2 层的 buffer_head 依赖

jbd2（Journal Block Device 2）层同样基于 buffer_head：

```txt
// include/linux/jbd2.h
struct jbd2_buffer_trigger_type {
    void (*t_frozen)(struct buffer_head *bh, ...);
    void (*t_abort)(struct buffer_head *bh, ...);
};

// fs/jbd2/transaction.c
int jbd2_journal_get_write_access(handle_t *handle, struct buffer_head *bh)
int jbd2_journal_dirty_metadata(handle_t *handle, struct buffer_head *bh)
int jbd2_journal_forget(handle_t *handle, struct buffer_head *bh)
```

对比：现代文件系统

```txt
 文件系统   Journal 机制   是否依赖 buffer_head
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 ext4       jbd2           ✅ 依赖
 xfs        XFS log        ❌ 使用自己的 buffer 管理
 btrfs      COW + 日志树   ❌ 使用 extent buffer
 f2fs       Checkpoint     ⚠️ 部分使用
```

结论
ext4 的 journal 机制完全建立在 buffer_head 之上，这是其设计的核心基础。这也是为什么 ext4 难以直接迁移到纯 folio/page-based 的 I/O
模型，而 simplefs 这样的新文件系统在实现 journal 时需要仔细考虑如何与内核的页缓存机制集成。

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
