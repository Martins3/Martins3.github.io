# nvme

默认的情况下，使用的 sata 协议访问，


Block devices are significant components of the Linux kernel.
In this article, we introduce the usage of QEMU to emulate some of these block devices, including *SCSI*, NVMe, Virtio and NVDIMM. [^1]

- [ ] SCSI 怎么回事 ?
- [ ] NVDIMM 怎么回事

实际上，按照[^1]的教程添加的参数并没有办法让内核检测到这个 nvme 设备，但是 info qtree 中可以看到, [^2] 提供更加详细的内容
最后，发现因为内核没有包含 nvme 模块而已。

[^1]: https://blogs.oracle.com/linux/how-to-emulate-block-devices-with-qemu
[^2]: https://github.com/manishrma/nvme-qemu

## trace 一下 nvme 设备
- [x] 没有了网络 stack 的拆解，如何对于 nvme 进行模拟
  - 来什么数据，发送什么数据，只是简单的拷贝吧，更加简单

- [x] qemu 下存在两个 nvme.c
  - hw/block/nvme.c 是用于模拟 nvme 设备的
  - block/nvme.c 是 nvme 设备穿透的

```c
/*
#0  blk_aio_prwv (blk=0x5555568dc990, offset=4275045376, bytes=1024, iobuf=0x7ffdc4000ee0, co_entry=co_entry@entry=0x555555c8a3e0 <blk_aio_read_entry>, flags=0, cb=0x55
555587ece0 <dma_blk_cb>, opaque=0x7ffdc4000e80) at ../block/block-backend.c:1433
#1  0x0000555555c8b345 in blk_aio_preadv (blk=<optimized out>, offset=<optimized out>, qiov=<optimized out>, flags=<optimized out>, cb=<optimized out>, opaque=<optimize
d out>) at ../block/block-backend.c:1540
#2  0x000055555587ee5f in dma_blk_cb (ret=<optimized out>, opaque=0x7ffdc4000e80) at ../softmmu/dma-helpers.c:191
#3  dma_blk_cb (opaque=0x7ffdc4000e80, ret=<optimized out>) at ../softmmu/dma-helpers.c:127
#4  0x000055555587f4bf in dma_blk_io (ctx=0x5555565d09b0, sg=0x5555579f6300, offset=4275045376, align=512, io_func=0x55555587eab0 <dma_blk_read_io_func>, io_func_opaque
=0x5555568dc990, cb=0x555555a27050 <ide_dma_cb>, opaque=0x5555579f5fd8, dir=DMA_DIRECTION_FROM_DEVICE) at ../softmmu/dma-helpers.c:255
#5  0x000055555587f5fe in dma_blk_read (blk=0x5555568dc990, sg=sg@entry=0x5555579f6300, offset=offset@entry=4275045376, align=align@entry=512, cb=cb@entry=0x555555a2705
0 <ide_dma_cb>, opaque=opaque@entry=0x5555579f5fd8) at ../softmmu/dma-helpers.c:273
#6  0x0000555555a273dc in ide_dma_cb (opaque=0x5555579f5fd8, ret=<optimized out>) at ../hw/ide/core.c:935
#7  0x00005555558b9506 in bmdma_cmd_writeb (bm=bm@entry=0x5555579f7130, val=val@entry=9) at /home/maritns3/core/kvmqemu/include/hw/ide/pci.h:61
#8  0x00005555558d612a in bmdma_write (opaque=0x5555579f7130, addr=<optimized out>, val=9, size=<optimized out>) at ../hw/ide/piix.c:75
#9  0x0000555555b8d7c0 in memory_region_write_accessor (mr=0x5555579f7280, addr=0, value=<optimized out>, size=1, shift=<optimized out>, mask=<optimized out>, attrs=...
) at ../softmmu/memory.c:491
#10 0x0000555555b8c0ee in access_with_adjusted_size (addr=0, value=0x7fffd97f80a8, size=1, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=0
x555555b8d730 <memory_region_write_accessor>, mr=0x5555579f7280, attrs=...) at ../softmmu/memory.c:552
#11 0x0000555555b8fc47 in memory_region_dispatch_write (mr=0x5555579f7280, addr=0, data=<optimized out>, op=<optimized out>, attrs=...) at ../softmmu/memory.c:1502
#12 0x0000555555bd7090 in flatview_write_continue (fv=0x7ffdcc2dcc30, addr=49216, attrs=..., ptr=<optimized out>, len=1, addr1=<optimized out>, l=<optimized out>, mr=0x
5555579f7280) at /home/maritns3/core/kvmqemu/include/qemu/host-utils.h:164
#13 0x0000555555bd72a6 in flatview_write (fv=0x7ffdcc2dcc30, addr=49216, attrs=..., buf=0x7fffeaf7f000, len=1) at ../softmmu/physmem.c:2786
#14 0x0000555555bd9f66 in address_space_write (as=0x5555564e0ba0 <address_space_io>, addr=49216, attrs=..., buf=0x7fffeaf7f000, len=1) at ../softmmu/physmem.c:2878
#15 0x0000555555bb8839 in kvm_handle_io (count=1, size=1, direction=<optimized out>, data=<optimized out>, attrs=..., port=49216) at ../accel/kvm/kvm-all.c:2256
#16 kvm_cpu_exec (cpu=cpu@entry=0x555556a861e0) at ../accel/kvm/kvm-all.c:2507
#17 0x0000555555b77185 in kvm_vcpu_thread_fn (arg=arg@entry=0x555556a861e0) at ../accel/kvm/kvm-accel-ops.c:49
#18 0x0000555555d249a3 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:521
#19 0x00007ffff6096609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#20 0x00007ffff5fbb293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

```c
/*
#0  huxueshi () at ../block/block-backend.c:1426
#1  0x0000555555c8b215 in blk_aio_prwv (blk=0x55555691b4a0, offset=0, bytes=4096, iobuf=0x555556942dd0, co_entry=0x555555c8a2e0 <blk_aio_read_entry>, flags=0, cb=0x5555
5587ece0 <dma_blk_cb>, opaque=0x555556942d70) at ../block/block-backend.c:1439
#2  0x0000555555c8b275 in blk_aio_preadv (blk=<optimized out>, offset=<optimized out>, qiov=<optimized out>, flags=<optimized out>, cb=<optimized out>, opaque=<optimize
d out>) at ../block/block-backend.c:1549
#3  0x000055555587ee5f in dma_blk_cb (ret=<optimized out>, opaque=0x555556942d70) at ../softmmu/dma-helpers.c:191
#4  dma_blk_cb (opaque=0x555556942d70, ret=<optimized out>) at ../softmmu/dma-helpers.c:127
#5  0x000055555587f4bf in dma_blk_io (ctx=0x5555565d09b0, sg=sg@entry=0x555557b158e8, offset=offset@entry=0, align=align@entry=512, io_func=io_func@entry=0x55555587eab0
 <dma_blk_read_io_func>, io_func_opaque=io_func_opaque@entry=0x55555691b4a0, cb=0x5555559508c0 <nvme_rw_cb>, opaque=0x555557b15850, dir=DMA_DIRECTION_FROM_DEVICE) at ..
/softmmu/dma-helpers.c:255
#6  0x000055555587f5fe in dma_blk_read (blk=blk@entry=0x55555691b4a0, sg=sg@entry=0x555557b158e8, offset=offset@entry=0, align=align@entry=512, cb=cb@entry=0x5555559508
c0 <nvme_rw_cb>, opaque=opaque@entry=0x555557b15850) at ../softmmu/dma-helpers.c:273
#7  0x000055555595353a in nvme_blk_read (req=0x555557b15850, cb=0x5555559508c0 <nvme_rw_cb>, offset=0, blk=0x55555691b4a0) at ../hw/block/nvme.c:1221
#8  nvme_read (req=0x555557b15850, n=0x8) at ../hw/block/nvme.c:2962
#9  nvme_io_cmd (n=n@entry=0x555557c96d20, req=req@entry=0x555557b15850) at ../hw/block/nvme.c:3643
#10 0x0000555555955c73 in nvme_process_sq (opaque=opaque@entry=0x555556964cf0) at ../hw/block/nvme.c:5128
#11 0x0000555555d3ed28 in timerlist_run_timers (timer_list=0x5555565d07c0) at ../util/qemu-timer.c:586
#12 timerlist_run_timers (timer_list=0x5555565d07c0) at ../util/qemu-timer.c:511
#13 0x0000555555d3ef37 in qemu_clock_run_all_timers () at ../util/qemu-timer.c:682
#14 0x0000555555d21949 in main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:541
#15 0x0000555555ae9631 in qemu_main_loop () at ../softmmu/runstate.c:725
#16 0x000055555582b4c2 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:50
```
