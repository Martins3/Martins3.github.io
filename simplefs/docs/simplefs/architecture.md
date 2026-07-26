# SimpleFS 架构与正确性不变量

## 分层

用户请求先经过 VFS。普通文件的数据 I/O 由 iomap 驱动，SimpleFS 提供逻辑块到物理块的映射、extent 更新和完成时状态转换。目录、inode、extent leaf、xattr 等元数据通过块设备 mapping 读取和写回。块位图是所有物理块最终的所有权来源。

这一设计有两个不同的 page cache 域：

- 文件数据在 `inode->i_mapping` 中；
- 文件系统元数据在 `sb->s_bdev->bd_mapping` 中。

它们可能指向同一个物理块，但缓存身份不同。释放和重新分配物理块时必须显式处理这一点。

## Extent 模型

每个 extent 记录逻辑起始块、长度、物理起始块，以及 written/unwritten 状态。查找结果只有三类：

- hole：没有物理块；
- unwritten：已经分配但读取必须返回零；
- mapped：已包含有效文件数据。

相邻且逻辑连续、物理连续、状态相同的 extent 可以合并。合并只是磁盘表示优化，不能扩大当前 I/O 的语义范围。例如一次新分配 1 块后可能与后面的 6 个旧块合并，但返回给 iomap 的 `IOMAP_F_NEW` 范围仍然只能是新分配的 1 块。

extent 数量超过根块容量时会使用 leaf 块。重建 extent 集合的操作必须转移或释放旧 leaf 的所有权；只复制 extent 内容而遗失 leaf 列表会造成块位图泄漏。

空文件第一次分配数据时已经需要一个 extent leaf。为了避免 leaf 插入第一个数据 run 中间，分配器把 leaf 和数据作为一个连续请求预留，布局为 `[leaf][data...]`。leaf 只有在 extent 同步成功后才算提交；失败回滚必须按元数据和数据各自的退役规则处理。这使第一个用户可见数据 run 保持物理连续。

## Buffered write 与 unwritten 转换

写入预分配空间时，`iomap_begin` 返回 `IOMAP_UNWRITTEN`，让 iomap 正确处理未覆盖的块头和块尾。extent 不能在提交数据 I/O 前就转换为 written，否则崩溃或 I/O 失败会暴露未初始化数据。

当前 buffered writeback 的顺序是：

1. 建立 unwritten iomap 并提交 bio；
2. bio 完成回调把完成工作放到 workqueue；
3. 仅在数据 I/O 成功后，把本次 I/O 范围转换为 written；
4. 调用 iomap 完成处理。

Direct I/O 同样只在成功完成且带有 unwritten 标志时转换。转换范围按实际 I/O offset 和 size 切分，不能把同一 extent 中未写到的头尾一起转换。

## EOF 与页缓存

文件扩展、收缩和区间移动必须同时维护三个对象：`i_size`、extent 映射、页缓存。

- truncate 扩展不会分配 hole，但旧 EOF 所在的部分块必须清零到新可见区间；
- truncate 收缩先等待脏页写回并锁住 mapping，再清理尾部、截断页缓存、最后释放 extent；
- collapse range 先写回并清除将移动范围的页缓存，再移动逻辑 extent；否则缓存页仍按旧逻辑偏移解释；
- buffered write 跨过 EOF 时，要清零旧 EOF 到实际写入起点之间位于已分配块中的间隙。

## 元数据块退役

元数据块不能直接清位图后立即复用。`simplefs_retire_metadata_blocks()` 执行：

1. 等待块设备 mapping 对应范围写回；
2. 持有 invalidate lock，移除对应 bdev folio；
3. 最后把块归还位图。

仅等待写回不够：clean folio 仍保存旧元数据镜像。若同一物理块先通过文件 mapping 写入数据，随后又被当作元数据读取，旧 bdev folio 可能覆盖磁盘上的新事实。

普通文件数据块不使用这个退役函数，因为其缓存和生命周期由文件 mapping、truncate 和 iomap 路径管理。

extent tree 的发布还要遵守指针依赖顺序：先写回所有新 leaf，再写回 root；只有 durable root 不再引用旧 leaf 或数据范围后，旧块才能进入 free bitmap。punch、truncate 和 collapse 因此先记录待释放范围，成功提交新 extent tree 后才统一释放。若在 ENOSPC 或 I/O 错误中无法提交，保留旧块所有权比提前复用安全。

## 保留区与空闲空间

块位图是空间所有权的权威来源，`nr_free_blocks` 是由它派生的缓存计数。superblock、inode table、位图自身以及可选 journal 尾区都必须在 mkfs 阶段标为已占用。挂载时会校验 journal 起点没有覆盖元数据且位于设备内，清除旧格式镜像可能遗留的 journal free bits，并用实际位图协调空闲计数。

`statfs` 中 `f_blocks`/`f_bfree` 描述数据块容量和空闲量，`f_files`/`f_ffree` 描述 inode 总容量和空闲量。不能把已用 inode 数填入 `f_files`，也不能让 journal 的计数扣除和位图标记表达两个不同的布局。

磁盘 inode 的 `i_blocks` 字段以 4 KiB SimpleFS 块计数，VFS `inode->i_blocks` 则按固定的 512 字节扇区计数。inode 读写边界负责双向转换；运行时每次分配或释放数据块、extent leaf、目录块、长 symlink 或 xattr 块，也都必须转换后再更新 VFS 计数。

目录的每个 extent 固定为 8 块，它与普通文件单 extent 的最大数据 run 是不同概念。目录条目上限由磁盘目录项大小、每块条目数、每个目录 extent 的块数和最大 extent 数共同推导；到达格式上限后直接返回 `EMLINK`。

## Inode 创建与复用

已存在 inode 由 `simplefs_iget()` 从 inode table 读取，并根据磁盘 mode 初始化 `i_op`、`i_fop`、symlink 和特殊设备状态。新 inode 不读取空闲槽的旧内容：它由 `new_inode()` 创建，用 `insert_inode_locked()` 按新 inode 号插入 hash，然后初始化本次所有的私有块指针。

这个区分也保护 VFS inode 内部的共用 union：`i_link`、`i_cdev`、`i_pipe` 不能从上一个磁盘文件类型泄漏到新文件。inode 在分配时生成非零 `i_generation`并持久化，exportfs 用它区分同一 inode 号的不同世代。

## Export 文件句柄

SimpleFS 持久化 inode generation，并在句柄解码时同时校验 inode 位图和 generation。目录的父 inode 号保存在 `i_data`，`get_name` 通过父目录项反查子 inode 名称。普通 lookup 使用 `d_splice_alias()`，以便 exportfs 能把 disconnected directory dentry 重新挂回目录树。

## 锁与更新边界

- inode 的 `extent_lock` 串行化 extent 查找、切分、合并和落盘；
- 改变逻辑文件布局的操作还需要 inode lock 和 page-cache invalidate lock；
- 位图释放必须发生在旧映射不再可能产生 I/O 之后；
- I/O 完成上下文不能直接执行可能睡眠的 extent 更新，所以新 iomap 接口通过 workqueue 完成 unwritten 转换。

普通 iomap buffered write 只持有 inode 写锁，不包围整个操作取得 invalidate 写锁。用户 iov 可能来自同一文件的 mmap；若在 invalidate 写锁内 fault-in，readahead 会递归申请同一 mapping 的读锁而死锁。invalidate 锁只用于 truncate、collapse、punch、DIO 页缓存协调等真正改变或清除 mapping 内容的边界。

DIO 在 invalidate 锁内也不能直接 pin 普通用户 iov，因为 GUP fallback 可能取得 `mmap_lock`。SimpleFS 先在锁外把用户页提取成有界 BVEC batch，锁内只提交 BVEC，强制同步完成后再 unpin。fallocate 和 truncate 在改变 extent 所有权前调用 `inode_dio_wait()`，避免迟到 bio 写入已经复用的块。

这些规则比某个具体函数的实现更重要。修改代码时应逐项验证所有权、缓存身份、I/O 完成时序和错误回滚。

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
