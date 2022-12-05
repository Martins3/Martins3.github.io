# [level-ip](https://github.com/saminiir/level-ip)

- [ ] 画个图好吧 !
- [ ] 将 linux kernel 中间对应的部分写出来好吧

## env setup
ref : https://github.com/saminiir/level-ip/blob/master/Documentation/getting-started.md
```sh
sudo mknod /dev/net/tap c 10 200
sudo chmod 0666 /dev/net/tap


# 似乎这才是正确使用 tap 的 : https://stackoverflow.com/questions/15626088/tap-interfaces-and-dev-net-tun-device-using-ip-tuntap-command
# sudo ip tuntap add mode tap tap0
# sudo ip link set dev tap0 up

sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -I INPUT --source 10.0.0.0/24 -j ACCEPT
sudo iptables -t nat -I POSTROUTING --out-interface wlp2s0  -j MASQUERADE
sudo iptables -I FORWARD --in-interface wlp2s0 --out-interface tap0 -j ACCEPT
sudo iptables -I FORWARD --in-interface tap0 --out-interface wlp2s0 -j ACCEPT

# 其他的配置
sudo iptables -t nat -I POSTROUTING --out-interface  enxd43a650739d8 -j MASQUERADE
sudo iptables -I FORWARD --in-interface enxd43a650739d8 --out-interface tap0 -j ACCEPT
sudo iptables -I FORWARD --in-interface tap0 --out-interface enxd43a650739d8 -j ACCEPT

sudo iptables -I FORWARD --in-interface enxd43a650739d8 --out-interface tap -j ACCEPT
sudo iptables -I FORWARD --in-interface tap --out-interface enxd43a650739d8 -j ACCEPT
```

replace wlp2s0 with enxd43a650739d, because this is part of output of ifconfig:
```
enxd43a650739d8: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 10.170.133.9  netmask 255.255.0.0  broadcast 10.170.255.255
        ether d4:3a:65:07:39:d8  txqueuelen 1000  (Ethernet)
        RX packets 5951876  bytes 7731714225 (7.7 GB)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 3694072  bytes 318623167 (318.6 MB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```
remove -Werror in ./Makefile

由于不支持 ipv6，也许有用的内容:
sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1
sudo sysctl -w net.ipv6.conf.default.disable_ipv6=1
sudo sysctl -w net.ipv6.conf.lo.disable_ipv6=1

注意点 :  tools/liblevelip.c 不能包含 /usr/include 的头文件，否则由于符号使用不同类型的重复定义而出错。
```c
Unknown IP header proto 17
Unknown IP header proto 17
Unknown IP header proto 17
Unknown IP header proto 17
```
使用方法:
1. 在一个终端  : $ ./lvl-ip
2. 在另一个终端 :

- [ ] 配置问题 ： 到底是 tap 还是 tap0

## notes
到底如何处理 ofo_queue 的 ？

- [ ] struct sk_buff_head tcp_sock::ofo_queue


- [ ] TCP 的变量，一个窗口就是两个数值就对了啊，实际上，

tcp_data_queue 中间， `int expected = skb->seq == tcb->rcv_nxt;`

```c
static void tcp_init_segment(struct tcphdr *th, struct iphdr *ih, struct sk_buff *skb)
{
    th->sport = ntohs(th->sport);
    th->dport = ntohs(th->dport);
    th->seq = ntohl(th->seq);
    th->ack_seq = ntohl(th->ack_seq);
    th->win = ntohs(th->win);
    th->csum = ntohs(th->csum);
    th->urp = ntohs(th->urp);

    skb->seq = th->seq; // TODO seq 数值就是代码的长度 ? 感觉哪里有点不对，应该用工具查看一下
    skb->dlen = ip_len(ih) - tcp_hlen(th);
    skb->len = skb->dlen + th->syn + th->fin;
    skb->end_seq = skb->seq + skb->dlen;
    skb->payload = th->data;
}
```



```c
static struct net_family *families[128] = {
    [AF_INET] = &inet,
};

struct net_family inet = {
    .create = inet_create,
};

static struct sock_type inet_ops[] = {
    {
        .sock_ops = &inet_stream_ops,
        .net_ops = &tcp_ops,
        .type = SOCK_STREAM,
        .protocol = IPPROTO_TCP,
    }
};

static struct sock_ops inet_stream_ops = {
    .connect = &inet_stream_connect,
    .write = &inet_write,
    .read = &inet_read,
    .close = &inet_close,
    .free = &inet_free,
    .abort = &inet_abort,
    .getpeername = &inet_getpeername,
    .getsockname = &inet_getsockname,
};
```

- [ ] Why we need unix socket to handle it ?

- `_read` : this is almost the
  - inet_read
    - tcp_read
      - tcp_receive
        - tcp_data_dequeue


## linux kernel

## 其他的用户态网络栈
- [mtcp](https://github.com/mtcp-stack/mtcp) : 这个是个工业级的

[^1]: https://benohead.com/blog/2013/07/21/tcp-about-fin_wait_2-time_wait-and-close_wait/
