# [level-ip](https://github.com/saminiir/level-ip)


## env setup
ref : https://github.com/saminiir/level-ip/blob/master/Documentation/getting-started.md
```sh
sudo mknod /dev/net/tap c 10 200
sudo chmod 0666 /dev/net/tap

sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -I INPUT --source 10.0.0.0/24 -j ACCEPT
sudo iptables -t nat -I POSTROUTING --out-interface wlp2s0 -j MASQUERADE
sudo iptables -I FORWARD --in-interface wlp2s0 --out-interface tap0 -j ACCEPT
sudo iptables -I FORWARD --in-interface tap0 --out-interface wlp2s0 -j ACCEPT
# replace wlp2s0 with enxd43a650739d
```

part of output of ifconfig:
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





TCP close : client and server do a "active" finished and receive the corresponding ack, then they are closed, this is diagram.[^1]
![](https://benohead.com/wp-content/uploads/2013/07/TCP-CLOSE_WAIT.png)

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

[^1]: https://benohead.com/blog/2013/07/21/tcp-about-fin_wait_2-time_wait-and-close_wait/
