# nfs rfc
https://www.rfc-editor.org/rfc/rfc8881.html

状态恢复，分布式锁，缓存策略

## v3 vs v4 的差别
> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
  NFSv3 更适合“最小用户态 server”

  - v3 是无状态协议，核心就是一组独立 RPC：LOOKUP/GETATTR/READ/WRITE/CREATE/REMOVE。
  - 只要再补一个 mountd 的 MNT，Linux client 就能拿到 root file handle，然后开始发 NFS 请求。
  - 不需要实现 open/close 状态机、lease、clientid、sequence、compound op 这些东西。
  - 对“研究 NFS 基本 IO 路径”来说，v3 足够清楚。

  NFSv4 不适合这个最小目标

  - v4 把 mount 协议合并进 NFS 本身，但代价是协议复杂很多。
  - Linux client 挂载 v4 时会走 COMPOUND 请求，需要支持 PUTROOTFH/LOOKUP/GETFH/GETATTR/OPEN/READ/WRITE/CLOSE/REMOVE/CREATE 等一串操
    作。

  - v4 有 stateful open、stateid、clientid、lease、reclaim、sequence/session 相关语义。哪怕“假实现”，也要骗过 Linux client 的状态检
    查。

  - 如果只想支持基本 io/remove/create，v4 的前置机制会占掉大部分实现量。
```
## 问题

1. nfs 可以基于 tcp ，也可以基于 udp 啊


## 如果 server 退出，client 卡住了，如何解决
sudo umount -f -l /tmp/user-nfsd-mnt

-f
-l : 延迟退出

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
