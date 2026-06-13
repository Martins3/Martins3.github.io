# qdisc
- linux/net/sched/ : 例如 RED ，主要是队列的管理

需要指出的是，这个东西 queue 的 scheduler
和 tcp 的 congestion control 不是一个东西

queue disc 是给网卡使用的。

```sh
cat /proc/sys/net/ipv4/tcp_congestion_control
```

## 基本的观察
通过 ip a 查看 queue disc

物理机中中的查询的结果:
```txt
4: tailscale0: <POINTOPOINT,MULTICAST,NOARP,UP,LOWER_UP> mtu 1280 qdisc fq_codel state UNKNOWN group default qlen 500
    link/none
    inet 100.93.105.2/32 scope global tailscale0
       valid_lft forever preferred_lft forever
    inet6 fd7a:115c:a1e0:ab12:4843:cd96:625d:6902/128 scope global
       valid_lft forever preferred_lft forever
    inet6 fe80::be3b:cc33:7faa:4100/64 scope link stable-privacy
```

```txt
2: enp5s0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether a0:36:bc:ad:c2:ce brd ff:ff:ff:ff:ff:ff
    inet 10.0.0.1/24 brd 10.0.0.255 scope global noprefixroute enp5s0
       valid_lft forever preferred_lft forever
    inet6 fe80::ed4b:6a96:9898:1585/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
```

虚拟机中
```txt
4: ens5: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
    link/ether 52:54:00:12:34:56 brd ff:ff:ff:ff:ff:ff
    inet 10.0.2.15/24 brd 10.0.2.255 scope global dynamic noprefixroute ens5
       valid_lft 82110sec preferred_lft 82110sec
    inet6 fec0::f27:e801:b5bc:6fb/64 scope site dynamic noprefixroute
       valid_lft 86029sec preferred_lft 14029sec
    inet6 fe80::cd95:50e6:e06e:5d9b/64 scope link noprefixroute
       valid_lft forever preferred_lft forever
5: ens7: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc mq state UP group default qlen 1000
    link/ether a0:36:bc:ad:c2:ce brd ff:ff:ff:ff:ff:ff
    inet 10.0.0.3/24 scope global ens7
       valid_lft forever preferred_lft forever
```


使用 tc 来玩一玩吧:
```txt
tc qdisc show

qdisc noqueue 0: dev lo root refcnt 2
qdisc mq 0: dev enp4s0 root
qdisc fq_codel 0: dev enp4s0 parent :4 limit 10240p flows 1024 quantum 1514 target 5ms interval 100ms memory_limit 32Mb ecn drop_batch 64
qdisc fq_codel 0: dev enp4s0 parent :3 limit 10240p flows 1024 quantum 1514 target 5ms interval 100ms memory_limit 32Mb ecn drop_batch 64
qdisc fq_codel 0: dev enp4s0 parent :2 limit 10240p flows 1024 quantum 1514 target 5ms interval 100ms memory_limit 32Mb ecn drop_batch 64
qdisc fq_codel 0: dev enp4s0 parent :1 limit 10240p flows 1024 quantum 1514 target 5ms interval 100ms memory_limit 32Mb ecn drop_batch 64
qdisc noqueue 0: dev wlo1 root refcnt 2
qdisc fq_codel 0: dev tailscale0 root refcnt 2 limit 10240p flows 1024 quantum 1280 target 5ms interval 100ms memory_limit 32Mb ecn drop_batch 64
qdisc noqueue 0: dev docker0 root refcnt 2
qdisc noqueue 0: dev br-a7c25b8e6a73 root refcnt 2
qdisc noqueue 0: dev br-1ef4b93ecdc9 root refcnt 2
qdisc noqueue 0: dev vethbb6ea4c root refcnt 2
qdisc noqueue 0: dev vethff74328 root refcnt 2
qdisc noqueue 0: dev vethdb753db root refcnt 2
qdisc noqueue 0: dev veth66b141c root refcnt 2
qdisc noqueue 0: dev vethe499c95 root refcnt 2
```

似乎大多数时候是采用这个:
```txt
CONFIG_NET_SCH_FQ_CODEL=m
```
但是不知道为什么 wl01 是 noqueue 的。

https://superuser.com/questions/708391/how-to-learn-the-current-network-scheduler-used-in-my-linux-machine
说一般是 pfifo_fast

qemu 所有的 NET_SCH 都不打开的时候:
```txt
➜  ~ tc qdisc show

qdisc noqueue 0: dev lo root refcnt 2
qdisc pfifo_fast 0: dev enp0s5 root refcnt 2 bands 3 priomap 1 2 2 2 1 2 0 0 1 1 1 1 1 1 1 1
```
在内核选项中打开 CONFIG_NET_SCH_FQ_CODEL=y 之后
```txt
qdisc noqueue 0: dev lo root refcnt 2
qdisc fq_codel 0: dev enp0s5 root refcnt 2 limit 10240p flows 1024 quantum 1514 target 5ms interval 100ms memory_limit 32Mb ecn drop_batch 64
```
可以调查下这里的 scheduler 的选择标准是啥。

## 简单跑点 backtrace 出来

发送过程:
```txt
#0  fq_codel_dequeue (sch=0xffff88810428a800) at net/sched/sch_fq_codel.c:282
#1  0xffffffff81e5121c in dequeue_skb (packets=<synthetic pointer>, validate=<synthetic pointer>, q=0xffff88810428a800)
    at net/sched/sch_generic.c:292
#2  qdisc_restart (packets=<synthetic pointer>, q=0xffff88810428a800) at net/sched/sch_generic.c:397
#3  __qdisc_run (q=q@entry=0xffff88810428a800) at net/sched/sch_generic.c:415
#4  0xffffffff81df225d in __dev_xmit_skb (txq=0xffff8881052d4400, dev=0xffff88810d74e000, q=0xffff88810428a800,
    skb=0xffff888104562700) at net/core/dev.c:3815
#5  __dev_queue_xmit (skb=skb@entry=0xffff888104562700, sb_dev=sb_dev@entry=0x0 <fixed_percpu_data>) at net/core/dev.c:4169
#6  0xffffffff81ebbc61 in dev_queue_xmit (skb=0xffff888104562700) at ./include/linux/netdevice.h:3088
#7  neigh_hh_output (skb=<optimized out>, hh=<optimized out>) at ./include/net/neighbour.h:528
#8  neigh_output (skip_cache=<optimized out>, skb=<optimized out>, n=<optimized out>) at ./include/net/neighbour.h:542
#9  ip_finish_output2 (net=<optimized out>, sk=<optimized out>, skb=0xffff888104562700) at net/ipv4/ip_output.c:230
#10 0xffffffff81ebf3d6 in dst_output (skb=0xffff888104562700, sk=0xffff88810f394c80, net=0xffffffff83b012c0 <init_net>)
    at ./include/net/dst.h:458
#11 ip_local_out (skb=0xffff888104562700, sk=0xffff88810f394c80, net=0xffffffff83b012c0 <init_net>)
    at net/ipv4/ip_output.c:127
#12 ip_send_skb (net=0xffffffff83b012c0 <init_net>, skb=skb@entry=0xffff888104562700) at net/ipv4/ip_output.c:1486
#13 0xffffffff81ef7aee in udp_send_skb (skb=0xffff888104562700, fl4=fl4@entry=0xffff88810f394ff0,
    cork=cork@entry=0xffffc900011d7b98) at ./include/net/net_namespace.h:383
#14 0xffffffff81efccb5 in udp_sendmsg (sk=0xffff88810f394c80, msg=<optimized out>, len=<optimized out>)
    at net/ipv4/udp.c:1265
#15 0xffffffff81dc193f in sock_sendmsg_nosec (msg=0xffffc900011d7ea8, sock=0xffff888110ddde40) at net/socket.c:728
#16 sock_sendmsg (sock=0xffff888110ddde40, msg=0xffffc900011d7ea8) at net/socket.c:748
#17 0xffffffff81dc1b9d in ____sys_sendmsg (sock=sock@entry=0xffff888110ddde40, msg_sys=msg_sys@entry=0xffffc900011d7ea8,
    flags=flags@entry=0, used_address=used_address@entry=0x0 <fixed_percpu_data>,
    allowed_msghdr_flags=allowed_msghdr_flags@entry=0) at net/socket.c:2494
#18 0xffffffff81dc4b7a in ___sys_sendmsg (sock=sock@entry=0xffff888110ddde40, msg=msg@entry=0x7fff754e33e0,
    msg_sys=msg_sys@entry=0xffffc900011d7ea8, flags=flags@entry=0, used_address=used_address@entry=0x0 <fixed_percpu_data>,
    allowed_msghdr_flags=allowed_msghdr_flags@entry=0) at net/socket.c:2548
#19 0xffffffff81dc4cca in __sys_sendmsg (fd=<optimized out>, msg=0x7fff754e33e0, flags=0, forbid_cmsg_compat=<optimized out>)
    at net/socket.c:2577
#20 0xffffffff82197d9b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900011d7f58) at arch/x86/entry/common.c:50
#21 do_syscall_64 (regs=0xffffc900011d7f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#22 0xffffffff822000ef in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

不知道为什么对称的函数总是无法被调用 fq_codel_enqueue，虚拟机和物理机中都尝试过。


## 默认 scheduler 是 kernel
```txt
#0  set_default_qdisc (table=0xffffffff82ea3360 <net_core_table+1216>, write=1, buffer=0xffff888102559550, lenp=0xffffc900005f3e28, ppos=0xffffc900005f3ea0)
    at net/core/sysctl_net_core.c:275
#1  0xffffffff814ed8f2 in proc_sys_call_handler (iocb=0xffffc900005f3e98, iter=0xffffc900005f3e70, write=<optimized out>) at fs/proc/proc_sysctl.c:598
#2  0xffffffff81442bfa in call_write_iter (iter=0x1 <fixed_percpu_data+1>, kio=0xffffc900005f3e98, file=0xffff888103f87900) at ./include/linux/fs.h:1877
#3  new_sync_write (ppos=0xffffc900005f3f08, len=9, buf=0x7fff9978ee20 "fq_codel\n", filp=0xffff888103f87900) at fs/read_write.c:491
#4  vfs_write (file=file@entry=0xffff888103f87900, buf=buf@entry=0x7fff9978ee20 "fq_codel\n", count=count@entry=9, pos=pos@entry=0xffffc900005f3f08) at fs/read_write.c:584
#5  0xffffffff8144317f in ksys_write (fd=<optimized out>, buf=0x7fff9978ee20 "fq_codel\n", count=9) at fs/read_write.c:637
#6  0xffffffff82197d9b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900005f3f58) at arch/x86/entry/common.c:50
#7  do_syscall_64 (regs=0xffffc900005f3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#8  0xffffffff822000ef in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## 如何使用
https://tldp.org/HOWTO/Traffic-Control-HOWTO/

## 源码分布位置
- `linux/net/sched/cls_*.c`
  - `struct tcf_proto_ops`
- `linux/net/sched/sch_*.c`
  - `struct Qdisc_ops`
  - `struct Qdisc_class_ops`
- `linux/net/sched/act_*.c`
  - `struct tc_action_ops`

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
