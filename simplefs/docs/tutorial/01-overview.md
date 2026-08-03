# 1. 总览：四层模型与源码地图

文件系统同时面对四种语言：用户看到路径和文件描述符，VFS 看到内存对象与回调，
页缓存和 iomap 看到 offset 到 block 的映射，磁盘只看到固定位置的字节。很多难题
都来自把其中两层误认为同一层。

## 四层模型

```text
用户语义       open/read/write/rename/fsync
                    |
VFS 对象       mount -> dentry -> inode <- file
                    |
缓存与映射     address_space/folio -> iomap -> bio
                    |
磁盘事实       superblock/inode/bitmap/dirent/extent/journal/data
```

`struct file` 是一次打开的状态，保存 offset、flags 和 `file_operations`；
`struct dentry` 是某个父目录下的名字缓存；`struct inode` 是文件对象本身；
`struct super_block` 是一次已挂载文件系统实例。多个 dentry 可以指向同一 inode，
多个 file 也可以指向同一 dentry，因此它们不能合并成一个“文件结构”。

## 五组操作表

| 操作表 | SimpleFS 注册处 | 解决的问题 |
| --- | --- | --- |
| `file_system_type` | `simplefs_fs.c` | 这种文件系统如何创建和销毁挂载实例 |
| `super_operations` | `simplefs_super.c` | inode 生命周期、sync、statfs、freeze |
| `inode_operations` | `simplefs_inode.c` | lookup/create/unlink/rename/setattr 等命名空间语义 |
| `file_operations` | `simplefs_file.c`, `simplefs_dir.c` | 已打开普通文件或目录如何读写、遍历和 fsync |
| `address_space_operations` | `simplefs_file.c` | page cache folio 如何读入、标脏、写回和失效 |

学习一个功能时先找操作表，再找回调的 helper。这样能区分“VFS 已经做掉的通用工作”
与“SimpleFS 必须提供的格式相关工作”。

## 三条代表性调用链

路径查找：

```text
openat("dir/a")
  -> VFS pathname walk
  -> simplefs_lookup(parent inode, dentry)
  -> scan directory entries
  -> simplefs_iget(inode number)
  -> d_splice_alias(dentry, inode)
```

buffered write：

```text
write(fd, buffer)
  -> simplefs_file_write_iter
  -> iomap_file_buffered_write
  -> simplefs_write_iomap_begin/end
  -> copy into page cache and mark folio dirty
  ... later ...
  -> simplefs_writepages
  -> iomap_writepages -> bio
  -> successful ioend converts unwritten extent
```

创建文件：

```text
openat(..., O_CREAT)
  -> simplefs_create
  -> start journal transaction
  -> allocate inode and its metadata root
  -> add parent directory entry
  -> persist parent/new inode
  -> instantiate dentry
  -> stop transaction
```

这三条链分别代表命名空间、文件数据和多对象元数据事务，是后续章节的骨架。

## SimpleFS 简单在哪里，又不简单在哪里

它简单在：固定 4 KiB block、固定 inode table、线性位图、目录项格式直接、普通
文件 extent tree 只有两级、没有 reflink/quota/encryption/compression。

它并不玩具化：支持 page cache、mmap、DIO、unwritten extent、fallocate、xattr、
ACL、NFS file handle、freeze、discard 和 JBD2。只要允许并发、缓存和崩溃，
“先改哪一个对象”与“错误时谁仍拥有这个块”就与生产文件系统同样重要。

## 对照源码

- `fs/namei.c`：系统调用如何走到 `inode_operations`；
- `fs/open.c`、`fs/read_write.c`：file 和 read/write 的 VFS 入口；
- `fs/ext2/`：结构最接近教科书式块文件系统；
- `fs/xfs/xfs_file.c`、`fs/xfs/xfs_iomap.c`：成熟 iomap 路径。

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
