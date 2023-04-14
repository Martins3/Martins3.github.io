# QEMU 还有的挑战
- [ ] QEMU 的 blocking layer 如此复杂
- [ ] 为什么 QEMU 中的 block 感觉比网络复杂很多的。
- [ ] 顶层的 job.c 的内容是什么?

阅读 QIOChannel 的时候发现:
- [ ] AioContext 是什么，和 qemu/threads.md 重新联合起来分析一下。
- [ ] coroutine 的作用?
    - 我想要核实一下，当使用 coroutine 的时候，性能相对于 poll 模式其实就是已经不行了。
- [ ] `g_autoptr` 是什么? 例如在 `fd_chr_add_watch` 中看到的。
- [ ] docs/devel/qapi-code-gen.txt 和 qmp 如何工作的，是如何生成的。

- [ ] 我们发现 guest 中的 ip 总是 10.0.2.5 ，但是，实际上，可以给 guest 一个 LAN 的 ip，似乎是可以让 guest dhcp 直接和物理环境中的网关的沟通的
## qmp

- [ ] `qmp_block_commit` 的唯一调用者是如何被生成的。
- [ ] QEMU 这种可以接受命令行的参数吗?
```txt
-blockdev '{
"driver": "file",
"node-name": "protocol-node",
"filename": "foo.qcow2"
}'
```
- [ ] `OBJECT_DECLARE_SIMPLE_TYPE` 是什么意思，和类似的 macro 有什么区别

## 图形显示
之前在使用 QEMU 安装 nixos 的时候，发现有时候 alacirtty 不能全屏，似乎如果将 -vga virtio 修改为 -vga std 就可以解决


## 为什么总是需要创建一个 bridge 让 Linux 访问网络啊
- https://myme.no/posts/2021-11-25-nixos-home-assistant.html
- https://gist.github.com/extremecoders-re/e8fd8a67a515fee0c873dcafc81d811c

# None pci device
有趣的 80 : https://stackoverflow.com/questions/6793899/what-does-the-0x80-port-address-connect-to

- i8257 : 最爱的 DMA 控制器
- mc146818rtc : 时钟
- kvm-i8259 : 中断控制器
- i8042 : 接入到 isa 总线上的

- [ ] pc_superio_init : 中间竟然处理了 a20 线
- [ ] i8042 的 ioport 是约定，还是通过 bios 告诉的

## i8257 : dma
i8257_dma_init

- [An overview of direct memory access](https://geidav.wordpress.com/2014/04/27/an-overview-of-direct-memory-access)
- [How does DMA work with PCI Express devices?](https://stackoverflow.com/questions/27470885/how-does-dma-work-with-pci-express-devices)

> Today’s computers don’t contain DMA controllers anymore.

实际操作是，通过写 pcie 设备的 mmio 空间，让设备开始进行传输，当设备传输完成之后，设备通过中断的方式加以通知。

```c
    // first reset the DMA controllers
    outb(0, PORT_DMA1_MASTER_CLEAR); // d
    outb(0, PORT_DMA2_MASTER_CLEAR); // da

    // then initialize the DMA controllers
    outb(0xc0, PORT_DMA2_MODE_REG);
    outb(0x00, PORT_DMA2_MASK_REG);
```

```txt
  0000000000000000-0000000000000007 (prio 0, i/o): dma-chan
  0000000000000008-000000000000000f (prio 0, i/o): dma-cont


  0000000000000081-0000000000000083 (prio 0, i/o): dma-page
  0000000000000087-0000000000000087 (prio 0, i/o): dma-page
  0000000000000089-000000000000008b (prio 0, i/o): dma-page
  000000000000008f-000000000000008f (prio 0, i/o): dma-page

  00000000000000c0-00000000000000cf (prio 0, i/o): dma-chan
  00000000000000d0-00000000000000df (prio 0, i/o): dma-cont
```

在 seabios 的使用位置仅仅在 dma.c 中， 因为 dma_floppy 不会调用，实际上，只有 dma_setup 被使用的

实际上，在内核中有使用:
```c
>>> p /x ((CPUX86State *)current_cpu->env_ptr)->eip
$1 = 0xffffffff81738d84
```
disass 其位置，在 fd_disable_dma 上，所以 dma 暂时不用考虑了

## port 92

## ata
- [ ] 在 info qtree 中间暂时找不到啊

## rtc
主要发生在 xqm/hw/rtc/mc146818rtc.c 中间
- [ ] qemu_system_wakeup_request
  - 既然调用到这里了，那么说明之前存在让 guest 睡眠的情况

```c
cmos: read index=0x0f val=0x00
cmos: write index=0x0f val=0x00
cmos: read index=0x38 val=0x30
cmos: read index=0x3d val=0x12
cmos: read index=0x38 val=0x30
cmos: read index=0x08 val=0x10
cmos: read index=0x5f val=0x00
cmos: read index=0x08 val=0x10
cmos: read index=0x5f val=0x00
cmos: read index=0x00 val=0x58
cmos: write index=0x0a val=0x26
cmos: read index=0x0b val=0x02
cmos: write index=0x0b val=0x02
cmos: read index=0x0c val=0x00
cmos: read index=0x0d val=0x80
cmos: read index=0x0a val=0x26
cmos: read index=0x00 val=0x58
cmos: read index=0x02 val=0x12
cmos: read index=0x04 val=0x16
cmos: read index=0x32 val=0x20
cmos: read index=0x00 val=0x58
cmos: read index=0x10 val=0x50
cmos: read index=0x00 val=0x58
cmos: read index=0x00 val=0x58
cmos: read index=0x39 val=0x01
cmos: read index=0x00 val=0x58
cmos: read index=0x0f val=0x00
cmos: read index=0x00 val=0x58
cmos: read index=0x0a val=0x26
cmos: read index=0x00 val=0x59
cmos: read index=0x02 val=0x12
cmos: read index=0x04 val=0x16
cmos: read index=0x07 val=0x31
cmos: read index=0x08 val=0x10
cmos: read index=0x09 val=0x21
cmos: read index=0x0b val=0x02
cmos: read index=0x0d val=0x80
cmos: read index=0x00 val=0x59
cmos: read index=0x0a val=0x26
cmos: read index=0x00 val=0x59
cmos: read index=0x02 val=0x12
cmos: read index=0x04 val=0x16
cmos: read index=0x07 val=0x31
cmos: read index=0x08 val=0x10
cmos: read index=0x09 val=0x21
cmos: read index=0x0b val=0x02
cmos: read index=0x0a val=0x26
cmos: read index=0x00 val=0x59
kcmos: read index=0x10 val=0x50
cmos: read index=0x10 val=0x50
```

# 分析一下可能需要模拟的设备

看来一下，感觉其实还好吧！

| Device           | Strategy             |
|------------------|----------------------|
| port92           |                      |
| ioport80         |                      |
| ioportF0         |                      |
| rtc              |                      |
| isa-debugcon     |                      |
| pci-conf-idx     |                      |
| pci-conf-data    |                      |
| fwcfg            |                      |
| fwcfg.dma        |                      |
| io               |                      |
| apm-io           |                      |
| rtc-index        |                      |
| vga              |                      |
| vbe              |                      |
| i8042-cmd        |                      |
| i8042-data       |                      |
| parallel         |                      |
| serial           |                      |
| kvmvapic         |                      |
| pcspk            | speaker 暂时不用考虑 |
| acpi-cnt         |                      |
| acpi-evt         |                      |
| acpi-gpe0        |                      |
| acpi-cpu-hotplug |                      |
| acpi-tmr         |                      |
| dma-page         |                      |
| dma-cont         |                      |
| fdc              |                      |
| e1000-io         |                      |
| piix-bmdma       |                      |
| bmdma            |                      |
| ide              |                      |

## vbe
https://wiki.osdev.org/VBE

## debugcon
创建的位置
```txt
#0  debugcon_isa_realizefn (dev=0x5555579225c0, errp=0x7fffffffcc80) at /home/maritns3/core/xqm/hw/char/debugcon.c:99
#1  0x0000555555a25435 in device_set_realized (obj=<optimized out>, value=<optimized out>, errp=0x7fffffffcda8) at /home/maritns3/core/xqm/hw/core/qdev.c:876
#2  0x0000555555bb1deb in property_set_bool (obj=0x5555579225c0, v=<optimized out>, name=<optimized out>, opaque=0x555557922480, errp=0x7fffffffcda8) at /home/maritns3
/core/xqm/qom/object.c:2078
#3  0x0000555555bb65d4 in object_property_set_qobject (obj=obj@entry=0x5555579225c0, value=value@entry=0x555557922b80, name=name@entry=0x555555db1285 "realized", errp=
errp@entry=0x7fffffffcda8) at /home/maritns3/core/xqm/qom/qom-qobject.c:26
#4  0x0000555555bb3e0a in object_property_set_bool (obj=0x5555579225c0, value=<optimized out>, name=0x555555db1285 "realized", errp=0x7fffffffcda8) at /home/maritns3/c
ore/xqm/qom/object.c:1336
#5  0x00005555559b7d01 in qdev_device_add (opts=0x55555650a4b0, errp=<optimized out>) at /home/maritns3/core/xqm/qdev-monitor.c:673
#6  0x00005555559ba4e3 in device_init_func (opaque=<optimized out>, opts=<optimized out>, errp=0x555556424eb0 <error_fatal>) at /home/maritns3/core/xqm/vl.c:2212
#7  0x0000555555cc1fa2 in qemu_opts_foreach (list=<optimized out>, func=0x5555559ba4d0 <device_init_func>, opaque=0x0, errp=0x555556424eb0 <error_fatal>) at /home/mari
tns3/core/xqm/util/qemu-option.c:1170
#8  0x000055555582b15c in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4372
```

## pit

```txt
#0  i8254_pit_init (base=64, alt_irq=0x555556a04740, isa_irq=-1, bus=0x55555667b1a0) at /home/maritns3/core/xqm/include/hw/timer/i8254.h:57
#1  pc_basic_device_init (isa_bus=0x55555667b1a0, gsi=<optimized out>, rtc_state=rtc_state@entry=0x7fffffffcf38, create_fdctrl=create_fdctrl@entry=true, no_vmport=<opt
imized out>, has_pit=<optimized out>, hpet_irqs=4) at /home/maritns3/core/xqm/hw/i386/pc.c:1433
#2  0x0000555555913c02 in pc_init1 (machine=0x55555659a000, pci_type=0x555555d75e48 "i440FX", host_type=0x555555d74f1b "i440FX-pcihost") at /home/maritns3/core/xqm/hw/
i386/pc_piix.c:235
#3  0x0000555555a2c693 in machine_run_board_init (machine=0x55555659a000) at /home/maritns3/core/xqm/hw/core/machine.c:1143
#4  0x000055555582b0b8 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4348
```

# qemu overview

- [ ] ./replay 只有 1000 行左右，值得分析一下。
- [ ] tcg 相关联的代码在什么位置 ?

## qga
生成一个运行在虚拟机中间的程序，然后和 host 之间进行通信。

## structure
- 入口应该是 ./softmmu/main.c

- virtio
  - hw/block/virtio-blk.c
  - hw/net/virtio-blk.c
  - hw/virtio

## chardev
chardev 的一种使用方法[^2][^3], 可以将在 host 和 guest 中间同时创建设备，然后 guest 和 host 通过该设备进行交互。
-chardev 表示在 host 创建的设备，需要有一个 id, -device 指定该设备

- [x] -device virtio-serial 是必须的 ?
  - [x] 和 -device virtio-serial-bus 的关系是什么 ?
    - 似乎只有存在了 virtio-serial-bus 之后才可以将  virtio console 挂载到上面去

./chardev 就是为了支持方式将 guest 的数据导出来, 但是 guest 那边的数据一般来说 virtio 设备了
./hw/char 中间是为了对于 guest 的模拟和 host 端的 virtio 实现

## blockdev
- qemu 的 image 是支持多种模式的, 而 kvmtool 只是支持一个模式，如果
- qcow2 : qemu copy on write 的 image 格式

blockdev 文件夹下为了支持各种种类 image 访问方法，甚至可以直接访问 nvme 的方法


## capstone
- 显然 capstone 是被调用过的，在 qemu 看到的代码都是直接一条条的分析的

编译方法
```plain
➜  capstone git:(master) ✗ CAPSTONE_ARCHS="x86" bear make -j10
```

和 capstone 的玩耍:
- ./capstone
- [ref](http://www.capstone-engine.org/lang_c.html)

其实每一个架构的代码是很少的

## migration
- [ ] 有意思的东西
## scsi
scsi 多增加了一个抽象层次，导致其性能上有稍微的损失，但是存在别的优势。[^5][^6]
> Shortcomings of virtio-blk include a small feature set (requiring frequent updates to both the host and the guests) and limited scalability. [^7]

和实际上，scsi 文件夹下和 vritio 关系不大，反而是用于 persistent reservation
https://qemu.readthedocs.io/en/latest/tools/qemu-pr-helper.html

- [ ] pr 只是利用了 scsi 机制，但是非要使用 scsi, 不知道

## trace
- [ ] 为什么需要使用 ftrace，非常的 interesting !



[^1]: https://developer.apple.com/documentation/hypervisor
[^2]: https://stackoverflow.com/questions/63357744/qemu-socket-communication
[^3]: https://wiki.qemu.org/Features/ChardevFlowControl
[^4]: https://qkxu.github.io/2019/03/24/Qemu-Guest-Agent-(QGA)%E5%8E%9F%E7%90%86%E7%AE%80%E4%BB%8B.html
[^7]: https://wiki.qemu.org/Features/SCSI

## 为什么在 QEMU 中执行 stress-ng --vm-bytes 1000M --vm-keep -m 1 ，但是再 host 中的 htop 中可以观测到两个 CPU 非常繁忙

## dirty ring 的机制
- https://lwn.net/Articles/825743/

## 简单分析一下 QEMU 中的信号机制，如果够复杂，可以单独整理一下

- kill -s SIGBUS $(pgrep qemu)  导致的结果就是 QEMU 死掉

- [ ] kvm_init_cpu_signals : 可以进一步深入调查下

简而言之，就是如果 host 遇到了 SIGBUS 信号，那么错误首先会注入给 Guest 一下，最终 QEMU 会因为 SIGBUS 信号而挂掉。
- qemu_init_sigbus
  - sigbus_handler
    - kvm_on_sigbus_vcpu
      - kvm_on_sigbus
        - kvm_arch_on_sigbus_vcpu
      - sigbus_reraise

## [ ] MemoryListener::commit 的注册用户可以专门分析下

## [ ] 嵌套虚拟化的环境如果搭建的完善之后
1. vIOMMU 可以分析调试一下如何使用

## 最近调试脚本的遇到的问题
-bootindex 为什么只能跟在 -device 后面，而不是跟在 -drive 后面

- 才发现 -device 到底是如何实现的?
  - qemu-system-x86_64 -device help 可以查看后面可以跟什么？

```sh
arg_sata="-device virtio-scsi-pci,id=scsi -device scsi-hd,drive=jj,bootindex=10 -drive file=${workstation}/img4,format=raw,id=jj"
```

```txt
(qemu) qemu-system-x86_64: -device scsi-hd,drive=jj,bootindex=10: Drive 'jj' is already in use because it has been automatically connected to another
device (did you need 'if=none' in the drive options?)
```
这里的 if=none 是什么意思？


如果想要使用 -device scsi-hd 的时候，那么必须增加上
```txt
-device virtio-scsi-pci,id=scsi
```
