# SimpleFS 源码索引

## 按文件

| 文件 | 主要入口 |
| --- | --- |
| `simplefs_fs.c` | `simplefs_init`, `simplefs_init_fs_context`, `simplefs_parse_param` |
| `simplefs_super.c` | `simplefs_fill_super`, `simplefs_iget` 的持久化配套、super ops、export ops |
| `simplefs_inode.c` | lookup/create/unlink/rmdir/rename/link/symlink/tmpfile/mknod/setattr |
| `simplefs_dir.c` | `simplefs_iterate`, `simplefs_dir_fsync` |
| `simplefs_file.c` | read/write iomap、writeback、DIO、mmap、fallocate、fiemap、ioctl、fsync |
| `simplefs_extent.c` | extent root/leaf load、find、normalize、sync、free |
| `simplefs_metabuf.c` | `simplefs_get_folio`, `simplefs_retire_metadata_blocks` |
| `simplefs_journal.c` | JBD2 load/start/dirty/stop/commit/recovery adapter |
| `simplefs_xattr.c` | xattr block validate/get/set/list/delete |
| `simplefs_acl.c` | ACL get/set/inherit |
| `mkfs_common.c` | layout、root inode、bitmap、journal 写入与回读校验 |

## 按用户动作

| 动作 | 第一批 SimpleFS 入口 |
| --- | --- |
| `mount -t simplefs` | `simplefs_get_tree` → `simplefs_fill_super` |
| `open("a")` | VFS pathname walk → `simplefs_lookup` → `simplefs_iget` |
| `open(..., O_CREAT)` | `simplefs_create` → `simplefs_create_internal` |
| `ls` | `simplefs_iterate` |
| `read` | `simplefs_file_read_iter` → filemap/iomap read |
| `write` | `simplefs_file_write_iter` → write iomap → writeback |
| `mmap` 写 | `simplefs_file_mmap_prepare` → `simplefs_page_mkwrite` |
| `O_DIRECT` | `simplefs_file_read_iter/write_iter` → `iomap_dio_rw` |
| `truncate` | `simplefs_setattr` → `simplefs_truncate` |
| `fallocate` | `simplefs_fallocate` 的各 mode 分支 |
| `fsync` | `simplefs_file_fsync` 或 `simplefs_dir_fsync` |
| `setxattr` | handler → `simplefs_xattr_set` |
| `setfacl` | `simplefs_set_acl` → xattr |
| `umount` | `simplefs_kill_sb` → `simplefs_put_super` |

## 操作表位置

- `simplefs_file_system_type`：`simplefs_fs.c`；
- `simplefs_super_ops`, `simplefs_export_ops`：`simplefs_super.c`；
- `simplefs_inode_ops`, `symlink_inode_ops`：`simplefs_inode.c`；
- `simplefs_dir_ops`：`simplefs_dir.c`；
- `simple_fs_iomap_fops`, `simplefs_iomap_aops`, read/write iomap ops：
  `simplefs_file.c`。

操作表是最稳定的阅读起点。函数行号随代码变化，不在文档里固化。

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
