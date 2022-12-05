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
  o

## xps

https://www.kernel.org/doc/html/latest/networking/scaling.html
