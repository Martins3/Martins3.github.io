# 记录一下网络栈的一些源码分析

## 调试 bmbt 的时候，打过断点的位置

### arch/x86/kernel/apic/msi.c: `irq_msi_compose_msg`

### drivers/net/ethernet/intel/e1000e/netdev.c

- `e1000_intr_msix_tx` => `e1000_clean_tx_irq` => `netdev_completed_queue` => `netdev_tx_completed_queue`

`e1000_clean_tx_ring` => `netdev_reset_queue` => `netdev_tx_reset_queue`

- `e1000_xmit_frame`
  - `netdev_sent_queue`
  - `e1000_tx_queue`

### net/core/dev.c
- `xmit_one`
  - `dev_queue_xmit`

### net/core/neighbour.c
- `neigh_direct_output`

### `net/ipv6/ip6_output.c`

- `ip6_finish_output2`

### net/ipv6/mcast.c
- `mld_ifc_start_timer`

### `net/sched/sch_generic.c`
- `sch_direct_xmit`

## 关键的结构体
- `struct socket`

## net 目录的简单阅读
