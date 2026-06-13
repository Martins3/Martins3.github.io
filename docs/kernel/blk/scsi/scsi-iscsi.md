# iscsi

- https://linux.vbird.org/linux_server/centos6/0460iscsi.php

## openiscsi 和 libiscsi
<!-- 6c04b0b4-df5e-4a1a-ad2c-939753fd41b5 -->

- kernel 实现 openiscsi ，配合用户态 iscsid 使用。
- libiscsi 是用户态的，所以每次构建 qemu 会有问题

open-iscsi 是这个:
https://github.com/open-iscsi/open-iscsi

## 基本原理
- https://wiki.nix-pro.com/view/ISCSI

## targetcli 和 openiscsi 的基本使用

https://manpages.ubuntu.com/manpages/focal/man8/targetcli.8.html

目前找到的最好操作方法:
- https://www.cnblogs.com/xiangsikai/p/10876534.html
  - 服务器端的配置，无需步骤 8 ，同样可以正常运行。

在 openeuler 中测试结果如下:

### server

sudo yum -y install targetcli

执行 targetcli

执行 ls
```txt
o- / .................................................... [...]
  o- backstores ......................................... [...]
  | o- block ............................. [Storage Objects: 0]
  | o- fileio ............................ [Storage Objects: 0]
  | o- pscsi ............................. [Storage Objects: 0]
  | o- ramdisk ........................... [Storage Objects: 0]
  o- iscsi ....................................... [Targets: 0]
  o- loopback .................................... [Targets: 0]
  o- srpt ........................................ [Targets: 0]
  o- vhost ....................................... [Targets: 0]
  o- xen-pvscsi .................................. [Targets: 0]
```
显然，backstores 就是当前的后端，
iscsi 等可以提供的前端。

#### 创建后端
```txt
# 进入指定目录下
/> cd /backstores/block
# 创建共享设备，命名为 “disk0”
/backstores/block> create disk0 /dev/nvme1n1
```

```txt
o- / ....................................................................... [...]
  o- backstores ............................................................ [...]
  | o- block ................................................ [Storage Objects: 1]
  | | o- disk0 .................... [/dev/nvme1n1 (1.5TiB) write-thru deactivated]
  | |   o- alua ................................................. [ALUA Groups: 1]
  | |     o- default_tg_pt_gp ..................... [ALUA state: Active/optimized]
  | o- fileio ............................................... [Storage Objects: 0]
  | o- pscsi ................................................ [Storage Objects: 0]
  | o- ramdisk .............................................. [Storage Objects: 0]
  o- iscsi .......................................................... [Targets: 0]
  o- loopback ....................................................... [Targets: 0]
  o- srpt ........................................................... [Targets: 0]
  o- vhost .......................................................... [Targets: 0]
  o- xen-pvscsi ..................................................... [Targets: 0]
```

当然最简单的是构建一个文件来作为后端:
```txt
       $ sudo targetcli
       /> backstores/fileio create test /tmp/test.img 100m
       /> iscsi/ create iqn.2006-04.com.example:test-target
       /> cd iscsi/iqn.2006-04.com.example:test-target/tpg1/
       tpg1/> luns/ create /backstores/fileio/test
       tpg1/> set attribute generate_node_acls=1
       tpg1/> exit
```

#### 创建前端
```txt
cd /iscsi
/iscsi> create
```

```txt
o- / ..................................................................................................................... [...]
  o- backstores .......................................................................................................... [...]
  | o- block .............................................................................................. [Storage Objects: 1]
  | | o- disk0 .................................................................... [/dev/nvme1n1 (1.5TiB) write-thru activated]
  | |   o- alua ............................................................................................... [ALUA Groups: 1]
  | |     o- default_tg_pt_gp ................................................................... [ALUA state: Active/optimized]
  | o- fileio ............................................................................................. [Storage Objects: 0]
  | o- pscsi .............................................................................................. [Storage Objects: 0]
  | o- ramdisk ............................................................................................ [Storage Objects: 0]
  o- iscsi ........................................................................................................ [Targets: 1]
  | o- iqn.2003-01.org.linux-iscsi.localhost.aarch64:sn.980b9a90bf36 ................................................. [TPGs: 1]
  |   o- tpg1 ........................................................................................... [no-gen-acls, no-auth]
  |     o- acls ...................................................................................................... [ACLs: 0]
  |     o- luns ...................................................................................................... [LUNs: 1]
  |     | o- lun0 .............................................................. [block/disk0 (/dev/nvme1n1) (default_tg_pt_gp)]
  |     o- portals ................................................................................................ [Portals: 1]
  |       o- 0.0.0.0:3260 ................................................................................................. [OK]
  o- loopback ..................................................................................................... [Targets: 0]
  o- srpt ......................................................................................................... [Targets: 0]
  o- vhost ........................................................................................................ [Targets: 0]
  o- xen-pvscsi ................................................................................................... [Targets: 0]
```

#### 关联 backstores 和前端:
```txt
# 进入指定目录
/iscsi> cd /iscsi/iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.037746bdcf2d/tpg1/luns
# 创建共享设备到 target
/iscsi/iqn.20...f2d/tpg1/luns> create /backstores/block/disk0
```

#### 关闭认证

```txt
/> cd /iscsi/iqn.2003-01.org.linux-iscsi.localhost.aarch64:sn.980b9a90bf36/tpg1

/iscsi/.../tpg1> set attribute authentication=0
/iscsi/.../tpg1> set attribute generate_node_acls=1
/iscsi/.../tpg1> set attribute demo_mode_write_protect=0
```

### client 操作


iscsiadm 还需要服务 iscsid 的服务
```sh
sudo yum install open-iscsi
sudo systemctl enable iscsid
sudo systemctl start iscsid
```

```txt
🍎 sudo iscsiadm -m discovery -t st -p 10.0.0.2

10.0.0.2:3260,1 iqn.2003-01.org.linux-iscsi.localhost.aarch64:sn.980b9a90bf36

🍎 sudo iscsiadm -m node -T iqn.2003-01.org.linux-iscsi.localhost.aarch64:sn.980b9a90bf36 -p 10.0.0.2:3260 --login
```

然后可以得到:
```txt
lrwxrwxrwx - root  5 Mar 23:42  sdb -> ../../devices/platform/host1/session1/target1:0:0/1:0:0:0/block/sdb
```

```txt
iscsiadm -m discovery -t st -p 127.0.0.1
iscsiadm -m node -T iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.037746bdcf2d -p 127.0.0.1 --login
```

## 常见命令
- iscsiadm -m session : 枚举所有的 session 的内容

## 内核 iscsi 代码简单分析

### client 侧

主要的代码在:
drivers/scsi/iscsi_tcp.c
drivers/scsi/libiscsi.c
drivers/scsi/libiscsi_tcp.c

相当于 iscsi 也是一个 hba 卡了:

```c
static const struct scsi_host_template iscsi_sw_tcp_sht = {
	.module			= THIS_MODULE,
	.name			= "iSCSI Initiator over TCP/IP",
	.queuecommand           = iscsi_queuecommand,
	.change_queue_depth	= scsi_change_queue_depth,
	.can_queue		= ISCSI_TOTAL_CMDS_MAX,
	.sg_tablesize		= 4096,
	.max_sectors		= 0xFFFF,
```

`iscsi_target_transport` 只是 `iscsit_transport` 的一种实现，也就是 TCP/IP 来实现的。

例如 iscsit_get_rx_pdu，可以找到如下三个:
```txt
drivers/infiniband/ulp/isert/ib_isert.c
2603:   .iscsit_get_rx_pdu      = isert_get_rx_pdu,

drivers/target/iscsi/cxgbit/cxgbit_main.c
684:    .iscsit_get_rx_pdu      = cxgbit_get_rx_pdu,

drivers/target/iscsi/iscsi_target.c
679:    .iscsit_get_rx_pdu      = iscsit_get_rx_pdu,
```


- do_softirq
  - __do_softirq
    - net_rx_action
      - napi_poll
        - __napi_poll
          - process_backlog
            - __netif_receive_skb_one_core
              - ip_local_deliver_finish
                - ip_protocol_deliver_rcu
                  - tcp_v4_rcv
                    - tcp_v4_do_rcv
                      - tcp_rcv_established
                        - tcp_data_queue
                          - iscsi_sw_tcp_data_ready
                            - iscsi_sw_tcp_recv_data
                              - tcp_read_sock
                                - iscsi_sw_tcp_recv
                                  - iscsi_tcp_recv_skb
                                    - iscsi_tcp_process_data_in
                                      - iscsi_complete_pdu
                                        - __iscsi_complete_pdu
                                          - iscsi_data_in_rsp
                                            - __iscsi_put_task
                                              - iscsi_free_task
                                                - scsi_done
                                                  - scsi_done_internal
                                                    - blk_mq_complete_request
                                                      - blk_mq_complete_request_remote
                                                        - blk_mq_complete_send_ipi
                                                          - smp_call_function_single_async
                                                            - generic_exec_single
                                                              - __smp_call_single_queue
                                                                - __send_ipi_mask

```txt
@[
    iscsi_queuecommand+0
    blk_mq_dispatch_rq_list+260
    __blk_mq_sched_dispatch_requests+1172
    blk_mq_sched_dispatch_requests+56
    blk_mq_run_hw_queue+388
    blk_mq_dispatch_list+308
    blk_mq_flush_plug_list+88
    __blk_flush_plug+248
    blk_finish_plug+64
    read_pages+312
    page_cache_ra_unbounded+316
    force_page_cache_ra+176
    page_cache_sync_ra+72
    filemap_get_pages+248
    filemap_read+268
    blkdev_read_iter+140
    aio_read.constprop.0+240
    io_submit_one.constprop.0+408
    __arm64_sys_io_submit+220
    invoke_syscall.constprop.0+88
    do_el0_svc+72
    el0_svc+92
    el0t_64_sync_handler+268
    el0t_64_sync+408
]: 5657
```

```txt
@[
    virtnet_poll_tx+0
    net_rx_action+364
    handle_softirqs+304
    __do_softirq+28
    ____do_softirq+24
    call_on_irq_stack+48
    do_softirq_own_stack+36
    do_softirq+152
    __local_bh_enable_ip+336
    __dev_queue_xmit+676
    ip_finish_output2+1764
    __ip_finish_output+176
    ip_finish_output+60
    ip_output+120
    __ip_queue_xmit+420
    ip_queue_xmit+28
    __tcp_transmit_skb+1184
    tcp_write_xmit+1148
    __tcp_push_pending_frames+68
    tcp_push+144
    tcp_sendmsg_locked+800
    tcp_sendmsg+64
    inet_sendmsg+76
    sock_sendmsg+136
    iscsi_sw_tcp_xmit_segment+164
    iscsi_sw_tcp_pdu_xmit+132
    iscsi_tcp_task_xmit+88
    iscsi_xmit_task+96
    iscsi_xmitworker+216
    process_one_work+540
    worker_thread+452
    kthread+340
    ret_from_fork+16
]: 19
```

### server 侧

主要的代码在:
drivers/target/

当时这个 target 应该是在 virtio-scsi 的盘上

- common_interrupt
  - __common_interrupt
    - handle_irq
      - generic_handle_irq_desc
        - handle_edge_irq
          - handle_irq_event
            - handle_irq_event_percpu
              - __handle_irq_event_percpu
                - vring_interrupt
                  - vring_interrupt
                    - virtscsi_req_done
                      - virtscsi_vq_done
                        - scsi_io_completion
                          - scsi_end_request
                            - blk_update_request
                              - req_bio_endio
                                - iblock_complete_cmd
                                  - target_complete_cmd
                                    - target_complete_cmd_with_sense


- ret_from_fork
  - kthread
    - worker_thread
      - process_one_work
        - target_complete_ok_work
          - iscsit_add_cmd_to_response_queue


target 侧发送数据:

- ret_from_fork
  - kthread
    - iscsi_target_tx_thread
      - iscsit_handle_response_queue
        - iscsit_response_queue
          - iscsit_send_datain
            - iscsit_xmit_pdu
              - iscsit_xmit_datain_pdu
                - iscsit_fe_sendpage_sg
                  - tx_data
                    - sock_sendmsg
                      - sock_sendmsg_nosec
                        - tcp_sendmsg
                          - tcp_sendmsg_locked
                            - tcp_push

## 用 scsi_debug 做 iscsi 的后端，有问题?
```txt
[ 1576.866364] Unable to recover from DataOut timeout while in ERL=0, closing iSCSI connection for I_T Nexus iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.a6b106aa84c2:xsk,i,0x00023d000004,iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.a6b106aa84c2,t,0x01
[ 1584.148092]  connection4:0: ping timeout of 5 secs expired, recv timeout 5, last rx 4296241093, last ping 4296246144, now 4296251392
[ 1584.148816]  connection4:0: detected conn error (1022)
[ 1601.556022] iSCSI Login timeout on Network Portal 0.0.0.0:3260
[ 1617.428294] Did not receive response to NOPIN on CID: 0, failing connection for I_T Nexus iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.a6b106aa84c2:xsk,i,0x00023d000004,iqn.2003-01.org.linux-iscsi.localhost.x8664:sn.a6b106aa84c2,t,0x01
[ 1704.980737]  session4: session recovery timed out after 120 secs
[ 1704.984660] sd 6:0:0:0: rejecting I/O to offline device
[ 1704.984883] I/O error, dev sdf, sector 401408 op 0x1:(WRITE) flags 0x2004000 phys_seg 128 prio class 2
[ 1704.985221] I/O error, dev sdf, sector 399360 op 0x1:(WRITE) flags 0x2004000 phys_seg 128 prio class 2
[ 1704.985543] I/O error, dev sdf, sector 398336 op 0x1:(WRITE) flags 0x2004000 phys_seg 128 prio class 2
[ 1704.985874] I/O error, dev sdf, sector 397312 op 0x1:(WRITE) flags 0x2004000 phys_seg 128 prio class 2
[ 1704.986199] I/O error, dev sdf, sector 285696 op 0x1:(WRITE) flags 0x2000000 phys_seg 128 prio class 2
[ 1704.986528] I/O error, dev sdf, sector 284672 op 0x1:(WRITE) flags 0x2004000 phys_seg 128 prio class 2
[ 1704.986534] I/O error, dev sdf, sector 530432 op 0x1:(WRITE) flags 0x2004000 phys_seg 128 prio class 2
[ 1704.986579] I/O error, dev sdf, sector 531456 op 0x1:(WRITE) flags 0x2000000 phys_seg 128 prio class 2
[ 1704.986582] I/O error, dev sdf, sector 532480 op 0x1:(WRITE) flags 0x2004000 phys_seg 128 prio class 2
[ 1704.986584] I/O error, dev sdf, sector 533504 op 0x1:(WRITE) flags 0x2004000 phys_seg 128 prio class 2
[ 1704.986590] Write-error on swap-device (8:80:535552)
[ 1704.986598] Write-error on swap-device (8:80:662528)
[ 1704.986599] Write-error on swap-device (8:80:666624)
[ 1704.986605] Write-error on swap-device (8:80:670720)
[ 1704.986606] Write-error on swap-device (8:80:674816)
[ 1704.986608] Write-error on swap-device (8:80:793600)
[ 1704.986610] Write-error on swap-device (8:80:797696)
[ 1704.986611] Write-error on swap-device (8:80:801792)
[ 1704.986616] Write-error on swap-device (8:80:1059840)
[ 1704.986616] Write-error on swap-device (8:80:805888)
[ 1704.987260] iscsi_trx (1489) used greatest stack depth: 12736 bytes left
[ 1704.990926] iSCSI Login negotiation failed.
[ 1704.991308] iSCSI Login negotiation failed.
[ 1704.991478] iSCSI Login negotiation failed.
[ 1704.991641] iSCSI Login negotiation failed.
[ 1704.991806] iSCSI Login negotiation failed.
[ 1704.991966] iSCSI Login negotiation failed.
[ 1704.992127] iSCSI Login negotiation failed.
[ 1704.992486] stress-ng (1609) used greatest stack depth: 12352 bytes left
```
看来是是网络的问题吧，只要这样配置就会有问题吗？


是不可以通过网络吗?
```txt
[ 1926.637286] sd 5:0:0:0: [sde] tag#148 CDB: Read(10) 28 00 00 19 93 d8 00 00 08 00
[ 1926.637547] I/O error, dev sde, sector 1676248 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 2
[ 1926.637843] Read-error on swap-device (8:64:1676248)
[ 1926.638025] sd 5:0:0:0: [sde] tag#149 FAILED Result: hostbyte=DID_TIME_OUT driverbyte=DRIVER_OK cmd_age=124s
[ 1926.638379] sd 5:0:0:0: [sde] tag#149 CDB: Read(10) 28 00 00 02 35 c0 00 00 08 00
[ 1926.638639] I/O error, dev sde, sector 144832 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 2
[ 1926.638931] Read-error on swap-device (8:64:144832)
[ 1926.639102] sd 5:0:0:0: [sde] tag#150 FAILED Result: hostbyte=DID_TIME_OUT driverbyte=DRIVER_OK cmd_age=121s
[ 1926.639452] sd 5:0:0:0: [sde] tag#150 CDB: Read(10) 28 00 00 04 77 a0 00 00 08 00
[ 1926.639711] I/O error, dev sde, sector 292768 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 2
[ 1926.640002] Read-error on swap-device (8:64:292768)
[ 1926.640180] sd 5:0:0:0: rejecting I/O to offline device
[ 1926.640317] I/O error, dev sde, sector 1084688 op 0x1:(WRITE) flags 0x2000000 phys_seg 1 prio class 2
[ 1926.640366] I/O error, dev sde, sector 39184 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 2
[ 1926.640896] Write-error on swap-device (8:64:1084688)
[ 1926.641791] Read-error on swap-device (8:64:39184)
[ 1926.642676] I/O error, dev sde, sector 1113808 op 0x1:(WRITE) flags 0x2000000 phys_seg 1 prio class 2
[ 1926.643217] Write-error on swap-device (8:64:1113808)
[ 1926.643395] I/O error, dev sde, sector 1486552 op 0x1:(WRITE) flags 0x2000000 phys_seg 1 prio class 2
[ 1926.643890] Write-error on swap-device (8:64:1486552)
[ 1926.644115] I/O error, dev sde, sector 397880 op 0x1:(WRITE) flags 0x2000000 phys_seg 1 prio class 2
[ 1926.644430] Write-error on swap-device (8:64:397880)
[ 1926.644617] I/O error, dev sde, sector 589592 op 0x1:(WRITE) flags 0x2000000 phys_seg 1 prio class 2
[ 1926.645294] Write-error on swap-device (8:64:589592)
[ 1926.645527] I/O error, dev sde, sector 601712 op 0x1:(WRITE) flags 0x2000000 phys_seg 1 prio class 2
[ 1926.645953] Write-error on swap-device (8:64:601712)
[ 1926.646127] Write-error on swap-device (8:64:649976)
[ 1926.679777] Read-error on swap-device (8:64:1200712)
[ 1926.681659] Read-error on swap-device (8:64:1200736)
[ 1926.681812] htop (1453) used greatest stack depth: 11088 bytes left
[ 1926.685040] Read-error on swap-device (8:64:1356296)
[ 1926.685273] Read-error on swap-device (8:64:1078848)
[ 1926.685728] Read-error on swap-device (8:64:1078448)
[ 1926.685907] Read-error on swap-device (8:64:1080016)
```

## 对于 fio 的盘分析一下

测试的 client 和 server 在一起的:

```txt
   - 14.34% do_syscall_64                                                                                                                    ▒
      - 13.80% __x64_sys_io_uring_enter                                                                                                      ▒
         - 9.06% io_submit_sqes                                                                                                              ▒
            - 8.98% io_issue_sqe                                                                                                             ▒
               - 8.82% io_read                                                                                                               ▒
                  - __io_read                                                                                                                ▒
                     - 8.79% blkdev_read_iter                                                                                                ▒
                        - 8.71% blkdev_direct_IO                                                                                             ▒
                           - 7.62% submit_bio_noacct_nocheck                                                                                 ▒
                              - 7.48% __submit_bio                                                                                           ▒
                                 - 6.55% __blk_flush_plug                                                                                    ▒
                                    - 6.55% blk_mq_flush_plug_list                                                                           ▒
                                       - 6.04% blk_mq_run_hw_queue                                                                           ▒
                                          - 5.98% blk_mq_sched_dispatch_requests                                                             ▒
                                             - 5.98% __blk_mq_sched_dispatch_requests                                                        ▒
                                                - 4.82% blk_mq_dispatch_rq_list                                                              ▒
                                                   - 4.81% scsi_queue_rq                                                                     ▒
                                                      - 4.09% iscsi_queuecommand                                                             ▒
                                                           3.47% queue_work_on                                                               ▒
                                   0.88% blk_mq_submit_bio
```

```txt
-   25.87%     0.00%  iscsi_ttx        [kernel.kallsyms]                [k] ret_from_fork_asm                                                ▒
     ret_from_fork_asm                                                                                                                       ▒
     ret_from_fork                                                                                                                           ▒
     kthread                                                                                                                                 ▒
   - iscsi_target_tx_thread                                                                                                                  ▒
      - 24.06% iscsit_response_queue                                                                                                         ▒
         - 23.74% iscsit_xmit_pdu                                                                                                            ▒
            - 23.69% iscsit_fe_sendpage_sg                                                                                                   ▒
               - 18.28% tcp_sendmsg                                                                                                          ▒
                  - 16.76% tcp_sendmsg_locked                                                                                                ▒
                     - 16.26% __tcp_push_pending_frames                                                                                      ▒
                        - tcp_write_xmit                                                                                                     ▒
                           - 15.76% __tcp_transmit_skb                                                                                       ▒
                              - 15.54% __ip_queue_xmit                                                                                       ▒
                                 - 15.42% ip_finish_output2                                                                                  ▒
                                    - 15.37% __dev_queue_xmit                                                                                ▒
                                       - 15.06% __local_bh_enable_ip                                                                         ▒
                                          - do_softirq                                                                                       ▒
                                             - handle_softirqs                                                                               ▒
                                                - 10.14% blk_complete_reqs                                                                   ▒
                                                   - 9.61% scsi_io_completion                                                                ▒
                                                      - scsi_end_request                                                                     ▒
                                                         - 8.20% blk_update_request                                                          ▒
                                                            - 7.71% blkdev_bio_end_io_async                                                  ▒
                                                               - 7.11% __io_req_task_work_add                                                ▒
                                                                  - 7.03% try_to_wake_up                                                     ▒
                                                                       7.00% _raw_spin_unlock_irqrestore
                                                - 4.54% net_rx_action                                                                        ▒
                                                   - 4.47% __napi_poll                                                                       ▒
                                                      - process_backlog                                                                      ▒
                                                         - 4.20% __netif_receive_skb_one_core                                                ▒
                                                            - 4.06% ip_local_deliver                                                         ▒
                                                               - 3.81% ip_protocol_deliver_rcu                                               ▒
                                                                  - 3.77% tcp_v4_rcv                                                         ▒
                                                                     - 3.28% tcp_v4_do_rcv                                                   ▒
                                                                        - tcp_rcv_established                                                ▒
                                                                           - 3.00% tcp_data_queue                                            ▒
                                                                              - 2.66% iscsi_sw_tcp_data_ready                                ▒
                                                                                 - 2.63% tcp_read_sock                                       ▒
                                                                                    - 1.44% iscsi_sw_tcp_recv                                ▒
                                                                                       - 1.41% iscsi_tcp_recv_skb                            ▒
                                                                                          - 0.99% iscsi_tcp_process_data_in                  ▒
                                                                                             - 0.96% iscsi_complete_pdu                      ▒
                                                                                                - 0.72% __iscsi_complete_pdu                 ▒
                                                                                                     0.56% iscsi_free_task                   ▒
                                                                                      0.61% __tcp_transmit_skb                               ▒
                  - 1.42% release_sock                                                                                                       ▒
                     - 1.37% __release_sock                                                                                                  ▒
                        - 1.26% tcp_v4_do_rcv                                                                                                ▒
                           - 1.25% tcp_rcv_established                                                                                       ▒
                                0.86% tcp_ack                                                                                                ▒
               - 5.32% tx_data                                                                                                               ▒
                  - 5.23% sock_sendmsg                                                                                                       ▒
                     - 5.17% tcp_sendmsg                                                                                                     ▒
                        - 4.98% tcp_sendmsg_locked                                                                                           ▒
                           - 4.58% __tcp_push_pending_frames                                                                                 ▒
                              - tcp_write_xmit                                                                                               ▒
                                 - 4.14% __tcp_transmit_skb
                                    - 3.87% __ip_queue_xmit                                                                                  ▒
                                       - 3.75% ip_finish_output2                                                                             ▒
                                          - 3.66% __dev_queue_xmit                                                                           ▒
                                             - 3.29% __local_bh_enable_ip                                                                    ▒
                                                - do_softirq                                                                                 ▒
                                                   - handle_softirqs                                                                         ▒
                                                      - net_rx_action                                                                        ▒
                                                         - 2.84% __napi_poll                                                                 ▒
                                                            - process_backlog                                                                ▒
                                                               - 2.62% __netif_receive_skb_one_core                                          ▒
                                                                  - 2.52% ip_local_deliver                                                   ▒
                                                                     - 2.39% ip_protocol_deliver_rcu                                         ▒
                                                                        - 2.36% tcp_v4_rcv                                                   ▒
                                                                           - 2.11% tcp_v4_do_rcv                                             ▒
                                                                              - 2.09% tcp_rcv_established                                    ▒
                                                                                 - 1.18% tcp_data_queue                                      ▒
                                                                                    - 0.89% iscsi_sw_tcp_data_ready                          ▒
                                                                                       - 0.81% tcp_read_sock                                 ▒
                                                                                          - 0.55% iscsi_sw_tcp_recv                          ▒
                                                                                               0.52% iscsi_tcp_recv_skb                      ▒
      - 0.99% schedule                                                                                                                       ▒
         - __schedule                                                                                                                        ▒
              finish_task_switch.isra.0
```

## 其他
https://github.com/Masorubka1/iscsi-client-rs


不知道为什么性能很差:
```txt
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=64
fio-3.39
Starting 1 process
^Cbs: 1 (f=1): [r(1)][0.6%][r=8336KiB/s][r=2084 IOPS][eta 16m:34s]
```
注意，后端是一个 nvme 的裸盘:

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
