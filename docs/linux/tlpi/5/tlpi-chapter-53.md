# POSIX Semaphores
The oflag argument is a bit mask that determines whether we are opening an existing semaphore or creating and opening a new semaphore. If oflag is 0, we are accessing
an existing semaphore. If O_CREAT is specified in oflag, then a new semaphore is created
if one with the given name doesn’t already exist. If oflag specifies both O_CREAT and
O_EXCL, and a semaphore with the given name already exists, then sem_open() fails.
> 所以当， 名称已经重复而且出现了，而且没有设置 O_EXCL 并且设置了 O_CREAT 的时候，那么如何办。

If a blocked sem_wait() call is interrupted by a signal handler, then it fails with
the error EINTR, regardless of whether the SA_RESTART flag was used when establishing the signal handler with sigaction().
# Linux Programming Interface: Chapter 53

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
