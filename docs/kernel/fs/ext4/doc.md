# ext4 文档
## 内部文档
https://docs.kernel.org/filesystems/ext4/

- Blocks
- Block Groups
- Group Descriptors
- Bitmaps
- Inode Table

文件数据组织方式：
Extent 树：这是 ext4 的主要特性，用以替代 ext2/3 的间接块映射。它通过树形结构高效地记录文件数据块的连续范围，极大减少了大文件的元数据开销。
传统块映射 (Direct/Indirect Block Addressing)：为了向后兼容，文档也解释了旧的三级间接块寻址方式。
目录结构 (Directory Entries)：详细描述了目录如何存储为特殊文件，包含线性目录和更高效的哈希树 (htree) 目录两种格式。

高级特性和优化：
- 分配策略 (Allocation Policy)：探讨了 ext4 如何通过多块分配器、延迟分配、数据局部性（将文件数据、inode、目录项尽量放在一起）等策略来减少碎片。
- 校验和 (Checksums)：介绍了为超级块、组描述符、位图、inode、目录项、Extent 块等关键元数据添加校验和的功能，以增强数据一致性检测能力。
- 大块分配 (Bigalloc)：允许以“簇”（多个块的集合）为单位进行分配，适用于存储超大文件的场景，可以减少元数据量。
- 内联数据 (Inline Data)：对于非常小的文件或目录，其数据可以直接存储在 inode 结构内部，避免分配单独的数据块。
- 扩展属性 (Extended Attributes, xattrs)：解释了如何存储文件的额外元数据（如 ACL），包括存储在 inode 内部或外部数据块中的格式。
- 原子写入 (Atomic Block Writes)：详细说明了 ext4 如何支持硬件级别的原子写入操作，确保写入要么完全成功，要么完全失败，防止“撕裂写入”。

日志 (Journal, jbd2)：深入剖析了 ext4 用于保证元数据一致性的日志系统，包括其磁盘布局、事务结构（描述符块、数据块、提交块、撤销块）、外部日志以及新的“快速提交 (Fast Commit)”功能。
特殊 inode：列出了保留给特定功能的 inode 编号，如根目录 (2)、日志 (8)、配额文件等。

## inode table
Inode tables are statically allocated at mkfs time.  Each block group
descriptor points to the start of the table, and the superblock records
the number of inodes per group.  See the section on inodes for more
information.

应该是一个 block group ，就有一个 inode table ，inode table 是静态分配的。

## ext4 : extend 树
<!-- 4a616a59-ff01-49db-9c1f-16e135be938c -->

树的结构如下：

- 根节点 (Root Node)：直接存储在 inode 的 `i_block[15]` 数组中。这是一个非常聪明的设计，意味着对于小文件（extent 数量 ≤ 4），根本不需要额外的磁盘块来存储元数据，所有信息都在 inode 里。
- 内部节点 (Index Nodes / Interior Nodes)：如果 extent 太多，根节点放不下，就会分配额外的磁盘块作为内部节点。这些节点不直接指向数据块，而是指向下一层的节点（可以是内部节点或叶子节点）。
- 叶子节点 (Leaf Nodes)：树的最底层，包含实际指向文件数据块的 extent 条目。

```c
/*
 * Each block (leaves and indexes), even inode-stored has header.
 */
struct ext4_extent_header {
	__le16	eh_magic;	/* probably will support different formats */
	__le16	eh_entries;	/* number of valid entries */
	__le16	eh_max;		/* capacity of store in entries */
	__le16	eh_depth;	/* has tree real underlying blocks? */
	__le32	eh_generation;	/* generation of the tree */
};
```
- eh_depth = 0：这是一个叶子节点，其后的条目是 struct ext4_extent。
- eh_depth > 0：这是一个内部节点，其后的条目是 struct ext4_extent_idx。


```c
/*
 * This is the extent on-disk structure.
 * It's used at the bottom of the tree.
 */
struct ext4_extent {
	__le32	ee_block;	/* first logical block extent covers */
	__le16	ee_len;		/* number of blocks covered by extent */
	__le16	ee_start_hi;	/* high 16 bits of physical block */
	__le32	ee_start_lo;	/* low 32 bits of physical block */
};

/*
 * This is index on-disk structure.
 * It's used at all the levels except the bottom.
 */
struct ext4_extent_idx {
	__le32	ei_block;	/* index covers logical blocks from 'block' */
	__le32	ei_leaf_lo;	/* pointer to the physical block of the next *
				 * level. leaf or next index could be there */
	__le16	ei_leaf_hi;	/* high 16 bits of physical block */
	__u16	ei_unused;
};
```
ext4_extent 中的 logical 就文件中偏移量，而 physical block 就是在盘中偏移量

struct ext4_extent_idx (内部节点条目)：
- ei_block：这个索引条目所覆盖的子树中，最小的文件逻辑块号。
- ei_leaf_hi / ei_leaf_lo：指向的下一层节点（子树根节点）的物理块号

高级特性:
- 未初始化 Extent (Uninitialized Extent)：ee_len 字段的最高位被用作标志位。如果 ee_len > 32768，则表示这是一个“未初始化”的 extent。这意味着文件系统已经为这个范围预分配了磁盘空间，但里面的数据还是垃圾（未被写入）。当应用程序实际写入数据时，文件系统会将这个 extent 标记为“已初始化”。这有助于减少碎片。
- 校验和 (Checksum)：为了保证元数据的完整性，ext4 会对存储 extent 树的磁盘块（非 inode 内的根节点）计算校验和，并存储在块末尾的 struct ext4_extent_tail 中。
- 与 Flex_bg 和 Bigalloc 的协同：结合“柔性块组”（flex_bg）和“大块分配”（bigalloc）特性，extent 树可以更高效地分配超大、连续的存储空间，进一步减少元数据开销。

## ext4 : 哈希树 (htree) 目录
<!-- 22053474-5a0e-4a3e-86d9-6b566eb06f50 -->

1. 根节点 (Root Node)：
```c
struct dx_root
{
	struct fake_dirent dot;
	char dot_name[4];
	struct fake_dirent dotdot;
	char dotdot_name[4];
	struct dx_root_info
	{
		__le32 reserved_zero;
		u8 hash_version;
		u8 info_length; /* 8 */
		u8 indirect_levels;
		u8 unused_flags;
	}
	info;
	struct dx_entry	entries[];
};
```

- 位于目录文件的第一个数据块。
- 开头必须包含 . 和 .. 这两个特殊的目录项，这是 ext2 的传统。
- 紧接着是 struct dx_root_info，它包含了树的元信息，如哈希算法类型、树的深度等。
- 最后是一个“哈希 -> 块号”的映射表 (struct dx_entry)。这个表告诉系统：“哈希值在 0x0000-0x1FFF 范围内的文件，去第 2 个数据块找；哈希值在 0x2000-0x3FFF 范围内的文件，去第 3 个数据块找...”。

2. 内部节点 (Interior Nodes)：
```c
struct dx_node
{
	struct fake_dirent fake;
	struct dx_entry	entries[];
};
```

- 如果树的深度大于 1，根节点指向的就是内部节点。
- 内部节点的开头是一个“伪装”的、无效的目录项（inode号为0），这样旧的线性扫描程序会认为这个块是“空的”，从而跳过它。
- 紧接着也是一个“哈希 -> 块号”的映射表，用于进一步细化查找范围。
- 叶子节点 (Leaf Nodes)：

3. 树的最底层。
```c
/*
 * The new version of the directory entry.  Since EXT4 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 */
struct ext4_dir_entry_2 {
	__le32	inode;			/* Inode number */
	__le16	rec_len;		/* Directory entry length */
	__u8	name_len;		/* Name length */
	__u8	file_type;		/* See file type macros EXT4_FT_* below */
	char	name[EXT4_NAME_LEN];	/* File name */
};
```

- 存储的是真实的、线性的 struct ext4_dir_entry_2 目录项数组。
- 一个叶子节点内的所有目录项，它们的文件名哈希值都落在同一个范围内。
- 如果一个叶子节点存不下所有哈希值相同的项，它们会“溢出”到下一个叶子节点，并在上层索引中做标记。


htree 的一个精妙之处在于它的向后兼容性。
- 对旧系统：旧的、不支持 htree 的程序或内核，在读取一个 htree 目录时，会把它当作一个普通的线性目录文件来处理。它会从第一个块开始读，看到 . 和 .. 后，接着读到那些“伪装”的无效目录项（inode=0），就会认为后续的数据块是“空的”，从而跳过所有包含树结构的内部节点和根节点的索引部分，直接去读取真正的叶子节点数据块。虽然这样会遍历所有叶子块，效率不高，但至少能正常工作，不会出错。
- 对新系统：支持 htree 的系统会识别 inode 的 EXT4_INDEX_FL 标志位，知道这是一个树形目录，然后使用高效的树形查找算法。

## bigalloc
类似文件系统中 hugetlb 了:

mkfs.ext4 -F -O bigalloc -b 4096 -C 65536 /dev/nvme1n1
sudo dumpe2fs -h  /dev/nvme1n1

可以看到 :

```txt
Cluster size:             65536
```

## Atomic Block Writes

忽然意识到，没有操作元数据，就不要和 journal 打交道了

## jbd2
<!-- f66b4b1e-c8c1-4bb8-ad78-7a5c6a76c3ed -->

在将任何脏的元数据块写回到它们在文件系统中的“最终位置”（home location）之前，jbd2 会先将这些块的副本（或描述其变更的信息）按顺序写入到一个连续的、专用的日志区域（journal）。

只有当整个事务的所有块都安全地写入日志并被“提交”（commit）后，jbd2 才会在后台异步地将这些块写回到它们原本在文件系统中的位置。

崩溃恢复的关键：如果系统在“写日志”和“写回原位置”之间崩溃，重启后，文件系统只需重放（replay）日志中已提交的事务，就能将文件系统恢复到崩溃前的一致状态。那些未提交的事务会被直接丢弃。

高性能与低延迟：
- 批量提交：jbd2 不会为每个小操作都立即提交一个事务，而是会将多个操作打包成一个事务一起提交，以减少磁盘寻道和旋转延迟。
- 异步写回：提交事务后，将数据块写回其“最终位置”的操作是异步进行的，不会阻塞上层应用。
- Fast Commit：这是 jbd2 的一个高级优化（在文档中提及）。对于某些简单的操作（如扩展文件大小），它不记录完整的数据块，而是只记录一个非常小的“操作摘要”（如“inode X 的 i_size 增加了 Y”）。这极大地减少了日志写入量，显著降低了提交延迟。

所以 jdb 的运行过程为:
我们可以更精确地阐述这个过程：

- 元数据更新由 ext4 发起，但由 jbd2 管理：当 ext4 需要修改一个 inode（例如，更改文件大小、时间戳）、更新一个块位图（标记一个块被分配或释放）、或者修改一个目录项时，它并不是直接将这些修改写入它们在磁盘上的“最终位置”（home location）。相反，ext4 会通过 jbd2 的 API（如 jbd2_journal_start(), jdb2_journal_get_write_access(), journal_dirty_metadata() 等）将这些“即将发生的修改”交给 jbd2。
- jbd2 负责“先写日志”（Write-Ahead Logging）：jbd2 会将这些待修改的元数据块（或描述其变更的信息）作为一个“事务”（Transaction）的一部分，先写入到一个连续的、专用的日志（journal）区域。这个过程是原子的：要么整个事务的所有块都成功写入日志，要么都不写。
- jbd2 负责“提交”（Commit）：只有当日志中的这个事务被成功“提交”（写入一个 commit block）后，jbd2 才会在后台异步地将这些修改应用到它们原本在文件系统中的位置。
- 崩溃恢复由 jbd2 执行：如果系统在“写日志”和“写回原位置”之间崩溃，那么在下次挂载时，ext4 会调用 jbd2 的恢复代码。jbd2 会扫描日志，找到所有已提交但尚未写回原位置的事务，并将它们“重放”（replay），从而将文件系统恢复到崩溃前的一致状态。那些未提交的事务会被直接丢弃。

```txt
@[
    jbd2_journal_dirty_metadata+0
    ext4_mark_iloc_dirty+336
    __ext4_mark_inode_dirty+212
    ext4_dirty_inode+108
    __mark_inode_dirty+160
    generic_update_time+88
    file_modified+188
    ext4_buffered_write_iter+96
    ext4_file_write_iter+156
    vfs_write+548
    __arm64_sys_pwrite64+176
    invoke_syscall.constprop.0+88
    do_el0_svc+72
    el0_svc+92
    el0t_64_sync_handler+268
    el0t_64_sync+408
]: 62

@[
    jbd2_journal_get_write_access+0
    ext4_reserve_inode_write+180
    __ext4_mark_inode_dirty+172
    ext4_da_write_end+932
    generic_perform_write+316
    ext4_buffered_write_iter+116
    ext4_file_write_iter+156
    vfs_write+548
    __arm64_sys_pwrite64+176
    invoke_syscall.constprop.0+88
    do_el0_svc+72
    el0_svc+92
    el0t_64_sync_handler+268
    el0t_64_sync+408
]: 390
```

当完成提交之后
```txt
@[
    jbd2_journal_stop+0
    ext4_da_write_end+948
    generic_perform_write+316
    ext4_buffered_write_iter+116
    ext4_file_write_iter+156
    vfs_write+548
    __arm64_sys_pwrite64+176
    invoke_syscall.constprop.0+88
    do_el0_svc+72
    el0_svc+92
    el0t_64_sync_handler+268
    el0t_64_sync+408
]: 809
```

真正的提交在另外一个 thread 中:
```txt
@[
    jbd2_journal_commit_transaction+0
    kthread+340
    ret_from_fork+16
]: 11
```
问题:
- 如果 jbd2 的写的时候，出现了问题，该如何办?
- 哪些操作是需要 jbd2 的支持的?

## TODO
- https://ext4.wiki.kernel.org/index.php/Main_Page
- https://man7.org/linux/man-pages/man5/ext4.5.html
- https://metebalci.com/blog/a-minimum-complete-tutorial-of-linux-ext4-file-system/
- https://zorrozou.github.io/docs/ext4/ext4.html
- https://zhuanlan.zhihu.com/p/21441932933 : 像是机器人的结果
- https://github.com/PKTH-Jx/another_ext4
- https://mp.weixin.qq.com/s/TbDV-Cd2Ohxqr2qY6Oct4w
- https://mp.weixin.qq.com/s/QgV2LCLEalLDzurU4rqKoA
- https://mp.weixin.qq.com/s/BBHRZysRzMSD9EM3mVXxKQ

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
