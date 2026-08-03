# 手工实验

这些程序用于观察单个概念，不替代 xfstests。

| 文件 | 观察内容 |
| --- | --- |
| `smoke.sh` | mkfs、mount、普通文件/目录、fsync、重挂载读回 |
| `name-max.sh` | 255 字节名字的 create/remount/rename/unlink 和 256 拒绝 |
| `mmap.c` | MAP_SHARED 写入、msync、重开读回 |
| `fibmap.c` | 逻辑 block 0 到物理 block 的 FIBMAP 查询 |

先在物理机运行 `./build.sh`。Shell 实验在 yyds-fs 中以 root 执行；C 程序生成的
binary 必须使用 `.out` 后缀：

```bash
cc -Wall -Wextra -O2 tests/manual/mmap.c -o mmap-test.out
cc -Wall -Wextra -O2 tests/manual/fibmap.c -o fibmap-test.out
```

实验默认使用 `/mnt/simplefs` 或脚本自己的 `/var/tmp` loop image。执行前确认没有需要
保留的数据。

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
