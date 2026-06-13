# openvswitch

## 基本的
- [bridge 和 ovs 的对比](https://www.fiber-optic-transceiver-module.com/ovs-vs-linux-bridge-who-is-the-winner.html)
- [OpenVSwitch实现浅谈（一）](https://zhuanlan.zhihu.com/p/66216907)
- [openvswitch](https://www.zhihu.com/column/software-defined-network)

参考这个简单的搭建尝试一下吧:
- https://kiosk007.top/post/%E4%BD%BF%E7%94%A8open-vswitch%E6%9E%84%E5%BB%BA%E8%99%9A%E6%8B%9F%E7%BD%91%E7%BB%9C/

内核中打开 ovs 的时候，会自动打开如下的选项:
```txt
CONFIG_NF_NAT_OVS=y
CONFIG_OPENVSWITCH=m
CONFIG_MPLS=y
CONFIG_NET_MPLS_GSO=y
# CONFIG_MPLS_ROUTING is not set
CONFIG_NET_NSH=y
```

## tun
还要这么复杂啊
https://lists.gnu.org/archive/html/qemu-discuss/2021-05/msg00073.html

## bridge

https://wiki.archlinux.org/title/network_bridge

https://en.wikibooks.org/wiki/QEMU/Networking

sudo chmod 0666 /dev/net/tun

真的很 simple

sudo ip link add br2 type bridge
sudo ip link set enp7s0 master br2
sudo ip address add 10.0.0.5/24 dev br2
sudo ip link set dev br2 up

```txt
7: tap0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel master br0 state UNKNOWN group default qlen 1000
    link/ether fe:e0:88:d7:66:4a brd ff:ff:ff:ff:ff:ff
    inet6 fe80::fce0:88ff:fed7:664a/64 scope link proto kernel_ll
       valid_lft forever preferred_lft forever
```

然后进入到虚拟机中，执行:
sudo ip address add 10.0.0.10/24 dev ens3


## https://developers.redhat.com/articles/2022/04/06/introduction-linux-bridging-commands-and-features#

> `bridge` displays and manipulates bridges on final distribution boards (FDBs), main distribution boards (MDBs), and virtual local area networks (VLANs).

## tap

https://gist.github.com/extremecoders-re/e8fd8a67a515fee0c873dcafc81d811c

https://www.redhat.com/sysadmin/setup-network-bridge-VM


# bridge && switch

http://cxd2014.github.io/2016/11/08/linux-bridge/

https://www.cnblogs.com/zmkeil/archive/2013/04/21/3034733.html

交换机(switch) 集线器(hub) 网桥(bridge) 中继器(repeater) 的关系[^1]

![](https://ipwithease.com/wp-content/uploads/2020/06/Network-Bridge-vs-Switch.jpg)
> 所有 switch 比 bridge 更加强大

[The rise of Linux-based networking hardware](https://lwn.net/Articles/720313/)
> 似乎，其实 linux 并没有在 switch 上占据优势

- https://developers.redhat.com/articles/2022/04/06/introduction-linux-bridging-commands-and-features#spanning_tree_protocol

## 似乎 wifi 很难增加 bridge 啊?

[^1]: https://www.zhihu.com/question/20279620/answer/67985156

## 看了下逻辑

1. 构建两个网卡
2. 从 virtio 那里接受到包，立刻到 tun ，然后 ovs 转发，在另一侧像是真的有一个网卡设备一样

```txt
         - 13.43% do_writev                                                                                                                                                                                                            ▒
            - 13.38% vfs_writev                                                                                                                                                                                                        ▒
               - 13.23% do_iter_write                                                                                                                                                                                                  ▒
                  - 13.13% do_iter_readv_writev                                                                                                                                                                                        ▒
                     - 13.09% tun_chr_write_iter                                                                                                                                                                                       ▒
                        - 13.04% tun_get_user                                                                                                                                                                                          ▒
                           - 9.34% __local_bh_enable_ip                                                                                                                                                                                ▒
                              - do_softirq.part.0                                                                                                                                                                                      ▒
                                 - __do_softirq                                                                                                                                                                                        ▒
                                    - 9.20% net_rx_action                                                                                                                                                                              ▒
                                       - 9.06% __napi_poll                                                                                                                                                                             ▒
                                          - 9.04% process_backlog                                                                                                                                                                      ▒
                                             - 8.91% __netif_receive_skb_one_core                                                                                                                                                      ▒
                                                - 7.30% ip_local_deliver_finish                                                                                                                                                        ▒
                                                   - 7.28% ip_protocol_deliver_rcu                                                                                                                                                     ▒
                                                      - 7.24% tcp_v4_rcv                                                                                                                                                               ▒
                                                         - 6.90% tcp_v4_do_rcv                                                                                                                                                         ▒
                                                            - 6.87% tcp_rcv_established                                                                                                                                                ▒
                                                               - 4.73% __tcp_push_pending_frames                                                                                                                                       ▒
                                                                  - 4.72% tcp_write_xmit                                                                                                                                               ▒
                                                                     - 3.75% __tcp_transmit_skb                                                                                                                                        ▒
                                                                        - 3.67% __ip_queue_xmit                                                                                                                                        ▒
                                                                           - 3.51% ip_finish_output2                                                                                                                                   ▒
                                                                              - 3.50% __dev_queue_xmit                                                                                                                                 ▒
                                                                                 - 3.48% dev_hard_start_xmit                                                                                                                           ▒
                                                                                    - internal_dev_xmit                                                                                                                                ▒
                                                                                       - 3.47% ovs_vport_receive                                                                                                                       ▒
                                                                                          - 3.44% ovs_dp_process_packet                                                                                                                ▒
                                                                                             - 3.41% ovs_execute_actions                                                                                                               ▒
                                                                                                - do_execute_actions                                                                                                                   ▒
                                                                                                   - 3.40% __dev_queue_xmit                                                                                                            ▒
                                                                                                      - 3.38% sch_direct_xmit                                                                                                          ▒
                                                                                                         - 2.61% validate_xmit_skb_list                                                                                                ▒
                                                                                                            - validate_xmit_skb                                                                                                        ▒
                                                                                                               - 2.58% __skb_gso_segment                                                                                               ▒
                                                                                                                  - skb_mac_gso_segment                                                                                                ▒
                                                                                                                     - 2.56% inet_gso_segment                                                                                          ▒
                                                                                                                        - 2.49% tcp_gso_segment                                                                                        ▒
                                                                                                                           - 2.38% skb_segment                                                                                         ▒
                                                                                                                              - 1.34% __alloc_skb                                                                                      ▒
                                                                                                                                   0.51% kmalloc_reserve                                                                               ▒
                                                                                                         - 0.76% dev_hard_start_xmit                                                                                                   ▒
                                                                                                              tun_net_xmit                                                                                                             ▒
                                                                     - 0.82% ktime_get                                                                                                                                                 ▒
                                                                          read_hpet                                                                                                                                                    ▒
                                                               - 1.09% tcp_mstamp_refresh                                                                                                                                              ▒
                                                                  - ktime_get                                                                                                                                                          ▒
                                                                       read_hpet                                                                                                                                                       ▒
                                                                 0.80% tcp_ack
                                                - 1.32% ip_rcv                                                                                                                                                                         ◆
                                                   - 1.13% nf_hook_slow                                                                                                                                                                ▒
                                                        0.75% nft_do_chain_inet                                                                                                                                                        ▒
                           - 3.07% netif_receive_skb                                                                                                                                                                                   ▒
                              - 1.99% __netif_receive_skb_one_core                                                                                                                                                                     ▒
                                 - __netif_receive_skb_core.constprop.0                                                                                                                                                                ▒
                                    - 1.96% netdev_frame_hook                                                                                                                                                                          ▒
                                       - 1.92% ovs_vport_receive                                                                                                                                                                       ▒
                                          - 1.59% ovs_dp_process_packet                                                                                                                                                                ▒
                                             - 1.30% ovs_execute_actions                                                                                                                                                               ▒
                                                - do_execute_actions                                                                                                                                                                   ▒
                                                   - 1.24% internal_dev_recv                                                                                                                                                           ▒
                                                      - 1.21% netif_rx                                                                                                                                                                 ▒
                                                         - netif_rx_internal                                                                                                                                                           ▒
                                                            - 1.14% ktime_get_with_offset                                                                                                                                              ▒
                                                                 read_hpet                                                                                                                                                             ▒
                              - 1.05% ktime_get_with_offset                                                                                                                                                                            ▒
                                   read_hpet                                                                                                                                                                                           ▒
         - 6.24% ksys_read                                                                                                                                                                                                             ▒
            - 6.08% vfs_read                                                                                                                                                                                                           ▒
               - 5.01% tun_chr_read_iter                                                                                                                                                                                               ▒
                  - 4.86% tun_do_read                                                                                                                                                                                                  ▒
                     - 2.44% skb_copy_datagram_iter                                                                                                                                                                                    ▒
                        - 2.41% __skb_datagram_iter                                                                                                                                                                                    ▒
                           - 1.15% simple_copy_to_iter                                                                                                                                                                                 ▒
                                1.01% __check_object_size                                                                                                                                                                              ▒
                           - 1.08% _copy_to_iter                                                                                                                                                                                       ▒
                                0.96% copyout                                                                                                                                                                                          ▒
                     - 1.09% consume_skb                                                                                                                                                                                               ▒
                        - 0.97% skb_release_data                                                                                                                                                                                       ▒
                             0.61% __slab_free                                                                                                                                                                                         ▒
         - 3.03% __x64_sys_clock_gettime                                                                                                                                                                                               ▒
            - 2.87% posix_get_monotonic_timespec                                                                                                                                                                                       ▒
               - ktime_get_ts64                                                                                                                                                                                                        ▒
                    read_hpet                                                                                                                                                                                                          ▒
         - 2.32% syscall_exit_to_user_mode                                                                                                                                                                                             ▒
              0.57% exit_to_user_mode_prepare                                                                                                                                                                                          ▒
         - 0.75% __x64_sys_ppoll                                                                                                                                                                                                       ▒
              0.59% do_sys_poll                                                                                                                                                                                                        ▒
   - 0.54% _start                                                                                                                                                                                                                      ▒
        __libc_start_main@@GLIBC_2.34                                                                                                                                                                                                  ▒
        __libc_start_call_main                                                                                                                                                                                                         ▒
        qemu_default_main                                                                                                                                                                                                              ▒
        qemu_main_loop                                                                                                                                                                                                                 ▒
      - main_loop_wait                                                                                                                                                                                                                 ▒
           0.53% os_host_main_loop_wait (inlined)
```

```txt
      - 13.43% do_writev                                                                                                                                                                                                               ▒
         - 13.37% vfs_writev                                                                                                                                                                                                           ◆
            - 13.23% do_iter_write                                                                                                                                                                                                     ▒
               - 13.13% do_iter_readv_writev                                                                                                                                                                                           ▒
                  - 13.09% tun_chr_write_iter                                                                                                                                                                                          ▒
                     - 13.04% tun_get_user                                                                                                                                                                                             ▒
                        - 9.34% __local_bh_enable_ip                                                                                                                                                                                   ▒
                           - do_softirq.part.0                                                                                                                                                                                         ▒
                              - __do_softirq                                                                                                                                                                                           ▒
                                 - 9.20% net_rx_action                                                                                                                                                                                 ▒
                                    - 9.06% __napi_poll                                                                                                                                                                                ▒
                                       - 9.04% process_backlog                                                                                                                                                                         ▒
                                          - 8.91% __netif_receive_skb_one_core                                                                                                                                                         ▒
                                             - 7.30% ip_local_deliver_finish                                                                                                                                                           ▒
                                                - 7.28% ip_protocol_deliver_rcu                                                                                                                                                        ▒
                                                   - 7.24% tcp_v4_rcv                                                                                                                                                                  ▒
                                                      - 6.90% tcp_v4_do_rcv                                                                                                                                                            ▒
                                                         - 6.87% tcp_rcv_established                                                                                                                                                   ▒
                                                            - 4.73% __tcp_push_pending_frames                                                                                                                                          ▒
                                                               - 4.72% tcp_write_xmit                                                                                                                                                  ▒
                                                                  - 3.75% __tcp_transmit_skb                                                                                                                                           ▒
                                                                     - 3.67% __ip_queue_xmit                                                                                                                                           ▒
                                                                        - 3.51% ip_finish_output2                                                                                                                                      ▒
                                                                           - 3.50% __dev_queue_xmit                                                                                                                                    ▒
                                                                              - 3.48% dev_hard_start_xmit                                                                                                                              ▒
                                                                                 - internal_dev_xmit                                                                                                                                   ▒
                                                                                    - 3.47% ovs_vport_receive                                                                                                                          ▒
                                                                                       - 3.44% ovs_dp_process_packet                                                                                                                   ▒
                                                                                          - 3.41% ovs_execute_actions                                                                                                                  ▒
                                                                                             - do_execute_actions                                                                                                                      ▒
                                                                                                - 3.40% __dev_queue_xmit                                                                                                               ▒
                                                                                                   - 3.38% sch_direct_xmit                                                                                                             ▒
                                                                                                      - 2.61% validate_xmit_skb_list                                                                                                   ▒
                                                                                                         - validate_xmit_skb                                                                                                           ▒
                                                                                                            - 2.58% __skb_gso_segment                                                                                                  ▒
                                                                                                               - skb_mac_gso_segment                                                                                                   ▒
                                                                                                                  - 2.56% inet_gso_segment                                                                                             ▒
                                                                                                                     - 2.49% tcp_gso_segment                                                                                           ▒
                                                                                                                        - 2.38% skb_segment                                                                                            ▒
                                                                                                                           - 1.34% __alloc_skb                                                                                         ▒
                                                                                                                                0.51% kmalloc_reserve                                                                                  ▒
                                                                                                      - 0.76% dev_hard_start_xmit                                                                                                      ▒
                                                                                                           tun_net_xmit                                                                                                                ▒
                                                                  - 0.82% ktime_get                                                                                                                                                    ▒
                                                                       read_hpet                                                                                                                                                       ▒
                                                            - 1.09% tcp_mstamp_refresh                                                                                                                                                 ▒
                                                               - ktime_get                                                                                                                                                             ▒
                                                                    read_hpet                                                                                                                                                          ▒
                                                              0.80% tcp_ack                                                                                                                                                            ▒
                                             - 1.32% ip_rcv                                                                                                                                                                            ▒
                                                - 1.13% nf_hook_slow                                                                                                                                                                   ▒
                                                     0.75% nft_do_chain_inet                                                                                                                                                           ▒
                        - 3.07% netif_receive_skb
                           - 1.99% __netif_receive_skb_one_core                                                                                                                                                                        ◆
                              - __netif_receive_skb_core.constprop.0                                                                                                                                                                   ▒
                                 - 1.96% netdev_frame_hook                                                                                                                                                                             ▒
                                    - 1.92% ovs_vport_receive                                                                                                                                                                          ▒
                                       - 1.59% ovs_dp_process_packet                                                                                                                                                                   ▒
                                          - 1.30% ovs_execute_actions                                                                                                                                                                  ▒
                                             - do_execute_actions                                                                                                                                                                      ▒
                                                - 1.24% internal_dev_recv                                                                                                                                                              ▒
                                                   - 1.21% netif_rx                                                                                                                                                                    ▒
                                                      - netif_rx_internal                                                                                                                                                              ▒
                                                         - 1.14% ktime_get_with_offset                                                                                                                                                 ▒
                                                              read_hpet                                                                                                                                                                ▒
                           - 1.05% ktime_get_with_offset                                                                                                                                                                               ▒
                                read_hpet                                                                                                                                                                                              ▒
      - 6.52% ksys_read                                                                                                                                                                                                                ▒
         - 6.08% vfs_read                                                                                                                                                                                                              ▒
            - 5.01% tun_chr_read_iter                                                                                                                                                                                                  ▒
               - 4.86% tun_do_read                                                                                                                                                                                                     ▒
                  - 2.44% skb_copy_datagram_iter                                                                                                                                                                                       ▒
                     - 2.41% __skb_datagram_iter                                                                                                                                                                                       ▒
                        - 1.15% simple_copy_to_iter                                                                                                                                                                                    ▒
                             1.01% __check_object_size                                                                                                                                                                                 ▒
                        - 1.08% _copy_to_iter                                                                                                                                                                                          ▒
                             0.96% copyout                                                                                                                                                                                             ▒
                  - 1.09% consume_skb                                                                                                                                                                                                  ▒
                     - 0.97% skb_release_data                                                                                                                                                                                          ▒
                          0.61% __slab_free                                                                                                                                                                                            ▒
      - 3.08% __x64_sys_clock_gettime                                                                                                                                                                                                  ▒
         - 2.92% posix_get_monotonic_timespec                                                                                                                                                                                          ▒
            - ktime_get_ts64                                                                                                                                                                                                           ▒
                 read_hpet                                                                                                                                                                                                             ▒
      - 2.28% syscall_exit_to_user_mode                                                                                                                                                                                                ▒
           0.57% exit_to_user_mode_prepare                                                                                                                                                                                             ▒
      - 0.75% __x64_sys_ppoll                                                                                                                                                                                                          ▒
           0.59% do_sys_poll
```

```txt
   - 4.03% virtio_net_flush_tx                                                                                                                                                                                                         ▒
      - 4.03% qemu_net_queue_send_iov                                                                                                                                                                                                  ▒
           qemu_net_queue_deliver_iov (inlined)                                                                                                                                                                                        ▒
           qemu_deliver_packet_iov                                                                                                                                                                                                     ◆
           qemu_deliver_packet_iov                                                                                                                                                                                                     ▒
           net_hub_port_receive_iov                                                                                                                                                                                                    ▒
           net_hub_receive_iov (inlined)                                                                                                                                                                                               ▒
           qemu_net_queue_send_iov                                                                                                                                                                                                     ▒
           qemu_net_queue_deliver_iov (inlined)                                                                                                                                                                                        ▒
           qemu_deliver_packet_iov                                                                                                                                                                                                     ▒
           qemu_deliver_packet_iov                                                                                                                                                                                                     ▒
           tap_receive_iov                                                                                                                                                                                                             ▒
           tap_write_packet                                                                                                                                                                                                            ▒
         - __GI___writev                                                                                                                                                                                                               ▒
            - 4.03% entry_SYSCALL_64                                                                                                                                                                                                   ▒
                 do_syscall_64                                                                                                                                                                                                         ▒
                 do_writev                                                                                                                                                                                                             ▒
               - vfs_writev                                                                                                                                                                                                            ▒
                  - 4.03% do_iter_write                                                                                                                                                                                                ▒
                       do_iter_readv_writev                                                                                                                                                                                            ▒
                       tun_chr_write_iter                                                                                                                                                                                              ▒
                     - tun_get_user                                                                                                                                                                                                    ▒
                        - 2.13% netif_receive_skb                                                                                                                                                                                      ▒
                           - 1.15% __netif_receive_skb_one_core                                                                                                                                                                        ▒
                              - __netif_receive_skb_core.constprop.0                                                                                                                                                                   ▒
                                 - netdev_frame_hook                                                                                                                                                                                   ▒
                                    - ovs_vport_receive                                                                                                                                                                                ▒
                                       - ovs_dp_process_packet                                                                                                                                                                         ▒
                                         ovs_execute_actions                                                                                                                                                                           ▒
                                         do_execute_actions                                                                                                                                                                            ▒
                                       - internal_dev_recv                                                                                                                                                                             ▒
                                          - netif_rx                                                                                                                                                                                   ▒
                                          - netif_rx_internal                                                                                                                                                                          ▒
                                             - 1.08% ktime_get_with_offset                                                                                                                                                             ▒
                                                  read_hpet                                                                                                                                                                            ▒
                           - 0.98% ktime_get_with_offset                                                                                                                                                                               ▒
                                read_hpet                                                                                                                                                                                              ▒
                        - 1.89% __local_bh_enable_ip                                                                                                                                                                                   ▒
                             do_softirq.part.0                                                                                                                                                                                         ▒
                             __do_softirq                                                                                                                                                                                              ▒
                             net_rx_action                                                                                                                                                                                             ▒
                             __napi_poll                                                                                                                                                                                               ▒
                             process_backlog                                                                                                                                                                                           ▒
                           - __netif_receive_skb_one_core                                                                                                                                                                              ▒
                              - 1.89% ip_local_deliver_finish                                                                                                                                                                          ▒
                                   ip_protocol_deliver_rcu                                                                                                                                                                             ▒
                                 - tcp_v4_rcv                                                                                                                                                                                          ▒
                                    - 1.88% tcp_v4_do_rcv                                                                                                                                                                              ▒
                                       - tcp_rcv_established                                                                                                                                                                           ▒
                                          - 1.02% tcp_mstamp_refresh                                                                                                                                                                   ▒
                                             - ktime_get                                                                                                                                                                               ▒
                                                  read_hpet                                                                                                                                                                            ▒
                                          - 0.81% __tcp_push_pending_frames
                                             - tcp_write_xmit                                                                                                                                                                          ▒
                                                - 0.76% ktime_get                                                                                                                                                                      ▒
                                                     read_hpet
```

## TODO
 https://www.ibm.com/docs/en/linux-on-systems?topic=choices-using-open-vswitch


## 似乎经过漫长的折磨，现在只是存在一个问题了

如果给 br-in 配置上 ip addresss ，或者物理网卡配置上 ip ，另外的一个物理机器是否可以连接上它

似乎现在的做法是，将物理网卡和 ovs bridge 链接上，然后给 ovs-bridge 链接上网络

## ovs 和 dpdk 如何链接起来 ?
https://docs.openvswitch.org/en/latest/intro/install/


## 继续分析下
➜  ~ systemctl list-dependencies  openvswitch

openvswitch.service
× ├─ovs-vswitchd.service

看来是需要找一个干净的环境搞下:
```txt
× ovs-vswitchd.service - Open vSwitch Forwarding Unit
     Loaded: loaded (/usr/lib/systemd/system/ovs-vswitchd.service; static)
     Active: failed (Result: exit-code) since Wed 2024-01-17 14:55:12 CST; 2min 35s ago
    Process: 1931 ExecStart=/usr/share/openvswitch/scripts/ovs-ctl --no-ovsdb-server --no-monitor --system-id=random ${OVS_USER_OPT} start $OPTIONS (code=exited, status=1/FAILURE)
        CPU: 28ms

Jan 17 14:55:12 bogon systemd[1]: ovs-vswitchd.service: Scheduled restart job, restart counter is at 5.
Jan 17 14:55:12 bogon systemd[1]: Stopped Open vSwitch Forwarding Unit.
Jan 17 14:55:12 bogon systemd[1]: ovs-vswitchd.service: Start request repeated too quickly.
Jan 17 14:55:12 bogon systemd[1]: ovs-vswitchd.service: Failed with result 'exit-code'.
Jan 17 14:55:12 bogon systemd[1]: Failed to start Open vSwitch Forwarding Unit.
```

这个目录是做啥的 ?


## 做基本的连接测试

将 13900k 的网卡直接连接到另外的机器中，不使用交换机，但是所有的物理网卡连上 ovs :

```txt
       13900K
     +-----------+                   Mac
     |           |                 +-------+
     |  +--+-+   |                 |       |
     |  |  |++---+-----------------+----+  |
     |  |  |++---+-----------------+----+  |
     |  +--+-+   |                 |       |
     |           |                 +-------+
     |           |                  Lenove
     |           |                 +-------+
     |  +--+-+   |                 |       |
     |  |  |++---+-----------------+----+  |
     |  |  |++---+-----------------+----+  |
     |  +--+-+   |                 |       |
     |           |                 +-------+
     +-----------+
```

如果不是用 ovs ，直接给物理网卡 ip ， 13900k 上的两个网卡只有一个有效

Mac 和 Lenove 只有一个可以和 13900K ping 通，更不用说 Mac 和 Lenove 的方法联通了。

## 这个会导致什么问题吗?
```txt
May  2 04:18:05 HOSTNAME kernel: openvswitch: ovs-system: deferred action limit reached, drop recirc action
```
https://bugzilla.redhat.com/show_bug.cgi?id=2203811

似乎总是遇到从这个问题:

## 从打包入手
- https://gitee.com/src-openeuler/openvswitch
  - 指向了这里: https://github.com/openvswitch/ovs

## 代码统计

### 用户态部分
主要的代码都在 lib (21w) 和 ofproto (4w)

lib 下主要的代码是:
```txt
 ./dpif-netdev.c                                                                                                             10601         8135         1008         1458
 ./ofp-actions.c                                                                                                             10038         7358         1484         1196
 ./odp-util.c                                                                                                                 8971         7462          481         1028
 ./netdev-linux.c                                                                                                             7305         6781          305          219
 ./netdev-dpdk.c                                                                                                              6828         5121          649         1058
 ./dpif-netlink.c                                                                                                             5324         4222          337          765
 ./ovsdb-idl.c                                                                                                                4509         3157          885          467
 ./tc.c                                                                                                                       4034         3387          129          518
 ./flow.c                                                                                                                     3735         2854          445          436
 ./meta-flow.c                                                                                                                3705         2989          212          504
 ./conntrack.c                                                                                                                3563         2837          256          470
 ./netdev-offload-tc.c                                                                                                        3232         2651          139          442
 ./dpctl.c                                                                                                                    3221         2711           99          411
 ./netdev-offload-dpdk.c                                                                                                      2776         2273          126          377
 ./db-ctl-base.c                                                                                                              2712         2226          172          314
 ./ovsdb-data.c                                                                                                               2595         1963          327          305
 ./util.c                                                                                                                     2533         1779          451          303
 ./netdev.c                                                                                                                   2425         1665          503          257
 ./ofp-group.c                                                                                                                2413         1953          146          314
 ./ofp-table.c                                                                                                                2412         1924          215          273
 ./ovsdb-cs.c                                                                                                                 2388         1743          388          257
 ./nx-match.c                                                                                                                 2335         1688          352          295
 ./classifier.c                                                                                                               2276         1497          500          279
 ./rstp-state-machines.c                                                                                                      2221         1822          266          133
 ./netdev-dummy.c                                                                                                             2209         1776           68          365
 ./dpif.c                                                                                                                     2147         1522          362          263
```
### 内核部分

对比来看，内核代码是相当简单了:
```txt
 ./flow_netlink.c                                                                                                         3840         3034          198          608
 ./datapath.c                                                                                                             2811         2205          143          463
 ./conntrack.c                                                                                                            2031         1600          145          286
 ./actions.c                                                                                                              1725         1326          102          297
 ./flow_table.c                                                                                                           1220          922           77          221
 ./flow.c                                                                                                                 1120          824          146          150
 ./meter.c                                                                                                                 766          566           63          137
 ./vport.c                                                                                                                 581          334          168           79
 ./vport-internal_dev.c                                                                                                    247          188            8           51
 ./vport-netdev.c                                                                                                          216          161           17           38
 ./vport-vxlan.c                                                                                                           169          130            7           32
 ./vport-geneve.c                                                                                                          140          107            9           24
 ./vport-gre.c                                                                                                             103           81            4           18
 ./dp_notify.c                                                                                                              86           63            6           17
```

## container 下也是可以用的吗?
- https://github.com/servicefractal/ovs/blob/master/Dockerfile

倒不是 systemd 封装，但是 kernel 支持吗?

## linux bridge 需要查询 flow 吗?

在 ovs_dp_process_packet 中，如果没有查到，那么需要 upcall 到用户态中去:

```c
	flow = ovs_flow_tbl_lookup_stats(&dp->table, key, skb_get_hash(skb),
					 &n_mask_hit, &n_cache_hit);
  error = ovs_dp_upcall(dp, skb, key, &upcall, 0);
	if (unlikely(!flow)) {
  			consume_skb(skb);
```

但是显然，linux bridge 中是没有这种用户态的 systemd 服务的

## 开机的时候，这个日志是什么意思?
```txt
[   41.025666] FS-Cache: Netfs 'nfs' registered for caching
[   41.728610] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   41.813285] sched: RT throttling activated
[   50.710885] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712474] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712542] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712603] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712675] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712736] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712798] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712858] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712919] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   50.712979] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[   51.171712] md: md127: resync done.
[   51.183560] md: resync of RAID array md126
```

## ovs 的 bridge port interface
<!-- b4695e04-e7fe-4198-a902-c962e2f5e6bd -->

sudo ovs-vsctl list-interface br-in
sudo ovs-vsctl list-ifaces br-in
sudo ovs-vsctl list-ports br-in

- Bridge（网桥） 类似于一个虚拟交换机，是 OVS 的顶层容器。一个 OVS 实例可以有多个 bridge（如 br0, br1）。
- Port（端口） 是 bridge 上的逻辑端口。每个 port 可以绑定一个或多个 interface（用于链路聚合等场景）。port 代表了 bridge 的一个“接入点”。
- Interface（接口） 是实际的数据面实体，对应一个网络设备（如 veth、tap、物理网卡、internal 类型接口等）。一个 interface 必须属于一个 port。

```txt
e64d59b7-c72b-4102-904a-e014277ed567
    Bridge br-in
        Port vif_t_11_2
            Interface vif_t_11_2
                error: "could not open network device vif_t_11_2 (No such device)"
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
