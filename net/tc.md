# 流量控制

- https://tldp.org/HOWTO/Traffic-Control-HOWTO/

## 源码分布位置
- `linux/net/sched/cls_*.c`
  - `struct tcf_proto_ops`
- `linux/net/sched/sch_*.c`
  - `struct Qdisc_ops`
  - `struct Qdisc_class_ops`
- `linux/net/sched/act_*.c`
  - `struct tc_action_ops`
