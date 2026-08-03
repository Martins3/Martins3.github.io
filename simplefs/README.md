# SimpleFS

SimpleFS 是一个用于学习 Linux 文件系统的可运行实现。它有固定、可画出来的
磁盘格式，也接入了现代 VFS、folio、iomap 和 JBD2。它的目标不是与 ext4/XFS
竞争功能，而是让一次系统调用如何穿过 VFS、页缓存、块映射和一致性边界变得
可以追踪。

当前代码已经覆盖普通文件、目录、软硬链接、mmap、Direct I/O、fallocate、
xattr、ACL、exportfs 和元数据日志等主线。2026-07-26 的统一环境全量结果是
generic 001--787 中 PASS 432 / NOTRUN 355 / FAIL 0 / TIMEOUT 0。NOTRUN 表示
格式或功能明确不支持，不等于 PASS；完整证据在 [record/README.md](record/README.md)。

## 从哪里开始

第一次阅读按下面的顺序：

1. [教程目录](docs/tutorial/README.md)：从 VFS 对象到磁盘一致性的连续主线；
2. [架构与正确性不变量](docs/reference/architecture.md)：修改代码前必须守住的边界；
3. [构建与测试](docs/development/testing.md)：在物理机编译、在 yyds-fs 中运行；
4. [调试案例](docs/development/debugging.md)：xfstests 暴露过的真实错误及定位方法；
5. [历史记录](record/README.md)：阶段结果、调查证据和未实现功能，不作为当前设计入口。

如果只想建立整体印象，先读教程的第 1、2、3、6、8 章；如果准备修改代码，
再补齐全部章节和架构不变量。

## 最小构建与回归

在物理机以普通用户构建：

```bash
./build.sh
```

在 yyds-fs 中以 root 跑一个用例：

```bash
ssh -p 51404 root@localhost \
  "cd /home/martins3/data/vn/simplefs && ./xfstests-full.sh 001"
```

不带参数覆盖 001--787；也可传 `74`、`generic/074`、`100-200` 或多个参数。
runner 的结构和结果判定见 [xfstests 说明](docs/development/xfstests.md)。

## 仓库地图

| 路径 | 职责 |
| --- | --- |
| `simplefs_fs.c` | 模块注册、fs_context 和挂载参数 |
| `simplefs_super.c` | superblock、挂载、inode 生命周期、statfs、exportfs |
| `simplefs_inode.c` | pathname 回调和目录修改操作 |
| `simplefs_dir.c` | readdir 与目录 fsync |
| `simplefs_file.c` | iomap、页缓存、DIO、mmap、fallocate 和 ioctl |
| `simplefs_extent.c` | 普通文件两级 extent tree |
| `simplefs_metabuf.c` | folio-native 元数据块访问与退役 |
| `simplefs_journal.c` | folio 到 JBD2 的隔离适配层 |
| `simplefs_xattr.c`, `simplefs_acl.c` | 扩展属性和 POSIX ACL |
| `mkfs_common.c`, `user_mkfs.c` | 磁盘布局生成和 mkfs 用户态入口 |
| `docs/` | 当前教程、参考和开发说明 |
| `record/` | 历史证据、阶段记录和归档草稿 |
| `tests/manual/` | 小型、可观察的手工实验 |
| `tests/xfstests/` | xfstests 私有副本的适配层 |

核心约束：普通文件和核心元数据路径保持 folio-native；`buffer_head` 只允许存在于
`simplefs_journal.c` 的 JBD2 客户端适配层。

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
