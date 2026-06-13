# fs jbd2

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
