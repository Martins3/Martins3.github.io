# snmp

## 操作一下
检查下 /proc/net 的两个文件:
```txt
🧀  cat snmp
Ip: Forwarding DefaultTTL InReceives InHdrErrors InAddrErrors ForwDatagrams InUnknownProtos InDiscards InDelivers OutRequests OutDiscards OutNoRoutes ReasmTimeout ReasmReqds R
easmOKs ReasmFails FragOKs FragFails FragCreates OutTransmits
Ip: 1 64 93312031 18 0 0 0 0 91627381 66278051 964 115708 0 2 1 0 0 0 0 66278051
Icmp: InMsgs InErrors InCsumErrors InDestUnreachs InTimeExcds InParmProbs InSrcQuenchs InRedirects InEchos InEchoReps InTimestamps InTimestampReps InAddrMasks InAddrMaskReps O
utMsgs OutErrors OutRateLimitGlobal OutRateLimitHost OutDestUnreachs OutTimeExcds OutParmProbs OutSrcQuenchs OutRedirects OutEchos OutEchoReps OutTimestamps OutTimestampReps O
utAddrMasks OutAddrMaskReps
Icmp: 17913 5931 0 17537 0 0 0 0 342 34 0 0 0 0 35254 0 402 3156 34742 0 0 0 0 170 342 0 0 0 0
IcmpMsg: InType0 InType3 InType8 OutType0 OutType3 OutType8
IcmpMsg: 34 17537 342 342 34742 170
Tcp: RtoAlgorithm RtoMin RtoMax MaxConn ActiveOpens PassiveOpens AttemptFails EstabResets CurrEstab InSegs OutSegs RetransSegs InErrs OutRsts InCsumErrors
Tcp: 1 200 120000 -1 564159 148221 3693 19921 112 78525027 103213120 313403 587 67769 0
Udp: InDatagrams NoPorts InErrors OutDatagrams RcvbufErrors SndbufErrors InCsumErrors IgnoredMulti MemErrors
Udp: 72574711 32340 3457 1998611 3457 2 0 0 0
UdpLite: InDatagrams NoPorts InErrors OutDatagrams RcvbufErrors SndbufErrors InCsumErrors IgnoredMulti MemErrors
UdpLite: 0 0 0 0 0 0 0 0 0

🧀  cat snmp6
Ip6InReceives                           331662
Ip6InHdrErrors                          0
Ip6InTooBigErrors                       0
Ip6InNoRoutes                           0
Ip6InAddrErrors                         0
Ip6InUnknownProtos                      0
Ip6InTruncatedPkts                      0
Ip6InDiscards                           0
Ip6InDelivers                           113012
Ip6OutForwDatagrams                     0
Ip6OutRequests                          126113
Ip6OutDiscards                          308
Ip6OutNoRoutes                          596367
Ip6ReasmTimeout                         0
Ip6ReasmReqds                           0
Ip6ReasmOKs                             0
Ip6ReasmFails                           0
Ip6FragOKs                              0
Ip6FragFails                            0
Ip6FragCreates                          0
Ip6InMcastPkts                          67044
Ip6OutMcastPkts                         13101
Ip6InOctets                             568655172
Ip6OutOctets                            548643176
Ip6InMcastOctets                        7069630
Ip6OutMcastOctets                       868576
Ip6InBcastOctets                        0
Ip6OutBcastOctets                       0
Ip6InNoECTPkts                          331662
Ip6InECT1Pkts                           0
Ip6InECT0Pkts                           0
Ip6InCEPkts                             0
Ip6OutTransmits                         126113
Icmp6InMsgs                             88
Icmp6InErrors                           0
Icmp6OutMsgs                            8792
Icmp6OutErrors                          0
Icmp6InCsumErrors                       0
Icmp6OutRateLimitHost                   0
Icmp6InDestUnreachs                     88
Icmp6InPktTooBigs                       0
Icmp6InTimeExcds                        0
Icmp6InParmProblems                     0
Icmp6InEchos                            0
Icmp6InEchoReplies                      0
Icmp6InGroupMembQueries                 0
Icmp6InGroupMembResponses               0
Icmp6InGroupMembReductions              0
Icmp6InRouterSolicits                   0
Icmp6InRouterAdvertisements             0
Icmp6InNeighborSolicits                 0
Icmp6InNeighborAdvertisements           0
Icmp6InRedirects                        0
Icmp6InMLDv2Reports                     0
Icmp6OutDestUnreachs                    88
Icmp6OutPktTooBigs                      0
Icmp6OutTimeExcds                       0
Icmp6OutParmProblems                    0
Icmp6OutEchos                           0
Icmp6OutEchoReplies                     0
Icmp6OutGroupMembQueries                0
Icmp6OutGroupMembResponses              0
Icmp6OutGroupMembReductions             0
Icmp6OutRouterSolicits                  1880
Icmp6OutRouterAdvertisements            0
Icmp6OutNeighborSolicits                22
Icmp6OutNeighborAdvertisements          0
Icmp6OutRedirects                       0
Icmp6OutMLDv2Reports                    6802
Icmp6InType1                            88
Icmp6OutType1                           88
Icmp6OutType133                         1880
Icmp6OutType135                         22
Icmp6OutType143                         6802
Udp6InDatagrams                         0
Udp6NoPorts                             88
Udp6InErrors                            0
Udp6OutDatagrams                        88
Udp6RcvbufErrors                        0
Udp6SndbufErrors                        0
Udp6InCsumErrors                        0
Udp6IgnoredMulti                        0
Udp6MemErrors                           0
UdpLite6InDatagrams                     0
UdpLite6NoPorts                         0
UdpLite6InErrors                        0
UdpLite6OutDatagrams                    0
UdpLite6RcvbufErrors                    0
UdpLite6SndbufErrors                    0
UdpLite6InCsumErrors                    0
UdpLite6MemErrors                       0
```

## 基本文档
- Documentation/networking/snmp_counter.rst

- https://info.support.huawei.com/info-finder/encyclopedia/zh/SNMP.html

- [snmp wiki](https://zh.wikipedia.org/wiki/%E7%AE%80%E5%8D%95%E7%BD%91%E7%BB%9C%E7%AE%A1%E7%90%86%E5%8D%8F%E8%AE%AE)
  - 讲的很清楚了，也就是一个用户态的包，然后靠 kernel 的信息导出

snmp 还可以限制上网，应该是靠这个实现的:
net/netfilter/nf_conntrack_snmp.c

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
