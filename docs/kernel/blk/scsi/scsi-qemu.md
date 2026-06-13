## qemu 是如何支持 scsi 的


这是 virtio-scsi 的调用路线:
```txt
#0  scsi_req_enqueue (req=req@entry=0x555557037c20) at ../hw/scsi/scsi-bus.c:903
#1  0x0000555555be53ef in virtio_scsi_handle_cmd_req_submit (req=<optimized out>, s=0x555557f29970) at ../hw/scsi/virtio-scsi.c:810
#2  virtio_scsi_handle_cmd_vq (vq=<optimized out>, s=0x555557f29970) at ../hw/scsi/virtio-scsi.c:853
#3  virtio_scsi_handle_cmd (vq=<optimized out>, vdev=<optimized out>) at ../hw/scsi/virtio-scsi.c:867
#4  virtio_scsi_handle_cmd (vdev=0x555557f29970, vq=<optimized out>) at ../hw/scsi/virtio-scsi.c:857
#5  0x0000555555bff33f in virtio_queue_notify_vq (vq=0x7ffddc0d8140) at ../hw/virtio/virtio.c:2263
#6  0x0000555555e0c875 in aio_dispatch_handler (ctx=ctx@entry=0x555556801a80, node=0x7ffdd011e590) at ../util/aio-posix.c:372
```

这是 lsi53c895a 的调用路线:

```txt
#0  scsi_req_enqueue (req=0x7ffdd494f520) at ../hw/scsi/scsi-bus.c:903
#1  0x00005555559ec074 in lsi_do_command (s=0x555558358e10) at ../hw/scsi/lsi53c895a.c:869
#2  lsi_execute_script (s=0x555558358e10) at ../hw/scsi/lsi53c895a.c:1271
#3  0x0000555555c250b0 in memory_region_write_accessor (mr=0x555558359a70, addr=47, value=<optimized out>, size=1, shift=<optimized out>, mask=<optimized out>, attrs=...) at ../system/memory.c:497
```

## 主要文件分析
- hw/scsi/scsi-bus.c : 这个有点难
- hw/block/cdrom.c : scsi cd 的实现
- hw/scsi/virtio-scsi.c : virtio-scsi 具体的实现
- hw/scsi/scsi-disk.c : scsi sd 实现


### hw/scsi/scsi-disk.c
```c
static const SCSIReqOps *const scsi_disk_reqops_dispatch[256] = { }

// 主要是定义类似这些内容
static const struct SCSIReqOps reqops_unit_attention = {
    .size         = sizeof(SCSIRequest),
    .init_req     = scsi_fetch_unit_attention_sense,
    .send_command = scsi_unit_attention
};
```

这里具体实现在 scsi-bus.c 中注册的内容:

- scsi_disk_emulate_command
- scsi_block_dma_readv

lsi53c895a 和 vritio-scsi 下面挂载的设备,
设置方法为 `-device scsi-hd` 是从这里来的

```c
static const TypeInfo scsi_hd_info = {
    .name          = "scsi-hd",
    .parent        = TYPE_SCSI_DISK_BASE,
    .class_init    = scsi_hd_class_initfn,
};
```

## 问题
观察 scsi-bus.c 的实现，发现了如下问题，让我开始好奇整个 qemu 是如何实现总线机制的
有时间可以对比 scsi-bus.c hw/usb/bus.c

1. 如何理解 TYPE_BUS ，TYPE_BUS 一共存在 32 个应用的地方
```c
static const TypeInfo scsi_bus_info = {
    .name = TYPE_SCSI_BUS,
    .parent = TYPE_BUS,
    .instance_size = sizeof(SCSIBus),
    .class_init = scsi_bus_class_init,
    .interfaces = (InterfaceInfo[]) {
        { TYPE_HOTPLUG_HANDLER },
        { }
    }
};
```
2. 从 kernel 的角度来说，几乎看不到 scsi bus 的概念，qemu 的中出现了 bus 的概念，那么 scsi device 和 virtio-scsi HBA 卡和 scsi bus 是如何交互的，
或者说，是如何互相关联起来的

3. scsi-bus.c 是提供了一些公共的函数吗?

## ncr53c710 咋用来着?

hw/scsi/ncr53c710.c 是一个 Google summer project 的项目

默认不会编译，需要在 qemu 的目录中添加一个 .config ，其中的内容为
```txt
CONFIG_LASI=y
```

这是由于
```txt
config LASI
    bool
    select LASI_82596
    select LASIPS2
    select NCR710_SCSI
    select PARALLEL
    select SERIAL_MM
```
这个时候，可以确认 hw/scsi/ncr53c710.c 一定被编译进去了，不不能
问题在于 ./qemu-system-x86_64 -device help 找不到这个设备了

hw/net/i82596.c 这个网卡也是类似的，他们都是依赖 lasi 的东西。

## 之前的各种尝试
这里可以配置为 megasas-gen2 或者 megasas ，有两个设备
2025-11-18 最新内核和 qemu ，都是可以正常工作的
root@localhost:~# lspci -s 0000:00:08.0 -v
Kernel driver in use: megaraid_sas

mptsas1068

这个内核无法识别的，切换了各种版本的内核都不支持
2025-11-18 发现启动之后，会导致虚拟机启动的时候 zstd compressed data corrupted
这其实非常不科学，按道理，这个设备只会不识别，但是现在让 initramfs 中出现了问题

，但是被 https://patchwork.kernel.org/project/qemu-devel/patch/1485444454-30749-4-git-send-email-armbru@redhat.com/ 中说这个该被 deprecated
lspci -s 0000:00:09.0 -vv
Kernel driver in use: sym53c8xx
local arg_lsi53c895a="  -device lsi53c895a,id=scsi3 "

## ./scsi 目录
scsi 多增加了一个抽象层次，导致其性能上有稍微的损失，但是存在别的优势。[^5][^6]

> Shortcomings of virtio-blk include a small feature set (requiring frequent updates to both the host and the guests) and limited scalability. [^7]

和实际上，scsi 文件夹下和 vritio 关系不大，反而是用于 persistent reservation

https://qemu.readthedocs.io/en/latest/tools/qemu-pr-helper.html

- [ ] pr 只是利用了 scsi 机制，但是非要使用 scsi, 不知道

[^1]: https://developer.apple.com/documentation/hypervisor

## [ ] QEMU 中的 -cdrom 是如何实现的
qemu : hw/block/cdrom.c 超级简单的

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
