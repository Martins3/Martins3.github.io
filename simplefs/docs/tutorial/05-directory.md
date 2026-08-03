# 5. 目录：名字到 inode 号的映射

目录在 VFS 中是 inode，在磁盘上是特殊格式的文件。它不保存子文件内容，只保存
名字和 inode number。SimpleFS 故意使用线性目录格式，便于观察 lookup、readdir
和修改操作的完整语义。

## 两级目录存储

目录 inode 的 `ei_block` 指向 `simplefs_file_ei_block`。这个索引块保存
`nr_files` 和目录 extent；每个 extent 固定覆盖 8 个目录 data block。data block
中的 `simplefs_file` 只有 inode number 和 255 字节名字。

```text
directory inode
  -> directory extent index
       -> extent 0 -> 8 directory blocks -> fixed-size dirents
       -> extent 1 -> 8 directory blocks -> fixed-size dirents
```

删除条目时把 inode number 清零形成 hole，创建会优先复用 hole。格式没有 hash 或
B-tree，因此 lookup 是 O(n)。这对教学很直观，但大目录性能不是设计目标。

## readdir 与 ctx->pos

`getdents64()` 最终调用 `simplefs_dir_ops.iterate_shared`。`simplefs_iterate()` 先用
`dir_emit_dots()` 产生 `.`、`..`，再把 `ctx->pos - 2` 换算成 extent/block/slot，
从该位置继续扫描。

`ctx->pos` 是可恢复游标，不只是本次循环下标。用户缓冲区满时 `dir_emit()` 返回
false，函数必须保留准确位置，使下一次 readdir 不重复也不漏项。目录洞仍会推进
pos，因为 position 对应磁盘 slot，而不是“第几个有效文件”。

## create 与 mkdir

创建首先扫描已有 data block 的空 slot；没有空位时一次分配 8-block 目录 extent。
然后写 `{inode, name}`，增加 `nr_files`，更新父目录时间戳和 inode。mkdir 还要：

- 初始化新目录自己的索引块；
- 保存父 inode number，供 exportfs `get_parent`；
- 增加父目录 nlink。

这些对象应属于同一 transaction，否则崩溃可能留下“目录项存在但 inode free”或
“inode 已占用但没有任何名字”的半状态。

## unlink 与 rmdir

unlink 清目录项、减少文件 nlink，并更新目录元数据。rmdir 在此基础上先确认目标
目录没有有效子项，再处理父/子目录的 nlink。不能用 `nr_files == 0` 作为唯一证据
而跳过格式校验；损坏的计数与真实 dirent 不一致时，应返回错误而非错误删除。

打开后 unlink 的文件仍可通过 fd 访问，说明 VFS 对象生命周期与目录名字生命周期
分离。目录项消失只阻止新的 pathname lookup。

## rename 为什么最难

rename 可能同时修改旧父目录、新父目录、源 inode、目标 inode 和多个 nlink；还要
支持覆盖、跨目录、目录祖先检查、`RENAME_EXCHANGE`、`RENAME_WHITEOUT` 等 flags。
VFS 提供目录锁序，但文件系统仍要保证自己的 transaction 和错误回滚完整。

判断 rename 的方法不是只看最终名字，而是枚举必须一直成立的不变量：

- 一个普通 rename 完成后源名字消失、目标名字出现；
- 覆盖目标时目标 inode 的 nlink 正确下降；
- 目录不能被移动到自己的后代中；
- 跨父目录移动子目录时两边 nlink 和保存的 parent ino 一起更新；
- 失败时旧命名空间仍完整，不能只完成一半。

## 名字不是 C 字符串

VFS 提供 `struct qstr { name, len, hash }`。合法的 255 字节名字没有空间存 NUL，
所以写盘必须 `memcpy(..., qstr.len)`，读盘必须有界。比较时先比较长度，再比较字节；
无界 `strlen`、`strcpy` 或强制覆盖第 255 字节都会损坏合法名字。

## 对照源码

- `Documentation/filesystems/directory-locking.rst`：VFS 目录锁规则；
- `fs/libfs.c`：简单目录 helper；
- `fs/ext2/dir.c`、`fs/ext2/namei.c`：变长 dirent 与目录修改；
- `fs/xfs/xfs_dir2*`：可扩展目录格式的另一端。

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
