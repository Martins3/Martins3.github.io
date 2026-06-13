# Linux kernel network stack lock

## 似乎 network 中没有特别复杂的 lock 机制

- [The design of lock_sock() in Linux kernel](https://medium.com/@c0ngwang/the-design-of-lock-sock-in-linux-kernel-69c3406e504b)

## RTNL

- [RTNL mutex, the network stack big kernel lock](https://netdevconf.info/2.2/papers/westphal-rtnlmutex-talk.pdf)

全称 : Routing Netlink Lock

```c
static DEFINE_MUTEX(rtnl_mutex);

void rtnl_lock(void)
{
	mutex_lock(&rtnl_mutex);
}
EXPORT_SYMBOL(rtnl_lock);
```

的确有点逆天了:
```txt
 rg rtnl_lock | wc -l
1088
```

在 virtio_net.c 就有 8 个使用的地方。

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
