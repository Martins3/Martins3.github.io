# iscsi

- https://linux.vbird.org/linux_server/centos6/0460iscsi.php

## 基本原理
- https://wiki.nix-pro.com/view/ISCSI

- target : server

## 使用

目前找到的最好操作方法:
- https://www.cnblogs.com/xiangsikai/p/10876534.html
  - 服务器端的配置，无需步骤 8 ，同样可以正常运行。

### server
```txt
# 进入指定目录下
/> cd /backstores/block
# 创建共享设备，命名为 “disk0”
/backstores/block> create disk0 /dev/sda

# 进入指定目录
cd /iscsi
# 创建新的 target 名称
/iscsi> create

# 进入指定目录
/iscsi> cd /iscsi/iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.037746bdcf2d/tpg1/luns
# 创建共享设备到 target
/iscsi/iqn.20...f2d/tpg1/luns> create /backstores/block/disk0

# 进入指定目录
/iscsi/iqn.20...f2d/tpg1/luns> cd /iscsi/iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.037746bdcf2d/tpg1/acls/
# 创建存入客户端可访问名称
/iscsi/iqn.20...f2d/tpg1/acls> create iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.037746bdcf2d:xsk
```

### clinet 操作
```txt
iscsiadm -m discovery -t st -p 127.0.0.1
iscsiadm -m node -T iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.037746bdcf2d -p 127.0.0.1 --login
```

```sh
vim /etc/iscsi/initiatorname.iscsi
systemctl restart iscsid
systemctl enable iscsid
```

原来是
```txt
InitiatorName=iqn.2012-01.com.openeuler:ee71c1a1eeb
```
将原来的数值替换为:

```txt
InitiatorName=iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.67ec603fab9b:xsk
```

开机之后一次:
```sh
cp saveconfig.json /etc/target/saveconfig.json
targetcli restoreconfig
iscsiadm -m node -T iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.44a67a34a490 -p 127.0.0.1 -l
```

# https://github.com/open-iscsi/open-iscsi

## 常见命令
- iscsiadm -m session : 枚举所有的 session 的内容

## 代码的简单分析

`iscsi_target_transport` 只是 `iscsit_transport` 的一种实现，也就是 TCP/IP 来实现的。

例如 iscsit_get_rx_pdu，可以找到如下三个:
```c
drivers/infiniband/ulp/isert/ib_isert.c
2603:   .iscsit_get_rx_pdu      = isert_get_rx_pdu,

drivers/target/iscsi/cxgbit/cxgbit_main.c
684:    .iscsit_get_rx_pdu      = cxgbit_get_rx_pdu,

drivers/target/iscsi/iscsi_target.c
679:    .iscsit_get_rx_pdu      = iscsit_get_rx_pdu,
```

2. 接受数据
```txt
#0  __send_ipi_mask (mask=0xffffffff82427108 <cpu_bit_bitmap+40>, vector=<optimized out>) at arch/x86/kernel/kvm.c:549
#1  0xffffffff811f3317 in __smp_call_single_queue (node=0xffff8880139223d8, cpu=4) at kernel/smp.c:498
#2  generic_exec_single (cpu=cpu@entry=4, csd=csd@entry=0xffff8880139223d8) at kernel/smp.c:531
#3  0xffffffff811f366f in smp_call_function_single_async (cpu=cpu@entry=4, csd=csd@entry=0xffff8880139223d8) at kernel/smp.c:821
#4  0xffffffff816cb2ba in blk_mq_complete_send_ipi (rq=0xffff888013922300) at block/blk-mq.c:1185
#5  blk_mq_complete_request_remote (rq=0xffff888013922300) at block/blk-mq.c:1214
#6  0xffffffff816cb2ed in blk_mq_complete_request (rq=0xffff888013922300) at block/blk-mq.c:1235
#7  0xffffffff81ac59fd in scsi_done_internal (cmd=<optimized out>, complete_directly=<optimized out>) at drivers/scsi/scsi_lib.c:1630
#8  0xffffffff81ac5a5b in scsi_done (cmd=<optimized out>) at drivers/scsi/scsi_lib.c:1635
#9  0xffffffff81addc35 in iscsi_free_task (task=<optimized out>) at drivers/scsi/libiscsi.c:483
#10 0xffffffff81adfa85 in __iscsi_put_task (task=<optimized out>) at drivers/scsi/libiscsi.c:502
#11 0xffffffff81ae0670 in iscsi_data_in_rsp (task=<optimized out>, hdr=<optimized out>, conn=<optimized out>) at drivers/scsi/libiscsi.c:993
#12 __iscsi_complete_pdu (conn=conn@entry=0xffff888140fd7b60, hdr=hdr@entry=0xffff888140fd7d60, data=data@entry=0x0 <fixed_percpu_data>, datalen=datalen@entry=0) at drivers/scsi/libiscsi.c:1299
#13 0xffffffff81ae0a08 in iscsi_complete_pdu (conn=conn@entry=0xffff888140fd7b60, hdr=hdr@entry=0xffff888140fd7d60, data=data@entry=0x0 <fixed_percpu_data>, datalen=datalen@entry=0) at drivers/scsi/libiscsi.c:1359
#14 0xffffffff81ae4f89 in iscsi_tcp_process_data_in (tcp_conn=0xffff888140fd7cf0, segment=<optimized out>) at drivers/scsi/libiscsi_tcp.c:669
#15 0xffffffff81ae5541 in iscsi_tcp_recv_skb (conn=conn@entry=0xffff888140fd7b60, skb=skb@entry=0xffff8880070838e8, offset=offset@entry=0, offloaded=offloaded@entry=false, status=status@entry=0xffffc90000003c64) at drivers/scsi/libiscsi_tcp.c:974
#16 0xffffffff81ae6f48 in iscsi_sw_tcp_recv (rd_desc=<optimized out>, skb=0xffff8880070838e8, offset=0, len=<optimized out>) at drivers/scsi/iscsi_tcp.c:95
#17 0xffffffff81e80a6d in tcp_read_sock (sk=sk@entry=0xffff888103548000, desc=desc@entry=0xffffc90000003ce8, recv_actor=recv_actor@entry=0xffffffff81ae6ec0 <iscsi_sw_tcp_recv>) at net/ipv4/tcp.c:1704
#18 0xffffffff81ae75b1 in iscsi_sw_tcp_recv_data (conn=<optimized out>) at drivers/scsi/iscsi_tcp.c:145
#19 iscsi_sw_tcp_data_ready (sk=0xffff888103548000) at drivers/scsi/iscsi_tcp.c:185
#20 0xffffffff81e8edd8 in tcp_data_queue (sk=0xffff888103548000, skb=0xffff8880070838e8) at net/ipv4/tcp_input.c:5080
#21 0xffffffff81e8f7b7 in tcp_rcv_established (sk=0xffff888103548000, skb=0xffff8880070838e8) at net/ipv4/tcp_input.c:6017
#22 0xffffffff81e9d4d2 in tcp_v4_do_rcv (sk=sk@entry=0xffff888103548000, skb=skb@entry=0xffff8880070838e8) at net/ipv4/tcp_ipv4.c:1721
#23 0xffffffff81e9f6bb in tcp_v4_rcv (skb=0xffff8880070838e8) at net/ipv4/tcp_ipv4.c:2142
#24 0xffffffff81e6a73d in ip_protocol_deliver_rcu (net=0xffffffff838f4480 <init_net>, skb=0xffff8880070838e8, protocol=<optimized out>) at net/ipv4/ip_input.c:205
#25 0xffffffff81e6a922 in ip_local_deliver_finish (net=0xffffffff838f4480 <init_net>, sk=<optimized out>, skb=0xffff8880070838e8) at net/ipv4/ip_input.c:233
#26 0xffffffff81db81f6 in __netif_receive_skb_one_core (skb=<optimized out>, pfmemalloc=<optimized out>) at net/core/dev.c:5482
#27 0xffffffff81db84c8 in process_backlog (napi=0xffff88807dc2da90, quota=64) at net/core/dev.c:5924
#28 0xffffffff81db90e4 in __napi_poll (n=0xffffffff82427108 <cpu_bit_bitmap+40>, n@entry=0xffff88807dc2da90, repoll=repoll@entry=0xffffc90000003f47) at net/core/dev.c:6485
#29 0xffffffff81db9634 in napi_poll (repoll=0xffffc90000003f58, n=0xffff88807dc2da90) at net/core/dev.c:6552
#30 net_rx_action (h=<optimized out>) at net/core/dev.c:6663
#31 0xffffffff8217f704 in __do_softirq () at kernel/softirq.c:571
#32 0xffffffff811309ad in do_softirq () at kernel/softirq.c:472
```

发送数据
```txt
#0  __skb_clone (n=0xffff888103ba6ee8, skb=skb@entry=0xffff888103ba6e00) at net/core/skbuff.c:1264
#1  0xffffffff81d9abc6 in skb_clone (skb=skb@entry=0xffff888103ba6e00, gfp_mask=<optimized out>) at net/core/skbuff.c:1721
#2  0xffffffff81e938a6 in __tcp_transmit_skb (sk=sk@entry=0xffff8881035488c0, skb=skb@entry=0xffff888103ba6e00, clone_it=clone_it@entry=1, gfp_mask=gfp_mask@entry=2592, rcv_nxt=178545944) at net/ipv4/tcp_output.c:1261
#3  0xffffffff81e94f79 in tcp_transmit_skb (gfp_mask=2592, clone_it=1, skb=0xffff888103ba6e00, sk=0xffff8881035488c0) at net/ipv4/tcp_output.c:1417
#4  tcp_write_xmit (sk=sk@entry=0xffff8881035488c0, mss_now=mss_now@entry=65483, nonagle=1, push_one=push_one@entry=0, gfp=2592) at net/ipv4/tcp_output.c:2693
#5  0xffffffff81e95db1 in __tcp_push_pending_frames (sk=0xffff8881035488c0, cur_mss=cur_mss@entry=65483, nonagle=<optimized out>) at net/ipv4/tcp_output.c:2877
#6  0xffffffff81e7f073 in tcp_push (sk=sk@entry=0xffff8881035488c0, flags=flags@entry=0, mss_now=mss_now@entry=65483, nonagle=<optimized out>, size_goal=<optimized out>) at net/ipv4/tcp.c:729
#7  0xffffffff81e7f9f9 in do_tcp_sendpages (sk=sk@entry=0xffff8881035488c0, page=page@entry=0xffffea00002ae940, offset=<optimized out>, offset@entry=0, size=<optimized out>, size@entry=4096, flags=flags@entry=0) at net/ipv4/tcp.c:1115
#8  0xffffffff81e7fad7 in tcp_sendpage_locked (flags=0, size=4096, offset=0, page=0xffffea00002ae940, sk=0xffff8881035488c0) at net/ipv4/tcp.c:1141
#9  tcp_sendpage_locked (flags=0, size=4096, offset=0, page=0xffffea00002ae940, sk=0xffff8881035488c0) at net/ipv4/tcp.c:1133
#10 tcp_sendpage (sk=0xffff8881035488c0, page=0xffffea00002ae940, offset=0, size=4096, flags=0) at net/ipv4/tcp.c:1151
#11 0xffffffff81ebbc0b in inet_sendpage (sock=0xffff88810091e700, page=0xffffea00002ae940, offset=0, size=4096, flags=0) at net/ipv4/af_inet.c:844
#12 0xffffffff81b71373 in iscsit_fe_sendpage_sg (cmd=cmd@entry=0xffff888105b0f300, conn=conn@entry=0xffff888140fd7000) at drivers/target/iscsi/iscsi_target_util.c:1124
#13 0xffffffff81b734f3 in iscsit_xmit_datain_pdu (datain=0xffffc900019fbe00, cmd=<optimized out>, conn=0xffff888140fd7000) at drivers/target/iscsi/iscsi_target.c:635
#14 iscsit_xmit_pdu (conn=0xffff888140fd7000, cmd=0xffff888105b0f300, dr=<optimized out>, buf=0xffffc900019fbe00, buf_len=<optimized out>) at drivers/target/iscsi/iscsi_target.c:652
#15 0xffffffff81b77839 in iscsit_send_datain (conn=0xffff888140fd7000, cmd=0xffff888105b0f300) at drivers/target/iscsi/iscsi_target.c:2914
#16 iscsit_response_queue (conn=0xffff888140fd7000, cmd=0xffff888105b0f300, state=<optimized out>) at drivers/target/iscsi/iscsi_target.c:3799
#17 0xffffffff81b766a5 in iscsit_handle_response_queue (conn=0xffff888140fd7000) at drivers/target/iscsi/iscsi_target.c:3911
#18 iscsi_target_tx_thread (arg=0xffff888140fd7000) at drivers/target/iscsi/iscsi_target.c:3949
#19 0xffffffff81153694 in kthread (_create=0xffff88810164c980) at kernel/kthread.c:376
#20 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

## 从 fio 到 scsi 的 backtrace

target 侧和底层的存储沟通!
```txt
#0  target_complete_cmd_with_sense (cmd=0xffff888105b12670, scsi_status=0 '\000', sense_reason=0) at drivers/target/target_core_transport.c:875
#1  0xffffffff81b4f246 in target_complete_cmd (cmd=<optimized out>, scsi_status=<optimized out>) at drivers/target/target_core_transport.c:917
#2  0xffffffff81b5d3b3 in iblock_complete_cmd (cmd=<optimized out>) at drivers/target/target_core_iblock.c:326
#3  0xffffffff816cc0cc in req_bio_endio (error=0 '\000', nbytes=4096, bio=0xffff888009b98d00, rq=0xffff8880057e7b80) at block/blk-mq.c:794
#4  blk_update_request (req=req@entry=0xffff8880057e7b80, error=error@entry=0 '\000', nr_bytes=nr_bytes@entry=4096) at block/blk-mq.c:926
#5  0xffffffff81ac5082 in scsi_end_request (req=req@entry=0xffff8880057e7b80, error=error@entry=0 '\000', bytes=bytes@entry=4096) at drivers/scsi/scsi_lib.c:539
#6  0xffffffff81ac5b69 in scsi_io_completion (cmd=0xffff8880057e7c88, good_bytes=4096) at drivers/scsi/scsi_lib.c:977
#7  0xffffffff81ae817b in virtscsi_vq_done (fn=0xffffffff81ae7f40 <virtscsi_complete_cmd>, virtscsi_vq=0xffff88800568ca60, vscsi=0xffff88800568c818) at drivers/scsi/virtio_scsi.c:183
#8  virtscsi_req_done (vq=<optimized out>) at drivers/scsi/virtio_scsi.c:198
#9  0xffffffff81802a66 in vring_interrupt (irq=<optimized out>, _vq=0xffff888105b12670) at drivers/virtio/virtio_ring.c:2470
#10 vring_interrupt (irq=<optimized out>, _vq=0xffff888105b12670) at drivers/virtio/virtio_ring.c:2445
#11 0xffffffff811a2cd2 in __handle_irq_event_percpu (desc=desc@entry=0xffff888004782c00) at kernel/irq/handle.c:158
#12 0xffffffff811a2eb3 in handle_irq_event_percpu (desc=0xffff888004782c00) at kernel/irq/handle.c:193
#13 handle_irq_event (desc=desc@entry=0xffff888004782c00) at kernel/irq/handle.c:210
#14 0xffffffff811a7b8e in handle_edge_irq (desc=0xffff888004782c00) at kernel/irq/chip.c:819
#15 0xffffffff810cc965 in generic_handle_irq_desc (desc=0xffff888004782c00) at ./include/linux/irqdesc.h:158
#16 handle_irq (regs=<optimized out>, desc=0xffff888004782c00) at arch/x86/kernel/irq.c:231
#17 __common_interrupt (regs=<optimized out>, vector=37) at arch/x86/kernel/irq.c:250
#18 0xffffffff8216a187 in common_interrupt (regs=0xffffffff82c03df8, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```

```txt
#0  iscsit_add_cmd_to_response_queue (cmd=0xffff888105b1ea80, conn=0xffff888140fd7000, state=12 '\f') at drivers/target/iscsi/iscsi_target_util.c:580
#1  0xffffffff81b51b6b in target_complete_ok_work (work=0xffff888105b1eda0) at drivers/target/target_core_transport.c:2602
#2  0xffffffff8114ad24 in process_one_work (worker=worker@entry=0xffff8880070a50c0, work=0xffff888105b1eda0) at kernel/workqueue.c:2289
#3  0xffffffff8114af4c in worker_thread (__worker=0xffff8880070a50c0) at kernel/workqueue.c:2436
#4  0xffffffff81153694 in kthread (_create=0xffff88800bdf9480) at kernel/kthread.c:376
#5  0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

target 侧发送数据:
```txt
#0  loopback_xmit (skb=0xffff8880142d3ee8, dev=0xffff8880045d0000) at drivers/net/loopback.c:71
#1  0xffffffff81db57c8 in __netdev_start_xmit (more=false, dev=0xffff8880045d0000, skb=0xffff8880142d3ee8, ops=0xffffffff82521c60 <loopback_ops>) at ./include/linux/netdevice.h:4865
#2  netdev_start_xmit (more=false, txq=0xffff88810017c800, dev=0xffff8880045d0000, skb=0xffff8880142d3ee8) at ./include/linux/netdevice.h:4879
#3  xmit_one (more=false, txq=0xffff88810017c800, dev=0xffff8880045d0000, skb=0xffff8880142d3ee8) at net/core/dev.c:3583
#4  dev_hard_start_xmit (first=first@entry=0xffff8880142d3ee8, dev=dev@entry=0xffff8880045d0000, txq=txq@entry=0xffff88810017c800, ret=ret@entry=0xffffc900019fb97c) at net/core/dev.c:3599
#5  0xffffffff81db6422 in __dev_queue_xmit (skb=skb@entry=0xffff8880142d3ee8, sb_dev=sb_dev@entry=0x0 <fixed_percpu_data>) at net/core/dev.c:4249
#6  0xffffffff81e6e438 in dev_queue_xmit (skb=0xffff8880142d3ee8) at ./include/linux/netdevice.h:3035
#7  neigh_hh_output (skb=<optimized out>, hh=<optimized out>) at ./include/net/neighbour.h:530
#8  neigh_output (skip_cache=<optimized out>, skb=0xffff8880142d3ee8, n=0xffff8881424f5200) at ./include/net/neighbour.h:544
#9  ip_finish_output2 (net=<optimized out>, sk=<optimized out>, skb=0xffff8880142d3ee8) at net/ipv4/ip_output.c:228
#10 0xffffffff81e709fb in __ip_queue_xmit (sk=0xffff8881035488c0, skb=0xffff8880142d3ee8, fl=0xffff888103548c30, tos=<optimized out>) at net/ipv4/ip_output.c:532
#11 0xffffffff81e70c80 in ip_queue_xmit (sk=<optimized out>, skb=<optimized out>, fl=<optimized out>) at net/ipv4/ip_output.c:546
#12 0xffffffff81e93c5f in __tcp_transmit_skb (sk=sk@entry=0xffff8881035488c0, skb=0xffff8880142d3ee8, skb@entry=0xffff8880142d3e00, clone_it=clone_it@entry=1, gfp_mask=gfp_mask@entry=2592, rcv_nxt=<optimized out>) at net/ipv4/tcp_output.c:1399
#13 0xffffffff81e94f79 in tcp_transmit_skb (gfp_mask=2592, clone_it=1, skb=0xffff8880142d3e00, sk=0xffff8881035488c0) at net/ipv4/tcp_output.c:1417
#14 tcp_write_xmit (sk=sk@entry=0xffff8881035488c0, mss_now=mss_now@entry=65483, nonagle=1, push_one=push_one@entry=0, gfp=2592) at net/ipv4/tcp_output.c:2693
#15 0xffffffff81e95db1 in __tcp_push_pending_frames (sk=0xffff8881035488c0, cur_mss=cur_mss@entry=65483, nonagle=<optimized out>) at net/ipv4/tcp_output.c:2877
#16 0xffffffff81e7f073 in tcp_push (sk=sk@entry=0xffff8881035488c0, flags=flags@entry=0, mss_now=mss_now@entry=65483, nonagle=<optimized out>, size_goal=<optimized out>) at net/ipv4/tcp.c:729
#17 0xffffffff81e7ff9a in tcp_sendmsg_locked (sk=sk@entry=0xffff8881035488c0, msg=msg@entry=0xffffc900019fbc90, size=size@entry=48) at net/ipv4/tcp.c:1455
#18 0xffffffff81e80947 in tcp_sendmsg (sk=0xffff8881035488c0, msg=0xffffc900019fbc90, size=48) at net/ipv4/tcp.c:1483
#19 0xffffffff81d884ca in sock_sendmsg_nosec (msg=0xffffc900019fbc90, sock=0xffff88810091e700) at net/socket.c:717
#20 sock_sendmsg (sock=0xffff88810091e700, msg=msg@entry=0xffffc900019fbc90) at net/socket.c:734
#21 0xffffffff81b7116a in tx_data (conn=conn@entry=0xffff888140fd7000, iov=iov@entry=0xffffc900019fbd30, iov_count=iov_count@entry=1, data=data@entry=48) at drivers/target/iscsi/iscsi_target_util.c:1267
#22 0xffffffff81b712cb in iscsit_fe_sendpage_sg (cmd=cmd@entry=0xffff888105b00480, conn=conn@entry=0xffff888140fd7000) at drivers/target/iscsi/iscsi_target_util.c:1094
#23 0xffffffff81b734f3 in iscsit_xmit_datain_pdu (datain=0xffffc900019fbe00, cmd=<optimized out>, conn=0xffff888140fd7000) at drivers/target/iscsi/iscsi_target.c:635
#24 iscsit_xmit_pdu (conn=0xffff888140fd7000, cmd=0xffff888105b00480, dr=<optimized out>, buf=0xffffc900019fbe00, buf_len=<optimized out>) at drivers/target/iscsi/iscsi_target.c:652
#25 0xffffffff81b77839 in iscsit_send_datain (conn=0xffff888140fd7000, cmd=0xffff888105b00480) at drivers/target/iscsi/iscsi_target.c:2914
#26 iscsit_response_queue (conn=0xffff888140fd7000, cmd=0xffff888105b00480, state=<optimized out>) at drivers/target/iscsi/iscsi_target.c:3799
#27 0xffffffff81b766a5 in iscsit_handle_response_queue (conn=0xffff888140fd7000) at drivers/target/iscsi/iscsi_target.c:3911
#28 iscsi_target_tx_thread (arg=0xffff888140fd7000) at drivers/target/iscsi/iscsi_target.c:3949
#29 0xffffffff81153694 in kthread (_create=0xffff88810164c980) at kernel/kthread.c:376
#30 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```
