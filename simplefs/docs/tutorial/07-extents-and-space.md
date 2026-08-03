# 7. extent 与空间所有权

extent 把一段连续逻辑 block 映射到连续物理 block：

```text
{ ee_block = 8, ee_len = 4, ee_start = 100 }

logical 8  9  10 11
         |  |  |  |
physical 100 101 102 103
```

SimpleFS 把 unwritten 状态编码在 `ee_len` 高位，低位才是长度。所有读取长度的代码
都应使用 `simplefs_ext_len()`，不能直接把原始 `ee_len` 当数量。

## 三种映射状态

- hole：没有物理块，读零，写时分配；
- unwritten：已有物理块，读零，数据 I/O 成功后可转 written；
- written/mapped：物理块中已有用户可见数据。

hole 和 unwritten 的读结果相同，但所有权不同。punch hole 可以释放物理块；
ZERO_RANGE 常把范围保留为 unwritten；预分配要占空间但可能不增加可见文件内容。

## 查找、切分、合并

`simplefs_file_load_extents()` 把 root/leaf 读成内存 buffer，修改逻辑在内存数组上
进行，`simplefs_file_sync_extents()` 再写回两级树。相邻 extent 只有在逻辑连续、
物理连续且 written 状态相同时才能合并。

合并后的磁盘 extent 长度不能反过来扩大本次 iomap 的语义。例如本次只新分配 1 块，
恰好和后面 6 个旧块合并，返回的 `IOMAP_F_NEW` 仍只能覆盖 1 块。否则 iomap 可能
合理地清零“新块尾部”，却把 6 个旧块的数据一起破坏。

## 两级树的发布顺序

更新 extent tree 不是把 root 和 leaf 随便写出：

1. 在内存构造完整新集合；
2. 分配并写好新 leaf；
3. 写 root，让持久指针指向新 leaf；
4. root 已不引用旧 leaf/数据后，才释放旧块。

如果先 free 旧数据，再因 ENOSPC 无法分配新 leaf，旧磁盘 root 仍引用已被其他 inode
复用的块，形成双重所有权。错误时安全泄漏比一块同时属于两个文件更可接受。

## 第一次写入为何先分配 leaf

空文件第一次写连续数据时，extent tree 还没有 leaf。如果先分配第一个数据 block，
再分配 leaf，再分配剩余数据，用户可见的连续写会变成 `1 + N` 两段。当前实现一次
预留 `[leaf][data...]`，首块归元数据，其后连续块归用户数据。

这说明内部元数据分配顺序会影响 fiemap/filefrag 的用户可见布局。

## Bitmap、计数与 reservation

block bitmap 是最终所有者记录，`nr_free_blocks` 是缓存计数。分配和释放在
`bitmap_lock` 下同时更新。journal 尾区和固定元数据永远不能进入 free run。

写路径还可能维护内存 preallocation reservation。reservation 是“这个 inode 暂时
保留但尚未都使用的空间”，close/fsync/DIO 边界要决定保留或丢弃；它不能在 statfs
或崩溃恢复中变成无主块。

## 释放数据块与退役元数据块

普通文件数据块的缓存身份在 inode mapping，由 truncate/invalidate/iomap 生命周期
处理。元数据块在 bdev mapping，必须经过 `simplefs_retire_metadata_blocks()`：

1. 写回并等待目标 bdev folio；
2. 持有 invalidate lock 移除旧缓存；
3. 最后归还 block bitmap。

两类 block 使用不同释放 helper，是因为缓存域不同，不是因为“元数据比数据更重要”
这种模糊判断。

## fallocate 是 extent 语义测试场

SimpleFS 实现 punch hole、preallocate、collapse range、insert range、zero range。
它们同时考验 extent 切分/平移、i_size、页缓存失效、DIO 等待和失败回滚：

- punch：释放中间范围，边缘部分块要清零；
- collapse：删除区间并把后续逻辑 offset 左移；
- insert：插入 hole，把后续逻辑 offset 右移；
- zero range：用户读到零，但可保留物理空间；
- keep size：分配空间但不扩展可见 EOF。

## 对照源码

- `fs/ext4/extents.c`：通用 extent 操作和深树；
- `fs/xfs/libxfs/xfs_bmap.c`：fork、bmap btree 与 reservation；
- `fs/iomap/`：文件系统映射结果如何被 I/O 层消费。

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
