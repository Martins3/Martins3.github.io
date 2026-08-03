# SimpleFS

## 目标

SimpleFS 用于学习 Linux 文件系统。代码、教程、正确性不变量、调试难点和测试证据
最终都要保持清晰。项目可以因真实的未实现功能让 xfstests NOTRUN，但不接受 FAIL、
TIMEOUT、错误结果、silent corruption 或内核 warning。

项目长期目标是通过 `/home/martins3/data/xfstests` 的全部 generic 测试。不要用伪造
capability、过滤错误或把真实失败改成 NOTRUN 的方式缩小目标。

## 文档入口

- 项目和源码地图：`README.md`；
- 连续教程：`docs/tutorial/README.md`；
- 正确性不变量：`docs/reference/architecture.md`；
- 构建与测试：`docs/development/testing.md`；
- 历史测试证据：`record/README.md`。

当前格式与行为以源码和上述当前文档为准。`record/archive/` 是早期草稿，不能作为
当前实现依据。

## 构建与回归

模块和 mkfs 在物理机以普通用户构建：

```bash
./build.sh
```

在 yyds-fs 中以 root 运行：

```bash
ssh -p 51404 root@localhost \
  "cd /home/martins3/data/vn/simplefs && ./xfstests-full.sh"
```

不带参数跑 generic/001--787；也可传单个编号、`generic/074`、范围或多个参数。修改
代码后立即跑定向回归并检查 dmesg，最终变化必须经过全量回归。

出现 warning/Oops/panic 时先保存：

```bash
./collei/scripts/collei-action.py -a log -n yyds-fs
```

保存现场后用快速重启，不在 guest 中执行完整 reboot：

```bash
./collei/scripts/collei-action.py -a force_reboot -n yyds-fs
```

## 架构约束

- 参考 `/home/martins3/data/kernel/linux-drm` 中的 ext2、ext4、XFS 和 iomap 实现；
- SimpleFS 核心保持 folio-native；
- `buffer_head` 只允许封装在 `simplefs_journal.c` 的 JBD2 适配层，不得扩散到
  inode、目录、extent、xattr、superblock 或普通文件 I/O；
- 位图是块/inode 所有权的权威来源，计数是派生缓存；
- 任何释放和复用都要同时审计持久指针、页缓存身份、I/O 完成和错误回滚。

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
