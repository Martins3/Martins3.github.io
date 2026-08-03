# 3. mkfs、注册与挂载

mkfs 创建的是“可被解释的磁盘事实”，mount 创建的是“可被 VFS 使用的内存实例”。
两者共享格式，却处在用户态和内核态两个完全不同的生命周期。

## mkfs 做什么

`user_mkfs.c` 负责参数、设备大小、已有签名检查和输出；`mkfs_common.c` 负责布局。
主流程是：

1. 读取设备或镜像大小并换算总 block；
2. 计算 inode store、两张 bitmap、数据区与 journal 尾区；
3. 写 superblock；
4. 清零 inode store，初始化 inode 1 作为根目录；
5. 写 inode/block free bitmap，并把所有固定元数据与 journal 标为占用；
6. 初始化根目录索引块和 JBD2 superblock；
7. 回读关键对象做校验。

`-f` 只用于确认覆盖已有签名。xfstests scratch 设备本来就要反复格式化，因此
runner 明确传 `-f`；日常 mkfs 默认拒绝覆盖是合理的安全边界。

## 模块注册

`simplefs_init()` 依次创建 inode slab、debugfs，再调用 `register_filesystem()`。
注册的 `simplefs_file_system_type` 声明 `FS_REQUIRES_DEV`，并通过
`init_fs_context` 接入新 mount API。

```text
mount(8)
  -> fsopen/fsconfig/fsmount 或 legacy mount(2)
  -> simplefs_init_fs_context
  -> simplefs_parse_param
  -> simplefs_get_tree
  -> get_tree_bdev
  -> simplefs_fill_super
```

当前参数有 `nojournal`、`norecovery` 和 `discard`。`norecovery` 必须只读挂载，因为
跳过旧日志回放后继续写，会以可能过时的元数据作为新事实。

## fill_super 的顺序

`simplefs_fill_super()` 的顺序本身就是一致性设计：

1. 设置 VFS block size、maxbytes、时间范围和操作表；
2. 通过 `simplefs_get_folio()` 从 block 0 读取并校验 magic；
3. 分配内存 `sbi`，复制磁盘字段，再初始化锁和运行时指针；
4. 将 inode/block bitmap 读入内存；
5. 校验并从 free bitmap 排除 journal 尾区；
6. 应用 mount 参数，加载 JBD2 并按需 recovery；
7. recovery 可能改写 bitmap，因此重新从磁盘加载两张位图；
8. 由位图重新计算空闲计数；
9. 最后 `simplefs_iget(sb, 1)` 创建根 inode 和 root dentry。

根 inode 必须在 recovery 后读取，否则内存可能缓存日志回放前的旧根目录。这个例子
说明 mount 不是“读 superblock 就结束”，而是建立一组彼此一致的缓存快照。

## 内存 superblock 与磁盘 superblock

VFS `struct super_block` 保存操作表、root dentry、block device、flags 等通用状态；
`struct simplefs_sb_info` 保存 SimpleFS 私有状态。磁盘 superblock 只是后者前部的
格式字段。三者相关但不是同一个对象。

## sync、freeze 与 unmount

`simplefs_super_ops` 提供 `sync_fs`、`freeze_fs`、`put_super` 等回调。干净卸载必须：

- 等待文件数据和异步 unwritten 转换；
- 提交 journal transaction；
- 写回必要的 superblock/bitmap 状态；
- 清除 `s_needs_recovery`；
- 释放内存 bitmap、journal 和私有 superblock。

freeze 先执行同步，随后 VFS 的 writer freeze 机制阻止新的修改。它不是普通 sync 的
别名：freeze 还建立了快照或设备操作所需的“没有新 writer 进入”边界。

## 对照源码

- `fs/super.c`、`fs/fs_context.c`：通用 mount 生命周期；
- `fs/ext2/super.c`：较直接的 fill_super；
- `fs/ext4/super.c`：recovery、feature 和错误策略更完整的挂载流程。

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
