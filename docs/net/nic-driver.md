# nic driver

使用 https://www.net-swift.com/

## ring buffer 内存分配的 Node 在哪里


## 关键代码
- txgbe_xmit_frame_ring

- txgbe_intr

- txgbe_alloc_mapped_skb : 发送 sb 的内存

- txgbe_ring 持有 txgbe_tx_buffer / txgbe_rx_buffer，最后持有 buffer 的性能不可以。

- txgbe_tx_map : 将 skb 中数据部分发送提交给网卡
```c
dma = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);
tx_desc->read.buffer_addr = cpu_to_le64(dma);
```

- txgbe_alloc_rx_buffers : 创建接受队列
  - txgbe_alloc_mapped_page
    - dev_alloc_pages : 创建的队列是



- 这个应该就是提交给网卡的内容吧:
```c
union txgbe_tx_desc {
    struct {
        __le64 buffer_addr; /* Address of descriptor's data buf */
        __le32 cmd_type_len;
        __le32 olinfo_status;
    } read;
    struct {
        __le64 rsvd; /* Reserved */
        __le32 nxtseq_seed;
        __le32 status;
    } wb;
};
```



```c
struct txgbe_ring {
    struct txgbe_ring *next;        /* pointer to next ring in q_vector */
    struct txgbe_q_vector *q_vector; /* backpointer to host q_vector */
    struct net_device *netdev;      /* netdev ring belongs to */
    struct device *dev;             /* device for DMA mapping */
    struct txgbe_fwd_adapter *accel;
    void *desc;                     /* descriptor ring memory */
    union {
        struct txgbe_tx_buffer *tx_buffer_info;
        struct txgbe_rx_buffer *rx_buffer_info;
    };
    unsigned long state;
    u8 __iomem *tail;
    dma_addr_t dma;                 /* phys. address of descriptor ring */
    unsigned int size;              /* length in bytes */

    u16 count;                      /* amount of descriptors */

    u8 queue_index; /* needed for multiqueue queue management */
    u8 reg_idx;                     /* holds the special value that gets
                     * the hardware register offset
                     * associated with this ring, which is
                     * different for DCB and RSS modes
                     */
    u16 next_to_use;
    u16 next_to_clean;

#ifdef HAVE_PTP_1588_CLOCK
    unsigned long last_rx_timestamp;

#endif
    u16 rx_buf_len;
    union {
#ifndef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
        u16 next_to_alloc;
#endif
        struct {
            u8 atr_sample_rate;
            u8 atr_count;
        };
    };

    u8 dcb_tc;
    struct txgbe_queue_stats stats;
#ifdef HAVE_NDO_GET_STATS64
    struct u64_stats_sync syncp;
#endif
    union {
        struct txgbe_tx_queue_stats tx_stats;
        struct txgbe_rx_queue_stats rx_stats;
    };
} ____cacheline_internodealigned_in_smp;
```


```c
/* wrapper around a pointer to a socket buffer,
 * so a DMA handle can be stored along with the buffer */
struct txgbe_tx_buffer {
    union txgbe_tx_desc *next_to_watch;
    unsigned long time_stamp;
    struct sk_buff *skb;
    unsigned int bytecount;
    unsigned short gso_segs;
    __be16 protocol;
    DEFINE_DMA_UNMAP_ADDR(dma);
    DEFINE_DMA_UNMAP_LEN(len);
    u32 tx_flags;
};

struct txgbe_rx_buffer {
    struct sk_buff *skb;
    dma_addr_t dma;
#ifndef CONFIG_TXGBE_DISABLE_PACKET_SPLIT
    dma_addr_t page_dma;
    struct page *page;
    unsigned int page_offset;
#endif
};
```


## igc 网卡

- igc_probe

https://fedoramagazine.org/use-sysfs-to-restart-failed-pci-devices/

pci 设备的 rescan

```txt
@[
    igc_probe+5
    local_pci_probe+63
    pci_device_probe+195
    really_probe+415
    __driver_probe_device+120
    driver_probe_device+31
    __device_attach_driver+137
    bus_for_each_drv+146
    __device_attach+178
    pci_bus_add_device+78
    pci_bus_add_devices+48
    pci_bus_add_devices+91
    pci_rescan_bus+39
    rescan_store+113
    kernfs_fop_write_iter+287
    vfs_write+555
    ksys_write+111
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 1
```

## mac 的有线网卡是在 usb 上的
在 mac 中找到:
```txt
May 26 23:51:55 localhost kernel: r8152 2-1.2:1.0 enu1u2: Promiscuous mode enabled
```
才意识到这个网卡需要经过: drivers/net/usb/r8152.c

但是换了一个转换器之后，测试结果如下:


```txt
🧀  ls -la /sys/class/net
lrwxrwxrwx 0 root 12 Jul 20:22  br-in -> ../../devices/virtual/net/br-in
lrwxrwxrwx 0 root 12 Jul 20:23  docker0 -> ../../devices/virtual/net/docker0
lrwxrwxrwx 0 root 12 Jul 20:23  enu1c2 -> ../../devices/platform/soc/382280000.usb/xhci-hcd.0.auto/usb2/2-1/2-1:2.0/net/enu1c2
lrwxrwxrwx 0 root 12 Jul 20:22  lo -> ../../devices/virtual/net/lo
lrwxrwxrwx 0 root 12 Jul 20:22  ovs-system -> ../../devices/virtual/net/ovs-system
lrwxrwxrwx 0 root 18 Jul 12:26  vif73.2 -> ../../devices/virtual/net/vif73.2
lrwxrwxrwx 0 root 18 Jul 12:26  vif73.3 -> ../../devices/virtual/net/vif73.3
lrwxrwxrwx 0 root 12 Jul 20:22  wlp1s0f0 -> ../../devices/platform/soc/690000000.pcie/pci0000:00/0000:00:00.0/0000:01:00.0/net/wlp1s0f0
~ 🍍
🧀  lsusb
Bus 001 Device 001: ID 1d6b:0002 Linux Foundation 2.0 root hub
Bus 002 Device 001: ID 1d6b:0003 Linux Foundation 3.0 root hub
Bus 002 Device 002: ID 0b95:1790 ASIX Electronics Corp. AX88179 Gigabit Ethernet
~ 🍍
🧀  lspci -s 0000:01:00.0
01:00.0 Network controller: Broadcom Inc. and subsidiaries BCM4378 802.11ax Dual Band Wireless Network Adapter (rev 05)
```

cat /proc/interrupts 看到是 usb 的中断:
```txt
113:    6245976    4451664    2445490    2871034     254413     445750     108901    1682222      AIC2 66567 Level     xhci-hcd:usb1
```

一波调查，也是是切换了接口，实际上现在的网卡是:
```txt
@[
    cdc_ncm_fill_tx_frame+0
    usbnet_start_xmit+104
    dev_hard_start_xmit+164
    sch_direct_xmit+156
    __qdisc_run+124
    net_tx_action+520
    __do_softirq+304
    ____do_softirq+24
    call_on_irq_stack+36
    do_softirq_own_stack+36
    __irq_exit_rcu+256
    irq_exit_rcu+24
    el1_interrupt+56
    el1h_64_irq_handler+24
    el1h_64_irq+104
    cpuidle_enter_state+228
    cpuidle_enter+64
    cpuidle_idle_call+300
    do_idle+168
    cpu_startup_entry+64
    rest_init+264
    arch_call_rest_init+24
    start_kernel+820
    __primary_switched+184
]: 8885
```

iperf3 进程的 backtrace :
```txt
[<0>] wait_woken+0x70/0x80
[<0>] sk_stream_wait_memory+0x240/0x3c8
[<0>] tcp_sendmsg_locked+0x644/0xbb0
[<0>] tcp_sendmsg+0x40/0x70
[<0>] inet_sendmsg+0x4c/0x80
[<0>] __sock_sendmsg+0x64/0xc0
[<0>] sock_write_iter+0xb0/0x120
[<0>] vfs_write+0x304/0x378
[<0>] ksys_write+0xf8/0x120
[<0>] __arm64_sys_write+0x24/0x38
[<0>] invoke_syscall+0x74/0x100
[<0>] el0_svc_common.constprop.0+0x48/0xf0
[<0>] do_el0_svc+0x24/0x38
[<0>] el0_svc+0x3c/0x138
[<0>] el0t_64_sync_handler+0x120/0x130
[<0>] el0t_64_sync+0x194/0x198
```


```c
static const struct net_device_ops cdc_ncm_netdev_ops = {
	.ndo_open	     = usbnet_open,
	.ndo_stop	     = usbnet_stop,
	.ndo_start_xmit	     = usbnet_start_xmit,
	.ndo_tx_timeout	     = usbnet_tx_timeout,
	.ndo_set_rx_mode     = usbnet_set_rx_mode,
	.ndo_get_stats64     = dev_get_tstats64,
	.ndo_change_mtu	     = cdc_ncm_change_mtu,
	.ndo_set_mac_address = eth_mac_addr,
	.ndo_validate_addr   = eth_validate_addr,
};
```
原来小米的也是需要使用这个 cdc_ncm 的

```txt
[    8.824240] br9527: port 1(enp0s20f0u1c2) entered blocking state
[    8.824843] br9527: port 1(enp0s20f0u1c2) entered disabled state
[    8.825420] cdc_ncm 2-1:2.0 enp0s20f0u1c2: entered allmulticast mode
[    8.826043] cdc_ncm 2-1:2.0 enp0s20f0u1c2: entered promiscuous mode
[    8.826690] br9527: port 1(enp0s20f0u1c2) entered blocking state
[    8.826692] br9527: port 1(enp0s20f0u1c2) entered listening state
[   24.125051] br9527: port 1(enp0s20f0u1c2) entered learning state
[   39.483800] br9527: port 1(enp0s20f0u1c2) entered forwarding state ```

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
