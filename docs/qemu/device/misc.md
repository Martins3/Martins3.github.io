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

```txt
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
