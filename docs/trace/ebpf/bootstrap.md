## 1 ；类似这种的如何理解?

```c
SEC("perf_event")
int profile(void *ctx)
```

## 2 : 利用 bpf 更加方便的统计

map 的使用?

可以在其中 dumpstack 吗?


## 3 : socket 啊
sockfilter.c 中，为什么 bpf 需要 raw socket 才可以正确运行?

```txt
SEC("socket")
int socket_handler(struct __sk_buff *skb)
```

是不是，tcpdump 就是用的这个技术?

## 4. tc

可以抓 ping 127.0.0.1 ，实在是有趣啊

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
