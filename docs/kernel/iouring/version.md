## io uring 版本迭代

1. https://github.com/axboe/liburing/wiki/io_uring-and-networking-in-2023
2. https://lwn.net/Articles/923369/

### 2023
- https://kernel-recipes.org/en/2023/schedule/on-the-way-to-io_uring-networking/

### 具体的
- https://kernelnewbies.org/LinuxChanges#Linux_6.7.io_uring_improvements

- Multishot reads
https://lwn.net/Articles/944291/
- Cancelable uring_cmd
  - 忽然意识到 aio 是无法撤销的，只有超时
- Initial support for {s,g}etsockopt commands
 - 写一个 getsocket 的测试吧


参考 : https://kernelnewbies.org/LinuxVersions


### 6.0
https://kernelnewbies.org/Linux_6.0

### 6.1
https://kernelnewbies.org/Linux_6.1

### 6.2
- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=54e60e505d6144a22c787b5be1fdce996a27be1b
- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=96f7e448b9f4546ffd0356ffceb2b9586777f316

### 6.3
https://kernelnewbies.org/Linux_6.3

### 6.4
https://kernelnewbies.org/Linux_6.4

https://kernelnewbies.org/Linux_6.5

https://kernelnewbies.org/Linux_6.6

## 参考这个吧
https://nick-black.com/dankwiki/index.php/Io_uring

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
