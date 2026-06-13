# find 命令
<!-- c291f0fa-bff2-4e1d-891d-58bdc17cd129 -->

## 基本操作
- find /sys -name numa_node -exec cat {} +

## `\;` 和 +

https://stackoverflow.com/questions/29360925/whats-the-difference-between-and-at-the-end-of-a-find-command

\; 方式（逐个文件执行）
```txt
🧀  find . -name "*hotplug*" -type f -exec ls {} \;
./docs/kernel/hp/hotplug-nvme.md
./docs/kernel/hp/mm-hotplug.md
./docs/kernel/hp/sched-hotplug.sh
./docs/kernel/hp/hotplug.md
./docs/kernel/hp/sched-cpu-hotplug.md
./docs/kernel/hp/storage-hotplug.md
vn on  master [$+] 🔥
```

+ 方式（批量执行，更高效）
```txt
🧀  find . -name "*hotplug*" -type f -exec ls {} +
./docs/kernel/hp/hotplug.md       ./docs/kernel/hp/sched-cpu-hotplug.md
./docs/kernel/hp/hotplug-nvme.md  ./docs/kernel/hp/sched-hotplug.sh
./docs/kernel/hp/mm-hotplug.md    ./docs/kernel/hp/storage-hotplug.md
```

+ 的意思，把所有的都合并起来，其要起 {} 必须在最后，否则有错误为:

```txt
🤒   find . -name "*mmmm*" -type f -exec ls {} mmmm +
find: missing argument to `-exec'
```
但是 \; 就没有这个限制了

## find 和 xargs 配合必须添加使用 print0
同时 xargs 必须 -0

参考 https://www.shellcheck.net/wiki/SC2038

> By default, xargs interprets spaces and quotes in an unsafe and unexpected way.
Whenever it's used, it should be used with -0 or --null to split on \0 bytes, and find should be made to output \0 separated filenames.

- find . -type f -print0 | xargs -0 md5sum


## -delete 参数
find . -size 0 -print0 -delete # 删除大小为 0 的文件"

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
