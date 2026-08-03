# 2. 磁盘格式：哪些事实必须持久化

磁盘格式不是 C 内存结构的随手 dump，而是一份长期协议。字段宽度决定容量，单位
决定 ABI 是否正确，块之间的引用决定崩溃后能否重建所有权。SimpleFS 的定义集中在
`simplefs.h`，布局计算和初始化集中在 `mkfs_common.c`。

## 分区布局

```text
block 0
+--------------------+
| superblock         |
+--------------------+
| inode store        | nr_istore_blocks
+--------------------+
| free inode bitmap  | nr_ifree_blocks
+--------------------+
| free block bitmap  | nr_bfree_blocks
+--------------------+
| data/metadata area |
|                    |
+--------------------+
| JBD2 journal       | optional, reserved tail range
+--------------------+
```

所有块固定为 4 KiB。位图中置 1 表示 free，清 0 表示 allocated。journal 尾区既要
从空闲计数中扣除，也必须在 block bitmap 中标为占用；计数只是缓存，位图才是块
所有权的权威来源。

## Superblock

磁盘上的 `struct simplefs_sb_info` 前 112 字节保存：magic、总 block/inode 数、
三个元数据区域长度、两个空闲计数、journal 位置与模式、63 字节卷标，以及
`s_needs_recovery`。`#ifdef __KERNEL__` 后的锁、指针和内存位图不属于磁盘格式。

区分这两部分很重要：mkfs 和内核共享格式字段，但绝不能把内核指针写到磁盘。

`nr_blocks`、`s_journal_start` 和物理块引用都是 32 位。固定 4 KiB block 下，卷大小
必须小于 2^32 blocks，也就是小于 16 TiB；17 TiB dm-huge-disk 测试属于当前磁盘
格式明确不支持的能力，不能让 mkfs 继续计算并截断块号。

## Inode table

当前 `struct simplefs_inode` 是 104 字节，因此一个 4 KiB inode-table block 可容纳
39 个 inode。它保存 mode、uid/gid、32 位 size、三组时间戳、btime、块计数、
nlink、generation、flags、xattr block、`ei_block` 和 32 字节内联数据。

几个容易错的单位：

- 磁盘 `i_blocks` 以 4 KiB SimpleFS block 计；
- VFS `inode->i_blocks` 和用户态 `stat.st_blocks` 固定以 512 字节扇区计；
- 磁盘时间秒是有符号 32 位，VFS 内存使用 `time64_t`；
- 磁盘 `i_size` 是 32 位，但 `sb->s_maxbytes` 由 extent 格式给出。两者目前存在
  格式演进上的不对称，阅读大文件行为时必须同时检查，不能只看一个常量。

`ei_block` 的解释由文件类型决定：普通文件指向 extent root；目录指向目录 extent
索引块；长 symlink 指向数据块；短 symlink 保存在 `i_data[32]`。

## 普通文件 extent tree

普通文件使用固定两级树：

```text
inode.ei_block
      |
      v
+---------------+       +----------------+
| root header   |       | leaf header    |
| index[0] -----|------>| extent[]       |--> data blocks
| index[1] -----|---+   +----------------+
+---------------+   |   +----------------+
                    +-->| extent[]       |--> data blocks
                        +----------------+
```

root 最多 255 个 index，leaf 最多 340 个 extent，每个 extent 最多 8192 个 block。
理论上限为 86,700 个 extent，约 2.91 TB（2.65 TiB）。这已经取代了早期“单个
extent block、最大约 10 MiB”的设计；旧笔记不能用于解释当前代码。

## 目录格式

目录的 `ei_block` 指向 `simplefs_file_ei_block`，其中保存文件数和最多 341 个目录
extent。每个目录 extent 固定 8 个 block。目录数据 block 是定长
`{ inode, filename[255] }` 数组，每块 15 项，因此上限是：

```text
15 entries/block * 8 blocks/extent * 341 extents = 40,920 entries
```

255 字节名字可以填满整个字段，没有结尾 NUL。内核必须按 `qstr.len` 写入，并用
有界 `strnlen` 读出，不能把磁盘字段当普通 C 字符串。

## Xattr 与其他块

每个 inode 最多有一个 4 KiB xattr block。entry 从块头向后增长，value 从块尾向前
增长。ACL 作为 system xattr 复用这一个块。目录索引、目录数据、extent root/leaf、
xattr 和长 symlink 都是“由 inode 间接拥有的元数据块”，释放时不仅要清位图，还要
处理 bdev mapping 中可能残留的 folio。

## 从 mkfs 验证格式

阅读顺序建议是 `mkfs_calculate_layout()` → `mkfs_init_superblock()` →
`mkfs_init_root_inode()` → 各区域 write helper。每个布局结论都应能同时由三个证据
支持：格式常量、mkfs 写入位置、mount 读取位置。

## 对照源码

- `fs/ext2/ext2.h`、`fs/ext2/super.c`：固定 inode table 与 bitmap；
- `fs/ext4/ext4_extents.h`：更完整的多级 extent tree；
- `fs/xfs/libxfs/`：显式 on-disk 类型和 endian 转换。

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
