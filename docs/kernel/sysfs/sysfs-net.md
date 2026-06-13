# sysfs net
## /proc/net
- net/ipv4/proc.c
- net/ipv6/proc.c

- Documentation/networking/proc_net_tcp.rst

搜索每一个字段实现的位置 `proc_create_net("`

这是一个超级大的目录
```txt
├── anycast6
├── arp
├── bnep
├── connector
├── dev
├── dev_mcast
├── dev_snmp6
│   ├── br-3177677df140
│   ├── br-a7c25b8e6a73
│   ├── br-c06975109cc3
│   ├── br-in
│   ├── cni0
│   ├── docker0
│   ├── enp5s0
│   ├── enp6s0
│   ├── flannel.1
│   ├── lo
│   ├── ovs-system
│   ├── tailscale0
│   ├── veth1004d86a
│   ├── veth1536e405
│   ├── veth40bec009
│   ├── veth9e2dea91
│   ├── vethe1fb6ce5
│   ├── vif12.0
│   ├── vif12.1
│   ├── vif12.2
│   ├── vif13.0
│   ├── vif13.1
│   ├── vif13.2
│   ├── vif21.0
│   ├── vif21.1
│   ├── vif21.2
│   ├── vif26.0
│   ├── vif26.1
│   ├── vif26.2
│   ├── vif27.0
│   ├── vif27.1
│   ├── vif27.2
│   ├── vif28.0
│   ├── vif28.1
│   ├── vif28.2
│   ├── vif29.0
│   ├── vif29.1
│   ├── vif29.2
│   ├── vif5.0
│   ├── vif5.1
│   ├── vif5.2
│   ├── vif9.0
│   ├── vif9.1
│   ├── vif9.2
│   └── wlo1
├── fib_trie
├── fib_triestat
├── hci
├── icmp
├── icmp6
├── if_inet6
├── igmp
├── igmp6
├── ip6_flowlabel
├── ip6_mr_cache
├── ip6_mr_vif
├── ip_mr_cache
├── ip_mr_vif
├── ip_tables_matches
├── ip_tables_names
├── ip_tables_targets
├── ipv6_route
├── l2cap
├── mcfilter
├── mcfilter6
├── netfilter
│   ├── nf_log
│   └── nfnetlink_log
├── netlink
├── netstat
├── packet
├── protocols
├── psched
├── ptype
├── raw
├── raw6
├── rfcomm
├── route
├── rpc
│   ├── auth.rpcsec.context
│   │   ├── channel
│   │   └── flush
│   ├── auth.rpcsec.init
│   │   ├── channel
│   │   └── flush
│   ├── auth.unix.gid
│   │   ├── channel
│   │   ├── content
│   │   └── flush
│   ├── auth.unix.ip
│   │   ├── channel
│   │   ├── content
│   │   └── flush
│   ├── gss_krb5_enctypes
│   ├── nfs4.idtoname
│   │   ├── channel
│   │   ├── content
│   │   └── flush
│   ├── nfs4.nametoid
│   │   ├── channel
│   │   ├── content
│   │   └── flush
│   ├── nfsd
│   ├── nfsd.export
│   │   ├── channel
│   │   ├── content
│   │   └── flush
│   ├── nfsd.fh
│   │   ├── channel
│   │   ├── content
│   │   └── flush
│   └── use-gss-proxy
├── rt6_stats
├── rt_acct
├── rt_cache
├── sco
├── snmp
├── snmp6
├── sockstat
├── sockstat6
├── softnet_stat
├── stat
│   ├── arp_cache
│   ├── ndisc_cache
│   └── rt_cache
├── tcp
├── tcp6
├── udp
├── udp6
├── udplite
├── udplite6
├── unix
└── wireless
```

### /proc/net/dev : 统计

```txt
🧀  column -t /proc/net/dev
Inter-|           Receive     |        Transmit
face              |bytes      packets  errs      drop  fifo  frame  compressed  multicast        |bytes      packets     errs     drop  fifo  colls  carrier  compressed
lo:               1359509820  1327480  0         0     0     0      0           0                1359509820  1327480  0     0     0      0        0           0
enp6s0:           3682998     14228    0         0     0     0      0           3614             736744      3334     0     0     0      0        0           0
enp5s0:           75513003    941577   0         0     0     0      0           27447            581828683   2765235  0     0     0      0        0           0
wlo1:             1230709118  1731240  0         0     0     0      0           0                215042305   808231   0     6     0      0        0           0
ovs-system:       0           0        0         0     0     0      0           0                0           0        0     0     0      0        0           0
br-in:            596100352   1737488  0         0     0     0      0           0                1174354586  2669311  0     0     0      0        0           0
tailscale0:       17177731    308895   0         0     0     0      0           0                116894797   411671   0     0     0      0        0           0
br-90f636c9ff3f:  0           0        0         0     0     0      0           0                0           0        0     333   0      0        0           0
br-a7c25b8e6a73:  0           0        0         0     0     0      0           0                0           0        0     333   0      0        0           0
br-c06975109cc3:  0           0        0         0     0     0      0           0                0           0        0     331   0      0        0           0
docker0:          0           0        0         0     0     0      0           0                0           0        0     331   0      0        0           0
br-3177677df140:  0           0        0         0     0     0      0           0                0           0        0     331   0      0        0           0
vif101.2:         13632207    54632    0         0     0     0      0           0                189461386   76442    0     51    0      0        0           0
vif101.3:         749181354   2189328  0         0     0     0      0           0                639916777   1463514  0     48    0      0        0           0
vif117.2:         5834        31       0         0     0     0      0           0                544888      3365     0     92    0      0        0           0
vif117.3:         0           0        0         0     0     0      0           0                550722      3396     0     91    0      0        0           0
vif11.2:          13454       50       0         0     0     0      0           0                13947       36       0     33    0      0        0           0
vif11.3:          2555        16       0         0     0     0      0           0                6616        48       0     33    0      0        0           0
vif16.2:          0           0        0         0     0     0      0           0                10012       60       0     32    0      0        0           0
vif16.3:          0           0        0         0     0     0      0           0                10012       60       0     31    0      0        0           0
vif88.2:          0           0        0         0     0     0      0           0                2152        22       0     60    0      0        0           0
vif88.3:          4418        69       0         0     0     0      0           0                81010       498      0     60    0      0        0           0
vif80.2:          12774       137      0         0     0     0      0           0                11429       124      0     30    0      0        0           0
vif80.3:          10173       106      0         0     0     0      0           0                14030       155      0     30    0      0        0           0
```

### /proc/net/unix
net/unix/af_unix.c : unix_seq_show

## /proc/sys/net


```txt
-  bridge            ->
-  core              -> net/core/sysctl_net_core.c
-  ipv4              -> net/ipv4/sysctl_net_ipv4.c
-  ipv6
-  mptcp             -> net/mptcp/ctrl.c
-  netfilter         -> net/netfilter/nf_conntrack_standalone.c
-  nf_conntrack_max
-  unix              -> net/unix/sysctl_net_unix.c
```

可以把 /proc/sys/net 下的所有文件都提供下

```txt
🧀  cat /proc/sys/net/core/message_burst
10
1076034/var2/log 13900k
🧀  cat /proc/sys/net/core/message_cost
5
1076034/var2/log 13900k
```


```txt
🤒  cat /proc/sys/net/ipv4/ip_local_port_range
32768   60999
```

和这个 IP_LOCAL_PORT_RANGE 是什么关系?

如果有所谓的 IANA 限制，只是分配而已，为什么代码上还需要考虑到他们？


## /sys/class/net

### /sys/class/net/$dev/statistics

## /proc/pid/net

和 /proc/net 中的内容相同，但是是取决于 process 所在的 namespace ，我猜测实现的难度并不高

例如 unix_net_init 中，可以参考 unix_seq_ops 的执行携带参数 net 的，net 就是当时的 namespace 了
```c
	if (!proc_create_net("unix", 0, net->proc_net, &unix_seq_ops,
			     sizeof(struct seq_net_private)))
		goto err_sysctl;
```


### cat /sys/class/net/enp5s0/address

如何快速获取到 interface 对应的源码

通过:
rg DEVICE_ATTR | grep address

- alloc_netdev : 在 /sys/class/net 创建一个文件夹出来

实现的文件的内容都在这里 : net/core/net-sysfs.c

- net/core/gen_stats.c 实现 /sys/class/net/enp7s0/statistics


### 不同类型的设备的结构不同
```txt
 addr_assign_type   carrier_changes      dormant             ifalias     mtu                    owner            proto_down   testing        uevent
 addr_len           carrier_down_count   duplex              ifindex     name_assign_type       phys_port_id     queues       threaded       upper_ovs-system
 address            carrier_up_count     flags               iflink      napi_defer_hard_irqs   phys_port_name   speed        tun_flags
 broadcast          dev_id               gro_flush_timeout   link_mode   netdev_group           phys_switch_id   statistics   tx_queue_len
 carrier            dev_port             group               master      operstate              power            subsystem    type
```

#### tap

```c
static DEVICE_ATTR_RO(tun_flags);
static DEVICE_ATTR_RO(owner);
static DEVICE_ATTR_RO(group);

static struct attribute *tun_dev_attrs[] = {
	&dev_attr_tun_flags.attr,
	&dev_attr_owner.attr,
	&dev_attr_group.attr,
	NULL
};

static const struct attribute_group tun_attr_group = {
	.attrs = tun_dev_attrs
};
```

#### wg

似乎是没有额外的内容的。

## /proc/$pid/net/dev

在 13900k 的物理机中:
```txt
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo: 45488359171 23380032    0    0    0     0          0         0 45488359171 23380032    0    0    0     0       0          0
enp5s0: 12001938009 10367664    0    0    0     0          0    210901 4255357361 5780796    0    0    0     0       0          0
  wlo1: 136959341039 126634386    0 186926    0     0          0         0 24044948181 38983909    0    0    0     0       0          0
ovs-system:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
 br-in: 11233358375 5109264    0    0    0     0          0         0 40134605873 3851332    0    0    0     0       0          0
tailscale0: 1427722   14051    0    0    0     0          0         0 42078882   74818    0    0    0     0       0          0
br-3177677df140:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
br-a7c25b8e6a73:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
br-c06975109cc3:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
docker0:       0       0    0    0    0     0          0         0    28211     156    0    0    0     0       0          0
vif50.2: 37193504  460048    0    0    0     0          0         0 33599674510 1067168    0 10119    0     0       0          0
vif50.3:  864968   19822    0    0    0     0          0         0 29531579  276515    0 10165    0     0       0          0
vif5.2: 1077035    9777    0    0    0     0          0         0 171396607   22809    0    0    0     0       0          0
vif5.3:       0       0    0    0    0     0          0         0  2759654   15949    0    0    0     0       0          0
vif55.2: 9271023   67999    0    0    0     0          0         0 2704004959  193348    0    1    0     0       0          0
vif55.3: 2897523   12626    0    0    0     0          0         0 14026330   87299    0    1    0     0       0          0
vif45.2:   19156     278    0    0    0     0          0         0  3796804   22299    0    3    0     0       0          0
vif45.3:   19154     277    0    0    0     0          0         0  3796786   22300    0   16    0     0       0          0
vif11.2:   17280     141    0    0    0     0          0         0  1789236   10489    0    0    0     0       0          0
vif11.3:  825921    4871    0    0    0     0          0         0   980595    5759    0    0    0     0       0          0
vif51.2:  731750    2936    0    0    0     0          0         0  3266848   23181    0    0    0     0       0          0
vif51.3:  730932    2917    0    0    0     0          0         0  3266670   23184    0    1    0     0       0          0
```

13900k 的容器中:
```txt
bash-5.0# cat /proc/self/net/dev
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
  eth0:    1830      15    0    0    0     0          0         0        0       0    0    0    0     0       0          0
```

具体代码就不看了，就是 namespace 下可以看到的 network device 吧



## /proc/$pid/fd 中的 socket 可以被找到详细信息的
<!-- 46fcf88e-932f-4c61-bf40-20efaa09a47c -->

```txt
🧀  l
Permissions Size User     Date Modified Name
lrwx------     - martins3 28 Sep 16:15   0 -> /dev/pts/6
lrwx------     - martins3 28 Sep 16:15   1 -> /dev/pts/6
lrwx------     - martins3 28 Sep 16:15   2 -> /dev/pts/6
lrwx------     - martins3 28 Sep 16:15   3 -> socket:[695550]
lrwx------     - martins3 28 Sep 16:15   4 -> anon_inode:[eventfd]
l-wx------     - martins3 28 Sep 16:15   5 -> /home/martins3/data/hack/vm/base/s/pid
lrwx------     - martins3 28 Sep 16:15   6 -> anon_inode:[signalfd]
lrwx------     - martins3 28 Sep 16:15   7 -> anon_inode:[eventfd]
lrwx------     - martins3 28 Sep 16:15   8 -> anon_inode:[eventfd]
lrwx------     - martins3 28 Sep 16:15   9 -> socket:[695551]
lrwx------     - martins3 28 Sep 16:15   10 -> socket:[695552]
lrwx------     - martins3 28 Sep 16:15   11 -> socket:[695553]
lrwx------     - martins3 28 Sep 16:15   12 -> socket:[695559]
```

grep -l 695559 /proc/net/{tcp,udp,tcp6,udp6,unix}

/proc/net/tcp 中内容

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
