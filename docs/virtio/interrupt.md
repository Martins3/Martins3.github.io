## interrupt

暂时不是特别感兴趣:

- vring_interrupt : qeueu 接受到信息
- vp_config_changed : 当出现配置的变化的时候，例如修改 virtio-mem 的大小的时候

- virtio_find_vqs
  - vp_modern_find_vqs : pci modern 注册的 hook
    - vp_find_vqs
      - vp_find_vqs_msix ：这是推荐的配置
        - vp_request_msix_vectors : 注册 vp_config_changed 和 vp_vring_interrupt 中断。
        - vp_setup_vq : 给每一个 queue 注册 vring_interrupt
          - setup_vq : 这是 virtio_pci_device::setup_vq 注册的 hook
            - vring_create_virtqueue : 创建 virtqueue
              - vring_create_virtqueue_packed
              - vring_create_virtqueue_split
                - vring_alloc_queue_split
                - vring_alloc_state_extra_split
                - virtqueue_vring_init_split
                - virtqueue_init
                - virtqueue_vring_attach_split
      - vp_find_vqs_intx
        - request_irq
        - vp_setup_vq
          - `vp_dev->setup_vq` : virtio_pci_device::setup_vq, 这个在 virtio_pci_legacy_probe 中间初始化
          - 使用 virtio_pci_legacy.c::setup_vq 作为例子
              - iowrite16(index, vp_dev->ioaddr + VIRTIO_PCI_QUEUE_SEL); // 告诉选择的数值是哪一个 queue
              - ioread16(vp_dev->ioaddr + VIRTIO_PCI_QUEUE_NUM); // 读 bar 0 约定的配置空间，得到 queue 的大小
              - vring_create_virtqueue
                 - 在这里，有一个参数 vp_nofify 作为 callback 函数
                 - vring_alloc_queue : 分配的页面是连续物理内存
              - iowrite32(q_pfn, vp_dev->ioaddr + VIRTIO_PCI_QUEUE_PFN); // 告诉 kvmtool，virtqueue 准备好了

## virtio guset 接受中断

- common_interrupt
  - __common_interrupt
    - handle_irq
      - generic_handle_irq_desc
        - handle_fasteoi_irq
          - handle_irq_event
            - handle_irq_event_percpu
              - __handle_irq_event_percpu
                - vp_interrupt
                  - vring_interrupt
                  - vp_config_changed

vp 发送中断的时候，总是 vp_interrupt 来接受，然后分为
vring_interrupt
vp_config_changed
## virtio guest 发送中断

- do_syscall_64
  - do_syscall_x64
    - __x64_sys_execve
      - __se_sys_execve
        - __do_sys_execve
          - do_execve
            - do_execveat_common
              - bprm_execve
                - bprm_execve
                  - exec_binprm
                    - search_binary_handler
                      - prepare_binprm
                        - kernel_read
                          - __kernel_read
                            - xfs_file_read_iter
                              - xfs_file_buffered_read
                                - generic_file_read_iter
                                  - filemap_read
                                    - filemap_get_pages
                                      - page_cache_sync_readahead
                                        - page_cache_sync_ra
                                          - ondemand_readahead
                                            - page_cache_ra_order
                                              - read_pages
                                                - blk_finish_plug
                                                  - blk_finish_plug
                                                    - __blk_flush_plug
                                                      - blk_mq_flush_plug_list
                                                        - __blk_mq_flush_plug_list
                                                          - __blk_mq_flush_plug_list
                                                            - virtio_queue_rqs
                                                              - virtqueue_notify
                                                                - vp_notify

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
