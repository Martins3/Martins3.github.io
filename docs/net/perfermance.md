# 网络性能

将其中的整理下:
https://l8liliang.github.io/2021/08/19/linux-network-performance.html#performance-tuning

## https://www.kernel.org/doc/Documentation/networking/ip-sysctl.txt

## https://github.com/leandromoreira/linux-network-performance-parameters

- [ ] https://blog.packagecloud.io/illustrated-guide-monitoring-tuning-linux-networking-stack-receiving-data/
- [ ] https://blog.cloudflare.com/how-to-achieve-low-latency/
  - 这个作者很厉害，还存在大量的 blog : https://blog.cloudflare.com/author/marek-majkowski/

- [ ] NAPI will poll data from the receive ring buffer until `netdev_budget_usecs` timeout or `netdev_budget` and `dev_weight` packets

- Linux will pass the skb to the kernel stack (`netif_receive_skb`)
  - 如此精确的描述

- soft IRQ (`NET_TX_SOFTIRQ`) after `tx-usecs` timeout or `tx-frames`
  - [ ] 我感觉这个，有趣的哇

## xps

介绍各种高级技术:

https://docs.kernel.org/networking/scaling.html

## 如何实现降低 latency
- https://blog.cloudflare.com/how-to-achieve-low-latency/
- https://news.ycombinator.com/item?id=9805412

onload : 有趣的
https://docs.xilinx.com/r/en-US/ug1586-onload-user/Onload-and-NIC-Partitioning
  - https://docs.xilinx.com/r/en-US/ug1586-onload-user/Pre-Test-Configuration?tocId=jsfcwmJFWkO18lAcyNAZHQ

- https://github.com/penberg/awesome-low-latency

## onload 基本知识


## iperf3 的各种测试
标准模式，vhost-net ，虚拟机无额外配置，host 中 iperf3 -s

### [ ] 虚拟机走自己的 ovs bridege 之后

13900k 中的测试:

似乎是，不太记得了，到时候复现下。
```txt
🧀  iperf3 -s
-----------------------------------------------------------
Server listening on 5201 (test #1)
-----------------------------------------------------------
Accepted connection from 10.0.0.2, port 34976
[  5] local 10.0.0.2 port 5201 connected to 10.0.0.2 port 34988
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-1.00   sec   338 MBytes  2.83 Gbits/sec
[  5]   1.00-2.00   sec   350 MBytes  2.93 Gbits/sec
[  5]   2.00-3.00   sec   350 MBytes  2.93 Gbits/sec
[  5]   3.00-4.00   sec   351 MBytes  2.95 Gbits/sec
[  5]   4.00-5.00   sec   344 MBytes  2.89 Gbits/sec
[  5]   5.00-6.00   sec   352 MBytes  2.96 Gbits/sec
[  5]   6.00-7.00   sec   352 MBytes  2.95 Gbits/sec
[  5]   7.00-8.00   sec   347 MBytes  2.91 Gbits/sec
[  5]   8.00-9.00   sec   351 MBytes  2.94 Gbits/sec
[  5]   9.00-10.00  sec   345 MBytes  2.90 Gbits/sec
[  5]  10.00-10.00  sec   512 KBytes  2.90 Gbits/sec
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-10.00  sec  3.40 GBytes  2.92 Gbits/sec                  receiver
```

正常的:
```txt
Server listening on 5201 (test #4)
-----------------------------------------------------------
Accepted connection from 10.0.88.0, port 49020
[  5] local 10.0.0.2 port 5201 connected to 10.0.88.0 port 49032
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-1.00   sec  11.7 GBytes   101 Gbits/sec
[  5]   1.00-2.00   sec  12.0 GBytes   103 Gbits/sec
[  5]   2.00-3.00   sec  12.1 GBytes   104 Gbits/sec
[  5]   3.00-4.00   sec  12.1 GBytes   104 Gbits/sec
[  5]   3.00-4.00   sec  12.1 GBytes   104 Gbits/sec
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-4.00   sec  55.4 GBytes   119 Gbits/sec                  receiver
iperf3: the client has terminated
-----------------------------------------------------------
Server listening on 5201 (test #5)
-----------------------------------------------------------
```


### asahi m2 中自己 ping 自己

```txt
🧀  iperf3 -s
-----------------------------------------------------------
Server listening on 5201 (test #1)
-----------------------------------------------------------
Accepted connection from 10.0.0.1, port 48556
[  5] local 10.0.0.1 port 5201 connected to 10.0.0.1 port 48568
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-1.00   sec  17.2 GBytes   147 Gbits/sec
[  5]   1.00-2.00   sec  16.6 GBytes   142 Gbits/sec
[  5]   2.00-3.00   sec  17.0 GBytes   146 Gbits/sec
[  5]   3.00-4.00   sec  17.4 GBytes   150 Gbits/sec
[  5]   3.00-4.00   sec  17.4 GBytes   150 Gbits/sec
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-4.00   sec  69.9 GBytes   150 Gbits/sec                  receiver
iperf3: the client has terminated
-----------------------------------------------------------
Server listening on 5201 (test #2)
-----------------------------------------------------------
```

### asahi m2 中 guest iperf3 to host

居然过了 vhost ，性能反而会更好
```txt
➜  ~ iperf3 -c 10.0.0.1 -t 0
Connecting to host 10.0.0.1, port 5201
[  5] local 10.0.73.0 port 58330 connected to 10.0.0.1 port 5201
[ ID] Interval           Transfer     Bitrate         Retr  Cwnd
[  5]   0.00-1.00   sec  19.3 GBytes   166 Gbits/sec    2   1.92 MBytes
[  5]   1.00-2.00   sec  21.5 GBytes   185 Gbits/sec    0   1.94 MBytes
[  5]   2.00-3.00   sec  21.6 GBytes   185 Gbits/sec    0   1.96 MBytes
[  5]   3.00-4.00   sec  21.7 GBytes   186 Gbits/sec    0   1.99 MBytes
[  5]   4.00-5.00   sec  21.6 GBytes   186 Gbits/sec    0   2.07 MBytes
[  5]   5.00-6.00   sec  21.7 GBytes   186 Gbits/sec    0   2.12 MBytes
[  5]   6.00-7.00   sec  21.7 GBytes   186 Gbits/sec    0   2.12 MBytes
[  5]   7.00-8.00   sec  21.7 GBytes   186 Gbits/sec    0   2.12 MBytes
[  5]   8.00-9.00   sec  21.6 GBytes   186 Gbits/sec    0   2.25 MBytes
^C[  5]   9.00-9.32   sec  5.45 GBytes   146 Gbits/sec    0   2.25 MBytes
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate         Retr
[  5]   0.00-9.32   sec   198 GBytes   182 Gbits/sec    2             sender
[  5]   0.00-9.32   sec  0.00 Bytes  0.00 bits/sec                  receiver
```


### 13900k 的 perf 工具
```txt
🧀  iperf3 -s
-----------------------------------------------------------
Server listening on 5201 (test #1)
-----------------------------------------------------------
Accepted connection from 10.0.0.2, port 36052
[  5] local 10.0.0.2 port 5201 connected to 10.0.0.2 port 36056
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-1.00   sec  14.7 GBytes   126 Gbits/sec
[  5]   1.00-2.00   sec  14.1 GBytes   121 Gbits/sec
[  5]   2.00-3.00   sec  14.5 GBytes   124 Gbits/sec
[  5]   3.00-4.00   sec  14.6 GBytes   126 Gbits/sec
[  5]   4.00-5.00   sec  14.6 GBytes   125 Gbits/sec
[  5]   4.00-5.00   sec  14.6 GBytes   125 Gbits/sec
- - - - - - - - - - - - - - - - - - - - - - - - -
[ ID] Interval           Transfer     Bitrate
[  5]   0.00-5.00   sec  80.8 GBytes   139 Gbits/sec                  receiver
```
想不到和 m2 的性能差别这么大。

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
