# 用 SimpleFS 学 Linux 文件系统

这套教程采用同一条主线：先看用户可见语义，再找 VFS 回调，最后落到磁盘对象
和持久化顺序。每章都以当前 SimpleFS 为具体例子，并给出在
`/home/martins3/data/kernel/linux-drm` 中继续比较的入口。

## 阅读顺序

1. [总览：四层模型与源码地图](01-overview.md)
2. [磁盘格式：哪些事实必须持久化](02-disk-format.md)
3. [mkfs、注册与挂载](03-mount.md)
4. [路径查找、dentry 与 inode](04-inode-and-path.md)
5. [目录：名字到 inode 号的映射](05-directory.md)
6. [普通文件 I/O：folio、page cache 与 iomap](06-file-io.md)
7. [extent 与空间所有权](07-extents-and-space.md)
8. [一致性：事务、JBD2 与恢复](08-consistency.md)
9. [xattr、ACL、链接、exportfs 与边界功能](09-features.md)
10. [测试与调试：如何证明“没有错误”](10-testing-and-debugging.md)

## 两条可选路线

只想理解 VFS 调用关系：读 1、3、4、5、6。重点追踪 `file_system_type`、
`super_operations`、`inode_operations`、`file_operations` 和
`address_space_operations` 五组操作表。

想理解磁盘正确性：读 2、6、7、8、10。重点追踪每个物理块的所有者、缓存身份、
dirty 到 durable 的路径，以及失败发生在更新一半时如何回滚或恢复。

## 阅读源码的方法

不要从 2,000 行函数中间开始。每次选择一个用户动作，例如 `touch a`：

1. 写出用户期望：成功后目录中出现名字，inode 已分配，重挂载后仍存在；
2. 从操作表找到入口：这里是 `simplefs_inode_ops.create`；
3. 列出被修改对象：父目录项、父 inode、新 inode、inode 位图、块位图；
4. 标出锁、transaction 和错误出口；
5. 用一个手工实验或定向 xfstests 验证，而不是只凭代码看起来合理。

每章末尾的“对照源码”用于扩展视野：ext2 适合看最直接的 VFS 映射，ext4 适合看
JBD2 和传统 buffer_head 数据路径，XFS 适合看成熟的 iomap、extent 与事务设计。

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
