## 基本环境搭建
最好是配置单独的网段
```sh
sudo yum install libibverbs-utils perftest rdma-core

sudo rdma link add rxe_0 type rxe netdev ens5
rdma link
ibv_devices
ibv_devinfo -d rxe_0

# 不过这个有什么区别?
ib_send_bw -d rxe_0
ib_send_bw -d rxe_0 10.10.155.0

ib_write_bw -D 1000
ib_write_bw 10.10.155.0 -D 1000

# 控制流测试，很好
ibv_rc_pingpong -d rxe_0 -g 2
ibv_rc_pingpong -d rxe_0 -g 2 10.10.155.0

ibv_ud_pingpong
```

```txt
🧀  tree /dev/infiniband 
/dev/infiniband
├── by-ibdev
│   └── uverbs-rxe_0 -> ../uverbs0
├── rdma_cm
└── uverbs0
```

## pyverbs 的基本尝试

在 fedora 中尝试结果为:

```sh
uv venv
uv pip install pyverbs
sudo yum install libnl3-devel rdma-core-devel
sudo yum install python3-devel
```

然后使用这个测试 rdma-core 仓库中的 pyverbs/examples/ib_devices.py 这个

## 基本 c 语言测试
https://github.com/StarryVae/RDMA-tutorial

* rdma_send_recv: two-sided verb(send/recv)
* rdma_write_read: one-sided verb(write/read)

## ib_send_bw 的测试结果

```txt
     ret_from_fork_asm                                                                                                                                               ▒
     ret_from_fork                                                                                                                                                   ▒
     kthread                                                                                                                                                         ▒
     worker_thread                                                                                                                                                   ▒
   - process_one_work                                                                                                                                                ▒
      - 99.43% do_task                                                                                                                                               ▒
         - 99.35% rxe_sender                                                                                                                                         ▒
            - 95.85% rxe_requester                                                                                                                                   ▒
               - 73.76% rxe_xmit_packet                                                                                                                              ▒
                  - 53.61% ip_finish_output2                                                                                                                         ▒
                     - 52.66% __dev_queue_xmit                                                                                                                       ▒
                        - 50.63% sch_direct_xmit                                                                                                                     ▒
                           - 45.96% dev_hard_start_xmit                                                                                                              ▒
                              - start_xmit                                                                                                                           ▒
                                 - 27.77% virtqueue_notify                                                                                                           ▒
                                    - vp_notify                                                                                                                      ▒
                                         iowrite16                                                                                                                   ▒
                                 - 8.86% free_old_xmit                                                                                                               ▒
                                    - __free_old_xmit                                                                                                                ▒
                                       - 4.02% virtqueue_get_buf_ctx                                                                                                 ▒
                                            detach_buf_split                                                                                                         ▒
                                       - 3.31% sk_skb_reason_drop                                                                                                    ▒
                                          - 2.91% skb_release_head_state                                                                                             ▒
                                             - rxe_skb_tx_dtor                                                                                                       ▒
                                                  0.65% 0xffffffffc101833c                                                                                           ▒
                                                  0.56% rxe_pool_get_index                                                                                           ▒
                                         0.51% kmem_cache_free                                                                                                       ▒
                                   5.29% virtqueue_kick_prepare                                                                                                      ▒
                                   0.67% virtqueue_enable_cb_delayed                                                                                                 ▒
                                   0.64% virtqueue_disable_cb                                                                                                        ▒
                                   0.51% virtqueue_add_outbuf                                                                                                        ▒
                           - 3.11% _raw_spin_lock                                                                                                                    ▒
                                __pv_queued_spin_lock_slowpath                                                                                                       ▒
                           - 0.80% _raw_spin_unlock                                                                                                                  ▒
                                __raw_callee_save___pv_queued_spin_unlock                                                                                            ▒
                           - 0.59% validate_xmit_skb_list                                                                                                            ▒
                                validate_xmit_skb.isra.0                                                                                                             ▒
                  - 10.87% rxe_icrc_generate                                          
                     - 6.93% rxe_icrc_hdr.isra.0                                                                                                                     ▒
                        - rxe_crc32.isra.0                                                                                                                           ▒
                           - 5.61% crc32_update_arch                                                                                                                 ▒
                                5.08% crc32_le_base                                                                                                                  ▒
                     - 3.79% rxe_crc32.isra.0                                                                                                                        ▒
                        - 3.56% crc32_update_arch                                                                                                                    ▒
                           - 2.92% crc32_le_arch                                                                                                                     ▒
                                2.11% crc32_pclmul_le_16                                                                                                             ▒
                             0.60% crc32_le_base                                                                                                                     ▒
                  - 5.51% ip_local_out                                                                                                                               ▒
                     - __ip_local_out                                                                                                                                ▒
                        - 4.73% nf_hook_slow                                                                                                                         ▒
                           - 3.36% nf_conntrack_in                                                                                                                   ▒
                              - 1.51% hash_conntrack_raw.constprop.0                                                                                                 ▒
                                   __siphash_unaligned                                                                                                               ▒
                                1.17% __nf_conntrack_find_get.isra.0                                                                                                 ▒
                  - 1.35% ip_output                                                                                                                                  ▒
                       0.71% nf_hook_slow                                                                                                                            ▒
                    0.69% _raw_spin_unlock_irqrestore                                                                                                                ▒
               - 7.30% copy_data                                                                                                                                     ▒
                    5.24% __rxe_put                                                                                                                                  ▒
                    0.98% rxe_mr_copy                                                                                                                                ▒
                  - 0.93% lookup_mr                                                                                                                                  ▒
                       rxe_pool_get_index                                                                                                                            ▒
               - 5.95% rxe_init_packet                                                                                                                               ▒
                  - 2.35% __alloc_skb                                                                                                                                ▒
                     - 1.04% kmalloc_reserve                                                                                                                         ▒
                          0.83% __kmalloc_node_track_caller_noprof                                                                                                   ▒
                  - 1.07% 0xffffffffc1019315                                                                                                                         ▒
                       _raw_read_unlock_irqrestore                                                                                                                   ▒
                  - 0.97% 0xffffffffc1019f16                                                                                                                         ▒
                       _raw_read_unlock_irqrestore                                                                                                                   ▒
                    0.86% 0xffffffffc101a087                                                                                                                         ▒
               - 4.88% rxe_prepare                                                                                                                                   ▒
                    2.22% __ip_select_ident                                                                                                                          ▒
                    1.76% rxe_find_route                                             
                 2.60% _raw_spin_unlock_irqrestore                                                                                                                   ▒
                 0.66% _raw_spin_lock_irqsave                                                                                                                        ▒
            - 3.42% rxe_completer                                                                                                                                    ▒
                 1.51% _raw_spin_unlock_irqrestore                                                                                                                   ▒
               - 1.13% skb_dequeue                                                                                                                                   ▒
                    0.77% _raw_spin_unlock_irqrestore                   
```

大致的路线是这样的:
```txt
// 接受，但是下面是 udp 的
@[
    rxe_sched_task+5
    rxe_udp_encap_recv+121
    udp_queue_rcv_one_skb+600
    udp_unicast_rcv_skb+118
    ip_protocol_deliver_rcu+163
    ip_local_deliver_finish+118
    ip_sublist_rcv_finish+100
    ip_sublist_rcv+336
    ip_list_rcv+259
    __netif_receive_skb_list_core+554
    netif_receive_skb_list_internal+472
    napi_complete_done+110
    virtnet_poll+1460
    __napi_poll+40
    net_rx_action+388
    handle_softirqs+220
    irq_exit_rcu+161
    common_interrupt+71
    asm_common_interrupt+38
]: 31433

// 发送端通过写直接发送
@[
    rxe_sched_task+5
    rxe_post_send+779
    ib_uverbs_post_send+1632
    ib_uverbs_write+1041
    vfs_write+257
    ksys_write+202
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 31567
```

这么看，整个路线就是把基本的操作

## nvme over rdma 的基本测试

target 端

网络接受到之后:
```txt
@[
    ib_cq_completion_workqueue+5
    rxe_cq_post+281
    do_complete+539
    rxe_completer+1873
    rxe_sender+29
    do_task+95
    process_one_work+325
    worker_thread+715
    kthread+240
    ret_from_fork+49
    ret_from_fork_asm+26
]: 24088
```

然后去 kick nvme 的工作:
```txt
@[
    nvme_queue_rqs+5
    blk_mq_flush_plug_list+1605
    __blk_flush_plug+214
    blk_finish_plug+40
    nvmet_bdev_execute_rw+571
    nvmet_rdma_execute_command+102
    nvmet_rdma_handle_command+232
    __ib_process_cq+120
    ib_cq_poll_work+42
    process_one_work+325
    worker_thread+715
    kthread+240
    ret_from_fork+49
    ret_from_fork_asm+26
]: 95350
```

host 端

发送:
```txt
@[
        nvme_rdma_post_send+1
        nvme_rdma_queue_rq+461
        __blk_mq_issue_directly+66
        blk_mq_plug_issue_direct+114
        blk_mq_flush_plug_list+406
        __blk_flush_plug+243
        __submit_bio+460
        __submit_bio_noacct+145
        __blkdev_direct_IO_async+408
        blkdev_read_iter+169
        aio_read+308
        io_submit_one+236
        __x64_sys_io_submit+149
        do_syscall_64+127
        entry_SYSCALL_64_after_hwframe+118
]: 229377
```

接收:
```txt
@[
        nvme_rdma_recv_done+5
        __ib_process_cq+127
        ib_poll_handler+48
        irq_poll_softirq+155
        handle_softirqs+239
        do_softirq.part.0+59
        __local_bh_enable_ip+96
        __dev_queue_xmit+1079
        ip_finish_output2+635
        rxe_xmit_packet+404
        rxe_requester+625
        rxe_sender+19
        do_task+94
        process_one_work+368
        worker_thread+597
        kthread+236
        ret_from_fork+49
        ret_from_fork_asm+26
]: 32230
```

似乎最后大家都是走 ib_post_send ，和用户态的接口好类似的啊。

```c
static inline int ib_post_send(struct ib_qp *qp,
			       const struct ib_send_wr *send_wr,
			       const struct ib_send_wr **bad_send_wr)
{
	const struct ib_send_wr *dummy;

	return qp->device->ops.post_send(qp, send_wr, bad_send_wr ? : &dummy);
}
```

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
