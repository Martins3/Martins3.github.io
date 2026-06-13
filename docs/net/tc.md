## tc 和 tcp congestion control
<!-- 004a16ff-400f-4527-92b3-b8e975955ab1 -->

这里的 `tc` 指 Linux `traffic control`，不是 TCP 拥塞控制。

这两个东西很容易混淆，但它们是不同层次的机制：

- TCP 拥塞控制
  - 典型源码在 `net/ipv4/`，例如 `tcp_vegas.c`、`tcp_cubic.c`、`tcp_bbr.c`
  - 核心接口是 `struct tcp_congestion_ops`
  - 解决的是 sender 端如何根据 ACK、RTT、丢包去调节 `cwnd`
- Linux `tc`
  - 主体源码在 `net/sched/`
  - 核心对象是 `qdisc`、`class`、`filter`、`action`
  - 解决的是包在网卡收发路径上如何排队、整形、分类、丢弃、重定向

例如下面这段：

```c
struct tcp_congestion_ops tcp_reno = {
	.flags		= TCP_CONG_NON_RESTRICTED,
	.name		= "reno",
	.owner		= THIS_MODULE,
	.ssthresh	= tcp_reno_ssthresh,
	.cong_avoid	= tcp_reno_cong_avoid,
	.undo_cwnd	= tcp_reno_undo_cwnd,
};
```

它属于 TCP 拥塞控制，不属于 `tc`。

如果要看默认 qdisc、`fq_codel`、`pfifo_fast` 一类内容，可以同时参考 [qdisc.md](./qdisc.md)。

所以，我们的确可以注意到，这里是存在 tc cgroup 和 tcp contestion 三个东西，
他们都是控制发包的。在不同的层次。

## 可直接跑的实验

旁边新增了一个脚本：[tc-demo.sh](./tc-demo.sh)。

这个脚本只用 `netns + veth + tc` 搭一个最小拓扑，在物理机上也比较安全，不需要碰真实网卡：

```txt
tc-left(veth-tc-left, 10.88.0.1) <-> tc-right(veth-tc-right, 10.88.0.2)
```

支持这些实验：

- `baseline`
  - 不加任何 `tc` 规则，先确认基线连通
- `delay`
  - 在双向 egress 上挂 `netem delay`
  - 现象：`ping` 的 RTT 明显增大
- `loss`
  - 在一侧 egress 上挂 `netem loss`
  - 现象：`ping` 出现丢包
- `rate`
  - 用 `tbf` 做限速
  - 有 `iperf3` 时会实际跑吞吐；没有就只展示 qdisc 规则
- `filter`
  - 用 `clsact + u32 + action drop` 在 ingress 丢 ICMP
  - 现象：`ping` 失败，但规则和计数器可见

最常用的运行方式：

```bash
# 需要 tc / ip / ping
nix-shell -p iproute2 iputils --run \
  'printf "a\n" | sudo -S env PATH="$PATH" ./net/tc-demo.sh all'

# 如果要看 tbf 的实际吞吐
nix-shell -p iproute2 iputils iperf3 --run \
  'printf "a\n" | sudo -S env PATH="$PATH" ./net/tc-demo.sh rate'
```

脚本默认不会自动清理，方便实验后继续手工观察：

```bash
nix-shell -p iproute2 iputils --run \
  'printf "a\n" | sudo -S env PATH="$PATH" ./net/tc-demo.sh cleanup'
```

## https://wiki.aalto.fi/download/attachments/69901948/TCP-CongestionControlFinal.pdf


## [万字详文：TCP 拥塞控制详解](https://zhuanlan.zhihu.com/p/144273871)

下面介绍几种常用的 RTT 算法。
1. rtt 经典算法 [RFC793]
2. rtt 标准算法（Jacobson / Karels 算法）
  - Linux 的源代码在：tcp_rtt_estimator
3. Karn 算法

> - [ ] 这里，只是找到了其中的一个算法，其他的实现暂时没有看, 但是应该也是很重要的
> - [ ]  文章中间还分析了一些辅助机制

几种快速重传算法:
- SACk
- FACk
- RACK
  - tcp_rack_advance

丢包是网络波动，还是真的出现了拥塞 ?

- [nagle](https://stackoverflow.com/questions/17842406/how-would-one-disable-nagles-algorithm-in-linux) ???

## tc ai 分析

• 如果你问的是 Linux 内核里的 tc（traffic control），主源码基本都在 net/sched/，再加上少量 net/core/、
  include/net/、include/uapi/linux/ 和驱动 offload 代码。

最核心的是这几类：

- 框架层
    - include/net/sch_generic.h:304：struct Qdisc_ops、struct tcf_proto_ops 等核心抽象
    - include/net/act_api.h:113：struct tc_action_ops
    - net/sched/sch_api.c:1581：qdisc/class 的 netlink 控制入口
    - net/sched/cls_api.c:280：classifier/filter 框架与链路管理
    - net/sched/act_api.c:1017：action 框架
    - net/sched/sch_generic.c:415：qdisc 运行与调度核心
- qdisc 实现
    - 文件名模式就是 net/sched/sch_*.c
    - 常见的有 net/sched/sch_ingress.c:334、net/sched/sch_htb.c:1、net/sched/sch_tbf.c:603、net/sched/
      sch_fq_codel.c:722、net/sched/sch_cake.c:3122、net/sched/sch_netem.c:1344
- classifier 实现
    - 文件名模式 net/sched/cls_*.c
    - 常见的有 net/sched/cls_u32.c:1455、net/sched/cls_flower.c:3832、net/sched/cls_bpf.c:682、net/sched/
      cls_matchall.c:389
- action 实现
    - 文件名模式 net/sched/act_*.c
    - 常见的有 net/sched/act_mirred.c:639、net/sched/act_police.c:1、net/sched/act_pedit.c:1、net/sched/
      act_bpf.c:392、net/sched/act_ct.c:1624
- 用户态到内核的协议定义
    - include/uapi/linux/pkt_sched.h:68：qdisc/class handle、TC_H_*
    - include/uapi/linux/pkt_cls.h:8：filter/action 的 netlink attr、TC_ACT_*
    - net/sched/sch_api.c:2486、net/sched/cls_api.c:4084、net/sched/act_api.c:2292：分别对应
      RTM_NEWQDISC、RTM_NEWTFILTER、RTM_NEWACTION
- 真正挂到收发路径的位置
    - net/core/dev.c:4433：ingress 路径 sch_handle_ingress()
    - net/core/dev.c:4497：egress 路径 sch_handle_egress()
- 硬件 offload
    - include/net/pkt_cls.h:595：tc_setup_cb_*
    - include/net/flow_offload.h:588：flow_block_offload / flow_cls_offload
    - 驱动里看 ndo_setup_tc，比如 drivers/net/...

如果你是想快速建立全貌，建议按这个顺序读：

1. net/sched/sch_api.c:1581
2. net/sched/sch_ingress.c:242
3. net/sched/cls_api.c:230
4. 具体 classifier：cls_flower.c 或 cls_u32.c
5. net/sched/act_api.c:1017
6. 具体 action：act_mirred.c / act_police.c / act_bpf.c
7. net/core/dev.c:4433

• tc 的核心作用是 控制网络包怎么排队、怎么发、怎么处理。

更具体地说，它主要做这几件事：

- 流量调度（queueing / scheduling）
    - 决定包先发哪个、后发哪个
    - 例如不同优先级、不同队列、公平排队
- 流量整形与限速（shaping / policing）
    - 控制发送速率，避免瞬时打爆链路
    - 例如限速、突发控制、超速丢包
- 分类与过滤（classification / filtering）
    - 按五元组、端口、协议、VLAN、BPF 等规则匹配报文
    - 把不同流量送到不同 class / queue / action
- 动作处理（actions）
    - 对匹配到的包执行操作
    - 例如 drop、redirect、mirror、pedit、ct、bpf
- 入口/出口处理
    - egress：发包前排队和整形，这是最典型的 tc 用途
    - ingress：收包时过滤、丢弃、重定向，常用于策略控制
- 网络仿真
    - 用 netem 模拟延迟、抖动、丢包、乱序，常用于测试

可以把它粗略理解成：

- qdisc：决定“怎么排队/发包”
- class：把流量分层分类
- filter：决定“这个包属于哪一类”
- action：决定“这个包要怎么处理”

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
