## mac 会不断的产生这个日志
```txt
May 26 23:51:55 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:52:39 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:52:39 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:52:39 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:52:41 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:53:24 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:53:24 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:58:24 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:58:24 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:58:24 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:58:24 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:58:24 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
May 26 23:58:26 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
```

### 重新购买了一个雷电转网卡的新设备之后，这个日志消失了

```txt
[   11.335597] warning: `P[proc]' uses wireless extensions which will stop working for Wi-Fi 7 hardware; use nl80211
[   36.617781] systemd-journald[813]: /var/log/journal/18ba38df95fc453ba5b2bd0e1891d552/user-1000.journal: Journal file uses a different sequence number ID, rotating.
[   75.023718] Key type dns_resolver registered
[   75.094570] NFS: Registering the id_resolver key type
[   75.094578] Key type id_resolver registered
[   75.094581] Key type id_legacy registered
[  256.338593] cdc_ncm 2-1:2.0 enu1c2: entered promiscuous mode
[ 2757.186166] tun: Universal TUN/TAP device driver, 1.6
[ 2757.187188] vif60.2: entered promiscuous mode
[ 2757.251965] vif60.3: entered promiscuous mode
[ 2759.181633] NET: Registered PF_VSOCK protocol family
```

## promiscuous mode
1. mac 的上的问题
2. iftop 有一个选项
```txt
🧀  iftop -h
iftop: display bandwidth usage on an interface by host

Synopsis: iftop -h | [-npblNBP] [-i interface] [-f filter code]
                               [-F net/mask] [-G net6/mask6]

   -h                  display this message
   -n                  don't do hostname lookups
   -N                  don't convert port numbers to services
   -p                  run in promiscuous mode (show traffic between other
                       hosts on the same network segment)
   -b                  don't display a bar graph of traffic
   -B                  Display bandwidth in bytes
   -i interface        listen on named interface
   -f filter code      use filter code to select packets to count
                       (default: none, but only IP packets are counted)
   -F net/mask         show traffic flows in/out of IPv4 network
   -G net6/mask6       show traffic flows in/out of IPv6 network
   -l                  display and count link-local IPv6 traffic (default: off)
   -P                  show ports as well as hosts
   -m limit            sets the upper limit for the bandwidth scale
   -c config file      specifies an alternative configuration file
   -t                  use text interface without ncurses

   Sorting orders:
   -o 2s                Sort by first column (2s traffic average)
   -o 10s               Sort by second column (10s traffic average) [default]
   -o 40s               Sort by third column (40s traffic average)
   -o source            Sort by source address
   -o destination       Sort by destination address

   The following options are only available in combination with -t
   -s num              print one single text output afer num seconds, then quit
   -L num              number of lines to print

iftop, version 1.0pre4
copyright (c) 2002 Paul Warren <pdw@ex-parrot.com> and contributors
```

- [网卡的工作模式](https://zdyxry.github.io/2020/03/18/%E7%90%86%E8%A7%A3%E7%BD%91%E5%8D%A1%E6%B7%B7%E6%9D%82%E6%A8%A1%E5%BC%8F/)
  - 广播和多播有什么区别吗?

### 测试下 libpcap 的编程
- https://www.tcpdump.org/pcap.html
- https://www.devdungeon.com/content/using-libpcap-c

关注一下，libpcap 会自动配置为 promiscuous mode 吗?

## 测试
```sh
ip link set eth1 promisc on
```

如果是 virtio io 网卡，最后是 : virtnet_rx_mode_work

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
