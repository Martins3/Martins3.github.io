# neighbour

https://man7.org/linux/man-pages/man8/ip-neighbour.8.html

>       The ip neigh command manipulates neighbour objects that establish
>       bindings between protocol addresses and link layer addresses for
>       hosts sharing the same link.  Neighbour entries are organized
>       into tables. The IPv4 neighbour table is also known by another
>       name - the ARP table.

- net/core/neighbour.c
- net/ipv4/arp.c

neighbour.c 是 arp.c 的基础。

但是 neighbour 也是 appletalk 的基础: https://developer.apple.com/library/archive/documentation/mac/pdf/Networking/Introduction_to_AppleTalk.pdf 的入口之一。

虚拟机中
```txt
10.0.2.3 dev ens4 lladdr 52:55:0a:00:02:03 STALE
10.0.0.109 dev ens5 INCOMPLETE
10.0.2.2 dev ens4 lladdr 52:55:0a:00:02:02 DELAY
fe80::2 dev ens4 lladdr 52:56:00:00:00:02 router STALE
```

13900k
```txt
🤒  ip neigh
10.0.0.4 dev br-in lladdr 74:5d:22:40:f8:94 STALE
10.0.105.188 dev br-in FAILED
192.168.8.1 dev wlo1 lladdr 94:04:9c:d1:26:3a REACHABLE
10.0.73.0 dev br-in lladdr 52:54:00:00:02:49 STALE
10.0.0.16 dev br-in FAILED
10.0.0.1 dev br-in lladdr c8:a3:62:37:f4:7a REACHABLE
10.0.0.107 dev br-in lladdr 52:54:00:00:03:32 STALE
10.0.50.0 dev br-in FAILED
10.0.5.0 dev br-in lladdr 52:54:00:00:02:05 STALE
10.0.0.108 dev br-in lladdr 52:54:00:00:02:32 STALE
10.0.55.0 dev br-in lladdr 52:54:00:00:02:37 STALE
```

mac 中，难道只是因为没有设置密码，就多了这么多的 neighbour ?
```txt
🧀  ip neigh
192.168.11.3 dev wlp1s0f0 lladdr 70:a8:d3:66:73:bc STALE
192.168.11.192 dev wlp1s0f0 lladdr 08:6a:c5:1a:72:96 STALE
10.0.2.15 dev br-in lladdr 52:54:00:00:02:33 STALE
10.0.60.0 dev br-in lladdr 52:54:00:00:02:3c STALE
192.168.10.125 dev wlp1s0f0 lladdr 32:b7:11:ba:c1:bf STALE
192.168.10.43 dev wlp1s0f0 FAILED
10.0.0.2 dev br-in lladdr 00:e0:4c:68:0c:0c REACHABLE
192.168.10.122 dev wlp1s0f0 FAILED
10.0.73.0 dev br-in lladdr 52:54:00:00:02:49 STALE
192.168.10.89 dev wlp1s0f0 FAILED
192.168.9.125 dev wlp1s0f0 lladdr f8:af:05:08:2f:b8 STALE
192.168.11.243 dev wlp1s0f0 lladdr ac:82:47:d7:10:90 STALE
10.0.0.108 dev br-in lladdr 52:54:00:00:02:32 STALE
192.168.10.216 dev wlp1s0f0 FAILED
192.168.8.1 dev wlp1s0f0 lladdr 94:04:9c:d1:26:3a DELAY
10.0.51.0 dev br-in lladdr 52:54:00:00:02:33 STALE
192.168.10.191 dev wlp1s0f0 lladdr 82:a1:9b:c4:a8:db STALE
fe80::2d73:47a8:338a:d1d8 dev br-in lladdr 52:54:00:00:02:49 STALE
```


## 原来 arp 是基于 neighbour 的
```txt
- sysvec_apic_timer_interrupt
  - instr_sysvec_apic_timer_interrupt
    - irq_exit_rcu
      - __irq_exit_rcu
        - invoke_softirq
          - __do_softirq
            - handle_softirqs
              - run_timer_softirq
                - run_timer_base
                  - __run_timer_base
                    - __run_timers
                      - expire_timers
                        - call_timer_fn
                          - neigh_timer_handler
                            - neigh_invalidate
                                - dst_link_failure
                                  - ipv4_link_failure
                                    - ipv4_send_dest_unreach
                                      - __icmp_send
                                        - ip_push_pending_frames
                                          - ip_send_skb
                                            - ip_local_out
                                              - dst_output
                                                - ip_output
                                                  - okfn=<optimized
                                                    - NF_HOOK_COND
                                                      - ip_finish_output
                                                        - cgroup_bpf_sock_enabled
```

## 所以什么是 rarp

### net/core/neighbour.c
- `neigh_direct_output`

进一步的分析参考:
https://github.com/liexusong/linux-source-code-analyze/blob/master/arp-neighbour.md

arp 协议


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
