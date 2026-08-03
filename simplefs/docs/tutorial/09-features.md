# 9. xattr、ACL、链接、exportfs 与边界功能

完成 read/write/create 只是文件系统主线的开始。很多“附加功能”实际会穿过 inode
生命周期、权限、安全、文件句柄和用户态 ABI，因此很适合检验对 VFS 的理解。

## Xattr

xattr 是 `{namespace.name -> byte value}`。SimpleFS 每个 inode 最多分配一个 4 KiB
xattr block，entry 从前向后、value 从后向前。`simplefs_xattr_get/set/listxattr()`
实现磁盘布局，xattr handler 决定 user/trusted/security namespace 的 VFS 接入。

修改 xattr 会分配或释放元数据 block，必须更新 inode 的 `i_xattr_block`、block
bitmap、journal 和 ctime。删除 inode 时也必须回收这个间接块。

## POSIX ACL

ACL 通过 `system.posix_acl_access/default` xattr 持久化。创建 inode 时
`simplefs_init_acl()` 继承父目录 default ACL，并可能调整 mode。它不是 create 完成后
可有可无的附加步骤：ACL 初始化失败时，新 inode 和目录项必须一起回滚。

单 xattr block 也决定 ACL 项数上限。xfstests capability helper 必须报告真实格式
上限，超限应返回明确错误或 NOTRUN，不能越界写 block。

## Symlink、hard link 与特殊文件

- hard link：新增名字指向已有 inode，增加 nlink，不复制数据；
- symlink：新 inode 保存路径字符串，短内容内联，长内容占 block；
- char/block/FIFO/socket inode：mode 决定 VFS 特殊初始化，设备号编码在 `i_data`。

VFS inode 内某些字段共用 union，类型转换或复用时必须从干净 inode 开始。磁盘 inode
槽可残留旧字节，free bitmap 本身不会替你清理 union。

## fileattr 与 ioctl

SimpleFS 支持 immutable、append、noatime、sync、dirsync、nodump 等用户可见 flags，
通过 `fileattr_get/set` 和磁盘 `i_flags` 持久化。只应接受明确支持且可执行语义的 flag；
把未知 DAX/compress/verity flag 写进磁盘再忽略，比返回 `EOPNOTSUPP` 更糟。

`simplefs_ioctl()` 还处理 fiemap、label、shutdown、FITRIM 等接口。ioctl 是用户 ABI，
参数长度、compat 入口、权限检查和错误码都属于语义，不是“能返回一个数字”即可。

## Fiemap、bmap 与 seek hole/data

三者都查询映射但接口不同：

- FIEMAP 返回 extent 列表及 unwritten/xattr 等 flags；
- FIBMAP 查询单个逻辑 block 的物理 block，通常要求特权；
- `SEEK_HOLE/SEEK_DATA` 返回下一个 hole/data offset。

它们必须和实际 read 语义一致：unwritten 物理上已分配，但对用户数据可见性通常按
hole/zero 处理。`tests/manual/fibmap.c` 可用于观察最小映射。

## Exportfs

NFS file handle 不能保存内核指针，只能保存可跨时间重建的 `{ino, generation}`。
SimpleFS 提供 fh_to_dentry、fh_to_parent、get_parent、get_name。目录 parent inode number
持久化在 `i_data`，lookup 使用 `d_splice_alias()` 处理 disconnected directory alias。

exportfs 把 dentry/inode identity 的边界放大：只实现“能 iget”仍不够，还要防 inode
复用、重连目录树并反查名字。

## Statfs、freeze、discard 与 shutdown

- statfs：报告总/空闲 data block 和 inode，不混淆格式单位与已用量；
- freeze：先同步，再阻止新 writer，给快照建立静止边界；
- discard/FITRIM：只对真正 free 且已达到持久化边界的范围下发；
- shutdown：严重 I/O error 后拒绝继续写，避免错误扩大。

## 明确不支持也是正确实现

SimpleFS 不实现 reflink、quota、encryption、verity、DAX、casefold、atomic write 等大量
功能。对应 xfstests 可以 NOTRUN，但原因必须来自真实 capability/格式限制。伪造
PASS、把执行失败改成跳过、或返回错误数据都不可接受。

## 对照源码

- `fs/xattr.c`, `fs/posix_acl.c`, `fs/exportfs/`：VFS 通用层；
- `fs/ext2/xattr.c`, `fs/ext2/acl.c`：简单磁盘 xattr/ACL；
- `fs/xfs/xfs_ioctl.c`：成熟 ioctl、fiemap、freeze/shutdown 接口。

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
