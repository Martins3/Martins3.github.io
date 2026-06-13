linkwatch_event 搞不清楚是什么

| File                  | Lines | Code | Comments | Blanks | Comments |
|-----------------------|-------|------|----------|--------|----------|
| filter.c              | 11947 | 9545 | 792      | 1610   | bpf 相关 |
| dev.c                 | 11660 | 7760 | 2093     | 1807   |
| skbuff.c              | 6990  | 4535 | 1379     | 1076   |
| rtnetlink.c           | 6688  | 5316 | 315      | 1057   |
| sock.c                | 4215  | 3043 | 537      | 635    |
| pktgen.c              | 4086  | 3088 | 352      | 646    |
| neighbour.c           | 3894  | 3132 | 192      | 570    |
| net-sysfs.c           | 2108  | 1610 | 106      | 392    |
| flow_dissector.c      | 2053  | 1610 | 136      | 307    |
| drop_monitor.c        | 1785  | 1350 | 83       | 352    |
| sock_map.c            | 1715  | 1405 | 45       | 265    |
| net_namespace.c       | 1400  | 1014 | 170      | 216    |
| fib_rules.c           | 1319  | 1056 | 26       | 237    |
| skmsg.c               | 1253  | 1023 | 61       | 169    |
| dev_addr_lists.c      | 1050  | 611  | 307      | 132    |
| page_pool.c           | 954   | 612  | 176      | 166    |
| bpf_sk_storage.c      | 930   | 724  | 54       | 152    |
| datagram.c            | 923   | 597  | 206      | 120    |
| netpoll.c             | 867   | 670  | 51       | 146    |
| dev_ioctl.c           | 817   | 557  | 128      | 132    |
| xdp.c                 | 804   | 575  | 84       | 145    |
| gro.c                 | 767   | 552  | 84       | 131    |
| sysctl_net_core.c     | 753   | 660  | 19       | 74     |
| sock_reuseport.c      | 749   | 501  | 110      | 138    |
| lwt_bpf.c             | 657   | 514  | 29       | 114    |
| flow_offload.c        | 638   | 525  | 7        | 106    |
| utils.c               | 486   | 361  | 77       | 48     |
| gen_stats.c           | 485   | 297  | 137      | 51     |
| lwtunnel.c            | 427   | 332  | 14       | 81     |
| net-procfs.c          | 415   | 339  | 13       | 63     |
| selftests.c           | 410   | 327  | 12       | 71     |
| scm.c                 | 373   | 288  | 26       | 59     |
| sock_diag.c           | 343   | 279  | 3        | 61     |
| dst.c                 | 340   | 265  | 22       | 53     |
| failover.c            | 315   | 212  | 42       | 61     |
| netprio_cgroup.c      | 295   | 197  | 49       | 49     |
| link_watch.c          | 294   | 187  | 44       | 63     |
| gen_estimator.c       | 278   | 171  | 71       | 36     |
| gso_test.c            | 274   | 211  | 19       | 44     |
| gso.c                 | 273   | 131  | 106      | 36     |
| dev_addr_lists_test.c | 237   | 183  | 8        | 46     |
| ptp_classifier.c      | 228   | 116  | 96       | 16     |
| stream.c              | 220   | 141  | 52       | 27     |
| secure_seq.c          | 200   | 161  | 19       | 20     |
| fib_notifier.c        | 199   | 162  | 0        | 37     |
| dst_cache.c           | 183   | 141  | 7        | 35     |
| netdev-genl.c         | 175   | 135  | 1        | 39     |
| of_net.c              | 172   | 91   | 58       | 23     |
| netclassid_cgroup.c   | 152   | 110  | 14       | 28     |
| gro_cells.c           | 138   | 103  | 11       | 24     |
| request_sock.c        | 132   | 44   | 77       | 11     |
| tso.c                 | 89    | 70   | 5        | 14     |
| hwbm.c                | 85    | 62   | 9        | 14     |
| timestamping.c        | 71    | 51   | 6        | 14     |
| net-traces.c          | 68    | 52   | 6        | 10     |
| netevent.c            | 63    | 20   | 36       | 7      |
| netdev-genl-gen.c     | 48    | 35   | 6        | 7      |


> [!NOTE]
> 参考 Deepseeek ，有待验证

- consume_skb : 释放掉 skb ，主要场景为:
- 数据包成功交付到上层协议
- 数据包被明确丢弃
  - 防火墙规则丢弃数据包（如iptables规则匹配为DROP）。
  - 路由失败（如无可用路由或TTL过期）。
  - 协议栈处理错误（如校验和失败、无效包头）。
递减skb的引用计数，当计数归零时，释放其占用的内存及相关资源

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
