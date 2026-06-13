## virtio-net
为什么这么复杂，比 virtio-blk 复杂多了

为什么 virtio-net 相比 host iperf 的性能那么差，而且的确是 CPU 打满了
想错了，不是 guest 中网卡驱动的问题，而是 host 中的解析的问题。
将 QEMU 的后端换成 ovs + tap 就没问题了

```c
static const struct net_device_ops virtnet_netdev = {
	.ndo_open            = virtnet_open,
	.ndo_stop   	     = virtnet_close,
	.ndo_start_xmit      = start_xmit,
	.ndo_validate_addr   = eth_validate_addr,
	.ndo_set_mac_address = virtnet_set_mac_address,
	.ndo_set_rx_mode     = virtnet_set_rx_mode,
	.ndo_get_stats64     = virtnet_stats,
	.ndo_vlan_rx_add_vid = virtnet_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = virtnet_vlan_rx_kill_vid,
	.ndo_bpf		= virtnet_xdp,
	.ndo_xdp_xmit		= virtnet_xdp_xmit,
	.ndo_features_check	= passthru_features_check,
	.ndo_get_phys_port_name	= virtnet_get_phys_port_name,
	.ndo_set_features	= virtnet_set_features,
	.ndo_tx_timeout		= virtnet_tx_timeout,
};
```

似乎每一个网卡都是需要实现这些
```c
static const struct ethtool_ops virtnet_ethtool_ops = {
	.supported_coalesce_params = ETHTOOL_COALESCE_MAX_FRAMES |
		ETHTOOL_COALESCE_USECS,
	.get_drvinfo = virtnet_get_drvinfo,
	.get_link = ethtool_op_get_link,
	.get_ringparam = virtnet_get_ringparam,
	.set_ringparam = virtnet_set_ringparam,
	.get_strings = virtnet_get_strings,
	.get_sset_count = virtnet_get_sset_count,
	.get_ethtool_stats = virtnet_get_ethtool_stats,
	.set_channels = virtnet_set_channels,
	.get_channels = virtnet_get_channels,
	.get_ts_info = ethtool_op_get_ts_info,
	.get_link_ksettings = virtnet_get_link_ksettings,
	.set_link_ksettings = virtnet_set_link_ksettings,
	.set_coalesce = virtnet_set_coalesce,
	.get_coalesce = virtnet_get_coalesce,
	.set_per_queue_coalesce = virtnet_set_per_queue_coalesce,
	.get_per_queue_coalesce = virtnet_get_per_queue_coalesce,
	.get_rxfh_key_size = virtnet_get_rxfh_key_size,
	.get_rxfh_indir_size = virtnet_get_rxfh_indir_size,
	.get_rxfh = virtnet_get_rxfh,
	.set_rxfh = virtnet_set_rxfh,
	.get_rxnfc = virtnet_get_rxnfc,
	.set_rxnfc = virtnet_set_rxnfc,
};
```

## qemu 中 slirp + virtio-net 的调用栈

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - notifier_list_notify
              - slirp_pollfds_poll
                - sorecvfrom
                  - udp_output
                    - ip_output
                      - if_start
                        - if_encap
                          - slirp_send_packet_all
                            - net_slirp_send_packet
                              - qemu_net_queue_send
                                - qemu_deliver_packet_iov
                                  - virtio_net_do_receive
                                    - virtio_net_receive_rcu
                                      - address_space_stl_le
                                        - memory_region_dispatch_write
                                          - access_with_adjusted_size
                                            - amdvi_mem_ir_write


## 看看这个东西
https://www.openeuler.org/en/blog/xinleguo/2020-11-23-Virtio_Net_Technology.html

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
