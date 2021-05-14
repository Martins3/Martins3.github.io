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

## kvm-i8259
- [ ] 无法理解，ISA 上的中断控制器和 kvm-ioapic 上的中断控制器的关系是什么啊!
- [ ] kvm_pic_in_kernel 意味着什么 ?

- pc_i8259_create
  - kvm_i8259_init
    - i8259_init_chip : 初始化所需要的地址空间
      - .... kvm_pic_realize 
        - memory_region_init_io : 申请地址空间
    
```c
/**
#0  kvm_pic_realize (dev=0x5555566eb000, errp=0x7fffffffd350) at ../hw/i386/kvm/i8259.c:124
#1  0x0000555555cade27 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffd3d0) at ../hw/core/qdev.c:761
#2  0x0000555555c9b17a in property_set_bool (obj=0x5555566eb000, v=<optimized out>, name=<optimized out>, opaque=0x5555565d1db0, errp=0x7fffffffd3d0) at ../qom/object.c
:2257
#3  0x0000555555c9d68c in object_property_set (obj=obj@entry=0x5555566eb000, name=name@entry=0x555555ed7f76 "realized", v=v@entry=0x555556bae400, errp=errp@entry=0x5555
564e2e30 <error_fatal>) at ../qom/object.c:1402
#4  0x0000555555c9f9c4 in object_property_set_qobject (obj=obj@entry=0x5555566eb000, name=name@entry=0x555555ed7f76 "realized", value=value@entry=0x555556baa4a0, errp=e
rrp@entry=0x5555564e2e30 <error_fatal>) at ../qom/qom-qobject.c:28
#5  0x0000555555c9d8d9 in object_property_set_bool (obj=0x5555566eb000, name=0x555555ed7f76 "realized", value=<optimized out>, errp=0x5555564e2e30 <error_fatal>) at ../
qom/object.c:1472
#6  0x0000555555caccf3 in qdev_realize_and_unref (dev=dev@entry=0x5555566eb000, bus=bus@entry=0x555556998560, errp=<optimized out>) at ../hw/core/qdev.c:396
#7  0x000055555590c2e9 in isa_realize_and_unref (dev=dev@entry=0x5555566eb000, bus=bus@entry=0x555556998560, errp=<optimized out>) at ../hw/isa/isa-bus.c:179
#8  0x00005555558fd454 in i8259_init_chip (name=<optimized out>, bus=0x555556998560, master=<optimized out>) at ../hw/intc/i8259_common.c:104
#9  0x0000555555a6cc6c in kvm_i8259_init (bus=0x555556998560) at ../hw/i386/kvm/i8259.c:136
#10 0x0000555555a62bca in pc_i8259_create (isa_bus=0x555556998560, i8259_irqs=0x555556aab200) at ../hw/i386/pc.c:1200
#11 0x0000555555a65b5f in pc_init1 (machine=0x5555566c0000, pci_type=0x555555dbe5ad "i440FX", host_type=0x555555d80e54 "i440FX-pcihost") at ../hw/i386/pc_piix.c:223
#12 0x00005555558ff1ae in machine_run_board_init (machine=machine@entry=0x5555566c0000) at ../hw/core/machine.c:1232
#13 0x0000555555bae1be in qemu_init_board () at ../softmmu/vl.c:2514
#14 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
#15 0x0000555555bb1e02 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3611
#16 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
*/
```

```c
/**
>>> bt
#0  kvm_pic_set_irq (opaque=0x0, irq=4, level=0) at ../hw/i386/kvm/i8259.c:115
#1  0x0000555555a7a814 in gsi_handler (opaque=0x555556aab200, n=4, level=0) at ../hw/i386/x86.c:596
#2  0x0000555555872752 in qemu_irq_lower (irq=<optimized out>) at /home/maritns3/core/kvmqemu/include/hw/irq.h:17
#3  serial_reset (opaque=0x5555569828b0) at ../hw/char/serial.c:888
#4  0x0000555555cade27 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffd1f0) at ../hw/core/qdev.c:761
#5  0x0000555555c9b17a in property_set_bool (obj=0x5555569828b0, v=<optimized out>, name=<optimized out>, opaque=0x5555565d1db0, errp=0x7fffffffd1f0) at ../qom/object.c
:2257
#6  0x0000555555c9d68c in object_property_set (obj=obj@entry=0x5555569828b0, name=name@entry=0x555555ed7f76 "realized", v=v@entry=0x5555569da470, errp=errp@entry=0x7fff
ffffd2e0) at ../qom/object.c:1402
#7  0x0000555555c9f9c4 in object_property_set_qobject (obj=obj@entry=0x5555569828b0, name=name@entry=0x555555ed7f76 "realized", value=value@entry=0x5555569da370, errp=e
rrp@entry=0x7fffffffd2e0) at ../qom/qom-qobject.c:28
#8  0x0000555555c9d8d9 in object_property_set_bool (obj=0x5555569828b0, name=name@entry=0x555555ed7f76 "realized", value=value@entry=true, errp=errp@entry=0x7fffffffd2e
0) at ../qom/object.c:1472
#9  0x0000555555cacc52 in qdev_realize (dev=<optimized out>, bus=bus@entry=0x0, errp=errp@entry=0x7fffffffd2e0) at ../hw/core/qdev.c:389
#10 0x000055555585e37b in serial_isa_realizefn (dev=0x555556982800, errp=0x7fffffffd2e0) at /home/maritns3/core/kvmqemu/include/hw/qdev-core.h:17
#11 0x0000555555cade27 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffd360) at ../hw/core/qdev.c:761
#12 0x0000555555c9b17a in property_set_bool (obj=0x555556982800, v=<optimized out>, name=<optimized out>, opaque=0x5555565d1db0, errp=0x7fffffffd360) at ../qom/object.c
:2257
#13 0x0000555555c9d68c in object_property_set (obj=obj@entry=0x555556982800, name=name@entry=0x555555ed7f76 "realized", v=v@entry=0x5555569da260, errp=errp@entry=0x5555
564e2e30 <error_fatal>) at ../qom/object.c:1402
#14 0x0000555555c9f9c4 in object_property_set_qobject (obj=obj@entry=0x555556982800, name=name@entry=0x555555ed7f76 "realized", value=value@entry=0x5555569da350, errp=e
rrp@entry=0x5555564e2e30 <error_fatal>) at ../qom/qom-qobject.c:28
#15 0x0000555555c9d8d9 in object_property_set_bool (obj=0x555556982800, name=0x555555ed7f76 "realized", value=<optimized out>, errp=0x5555564e2e30 <error_fatal>) at ../
qom/object.c:1472
#16 0x0000555555caccf3 in qdev_realize_and_unref (dev=dev@entry=0x555556982800, bus=bus@entry=0x555556998560, errp=<optimized out>) at ../hw/core/qdev.c:396
#17 0x000055555590c2e9 in isa_realize_and_unref (dev=dev@entry=0x555556982800, bus=bus@entry=0x555556998560, errp=<optimized out>) at ../hw/isa/isa-bus.c:179
#18 0x000055555585e602 in serial_isa_init (chr=0x5555568972b0, index=0, bus=0x555556998560) at ../hw/char/serial-isa.c:167
#19 serial_hds_isa_init (bus=bus@entry=0x555556998560, from=from@entry=0, to=to@entry=4) at ../hw/char/serial-isa.c:179
#20 0x0000555555a626f5 in pc_superio_init (no_vmport=false, create_fdctrl=true, isa_bus=0x555556998560) at ../hw/i386/pc.c:1064
#21 pc_basic_device_init (pcms=pcms@entry=0x5555566c0000, isa_bus=0x555556998560, gsi=<optimized out>, rtc_state=rtc_state@entry=0x7fffffffd538, create_fdctrl=create_fd
ctrl@entry=true, hpet_irqs=hpet_irqs@entry=4) at ../hw/i386/pc.c:1174
#22 0x0000555555a65bdd in pc_init1 (machine=0x5555566c0000, pci_type=0x555555dbe5ad "i440FX", host_type=0x555555d80e54 "i440FX-pcihost") at ../hw/i386/pc_piix.c:241
#23 0x00005555558ff1ae in machine_run_board_init (machine=machine@entry=0x5555566c0000) at ../hw/core/machine.c:1232
#24 0x0000555555bae1be in qemu_init_board () at ../softmmu/vl.c:2514
#25 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
#26 0x0000555555bb1e02 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3611
#27 0x000055555582b4bd in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```

```c
/**
 *
#0  ps2_queue_noirq (s=0x555556a59700, b=17) at ../hw/input/ps2.c:195
#1  0x0000555555a487cd in ps2_queue (s=0x555556a59700, b=<optimized out>) at ../hw/input/ps2.c:217
#2  0x0000555555a488ab in ps2_put_keycode (opaque=0x555556a59700, keycode=<optimized out>) at ../hw/input/ps2.c:275
#3  0x0000555555a48b05 in ps2_keyboard_event (dev=0x555556a59700, src=<optimized out>, evt=<optimized out>) at ../hw/input/ps2.c:475
#4  0x0000555555900af8 in qemu_input_event_send_impl (src=0x55555653a140, evt=0x555557bab8b0) at ../ui/input.c:349
#5  0x000055555590145b in qemu_input_event_send_key (src=0x55555653a140, key=0x555556b70910, down=<optimized out>) at ../ui/input.c:422
#6  0x00005555559014b6 in qemu_input_event_send_key_qcode (src=<optimized out>, q=q@entry=Q_KEY_CODE_ALT, down=down@entry=true) at ../ui/input.c:444
#7  0x0000555555a2176a in qkbd_state_key_event (down=<optimized out>, qcode=Q_KEY_CODE_ALT, kbd=0x555556e18d90) at ../ui/kbd-state.c:102
#8  qkbd_state_key_event (kbd=0x555556e18d90, qcode=qcode@entry=Q_KEY_CODE_ALT, down=<optimized out>) at ../ui/kbd-state.c:40
#9  0x0000555555893f73 in gd_key_event (widget=<optimized out>, key=0x55555682fc00, opaque=0x5555569c09b0) at ../ui/gtk.c:1179
#10 0x00007ffff78334fb in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#11 0x00007ffff6fcd802 in g_closure_invoke () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#12 0x00007ffff6fe1814 in  () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#13 0x00007ffff6fec47d in g_signal_emit_valist () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#14 0x00007ffff6fed0f3 in g_signal_emit () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#15 0x00007ffff77ddc23 in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#16 0x00007ffff77ff5db in gtk_window_propagate_key_event () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#17 0x00007ffff7803873 in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#18 0x00007ffff78335ef in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#19 0x00007ffff6fcda56 in  () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#20 0x00007ffff6febdf1 in g_signal_emit_valist () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#21 0x00007ffff6fed0f3 in g_signal_emit () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#22 0x00007ffff77ddc23 in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#23 0x00007ffff76991df in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#24 0x00007ffff769b3db in gtk_main_do_event () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#25 0x00007ffff7383f79 in  () at /lib/x86_64-linux-gnu/libgdk-3.so.0
#26 0x00007ffff73b7106 in  () at /lib/x86_64-linux-gnu/libgdk-3.so.0
#27 0x00007ffff6ee217d in g_main_context_dispatch () at /lib/x86_64-linux-gnu/libglib-2.0.so.0
#28 0x0000555555d219a8 in glib_pollfds_poll () at ../util/main-loop.c:231
#29 os_host_main_loop_wait (timeout=<optimized out>) at ../util/main-loop.c:254
#30 main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:530
#31 0x0000555555ae96a1 in qemu_main_loop () at ../softmmu/runstate.c:725
#32 0x000055555582b4c2 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:50
*/
```
很好，我们的想法的完整验证, 从 gtk 到 ps2 的缓冲区的注入。

## kvm-ioapic

## ata
- [ ] 在 info qtree 中间暂时找不到啊

