# nic driver

使用 https://www.net-swift.com/

## ethtool
https://man7.org/linux/man-pages/man8/ethtool.8.html

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

## 一个中断一个队列的证据找一下

## 一定是驱动的问题
