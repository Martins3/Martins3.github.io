<!-- source code record under net/ipv4/ -->

# af_net.c
- [ ] inet_sendmsg

```c
/* Upon startup we insert all the elements in inetsw_array[] into
 * the linked list inetsw.
 */
static struct inet_protosw inetsw_array[] =
{
	{
		.type =       SOCK_STREAM,
		.protocol =   IPPROTO_TCP,
		.prot =       &tcp_prot,
		.ops =        &inet_stream_ops,
		.flags =      INET_PROTOSW_PERMANENT |
			      INET_PROTOSW_ICSK,
	},

	{
		.type =       SOCK_DGRAM,
		.protocol =   IPPROTO_UDP,
		.prot =       &udp_prot,
		.ops =        &inet_dgram_ops,
		.flags =      INET_PROTOSW_PERMANENT,
       },

       {
		.type =       SOCK_DGRAM,
		.protocol =   IPPROTO_ICMP,
		.prot =       &ping_prot,
		.ops =        &inet_sockraw_ops,
		.flags =      INET_PROTOSW_REUSE,
       },

       {
	       .type =       SOCK_RAW,
	       .protocol =   IPPROTO_IP,	/* wild card */
	       .prot =       &raw_prot,
	       .ops =        &inet_sockraw_ops,
	       .flags =      INET_PROTOSW_REUSE,
       }
};
```

Both inet_stream_ops::sendmsg and inet_dgram_ops::sendmsg are registered with `inet_sendmsg`.

```c
int inet_sendmsg(struct socket *sock, struct msghdr *msg, size_t size)
{
	struct sock *sk = sock->sk;

	if (unlikely(inet_send_prepare(sk)))
		return -EAGAIN;

	return INDIRECT_CALL_2(sk->sk_prot->sendmsg, tcp_sendmsg, udp_sendmsg,
			       sk, msg, size);
}
```

# TCP UDP ICMP IGMP TUNNEL
```c
/* This is used to register protocols. */
struct net_protocol {
	int			(*early_demux)(struct sk_buff *skb);
	int			(*early_demux_handler)(struct sk_buff *skb);
	int			(*handler)(struct sk_buff *skb);

	/* This returns an error if we weren't able to handle the error. */
	int			(*err_handler)(struct sk_buff *skb, u32 info);

	unsigned int		no_policy:1,
				netns_ok:1,
				/* does the protocol do more stringent
				 * icmp tag validation than simple
				 * socket lookup?
				 */
				icmp_strict_tag_validation:1;
};
```
1. 看 handler 的注册函数就可以很清楚的知道, 其使用者在 ip_input.c

- [ ] 所以，现在需要分析一下 IP 和 ARP 的层次还有什么东西

2. [An introduction to Linux virtual interfaces: Tunnels](https://developers.redhat.com/blog/2019/05/17/an-introduction-to-linux-virtual-interfaces-tunnels/)
  - [https://en.wikipedia.org/wiki/Tunneling_protocol](https://en.wikipedia.org/wiki/Tunneling_protocol)
  - /home/maritns3/core/linux/net/ipv4/tunnel4.c 中间描述 tunnel 的通用接口吧, 证据在 tunnel4_rcv 中间的 `for_each_tunnel_rcu(tunnel4_handlers, handler)`


- IP in IP (Protocol 4): IP in IPv4/IPv6
- SIT/IPv6 (Protocol 41): IPv6 in IPv4/IPv6
- GRE (Protocol 47): Generic Routing Encapsulation
- OpenVPN (UDP port 1194)
- SSTP (TCP port 443): Secure Socket Tunneling Protocol
- IPSec (Protocol 50 and 51): Internet Protocol Security
- L2TP (Protocol 115): Layer 2 Tunneling Protocol
- VXLAN (UDP port 4789): Virtual Extensible Local Area Network.
- WireGuard

对应的文件在:
1. /home/maritns3/core/linux/net/ipv4/ipip.c
2. /home/maritns3/core/linux/net/ipv6/sit.c
3. /home/maritns3/core/linux/net/ipv4/ip_gre.c

- [ ] https://www.linuxjournal.com/article/6070

# ipconfig.c
Automatic Configuration of IP -- use DHCP, BOOTP, RARP, or user-supplied information to configure own IP address and routes.

为了获取 IP address 的机制，那么显然是在 ethernet 上工作的
