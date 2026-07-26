# ext4 中的 iomap 使用
(gpt 5.5 high 分析结果)

基于当前源码 v7.0.1-1-g8fb1782f0da0，ext4 并没有整体迁移到 iomap。基本边界是：

- Direct I/O、DAX、FIEMAP/SEEK_HOLE 等查询接口走 iomap。
- Buffered write、writeback、绝大多数元数据操作仍基于 buffer_head。
- Buffered read 的主路径直接构造 bio，通常既不是 iomap，也不依赖 buffer_head；复杂情况才回退到 buffer_head。

### 普通文件数据路径

 操作                            实现
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 buffered read                   ext4 自己的 mpage/bio 路径
──────────────────────────────  ───────────────────────────────────
 buffered read 复杂映射回退      buffer_head
──────────────────────────────  ───────────────────────────────────
 buffered write                  buffer_head
──────────────────────────────  ───────────────────────────────────
 delayed-allocation writeback    buffer_head 状态 + ext4 mpage/bio
──────────────────────────────  ───────────────────────────────────
 direct read/write               iomap
──────────────────────────────  ───────────────────────────────────
 DAX read/write/fault            iomap
──────────────────────────────  ───────────────────────────────────
 data=journal                    buffer_head + JBD2

#### Buffered read

入口：

- fs/ext4/readpage.c:395
- fs/ext4/readpage.c:416
- ext4_mpage_readpages()

主路径调用 ext4_map_blocks() 得到映射，然后直接：

ext4_mpage_readpages()
  -> ext4_map_blocks()
  -> bio_alloc()
  -> bio_add_folio()
  -> blk_crypto_submit_bio()

因此它不是 iomap，也不能简单称为 buffer_head 路径。

遇到 folio 已经挂有 buffers、hole 后又出现 mapped block、物理块不连续等复杂情况，会走：

confused:
  -> block_read_full_folio(folio, ext4_get_block)

这才是明确的 buffer_head fallback，见 fs/ext4/readpage.c:378。

#### Buffered write

入口由 aops 决定：

- 普通非 delalloc：ext4_write_begin()
- delalloc：ext4_da_write_begin()
- data=journal：ext4_write_begin() + journalled write_end

aops 定义见 fs/ext4/inode.c:3934。

典型调用链：

generic_perform_write()
  -> ext4_write_begin() / ext4_da_write_begin()
  -> create_empty_buffers()
  -> ext4_block_write_begin()
  -> ext4_get_block() / ext4_da_get_block_prep()
  -> block_write_end()

这是 buffer_head 路径。关键证据是：

- create_empty_buffers()
- folio_buffers()
- ext4_block_write_begin()
- block_write_end()

见 fs/ext4/inode.c:1283 和 fs/ext4/inode.c:3115。

#### Buffered writeback

入口是 fs/ext4/inode.c:3012：

ext4_writepages()
  -> ext4_do_writepages()
  -> mpage_prepare_extent_to_map()
  -> mpage_add_bh_to_extent()
  -> mpage_map_one_extent()
  -> ext4_map_blocks()
  -> mpage_submit_folio()
  -> bio submission

它利用每个 folio 上的 buffer_head 保存：

- BH_Dirty
- BH_Delay
- BH_Unwritten
- BH_Mapped
- BH_New

但最终数据 I/O 是聚合成 bio 提交的。因此这是“buffer_head 管理块状态和映射，bio 执行 I/O”，不是 iomap writeback。

### iomap 路径

ext4 的主要 iomap 映射实现位于：

- fs/ext4/inode.c:3758
- fs/ext4/inode.c:3837
- fs/ext4/inode.c:3841
- fs/ext4/inode.c:3892

它们底层仍调用 ext4 自己的 ext4_map_blocks()，只是将结果转换成：

- IOMAP_MAPPED
- IOMAP_UNWRITTEN
- IOMAP_DELALLOC
- IOMAP_HOLE

#### Direct I/O

ext4_file_read_iter()
  -> ext4_dio_read_iter()
  -> iomap_dio_rw()
  -> ext4_iomap_ops

见 fs/ext4/file.c:70。

写路径：

ext4_file_write_iter()
  -> ext4_dio_write_iter()
  -> iomap_dio_rw()
  -> ext4_iomap_ops
  -> ext4_iomap_alloc()
  -> ext4_map_blocks()

见 fs/ext4/file.c:503。

不支持 DIO 的情况可能回退到 buffered write，届时重新进入 buffer_head 路径。

#### DAX

以下全部使用 iomap：

- dax_iomap_rw()：DAX read/write
- dax_iomap_fault()：DAX mmap fault
- dax_zero_range()：DAX zero range

见 fs/ext4/file.c:103 和 fs/ext4/file.c:723。

#### 查询类接口

这些虽然不一定产生数据 I/O，但使用 iomap 描述文件映射：

- iomap_fiemap()：FIEMAP
- iomap_seek_hole() / iomap_seek_data()
- iomap_bmap()：FIBMAP/bmap
- iomap_swapfile_activate()：swapfile 激活

### 元数据路径

绝大多数 ext4 元数据仍然是 buffer_head：

- 超级块、group descriptor：super.c
- block/inode bitmap：balloc.c、ialloc.c、mballoc.c
- inode table：ext4_get_inode_loc()
- 目录块、htree：namei.c、dir.c
- xattr block：xattr.c
- extent tree 节点：extents.c
- indirect block tree：indirect.c
- journal 元数据：ext4_jbd2.c、JBD2
- resize、MMP、orphan、fast commit block

需要特别注意：extents.c 包含 iomap FIEMAP 接口，不代表 extent tree 本身由 iomap 管理。extent tree 节点的读取、校验和 journal 修改仍然使用 buffer_head。

一句话总结：

DIO / DAX / mapping-reporting  -> iomap
buffered write / writeback     -> buffer_head + bio
buffered read                  -> ext4 mpage + bio，复杂情况回退 buffer_head
filesystem metadata            -> buffer_head + JBD2

## 为什么 buffered io 相比而言难以支持

核心原因是状态生命周期不同：

> Direct I/O 的映射状态只需维持一次系统调用；buffered I/O 的状态要跨越 write、page cache 驻留、writeback、truncate 和 journal transaction。

### Direct I/O 为什么容易

DIO 不进入 page cache，调用链比较封闭：

write()
  -> ext4_dio_write_iter()
  -> iomap_dio_rw()
  -> ext4_iomap_begin()
  -> ext4_map_blocks()
  -> submit bio
  -> end_io：转换 unwritten extent

iomap 每次只需要回答：

文件偏移 -> hole / mapped / unwritten -> 物理地址和长度

这个 struct iomap 用完就丢弃。需要处理的状态较少：

- 当前 I/O 范围映射到哪里；
- 是否需要分配块；
- 是否是 unwritten extent；
- I/O 完成后是否转换为 written；
- 是否更新 i_size、orphan list。

映射、I/O 提交和完成处于一个相对完整的操作周期内，并且通常由 i_rwsem 等锁保护。

### Buffered I/O 的困难

Buffered write 被拆成两个相隔很久的阶段：

用户 write
  -> 数据复制进 page cache
  -> 标记 dirty
  -> write() 返回

可能很久以后：

writeback
  -> 找 dirty folio
  -> 分配 delayed-allocation block
  -> 建立映射
  -> 提交 bio
  -> 完成 unwritten extent 转换

中间还可能发生：

- 多次写入同一个 folio；
- mmap 修改；
- truncate、hole punch；
- reclaim、folio migration；
- fsync；
- journal commit；
- direct I/O 和 buffered I/O 并发；
- writeback 只写 folio 的一部分。

因此文件系统必须把块级状态长期附着在 page cache 上。

### buffer_head 在 ext4 中不只是 I/O 描述符

ext4 用每个 buffer_head 的 flag 保存文件系统块状态：

BH_Dirty
BH_Mapped
BH_New
BH_Delay
BH_Unwritten
BH_Uptodate
BH_JBD / BH_JWrite

例如 delayed allocation 的 buffer：

BH_Mapped | BH_New | BH_Delay
b_blocknr = -1

unwritten buffer：

BH_Mapped | BH_New | BH_Unwritten
b_blocknr = physical block

见 fs/ext4/inode.c:2010。

writeback 会逐个检查这些 buffer 的状态，将状态相同、逻辑连续的 buffer 合并：

mpage_add_bh_to_extent()
  -> 比较 bh->b_state
  -> 聚合 delayed/unwritten extent
  -> ext4_map_blocks()
  -> mpage_submit_folio()

见 fs/ext4/inode.c:2118。

换成 iomap 不是把 ext4_get_block() 换成 ext4_iomap_begin() 就够了，而是必须重建整套长期状态管理。

### folio 与文件系统块大小不一定相等

一个 64 KiB folio 可能包含多个 4 KiB ext4 block：

folio
  block 0: mapped + dirty
  block 1: delayed + dirty
  block 2: hole
  block 3: unwritten + uptodate

folio 自身的 dirty、uptodate 只有一个整体 bit，表达不了这些块级差异。

buffer_head 天然提供每个 filesystem block 的状态。iomap buffered I/O 则需要使用自己的 per-folio 状态/bitmap，例如 iomap_folio_state，并重写 ext4 所有依
赖 BH_* 的判断。

### ext4 有三种 data mode

迁移最困难的是 ext4 特有的 journal 语义：

- data=writeback
- data=ordered
- data=journal

尤其 data=journal 会把文件数据的 buffer_head 直接挂到 JBD2 transaction：

folio
  -> buffer_head
  -> journal_head
  -> jbd2 transaction

buffer 的 dirty 状态和 journal dirty 状态还有严格区别。源码明确说 buffer dirty 状态是“definitive”，不能只把 folio 标脏，见 fs/ext4/inode.c:3914。

要改成 iomap，必须同时解决：

- 数据如何加入 JBD2 transaction；
- transaction commit 时如何写保护 mmap；
- partial-folio journal 状态；
- revoke、invalidate、truncate；
- journal dirty 与普通 writeback dirty 的转换。

这已经不是单纯的 I/O API 替换。

### mmap 进一步拉长状态生命周期

buffered mmap 写路径可能是：

page fault
  -> page_mkwrite
  -> 分配或预留块
  -> 建立 journal/ordered-data 关系
  -> 用户在任意时刻修改页面
  -> 后续 writeback

文件系统无法像 DIO 那样，在一次调用内确定“这些数据已经写完”。页面还可能被长期映射、重复修改或 DMA pin。

### 为什么 buffered read 相对容易

读取没有 delayed allocation、dirty tracking 和 journal data，因此 ext4 buffered read 已经基本绕开 buffer_head：

ext4_mpage_readpages()
  -> ext4_map_blocks()
  -> bio_add_folio()
  -> submit bio

复杂映射才回退到：

block_read_full_folio(..., ext4_get_block)

因此真正难迁移的主要是 buffered write/writeback，不是 read。

### 本质区别

 Direct I/O                     Buffered I/O
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 不进入 page cache              数据长期驻留 page cache
─────────────────────────────  ─────────────────────────────────────
 mapping 是临时对象             mapping 状态必须长期保存
─────────────────────────────  ─────────────────────────────────────
 一次调用内提交 I/O             write 和 writeback 分离
─────────────────────────────  ─────────────────────────────────────
 通常整段 extent 操作           需要 filesystem-block 粒度状态
─────────────────────────────  ─────────────────────────────────────
 完成回调转换 unwritten         conversion 可能跨 writeback/journal
─────────────────────────────  ─────────────────────────────────────
 几乎不接触 JBD2 data buffer    data=journal 深度绑定 buffer_head

所以不是 iomap 做不了 buffered I/O，而是 ext4 的 delayed allocation、writeback 和 JBD2 语义已经围绕 buffer_head 构建。迁移需要同时替换“块状态存储模
型”和“journal 集成模型”，工程量和回归风险都远大于 DIO。


## martins3

1. 元数据为什么特殊考虑
	- 为什么 iomap 是如何解决的?

经典调用
```txt
🤒  sudo cat /proc/450025/stack
[<0>] ext4_read_bh+0x80/0x90 [ext4]
[<0>] ext4_read_inode_bitmap+0x38a/0x530 [ext4]
[<0>] __ext4_new_inode+0x3a7/0x16f0 [ext4]
[<0>] ext4_create+0x129/0x210 [ext4]
[<0>] path_openat+0xea6/0x1160
[<0>] do_filp_open+0xb3/0x160
[<0>] do_sys_openat2+0xab/0xe0
[<0>] __x64_sys_openat+0x6e/0xa0
[<0>] do_syscall_64+0x3b/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x6e/0xd8
```

cat /mnt/a 会得到如下的等待:
```txt
🤒  sudo cat /proc/454840/stack
[<0>] folio_wait_bit_common+0x13d/0x350
[<0>] filemap_get_pages+0x5ff/0x630
[<0>] filemap_read+0xd9/0x350
[<0>] vfs_read+0x1fe/0x350
[<0>] ksys_read+0x6f/0xf0
[<0>] do_syscall_64+0x3b/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x6e/0xd8
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
