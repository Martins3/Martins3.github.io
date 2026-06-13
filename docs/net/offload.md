# network offload

## 内核源码
net/ipv4/tcp_offload.c
net/ipv4/udp_offload.c
net/ipv4/gre_offload.c
net/ipv4/esp4_offload.c

net/core/flow_offload.c

Documentation/networking/checksum-offloads.rst

Documentation/networking/segmentation-offloads.rst

## GSO 是做啥的?

## UFO : UDP Fragmentation Offload

- [一个 UFO 引发的惨案](https://www.ichenfu.com/2024/04/01/ufo-feature-caused-network-failure/)

- https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/commit/?id=960b360ca7463921c1a6b72e7066a706d6406223

## USO

- 那么 USO 是想要加入就加入吗?
  - https://lore.kernel.org/netdev/20220125084702.3636253-1-andrew@daynix.com/

原来可以可以如此设计啊:
```xml
<interface type='vhostuser'>
  <mac address='{ {.MACAddress}}'/>
  <source type='unix' path='{ {.VhostPath}}' mode='server'/>
  <model type='virtio'/>
  <driver queues='16' rx_queue_size='1024' tx_queue_size='1024'>
    <host tso4='off' tso6='off' ufo='off' ecn='off' mrg_rxbuf='off'/>
    <guest tso4='off' tso6='off' ufo='off' ecn='off'/>
  </driver>
</interface>
```

> Network interface cards (NIC) with receive (RX) acceleration (GRO, LRO, TPA, etc) may suffer from bad performance.

## 看看这个
https://michael.mulqueen.me.uk/2018/08/disable-offloading-netplan-ubuntu/

其实价值不大:

> If this didn’t solve the problem, it may be worth looking at some of the features that can be enabled/disabled:
>
> rx - receive (RX) checksumming
> tx - transmit (TX) checksumming
> sg - scatter-gather
> tso - TCP segmentation offload
> ufo - UDP fragmentation offload
> gso - generic segmentation offload
> gro - generic receive offload
> lro - large receive offload
> rxvlan - receive (RX) VLAN acceleration
> txvlan - transmit (TX) VLAN acceleration
> ntuple - receive (RX) ntuple filters and actions
> rxhash - receive hashing offload

## 检查 nic 支持的 offload 类型
ethtool -k

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
