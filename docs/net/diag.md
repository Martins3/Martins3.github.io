## 如何理解这个这几个配置?

```txt
net/core/sock_diag.c
net/dccp/diag.c
net/ipv4/inet_diag.c
net/ipv4/raw_diag.c
net/ipv4/tcp_diag.c
net/ipv4/udp_diag.c
net/mptcp/diag.c
net/mptcp/mptcp_diag.c
net/netlink/diag.c
net/packet/diag.c
net/sctp/diag.c
net/smc/smc_diag.c
net/tipc/diag.c
net/unix/diag.c
net/vmw_vsock/diag.c
net/xdp/xsk_diag.c
```

## diag

- https://stackoverflow.com/questions/8236574/how-do-i-use-tcp-diag-with-c

自己写有点难度，但是 ss 命令就是靠这个机制:

```txt
@[
    tcp_diag_dump+5
    __inet_diag_dump+110
    netlink_dump+337
    __netlink_dump_start+477
    inet_diag_handler_cmd+196
    sock_diag_rcv_msg+356
    netlink_rcv_skb+88
    netlink_unicast+409
    netlink_sendmsg+546
    ____sys_sendmsg+924
    ___sys_sendmsg+154
    __sys_sendmsg+122
    do_syscall_64+184
    entry_SYSCALL_64_after_hwframe+119
]: 2
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
