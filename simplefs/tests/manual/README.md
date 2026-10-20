# 手工实验

这些程序用于观察单个概念，不替代 xfstests。

| 文件 | 观察内容 |
| --- | --- |
| `smoke.sh` | mkfs、mount、普通文件/目录、fsync、重挂载读回 |
| `name-max.sh` | 255 字节名字的 create/remount/rename/unlink 和 256 拒绝 |
| `mmap.c` | MAP_SHARED 写入、msync、重开读回 |
| `shutdown-read.c` | 专用 SimpleFS 卷 shutdown 后，read/sendfile 均须返回 EIO |
| `large-directory.c` | 10 万项目录、重挂内容、槽位复用、硬链接、rename 和 extent 根/叶转换 |
| `fibmap.c` | 逻辑 block 0 到物理 block 的 FIBMAP 查询 |

先在物理机运行 `./build.sh`。Shell 实验在 yyds-fs 中以 root 执行；C 程序生成的
binary 必须使用 `.out` 后缀：

```bash
cc -Wall -Wextra -O2 tests/manual/mmap.c -o mmap-test.out
cc -Wall -Wextra -O2 tests/manual/fibmap.c -o fibmap-test.out
```

实验默认使用 `/mnt/simplefs` 或脚本自己的 `/var/tmp` loop image。执行前确认没有需要
保留的数据。

`large-directory.c` 使用专用空测试卷，按顺序运行 `create`、卸载重挂、`verify`、
`mutate`、再次卸载重挂、`clean`；每次都传相同的目录和文件数，例如：

```bash
cc -Wall -Wextra -O2 tests/manual/large-directory.c -o tests/manual/large-directory.out
./tests/manual/large-directory.out /mnt/simplefs 100000 create
```

`verify` 同时检查 400 个稀疏 extent 并执行收缩，所以只在原始 create 后运行一次。
`clean` 删除本实验创建的 `many/` 与 `extents`，不清除测试卷其他内容。

`shutdown-read.c` 需要预先创建一个空测试文件，并会主动 shutdown 所在的 SimpleFS 卷；
仅在可重新格式化的专用测试卷上运行。它直接检查 read 和 sendfile，不依赖 cat 版本。
