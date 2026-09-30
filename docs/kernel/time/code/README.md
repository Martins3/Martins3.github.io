# timer 相关测试

测试 syscall 相关


clock_gettime 一共有那些参数，他们的区别是什么?
```txt
CLOCK_REALTIME : 1739203378.180 (20129 days + 16h  2m 58s)
CLOCK_TAI      : 1739203378.180 (20129 days + 16h  2m 58s)
CLOCK_MONOTONIC:     206410.612 (2 days +  9h 20m 10s)
CLOCK_BOOTTIME :     206410.612 (2 days +  9h 20m 10s)
CLOCK_MONOTONIC_RAW:     206410.410 (2 days +  9h 20m 10s)
```

需要有工具测试出来他们的区别是什么

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
