大量的调用:
```txt
migration_transferred_bytes qemu_file 2727 multifd 53855087 RDMA 0

multifd_recv_unfill channel 1 packet_num 31 flags 0x4 next packet size 524303

multifd_send_fill channel 2 packet_num 6454 flags 0x4 next packet size 0

multifd_send_ram_fill channel 0 normal pages 128 zero pages 0
```

然后就是
```txt
ram_load_complete exit_code 0 seq iteration 53
```

```txt
vmstate_subsection_load HIDPointerEventQueue
vmstate_load_state virtio_pci/modern_queue_state v1
vmstate_load_state_field xhci-intr:erstba_low exists=1
vmstate_field_exists xhci-intr:ev_buffer_put field_version 0 version 1 result 0
vmstate_subsection_load_good virtio_pci/modern_queue_state
vmstate_load_state_end virtio_pci/modern_queue_state end/0
vmstate_n_elems erstba_high: 1
```

这似乎就是整个 ram 迭代的过程:
```txt
savevm_section_start ram, section_id 2
savevm_section_end ram, section_id 2 -> 0
migration_rate_limit_pre 94 ms
migration_rate_limit_post urgent: 0
migrate_transferred transferred 111570 time_spent 100 bandwidth 1115 switchover_bw 0 max_size 334710
migrate_pending_estimate estimate pending size 8440582144 (pre = 8440582144 post=0)
savevm_state_iterate
```

```txt
migration_bitmap_clear_dirty rb mem0 start 0x0 size 0x40000000 page 0x0
migration_bitmap_clear_dirty rb mem0 start 0x40000000 size 0x40000000 page 0x40000
migration_bitmap_clear_dirty rb mem0 start 0x80000000 size 0x40000000 page 0x80000
migration_bitmap_clear_dirty rb mem0 start 0xc0000000 size 0x40000000 page 0xc0000
migration_bitmap_clear_dirty rb mem0 start 0x100000000 size 0x40000000 page 0x100000
```

```txt
vmstate_save_state_loop xhci-intr/iman[1]
vmstate_save_state_loop xhci-intr/imod[1]
vmstate_save_state_loop xhci-intr/erstsz[1]
vmstate_save_state_loop xhci-intr/erstba_low[1]
vmstate_save_state_loop xhci-intr/erstba_high[1]
vmstate_save_state_loop xhci-intr/erdp_low[1]
vmstate_save_state_loop xhci-intr/erdp_high[1]
vmstate_save_state_loop xhci-intr/msix_used[1]
vmstate_save_state_loop xhci-intr/er_pcs[1]
vmstate_save_state_loop xhci-intr/er_start[1]
vmstate_save_state_loop xhci-intr/er_size[1]
vmstate_save_state_loop xhci-intr/er_ep_idx[1]
```

这个东西:
```txt
vmstate_save_state_top virtio_pci/modern_queue_state
vmstate_save_state_loop virtio_pci/modern_queue_state/num[1]
vmstate_save_state_loop virtio_pci/modern_queue_state/unused[1]
vmstate_save_state_loop virtio_pci/modern_queue_state/enabled[1]
vmstate_save_state_loop virtio_pci/modern_queue_state/desc[2]
vmstate_save_state_loop virtio_pci/modern_queue_state/avail[2]
vmstate_save_state_loop virtio_pci/modern_queue_state/used[2]
vmstate_subsection_save_top virtio_pci/modern_queue_state
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
