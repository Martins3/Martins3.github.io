## https://linux.die.net/man/7/socket

```c
/* Sock flags */
enum sock_flags {
    SOCK_DEAD,
    SOCK_DONE,
    SOCK_URGINLINE,
    SOCK_KEEPOPEN,
    SOCK_LINGER,
    SOCK_DESTROY,
    SOCK_BROADCAST,
    SOCK_TIMESTAMP,
    SOCK_ZAPPED,
    SOCK_USE_WRITE_QUEUE, /* whether to call sk->sk_write_space in sock_wfree */
    SOCK_DBG, /* %SO_DEBUG setting */
    SOCK_RCVTSTAMP, /* %SO_TIMESTAMP setting */
    SOCK_RCVTSTAMPNS, /* %SO_TIMESTAMPNS setting */
    SOCK_LOCALROUTE, /* route locally only, %SO_DONTROUTE setting */
    SOCK_MEMALLOC, /* VM depends on this socket for swapping */
    SOCK_TIMESTAMPING_RX_SOFTWARE,  /* %SOF_TIMESTAMPING_RX_SOFTWARE */
    SOCK_FASYNC, /* fasync() active */
    SOCK_RXQ_OVFL,
    SOCK_ZEROCOPY, /* buffers from userspace */
    SOCK_WIFI_STATUS, /* push wifi status to userspace */
    SOCK_NOFCS, /* Tell NIC not to do the Ethernet FCS.
             * Will use last 4 bytes of packet sent from
             * user-space instead.
             */
    SOCK_FILTER_LOCKED, /* Filter cannot be changed anymore */
    SOCK_SELECT_ERR_QUEUE, /* Wake select on error queue */
    SOCK_RCU_FREE, /* wait rcu grace period in sk_destruct() */
    SOCK_TXTIME,
    SOCK_XDP, /* XDP is attached */
    SOCK_TSTAMP_NEW, /* Indicates 64 bit timestamps always */
    SOCK_RCVMARK, /* Receive SO_MARK  ancillary data with packet */
};
```

可以看看 socket 的所有的 option 都有啥 ?

- SO_BINDTODEVICE


- [AF 和 PF family 有什么区别?](https://stackoverflow.com/questions/6729366/what-is-the-difference-between-af-inet-and-pf-inet-in-socket-programming)
  - 没有区别

## everything is file 和 socket 接口的差别是什么，为什么如果是 socket 最好是使用 socket 的
参考 man send(2)

> The only difference between send() and write(2) is the  presence  of  flags.

常规的 read write 从这里切入:
```sh
static const struct file_operations socket_file_ops ;
```
使用 kprobe 观察下，应该是很清晰的:

```txt
@[
    inet_sendmsg+5
    sock_write_iter+367
    vfs_write+889
    ksys_write+187
    do_syscall_64+93
    entry_SYSCALL_64_after_hwframe+110
]: 1054
@[
    inet_sendmsg+5
    sock_sendmsg+254
    splice_to_socket+981
    do_splice+788
    __do_splice+496
    __x64_sys_splice+178
    do_syscall_64+93
    entry_SYSCALL_64_after_hwframe+110
]: 24459
@[
    inet_sendmsg+5
    __sys_sendto+478
    __x64_sys_sendto+36
    do_syscall_64+93
    entry_SYSCALL_64_after_hwframe+110
]: 35897
```

## AF_PACKET
<!-- 4034913e-c3ca-45d8-a6c2-7bddd795c780 -->

- [Wireshark 的工作原理](https://stackoverflow.com/questions/29620590/where-does-the-wireshark-capture-the-packets)
- [ ] wireshark 使用 `AF_PACKET` ，那么 bpf filter 发生在什么位置啊?
- [How does the `AF_PACKET` socket work in Linux?](https://stackoverflow.com/questions/62866943/how-does-the-af-packet-socket-work-in-linux)

[Linux drivers in user space — a survey](https://lwn.net/Articles/703785/)

> Direct access to a network device can be achieved by creating a network socket using the AF_PACKET address family and specifying the SOCK_RAW communication type. This socket can then be bound to a particular interface or a particular Ethernet protocol type. A slightly less direct interface can be had by using SOCK_RAW with AF_INET. This still provides the routing and other functionality common to all IP protocols, but gives complete control over the payload of each IP packet.

1. **最直接的访问：`AF_PACKET + SOCK_RAW`**

- **`AF_PACKET`** 是 Linux 特有的地址族，用于直接操作 **数据链路层（Layer 2）**，比如以太网帧（Ethernet frames）。
- 使用 `socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))` 可以：
  - **发送任意以太网帧**（包括自定义 MAC 地址、协议类型等）。
  - **接收来自指定网络接口的所有原始以太网帧**（需 root 权限或 CAP_NET_RAW 能力）。
- 可通过 `bind()` 绑定到：
  - **特定网络接口**（如 eth0）。
  - **特定以太网协议类型**（如只接收 ARP `ETH_P_ARP`，或只接收 IPv4 `ETH_P_IP`）。

> ✅ **用途**：实现协议栈底层工具（如 tcpdump、arping）、自定义协议、网络测试、安全工具等。

2. **稍微间接但更高级的访问：`AF_INET + SOCK_RAW`**

- **`AF_INET`** 是 IP 层（Layer 3）的地址族。
- 使用 `socket(AF_INET, SOCK_RAW, IPPROTO_RAW)`（或其他协议号如 `IPPROTO_TCP`）可以：
  - **完全控制 IP 数据包的载荷（payload）**，即你可自行构造 TCP/UDP/ICMP 等头部。
  - **但 IP 头部（或部分）可能由内核自动处理**（取决于是否设置 `IP_HDRINCL` 选项）。
- **内核仍负责**：
  - 路由决策（根据目标 IP 选择出口接口）。
  - IP 分片（如需要）。
  - 校验和计算（部分协议可由硬件或内核完成）。

> ✅ **用途**：实现自定义传输协议、ICMP 工具（如 ping）、网络探测、绕过标准 socket API 的限制等。
> ❌ **不能操作 MAC 层**，也无法指定出口接口的 MAC 地址或监听非 IP 协议（如 ARP）。

| 特性       | `AF_PACKET + SOCK_RAW`          | `AF_INET + SOCK_RAW`             |
|------------|---------------------------------|----------------------------------|
| 网络层     | **数据链路层（L2）**            | **网络层（L3）**                 |
| 可构造内容 | 完整以太网帧（含 MAC）          | IP 载荷（可含 IP 头）            |
| 路由       | 由应用决定（如指定接口）        | 由内核路由子系统决定             |
| 协议范围   | 任意以太网协议（IP/ARP/自定义） | 仅 IP 协议族（TCP/UDP/ICMP 等）  |
| 权限要求   | 高（CAP_NET_RAW）               | 高（通常也需要 CAP_NET_RAW）     |
| 典型工具   | tcpdump, Scapy (L2), arp        | ping, traceroute, 自定义 IP 工具 |

- 若你想 **伪造一个 ARP 请求** → 必须用 `AF_PACKET`，因为 ARP 不是 IP 协议。
- 若你想 **发送一个自定义的 ICMP Echo Request（ping）** → 可用 `AF_INET + SOCK_RAW`，更简单，且自动走正确路由。

## [ ] 分析一下这些 flags，比如 `SOCK_ZEROCOPY`

## AF_UNIX
unix domain socket 使用的

## 勉强读一读
- https://macoy.me/blog/programming/Sockets

## 用这个做对比
vn/code/src/c/dccp/README.md

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
