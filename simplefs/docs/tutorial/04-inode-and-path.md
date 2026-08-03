# 4. 路径查找、dentry 与 inode

磁盘目录只保存“名字 → inode 号”，VFS pathname walk 操作的却是 dentry 和 inode。
文件系统的职责是在 cache miss 时把两套世界接起来，并在创建、删除和重命名时让
内存命名空间与磁盘命名空间一起变化。

## dentry 不是 inode

`struct dentry` 表示父目录中的一个名字。它可以是 positive dentry（指向 inode），
也可以是 negative dentry（确认该名字当前不存在）。`struct inode` 表示对象身份和
属性，不保存“我在所有父目录中的名字”。硬链接正是多个 dentry 指向同一 inode。

路径还包含 mount：`struct path = { vfsmount, dentry }`。同一个 dentry tree 可以在
不同挂载点出现，因此只拿 inode 或 dentry 都不能完整表示用户路径。

## lookup

pathname walk 先查 dcache；未命中时调用父目录 inode 的 `.lookup`：

1. `simplefs_lookup()` 检查名字长度；
2. 读取父目录 `ei_block` 指向的目录 extent 索引；
3. 顺序扫描目录 data block，按长度比较名字；
4. 找到 inode 号后调用 `simplefs_iget()`；
5. 使用 `d_splice_alias()` 把 inode 与 dentry 连接；
6. 未找到则用 NULL inode 实例化 negative dentry。

`d_splice_alias()` 而不是简单 `d_add()`，是因为目录可能已有 exportfs 解码产生的
disconnected alias，需要把它重新接回树中。

## iget 与 inode cache

`simplefs_iget()` 先调用 `iget_locked(sb, ino)`。如果 inode 已在 cache 中，直接返回
同一对象；只有带 `I_NEW` 的首次实例才读取 inode table。读盘后按 mode 安装不同的
操作表：目录使用 `simplefs_dir_ops`，普通文件使用 iomap fops/aops，symlink 与
特殊设备有各自初始化。

这是重要的身份不变量：同一 superblock、同一 inode number 在内存中应只有一个
权威 inode。绕过 iget cache 会破坏锁、页缓存和引用计数语义。

## 新 inode 为什么不走“读取旧槽”

free bitmap 表示 inode 槽可复用，不保证 inode table 中的旧字节已经清零。新建路径
用 `new_inode()` 获取干净的 VFS inode，再用 `insert_inode_locked()` 建立新身份，
并显式初始化本次的 mode、generation、`ei_block`、xattr 和私有字段。

如果先把旧 symlink inode 读进 VFS 再改成字符设备，VFS inode 内部复用的 union
可能残留 `i_link`，之后被当成 `i_cdev` 使用。这不是磁盘内容错误，而是把“加载
已有对象”和“创建新对象”两个生命周期混用。

## create 的多对象更新

`simplefs_create()` 开启 transaction，`simplefs_create_internal()` 修改至少：

- inode free bitmap；
- 新 inode table 槽；
- 新 inode 的 extent root 或目录索引块；
- 父目录 data block；
- 父目录 extent index 和计数；
- 父目录 inode 时间戳；
- 可能还有继承的 ACL/security xattr。

成功后 `d_instantiate()` 让 dentry 立即指向新 inode。任何中途失败都必须撤销本次
分配，且不能释放仍被旧磁盘指针引用的块。

## unlink、link 与 eviction

unlink 删除的是目录项并递减 nlink，不等于立刻销毁 inode。只要进程仍打开文件，
`struct file` 的引用会让 inode 和数据继续存在。VFS 最终回收 nlink 为 0 的 inode
时，`simplefs_evict_inode()` 才释放 extent tree、数据块、xattr、目录块和 inode 位。

hard link 只增加同一 inode 的 nlink 并新增目录项；symlink 则创建新的 inode，内容
短时内联、长时使用数据块。把这三种操作都理解成“复制文件”会完全看错生命周期。

## generation 与 stale handle

inode number 会复用，NFS file handle 因此还保存 generation。`simplefs_nfs_get_inode()`
同时检查 inode 位图和 generation，不匹配返回 `ESTALE`。inode number 表示槽，
generation 才区分这个槽的不同时代。

## 对照源码

- `fs/namei.c`：RCU-walk、REF-walk、lookup/open/create 主流程；
- `fs/inode.c`、`fs/dcache.c`：inode/dentry cache 生命周期；
- `fs/ext2/namei.c`、`fs/ext2/inode.c`：传统实现的清晰对照。

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
