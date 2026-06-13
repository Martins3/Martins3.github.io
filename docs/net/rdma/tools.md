# rdma 常用工具
## ibv_devinfo -v

```txt
🧀   ibv_devinfo -v


hca_id: rocep0s6
        transport:                      InfiniBand (0)
        fw_ver:                         16.31.1014
        node_guid:                      8c2a:8e03:00d4:a538
        sys_image_guid:                 8c2a:8e03:00d4:a538
        vendor_id:                      0x02c9
        vendor_part_id:                 4119
        hw_ver:                         0x0
        board_id:                       HUA0000000024
        phys_port_cnt:                  1
        max_mr_size:                    0xffffffffffffffff
        page_size_cap:                  0xfffffffffffff000
        max_qp:                         262144
        max_qp_wr:                      32768
        device_cap_flags:               0x25321c36
                                        BAD_PKEY_CNTR
                                        BAD_QKEY_CNTR
                                        AUTO_PATH_MIG
                                        CHANGE_PHY_PORT
                                        PORT_ACTIVE_EVENT
                                        SYS_IMAGE_GUID
                                        RC_RNR_NAK_GEN
                                        MEM_WINDOW
                                        XRC
                                        MEM_MGT_EXTENSIONS
                                        MEM_WINDOW_TYPE_2B
                                        RAW_IP_CSUM
                                        MANAGED_FLOW_STEERING
        max_sge:                        30
        max_sge_rd:                     30
        max_cq:                         16777216
        max_cqe:                        4194303
        max_mr:                         16777216
        max_pd:                         8388608
        max_qp_rd_atom:                 16
        max_ee_rd_atom:                 0
        max_res_rd_atom:                4194304
        max_qp_init_rd_atom:            16
        max_ee_init_rd_atom:            0
        atomic_cap:                     ATOMIC_HCA (1)
        max_ee:                         0
        max_rdd:                        0
        max_mw:                         16777216
        max_raw_ipv6_qp:                0
        max_raw_ethy_qp:                0
        max_mcast_grp:                  2097152
        max_mcast_qp_attach:            240
        max_total_mcast_qp_attach:      503316480
        max_ah:                         2147483647
        max_fmr:                        0
        max_srq:                        8388608
        max_srq_wr:                     32767
        max_srq_sge:                    31
        max_pkeys:                      128
        local_ca_ack_delay:             16
        general_odp_caps:
                                        ODP_SUPPORT
                                        ODP_SUPPORT_IMPLICIT
        rc_odp_caps:
                                        SUPPORT_SEND
                                        SUPPORT_RECV
                                        SUPPORT_WRITE
                                        SUPPORT_READ
                                        SUPPORT_SRQ
        uc_odp_caps:
                                        NO SUPPORT
        ud_odp_caps:
                                        SUPPORT_SEND
        xrc_odp_caps:
                                        SUPPORT_SEND
                                        SUPPORT_WRITE
                                        SUPPORT_READ
                                        SUPPORT_SRQ
        completion timestamp_mask:                      0x7fffffffffffffff
        hca_core_clock:                 156250kHZ
        raw packet caps:
                                        C-VLAN stripping offload
                                        Scatter FCS offload
                                        IP csum offload
                                        Delay drop
        device_cap_flags_ex:            0x1425321C36
                                        RAW_SCATTER_FCS
                                        PCI_WRITE_END_PADDING
        tso_caps:
                max_tso:                        262144
                supported_qp:
                                        SUPPORT_RAW_PACKET
        rss_caps:
                max_rwq_indirection_tables:                     1048576
                max_rwq_indirection_table_size:                 2048
                rx_hash_function:                               0x1
                rx_hash_fields_mask:                            0x800000FF
                supported_qp:
                                        SUPPORT_RAW_PACKET
        max_wq_type_rq:                 8388608
        packet_pacing_caps:
                qp_rate_limit_min:      1kbps
                qp_rate_limit_max:      25000000kbps
                supported_qp:
                                        SUPPORT_RAW_PACKET
        tag matching not supported

        cq moderation caps:
                max_cq_count:   65535
                max_cq_period:  4095 us

        maximum available device memory:        131072Bytes

        num_comp_vectors:               8
                port:   1
                        state:                  PORT_ACTIVE (4)
                        max_mtu:                4096 (5)
                        active_mtu:             1024 (3)
                        sm_lid:                 0
                        port_lid:               0
                        port_lmc:               0x00
                        link_layer:             Ethernet
                        max_msg_sz:             0x40000000
                        port_cap_flags:         0x04010000
                        port_cap_flags2:        0x0000
                        max_vl_num:             invalid value (0)
                        bad_pkey_cntr:          0x0
                        qkey_viol_cntr:         0x0
                        sm_sl:                  0
                        pkey_tbl_len:           1
                        gid_tbl_len:            255
                        subnet_timeout:         0
                        init_type_reply:        0
                        active_width:           1X (1)
                        active_speed:           10.0 Gbps (4)
                        phys_state:             LINK_UP (5)
                        GID[  0]:               fe80:0000:0000:0000:8e2a:8eff:fe40:a37c, RoCE v1
                        GID[  1]:               fe80::8e2a:8eff:fe40:a37c, RoCE v2
                        GID[  2]:               fe80:0000:0000:0000:5ee4:835c:fc12:0336, RoCE v1
                        GID[  3]:               fe80::5ee4:835c:fc12:336, RoCE v2
                        GID[  4]:               0000:0000:0000:0000:0000:ffff:0a00:0301, RoCE v1
                        GID[  5]:               ::ffff:10.0.3.1, RoCE v2
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
