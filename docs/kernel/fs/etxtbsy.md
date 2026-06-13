# cp vs mv 替换运行中二进制

## 现象

`cp new old` 报 `Text file busy` (ETXTBSY)，而 `mv new old` 成功。

## 原因

| 操作 | 影响对象     | 内核行为                                                                                    |
| ---- | ------------ | ------------------------------------------------------------------------------------------- |
| cp   | inode 数据块 | 打开已有文件并写入 -> `deny_write_access()` 检查到该 inode 正被映射为可执行 -> 返回 ETXTBSY |
| mv   | dentry 路径  | `rename()` 只改目录项，不碰 inode 数据，旧进程继续持有旧 inode 引用                         |

## 内核检查点

- `fs/open.c:do_dentry_open()` -> `deny_write_access()`
- 被 `execve()` 加载时，inode 的 `i_writecount` 被置为负，标记为"执行中"

## 设计目的

- 安全性：禁止篡改正在执行的指令流
- 原子热更新：`mv` 换 inode，旧进程不受影响，新 exec 走新 inode

测试 demo:
```sh
#!/bin/bash
set -e
cat >/tmp/sleeper.c <<'EOF'
#include <unistd.h>
int main() { while (1) sleep(1); }
EOF
gcc -o /tmp/sleeper /tmp/sleeper.c
/tmp/sleeper &
sleep 0.5
echo "--- cp ---"
cp /bin/ls /tmp/sleeper 2>&1 || true
echo "--- mv ---"
cp /bin/ls /tmp/sleeper.new
mv /tmp/sleeper.new /tmp/sleeper && echo "mv ok"
kill %1 2>/dev/null || true
```

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
