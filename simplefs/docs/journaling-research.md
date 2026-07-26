# SimpleFS Journal 实现报告

**状态**: 已实现 (Phase 6 完成)  
**架构**: 纯 iomap（无 buffer_head）  
**版本**: 2.0 (2025-03-04)

---

## 1. 高层设计

### 1.1 设计哲学与定位

**为什么需要 Journal？**

文件系统面临的核心问题：
```
用户操作: create("/file.txt")
内部步骤:
  1. 分配 inode
  2. 分配数据块
  3. 写入目录项
  4. 更新位图

崩溃风险: 如果只完成步骤 1-2，未更新目录项
          -> inode 和数据块泄漏
          -> 文件系统不一致
```

Journal（Write-Ahead Logging）通过"先写日志，后写数据"保证原子性。

**SimpleFS Journal 特点**：
- 纯物理日志（Physical Logging）：记录修改的完整块内容
- 事务粒度：每个系统调用一个事务
- 支持的操作：create, mkdir, unlink, rmdir, rename

### 1.2 Journal 在系统中的位置

```
┌─────────────────────────────────────────────────────────────┐
│                     用户系统调用层                           │
│              (create, mkdir, unlink, rename)                │
├─────────────────────────────────────────────────────────────┤
│                  Journal 事务管理层                          │
│    ┌─────────────────┐  ┌─────────────────────────────┐    │
│    │ simplefs_handle │  │ simplefs_journal_start/stop │    │
│    │   (事务句柄)     │  │   (事务开始/提交)            │    │
│    └─────────────────┘  └─────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                  物理日志层                                 │
│    ┌──────────────────────────────────────────────────┐    │
│    │  Transaction Commit (descriptor → data → commit) │    │
│    │  Checksums, Barriers, Disk Ordering             │    │
│    └──────────────────────────────────────────────────┘    │
├─────────────────────────────────────────────────────────────┤
│                  恢复层                                     │
│    ┌──────────────────────────────────────────────────┐    │
│    │  Journal Scan → Replay → Checkpoint             │    │
│    │  自动恢复，无需 fsck                             │    │
│    └──────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### 1.3 与 Linux 主流 Journal 对比

| 特性 | SimpleFS Journal | ext4 Journal | XFS Log |
|------|------------------|--------------|---------|
| 日志类型 | 物理日志 | 物理/追加混合 | 逻辑日志 |
| 并发事务 | 单事务 | 多事务并发 | 多事务并发 |
| 事务隔离 | 完全串行 | 异步提交 | 组提交 |
| 校验和 | CRC32 | CRC32C | CRC32C |
| 数据日志 | 仅元数据 | 可选数据日志 | 仅元数据 |
| 检查点 | 挂载时 | 后台检查点 | 后台检查点 |

**SimpleFS 设计选择**：
- 单事务模型：简化实现，避免复杂的并发控制
- 纯物理日志：实现简单，恢复可靠
- 挂载时检查点：不依赖内核线程

---

## 2. 核心数据结构

### 2.1 Journal 控制结构

```c
struct simplefs_journal {
    struct super_block  *j_sb;           /* 关联的超级块 */
    struct mutex         j_state_lock;   /* 状态保护锁 */
    
    /* 事务列表 */
    struct list_head     j_transactions; /* 活跃事务列表 */
    
    /* Journal 位置信息 */
    sector_t             j_start;        /* Journal 起始扇区（块号） */
    uint32_t             j_blocks;       /* Journal 总块数 */
    uint32_t             j_maxlen;       /* 最大可用块数 */
    
    /* 运行状态 */
    atomic_t             j_running_transactions;  /* 当前活跃事务数 */
    
    /* 恢复状态 */
    uint64_t             j_tail_sequence; /* 最早未提交事务序号 */
};
```

### 2.2 事务结构

```c
struct simplefs_transaction {
    struct list_head     t_list;         /* 链表节点 */
    
    /* 事务标识 */
    tid_t                t_tid;          /* 事务 ID */
    struct simplefs_handle *t_handle;    /* 关联的 handle */
    
    /* 元数据块管理 */
    struct list_head     t_buffers;      /* 待提交的元数据块 */
    unsigned int         t_nr_buffers;   /* 缓冲区数量 */
    
    /* 状态 */
    enum {
        T_RUNNING,                       /* 运行中 */
        T_COMMITTING,                    /* 提交中 */
        T_FINISHED,                      /* 已完成 */
    } t_state;
    
    /* 资源追踪 */
    unsigned int         t_credits;      /* 预留的块数 */
    unsigned int         t_outstanding_credits; /* 剩余可用 */
};
```

### 2.3 事务句柄

```c
struct simplefs_handle {
    tid_t                h_transaction;  /* 事务 ID */
    unsigned int         h_credits;      /* 总信用额度 */
    unsigned int         h_credits_left; /* 剩余信用 */
    const char          *h_caller;       /* 调用者（调试用） */
    struct simplefs_transaction *h_transaction_ptr; /* 关联事务 */
    int                  h_err;          /* 错误码 */
};
```

### 2.4 Journal Buffer（纯 iomap 版本）

```c
/* 关键设计：使用数据指针而非 buffer_head */
struct simplefs_journal_buffer {
    struct list_head     b_list;         /* 链表节点 */
    sector_t             b_blocknr;      /* 磁盘块号 */
    void                *b_data;         /* 保存的数据 (kmalloc) */
};
```

**设计要点**：
- 不依赖 buffer_head（避免与 iomap 冲突）
- 数据通过 kmalloc 保存，确保事务提交时的一致性

---

## 3. 工作原理

### 3.1 事务生命周期

```
┌──────────────┐    ┌──────────────────┐    ┌──────────────────┐
│  User syscall │───▶│ journal_start() │───▶│    Operation     │
└──────────────┘    └──────────────────┘    └──────────────────┘
       │                                         │
       │    ┌────────────────────────────────────┘
       │    │
       │    ▼
       │  ┌─────────────────┐    ┌──────────────────┐
       └──│ journal_stop()  │───▶│ transaction_commit│
          └─────────────────┘    └──────────────────┘
                                              │
                          ┌───────────────────┴───────────────┐
                          ▼                                   ▼
                  ┌────────────────┐              ┌──────────────────────┐
                  │  Write Journal │              │    Checkpoint        │
                  │  Descriptor    │              │  (Update superblock) │
                  │  Data Blocks   │              └──────────────────────┘
                  │  Commit Block  │
                  └────────────────┘
```

### 3.2 事务提交流程

```c
int simplefs_transaction_commit(struct simplefs_transaction *transaction)
{
    // 1. 分配 journal 块
    blocknr = journal->j_start + 1;  // descriptor
    
    // 2. 写描述符块（包含事务元数据）
    write_journal_block(descriptor_folio, blocknr);
    
    // 3. 写数据块（每个元数据块的副本）
    for (each buffer in transaction) {
        write_journal_block(buffer->data, blocknr++);
    }
    
    // 4. 写提交块（屏障，确保顺序）
    write_journal_block(commit_folio, blocknr);
    
    // 5. 等待设备写入
    blkdev_issue_flush(sb->s_bdev);
}
```

### 3.3 数据写入 Disk 流程

```
Journal Area:                    Filesystem Area:
┌────────────────┐              ┌────────────────┐
│   Descriptor   │              │  Superblock    │
│   (metadata)   │              └────────────────┘
├────────────────┤              ┌────────────────┐
│  Block 0 Data  │─────────────▶│   Block 0      │
│  (FS block 42) │              │   (metadata)   │
├────────────────┤              └────────────────┘
│  Block 1 Data  │─────────────▶┌────────────────┐
│  (FS block 43) │              │   Block 1      │
├────────────────┤              └────────────────┘
│     ...        │              ┌────────────────┐
├────────────────┤              │      ...       │
│   Commit       │              └────────────────┘
│   (barrier)    │
└────────────────┘
```

**关键保证**：
1. 先写 journal，后写实际数据
2. Commit 块作为屏障，确保 journal 完整
3. 如果崩溃发生在 commit 前，丢弃 journal
4. 如果崩溃发生在 commit 后，从 journal 恢复

### 3.4 恢复流程

```
┌─────────────────────────────────────────────────────────────┐
│                    Mount Phase                               │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  1. Read Journal Superblock                                 │
│     └── Check magic, version, feature compatibility         │
│                                                             │
│  2. Scan Journal Blocks                                     │
│     └── Find descriptors with valid checksums               │
│                                                             │
│  3. Replay Transactions                                     │
│     └── For each valid transaction:                         │
│         ├── Verify commit block exists                      │
│         ├── Copy data blocks to filesystem area             │
│         └── Update in-memory structures                     │
│                                                             │
│  4. Checkpoint                                              │
│     └── Update journal superblock (clear committed TIDs)    │
│                                                             │
│  5. Enable New Transactions                                 │
│     └── Journal ready for new operations                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. 实现细节

### 4.1 纯 iomap 数据路径

**核心挑战**：在纯 iomap 架构中，没有 buffer_head 来标记元数据块。

**解决方案**：

```c
/* 标记元数据块脏 - 纯 iomap 实现 */
int simplefs_journal_dirty_metadata(struct simplefs_handle *handle,
                                    uint32_t block_nr, void *data)
{
    struct simplefs_journal_buffer *jb;
    
    // 1. 创建 journal buffer
    jb = kmalloc(sizeof(*jb), GFP_NOFS);
    jb->b_blocknr = block_nr;
    jb->b_data = kmalloc(SIMPLEFS_BLOCK_SIZE, GFP_NOFS);
    
    // 2. 复制数据（不是引用，确保一致性）
    memcpy(jb->b_data, data, SIMPLEFS_BLOCK_SIZE);
    
    // 3. 添加到事务列表
    list_add_tail(&jb->b_list, &transaction->t_buffers);
    transaction->t_nr_buffers++;
    
    return 0;
}
```

**与文件操作的集成**：

```c
static int simplefs_create(...)
{
    struct simplefs_handle *handle;
    void *folio_data;
    
    // 1. 开始事务
    handle = simplefs_journal_start(dir, 6);
    
    // 2. 执行操作
    // ... 分配 inode，写入目录项 ...
    
    // 3. 标记元数据脏
    folio_data = simplefs_get_folio(sb, block_nr, true);
    // ... 修改 folio_data ...
    simplefs_mark_folio_dirty(handle, sb, block_nr, folio_data);
    simplefs_put_folio(folio_data, true);
    
    // 4. 提交事务
    return simplefs_journal_stop(handle);
}
```

### 4.2 防止 Buffer Head 回归

```c
/* simplefs.h - 编译时保护 */
#ifdef _LINUX_BUFFER_HEAD_H
#error "simplefs.h should not include buffer_head.h - using pure iomap"
#endif
```

**文件操作中的模式**：

| 旧方法（使用 buffer_head） | 新方法（纯 iomap） |
|--------------------------|-------------------|
| `sb_getblk(sb, block)` | `simplefs_get_folio(sb, block, true)` |
| `bh->b_data` | `folio_data` (来自 `simplefs_get_folio`) |
| `mark_buffer_dirty(bh)` | `simplefs_mark_folio_dirty(handle, ...)` |
| `brelse(bh)` | `simplefs_put_folio(data, dirty)` |

### 4.3 事务 Credit 系统

每个文件操作需要预留足够的 journal 空间：

```c
/* Transaction credits for each operation */
#define SIMPLEFS_CREATE_CREDITS  6   /* dir block, inode block, 
                                       * 2 bitmap blocks, 2 data blocks */
#define SIMPLEFS_MKDIR_CREDITS   6   /* similar to create */
#define SIMPLEFS_UNLINK_CREDITS  5   /* dir block, inode block,
                                       * bitmap block, indirect block */
#define SIMPLEFS_RMDIR_CREDITS   6   /* dir + parent dir + inode + bitmaps */
#define SIMPLEFS_RENAME_CREDITS  6   /* 2 dir blocks + inode + bitmaps */
```

**Credit 计算**：
- 每个要修改的元数据块需要 1 个 credit
- 预留空间用于间接块和位图

### 4.4 Journal 位置布局

```
Filesystem Layout (100MB example):
┌──────────────────────────────────────────────────────────────┐
│ Block 0:     Superblock                                      │
├──────────────────────────────────────────────────────────────┤
│ Block 1:     Inode Bitmap                                    │
├──────────────────────────────────────────────────────────────┤
│ Block 2:     Data Bitmap                                     │
├──────────────────────────────────────────────────────────────┤
│ Block 3-10:  Inode Table (8 blocks = 1024 inodes)            │
├──────────────────────────────────────────────────────────────┤
│ Block 11-24575:  Data Blocks (user files and directories)    │
├──────────────────────────────────────────────────────────────┤
│ Block 24576:     Journal Superblock                          │
├──────────────────────────────────────────────────────────────┤
│ Block 24577-26623:  Journal Data (4MB - 1 block)             │
└──────────────────────────────────────────────────────────────┘
```

---

## 5. 边界条件与限制

### 5.1 当前限制

| 限制 | 值 | 说明 |
|------|-----|------|
| Journal 大小 | 4MB 固定 | 约 1000 个事务（平均 4 块/事务） |
| 单事务块数 | 16 最大 | 受限于 handle credits |
| 并发事务 | 1 | 完全串行 |
| 数据日志 | 不支持 | 仅元数据日志 |

### 5.2 边界处理

**Journal 空间不足**：
```c
if (journal_free_blocks() < required_blocks) {
    // 强制检查点，释放空间
    simplefs_journal_checkpoint();
}
```

**事务块数超限**：
```c
if (t_nr_buffers > JBD2_MAX_BATCH_SIZE) {
    // 分批提交，确保不超出单事务限制
    simplefs_transaction_commit_partial();
}
```

### 5.3 错误处理

**事务中止**：
```c
void simplefs_journal_abort(struct simplefs_handle *handle, int errno)
{
    handle->h_err = errno;
    transaction->t_state = T_ABORTED;
    // 清理已分配的 journal 空间
    // 不写入 commit 块，恢复时会忽略此事务
}
```

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
