- [ ] 参考一下狼书 P322 理解一下 vring vring_avail vring_used vring_desc 和 sg 的关系
  - virtqueue 的定位是什么?
- [ ] vring_interrupt 是在 softirq 的环境中吗?

## 文档
- https://docs.oasis-open.org/virtio/virtio/v1.1/virtio-v1.1.html


## 核心的结构体
- virtio_config_ops : <操作配置空间的> 是 virtio_device 和 virtio 连接的桥梁。
  - get/set : 读写 config
  - [ ] find_vqs :
  - [ ] set_vq_affinity : 一般初始化为 vp_set_vq_affinity
    - [ ] 软中断是如何设置 affinity 的

```c
static const struct virtio_config_ops virtio_pci_config_ops = {
  .get    = vp_get,
  .set    = vp_set,
  .generation = vp_generation,
  .get_status = vp_get_status,
  .set_status = vp_set_status,
  .reset    = vp_reset,
  .find_vqs = vp_modern_find_vqs,
  .del_vqs  = vp_del_vqs,
  .get_features = vp_get_features,
  .finalize_features = vp_finalize_features,
  .bus_name = vp_bus_name,
  .set_vq_affinity = vp_set_vq_affinity,
  .get_vq_affinity = vp_get_vq_affinity,
  .get_shm_region  = vp_get_shm_region,
};
```

## vring

```txt
#0  virtqueue_add (gfp=2592, ctx=0x0 <fixed_percpu_data>, data=0xffff88814003d508, in_sgs=1, out_sgs=1, total_sg=2, sgs=0xffffc9000007bc78, _vq=0xffff888100efd900) at drivers/virtio/virtio_ring.c:2086
#1  virtqueue_add_sgs (_vq=_vq@entry=0xffff888100efd900, sgs=sgs@entry=0xffffc9000007bc78, out_sgs=out_sgs@entry=1, in_sgs=in_sgs@entry=1, data=data@entry=0xffff88814003d508, gfp=gfp@entry=2592) at drivers/virtio/virtio_ring.c:2122
#2  0xffffffff81976586 in virtblk_add_req (vq=0xffff888100efd900, vbr=vbr@entry=0xffff88814003d508) at drivers/block/virtio_blk.c:130
#3  0xffffffff81977742 in virtio_queue_rq (hctx=0xffff88814003d200, bd=0xffffc9000007bd70) at drivers/block/virtio_blk.c:353
#4  0xffffffff816121cf in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff88814003d200, list=list@entry=0xffffc9000007bdc0, nr_budgets=nr_budgets@entry=0) at block/blk-mq.c:1902
#5  0xffffffff816184c3 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff88814003d200) at block/blk-mq-sched.c:306
#6  0xffffffff816185a0 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff88814003d200) at block/blk-mq-sched.c:339
#7  0xffffffff8160eff0 in __blk_mq_run_hw_queue (hctx=0xffff88814003d200) at block/blk-mq.c:2020
#8  0xffffffff8160f2a0 in __blk_mq_delay_run_hw_queue (hctx=<optimized out>, async=<optimized out>, msecs=msecs@entry=0) at block/blk-mq.c:2096
#9  0xffffffff8160f509 in blk_mq_run_hw_queue (hctx=<optimized out>, async=async@entry=false) at block/blk-mq.c:2144
#10 0xffffffff8160f8a0 in blk_mq_run_hw_queues (q=q@entry=0xffff888100c75fc8, async=async@entry=false) at block/blk-mq.c:2192
#11 0xffffffff816103db in blk_mq_requeue_work (work=0xffff888100c761f8) at block/blk-mq.c:1361
#12 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff888003850a80, work=0xffff888100c761f8) at kernel/workqueue.c:2289
#13 0xffffffff811232c8 in worker_thread (__worker=0xffff888003850a80) at kernel/workqueue.c:2436
#14 0xffffffff81129c73 in kthread (_create=0xffff88800425b180) at kernel/kthread.c:376
#15 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

- vring_virtqueue 持有 virtqueue 和 vring_virtqueue_split
  - vring_virtqueue::notify 一般赋值为 vp_notify
  - vring_virtqueue::last_used_idx :
  - virtqueue : virtqueue::callback，对于 virtio-balloon 而言，是 balloon_ack
  - vring_virtqueue_split 持有 vring
    - vring 持有 vring_desc，vring_avail 和 vring_used 的指针


```c
struct vring {
	unsigned int num;

	vring_desc_t *desc;

	vring_avail_t *avail;

	vring_used_t *used;
};
```
- desc, avail 和 used 都是构成一个链表的。

- [ ] virtqueue 的定位是什么?


- 一个 vring_desc 不是对应一个 page 的，而是对应一个 scatterlist
- vring_map_one_sg 获取一个 scatterlist 的 GPA，从而发传递给 Host


- [ ] 有趣，分析 scatterlist 和 blk 的联系，但是没有太看懂:
  - 从 virtblk_add_req 开始分析吧

## 数据通路
- virtqueue_get_buf
  - virtqueue_get_buf_ctx : 参数 virtqueue len
    - to_vvq : 从 virtqueue 获取 vring_virtqueue
    - virtqueue_get_buf_ctx_packed
    - virtqueue_get_buf_ctx_split : 访问 vring::used 和 vring_virtqueue::last_used_idx，获取灌注的内容

- virtqueue_add_outbuf
  - virtqueue_add
    - virtqueue_add_packed
    - virtqueue_add_split
      - alloc_indirect_split
        - virtqueue_add_desc_split : 初始化 desc 的内容，让其指向 buffer，记录长度
        - vq->split.vring.avail->ring[avail] = cpu_to_virtio16(_vq->vdev, head);
        - vq->split.vring.avail->idx = cpu_to_virtio16(_vq->vdev, vq->split.avail_idx_shadow);

- virtqueue_add_split

## TODO
- [ ] 有的设备不支持 PCI 总线，需要使用 MMIO 的方式，但是 kvmtool 怎么知道这个设备需要使用 MMIO
- [ ] 约定是第一个 bar 指向的 IO 空间在内核那一侧是怎么分配的 ?
- [ ] virtio_bus 是挂载到哪里的?
- [ ] virtio_console 的具体实现是怎么样子的 ?
- [ ] 现在对于 eventfd 都是从 virt-blk 角度理解的，其实如何利用 eventfd 实现 guest 到 kernel 的通知，比如 irqfd 来实现 Qemu 直接将 irq 注入到 guest 中

- [ ] 观察，如何从 blk 的数值是如何发送的，如果一会构成 block size ，一会合并，是不是很烦 ?
  - 网络中，是不是总是需要等到一整个 page 才可以发送，还是说没有这个问题 ?
  - 对于 virtio balloon 岂不是难受，所以这个想法是有问题的
- [ ] QEMU 是如何初始化 virtio 设备的
- [ ] 热插拔，参考 virtio-mem 吧
- [ ] 如何协商 vq 的数量，有时候 vq 的数量取决于是否打开一些 feature 的?
- [ ] 据说，msix 比 pin 的方式好很多。

- [ ] 的确是记得没有错，但是 `virtio_pci_common_cfg` 是处理 pci 的，但是不知道为什么是错的

- vring_create_virtqueue_packed 和 vring_create_virtqueue_split 的差别是什么?

1. 架构层次
2. 数据传输
  - [ ] 有拷贝吗?
3. 中断
4. 睡眠

## interrupt
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

```txt
#0  vring_interrupt (irq=irq@entry=11, _vq=0xffff888100d9c400) at drivers/virtio/virtio_ring.c:2441
#1  0xffffffff8172648f in vp_vring_interrupt (irq=11, opaque=0xffff88800413d800) at drivers/virtio/virtio_pci_common.c:68
#2  0xffffffff811658d1 in __handle_irq_event_percpu (desc=desc@entry=0xffff888003920e00) at kernel/irq/handle.c:158
#3  0xffffffff81165a7f in handle_irq_event_percpu (desc=0xffff888003920e00) at kernel/irq/handle.c:193
#4  handle_irq_event (desc=desc@entry=0xffff888003920e00) at kernel/irq/handle.c:210
#5  0xffffffff81169c1b in handle_fasteoi_irq (desc=0xffff888003920e00) at kernel/irq/chip.c:714
#6  0xffffffff810b9a14 in generic_handle_irq_desc (desc=0xffff888003920e00) at include/linux/irqdesc.h:158
#7  handle_irq (regs=<optimized out>, desc=0xffff888003920e00) at arch/x86/kernel/irq.c:231
#8  __common_interrupt (regs=<optimized out>, vector=34) at arch/x86/kernel/irq.c:250
#9  0xffffffff81ef93d3 in common_interrupt (regs=0xffffc90000197bf8, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```

```txt
#0  vp_config_changed (opaque=0xffff88800413d800, irq=11) at drivers/virtio/virtio_pci_common.c:54
#1  vp_interrupt (irq=11, opaque=0xffff88800413d800) at drivers/virtio/virtio_pci_common.c:97
#2  0xffffffff811658d1 in __handle_irq_event_percpu (desc=desc@entry=0xffff888003920e00) at kernel/irq/handle.c:158
#3  0xffffffff81165a7f in handle_irq_event_percpu (desc=0xffff888003920e00) at kernel/irq/handle.c:193
#4  handle_irq_event (desc=desc@entry=0xffff888003920e00) at kernel/irq/handle.c:210
#5  0xffffffff81169c1b in handle_fasteoi_irq (desc=0xffff888003920e00) at kernel/irq/chip.c:714
#6  0xffffffff810b9a14 in generic_handle_irq_desc (desc=0xffff888003920e00) at include/linux/irqdesc.h:158
#7  handle_irq (regs=<optimized out>, desc=0xffff888003920e00) at arch/x86/kernel/irq.c:231
#8  __common_interrupt (regs=<optimized out>, vector=34) at arch/x86/kernel/irq.c:250
#9  0xffffffff81ef93d3 in common_interrupt (regs=0xffffc900000bbe38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc90000101018
```

## notify host
- 通过 vp_notify 通知给 Host 的:
```txt
#0  vp_notify (vq=0xffff888140418e00) at drivers/virtio/virtio_pci_common.c:45
#1  0xffffffff81722296 in virtqueue_notify (_vq=0xffff888140418e00) at drivers/virtio/virtio_ring.c:2231
#2  0xffffffff8197812d in virtio_queue_rqs (rqlist=0xffffc90001027b68) at drivers/block/virtio_blk.c:441
#3  0xffffffff81612ffd in __blk_mq_flush_plug_list (plug=0xffffc90001027b68, q=0xffff888101d59fc8) at block/blk-mq.c:2577
#4  __blk_mq_flush_plug_list (plug=0xffffc90001027b68, q=0xffff888101d59fc8) at block/blk-mq.c:2572
#5  blk_mq_flush_plug_list (plug=plug@entry=0xffffc90001027b68, from_schedule=from_schedule@entry=false) at block/blk-mq.c:2633
#6  0xffffffff816070e1 in __blk_flush_plug (plug=0xffffc90001027b68, plug@entry=0xffffc90001027b18, from_schedule=from_schedule@entry=false) at block/blk-core.c:1153
#7  0xffffffff81607370 in blk_finish_plug (plug=0xffffc90001027b18) at block/blk-core.c:1177
#8  blk_finish_plug (plug=plug@entry=0xffffc90001027b68) at block/blk-core.c:1174
#9  0xffffffff8128a9e7 in read_pages (rac=rac@entry=0xffffc90001027c58) at mm/readahead.c:181
#10 0xffffffff8128b10d in page_cache_ra_order (ractl=0xffffc90001027c58, ractl@entry=0x0 <fixed_percpu_data>, ra=0xffff8881475a6598, new_order=2) at mm/readahead.c:539
#11 0xffffffff8128b3bb in ondemand_readahead (ractl=ractl@entry=0x0 <fixed_percpu_data>, folio=folio@entry=0x0 <fixed_percpu_data>, req_size=req_size@entry=1) at mm/readahead.c:672
#12 0xffffffff8128b605 in page_cache_sync_ra (ractl=0x0 <fixed_percpu_data>, ractl@entry=0xffffc90001027c58, req_count=req_count@entry=1) at mm/readahead.c:699
#13 0xffffffff8127ece6 in page_cache_sync_readahead (req_count=1, index=0, file=0xffff8881475a6500, ra=0xffff8881475a6598, mapping=0xffff8881068e6eb0) at include/linux/pagemap.h:1215
#14 filemap_get_pages (iocb=iocb@entry=0xffffc90001027e28, iter=iter@entry=0xffffc90001027e00, fbatch=fbatch@entry=0xffffc90001027d00) at mm/filemap.c:2566
#15 0xffffffff8127f304 in filemap_read (iocb=iocb@entry=0xffffc90001027e28, iter=iter@entry=0xffffc90001027e00, already_read=0) at mm/filemap.c:2660
#16 0xffffffff81280f20 in generic_file_read_iter (iocb=iocb@entry=0xffffc90001027e28, iter=iter@entry=0xffffc90001027e00) at mm/filemap.c:2806
#17 0xffffffff81560d0b in xfs_file_buffered_read (iocb=0xffffc90001027e28, to=0xffffc90001027e00) at fs/xfs/xfs_file.c:277
#18 0xffffffff81560df5 in xfs_file_read_iter (iocb=<optimized out>, to=<optimized out>) at fs/xfs/xfs_file.c:302
#19 0xffffffff8134a5e3 in __kernel_read (file=0xffff8881475a6500, buf=buf@entry=0xffff8881427266a0, count=count@entry=256, pos=pos@entry=0xffffc90001027e90) at fs/read_write.c:428
#20 0xffffffff8134a796 in kernel_read (file=<optimized out>, buf=buf@entry=0xffff8881427266a0, count=count@entry=256, pos=pos@entry=0xffffc90001027e90) at fs/read_write.c:446
#21 0xffffffff81352809 in prepare_binprm (bprm=0xffff888142726600) at fs/exec.c:1664
#22 search_binary_handler (bprm=0xffff888142726600) at fs/exec.c:1718
#23 exec_binprm (bprm=0xffff888142726600) at fs/exec.c:1775
#24 bprm_execve (flags=<optimized out>, filename=<optimized out>, fd=<optimized out>, bprm=0xffff888142726600) at fs/exec.c:1844
#25 bprm_execve (bprm=0xffff888142726600, fd=<optimized out>, filename=<optimized out>, flags=<optimized out>) at fs/exec.c:1806
#26 0xffffffff81352dfd in do_execveat_common (fd=fd@entry=-100, filename=0xffff888004379000, flags=0, envp=..., argv=..., envp=..., argv=...) at fs/exec.c:1949
#27 0xffffffff8135301e in do_execve (__envp=0x7fffbacdd978, __argv=0x7fffbacdafe0, filename=<optimized out>) at fs/exec.c:2023
#28 __do_sys_execve (envp=0x7fffbacdd978, argv=0x7fffbacdafe0, filename=<optimized out>) at fs/exec.c:2099
#29 __se_sys_execve (envp=<optimized out>, argv=<optimized out>, filename=<optimized out>) at fs/exec.c:2094
#30 __x64_sys_execve (regs=<optimized out>) at fs/exec.c:2094
#31 0xffffffff81ef8c2b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001027f58) at arch/x86/entry/common.c:50
#32 do_syscall_64 (regs=0xffffc90001027f58, nr=<optimized out>) at arch/x86/entry/common.c:80
```

## msix vs intx

- [ ] vp_find_vqs_intx : 为什么 balloon 总是调用 int 而不是 msi ，而且只有 balloon 会调用这个
  - [ ] virtio-balloon 的中断有

```txt
#0  vp_find_vqs_intx (ctx=0x0 <fixed_percpu_data>, names=0xffffc9000005fcf8, callbacks=0xffffc9000005fcd0, vqs=0xffffc9000005fca8, nvqs=5, vdev=0xffff8880041da000) at include/linux/slab.h:605
#1  vp_find_vqs (vdev=vdev@entry=0xffff8880041da000, nvqs=5, vqs=0xffffc9000005fca8, callbacks=0xffffc9000005fcd0, names=0xffffc9000005fcf8, ctx=0x0 <fixed_percpu_data>, desc=0x0 <fixed_percpu_data>) at drivers/virtio/virtio_pci_common.c:416
#2  0xffffffff817502d2 in vp_modern_find_vqs (vdev=0xffff8880041da000, nvqs=<optimized out>, vqs=<optimized out>, callbacks=<optimized out>, names=<optimized out>, ctx=<optimized out>, desc=0x0 <fixed_percpu_data>) at drivers/virtio/virtio_pci_modern.c:355
#3  0xffffffff81751e8d in virtio_find_vqs (desc=0x0 <fixed_percpu_data>, names=0xffffc9000005fcf8, callbacks=0xffffc9000005fcd0, vqs=0xffffc9000005fca8, nvqs=5, vdev=<optimized out>) at include/linux/virtio_config.h:227
#4  init_vqs (vb=vb@entry=0xffff8880041dc800) at drivers/virtio/virtio_balloon.c:527
#5  0xffffffff81752475 in virtballoon_probe (vdev=0xffff8880041da000) at drivers/virtio/virtio_balloon.c:888
#6  0xffffffff8174bd5a in virtio_dev_probe (_d=0xffff8880041da010) at drivers/virtio/virtio.c:305
#7  0xffffffff819842a4 in call_driver_probe (drv=0xffffffff82c0b880 <virtio_balloon_driver>, dev=0xffff8880041da010) at drivers/base/dd.c:560
#8  really_probe (dev=dev@entry=0xffff8880041da010, drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/dd.c:639
#9  0xffffffff819844cd in __driver_probe_device (drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>, dev=dev@entry=0xffff8880041da010) at drivers/base/dd.c:778
#10 0xffffffff81984549 in driver_probe_device (drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>, dev=dev@entry=0xffff8880041da010) at drivers/base/dd.c:808
#11 0xffffffff81984c59 in __driver_attach (data=0xffffffff82c0b880 <virtio_balloon_driver>, dev=0xffff8880041da010) at drivers/base/dd.c:1190
#12 __driver_attach (dev=0xffff8880041da010, data=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/dd.c:1134
#13 0xffffffff81982253 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff82c0b880 <virtio_balloon_driver>, fn=fn@entry=0xffffffff81984bf0 <__driver_attach>) at drivers/base/bus.c:301
#14 0xffffffff81983dc5 in driver_attach (drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/dd.c:1207
#15 0xffffffff8198381c in bus_add_driver (drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/bus.c:618
#16 0xffffffff8198581a in driver_register (drv=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/driver.c:246
#17 0xffffffff81000e7f in do_one_initcall (fn=0xffffffff8337a042 <virtio_balloon_driver_init>) at init/main.c:1303
#18 0xffffffff8333b4c7 in do_initcall_level (command_line=0xffff888003e31780 "root", level=6) at init/main.c:1376
#19 do_initcalls () at init/main.c:1392
#20 do_basic_setup () at init/main.c:1411
#21 kernel_init_freeable () at init/main.c:1631
#22 0xffffffff81fafc01 in kernel_init (unused=<optimized out>) at init/main.c:1519
#23 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

- [ ] msi 相对于 int 的性能优势是什么?

## 设备的初始化
- virtio_device::private 指向具体的 virtio 设备
- virtio_blk / virtio_balloon 具体的设备只有 virtio_device 指针

- virtio_pci_device 包含 virtio_device

系统启动的过程中，一共会 probe 两次:

```txt
#0  virtio_pci_modern_probe (vp_dev=vp_dev@entry=0xffff888100fc1000) at drivers/virtio/virtio_pci_modern.c:531
#1  0xffffffff81750885 in virtio_pci_probe (pci_dev=0xffff888100e50000, id=<optimized out>) at drivers/virtio/virtio_pci_common.c:551
#2  0xffffffff816c933d in local_pci_probe (_ddi=_ddi@entry=0xffffc9000005fd60) at drivers/pci/pci-driver.c:324
#3  0xffffffff816caaf9 in pci_call_probe (id=<optimized out>, dev=0xffff888100e50000, drv=<optimized out>) at drivers/pci/pci-driver.c:392
#4  __pci_device_probe (pci_dev=0xffff888100e50000, drv=<optimized out>) at drivers/pci/pci-driver.c:417
#5  pci_device_probe (dev=0xffff888100e500c8) at drivers/pci/pci-driver.c:460
#6  0xffffffff819842a4 in call_driver_probe (drv=0xffffffff82c0b778 <virtio_pci_driver+120>, dev=0xffff888100e500c8) at drivers/base/dd.c:560
#7  really_probe (dev=dev@entry=0xffff888100e500c8, drv=drv@entry=0xffffffff82c0b778 <virtio_pci_driver+120>) at drivers/base/dd.c:639
#8  0xffffffff819844cd in __driver_probe_device (drv=drv@entry=0xffffffff82c0b778 <virtio_pci_driver+120>, dev=dev@entry=0xffff888100e500c8) at drivers/base/dd.c:778
#9  0xffffffff81984549 in driver_probe_device (drv=drv@entry=0xffffffff82c0b778 <virtio_pci_driver+120>, dev=dev@entry=0xffff888100e500c8) at drivers/base/dd.c:808
#10 0xffffffff81984c59 in __driver_attach (data=0xffffffff82c0b778 <virtio_pci_driver+120>, dev=0xffff888100e500c8) at drivers/base/dd.c:1190
#11 __driver_attach (dev=0xffff888100e500c8, data=0xffffffff82c0b778 <virtio_pci_driver+120>) at drivers/base/dd.c:1134
#12 0xffffffff81982253 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff82c0b778 <virtio_pci_driver+120>, fn=fn@entry=0xffffffff81984bf0 <__driver_attach>) at drivers/base/bus.c:301
#13 0xffffffff81983dc5 in driver_attach (drv=drv@entry=0xffffffff82c0b778 <virtio_pci_driver+120>) at drivers/base/dd.c:1207
#14 0xffffffff8198381c in bus_add_driver (drv=drv@entry=0xffffffff82c0b778 <virtio_pci_driver+120>) at drivers/base/bus.c:618
#15 0xffffffff8198581a in driver_register (drv=0xffffffff82c0b778 <virtio_pci_driver+120>) at drivers/base/driver.c:246
#16 0xffffffff81000e7f in do_one_initcall (fn=0xffffffff8337a02d <virtio_pci_driver_init>) at init/main.c:1303
#17 0xffffffff8333b4c7 in do_initcall_level (command_line=0xffff888003e31780 "root", level=6) at init/main.c:1376
#18 do_initcalls () at init/main.c:1392
#19 do_basic_setup () at init/main.c:1411
#20 kernel_init_freeable () at init/main.c:1631
#21 0xffffffff81fafc01 in kernel_init (unused=<optimized out>) at init/main.c:1519
#22 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

```txt
#7  0xffffffff81752475 in virtballoon_probe (vdev=0xffff888024909800) at drivers/virtio/virtio_balloon.c:888
#8  0xffffffff8174bd5a in virtio_dev_probe (_d=0xffff888024909810) at drivers/virtio/virtio.c:305
#9  0xffffffff819842a4 in call_driver_probe (drv=0xffffffff82c0b880 <virtio_balloon_driver>, dev=0xffff888024909810) at drivers/base/dd.c:560
#10 really_probe (dev=dev@entry=0xffff888024909810, drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/dd.c:639
#11 0xffffffff819844cd in __driver_probe_device (drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>, dev=dev@entry=0xffff888024909810) at drivers/base/dd.c:778
#12 0xffffffff81984549 in driver_probe_device (drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>, dev=dev@entry=0xffff888024909810) at drivers/base/dd.c:808
#13 0xffffffff81984c59 in __driver_attach (data=0xffffffff82c0b880 <virtio_balloon_driver>, dev=0xffff888024909810) at drivers/base/dd.c:1190
#14 __driver_attach (dev=0xffff888024909810, data=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/dd.c:1134
#15 0xffffffff81982253 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff82c0b880 <virtio_balloon_driver>, fn=fn@entry=0xffffffff81984bf0 <__driver_attach>) at drivers/base/bus.c:301
#16 0xffffffff81983dc5 in driver_attach (drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/dd.c:1207
#17 0xffffffff8198381c in bus_add_driver (drv=drv@entry=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/bus.c:618
#18 0xffffffff8198581a in driver_register (drv=0xffffffff82c0b880 <virtio_balloon_driver>) at drivers/base/driver.c:246
#19 0xffffffff81000e7f in do_one_initcall (fn=0xffffffff8337a042 <virtio_balloon_driver_init>) at init/main.c:1303
#20 0xffffffff8333b4c7 in do_initcall_level (command_line=0xffff888003e31780 "root", level=6) at init/main.c:1376
#21 do_initcalls () at init/main.c:1392
#22 do_basic_setup () at init/main.c:1411
#23 kernel_init_freeable () at init/main.c:1631
#24 0xffffffff81fafc01 in kernel_init (unused=<optimized out>) at init/main.c:1519
#25 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#26 0x0000000000000000 in ?? ()
```

这个参考更加清晰: ![](https://img2020.cnblogs.com/blog/1771657/202102/1771657-20210224230417971-437424297.png)

```c
static struct pci_driver virtio_pci_driver = {
  .name   = "virtio-pci",
  .id_table = virtio_pci_id_table,
  .probe    = virtio_pci_probe,
  .remove   = virtio_pci_remove,
#ifdef CONFIG_PM_SLEEP
  .driver.pm  = &virtio_pci_pm_ops,
#endif
  .sriov_configure = virtio_pci_sriov_configure,
};
```

```c
static struct bus_type virtio_bus = {
  .name  = "virtio",
  .match = virtio_dev_match,
  .dev_groups = virtio_dev_groups,
  .uevent = virtio_uevent,
  .probe = virtio_dev_probe,
  .remove = virtio_dev_remove,
};
```

```c
static struct virtio_driver virtio_blk = {
  .feature_table      = features,
  .feature_table_size   = ARRAY_SIZE(features),
  .feature_table_legacy   = features_legacy,
  .feature_table_size_legacy  = ARRAY_SIZE(features_legacy),
  .driver.name      = KBUILD_MODNAME,
  .driver.owner     = THIS_MODULE,
  .id_table     = id_table,
  .probe        = virtblk_probe,
  .remove       = virtblk_remove,
  .config_changed     = virtblk_config_changed,
#ifdef CONFIG_PM_SLEEP
  .freeze       = virtblk_freeze,
  .restore      = virtblk_restore,
#endif
};
```

这个架构很有趣，同时挂载在 pci 和 virtio 两个总线上。

## vring_size
参数 align 为 :
```c
/* The alignment to use between consumer and producer parts of vring.
 * x86 pagesize again. */
#define VIRTIO_PCI_VRING_ALIGN    4096
```
参数 num 是 VIRTIO_PCI_QUEUE_NUM, kvmtool 配置的是 16

```c
static inline unsigned vring_size(unsigned int num, unsigned long align)
{
  return ((sizeof(struct vring_desc) * num + sizeof(__virtio16) * (3 + num)
     + align - 1) & ~(align - 1))
    + sizeof(__virtio16) * 3 + sizeof(struct vring_used_elem) * num;
}
```
- [x] vring_size 含义在百度书上解释过，但是页面对齐，为什么不包含 vring_used 的
  - 从 vring_init 中间可以找到对称的数值

- [ ] vring_avail 和 vring_used 的成员都是只有两个(flags 和 idx), 为什么需要三个

- [ ] vp_setup_vq 是需要和 vp_find_vqs_intx 下调用的，和中断是关系的吗?

- 通过 vp_active_vq 将 vring_desc , vring_avail 和 vring_used 的地址发送给 QEMU 的。

### desc
如果从 multiqueue 到达了一个消息下来，那么 vring_avail 增加一个。
如果在 host 中间将任务完成了，那么 vring_used 增加一个, 在 host 通过中断的方式通知 guest 之后，guest 处理 vring_used ，并且释放

从 5.2 的描述看，vring_desc 会构成链表，描述一次 IO 的所有数据。

vring_avail 表示当前设备可以使用的 vring_desc, 这些数据从上层到达之后，vring_avail::idx++,
vring_avail::ring 描述的项目增加，如果被设备消费了，那么 last_avail_idx ++

设备每处理一个可用描述符数组 ring 的描述符链，都需要将其追加到 vring_used 数组中。

设备通过 vring_used 将告诉驱动那些数据被使用了。感觉这就是一个返回值列表罢了。

- [ ] idx 是谁维护的，两个 last_avail_idx 和 last_used_idx 是谁维护的

- [ ] 这些队列都是 guest 维护的吗? 显然不是

- 在设备侧定义 last_avail_idx ，在驱动侧定义 last_used_idx
  - vring_virtqueue::last_used_idx 例如， virtblk_done -> virtqueue_get_buf 的时候会更新的，因为设备处理数据完成。

- [ ] last_avail_idx 和 last_used_idx 都是队列的一部分，总是需要和另一个共享吧，不然队列直接被毁掉了，如何处理?

## vring_used_elem::len 是什么作用的

```txt
#0  virtqueue_get_buf_ctx_split (ctx=0x0 <fixed_percpu_data>, len=0xffffc90000003f1c, _vq=0xffff888140418e00) at drivers/virtio/virtio_ring.c:790
#1  virtqueue_get_buf_ctx (_vq=0xffff888140418e00, len=len@entry=0xffffc90000003f1c, ctx=ctx@entry=0x0 <fixed_percpu_data>) at drivers/virtio/virtio_ring.c:2282
#2  0xffffffff81722f27 in virtqueue_get_buf (_vq=<optimized out>, len=len@entry=0xffffc90000003f1c) at drivers/virtio/virtio_ring.c:2288
#3  0xffffffff819766ba in virtblk_done (vq=0xffff888140418e00) at drivers/block/virtio_blk.c:283
#4  0xffffffff81722f85 in vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2462
#5  vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2437
#6  0xffffffff811658d1 in __handle_irq_event_percpu (desc=desc@entry=0xffff888140458400) at kernel/irq/handle.c:158
#7  0xffffffff81165a7f in handle_irq_event_percpu (desc=0xffff888140458400) at kernel/irq/handle.c:193
#8  handle_irq_event (desc=desc@entry=0xffff888140458400) at kernel/irq/handle.c:210
#9  0xffffffff81169e0a in handle_edge_irq (desc=0xffff888140458400) at kernel/irq/chip.c:819
#10 0xffffffff810b9a14 in generic_handle_irq_desc (desc=0xffff888140458400) at include/linux/irqdesc.h:158
#11 handle_irq (regs=<optimized out>, desc=0xffff888140458400) at arch/x86/kernel/irq.c:231
#12 __common_interrupt (regs=<optimized out>, vector=36) at arch/x86/kernel/irq.c:250
#13 0xffffffff81efa443 in common_interrupt (regs=0xffffc90001027998, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
- 修改 vring_used_elem::len 的位置只有在 `vhost_net` 和 `virtqueue_get_buf_ctx_split` 中，`virtqueue_get_buf_ctx_split` 的调用是非常频繁的，看来 vring_used_elem::len 总是在被 host 更新
  - 而且 vhost_net 本来就是 host 的代码，所以更加说明这是 host 告诉 guest 的。
  - 的确是设备写回到 guest 驱动的数据。


- [ ] 为什么 vring_avail 没有这个 length
- [ ] 难道不能从 vring_desc 中获取吗?

[^4]: [Standardizing virtio](https://lwn.net/Articles/580186/)

## 如何理解 virtio config
- 之前一直以为是类似 PCI 配置空间，但是似乎不是
  - virtio_balloon_get_config 是做什么的

## 如何理解 virtio_rmb

## 再去整理一下这个
https://www.cnblogs.com/LoyenWang/p/14589296.html

## 为什么感觉两个 config 是不一样的
- 一个 balloon 的 config
- 一个是 PCI 的 config
