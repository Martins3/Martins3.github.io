# virtio mm

- https://virtio-mem.gitlab.io/
- ppt : https://events19.linuxfoundation.org/wp-content/uploads/2017/12/virtio-mem-Paravirtualized-Memory-David-Hildenbrand-Red-Hat-1.pdf
- https://lwn.net/Articles/813638/

当时没有 virtio-mem 的时候对比 hotplug 和 balloon:
- Hotplug or Ballooning: A Comparative Study on Dynamic Memory Management Techniques for Virtual Machines

- [ ] virtio mm 似乎需要额外的考虑: CONFIG_PM_SLEEP
- [ ] virtio mm 是如何依赖本来的 hotplug 框架的 ?
  - 创建 page struct 等，直接映射等
- [ ] 直接在 QEMU 中插入一个新的 memory 设备不行吗? 为什么非要发明一个新的机制。
  - 似乎 virtio-mem 还是需要使用 memory hotplug 的
- [ ] 似乎 balloon 不会修改 watermark，但是 virito mm 会修改 watermark
  - [ ] 为什么 watermark 是按照总内存大小来设置，因为内核需要额外的内存来管理总的物理页面吗?

- [ ] 能不能让 guest 只是释放它认为可以释放的，而不是说让 host 决定那些必须释放，然后这些页被一定被清理了
  - [ ] 如果是，QEMU 需要接受一个 list 来被告知那些 4M 的 memory block 被 guest 释放了
- [ ] QEMU 释放的方法是什么 ? 还是曾经的 balloon 中使用 memory advise 还是 unmap ?

- [ ] 为什么不支持 VFIO 。
  - 是不是说，当内存被分配为 VFIO 的 dma 区域之后，无法被 unplug ，但是这种情况在 virtio-balloon 中也是如此啊。
  - 还是说，存在 VFIO，virtio-pmem 直接无法正常工作的

- 存在 guest oom 给 QEMU 提示的情况吗?

## keynote
nohup stress --vm-bytes 6000M --vm-keep -m 1 &

- drivers/base/memory.c : hotplug 的公共模块吧

## [virtio-mem: Paravirtualized Memory by David Hildenbrand](https://www.youtube.com/watch?v=H65FDUDPu9s)

## 相对于 balloon 的优缺点
- [ ] 将页面删除的时候，balloon 只是找到这些内存就可以了，而 hotplug 需要让 guest 的 migration 启动
- [ ] 会导致 host 碎片化吗?
  - 有什么方法防止碎片化?
- [ ] 对于 hotplug 的内存，内核有没有一种说法，就是尽量不要去使用这些内存。


## 问题
- [ ] 为什么需要架构支持?

## [ ]  sbm 和 bbm

## 记录几个 backtrace

这个被调用了两次:
```txt
#0  virtio_mem_init_hotplug (vm=0xffff88810078fc00) at drivers/virtio/virtio_mem.c:2456
#1  virtio_mem_init (vm=0xffff88810078fc00) at drivers/virtio/virtio_mem.c:2694
#2  virtio_mem_probe (vdev=0xffff888024580000) at drivers/virtio/virtio_mem.c:2782
#3  0xffffffff81721aaa in virtio_dev_probe (_d=0xffff888024580010) at drivers/virtio/virtio.c:305
#4  0xffffffff8194ec14 in call_driver_probe (drv=0xffffffff82c04380 <virtio_mem_driver>, dev=0xffff888024580010) at drivers/base/dd.c:530
#5  really_probe (dev=dev@entry=0xffff888024580010, drv=drv@entry=0xffffffff82c04380 <virtio_mem_driver>) at drivers/base/dd.c:609
#6  0xffffffff8194ee3d in __driver_probe_device (drv=drv@entry=0xffffffff82c04380 <virtio_mem_driver>, dev=dev@entry=0xffff888024580010) at drivers/base/dd.c:748
#7  0xffffffff8194eeb9 in driver_probe_device (drv=drv@entry=0xffffffff82c04380 <virtio_mem_driver>, dev=dev@entry=0xffff888024580010) at drivers/base/dd.c:778
#8  0xffffffff8194f5e6 in __driver_attach (data=0xffffffff82c04380 <virtio_mem_driver>, dev=0xffff888024580010) at drivers/base/dd.c:1150
#9  __driver_attach (dev=0xffff888024580010, data=0xffffffff82c04380 <virtio_mem_driver>) at drivers/base/dd.c:1099
#10 0xffffffff8194cc03 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff82c04380 <virtio_mem_driver>, fn=fn@entry=0xffffffff8194f540 <__driver_attach>) at drivers/base/bus.c:301
#11 0xffffffff8194e775 in driver_attach (drv=drv@entry=0xffffffff82c04380 <virtio_mem_driver>) at drivers/base/dd.c:1167
#12 0xffffffff8194e1cc in bus_add_driver (drv=drv@entry=0xffffffff82c04380 <virtio_mem_driver>) at drivers/base/bus.c:618
#13 0xffffffff8195015a in driver_register (drv=0xffffffff82c04380 <virtio_mem_driver>) at drivers/base/driver.c:240
#14 0xffffffff81000e7f in do_one_initcall (fn=0xffffffff833332c4 <virtio_mem_driver_init>) at init/main.c:1296
#15 0xffffffff832f44b8 in do_initcall_level (command_line=0xffff8881001393c0 "root", level=6) at init/main.c:1369
#16 do_initcalls () at init/main.c:1385
#17 do_basic_setup () at init/main.c:1404
#18 kernel_init_freeable () at init/main.c:1623 #19 0xffffffff81efb9f1 in kernel_init (unused=<optimized out>) at init/main.c:1512
#20 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

- [ ] plug 上是一个异步的操作?
```txt
#0  virtio_mem_sbm_add_mb (mb_id=40, vm=0xffff88810078fc00) at drivers/virtio/virtio_mem.c:665
#1  virtio_mem_sbm_plug_and_add_mb (vm=vm@entry=0xffff88810078fc00, mb_id=40, nb_sb=nb_sb@entry=0xffffc900001e7e48) at drivers/virtio/virtio_mem.c:1629
#2  0xffffffff8172c8c3 in virtio_mem_sbm_plug_request (diff=<optimized out>, vm=0xffff88810078fc00) at drivers/virtio/virtio_mem.c:1743
#3  virtio_mem_plug_request (diff=<optimized out>, vm=0xffff88810078fc00) at drivers/virtio/virtio_mem.c:1857
#4  virtio_mem_run_wq (work=0xffff88810078fc10) at drivers/virtio/virtio_mem.c:2378
#5  0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff88810062e480, work=0xffff88810078fc10) at kernel/workqueue.c:2289
#6  0xffffffff811232c8 in worker_thread (__worker=0xffff88810062e480) at kernel/workqueue.c:2436
#7  0xffffffff81129c73 in kthread (_create=0xffff88810062f4c0) at kernel/kthread.c:376
#8  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

```txt
#0  virtio_mem_memory_notifier_cb (nb=0xffff88810078fdd0, action=8, arg=0xffffc900001e7be0) at drivers/virtio/virtio_mem.c:952
#1  0xffffffff8112d1f8 in notifier_call_chain (nr_calls=0x0 <fixed_percpu_data>, nr_to_call=-9, v=0xffffc900001e7be0, val=8, nl=0xffffffff82c18a28 <memory_chain+40>) at kernel/notifier.c:87
#2  blocking_notifier_call_chain (v=0xffffc900001e7be0, val=8, nh=0xffffffff82c18a00 <memory_chain>) at kernel/notifier.c:382
#3  blocking_notifier_call_chain (nh=nh@entry=0xffffffff82c18a00 <memory_chain>, val=val@entry=8, v=v@entry=0xffffc900001e7be0) at kernel/notifier.c:370
#4  0xffffffff819680a2 in memory_notify (val=val@entry=8, v=v@entry=0xffffc900001e7be0) at drivers/base/memory.c:175
#5  0xffffffff81efd35e in online_pages (pfn=pfn@entry=1310720, nr_pages=nr_pages@entry=32768, zone=zone@entry=0xffff88807fffcd00, group=0xffff8881221d8040) at mm/memory_hotplug.c:1102
#6  0xffffffff81967d0c in memory_block_online (mem=0xffff88810017e800) at drivers/base/memory.c:202
#7  memory_block_action (action=1, mem=0xffff88810017e800) at drivers/base/memory.c:268
#8  memory_block_change_state (from_state_req=4, to_state=1, mem=0xffff88810017e800) at drivers/base/memory.c:293
#9  memory_subsys_online (dev=0xffff88810017e820) at drivers/base/memory.c:315
#10 0xffffffff8194c5ad in device_online (dev=0xffff88810017e820) at drivers/base/core.c:4048
#11 0xffffffff819683fd in walk_memory_blocks (start=start@entry=5368709120, size=size@entry=134217728, arg=arg@entry=0x0 <fixed_percpu_data>, func=func@entry=0xffffffff812e93b0 <online_memory_block>) at drivers/base/memory.c:969
#12 0xffffffff81efd77d in add_memory_resource (nid=nid@entry=0, res=res@entry=0xffff8881221d8100, mhp_flags=mhp_flags@entry=5) at mm/memory_hotplug.c:1421
#13 0xffffffff812ea686 in add_memory_driver_managed (nid=0, start=start@entry=5368709120, size=size@entry=134217728, resource_name=<optimized out>, mhp_flags=mhp_flags@entry=5) at mm/memory_hotplug.c:1500
#14 0xffffffff8172a071 in virtio_mem_add_memory (vm=vm@entry=0xffff88810078fc00, addr=addr@entry=5368709120, size=134217728) at drivers/virtio/virtio_mem.c:647
#15 0xffffffff8172b82e in virtio_mem_sbm_add_mb (mb_id=40, vm=0xffff88810078fc00) at drivers/virtio/virtio_mem.c:668
#16 virtio_mem_sbm_plug_and_add_mb (vm=vm@entry=0xffff88810078fc00, mb_id=40, nb_sb=nb_sb@entry=0xffffc900001e7e48) at drivers/virtio/virtio_mem.c:1629
#17 0xffffffff8172c8c3 in virtio_mem_sbm_plug_request (diff=<optimized out>, vm=0xffff88810078fc00) at drivers/virtio/virtio_mem.c:1743
#18 virtio_mem_plug_request (diff=<optimized out>, vm=0xffff88810078fc00) at drivers/virtio/virtio_mem.c:1857
#19 virtio_mem_run_wq (work=0xffff88810078fc10) at drivers/virtio/virtio_mem.c:2378
#20 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff88810062e480, work=0xffff88810078fc10) at kernel/workqueue.c:2289
#21 0xffffffff811232c8 in worker_thread (__worker=0xffff88810062e480) at kernel/workqueue.c:2436
#22 0xffffffff81129c73 in kthread (_create=0xffff88810062f4c0) at kernel/kthread.c:376
#23 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```
- virtio_mem_sbm_add_mb 和 virtio_mem_memory_notifier_cb 一共会被调用几百次。

注意，调用过 memory_block_online ，这是 acpi 和 virtio-mem 的共同路径。

## 一个猜测: virtio-mem 比 native hotplug 多出细粒度调整

- 应该是的，virtio-mem 的粒度是 4M
- [ ] native hotplug 是 ?

### [ ] 测试一下 qom-set vm0 size 64M 的效果

### qom-set vm0 requested-size 64M

- virtio_mem_run_wq
  - virtio_mem_plug_request
  - virtio_mem_unplug_request

在 virtio_mem_retry 中会启动 virtio_mem_run_wq
```txt
#0  virtio_mem_retry (vm=<optimized out>) at drivers/virtio/virtio_mem.c:789
#1  virtio_mem_config_changed (vdev=0xffff8881001e1000) at drivers/virtio/virtio_mem.c:2901
#2  0xffffffff81721b43 in __virtio_config_changed (dev=0xffff8881001e1000) at drivers/virtio/virtio.c:133
#3  virtio_config_changed (dev=dev@entry=0xffff8881001e1000) at drivers/virtio/virtio.c:141
#4  0xffffffff81726a83 in vp_config_changed (opaque=0xffff8881001e1000, irq=11) at drivers/virtio/virtio_pci_common.c:54
#5  vp_interrupt (irq=11, opaque=0xffff8881001e1000) at drivers/virtio/virtio_pci_common.c:97
#6  0xffffffff811658d1 in __handle_irq_event_percpu (desc=desc@entry=0xffff888003920e00) at kernel/irq/handle.c:158
#7  0xffffffff81165a7f in handle_irq_event_percpu (desc=0xffff888003920e00) at kernel/irq/handle.c:193
#8  handle_irq_event (desc=desc@entry=0xffff888003920e00) at kernel/irq/handle.c:210
#9  0xffffffff81169c1b in handle_fasteoi_irq (desc=0xffff888003920e00) at kernel/irq/chip.c:714
#10 0xffffffff810b9ad4 in generic_handle_irq_desc (desc=0xffff888003920e00) at include/linux/irqdesc.h:158
#11 handle_irq (regs=<optimized out>, desc=0xffff888003920e00) at arch/x86/kernel/irq.c:231
#12 __common_interrupt (regs=<optimized out>, vector=34) at arch/x86/kernel/irq.c:250
#13 0xffffffff81ef93f3 in common_interrupt (regs=0xffffc900000bbe38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc90000101018
```

在 virtio_mem_retry 中，会进一步的调用 retry 机制。
```txt
#0  virtio_mem_retry (vm=<optimized out>) at drivers/virtio/virtio_mem.c:789
#1  virtio_mem_offline_and_remove_memory (addr=<optimized out>, size=<optimized out>, vm=<optimized out>) at drivers/virtio/virtio_mem.c:748
#2  virtio_mem_offline_and_remove_memory (size=<optimized out>, addr=<optimized out>, vm=<optimized out>) at drivers/virtio/virtio_mem.c:731
#3  virtio_mem_sbm_offline_and_remove_mb (mb_id=<optimized out>, vm=<optimized out>) at drivers/virtio/virtio_mem.c:766
#4  virtio_mem_sbm_unplug_any_sb_online (nb_sb=<optimized out>, mb_id=<optimized out>, vm=<optimized out>) at drivers/virtio/virtio_mem.c:1999
#5  virtio_mem_sbm_unplug_any_sb (nb_sb=<optimized out>, mb_id=<optimized out>, vm=<optimized out>) at drivers/virtio/virtio_mem.c:2032
#6  virtio_mem_sbm_unplug_request (diff=1006632960, vm=0xffff888100155800) at drivers/virtio/virtio_mem.c:2073
#7  virtio_mem_unplug_request (diff=1006632960, vm=0xffff888100155800) at drivers/virtio/virtio_mem.c:2257
#8  virtio_mem_run_wq (work=0xffff888100155810) at drivers/virtio/virtio_mem.c:2381
#9  0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff888141a0bb40, work=0xffff888100155810) at kernel/workqueue.c:2289
#10 0xffffffff811232c8 in worker_thread (__worker=0xffff888141a0bb40) at kernel/workqueue.c:2436
#11 0xffffffff81129c73 in kthread (_create=0xffff888100725d80) at kernel/kthread.c:376
#12 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#13 0x0000000000000000 in ?? ()
```

## [ ] 看看因为收缩，导致的 migration 的效果和 swap 的效果

- [ ] 如果总是尽量将所有的 page 都来 migrate，然后这些 page 因为内存压力，立刻开始进行 swap，那还不如直接开始 swap 的

```txt
0  offline_pages (start_pfn=start_pfn@entry=1540096, nr_pages=nr_pages@entry=32768, zone=0xffff88807fffcd00, group=0xffff8880040ba740) at mm/memory_hotplug.c:1793
#1  0xffffffff81968233 in memory_block_offline (mem=0xffff88814049ac00) at drivers/base/memory.c:240
#2  memory_block_action (action=4, mem=0xffff88814049ac00) at drivers/base/memory.c:271
#3  memory_block_change_state (from_state_req=1, to_state=4, mem=0xffff88814049ac00) at drivers/base/memory.c:293
#4  memory_subsys_offline (dev=0xffff88814049ac20) at drivers/base/memory.c:328
#5  0xffffffff8194cb7c in device_offline (dev=0xffff88814049ac20) at drivers/base/core.c:4019
#6  device_offline (dev=dev@entry=0xffff88814049ac20) at drivers/base/core.c:4003
#7  0xffffffff812e9ccd in try_offline_memory_block (mem=mem@entry=0xffff88814049ac00, arg=arg@entry=0xffffc90000d2fdd0) at mm/memory_hotplug.c:2193
#8  0xffffffff81968a5d in walk_memory_blocks (start=start@entry=6308233216, size=size@entry=134217728, arg=arg@entry=0xffffc90000d2fdd0, func=func@entry=0xffffffff812e9c70 <try_offline_memory_block>) at drivers/base/memory.c:969
#9  0xffffffff812e9aa5 in offline_and_remove_memory (start=start@entry=6308233216, size=size@entry=134217728) at mm/memory_hotplug.c:2259
#10 0xffffffff8172d30d in virtio_mem_offline_and_remove_memory (size=134217728, addr=6308233216, vm=0xffff888100155800) at drivers/virtio/virtio_mem.c:741
#11 virtio_mem_sbm_offline_and_remove_mb (mb_id=47, vm=0xffff888100155800) at drivers/virtio/virtio_mem.c:766
#12 virtio_mem_sbm_unplug_any_sb_online (nb_sb=0xffffc90000d2fe48, mb_id=47, vm=0xffff888100155800) at drivers/virtio/virtio_mem.c:1999
#13 virtio_mem_sbm_unplug_any_sb (nb_sb=0xffffc90000d2fe48, mb_id=47, vm=0xffff888100155800) at drivers/virtio/virtio_mem.c:2032
#14 virtio_mem_sbm_unplug_request (diff=<optimized out>, vm=0xffff888100155800) at drivers/virtio/virtio_mem.c:2073
#15 virtio_mem_unplug_request (diff=<optimized out>, vm=0xffff888100155800) at drivers/virtio/virtio_mem.c:2257
#16 virtio_mem_run_wq (work=0xffff888100155810) at drivers/virtio/virtio_mem.c:2381
#17 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff888141a0bb40, work=0xffff888100155810) at kernel/workqueue.c:2289
#18 0xffffffff811232c8 in worker_thread (__worker=0xffff888141a0bb40) at kernel/workqueue.c:2436
#19 0xffffffff81129c73 in kthread (_create=0xffff888100725d80) at kernel/kthread.c:376
#20 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

在 migrate_pages 上打点:

- [ ] 似乎用起来的时候，是有 bug 的，那还搞个蛇皮。
  - 好像无法复现了


压力上来的时候，本来就有:
```txt
#0  migrate_pages (from=from@entry=0xffffc900001bfe00, get_new_page=get_new_page@entry=0xffffffff812aed80 <compaction_alloc>, put_new_page=put_new_page@entry=0xffffffff812ad1a0 <compaction_free>, private=private@entry=18446683600571858416, mode=MIGRATE_SYNC_LIGHT, reason=reason@entry=0, ret_succeeded=0xffffc900001bfdac) at mm/migrate.c:1398
#1  0xffffffff812b191e in compact_zone (cc=cc@entry=0xffffc900001bfdf0, capc=capc@entry=0x0 <fixed_percpu_data>) at mm/compaction.c:2416
#2  0xffffffff812b2383 in proactive_compact_node (pgdat=pgdat@entry=0xffff88813fff9000) at mm/compaction.c:2668
#3  0xffffffff812b2810 in kcompactd (p=0xffff88813fff9000) at mm/compaction.c:2978
#4  0xffffffff81129c73 in kthread (_create=0xffff888004258f80) at kernel/kthread.c:376
#5  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

分析一下，因为 virtio-mem 到底导致的 swap 的原因:

其实，非常有道理，利用上 `alloc_contig_range`，如果内存实在是不够，swap 应该是可以在主动触发出来，和 kthread 中来触发。
```txt
#0  start_isolate_page_range (start_pfn=start_pfn@entry=1566208, end_pfn=end_pfn@entry=1566720, migratetype=migratetype@entry=1, flags=flags@entry=0, gfp_flags=3264) at mm/page_isolation.c:534
#1  0xffffffff812e8896 in alloc_contig_range (start=start@entry=1566208, end=end@entry=1566720, migratetype=migratetype@entry=1, gfp_mask=gfp_mask@entry=3264) at mm/page_alloc.c:9218
#2  0xffffffff8172a772 in virtio_mem_fake_offline (pfn=pfn@entry=1566208, nr_pages=nr_pages@entry=512) at drivers/virtio/virtio_mem.c:1171
#3  0xffffffff8172bce9 in virtio_mem_sbm_unplug_sb_online (vm=vm@entry=0xffff88810016a000, mb_id=mb_id@entry=47, sb_id=sb_id@entry=51, count=count@entry=1) at drivers/virtio/virtio_mem.c:1920
#4  0xffffffff8172cb1a in virtio_mem_sbm_unplug_any_sb_online (nb_sb=0xffffc90000d17e48, mb_id=47, vm=0xffff88810016a000) at drivers/virtio/virtio_mem.c:1983
#5  virtio_mem_sbm_unplug_any_sb (nb_sb=0xffffc90000d17e48, mb_id=47, vm=0xffff88810016a000) at drivers/virtio/virtio_mem.c:2032
#6  virtio_mem_sbm_unplug_request (diff=<optimized out>, vm=0xffff88810016a000) at drivers/virtio/virtio_mem.c:2073
#7  virtio_mem_unplug_request (diff=<optimized out>, vm=0xffff88810016a000) at drivers/virtio/virtio_mem.c:2257
#8  virtio_mem_run_wq (work=0xffff88810016a010) at drivers/virtio/virtio_mem.c:2381
#9  0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff88814248e3c0, work=0xffff88810016a010) at kernel/workqueue.c:2289
#10 0xffffffff811232c8 in worker_thread (__worker=0xffff88814248e3c0) at kernel/workqueue.c:2436
#11 0xffffffff81129c73 in kthread (_create=0xffff8881424f8540) at kernel/kthread.c:376
#12 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#13 0x0000000000000000 in ?? ()
```

```txt
#0  swap_writepage (page=0xffffea0005b13d40, wbc=0xffffc9000129f828) at mm/page_io.c:185
#1  0xffffffff81293437 in pageout (folio=folio@entry=0xffffea0005b13d40, mapping=mapping@entry=0xffff888005676000, plug=plug@entry=0xffffc9000129f8f0) at mm/vmscan.c:1265
#2  0xffffffff81293f1a in shrink_page_list (page_list=page_list@entry=0xffffc9000129f9c8, pgdat=pgdat@entry=0xffff88807fffc000, sc=sc@entry=0xffffc9000129fba8, stat=stat@entry=0xffffc9000129fa50, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1886
#3  0xffffffff81296478 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc9000129fba8, lruvec=0xffff888144eaa400, nr_to_scan=<optimized out>) at mm/vmscan.c:2447
#4  shrink_list (sc=0xffffc9000129fba8, lruvec=0xffff888144eaa400, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2674
#5  shrink_lruvec (lruvec=lruvec@entry=0xffff888144eaa400, sc=sc@entry=0xffffc9000129fba8) at mm/vmscan.c:2991
#6  0xffffffff81296cd4 in shrink_node_memcgs (sc=0xffffc9000129fba8, pgdat=0xffff88807fffc000) at mm/vmscan.c:3180
#7  shrink_node (pgdat=pgdat@entry=0xffff88807fffc000, sc=sc@entry=0xffffc9000129fba8) at mm/vmscan.c:3304
#8  0xffffffff81297f00 in shrink_zones (sc=0xffffc9000129fba8, zonelist=<optimized out>) at mm/vmscan.c:3542
#9  do_try_to_free_pages (zonelist=zonelist@entry=0xffff88807fffda00, sc=sc@entry=0xffffc9000129fba8) at mm/vmscan.c:3601
#10 0xffffffff812989ea in try_to_free_pages (zonelist=0xffff88807fffda00, order=order@entry=0, gfp_mask=gfp_mask@entry=1314250, nodemask=<optimized out>) at mm/vmscan.c:3836
#11 0xffffffff812e6757 in __perform_reclaim (ac=0xffffc9000129fd30, order=0, gfp_mask=1314250) at mm/page_alloc.c:4726
#12 __alloc_pages_direct_reclaim (did_some_progress=<synthetic pointer>, ac=0xffffc9000129fd30, alloc_flags=2240, order=0, gfp_mask=1314250) at mm/page_alloc.c:4748
#13 __alloc_pages_slowpath (gfp_mask=<optimized out>, gfp_mask@entry=1314250, order=order@entry=0, ac=ac@entry=0xffffc9000129fd30) at mm/page_alloc.c:5151
#14 0xffffffff812e72f8 in __alloc_pages (gfp=gfp@entry=1314250, order=order@entry=0, preferred_nid=<optimized out>, nodemask=0x0 <fixed_percpu_data>) at mm/page_alloc.c:5528
#15 0xffffffff812e7bf2 in __folio_alloc (gfp=gfp@entry=1052106, order=order@entry=0, preferred_nid=<optimized out>, nodemask=<optimized out>) at mm/page_alloc.c:5546
#16 0xffffffff813055ca in vma_alloc_folio (gfp=gfp@entry=1052106, order=order@entry=0, vma=vma@entry=0xffff888102dc1c80, addr=<optimized out>, hugepage=hugepage@entry=false) at mm/mempolicy.c:2231
#17 0xffffffff812c4443 in alloc_page_vma (addr=<optimized out>, vma=0xffff888102dc1c80, gfp=1052106) at include/linux/gfp.h:290
#18 do_anonymous_page (vmf=0xffffc9000129fdf8) at mm/memory.c:4084
#19 handle_pte_fault (vmf=0xffffc9000129fdf8) at mm/memory.c:4909
#20 __handle_mm_fault (vma=vma@entry=0xffff888102dc1c80, address=address@entry=140390932926480, flags=flags@entry=597) at mm/memory.c:5053
#21 0xffffffff812c4a40 in handle_mm_fault (vma=0xffff888102dc1c80, address=address@entry=140390932926480, flags=flags@entry=597, regs=regs@entry=0xffffc9000129ff58) at mm/memory.c:5151
#22 0xffffffff810f2cd3 in do_user_addr_fault (regs=regs@entry=0xffffc9000129ff58, error_code=error_code@entry=6, address=address@entry=140390932926480) at arch/x86/mm/fault.c:1397
#23 0xffffffff81efbe32 in handle_page_fault (address=140390932926480, error_code=6, regs=0xffffc9000129ff58) at arch/x86/mm/fault.c:1488
#24 exc_page_fault (regs=0xffffc9000129ff58, error_code=6) at arch/x86/mm/fault.c:1544
#25 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

## BBM : Big Block Mode
- https://lwn.net/Articles/837008/

- 其实不是非常懂，为什么 BBM 需要特殊的支持:

```c
/*
 * Try to unplug the requested amount of memory.
 */
static int virtio_mem_unplug_request(struct virtio_mem *vm, uint64_t diff)
{
    if (vm->in_sbm)
        return virtio_mem_sbm_unplug_request(vm, diff);
    return virtio_mem_bbm_unplug_request(vm, diff);
}
```

## virtio-mem: Paravirtualized Memory Hot(Un)Plug
On the other hand, memory ballooning, i.e., controlling a VM’s memory footprint by relocating pages between the VM and its hypervisor,
either has to tolerate that even well-behaving VMs might exceed memory limits and **reuse inflated memory**, making it impossible to detect malicious VMs,
or has to enforce memory limits and disallow access to inflated memory, which results in problematic reboot handling.
- 猜测是因为 guest 内核被 hacking 过之后，balloon 就会不安全，但是 virtio-mem 可以回收之后立刻将这些内存让 QEMU 直接无法访问
- 猜测，在 balloon 的情况下，memory 被隔离之后，如果重启，这些 memory 需要被重新释放给 guest ?

- [ ] virtio-mem 收缩之后，会让 QEMU 通知 KVM 修改 memslot 吗?

(Un)plug requests are rejected while the VM is getting migrated; as one example, discarding pages is problematic during postcopy live migration [16] in QEMU

- [ ] 为什么不要在迁移的时候 plug / unplug ?

When processing requests, the bitmap is queried and modified on success, accordingly.
Initially, all blocks are unplugged. Just as most VIRTIO request in QEMU, requests are handled by a single thread, avoiding the need for manual locking.

System Reset Handling. On QEMU system resets, all device-managed memory is discarded and the bitmap is cleared, resulting in all device-managed memory being unplugged after a reboot.

- [ ] 为什么需要使用 bitmap 管理 ?

**Resizeable Allocations.**

- [ ] 如此迷茫? 这一个段都没有看懂是用于描述 QEMU 的什么的

- [ ] 据说是在 driver init 的时候确定 blocksize 的
  - [ ] 那么之后可以修改吗 ?

- 似乎有的内存加入到之后，并不会立刻提交给 Linux

Once all subblocks of a Linux memory block are unplugged by virtio-mem, we trigger offlining and removal from Linux.
**We extended memory offlining code in Linux to skip pages that are marked logically offline by a driver, but are still looking like ordinarily allocated pages.**

## 分析一下 QEMU’s pc-dimm
