# jbd2

Documentation/filesystems/ext4/journal.rst

到时候测试一下这个效果:
- jbd2_journal_abort
  - fs/jbd2/revoke.c

一个普通的系统中，jbd2 的调用路径:
```txt
  b'do_get_write_access'
  b'jbd2_journal_get_write_access'
  b'__ext4_journal_get_write_access'
  b'ext4_reserve_inode_write'
  b'ext4_mark_inode_dirty'
  b'ext4_dirty_inode'
  b'__mark_inode_dirty'
  b'ext4_setattr'
  b'notify_change'
  b'chmod_common'
  b'ksys_fchmod'
  b'__x64_sys_fchmod'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
    126

  b'do_get_write_access'
  b'jbd2_journal_get_write_access'
  b'__ext4_journal_get_write_access'
  b'ext4_reserve_inode_write'
  b'ext4_mark_inode_dirty'
  b'ext4_dirty_inode'
  b'__mark_inode_dirty'
  b'generic_update_time'
  b'file_update_time'
  b'__generic_file_write_iter'
  b'ext4_file_write_iter'
  b'__vfs_write'
  b'vfs_write'
  b'ksys_write'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
    128

  b'do_get_write_access'
  b'jbd2_journal_get_write_access'
  b'__ext4_journal_get_write_access'
  b'ext4_mb_mark_diskspace_used'
  b'ext4_mb_new_blocks'
  b'ext4_ext_map_blocks'
  b'ext4_map_blocks'
  b'ext4_alloc_file_blocks'
  b'ext4_fallocate'
  b'vfs_fallocate'
  b'ksys_fallocate'
  b'__x64_sys_fallocate'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
    135

  b'do_get_write_access'
  b'jbd2_journal_get_write_access'
  b'__ext4_journal_get_write_access'
  b'__ext4_new_inode'
  b'ext4_create'
  b'path_openat'
  b'do_filp_open'
  b'do_sys_open'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
    136

  b'do_get_write_access'
  b'jbd2_journal_get_write_access'
  b'__ext4_journal_get_write_access'
  b'ext4_reserve_inode_write'
  b'ext4_mark_inode_dirty'
  b'ext4_ext_tree_init'
  b'__ext4_new_inode'
  b'ext4_create'
  b'path_openat'
  b'do_filp_open'
  b'do_sys_open'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
    136

  b'do_get_write_access'
  b'jbd2_journal_get_write_access'
  b'__ext4_journal_get_write_access'
  b'ext4_reserve_inode_write'
  b'ext4_mark_inode_dirty'
  b'add_dirent_to_buf'
  b'ext4_add_entry'
  b'ext4_add_nondir'
  b'ext4_create'
  b'path_openat'
  b'do_filp_open'
  b'do_sys_open'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
    149

  b'do_get_write_access'
  b'jbd2_journal_get_write_access'
  b'__ext4_journal_get_write_access'
  b'add_dirent_to_buf'
  b'ext4_add_entry'
  b'ext4_add_nondir'
  b'ext4_create'
  b'path_openat'
  b'do_filp_open'
  b'do_sys_open'
  b'do_syscall_64'
  b'entry_SYSCALL_64_after_hwframe'
    180
```

看上去，也就是当文件系统的 meta 变化的时候，
会调用这个东西。

例如 add_dirent_to_buf 中调用这个:
```txt
	err = ext4_journal_get_write_access(handle, bh);
```

## 似乎 fsync 会导致这个

```txt
  b'raid1_make_request'
  b'md_handle_request'
  b'__submit_bio'
  b'__submit_bio_noacct'
  b'jbd2_journal_commit_transaction'
  b'kjournald2'
  b'kthread'
  b'ret_from_fork'
  b'ret_from_fork_asm'
    1856
```

## 哦，还有这个目录哦
/proc/fs/jbd2/dm-2-8/info

## 来点仔细的思考

### 为什么没有 journal 之后，修复需要全盘扫描？

### journal 居然是一个原子性解决方案

如果执行到了一半，之后重放 journal 日志就可以了，但是重放日志的过程中，
需要判断之前的操作是不是做了一半，当然最好是，之前的工作都是可以直接重复做的。

如果日志记录了一半咋办？（先进入到队列，然后刷新队列数值）

### [ ] 先来一个经典的例子吧

## fallocate 的时候，就是需要记录 journal 吧
- 例如增大文件，那么就需要修改磁盘的结构，那么就需要
，不然 crash 掉，会有很多问题吧。

## 三种日志模式
https://www.phoronix.com/news/EXT4-Deprecates-Journal-Mode

就这篇文章讨论的 数据日志模式（data=） 而言，ext4 有三种：journal、ordered、writeback。此外，ext4 还支持无日志运行，所以宽泛地说是“三种日志模式，加一种
无日志配置”。

新闻里弃用的是 data=journal。目前内核文档给出的移除时间是 2028 年 1 月；默认的 data=ordered 仍然保留元数据日志。内核文档
(https://www.kernel.org/doc/html/latest/admin-guide/ext4.html#data-mode)

 模式                  元数据进入日志    文件内容进入日志    核心约束
━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 data=journal          是                是                  数据、元数据先写日志，事务提交后再写正式位置
────────────────────  ────────────────  ──────────────────  ──────────────────────────────────────────────
 data=ordered，默认    是                否                  相关数据写入正式位置后，才能提交元数据事务
────────────────────  ────────────────  ──────────────────  ──────────────────────────────────────────────
 data=writeback        是                否                  元数据事务提交不等待相关文件数据
────────────────────  ────────────────  ──────────────────  ──────────────────────────────────────────────
 无日志                否                否                  直接更新数据与元数据，没有 JBD2 事务恢复

这里的“元数据”包括 inode、extent 树、分配位图、目录项等。目录块虽然也是块，其内容属于文件系统元数据。

先理解它们共同使用的日志机制。

ext4 的日志由 JBD2 实现，基本过程是：

修改内存中的块
    ↓
把修改后的块写入 journal
    ↓
持久化 commit record：事务提交
    ↓
随后将块写回文件系统正式位置，并完成 checkpoint
    ↓
相应日志空间可以回收

因此，事务提交不要求正式位置已经更新完毕：只要日志中已有完整、有效的提交记录，崩溃后就能重放。没有完成提交的事务会被恢复过程忽略。JBD2 文档
(https://www.kernel.org/doc/html/latest/filesystems/ext4/journal.html)

三种模式的区别在于：哪些块参加这个过程，以及提交前额外等待什么。

1. data=journal：文件内容也作为事务的一部分。

以“向文件追加一个新块”为例：

文件数据 D + 元数据 M → journal → commit
                                      ↓
                           D、M 写回各自正式位置

崩溃发生在 commit 之后、正式位置尚未写完时，恢复过程可以从日志重放文件内容和元数据。

实现上，文件使用专门的 ext4_journalled_aops。普通 buffered write 的关键调用链是：

ext4_journalled_write_end()
  → write_end_fn()
    → ext4_dirty_journalled_data()
      → ext4_handle_dirty_metadata()
        → jbd2_journal_dirty_metadata()

最后这个函数虽然叫 dirty_metadata，这里传入的却可以是文件数据块：JBD2 负责记录块，ext4 决定哪些块交给它。见 journal 数据处理代码 (fs/ext4/
inode.c:1149)。

代价是文件内容通常要写两遍：一次日志，一次正式位置。当前实现还会禁用 delayed allocation、Direct I/O、fast commit 等功能，见 挂载检查 (fs/ext4/
super.c:5040)。

2. data=ordered：文件内容直接写正式位置，但提交元数据前要等它。

逻辑依赖是：

相关文件数据 D → 正式位置 ──┐
                            ├→ commit → 元数据随后 checkpoint
元数据 M      → journal  ───┘

这里最关键的是 D 必须先于 commit 持久化。文件数据写入和元数据日志写入可以重叠，不要求所有 I/O 串行执行。

实现上，ext4 会把需要排序的数据范围登记到事务中。例如，新分配的块在事务提交后将变得可见时，通过：

ext4_jbd2_inode_add_write() / ext4_jbd2_inode_add_wait()
    → 将 inode 和相关范围关联到 JBD2 transaction

JBD2 提交时：

journal_submit_data_buffers()          提交相关数据写入
journal_finish_inode_data_buffers()    等待数据写入完成
journal_submit_commit_record()        提交事务

对应代码是 ext4 登记数据依赖 (fs/ext4/inode.c:835) 和 JBD2 提交流程 (fs/jbd2/commit.c:572)。

这样，对于新增块，不会仅仅提交“文件已经引用这个块”的元数据，却还没把本次数据写进去。它没有把文件内容存进日志，恢复时也无法从日志找回文件内容。

还要注意，ordered 约束的是相关数据与元数据事务的关系，尤其是新分配、新暴露的数据块；它不会把对已有文件内容的任意覆盖写变成原子更新。

3. data=writeback：文件数据回写与元数据事务提交没有上述依赖。

元数据 M → journal → commit → 元数据 checkpoint

文件数据 D → 正式位置
             ↑
       可以晚于上面的 commit

它和 ordered 一样，只对元数据记日志，但提交元数据事务时不要求相关文件数据已经写完。

因此，崩溃恢复后，文件系统的元数据可以保持结构一致，而刚写过的文件内容可能不是应用期望的数据，例如仍然是旧内容。具体表现还受 unwritten extent
等机制影响。内核对三种模式的说明 (https://www.kernel.org/doc/html/latest/admin-guide/ext4.html#data-mode)

从代码组织上看，ordered 和 writeback 大量共用数据写入路径。在 ext4_set_aops() (fs/ext4/inode.c:4005) 中，两者进入相同分支，根据是否开启 delayed
allocation 等条件选择 aops。主要差别是 ordered 会建立上述提交依赖。

无日志、fast commit 又分别是什么？

无日志配置下，s_journal == NULL，元数据修改直接进入普通脏块回写路径，不再交给 JBD2。崩溃后也就没有元数据事务日志可供重放，可能需要通过 fsck 修复。代码
在 元数据脏块处理 (fs/ext4/ext4_jbd2.c:353)。data=writeback 仍然有元数据日志，与无日志配置有本质区别。

fast_commit 则是 ordered 模式下的提交优化：尽可能记录重建元数据所需的变化量，不满足条件时回退到完整提交。它不是第四种 data=
模式；delalloc、O_DIRECT、DAX 也属于其他维度。fast commit 说明 (https://www.kernel.org/doc/html/latest/filesystems/ext4/journal.html)

最后，无论选择哪种模式，普通 write() 返回都不代表数据已经持久化。data=journal 提供的是 JBD2 事务层面的恢复能力，不能替代应用所需的 fsync()，也不意味着
任意一次跨多个块的 write() 都具有掉电原子性。当前 ext4_sync_file() (fs/ext4/fsync.c:143) 会协调数据写回、事务提交以及必要的设备 flush。

### human

2026-10-14 这里我没深入的看，两个问题
1. date=order 和 data=writeback 的区别实际上，在这里我没有看懂
2. 其他的文件系统如何处理 data 日志问题的?

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
