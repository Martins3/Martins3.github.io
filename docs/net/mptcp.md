## mptcp

- https://www.mptcp.dev/

- https://www.multipath-tcp.org/

> MultiPath TCP (MPTCP) is an effort towards enabling the simultaneous use of several IP-addresses/interfaces
by a modification of TCP that presents a regular TCP interface to applications, while in fact spreading data across several subflows.


代码在 net/mptcp/ 下，大约 1 万行，可以测试下

非常合理，万物皆可 multipath

https://www.redhat.com/en/blog/using-multipath-tcp-better-survive-outages-and-increase-bandwidth

https://github.com/pak-ji/mptcp-socket-example : 小例子


## 用户态接口
/proc/sys/net/mptcp

对应的文档: https://docs.kernel.org/networking/mptcp-sysctl.html

## 不过这个时候，问题来了

bonding , mptcp , nvmetcp 中的 bonding 的功能是不是有点互相重复啊!

也许只是对于这些东西都不熟采有这个疑问吧

## 一些产品的支持
- https://github.com/curl/curl/commit/ab6d5442e80aeb89bcaf932753babca54d41588d : 热乎的(2024-07-20 的时候)
- https://github.com/esnet/iperf/pull/1661

想不到 QEMU 居然也支持，尝试下如何使用吧:

```txt
	<ip>:<port>,mptcp
```

似乎支持起来并不复杂，而且 - https://www.mptcp.dev/ 中甚至介绍了如何通过外部

## 分析的方法
使用 tcpdump :
```txt
tcpdump -n -i ens5 tcp | grep mptcp
```

```txt
12:09:12.474832 IP 10.0.0.3.48302 > 10.0.20.0.48661: Flags [.], ack 343, win 502, options [nop,nop,TS val 325875 2725 ecr 4294749782,mptcp 12 dss ack 15884319971800887743], length 0
12:09:15.475326 IP 10.0.0.3.48302 > 10.0.20.0.48661: Flags [P.], seq 199:221, ack 343, win 502, options [nop,nop ,TS val 3258755725 ecr 4294749782,mptcp 26 dss ack 15884319971800887743 seq 6395071959233486153 subseq 199 len 2 2,nop,nop], length 22
```

## 问题

### 为什么有一个这么复杂的 net/mptcp/sockopt.c

### 如何使用 ebpf 的

### io 路径 如何和传统的 tcp 联系到一起的


## 和 bpf 的关系

> eBPF: since kernel v6.6, it is possible to change the socket being created per cGroup. A small eBPF program – e.g. mptcpify – can be used, see this example.

似乎是特地增加来一个 bpf 的 hook :

配合的地方为:

```c
int __sys_socket(int family, int type, int protocol)
{
	struct socket *sock;
	int flags;

	sock = __sys_socket_create(family, type,
				   update_socket_protocol(family, type, protocol));
	if (IS_ERR(sock))
		return PTR_ERR(sock);

	flags = type & ~SOCK_TYPE_MASK;
	if (SOCK_NONBLOCK != O_NONBLOCK && (flags & SOCK_NONBLOCK))
		flags = (flags & ~SOCK_NONBLOCK) | O_NONBLOCK;

	return sock_map_fd(sock, flags & (O_CLOEXEC | O_NONBLOCK));
}
```

net/mptcp/bpf.c 似乎定义了一个 context 来修改

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
