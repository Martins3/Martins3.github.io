# SimpleFS

## 目标

最后整理好代码，文档，难点，容易错的点，
我的目标是，如果我有了一个清晰的小的文件系统，我就可以很方便的学习文件系统了。
我希望这个问题文件系统可以接受 xfstests 的 NORUN ，也就是功能不实现，
但是不可以接受错误


## 开发环境

### yyds-fs 虚拟机测试

### 物理机编译，虚拟机中运行
1. 先在主机上编译模块（因为虚拟机 NFS root 编译会有问题）
使用 build.sh 编译，无需拷贝到虚拟机VS，因为虚拟机通过 nfs 共享了物理机的 ~/data 目录:

2. 在虚拟机中用 root 直接执行测试:

```txt
ssh -p 51404 root@localhost "cd /home/martins3/data/vn/.worktrees/simplefs/simplefs && ./xfstests-full.sh"
```
xfstests-full.sh 是标准回归入口：不带参数跑全部 787 个 generic case，也可以传
单个 case（`./xfstests-full.sh 74`）或范围（`./xfstests-full.sh 100-200`）。

### 虚拟机中出现警告，应该及时重启虚拟机

不要用 ssh 到虚拟机中 reboot 的方法，那种方法由于是完整的 reboot 可能会很慢。

重启的方法:
```sh
./collei/scripts/collei-action.py -a force_reboot -n yyds-fs
```

### 如果遇到异常，及时检查内核日志

```txt
./collei/scripts/collei-action.py -a log -n yyds-fs
```

### 分析宕机问题
参考命令:
```txt
# 虚拟机中
cd /var/crash/127.0.0.1-2026-03-14-07:27:02
scp * martins3@10.0.0.2:/home/martins3/data/vn
crash ~/data/kernel/linux-drm/vmlinux vmcore
```

## 项目目标，通过 ~/data/xfstests 中的所有的 generic 测试
Keep working on this until your completely done, do not stop, do not pause. No hacks, no shortcuts, no giving up. Add this to your todo list as a reminder

## 文件系统概述

### 分区布局
```
+------------+-------------+-------------------+-------------------+-------------+
| 超级块     | 索引节点存储| 索引节点空闲位图  | 块空闲位图        | 数据块      |
+------------+-------------+-------------------+-------------------+-------------+
+------------+-------------+-------------------+-------------------+-------------+
| superblock | inode store | inode free bitmap | block free bitmap | data blocks |
+------------+-------------+-------------------+-------------------+-------------+
```
每个块大小为 4 KiB。

### 超级块
超级块位于分区的第一个块（块 0），存储分区的元数据。这包括总块数、总索引节点数，以及空闲索引节点和空闲块的数量。

The superblock, located at the first block of the partition (block 0), stores
the partition's metadata. This includes the total number of blocks, the total
number of inodes, and the counts of free inodes and blocks.

### 索引节点存储区
此区域包含分区中的所有索引节点，最大索引节点数等于分区中的块数。每个索引节点占用 72 字节数据，
包含标准信息如文件大小和使用的块数，以及一个 simplefs 特有的字段 `ei_block`。该字段 `ei_block` 根据文件类型有不同的用途：
  - 对于目录，它包含该目录中的文件列表。
    一个目录最多可以容纳 40,920 个文件，文件名限制为最多 255 个字符，以确保它们能放入单个块中。

This section contains all the inodes of the partition, with the maximum number
of inodes being equal to the number of blocks in the partition. Each inode
occupies 72 bytes of data, encompassing standard information such as the file
size and the number of blocks used, in addition to a simplefs-specific field
named `ei_block`. This field, `ei_block`, serves different purposes depending
on the type of file:
  - For a directory, it contains the list of files within that directory.
    A directory can hold a maximum of 40,920 files, with filenames restricted
    to a maximum of 255 characters to ensure they fit within a single block.

```
  inode
  +-----------------------+
  | i_mode = IFDIR | 0755 |      block 123 (simplefs_file_ei_block)
  | ei_block = 123    ----|--->  +----------------+
  | i_size = 4 KiB        |      | nr_files  = 7  |
  | i_blocks = 1          |      |----------------|
  +-----------------------+    0 | ee_block  = 0  |
                                 | ee_len    = 8  |      block 84(simplefs_dir_block)
                                 | ee_start  = 84 |--->  +-------------+
                                 | nr_file   = 2  |      |nr_files = 2 |
                                 |----------------|      |-------------|
                               1 | ee_block  = 8  |    0 | inode  = 24 |
                                 | ee_len    = 8  |      | nr_blk = 1  |
                                 | ee_start  = 16 |      | (foo)       |
                                 | nr_file   = 5  |      |-------------|
                                 |----------------|    1 | inode  = 45 |
                                 | ...            |      | nr_blk = 14 |
                                 |----------------|      | (bar)       |
                             341 | ee_block  = 0  |      |-------------|
                                 | ee_len    = 0  |      | ...         |
                                 | ee_start  = 0  |      |-------------|
                                 | nr_file   = 12 |   14 | 0           |
                                 +----------------+      +-------------+
```

- 对于文件，它列出保存文件实际数据的扩展。由于块 ID 以 `sizeof(struct simplefs_extent)` 字节存储，单个块最多可容纳 341 个链接。
这个限制将文件的最大大小限制在约 10.65 MiB（10,912 KiB）。
```
inode
+-----------------------+
| i_mode = IFDIR | 0644 |          block 93
| ei_block = 93     ----|------>  +----------------+
| i_size = 10 KiB       |       0 | ee_block  = 0  |
| i_blocks = 25         |         | ee_len    = 8  |      extent 94
+-----------------------+         | ee_start  = 94 |---> +--------+
                                  |----------------|     |        |
                                1 | ee_block  = 8  |     +--------+
                                  | ee_len    = 8  |      extent 99
                                  | ee_start  = 99 |---> +--------+
                                  |----------------|     |        |
                                2 | ee_block  = 16 |     +--------+
                                  | ee_len    = 8  |      extent 66
                                  | ee_start  = 66 |---> +--------+
                                  |----------------|     |        |
                                  | ...            |     +--------+
                                  |----------------|
                              341 | ee_block  = 0  |
                                  | ee_len    = 0  |
                                  | ee_start  = 0  |
                                  +----------------+
```

### Extent support
扩展跨越连续的块；因此，我们在单个操作中为它分配连续的磁盘块。
它由 `struct simplefs_extent` 定义，包含三个成员：
- `ee_block`：扩展覆盖的第一个逻辑块。
- `ee_len`：扩展覆盖的块数。
- `ee_start`：扩展覆盖的第一个物理块。

An extent spans consecutive blocks; therefore, we allocate consecutive disk blocks
for it in a single operation. It is defined by `struct simplefs_extent`, which
comprises three members:
- `ee_block`: the first logical block that the extent covers.
- `ee_len`: the number of blocks the extent covers.
- `ee_start`: the first physical block that the extent covers."

```
struct simplefs_extent
  +----------------+
  | ee_block =  0  |
  | ee_len   =  200|              extent
  | ee_start =  12 |-----------> +---------+
  +----------------+    block 12 |         |
                                 +---------+
                              13 |         |
                                 +---------+
                                 | ...     |
                                 +---------+
                             211 |         |
                                 +---------+
```

## 注意记得参考 /home/martins3/data/kernel/linux-drm 中现有的 fs 的实现，尤其是 xfs ext4 ext2

## 元数据缓存约束

SimpleFS 核心实现必须保持 folio-native，不能直接依赖 `buffer_head`。
`buffer_head` 只允许封装在 JBD2 适配层中，用于调用上游 JBD2 客户端 API；
不得扩散到 inode、目录、extent、xattr、superblock 或普通文件 I/O 路径。

## 修改代码后，记得立刻回归

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
