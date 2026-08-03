# 6. 普通文件 I/O：folio、page cache 与 iomap

普通文件读写有两个互相独立的问题：数据暂时放在哪里，以及文件 offset 最终映射到
哪个物理 block。page cache/folio 解决前者，iomap 回调和 extent tree 解决后者。

## 两个 page-cache 域

SimpleFS 同时使用：

- `inode->i_mapping`：某个普通文件的数据 folio；
- `sb->s_bdev->bd_mapping`：superblock、inode table、bitmap、目录、extent、xattr
  等元数据 block 的 folio。

同一物理 block 被释放后可能从“旧元数据”变成“新文件数据”。如果 bdev mapping
仍缓存旧 clean folio，即使磁盘已写入新数据，后续元数据读取仍可能命中旧缓存。
因此物理块复用必须考虑缓存身份，不能只改 bitmap。

## iomap 的问题模型

iomap 请求文件系统回答一个范围：

```text
logical file offset -> hole / unwritten / mapped -> physical address + length
```

SimpleFS 的 read/write iomap begin 从 extent tree 查找或分配映射，iomap 通用代码
负责 page cache 复制、bio 组织、DIO 和部分块处理。文件系统仍负责空间所有权、
extent 状态、i_size 和完成时转换。

## Buffered read

`simplefs_file_read_iter()` 对普通读取调用 `filemap_read()`。cache hit 直接复制数据；
cache miss 通过 `simplefs_read_folio()`/`simplefs_readahead()` 进入 iomap read path。
hole 和 unwritten 都向用户返回零，mapped 才从物理块读取。

readahead 是预测性填充 page cache，不改变 read 的语义。它可以比当前请求多读，但
不能越过映射边界把 hole 当数据。

## Buffered write 与 writeback 分离

用户 write 通常只把数据复制进 page cache，标记 folio dirty 后就返回。真正 bio
可能由后台回写或 fsync 很久以后提交：

```text
write_iter
  -> inode lock + generic_write_checks
  -> iomap_file_buffered_write
  -> allocate/return unwritten mapping
  -> dirty page-cache folio
  -> return to user

writeback/fsync
  -> simplefs_writepages
  -> iomap_writepages
  -> bio completion
  -> worker converts only successful range to written
```

新分配 extent 先标为 unwritten。这样在数据 I/O 成功前，即使元数据已能找到物理块，
读取仍返回零，不会暴露旧盘内容。转换不能在 iomap_begin 提前做，也不能把相邻但
未实际写到的范围一起转换。

## Address-space operations

`simplefs_iomap_aops` 注册 read_folio、readahead、writepages、dirty/release/
invalidate/migrate folio 和 bmap。它不使用 buffer_head。folio 状态告诉 VM 哪些缓存
需要写回；iomap folio state 在需要时表达更细的 block 状态。

folio 不是磁盘 block 的同义词：folio 大小可能大于 4 KiB，一个 folio 内可以包含
多个文件系统 block。写“整页 dirty”与写“哪些 block 有效”是不同粒度的问题。

## mmap write

mmap read fault 由 filemap 通用逻辑填页；第一次写保护 fault 进入
`simplefs_page_mkwrite()`，iomap 在用户真正能修改页面前建立安全映射。之后用户写
内存，dirty folio 仍由普通 writeback 落盘。

mmap 让用户 buffer 与目标文件 mapping 可能是同一个对象。若在持有 invalidate 写锁
时访问这个 buffer 并触发 fault，readahead 会反向申请同一 mapping 的锁，形成递归。
这也是“锁不能只看当前函数，必须看 fault/GUP 隐式调用”的典型例子。

## Direct I/O

DIO 绕过 page cache 直接提交 bio，但必须与已缓存数据保持一致：读前写回脏缓存，
写时协调 invalidate，完成后更新 unwritten extent 与 i_size。SimpleFS 还要处理
iomap 用 `-ENOTBLK` 请求 buffered fallback 的协议。

用户 iov 可能来自 mmap，GUP 也可能取得 `mmap_lock`。当前实现先在 invalidate 锁外
提取有界 BVEC，再在锁内提交同步 DIO，避免 invalidate lock、fault 和 GUP 的锁环。

## fsync 到底保证什么

`simplefs_file_fsync()` 先等待指定范围文件数据，再等待异步 unwritten 转换，随后把
inode/extent 元数据加入 JBD2 并等待 commit。顺序必须是“数据稳定 → 发布指向数据的
元数据 transaction”，否则恢复后可能看到已经 written 的 extent，但数据仍未落盘。

## 对照源码

- `mm/filemap.c`：page cache 通用读写与 fault；
- `fs/iomap/`：buffered I/O、DIO、writeback 和 seek；
- `fs/xfs/xfs_aops.c`, `fs/xfs/xfs_file.c`：folio/iomap 成熟用法；
- `fs/ext4/inode.c`：buffer_head/delalloc/JBD2 路径为何更复杂。

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
