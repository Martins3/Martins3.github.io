# platform bus

## TODO

1. lspci 显示在 pci 下接入了一个 ISA bridge
```plain
00:1f.0 ISA bridge: Intel Corporation Sunrise Point LPC Controller/eSPI Controller (rev 21)
```
所以，在 isa 下到底存在设备吗?[^1]




## Notes


### [Platform Devices and Drivers](https://www.kernel.org/doc/html/latest/driver-api/driver-model/platform.html)

### Others
Platform devices are inherently not discoverable.[^2]

Platform devices are devices that typically appear as autonomous entities in the system. This includes legacy port-based devices and host bridges to peripheral buses, and most controllers integrated into system-on-chip platforms. What they usually have in common is direct addressing from a CPU bus. Rarely, a platform_device will be connected through a segment of some other kind of bus; but its registers will still be directly addressable.[^4]

Note that probe() should in general verify that the specified device hardware actually exists; sometimes platform setup code can’t be sure. The probing can use device resources, including clocks, and device platform_data. [^4]

In many cases, the memory and IRQ resources associated with the platform device are not enough to let the device’s driver work. Board setup code will often provide additional information using the device’s platform_data field to hold additional information. [^4]

[^4] 中间还讨论了 early platform device init 的问题

## code analysis
```c
total 0
drwxr-xr-x    2 root     root             0 May 24 12:41 .
drwxr-xr-x    4 root     root             0 May 24 12:41 ..
lrwxrwxrwx    1 root     root             0 May 24 12:41 PNP0103:00 -> ../../../devices/platform/PNP0103:00
lrwxrwxrwx    1 root     root             0 May 24 12:41 QEMU0002:00 -> ../../../devices/pci0000:00/QEMU0002:00
lrwxrwxrwx    1 root     root             0 May 24 12:41 alarmtimer.0.auto -> ../../../devices/pnp0/00:05/rtc/rtc0/alarmtimer.0.auto
lrwxrwxrwx    1 root     root             0 May 24 12:41 i8042 -> ../../../devices/platform/i8042
lrwxrwxrwx    1 root     root             0 May 24 12:41 pcspkr -> ../../../devices/platform/pcspkr
lrwxrwxrwx    1 root     root             0 May 24 12:41 platform-framebuffer.0 -> ../../../devices/platform/platform-framebuffer.0
lrwxrwxrwx    1 root     root             0 May 24 12:41 regulatory.0 -> ../../../devices/platform/regulatory.0
lrwxrwxrwx    1 root     root             0 May 24 12:41 serial8250 -> ../../../devices/platform/serial8250
```

```c
hw/rtc/mc146818rtc.c:1041:10:    isa->build_aml = rtc_build_aml;                                                                                                                                                                                          │
hw/char/parallel.c:652:10:    isa->build_aml = parallel_isa_build_aml;                                                                                                                                                                                    │
hw/block/fdc.c:2854:10:    isa->build_aml = fdc_isa_build_aml;                                                                                                                                                                                            │
hw/char/serial-isa.c:129:10:    isa->build_aml = serial_isa_build_aml;                                                                                                                                                                                    │
hw/input/pckbd.c:598:10:    isa->build_aml = i8042_build_aml;                                                                                                                                                                                             │
```

- [x] 实际上，并不是所有的 isa 设备 platform device?
  - 因为编译的内核没有支持对应的驱动，所以 floppy parallel 没有出现在其中
  - 即使 info qtree 和 acpi 中间都是存在这个设备的

rtc / serial8250 / rtc 三者不同的注册方式和其他的不同
## `PNP0103` 和 `QEMU0002`
在 acpi_create_platform_device 中可以看到 `PNP0103` 和 `QEMU0002` 这两个用户

似乎 PNP0103 是: https://www.runonpc.com/high-precision-event-timer-driver-downloads-update/
从反汇编出来的 dsdt.dsl 也可以看出来 `Device (HPET) { Name (_HID, EisaId ("PNP0103") /* HPET System Timer */)  // _HID: Hardware ID`

再看 acpi 相关，可以找到 `Device (FWCF) { Name (_HID, "QEMU0002")  // _HID: Hardware ID`


regulatory 在真实的物理机上也是存在的，参考 https://www.linuxfromscratch.org/blfs/view/svn/postlfs/firmware.html
用于处理无线网卡的规定的。


从下面的日志看，五个设备是早就被探测好了的:
```c
[    1.121656] huxueshi:platform_match QEMU0002:00 == alarmtimer
[    1.131440] huxueshi:platform_match PNP0103:00 == alarmtimer
[    1.141871] huxueshi:platform_match regulatory == alarmtimer
[    1.150975] huxueshi:platform_match pcspkr == alarmtimer
[    1.161217] huxueshi:platform_match platform-framebuffer == alarmtimer
[    1.186162] huxueshi:platform_match QEMU0002:00 == efi-framebuffer
[    1.196054] huxueshi:platform_match PNP0103:00 == efi-framebuffer
[    1.206521] huxueshi:platform_match regulatory == efi-framebuffer
[    1.215138] huxueshi:platform_match pcspkr == efi-framebuffer
[    1.224812] huxueshi:platform_match platform-framebuffer == efi-framebuffer
```

## [ ] acpi bus 和 platform_bus 的关系是什么?
platform_probe 在 [^4] 中描述是用于探测真正的物理设备是否存在，但是实际上，legacy driver model 的设备才会调用
```c
[    0.989154] huxueshi:platform_probe serial8250==serial8250
[    1.030243] huxueshi:platform_probe floppy==floppy
[    1.390907] huxueshi:platform_probe i8042==i8042
[    1.409823] huxueshi:platform_probe alarmtimer==alarmtimer
```

## [x] acpi 真的会探测设备吗
- [x] 所以这五个设备如何探测的 ?
- [x] acpi 真的会探测设备, 但是不是所有的设备都是从 acpi 中探测的

```plain
QEMU0002:00
PNP0103:00
regulatory
pcspkr
platform-framebuffer
```
这些设备的探测划分为两种情况, 但是这些都是存在一个共同点，会通过一个公共入口 ： platform_device_register_full


1. QEMU0002 和 PNP0103 是通过 acpi_bus_scan
```c
/*
[    0.582758] huxueshi device: 'QEMU0002:00': device_add
[    0.583320] CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.12.0+ #42
[    0.583726] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-14-g748d619-dirty-20210524_151541-maritns3-pc 04/01/2014
[    0.583726] Call Trace:
[    0.583726]  dump_stack+0x64/0x7c
[    0.583726]  device_add.cold+0x18/0x753
[    0.583726]  ? dev_set_name+0x4e/0x70
[    0.583726]  platform_device_add+0xdf/0x1e0
[    0.583726]  platform_device_register_full+0xc1/0x130
[    0.583726]  acpi_create_platform_device.part.0+0xdb/0x210
[    0.583726]  acpi_default_enumeration+0x15/0x40
[    0.583726]  acpi_bus_attach+0x29c/0x2b0
[    0.583726]  acpi_bus_attach+0x90/0x2b0
[    0.583726]  ? __device_attach+0x66/0x150
[    0.583726]  acpi_bus_attach+0x90/0x2b0
[    0.583726]  ? __device_attach+0x66/0x150
[    0.583726]  acpi_bus_attach+0x90/0x2b0
[    0.583726]  acpi_bus_scan+0x4f/0xf0
[    0.583726]  ? acpi_sleep_proc_init+0x1f/0x1f
[    0.583726]  acpi_scan_init+0x115/0x27a
[    0.583726]  acpi_init+0x3f2/0x455
[    0.583726]  do_one_initcall+0x3f/0x1b0
[    0.583726]  kernel_init_freeable+0x19e/0x1e6
[    0.583726]  ? rest_init+0xa4/0xa4
[    0.583726]  kernel_init+0x5/0xfc
[    0.583726]  ret_from_fork+0x22/0x30

[    0.583816] huxueshi device: 'PNP0103:00': device_add
[    0.584365] CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.12.0+ #42
[    0.584726] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-14-g748d619-dirty-20210524_151541-maritns3-pc 04/01/2014
[    0.584726] Call Trace:
[    0.584726]  dump_stack+0x64/0x7c
[    0.584726]  device_add.cold+0x18/0x753
[    0.584726]  ? dev_set_name+0x4e/0x70
[    0.584726]  platform_device_add+0xdf/0x1e0
[    0.584726]  platform_device_register_full+0xc1/0x130
[    0.584726]  acpi_create_platform_device.part.0+0xdb/0x210
[    0.584726]  acpi_default_enumeration+0x15/0x40
[    0.584726]  acpi_bus_attach+0x29c/0x2b0
[    0.584726]  ? __device_attach+0x66/0x150
[    0.584726]  acpi_bus_attach+0x90/0x2b0
[    0.584726]  ? __device_attach+0x66/0x150
[    0.584726]  acpi_bus_attach+0x90/0x2b0
[    0.584726]  acpi_bus_scan+0x4f/0xf0
[    0.584726]  ? acpi_sleep_proc_init+0x1f/0x1f
[    0.584726]  acpi_scan_init+0x115/0x27a
[    0.584726]  acpi_init+0x3f2/0x455
[    0.584726]  do_one_initcall+0x3f/0x1b0
[    0.584726]  kernel_init_freeable+0x19e/0x1e6
[    0.584726]  ? rest_init+0xa4/0xa4
[    0.584726]  kernel_init+0x5/0xfc
[    0.584726]  ret_from_fork+0x22/0x30
```

2. 剩下三种是靠 driver init 的
```c
/*
[    1.493913] huxueshi device: 'regulatory.0': device_add
[    1.494492] CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.12.0+ #42
[    1.494809] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-14-g748d619-dirty-20210524_151541-maritns3-pc 04/01/2014
[    1.494809] Call Trace:
[    1.494809]  dump_stack+0x64/0x7c
[    1.494809]  device_add.cold+0x18/0x753
[    1.494809]  ? dev_set_name+0x4e/0x70
[    1.494809]  platform_device_add+0xdf/0x1e0
[    1.494809]  platform_device_register_full+0xc1/0x130
[    1.494809]  ? init_rpcsec_gss+0x62/0x62
[    1.494809]  regulatory_init+0x39/0x7b
[    1.494809]  cfg80211_init+0x62/0xca
[    1.494809]  do_one_initcall+0x3f/0x1b0
[    1.494809]  kernel_init_freeable+0x19e/0x1e6
[    1.494809]  ? rest_init+0xa4/0xa4
[    1.494809]  kernel_init+0x5/0xfc
[    1.494809]  ret_from_fork+0x22/0x30

[    1.533810] huxueshi device: 'pcspkr': device_add
[    1.534338] CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.12.0+ #42
[    1.534782] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-14-g748d619-dirty-20210524_151541-maritns3-pc 04/01/2014
[    1.534782] Call Trace:
[    1.534782]  dump_stack+0x64/0x7c
[    1.534782]  device_add.cold+0x18/0x753
[    1.534782]  ? dev_set_name+0x4e/0x70
[    1.534782]  platform_device_add+0xdf/0x1e0
[    1.534782]  platform_device_register_full+0xc1/0x130
[    1.534782]  ? early_is_amd_nb+0x4d/0x4d
[    1.534782]  add_pcspkr+0x46/0x6a
[    1.534782]  do_one_initcall+0x3f/0x1b0
[    1.534782]  kernel_init_freeable+0x19e/0x1e6
[    1.534782]  ? rest_init+0xa4/0xa4
[    1.534782]  kernel_init+0x5/0xfc
[    1.534782]  ret_from_fork+0x22/0x30

[    1.541961] huxueshi device: 'platform-framebuffer.0': device_add
[    1.542626] CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.12.0+ #42
[    1.542941] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-14-g748d619-dirty-20210524_151541-maritns3-pc 04/01/2014
[    1.542941] Call Trace:
[    1.542941]  dump_stack+0x64/0x7c
[    1.542941]  device_add.cold+0x18/0x753
[    1.542941]  ? dev_set_name+0x4e/0x70
[    1.542941]  platform_device_add+0xdf/0x1e0
[    1.542941]  platform_device_register_full+0xc1/0x130
[    1.542941]  ? pci_swiotlb_late_init+0x1f/0x1f
[    1.542941]  sysfb_init+0x76/0x9a
[    1.542941]  do_one_initcall+0x3f/0x1b0
[    1.542941]  kernel_init_freeable+0x19e/0x1e6
[    1.542941]  ? rest_init+0xa4/0xa4
[    1.542941]  kernel_init+0x5/0xfc
[    1.542941]  ret_from_fork+0x22/0x30
```



## legacy driver model
似乎总是 driver 首先启动，然后在 platform_bus 下去匹配对应的 device

这几个函数的调用，说明 platform_bus 上存在那些 devices 已经是清楚的:
```c
/*
>>> bt
#0  platform_probe (_dev=0xffff88810100a010) at drivers/base/platform.c:1424
#1  0xffffffff8168b4d6 in really_probe (dev=dev@entry=0xffff88810100a010, drv=drv@entry=0xffffffff825bbe88 <serial8250_isa_driver+40>) at drivers/base/dd.c:576
#2  0xffffffff8168b711 in driver_probe_device (drv=drv@entry=0xffffffff825bbe88 <serial8250_isa_driver+40>, dev=dev@entry=0xffff88810100a010) at drivers/base/dd.c:763
#3  0xffffffff8168b9de in device_driver_attach (drv=drv@entry=0xffffffff825bbe88 <serial8250_isa_driver+40>, dev=dev@entry=0xffff88810100a010) at drivers/base/dd.c:1039
#4  0xffffffff8168ba40 in __driver_attach (dev=0xffff88810100a010, data=0xffffffff825bbe88 <serial8250_isa_driver+40>) at drivers/base/dd.c:1117
#5  0xffffffff816896e3 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff825bbe88 <serial8250_isa_driver+40 >, fn=fn@entry=0xffffffff8168b9f0 <__driver_attach>) at drivers/base/bus.c:305
#6  0xffffffff8168aec5 in driver_attach (drv=drv@entry=0xffffffff825bbe88 <serial8250_isa_driver+40>) at drivers/base/dd.c:1133
#7  0xffffffff8168a892 in bus_add_driver (drv=drv@entry=0xffffffff825bbe88 <serial8250_isa_driver+40>) at drivers/base/bus.c:622
#8  0xffffffff8168c2d7 in driver_register (drv=drv@entry=0xffffffff825bbe88 <serial8250_isa_driver+40>) at drivers/base/driver.c:171
#9  0xffffffff8168d585 in __platform_driver_register (drv=drv@entry=0xffffffff825bbe60 <serial8250_isa_driver>, owner=owner@entry=0x0 <fixed_percpu_data>) at drivers/ba se/platform.c:894
#10 0xffffffff82b7fb2d in serial8250_init () at drivers/tty/serial/8250/8250_core.c:1204
#11 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b7fa15 <serial8250_init>) at init/main.c:1249
#12 0xffffffff82b4926b in do_initcall_level (command_line=0xffff888100123400 "root", level=6) at ./include/linux/compiler.h:234
#13 do_initcalls () at init/main.c:1338
#14 do_basic_setup () at init/main.c:1358
#15 kernel_init_freeable () at init/main.c:1560
#16 0xffffffff81b8f849 in kernel_init (unused=<optimized out>) at init/main.c:1447
#17 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#18 0x0000000000000000 in ?? ()


#0  platform_probe (_dev=0xffff88810138c810) at drivers/base/platform.c:1424
#1  0xffffffff8168b4d6 in really_probe (dev=dev@entry=0xffff88810138c810, drv=drv@entry=0xffffffff825e6ae8 <i8042_driver+40>) at drivers/base/dd.c:576
#2  0xffffffff8168b711 in driver_probe_device (drv=drv@entry=0xffffffff825e6ae8 <i8042_driver+40>, dev=dev@entry=0xffff88810138c810) at drivers/base/dd.c:763
#3  0xffffffff8168b9de in device_driver_attach (drv=drv@entry=0xffffffff825e6ae8 <i8042_driver+40>, dev=dev@entry=0xffff88810138c810) at drivers/base/dd.c:1039
#4  0xffffffff8168ba40 in __driver_attach (dev=0xffff88810138c810, data=0xffffffff825e6ae8 <i8042_driver+40>) at drivers/base/dd.c:1117
#5  0xffffffff816896e3 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff825e6ae8 <i8042_driver+40>, fn=fn@
entry=0xffffffff8168b9f0 <__driver_attach>) at drivers/base/bus.c:305
#6  0xffffffff8168aec5 in driver_attach (drv=drv@entry=0xffffffff825e6ae8 <i8042_driver+40>) at drivers/base/dd.c:1133
#7  0xffffffff8168a892 in bus_add_driver (drv=drv@entry=0xffffffff825e6ae8 <i8042_driver+40>) at drivers/base/bus.c:622
#8  0xffffffff8168c2d7 in driver_register (drv=drv@entry=0xffffffff825e6ae8 <i8042_driver+40>) at drivers/base/driver.c:171
#9  0xffffffff8168dc5e in __platform_driver_register (owner=<optimized out>, drv=0xffffffff825e6ac0 <i8042_driver>) at drivers/base/platform.c:894
#10 __platform_driver_probe (drv=0xffffffff825e6ac0 <i8042_driver>, probe=<optimized out>, module=<optimized out>) at drivers/base/platform.c:962
#11 0xffffffff8168e0fa in __platform_create_bundle (driver=driver@entry=0xffffffff825e6ac0 <i8042_driver>, probe=probe@entry=0xffffffff82b879eb <i8042_probe>, res=res@e
ntry=0x0 <fixed_percpu_data>, n_res=n_res@entry=0, data=data@entry=0x0 <fixed_percpu_data>, size=size@entry=0, module=0x0 <fixed_percpu_data>) at drivers/base/platform.
c:1026
#12 0xffffffff82b884b7 in i8042_init () at drivers/input/serio/i8042.c:1629
#13 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b88094 <i8042_init>) at init/main.c:1249
#14 0xffffffff82b4926b in do_initcall_level (command_line=0xffff888100123400 "root", level=6) at ./include/linux/compiler.h:234
#15 do_initcalls () at init/main.c:1338
#16 do_basic_setup () at init/main.c:1358
#17 kernel_init_freeable () at init/main.c:1560
#18 0xffffffff81b8f849 in kernel_init (unused=<optimized out>) at init/main.c:1447
#19 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#20 0x0000000000000000 in ?? ()

>>> bt
#0  platform_probe (_dev=0xffff888101394410) at drivers/base/platform.c:1424
#1  0xffffffff8168b4d6 in really_probe (dev=dev@entry=0xffff888101394410, drv=drv@entry=0xffffffff8253a888 <alarmtimer_driver+40>) at drivers/base/dd.c:576
#2  0xffffffff8168b711 in driver_probe_device (drv=0xffffffff8253a888 <alarmtimer_driver+40>, dev=0xffff888101394410) at drivers/base/dd.c:763
#3  0xffffffff816897a9 in bus_for_each_drv (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffc90000013a30, fn=fn@entry=0xffffffff81
68b900 <__device_attach_driver>) at drivers/base/bus.c:431
#4  0xffffffff8168b386 in __device_attach (dev=dev@entry=0xffff888101394410, allow_async=allow_async@entry=true) at drivers/base/dd.c:938
#5  0xffffffff8168b98a in device_initial_probe (dev=dev@entry=0xffff888101394410) at drivers/base/dd.c:985
#6  0xffffffff8168a5f9 in bus_probe_device (dev=dev@entry=0xffff888101394410) at drivers/base/bus.c:491
#7  0xffffffff816881a0 in device_add (dev=dev@entry=0xffff888101394410) at drivers/base/core.c:3319
#8  0xffffffff8168d3ff in platform_device_add (pdev=pdev@entry=0xffff888101394400) at drivers/base/platform.c:748
#9  0xffffffff8168dfe1 in platform_device_register_full (pdevinfo=pdevinfo@entry=0xffffc90000013b60) at drivers/base/platform.c:871
#10 0xffffffff810e373c in platform_device_register_resndata (size=0, data=0x0 <fixed_percpu_data>, num=0, res=0x0 <fixed_percpu_data>, id=-2, name=0xffffffff822261c5 "a
larmtimer", parent=0xffff888101391800) at ./include/linux/platform_device.h:140
#11 platform_device_register_data (size=0, data=0x0 <fixed_percpu_data>, id=-2, name=0xffffffff822261c5 "alarmtimer", parent=0xffff888101391800) at ./include/linux/plat
form_device.h:193
#12 alarmtimer_rtc_add_device (dev=0xffff888101391800, class_intf=<optimized out>) at kernel/time/alarmtimer.c:100
#13 0xffffffff81688242 in device_add (dev=dev@entry=0xffff888101391800) at drivers/base/core.c:3343
#14 0xffffffff811e70e1 in cdev_device_add (cdev=cdev@entry=0xffff888101391b00, dev=dev@entry=0xffff888101391800) at fs/char_dev.c:549
#15 0xffffffff817e90bb in __devm_rtc_register_device (rtc=0xffff888101391800, owner=0x0 <fixed_percpu_data>) at drivers/rtc/class.c:399
#16 __devm_rtc_register_device (owner=owner@entry=0x0 <fixed_percpu_data>, rtc=0xffff888101391800) at drivers/rtc/class.c:376
#17 0xffffffff817ee525 in cmos_do_probe (dev=0xffff888100323800, ports=0xffff888101264c00, rtc_irq=8) at drivers/rtc/rtc-cmos.c:867
#18 0xffffffff814ac336 in pnp_device_probe (dev=0xffff888100323800) at drivers/pnp/driver.c:109
#19 0xffffffff8168b4d6 in really_probe (dev=dev@entry=0xffff888100323800, drv=drv@entry=0xffffffff825e9260 <cmos_pnp_driver+64>) at drivers/base/dd.c:576
#20 0xffffffff8168b711 in driver_probe_device (drv=drv@entry=0xffffffff825e9260 <cmos_pnp_driver+64>, dev=dev@entry=0xffff888100323800) at drivers/base/dd.c:763
#21 0xffffffff8168b9de in device_driver_attach (drv=drv@entry=0xffffffff825e9260 <cmos_pnp_driver+64>, dev=dev@entry=0xffff888100323800) at drivers/base/dd.c:1039
#22 0xffffffff8168ba40 in __driver_attach (dev=0xffff888100323800, data=0xffffffff825e9260 <cmos_pnp_driver+64>) at drivers/base/dd.c:1117
#23 0xffffffff816896e3 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff825e9260 <cmos_pnp_driver+64>, fn=
fn@entry=0xffffffff8168b9f0 <__driver_attach>) at drivers/base/bus.c:305
#24 0xffffffff8168aec5 in driver_attach (drv=drv@entry=0xffffffff825e9260 <cmos_pnp_driver+64>) at drivers/base/dd.c:1133
#25 0xffffffff8168a892 in bus_add_driver (drv=drv@entry=0xffffffff825e9260 <cmos_pnp_driver+64>) at drivers/base/bus.c:622
#26 0xffffffff8168c2d7 in driver_register (drv=drv@entry=0xffffffff825e9260 <cmos_pnp_driver+64>) at drivers/base/driver.c:171
#27 0xffffffff814ac1c7 in pnp_register_driver (drv=drv@entry=0xffffffff825e9220 <cmos_pnp_driver>) at drivers/pnp/driver.c:272
#28 0xffffffff82b88849 in cmos_init () at drivers/rtc/rtc-cmos.c:1465
#29 0xffffffff81000def in do_one_initcall (fn=0xffffffff82b8883b <cmos_init>) at init/main.c:1249
#30 0xffffffff82b4926b in do_initcall_level (command_line=0xffff888100123400 "root", level=6) at ./include/linux/compiler.h:234
#31 do_initcalls () at init/main.c:1338
#32 do_basic_setup () at init/main.c:1358
#33 kernel_init_freeable () at init/main.c:1560
#34 0xffffffff81b8f849 in kernel_init (unused=<optimized out>) at init/main.c:1447
#35 0xffffffff810019b2 in ret_from_fork () at arch/x86/entry/entry_64.S:294
#36 0x0000000000000000 in ?? ()
```

在 ./platform_match_dmesg.txt 中，platform_match 调用 i8042 / serial8250 的时候其行为很奇怪:
具体原因是在 [^4] 的 Legacy Drivers: Device Probing 提到过，只是这个接口现在变成了 platform_device_add

```plain
// 添加 device
[    1.320115] Call Trace:
[    1.320115]  dump_stack+0x64/0x7c
[    1.320115]  platform_match+0x3f/0xd4
[    1.320115]  __device_attach_driver+0x25/0x80
[    1.320115]  ? driver_allows_async_probing+0x40/0x40
[    1.320115]  bus_for_each_drv+0x79/0xc0
[    1.320115]  __device_attach+0xe6/0x150
[    1.320115]  bus_probe_device+0x89/0xa0
[    1.320115]  device_add+0x3c0/0x8e0
[    1.320115]  ? dev_set_name+0x4e/0x70
[    1.320115]  platform_device_add+0xdf/0x1e0
[    1.320115]  ? univ8250_console_init+0x22/0x22
[    1.320115]  serial8250_init+0x9f/0x158
[    1.320115]  ? univ8250_console_init+0x22/0x22
[    1.320115]  do_one_initcall+0x3f/0x1b0
[    1.320115]  kernel_init_freeable+0x19e/0x1e6
[    1.320115]  ? rest_init+0xa4/0xa4
[    1.320115]  kernel_init+0x5/0xfc
[    1.320115]  ret_from_fork+0x22/0x30

// 添加完成 device 之后再去添加 driver
[    1.516664] Call Trace:
[    1.516664]  dump_stack+0x64/0x7c
[    1.516664]  platform_match+0x2b/0xc0
[    1.516664]  __driver_attach+0x1d/0xc0
[    1.516664]  ? device_driver_attach+0x60/0x60
[    1.516664]  bus_for_each_dev+0x73/0xb0
[    1.516664]  bus_add_driver+0x172/0x1c0
[    1.516664]  driver_register+0x67/0xb0
[    1.516664]  serial8250_init+0x118/0x158
[    1.516664]  ? univ8250_console_init+0x22/0x22
[    1.516664]  do_one_initcall+0x3f/0x1b0
[    1.516664]  kernel_init_freeable+0x19e/0x1e6
[    1.516664]  ? rest_init+0xa4/0xa4
[    1.516664]  kernel_init+0x5/0xfc
[    1.516664]  ret_from_fork+0x22/0x30
```

一般人走的道 : device 已经探测完成，直接进行 driver 的探测
```plain
[    1.396540] Call Trace:
[    1.396540]  dump_stack+0x64/0x7c
[    1.396540]  platform_match+0x2b/0xc0
[    1.396540]  __driver_attach+0x1d/0xc0
[    1.396540]  ? device_driver_attach+0x60/0x60
[    1.396540]  bus_for_each_dev+0x73/0xb0
[    1.396540]  bus_add_driver+0x172/0x1c0
[    1.396540]  driver_register+0x67/0xb0
[    1.396540]  ? gpio_clk_driver_init+0xe/0xe
[    1.396540]  do_one_initcall+0x3f/0x1b0
[    1.396540]  kernel_init_freeable+0x19e/0x1e6
[    1.396540]  ? rest_init+0xa4/0xa4
[    1.396540]  kernel_init+0x5/0xfc
[    1.396540]  ret_from_fork+0x22/0x30
```

实际上，这个注释说的很清楚，一般来说所有的 platform_device 都是走 platform_device_register 的
但是有些 legacy 的驱动会直接调用 platform_device_add
```c
/**
 * platform_device_add - add a platform device to device hierarchy
 * @pdev: platform device we're adding
 *
 * This is part 2 of platform_device_register(), though may be called
 * separately _iff_ pdev was allocated by platform_device_alloc().
 */
int platform_device_add(struct platform_device *pdev)
```

## 奇怪的 rtc
```c
[    2.308385] huxueshi:platform_match alarmtimer == alarmtimer
[    2.309065] CPU: 0 PID: 1 Comm: swapper/0 Not tainted 5.12.0+ #39
[    2.309766] Hardware name: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-14-g748d619-dirty-20210524_151541-maritns3-pc 04/01/2014
[    2.310062] Call Trace:
[    2.310062]  dump_stack+0x64/0x7c
[    2.310062]  platform_match+0x2b/0xc0
[    2.310062]  __device_attach_driver+0x25/0x80
[    2.310062]  ? driver_allows_async_probing+0x40/0x40
[    2.310062]  bus_for_each_drv+0x79/0xc0
[    2.310062]  __device_attach+0xe6/0x150
[    2.310062]  bus_probe_device+0x89/0xa0
[    2.310062]  device_add+0x3c0/0x8e0
[    2.310062]  ? dev_set_name+0x4e/0x70
[    2.310062]  platform_device_add+0xdf/0x1e0
[    2.310062]  platform_device_register_full+0xc1/0x130
[    2.310062]  alarmtimer_rtc_add_device+0x8c/0x150
[    2.310062]  device_add+0x462/0x8e0
[    2.310062]  ? cdev_get+0x60/0x60
[    2.310062]  ? cdev_purge+0x50/0x50
[    2.310062]  cdev_device_add+0x41/0x70
[    2.310062]  __devm_rtc_register_device+0x10b/0x1a0
[    2.310062]  ? request_threaded_irq+0x107/0x170
[    2.310062]  cmos_do_probe+0x275/0x420
[    2.310062]  ? rtc_wake_on+0x20/0x20
[    2.310062]  ? cmos_nvram_read+0x70/0x70
[    2.310062]  ? cmos_do_probe+0x420/0x420
[    2.310062]  pnp_device_probe+0x56/0xb0
[    2.310062]  really_probe+0xd6/0x2c0
[    2.310062]  driver_probe_device+0x51/0xb0
[    2.310062]  device_driver_attach+0x4e/0x60
[    2.310062]  __driver_attach+0x50/0xc0
[    2.310062]  ? device_driver_attach+0x60/0x60
[    2.310062]  bus_for_each_dev+0x73/0xb0
[    2.310062]  bus_add_driver+0x172/0x1c0
[    2.310062]  driver_register+0x67/0xb0
[    2.310062]  ? rtc_dev_init+0x2b/0x2b
[    2.310062]  cmos_init+0xe/0x6b
[    2.310062]  do_one_initcall+0x3f/0x1b0
[    2.310062]  kernel_init_freeable+0x19e/0x1e6
[    2.310062]  ? rest_init+0xa4/0xa4
[    2.310062]  kernel_init+0x5/0xfc
[    2.310062]  ret_from_fork+0x22/0x30
```


[^1]: https://unix.stackexchange.com/questions/397701/how-to-list-the-devices-attached-to-an-isa-bus-in-linux
[^2]: https://lwn.net/Articles/448499/ : 介绍一个大概
[^3]: https://lwn.net/Articles/448502/ : platform 和 device tree 的关系
[^4]: https://www.kernel.org/doc/html/latest/driver-api/driver-model/platform.html
