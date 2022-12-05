# QEMU 如何模拟中断

<!-- vim-markdown-toc GitLab -->

* [how interrupt are generated](#how-interrupt-are-generated)
  * [keyboard](#keyboard)
  * [ide](#ide)
* [QEMU irqchip](#qemu-irqchip)
  * [files related with irqchip](#files-related-with-irqchip)
  * [TypeInfo related with irqchip](#typeinfo-related-with-irqchip)
  * [irqchip select](#irqchip-select)
* [irq routing](#irq-routing)
  * [qemu side irq routing](#qemu-side-irq-routing)
  * [kernel side irq routing](#kernel-side-irq-routing)
* [irqchip internal](#irqchip-internal)
  * [irqchip init](#irqchip-init)
  * [tcg pic](#tcg-pic)
  * [tcg ioapic](#tcg-ioapic)
* [how interrupt inserted to vCPU](#how-interrupt-inserted-to-vcpu)
* [interrupt in x86 Linux kernel](#interrupt-in-x86-linux-kernel)
  * [code flow from first instruction](#code-flow-from-first-instruction)
    * [start from idt](#start-from-idt)
    * [jump to C function](#jump-to-c-function)
    * [route to interrupt handler](#route-to-interrupt-handler)
  * [how ioapic got programmed](#how-ioapic-got-programmed)
* [intel manual](#intel-manual)
  * [irr and isr](#irr-and-isr)
  * [tpr](#tpr)
* [Advanced topic](#advanced-topic)
  * [how kernel switch from pic to apic](#how-kernel-switch-from-pic-to-apic)
  * [kvmvapic](#kvmvapic)
  * [switch intc](#switch-intc)
  * [legacy PCI interrupt](#legacy-pci-interrupt)
  * [interrupt flow](#interrupt-flow)
* [references](#references)

<!-- vim-markdown-toc -->
本文简单地分析一下一个中断从产生到 Linux kernel 执行该中断的 interrupt handler 的过程。
在 [how interrupt are generated](#how-interrupt-are-generated) 中分析键盘和 ide 设备的中断是如何被 QEMU 模拟的。
[QEMU irqchip](#qemu-irqchip) 分析 QEMU 模拟 irqchip 的文件结构和关键结构体，可以建立一个对于 x86 下 QEMU 需要模拟的几个 intc, 也即是 pic iopaic 和 lapic 有一个大致的印象。
因为 pic 和 iopaic 的共存，所以需要 [irq routing](#irq-routing) 机制将中断同时注入到 pic 和 iopaic。
在 [irqchip internal](#irqchip-internal) 中，分析了 pic 和 iopic 总接受中断到将中断转发到 lapic 或者 cpu 的过程。
在 [how interrupt inserted to vCPU](#how-interrupt-inserted-to-vcpu) 中，分析 vCPU 如何收到中断，并且跳转到 guest 的中断入口处。
在 [interrupt in x86 Linux kernel](#interrupt-in-x86-linux-kernel) 中一步步的分析中断从汇编到 interrupt handler 的过程。

## how interrupt are generated

### keyboard
```c
/*
#0  kbd_update_irq_lines (s=0x555556844d98) at ../hw/input/pckbd.c:144
#1  0x00005555559f5b9b in ps2_put_keycode (opaque=0x555556d28f20, keycode=<optimized out>) at ../hw/input/ps2.c:277
#2  0x00005555559f5e05 in ps2_keyboard_event (dev=0x555556d28f20, src=<optimized out>, evt=<optimized out>) at ../hw/input/ps2.c:478
#3  0x0000555555a35f88 in qemu_input_event_send_impl (src=0x55555668ffb0, evt=0x555556e5ad40) at ../ui/input.c:349
#4  0x0000555555a368eb in qemu_input_event_send_key (src=0x55555668ffb0, key=0x555556d3fcc0, down=<optimized out>) at ../ui/input.c:422
#5  0x0000555555a36946 in qemu_input_event_send_key_qcode (src=<optimized out>, q=q@entry=Q_KEY_CODE_R, down=down@entry=true) at ../ui/input.c:444
#6  0x000055555595afea in qkbd_state_key_event (down=<optimized out>, qcode=Q_KEY_CODE_R, kbd=0x555556a42c10) at ../ui/kbd-state.c:102
#7  qkbd_state_key_event (kbd=0x555556a42c10, qcode=qcode@entry=Q_KEY_CODE_R, down=<optimized out>) at ../ui/kbd-state.c:40
#8  0x0000555555b4cb23 in gd_key_event (widget=<optimized out>, key=0x555556971e40, opaque=0x555556b24d70) at ../ui/gtk.c:1112
#9  0x00007ffff78dd4fb in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#10 0x00007ffff7077802 in g_closure_invoke () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#11 0x00007ffff708b814 in  () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#12 0x00007ffff709647d in g_signal_emit_valist () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#13 0x00007ffff70970f3 in g_signal_emit () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#14 0x00007ffff7887c23 in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#15 0x00007ffff78a95db in gtk_window_propagate_key_event () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#16 0x00007ffff78ad873 in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#17 0x00007ffff78dd5ef in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#18 0x00007ffff7077a56 in  () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#19 0x00007ffff7095df1 in g_signal_emit_valist () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#20 0x00007ffff70970f3 in g_signal_emit () at /lib/x86_64-linux-gnu/libgobject-2.0.so.0
#21 0x00007ffff7887c23 in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#22 0x00007ffff77431df in  () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#23 0x00007ffff77453db in gtk_main_do_event () at /lib/x86_64-linux-gnu/libgtk-3.so.0
#24 0x00007ffff742df79 in  () at /lib/x86_64-linux-gnu/libgdk-3.so.0
#25 0x00007ffff7461106 in  () at /lib/x86_64-linux-gnu/libgdk-3.so.0
#26 0x00007ffff6f8c17d in g_main_context_dispatch () at /lib/x86_64-linux-gnu/libglib-2.0.so.0
#27 0x0000555555e4ce88 in glib_pollfds_poll () at ../util/main-loop.c:232
#28 os_host_main_loop_wait (timeout=<optimized out>) at ../util/main-loop.c:255
#29 main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:531
#30 0x0000555555c58261 in qemu_main_loop () at ../softmmu/runstate.c:726
#31 0x0000555555940c92 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:50
*/
```

具体分析 `kbd_update_irq_lines` 的内容:
```c
/* XXX: not generating the irqs if KBD_MODE_DISABLE_KBD is set may be
   incorrect, but it avoids having to simulate exact delays */
static void kbd_update_irq_lines(KBDState *s)
{
    int irq_kbd_level, irq_mouse_level;

    irq_kbd_level = 0;
    irq_mouse_level = 0;

    if (s->status & KBD_STAT_OBF) {
        if (s->status & KBD_STAT_MOUSE_OBF) {
            if (s->mode & KBD_MODE_MOUSE_INT) {
                irq_mouse_level = 1;
            }
        } else {
            if ((s->mode & KBD_MODE_KBD_INT) &&
                !(s->mode & KBD_MODE_DISABLE_KBD)) {
                irq_kbd_level = 1;
            }
        }
    }
    qemu_set_irq(s->irq_kbd, irq_kbd_level);
    qemu_set_irq(s->irq_mouse, irq_mouse_level);
}
```

### ide

```txt
/*
#0  huxueshi (i=34) at ../hw/intc/apic.c:402
#1  0x0000555555c6a21a in apic_set_irq (s=0x55555697f560, vector_num=34, trigger_mode=0) at ../hw/intc/apic.c:411
#2  0x0000555555c6a4a3 in apic_bus_deliver (deliver_bitmask=<optimized out>, delivery_mode=<optimized out>, vector_num=34 '"', trigger_mode=0 '\000') at ../hw/intc/apic.c:273
#3  0x0000555555c6a66f in apic_deliver_irq (dest=1 '\001', dest_mode=1 '\001', delivery_mode=0 '\000', vector_num=34 '"', trigger_mode=0 '\000') at ../hw/intc/apic.c:286
#4  0x0000555555c6aadb in apic_mem_write (opaque=<optimized out>, addr=4100, val=34, size=<optimized out>) at ../hw/intc/apic.c:766
#5  0x0000555555cd2611 in memory_region_write_accessor (mr=mr@entry=0x55555697f5f0, addr=4100, value=value@entry=0x7fffe888b7c8, size=size@entry=4, shift=<optimized out>, mask=mask@entry=4294967295, attrs=...) at ../softmmu/memory.c:492
#6  0x0000555555ccea9e in access_with_adjusted_size (addr=addr@entry=4100, value=value@entry=0x7fffe888b7c8, size=size@entry=4, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=access_fn@entry=0x555555cd2580 <memory_region_write_accessor>, mr=0x55555697f5f0, attrs=...) at ../softmmu/memory.c:554
#7  0x0000555555cd1b47 in memory_region_dispatch_write (mr=mr@entry=0x55555697f5f0, addr=4100, data=<optimized out>, data@entry=34, op=op@entry=MO_32, attrs=attrs@entry=...) at ../softmmu/memory.c:1504
#8  0x0000555555ca11ed in address_space_stl_internal (endian=DEVICE_LITTLE_ENDIAN, result=0x0, attrs=..., val=34, addr=<optimized out>, as=0x0) at /home/maritns3/core/kvmqemu/include/exec/memory.h:2868
#9  address_space_stl_le (as=as@entry=0x555556606820 <address_space_memory>, addr=<optimized out>, val=34, attrs=attrs@entry=..., result=result@entry=0x0) at /home/maritns3/core/kvmqemu/memory_ldst.c.inc:357
#10 0x0000555555cee024 in stl_le_phys (val=<optimized out>, addr=<optimized out>, as=0x555556606820 <address_space_memory>) at /home/maritns3/core/kvmqemu/include/exec/memory_ldst_phys.h.inc:121
#11 ioapic_service (s=s@entry=0x555556a49e10) at ../hw/intc/ioapic.c:139
#12 0x0000555555cee2ff in ioapic_set_irq (opaque=0x555556a49e10, vector=<optimized out>, level=1) at ../hw/intc/ioapic.c:187
#13 0x0000555555b92644 in gsi_handler (opaque=0x555556a07620, n=15, level=1) at ../hw/i386/x86.c:600
#14 0x0000555555a9dc41 in qemu_irq_raise (irq=<optimized out>) at /home/maritns3/core/kvmqemu/include/hw/irq.h:12
#15 ide_set_irq (bus=<optimized out>, bus=<optimized out>) at /home/maritns3/core/kvmqemu/include/hw/ide/internal.h:576
#16 ide_set_irq (bus=<optimized out>, bus=<optimized out>) at /home/maritns3/core/kvmqemu/include/hw/ide/internal.h:573
#17 ide_atapi_cmd_error (s=s@entry=0x555557aa7df8, sense_key=sense_key@entry=2, asc=asc@entry=58) at ../hw/ide/atapi.c:193
#18 0x0000555555a9f622 in ide_atapi_cmd (s=0x555557aa7df8) at ../hw/ide/atapi.c:1356
#19 0x0000555555980bce in ide_data_writel (opaque=<optimized out>, addr=<optimized out>, val=0) at ../hw/ide/core.c:2398
#20 0x0000555555cd2611 in memory_region_write_accessor (mr=mr@entry=0x555557ba5640, addr=0, value=value@entry=0x7fffe888bbe8, size=size@entry=4, shift=<optimized out>,mask=mask@entry=4294967295, attrs=...) at ../softmmu/memory.c:492
#21 0x0000555555ccea9e in access_with_adjusted_size (addr=addr@entry=0, value=value@entry=0x7fffe888bbe8, size=size@entry=4, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=access_fn@entry=0x555555cd2580 <memory_region_write_accessor>, mr=0x555557ba5640, attrs=...) at ../softmmu/memory.c:554
#22 0x0000555555cd1b47 in memory_region_dispatch_write (mr=mr@entry=0x555557ba5640, addr=0, data=<optimized out>, data@entry=0, op=op@entry=MO_32, attrs=attrs@entry=...) at ../softmmu/memory.c:1504
#23 0x0000555555ca102d in address_space_stl_internal (endian=DEVICE_NATIVE_ENDIAN, result=0x0, attrs=..., val=0, addr=<optimized out>, as=<optimized out>) at /home/maritns3/core/kvmqemu/include/exec/memory.h:2868
#24 address_space_stl (as=<optimized out>, addr=<optimized out>, val=0, attrs=..., result=0x0) at /home/maritns3/core/kvmqemu/memory_ldst.c.inc:350
#25 0x00007fff959ff27a in code_gen_buffer ()
#26 0x0000555555cd7c2d in cpu_tb_exec (tb_exit=<synthetic pointer>, itb=<optimized out>, cpu=0x555556aff890) at ../accel/tcg/cpu-exec.c:353
#27 cpu_loop_exec_tb (tb_exit=<synthetic pointer>, last_tb=<synthetic pointer>, tb=<optimized out>, cpu=0x555556aff890) at ../accel/tcg/cpu-exec.c:812
#28 cpu_exec (cpu=cpu@entry=0x555556af7000) at ../accel/tcg/cpu-exec.c:970
#29 0x0000555555c3ee57 in tcg_cpus_exec (cpu=cpu@entry=0x555556af7000) at ../accel/tcg/tcg-accel-ops.c:67
#30 0x0000555555cb86c3 in rr_cpu_thread_fn (arg=arg@entry=0x555556b02660) at ../accel/tcg/tcg-accel-ops-rr.c:216
#31 0x0000555555e55903 in qemu_thread_start (args=<optimized out>) at ../util/qemu-thread-posix.c:541
#32 0x00007ffff628d609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#33 0x00007ffff61b4293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

总而言之，是通过 interrupt 模块提供给设备模拟模块的标准函数，例如 qemu_irq_raise 和 qemu_set_irq

## QEMU irqchip
QEMU 的主要工作是模拟这些 intc

### files related with irqchip
QEMU 可以同时支持 kvm 模拟和用户态模拟这些 intc, 所以自然可以拆分为不同的模块，这体现在相关的源文件上。
- common code
  - hw/intc/ioapic_common.c
  - hw/intc/i8259_common.c
  - intc/apic_common.c
- lapic
  - intc/apic.c : apic_info
  - i386/kvm/apic.c : kvm_apic_info
- ioapic
  - hw/intc/ioapic.c :
  - hw/i386/kvm/ioapic.c
- pic
  - hw/i386/kvm/i8259.c
  - hw/intc/i8259.c

### TypeInfo related with irqchip
TypeInfo 是 QEMU Object Model 中抽象出来的概念，简单来说一个 non abstract Class 了。
```c
static const TypeInfo apic_info = {
    .name          = TYPE_APIC,
    .instance_size = sizeof(APICCommonState),
    .parent        = TYPE_APIC_COMMON,
    .class_init    = apic_class_init,
};

static const TypeInfo kvm_apic_info = {
    .name = "kvm-apic",
    .parent = TYPE_APIC_COMMON,
    .instance_size = sizeof(APICCommonState),
    .class_init = kvm_apic_class_init,
};
```

```c
static const TypeInfo ioapic_info = {
    .name          = TYPE_IOAPIC,
    .parent        = TYPE_IOAPIC_COMMON,
    .instance_size = sizeof(IOAPICCommonState),
    .class_init    = ioapic_class_init,
};

static const TypeInfo kvm_ioapic_info = {
    .name  = TYPE_KVM_IOAPIC,
    .parent = TYPE_IOAPIC_COMMON,
    .instance_size = sizeof(KVMIOAPICState),
    .class_init = kvm_ioapic_class_init,
};
```

```c
static const TypeInfo i8259_info = {
    .name       = TYPE_I8259,
    .instance_size = sizeof(PICCommonState),
    .parent     = TYPE_PIC_COMMON,
    .class_init = i8259_class_init,
    .class_size = sizeof(PICClass),
};

static const TypeInfo kvm_i8259_info = {
    .name = TYPE_KVM_I8259,
    .parent = TYPE_PIC_COMMON,
    .instance_size = sizeof(PICCommonState),
    .class_init = kvm_i8259_class_init,
    .class_size = sizeof(KVMPICClass),
};
```
从上面的看到，对于一个 intc QEMU 总是定义两种解决方案，可以让 QEMU 在用户态模拟或者是让 kvm 在内核模拟。

QEMU 一共支持三种模式，分别 off / split / on
- off : lapic 以及 pic ioapic 都是在用户态模拟的
- split : lapic 在内核中
- on : 都是在系统态

hw/intc/ioapic.c 会出现 kvm_irqchip_is_split 是因为 ioapic 在用户态模拟，但是需要将消息注入到 kernel 中。

观察上面的 TypeInfo 还有一个比较有意思的地方在于，其 instance_size 总是等于 sizeof(CommonState)，这说明
QEMU 模拟和内核模拟使用结构体是相同的，实际上，对于 kvm 而言，因为 irqchip 是在内核模拟的，其作用在于临时保存一下内核中的数据(使用 kvm_get_apic_state / kvm_put_apic_state) 从而实现虚拟机的迁移

### irqchip select

第一步: 初始化 KVMState 的成员
- do_configure_accelerator
  - object_new_with_type
    - object_initialize_with_type
       - kvm_accel_instance_init : 初始化 KVMState::kernel_irqchip_allowed 和 KVMState::kernel_irqchip_split

第二个，使用 KVMState 的成员初始化全局变量 kvm_kernel_irqchip_split 和 kvm_split_irqchip
- do_configure_accelerator
  - accel_init_machine : 调用 AccelClass::init_machine
    - kvm_init
      - kvm_irqchip_create 当 KVMState::kernel_irqchip_allowed 的时候，才会调用这个函数
          - kvm_vm_ioctl(s, KVM_CREATE_IRQCHIP)
          - kvm_kernel_irqchip = true; :star:
          - kvm_arch_irqchip_create
            - kvm_kernel_irqchip_split
            - kvm_split_irqchip = true :star:

第三部: 利用全局变量 kvm_kernel_irqchip_split 和 kvm_split_irqchip 构建的 macro 来决定具体初始化哪一个 TypeInfo
- x86_cpu_realizefn
  - x86_cpu_apic_create
    - x86_cpu_apic_create : 调用这个函数实际上会做出判断, 只有当前 CPU 支持这个特性 或者 cpu 的数量超过两个的时候才可以调用下面的函数
        - apic_get_class
          - kvm_apic_in_kernel
        - `cpu->apic_state = DEVICE(object_new_with_class(apic_class));`

- pc_init1
  - pc_i8259_create
  - ioapic_init_gsi

全局变量 kvm_kernel_irqchip_split 和 kvm_split_irqchip 构建的 macro 构建的 macro 就是长成这个样子了
```c
#define kvm_irqchip_in_kernel() (kvm_kernel_irqchip)
#define kvm_irqchip_is_split() (kvm_split_irqchip)
```

```c
#define kvm_apic_in_kernel() (kvm_irqchip_in_kernel())

#define kvm_pit_in_kernel() (kvm_irqchip_in_kernel() && !kvm_irqchip_is_split())
#define kvm_pic_in_kernel() (kvm_irqchip_in_kernel() && !kvm_irqchip_is_split())
#define kvm_ioapic_in_kernel() (kvm_irqchip_in_kernel() && !kvm_irqchip_is_split())
```

## irq routing
指的不是通过编码 ioapic 可以控制中断转发到哪一个 CPU 上，而是因为 x86 上的中断控制的演化导致的，
guest os 可以动态决定其使用哪一个 intc ，但是 host 不知道 guest os 使用哪一个 intc, 甚至 guest os 可以同时打开两个 intc 使用，所以模拟一个中断的时候，只好让两个 intc 都发送中断。

> 虚拟触发了 irq 1，那么需要经过 irq routing：
> irq 1 在 0-7 的范围内，所以会路由到 i8259 master，随后 i8259 master 会向 vCPU 注入中断。
> 同时，irq 1 也会路由到 io apic 一份，io apic 也会向 lapic 继续 delivery。lapic 继续向 vCPU 注入中断。
> linux 在启动阶段，检查到 io apic 后，会选择使用 io apic。尽管经过 irq routing 产生了 i8259 master 和 io apic 两个中断，但是 Linux 选择 io apic 上的中断。[^11]

### qemu side irq routing
每次调用中断，都是从 X86MachineState::gsi 开始的，在 X86MachineState::gsi 是
其在 pc_gsi_create 中初始化,
handler 为 gsi_handler, gsi_handler 再去调用 GSIState 中三个 irqchip 对应的 handler

```c
typedef struct GSIState {
    qemu_irq i8259_irq[ISA_NUM_IRQS];
    qemu_irq ioapic_irq[IOAPIC_NUM_PINS];
    qemu_irq ioapic2_irq[IOAPIC_NUM_PINS];
} GSIState;

/*
 * Pointer types
 * Such typedefs should be limited to cases where the typedef's users
 * are oblivious of its "pointer-ness".
 * Please keep this list in case-insensitive alphabetical order.
 */
typedef struct IRQState *qemu_irq;

struct IRQState {
    Object parent_obj;

    qemu_irq_handler handler;
    void *opaque;
    int n;
};
```


```c
void gsi_handler(void *opaque, int n, int level)
{
    GSIState *s = opaque;

    trace_x86_gsi_interrupt(n, level);
    switch (n) {
    case 0 ... ISA_NUM_IRQS - 1:
        if (s->i8259_irq[n]) {
            /* Under KVM, Kernel will forward to both PIC and IOAPIC */
            qemu_set_irq(s->i8259_irq[n], level);
        }
        /* fall through */
    case ISA_NUM_IRQS ... IOAPIC_NUM_PINS - 1:
        qemu_set_irq(s->ioapic_irq[n], level);
        break;
    case IO_APIC_SECONDARY_IRQBASE
        ... IO_APIC_SECONDARY_IRQBASE + IOAPIC_NUM_PINS - 1:
        qemu_set_irq(s->ioapic2_irq[n - IO_APIC_SECONDARY_IRQBASE], level);
        break;
    }
}
```

使用键盘为例子, 在 kbd_update_irq_lines 中会调用 `qemu_set_irq(s->irq_kbd, irq_kbd_level);` 来触发中断，
其中的 `KBDState::irq_kbd` 是在 isa_get_irq 中初始化的:

- i8042_realizefn
  - `isa_init_irq(isadev, &s->irq_kbd, 1);`
  - `isa_init_irq(isadev, &s->irq_mouse, 12);`
    - isa_get_irq : 其实获取的就是 X86MachineState::gsi

而 isa_get_irq 获取的就是 X86MachineState::gsi, 而 X86MachineState::gsi 上的 qemu_irq 注册的 handler 就是
gsi_handler 了。

### kernel side irq routing
- 为什么需要 irq routing
  - 同时 kvm_vm_ioctl_irq_line 其实是一个标准的接口, arm 对于函数实现就是不需要 irq routing 的操作
      - 所以，当调用 KVM_IRQ_LINE 的时候, 只是需要提供一个中断号
  - kvm 和 QEMU 需要同时支持各种类型的主板，类似一个 kbd 之类的设备其实也不知道具体是和 pic 还是 ioapic 上的, 目前这个处理方法是有效并且最简单的
  - kvm 不知道 os 使用的是哪一个中断控制器

- kvm_vm_ioctl
  - kvm_vm_ioctl_irq_line
    - kvm_set_irq : 循环调用注册到这个 gsi 上的函数
        - kvm_set_pic_irq
        - kvm_set_msi
        - kvm_set_ioapic_irq
          - kvm_ioapic_set_irq
            - ioapic_set_irq
              - ioapic_service
                - kvm_irq_delivery_to_apic
                  - kvm_apic_set_irq
                    - `__apic_accept_irq`
                      - kvm_make_request
                      - kvm_vcpu_kick : ipi 中断

进而分析 pic 的处理过程:
1. 想要发送中断总是 kvm_make_request + kvm_vcpu_kick 两件套实现的, arch/x86/kvm/i8259.c 中只有 pic_unlock 这个函数使用上
2. 调用之前, 会通过 kvm_apic_accept_pic_intr 检查一下, 当前配置( amd64 内核) 下，这个检查总是失败
3. 分析 kvm_apic_accept_pic_intr 的实现，其中只是相当于看 guest 的实现了

现在在 QEMU 中一个中断变为两个，在 kvm 再去处理 irq routing 机制, 那么岂不是一共需要四次? 是的，就是如此。

## irqchip internal

### irqchip init
0. pc_gsi_create : 创建了 qemu_irq，分配了 GSIState , 但是 GSIState 没有被初始化

- pc_gsi_create
  - kvm_pc_setup_irq_routing : 如果采用 kvm 模拟
    - kvm_irqchip_add_irq_route(s, i, KVM_IRQCHIP_PIC_MASTER, i); `i = [0, 8)`
    - kvm_irqchip_add_irq_route(s, i, KVM_IRQCHIP_PIC_SLAVE, i - 8); `i = [8, 16)`
    - kvm_irqchip_add_irq_route(s, i, KVM_IRQCHIP_IOAPIC, i); `i = [0, 24)` i == 2 被特殊处理了
    - kvm_irqchip_commit_routes
        - `kvm_vm_ioctl(s, KVM_SET_GSI_ROUTING, s->irq_routes);`
  - qemu_allocate_irqs : 创建一组 IRQState 其 handler 是 gsi_handler, 其 opaque 是 GSIState, 这些 gsi 通过 X86MachineState::gsi 来索引的

1. apic_common_realize
  - x86_cpu_new
    - qdev_realize
      - x86_cpu_realizefn
        - x86_cpu_apic_realize
          - qdev_realize
            - apic_common_realize
              - kvm_apic_realize
                - memory_region_init_io(&s->io_memory, OBJECT(s), &kvm_apic_io_ops, s, "kvm-apic-msi", APIC_SPACE_SIZE);
              - apic_realize
                - memory_region_init_io(&s->io_memory, OBJECT(s), &apic_io_ops, s, "apic-msi", APIC_SPACE_SIZE);
                - timer_new_ns(QEMU_CLOCK_VIRTUAL, apic_timer, s);

2. `pc_i8259_create(isa_bus, gsi_state->i8259_irq);`

- pc_i8259_create
  - kvm_i8259_init : i8259 也就是 pic 本身是一个设备，所以需要调用 i8259_init_chip 初始化一下，这很好
    - i8259_init_chip(TYPE_KVM_I8259, bus, true); // master
    - i8259_init_chip(TYPE_KVM_I8259, bus, false); // slave
    - qemu_allocate_irqs(kvm_pic_set_irq, NULL, ISA_NUM_IRQS); // 创建出来的 qemu_irq 赋值给
  - xen_interrupt_controller_init
  - i8259_init :其参数为 x86_allocate_cpu_irq 创建出来的 qemu_irq，这个 qemu_irq 注册 handler 为 **pic_irq_request**
    - i8259_init_chip : 初始化 master i8259
    - i8259_init_chip : 初始化 slave i8259
  - 将初始化完成的 qemu_irq 拷贝到 GSIState::i8259_irq 中

3. `ioapic_init_gsi(gsi_state, "i440fx");`

- ioapic_init_gsi
  - sysbus_realize_and_unref : 穿越漫长的 QOM
    - qdev_realize_and_unref
      - object_property_set_bool
        - object_property_set_qobject
          - object_property_set
            - property_set_bool
              - ioapic_common_realize
                - kvm_ioapic_realize :star: 注册 qemu_irq 中 handler 为 kvm_ioapic_set_irq
                - ioapic_realize :star:
                  - memory_region_init_io(&s->io_memory, OBJECT(s), &ioapic_io_ops, s, "ioapic", 0x1000);
                  - 注册 qemu_irq 中 handler 为 **ioapic_set_irq**
  - 将初始化完成的 qemu_irq 拷贝到 GSIState::ioapic_irq 中

总结一下，在 tcg 模式下，gsi_handler 对于中断号小于 16 的会分别调用 pic 和 iopaic 的 hook，
- pic_irq_request
- ioapic_set_irq

lapic 因为是每一个 CPU 需要的，所以其初始化在 x86_cpu_realizefn 中进行的。下面分析 pic 和 ioapic 的中断如何送到 lapic 的。

### tcg pic

- pc_i8259_create
  - `i8259_init(isa_bus, x86_allocate_cpu_irq())`

x86_allocate_cpu_irq 会创建出来一个 qemu_irq 出来，其 handler 为 pic_irq_request

在 pic_irq_request 中，会首先判断是否需要发送给 lapic，如果是，那么调用 apic_deliver_pic_intr
如果不是，那么使用 cpu_interrupt 直接发送给 cpu 了。

- gsi_handler : irq routing
  - pic_set_irq : pic 的入口
    - pic_update_irq
      - qemu_irq_raise
        - pic_irq_request : 这个位置就是注册的 pic 中断的操作
          - pic_irq_request
            - apic_local_deliver
              - apic_set_irq
                - apic_update_irq
                  - cpu_interrupt

### tcg ioapic

- ioapic_realize
  - qdev_init_gpio_in(dev, ioapic_set_irq, IOAPIC_NUM_PINS);

发送给 ioapic 的中断的入口是 ioapic_set_irq，而经过 ioapic 通过调用 ioapic_service 将中断转发到 ioapic 上。

- ioapic_service
  - ioapic_entry_parse : 从 ioredtbl 中解析填充数据，填充 ioapic_entry_info
    - 在 ioapic_mem_write 可以修改 IOAPICCommonState::ioredtbl 来修改 ioapic 的中断路由。
  - stl_le_phys(ioapic_as, info.addr, info.data);
      - 实际上，这就是将中断发送到 apic 的过程，上面的注释也分析过，采用类似 msi 的过程
      - stl_le_phys 经过 QEMU 构建的 memory model
        - apic_mem_write
          - apic_send_msi : 最终选择正确的 CPU 进行中断的发送

```txt
#0  apic_send_msi (msi=0x7fffffffd110) at ../hw/intc/apic.c:726
#1  0x0000555555c6ab4c in apic_mem_write (opaque=<optimized out>, addr=4100, val=48, size=<optimized out>) at ../hw/intc/apic.c:757
#2  0x0000555555cd2711 in memory_region_write_accessor (mr=mr@entry=0x55555698bc90, addr=4100, value=value@entry=0x7fffffffd298, size=size@entry=4, shift=<optimized out>, mask=mask@entry=4294967295, attrs=...) at ../softmmu/memory.c:492
#3  0x0000555555cceb9e in access_with_adjusted_size (addr=addr@entry=4100, value=value@entry=0x7fffffffd298, size=size@entry=4, access_size_min=<optimized out>, access_size_max=<optimized out>, access_fn=access_fn@entry=0x555555cd2680 <memory_region_write_accessor>, mr=0x55555698bc90, attrs=...) at ../softmmu/memory.c:554
#4  0x0000555555cd1c47 in memory_region_dispatch_write (mr=mr@entry=0x55555698bc90, addr=4100, data=<optimized out>, data@entry=48, op=op@entry=MO_32, attrs=attrs@entry=...) at ../softmmu/memory.c:1504
#5  0x0000555555ca12ed in address_space_stl_internal (endian=DEVICE_LITTLE_ENDIAN, result=0x0, attrs=..., val=48, addr=<optimized out>, as=0x0) at /home/maritns3/core/kvmqemu/include/exec/memory.h:2868
#6  address_space_stl_le (as=as@entry=0x555556606820 <address_space_memory>, addr=<optimized out>, val=48, attrs=attrs@entry=..., result=result@entry=0x0) at /home/maritns3/core/kvmqemu/memory_ldst.c.inc:357
#7  0x0000555555cee124 in stl_le_phys (val=<optimized out>, addr=<optimized out>, as=0x555556606820 <address_space_memory>) at /home/maritns3/core/kvmqemu/include/exec/memory_ldst_phys.h.inc:121
#8  ioapic_service (s=s@entry=0x555556a42360) at ../hw/intc/ioapic.c:138
#9  0x0000555555cee3ff in ioapic_set_irq (opaque=0x555556a42360, vector=<optimized out>, level=1) at ../hw/intc/ioapic.c:186
#10 0x0000555555b92664 in gsi_handler (opaque=0x555556af6ff0, n=0, level=1) at ../hw/i386/x86.c:600
#11 0x0000555555b42a9e in qemu_irq_pulse (irq=0x555556ab3b50) at /home/maritns3/core/kvmqemu/include/hw/irq.h:22
#12 update_irq (timer=<optimized out>, set=<optimized out>) at ../hw/timer/hpet.c:219
#13 0x0000555555e6fe88 in timerlist_run_timers (timer_list=0x555556707120) at ../util/qemu-timer.c:573
#14 timerlist_run_timers (timer_list=0x555556707120) at ../util/qemu-timer.c:498
#15 0x0000555555e70097 in qemu_clock_run_all_timers () at ../util/qemu-timer.c:669
#16 0x0000555555e4ced9 in main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:542
#17 0x0000555555c58231 in qemu_main_loop () at ../softmmu/runstate.c:726
#18 0x0000555555940c92 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:50
```

## how interrupt inserted to vCPU
首先来回忆一下整个 vCPU 的执行流程

- `sigsetjmp(cpu->jmp_env, 0)`
-  while (!cpu_handle_exception(cpu))
    -  while (!cpu_handle_interrupt(cpu))
      - tb = tb_find()
      - cpu_loop_exec_tb(tb)

当需要插入中断的时候，最后调用到 cpu_interrupt 上，
```c
    cpu->interrupt_request |= mask;
```
在 cpu_handle_interrupt 中检测到 interrupt_request 上之后，会调用和 arch 相关的实现函数来处理:

- x86_cpu_exec_interrupt : 调用 x86 注册
  - x86_cpu_pending_interrupt : 获取当前到来的是什么中断
  - cpu_get_pic_interrupt
    - apic_get_interrupt : 获取 into
  - do_interrupt_x86_hardirq
    - do_interrupt_all
      - do_interrupt64 : 64 bit 的模式
      - do_interrupt_protected : 32 bit 的处理模式
        - 在其中解析 guest idt 的内容，然后让 guest 的执行跳转到该位置

## interrupt in x86 Linux kernel
可以首先读读 [Writing an OS in Rust : CPU Exceptions](https://os.phil-opp.com/cpu-exceptions/) 来复习一下 x86 中断。

### code flow from first instruction
使用一些[技术](https://martins3.github.io/tips-reading-kernel.html)在 interrupt handler 的位置 backtrace


- i8042 键盘中断流程
```txt
[   36.794135] Call Trace:
[   36.794140]  <IRQ>
[   36.794143]  dump_stack+0x64/0x7c
[   36.794150]  pollwake+0x2a/0x90
[   36.794157]  ? check_preempt_curr+0x3a/0x70
[   36.794162]  ? ttwu_do_wakeup.isra.0+0xd/0xd0
[   36.794167]  __wake_up_common+0x75/0x140
[   36.794172]  __wake_up_common_lock+0x77/0xb0
[   36.794178]  evdev_events+0x7c/0xa0
[   36.794184]  input_to_handler+0x90/0xf0
[   36.794190]  input_pass_values.part.0+0x119/0x140
[   36.794196]  input_handle_event+0x20e/0x5f0
[   36.794203]  input_event+0x4a/0x70
[   36.794208]  atkbd_interrupt+0x47f/0x640
[   36.794213]  serio_interrupt+0x42/0x90
[   36.794218]  i8042_interrupt+0x146/0x250
[   36.794223]  __handle_irq_event_percpu+0x38/0x150
[   36.794229]  handle_irq_event_percpu+0x2c/0x80
[   36.794234]  handle_irq_event+0x23/0x50
[   36.794252]  handle_edge_irq+0x79/0x190
[   36.794257]  __common_interrupt+0x39/0x90
[   36.794277]  common_interrupt+0x76/0xa0
[   36.794283]  </IRQ>
[   36.794285]  asm_common_interrupt+0x1e/0x40
```

- usb 键盘中断流程
```txt
[   75.597619] [<900000000020866c>] show_stack+0x2c/0x100
[   75.597621] [<9000000000ec39c8>] dump_stack+0x90/0xc0
[   75.597624] [<9000000000c4b1b0>] input_event+0x30/0xc8
[   75.597626] [<9000000000ca3ee4>] hidinput_report_event+0x44/0x68
[   75.597628] [<9000000000ca1e30>] hid_report_raw_event+0x230/0x470
[   75.597631] [<9000000000ca21a4>] hid_input_report+0x134/0x1b0
[   75.597632] [<9000000000cb07ac>] hid_irq_in+0x9c/0x280
[   75.597634] [<9000000000be9cf0>] __usb_hcd_giveback_urb+0xa0/0x120
[   75.597636] [<9000000000c23a7c>] finish_urb+0xac/0x1c0
[   75.597638] [<9000000000c24b50>] ohci_work.part.8+0x218/0x550
[   75.597640] [<9000000000c27f98>] ohci_irq+0x108/0x320
[   75.597642] [<9000000000be96e8>] usb_hcd_irq+0x28/0x40
[   75.597644] [<9000000000296430>] __handle_irq_event_percpu+0x70/0x1b8
[   75.597645] [<9000000000296598>] handle_irq_event_percpu+0x20/0x88
[   75.597647] [<9000000000296644>] handle_irq_event+0x44/0xa8
[   75.597648] [<900000000029abfc>] handle_level_irq+0xdc/0x188
[   75.597651] [<90000000002952a4>] generic_handle_irq+0x24/0x40
[   75.597652] [<900000000081dc50>] extioi_irq_dispatch+0x178/0x210
[   75.597654] [<90000000002952a4>] generic_handle_irq+0x24/0x40
[   75.597656] [<9000000000ee4eb8>] do_IRQ+0x18/0x28
[   75.597658] [<9000000000203ffc>] except_vec_vi_end+0x94/0xb8
[   75.597660] [<9000000000203e80>] __cpu_wait+0x20/0x24
[   75.597662] [<900000000020fa90>] calculate_cpu_foreign_map+0x148/0x180
```

- ssd
```txt
/*
#0  nvme_irq (irq=24, data=0xffff888101150e00) at drivers/nvme/host/pci.c:1066
#1  0xffffffff810bb448 in __handle_irq_event_percpu (desc=desc@entry=0xffff888101163200, flags=flags@entry=0xffffc90000003f84) at kernel/irq/handle.c:156
#2  0xffffffff810bb58c in handle_irq_event_percpu (desc=desc@entry=0xffff888101163200) at kernel/irq/handle.c:196
#3  0xffffffff810bb603 in handle_irq_event (desc=desc@entry=0xffff888101163200) at kernel/irq/handle.c:213
#4  0xffffffff810bf4c9 in handle_edge_irq (desc=0xffff888101163200) at kernel/irq/chip.c:819
#5  0xffffffff81021c19 in generic_handle_irq_desc (desc=0xffff888101163200) at ./include/linux/irqdesc.h:158
#6  handle_irq (regs=<optimized out>, desc=0xffff888101163200) at arch/x86/kernel/irq.c:231
#7  __common_interrupt (regs=<optimized out>, vector=37) at arch/x86/kernel/irq.c:250
#8  0xffffffff81b94c46 in common_interrupt (regs=0xffffc90000013868, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc90000004010
*/
```
这里留下一个问题，在 `common_interrupt` 的参数 vector = 37 和 `nvme_irq` 的参数 irq=24 分别值得是什么?

#### start from idt

```c
/**
 * idt_setup_apic_and_irq_gates - Setup APIC/SMP and normal interrupt gates
 */
void __init idt_setup_apic_and_irq_gates(void)
{
  int i = FIRST_EXTERNAL_VECTOR;
  void *entry;

  idt_setup_from_table(idt_table, apic_idts, ARRAY_SIZE(apic_idts), true);

  for_each_clear_bit_from(i, system_vectors, FIRST_SYSTEM_VECTOR) {
    entry = irq_entries_start + 8 * (i - FIRST_EXTERNAL_VECTOR);
    set_intr_gate(i, entry);
}
```

在 idtentry.h 中间:
```asm
/*
 * ASM code to emit the common vector entry stubs where each stub is
 * packed into 8 bytes.
 *
 * Note, that the 'pushq imm8' is emitted via '.byte 0x6a, vector' because
 * GCC treats the local vector variable as unsigned int and would expand
 * all vectors above 0x7F to a 5 byte push. The original code did an
 * adjustment of the vector number to be in the signed byte range to avoid
 * this. While clever it's mindboggling counterintuitive and requires the
 * odd conversion back to a real vector number in the C entry points. Using
 * .byte achieves the same thing and the only fixup needed in the C entry
 * point is to mask off the bits above bit 7 because the push is sign
 * extending.
 */
  .align 8
SYM_CODE_START(irq_entries_start)
    vector=FIRST_EXTERNAL_VECTOR // FIRST_EXTERNAL_VECTOR 是 0x20
    .rept NR_EXTERNAL_VECTORS    //
  UNWIND_HINT_IRET_REGS
0 :
  .byte 0x6a, vector            // 这里就是装配
  jmp asm_common_interrupt
  nop
  /* Ensure that the above is 8 bytes max */
  . = 0b + 8
  vector = vector+1
    .endr
SYM_CODE_END(irq_entries_start)
```
通过 `.rept` 上面的代码自动生成了很多类似的代码，大致如下:
```asm
pushq 0x20
jmp asm_common_interrupt
pushq 0x21
jmp asm_common_interrupt
pushq 0x22
jmp asm_common_interrupt
...
```

#### jump to C function

在 idtentry.h 中定义了:
```c
/* Device interrupts common/spurious */
DECLARE_IDTENTRY_IRQ(X86_TRAP_OTHER,  common_interrupt);
```

idtentry.h 会分别被 c 源文件和 asm 源文件 include，所以其定义也分别有两种
```c
#define DECLARE_IDTENTRY_IRQ(vector, func)        \
  asmlinkage void asm_##func(void);       \
  asmlinkage void xen_asm_##func(void);       \
  __visible void func(struct pt_regs *regs, unsigned long error_code)

/* Entries for common/spurious (device) interrupts */
#define DECLARE_IDTENTRY_IRQ(vector, func)        \
  idtentry_irq vector func
```

在 `arch/x86/entry/entry_64.S` 中间定义了 `idtentry_irq`，下面分析其是如何被一步步展开的:
```asm
.macro idtentry_irq vector cfunc
  idtentry \vector asm_\cfunc \cfunc has_error_code=1
.endm
```

```asm
.macro idtentry vector asmsym cfunc has_error_code:req
SYM_CODE_START(\asmsym)
  idtentry_body \cfunc \has_error_code
SYM_CODE_END(\asmsym)
.endm
```

```asm
.macro idtentry_body cfunc has_error_code:req

  call  error_entry // 其中的 error_entry 是用于保存 pt_regs 的上下文的

  // 将 rsp 数值传递给 rdi
  // 根据 x86 abi 的标准, rdi 就是第二个参数，rsi 是第一个参数
  // pt_regs 正好是 x86 放到 stack 的内容
  movq  %rsp, %rdi /* pt_regs pointer into 1st argument*/

  .if \has_error_code == 1
    movq  ORIG_RAX(%rsp), %rsi  /* get error code into 2nd argument*/
  .endif

  // 当然这个函数就是 common_interrupt 了
  call  \cfunc

  jmp error_return
.endm
```

第二个参数，也即是 irq number 是从 `(void *)pt_regs + ORIG_RAX` 获取的，这就是在 irq_entries_start 中生成的，并且通过 `.byte 0x6a, vector` 放到 stack 上的。
```c
/*
 * On syscall entry, this is syscall#. On CPU exception, this is error code.
 * On hw interrupt, it's IRQ number:
 */
#define ORIG_RAX 120
```
将上面所有的整合起来，就相当于生成了:
```asm
asm_common_interrupt
  call  error_entry
  movq  %rsp, %rdi /* pt_regs pointer into 1st argument*/
  movq  ORIG_RAX(%rsp), %rsi  /* get error code into 2nd argument*/
  call  common_interrupt
  jmp error_return
```
`common_interrupt` 是一般设备中断的入口, 例如 ipi 以及 timer 等中断的走的入口不同。

#### route to interrupt handler
不同的中断会走不同的 idt 入口，但是那些常规中断最后到达 `common_interrupt`, 在 idt 不同入口体现在其调用 `common_interrupt` 的参数 vector 不同。

- `common_interrupt` : 查看 `DEFINE_IDTENTRY_IRQ` 的定义，`common_interrupt` 接受两个参数 `struct pt_regs *regs, u32 vector`
  - 从 `percpu irq_desc` 数组也就是 `vector_irq` 中找到获取 `irq_desc`
  - `handle_irq`
    - `generic_handle_irq_desc` : 调用 `irq_desc::handle_irq` 来选择 edge 还是 level 的处理
      - `handle_edge_irq`
        - `handle_irq_event`
          - `handle_irq_event_percpu`
            - `__handle_irq_event_percpu`
              - `for_each_action_of_desc(desc, action)`
              - `action->handler(irq, action->dev_id)`

`irq_desc` 同时存在 `handle_irq` 和 action，前者来注册 `handle_edge_irq` ，后者注册 `nvme_irq`
在 Professional Linux Kerne Architecture 的 14.1.5 Interrupt Flow Handling 的分析是很有道理的，通过 `irq_desc::handle_irq` 来处理 flow 的，
通过 `irq_desc::action` 实现具体 irq 需要执行的动作。

```c
struct irq_desc {
  irq_flow_handler_t  handle_irq;
  struct irqaction  *action;  /* IRQ action list */
}
```

### how ioapic got programmed
ioapic 的作用的输入是引脚编号，其最后会告知一个 lapic 的哪一个中断到了

简单来说，这个引脚编号，就是对应着 gsi, 在内核中，也称之为 linux irq
通过调用函数 `irq_to_desc` 可以索引到 `irq_desc`
```c
struct irq_desc *irq_to_desc(unsigned int irq)
{
  return radix_tree_lookup(&irq_desc_tree, irq);
}
```
使用 `cat /proc/interrupts` 看到的是
```txt
            CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7
   0:          8          0          0          0          0          0          0          0  IR-IO-APIC    2-edge      timer
   1:         53          0          0          0          0          0        150          0  IR-IO-APIC    1-edge      i8042
   8:          0          0          0          0          0          0          0          1  IR-IO-APIC    8-edge      rtc0
   9:         13         13          0          0          0          0          0          0  IR-IO-APIC    9-fasteoi   acpi
  12:         83          0          0          0          0        143          0          0  IR-IO-APIC   12-edge      i8042
  14:       1000          1          0          0          0          0          0          0  IR-IO-APIC   14-fasteoi   INT344B:00
  16:       7726          0          0        756          0          0          0          0  IR-IO-APIC   16-fasteoi   idma64.0, i801_smbus, i2c_designware.0
  17:          0          0          0          0          0          0          0          0  IR-IO-APIC   17-fasteoi   idma64.1, i2c_designware.1
 120:          0          0          0          0          0          0          0          0  DMAR-MSI    0-edge      dmar0
 121:          0          0          0          0          0          0          0          0  DMAR-MSI    1-edge      dmar1
```
最左侧的编号就是 gsi 了。

vector index 就是 common_interrupt 中的参数，用于索引 `vector_irq`
vector index 实际上是 lapic 发送给 CPU 的中断数值，导致第 vector index 的 idt 被执行。
```c
DEFINE_PER_CPU(vector_irq_t, vector_irq) = {
  [0 ... NR_VECTORS - 1] = VECTOR_UNUSED,
};
```

Understanding Linux Kernel 中的 Table 4-2. Interrupt vectors in Linux

| Vector range        | Use                                                                                                         |
|---------------------|-------------------------------------------------------------------------------------------------------------|
| 0–19 (0x0-0x13)     | Nonmaskable interrupts and exceptions                                                                       |
| 20–31 (0x14-0x1f)   | Intel-reserved                                                                                              |
| 32–127 (0x20-0x7f)  | External interrupts (IRQs)                                                                                  |
| 128 (0x80)          | Programmed exception for system calls (see Chapter 10)                                                      |
| 129–238 (0x81-0xee) | External interrupts (IRQs)                                                                                  |
| 239 (0xef)          | Local APIC timer interrupt (see Chapter 6)                                                                  |
| 240 (0xf0)          | Local APIC thermal interrupt (introduced in the Pentium 4 models)                                           |
| 241–250 (0xf1-0xfa) | Reserved by Linux for future use                                                                            |
| 251–253 (0xfb-0xfd) | Interprocessor interrupts (see the section "Interprocessor Interrupt Handling" later in this chapter)       |
| 254 (0xfe)          | Local APIC error interrupt (generated when the local APIC detects an erroneous condition)                   |
| 255 (0xff)          | Local APIC spurious interrupt (generated if the CPU masks an interrupt while the hardware device raises it) |

其中用于 External interrupts 的那些 vector 数值就是 vector index 的可以取值。

而 ioapic 中映射表其实存储了 gsi 到 vector index 的。


下面分析一个经典例子:
```txt
/*
#0  apic_update_irq_cfg (irqd=irqd@entry=0xffff88810004e840, vector=33, cpu=1) at arch/x86/kernel/apic/vector.c:120
#1  0xffffffff810bd9a8 in assign_vector_locked (irqd=irqd@entry=0xffff88810004e840, dest=dest@entry=0xffffffff82efdb60 <vector_searchmask>) at arch/x86/kernel/apic/vector.c:253
#2  0xffffffff810bdb9b in assign_irq_vector_any_locked (irqd=0xffff88810004e840) at arch/x86/kernel/apic/vector.c:279
#3  0xffffffff810bdfa4 in activate_reserved (irqd=0xffff88810004e840) at arch/x86/kernel/apic/vector.c:393
#4  x86_vector_activate (dom=<optimized out>, irqd=0xffff88810004e840, reserve=<optimized out>) at arch/x86/kernel/apic/vector.c:462
#5  0xffffffff81136d4e in __irq_domain_activate_irq (irqd=0xffff88810004e840, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1761
#6  0xffffffff81136d2d in __irq_domain_activate_irq (irqd=irqd@entry=0xffff888100100a28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1758
#7  0xffffffff811387f0 in irq_domain_activate_irq (irq_data=irq_data@entry=0xffff888100100a28, reserve=reserve@entry=false) at kernel/irq/irqdomain.c:1784
#8  0xffffffff81135bfb in irq_activate (desc=desc@entry=0xffff888100100a00) at kernel/irq/chip.c:291
#9  0xffffffff81133515 in __setup_irq (irq=irq@entry=9, desc=desc@entry=0xffff888100100a00, new=new@entry=0xffff888100325a00) at kernel/irq/manage.c:1709
#10 0xffffffff81133a57 in request_threaded_irq (irq=9, handler=handler@entry=0xffffffff814e36f0 <acpi_irq>, thread_fn=thread_fn@entry=0x0 <fixed_percpu_data>, irqflags=irqflags@entry=128, devname=devname@entry=0xffffffff8243c085 "acpi", dev_id=dev_id@entry=0xffffffff814e36f0 <acpi_irq>) at kernel/irq/manage.c:2173
#11 0xffffffff814e3af7 in request_irq (dev=0xffffffff814e36f0 <acpi_irq>, name=0xffffffff8243c085 "acpi", flags=128, handler=0xffffffff814e36f0 <acpi_irq>, irq=<optimized out>) at ./include/linux/interrupt.h:167
#12 acpi_os_install_interrupt_handler (gsi=9, handler=handler@entry=0xffffffff814ffd00 <acpi_ev_sci_xrupt_handler>, context=0xffff888100309ae0) at drivers/acpi/osl.c:586
#13 0xffffffff814ffd47 in acpi_ev_install_sci_handler () at drivers/acpi/acpica/evsci.c:156
#14 0xffffffff814fd4c3 in acpi_ev_install_xrupt_handlers () at drivers/acpi/acpica/evevent.c:94
#15 0xffffffff82ddf987 in acpi_enable_subsystem (flags=flags@entry=2) at drivers/acpi/acpica/utxfinit.c:184
#16 0xffffffff82dddca0 in acpi_bus_init () at drivers/acpi/bus.c:1230
#17 acpi_init () at drivers/acpi/bus.c:1323
#18 0xffffffff81000def in do_one_initcall (fn=0xffffffff82dddc1c <acpi_init>) at init/main.c:1278
#19 0xffffffff82daa3d3 in do_initcall_level (command_line=0xffff8881001277c0 "root", level=4) at ./include/linux/compiler.h:250
#20 do_initcalls () at init/main.c:1367
#21 do_basic_setup () at init/main.c:1387
#22 kernel_init_freeable () at init/main.c:1589
#23 0xffffffff81c38121 in kernel_init (unused=<optimized out>) at init/main.c:1481
#24 0xffffffff81001992 in ret_from_fork () at arch/x86/entry/entry_64.S:295
#25 0x0000000000000000 in ?? ()
```

在 acpi_ev_install_sci_handler 中，传递给 acpi_os_install_interrupt_handler 的参数 gsi 的数值是从 acpi_gbl_FADT.sci_interrupt 中获取的。
这非常符合逻辑，因为中断接入到哪一个引脚上的信息就是从 acpi 上获取的，acpi 的信息需要和主板上是对应的。
而 vector index 则是动态生成的。

在 irq_domain_activate_irq 会递归的调用 irq_domain_ops::activate 的，从 parent 开始，也即是 lapic 然后是 ioapic 的
在 lapic 的 irq_domain_ops::activate 调用中，分配出来 vector index, 而在 ioapic 的 irq_domain_ops::activate 中，
建立 vector index 和 gsi（也即是 ioapic 的引脚）的联系，这是靠写 ioapic 的地址空间实现的。

- x86_vector_activate
  - activate_reserved
    - assign_irq_vector_any_locked
      - assign_vector_locked
        - irq_matrix_alloc : irq_matrix::alloc_start 和 irq_matrix::alloc_end 决定了分配的起始位置
        - apic_update_vector :  通过 gsi 用于索引出来一个 irq_desc 之后，该 desc 最后赋值给 vector_irq 上
        - apic_update_irq_cfg
          - apicd->hw_irq_cfg.vector = vector; // 将分配出来的 vector 存储到此处, 给接下来 ioapic 更新路由表使用

- mp_irqdomain_activate
  - ioapic_configure_entry : 看 `__add_pin_to_irq_node` 的注释: The common case is 1:1 `IRQ<->pin` mappings.
      - ioapic_setup_msg_from_msi : 构造 msi
          - irq_chip_compose_msi_msg :  构建 struct msi_msg
              - irq_chip::irq_compose_msi_msg
                - x86_vector_msi_compose_msg  : 这就是注册的 hook 函数
                  - __x86_vector_msi_compose_msg
                    - msg->arch_data.vector = cfg->vector; // 使用 x86_vector_activate 初始化出来的 vector
          - 使用 struct msi_msg 来填充 struct IO_APIC_route_entry
      - `__ioapic_write_entry` : 将 msi 信息写入到 ioapic 的重定向表格

现在可以回答刚才的问题:
> 在 common_interrupt 的参数 vector = 37 和 nvme_irq 的参数 irq=24 分别值得是什么?

vector = 37 就是 vector index, 而  irq = 24 是 gsi 也即是 linux irq.

## intel manual
如果完全没有基础，可以阅读一下 Understanding Linux kernel 的相关章节。

下面是 intel SDM 中相关的几个章节:
- volume 3 CHAPTER 6 (INTERRUPT AND EXCEPTION HANDLING) : 从 CPU 的角度描述了中断的处理过程
- volume 3 CHAPTER 10 (ADVANCED PROGRAMMABLE INTERRUPT CONTROLLER (APIC)): apic
  - 10.8.3.1 Task and Processor Priorities
  - 10.8.4 Interrupt Acceptance for Fixed Interrupts : irr 表示 apic 接受的中断，isr 表示正在处理的中断
  - 10.8.5 Signaling Interrupt Servicing Completion : 描述 eoi 的作用, 软件写 eoi，然后就从 isr 中可以获取下一个需要处理的中断
  - 10.11.1 Message Address Register Format : 描述 MSI 地址的格式, 从中看到一个中断如何发送到特定的 vector 的

还有几个不错的文档可以阅读一下:
- [Part 1. Interrupt controller evolution](https://habr.com/en/post/446312/)
- [Part 2. Linux kernel boot options](https://habr.com/en/post/501660/)
- [Part 3. Interrupt routing setup in a chipset, with the example of coreboot](https://habr.com/en/post/501912/)
- [How to figure out the interrupt source on I/O APIC?](https://stackoverflow.com/questions/57704146/how-to-figure-out-the-interrupt-source-on-i-o-apic)

### irr and isr
- apic_set_irq : 中断首先提交给 irr 的
- apic_get_interrupt : 进行从 irr 到 isr 的转移, 表示 cpu 将会处理该中断
- apic_update_irq : 提醒 cpu 存在有, 整个模拟过程中，很多位置都采用

如果没有 priority 的限制，从 irr 就是立刻到 isr 上，否则就首先在 irr 上等着
高优先级的可以打断低优先级的。
发送 EOI 中断可以接下来执行 isr 上的下一个中断，当然高优先级的也可以让 cpu 执行下一个中断。

### tpr
- cr8 和 tpr 的关系是什么？
    - apic_set_tpr 的唯一调用者是 helper_write_crN
    - 从 apic_set_tpr 和 apic_get_tpr 的效果看，intel SDM Figure 10-18 的 sub-class 的实际上没有用的
    - [^5] 中间描述，如果一个中断的优先级不够, 那么是无法通知 CPU 的，那么如何知道一个中断的优先级
    - tpr 的意义在于计算出来 ppr, 因为 ppr 是 tpr + isrv 中的较大值，只有一个中断的优先级大于 ppr 才可以

- apic_get_interrupt : 从 irr 中接受中断之后，然后立刻装换到系统中间
  - apic_irq_pending : 获取 [0, 256] 的 intno
    - get_highest_priority_int : 看看到底有没有 irr, 如果没有就是没有中断发生了
    - apic_get_ppr : 通过 isr 获取 isrv


## Advanced topic

### how kernel switch from pic to apic
从 gsi 到 apic 的基本流程:

和 kvm 非常类似，在系统启动之后，抛弃使用 pic,
具体表现为 apic_accept_pic_intr 的这个判断失败
通过分析 QEMU 的源代码，可以知道
在 apic_mem_write 中，对于 lvt 被 masked 了

在  apic_mem_write 中添加如下的代码:
```c
X86CPU *cpu = X86_CPU(current_cpu);

if(index == 0x32 + APIC_LVT_LINT0)
  printf("huxueshi:%s %lx\n", __FUNCTION__, cpu->env.eip);
```

输出发现 guest 的地址为 : 0xffffffff810c14d0

使用 gdb 找到这一行:
```gdb
>>> info line *0xffffffff810c14d0
Line 33 of "./include/asm-generic/fixmap.h" starts at address 0xffffffff810c14d0 <native_apic_mem_write> and ends at 0xffffffff810c14d2 <native_apic_mem_write+2>.
```

```txt
#0  native_apic_mem_write (reg=848, v=1792) at ./include/asm-generic/fixmap.h:33
#1  0xffffffff810bc7eb in apic_write (val=1792, reg=848) at ./arch/x86/include/asm/apic.h:394
#2  setup_local_APIC () at arch/x86/kernel/apic/apic.c:1698
#3  0xffffffff82dbd0ad in apic_bsp_setup (upmode=<optimized out>) at arch/x86/kernel/apic/apic.c:2601
#4  apic_intr_mode_init () at arch/x86/kernel/apic/apic.c:1444
#5  0xffffffff82db1cd8 in x86_late_time_init () at arch/x86/kernel/time.c:100
#6  0xffffffff82daa109 in start_kernel () at init/main.c:1080
#7  0xffffffff81000107 in secondary_startup_64 () at arch/x86/kernel/head_64.S:283
```

进而可以找到 在 setup_local_APIC 中存在 Set up LVT0, LVT1 相关的代码, 这个会进行屏蔽

使用 tcg 的时候(否则是 kvm 模拟了)，在 QEMU 初始化会调用一次 apic_mem_write
在内核启动之前会调用一次, 之后 seabios 会调用数次
```txt
(qemu) huxueshi:apic_mem_write addr=0 // qemu 初始化 hpet 的时候代码自动触发的
huxueshi:apic_mem_write addr=f0 // 都是 kernel 启动之前搞定的
huxueshi:apic_mem_write addr=350
huxueshi:apic_mem_write eip=ec676 // 暂时没有方法通过地址找 seabios 的源代码
huxueshi:apic_mem_write val=8700
huxueshi:apic_mem_write addr=360
huxueshi:apic_mem_write addr=300
huxueshi:apic_mem_write addr=300
```
因为 seabios 的代码很简单，其实可以很容易的 seabios 操作 apic 的位置在 smp_scan 中


### kvmvapic
在 https://lists.gnu.org/archive/html/qemu-devel/2012-02/msg00519.html 描述了大致原理，使用 paravirt 减少 vm exit

guest 需要执行 pc-bios/kvmvapic.bin 的代码来实现和 host 交互
在 vapic_realize 中间，注释掉代码，不添加 kvmvapic.bin 这个 rom 那么就取消掉这个加速了。

### switch intc
我们知道最开始的时候，guest 会使用 pic ，之后切换为 apic 的，现在我试图找到在 linux kernel 的源码中定位具体发生修改的位置。

在系统启动之后，抛弃使用 pic 的具体表现为 apic_accept_pic_intr 的这个判断失败
也就是 APICCommonState::lvt[APIC_LVT_NB] 被 mask 掉了，而这个操作是在
apic_mem_write 中进行的。

在 apic_mem_write 中添加如下的代码:
```c
X86CPU *cpu = X86_CPU(current_cpu);

if(index == 0x32 + APIC_LVT_LINT0)
  printf("%lx\n", cpu->env.eip);
```

输出发现 guest 的地址为 : 0xffffffff810c14d0

使用 gdb 找到这一行:
```txt
>>> info line *0xffffffff810c14d0
Line 33 of "./include/asm-generic/fixmap.h" starts at address 0xffffffff810c14d0 <native_apic_mem_write> and ends at 0xffffffff810c14d2 <native_apic_mem_write+2>.
```

```txt
/*
#0  native_apic_mem_write (reg=848, v=1792) at ./include/asm-generic/fixmap.h:33
#1  0xffffffff810bc7eb in apic_write (val=1792, reg=848) at ./arch/x86/include/asm/apic.h:394
#2  setup_local_APIC () at arch/x86/kernel/apic/apic.c:1698
#3  0xffffffff82dbd0ad in apic_bsp_setup (upmode=<optimized out>) at arch/x86/kernel/apic/apic.c:2601
#4  apic_intr_mode_init () at arch/x86/kernel/apic/apic.c:1444
#5  0xffffffff82db1cd8 in x86_late_time_init () at arch/x86/kernel/time.c:100
#6  0xffffffff82daa109 in start_kernel () at init/main.c:1080
#7  0xffffffff81000107 in secondary_startup_64 () at arch/x86/kernel/head_64.S:283
```

进而可以找到 在 **setup_local_APIC** 中存在 Set up LVT0, LVT1 相关的代码了。

### legacy PCI interrupt
legacy PCI interrupt 指的是 PCI 提供四根 interrupt line 链接到 intc 上，所有的 pci 设备需要共享这些 interrupt line

在 guest 中运行 lspci 来查看所有的 PCI 设备:

```txt
00:00.0 Host bridge: Intel Corporation 440FX - 82441FX PMC [Natoma] (rev 02)
00:01.0 ISA bridge: Intel Corporation 82371SB PIIX3 ISA [Natoma/Triton II]
00:01.1 IDE interface: Intel Corporation 82371SB PIIX3 IDE [Natoma/Triton II]
00:01.3 Bridge: Intel Corporation 82371AB/EB/MB PIIX4 ACPI (rev 03)
00:02.0 VGA compatible controller: Red Hat, Inc. Virtio GPU (rev 01)
00:03.0 Ethernet controller: Intel Corporation 82540EM Gigabit Ethernet Controller (rev 03)
00:04.0 Non-Volatile memory controller: Red Hat, Inc. Device 0010 (rev 02)
00:05.0 Non-Volatile memory controller: Red Hat, Inc. Device 0010 (rev 02)
00:06.0 Unclassified device [0002]: Red Hat, Inc. Virtio filesystem
```

虽然 piix3 是一个 ISA bridge, 但是实际上在 QEMU 中 piix3 和 ISA 没有啥关系，
piix3 负责控制 pci interrupt line 具体路由到那个中断上, 在 seabios 配置 piix3 的配置空间就可以了:

```c
/* PIIX3/PIIX4 PCI to ISA bridge */
static void piix_isa_bridge_setup(struct pci_device *pci, void *arg)
{
    int i, irq;
    u8 elcr[2];

    elcr[0] = 0x00;
    elcr[1] = 0x00;
    for (i = 0; i < 4; i++) {
        irq = pci_irqs[i];
        /* set to trigger level */
        elcr[irq >> 3] |= (1 << (irq & 7));
        /* activate irq remapping in PIIX */
        pci_config_writeb(pci->bdf, 0x60 + i, irq);
    }
    outb(elcr[0], PIIX_PORT_ELCR1);
    outb(elcr[1], PIIX_PORT_ELCR2);
    dprintf(1, "PIIX3/PIIX4 init: elcr=%02x %02x\n", elcr[0], elcr[1]);
}
```
其中的 pci_config_writeb 最后会调用到 QEMU 中的  piix3_write_config

- nvme_irq_assert
  - nvme_irq_check
    - pci_irq_assert
      - pci_set_irq
        - int intx = pci_intx(pci_dev) : 从 PCI 配置空间读去这个设备的 PCI_INTERRUPT_PIN, PCI_INTERRUPT_PIN 表示 PCI 设备接入到 PCI interrupt line 的输入端
        - pci_irq_handler
          - pci_change_irq_level
            - PCIBus::map_irq : 也即是 pci_slot_get_pirq 根据 dev 的位置计算出来在 PCI interrupt line 的输出位置。
            - pci_bus_change_irq_level : 调用 PCIBus::map_irq，这个 hook 是在 piix3_create 中初始化为 piix3_set_irq
              - piix3_set_irq
                - piix3_set_irq_level
                  - pic_irq = piix3->dev.config[PIIX_PIRQCA + pirq] : 读去配置从而知道中断是路由到哪里去的

但是，一个设备是如何知道自己发出的中断是发送到哪一个 pci interrupt line 上的，这就靠 PCIBus::map_irq 了
具体而言，这个 hook 注册的函数是:

```c
/*
 * Return the global irq number corresponding to a given device irq
 * pin. We could also use the bus number to have a more precise mapping.
 */
static int pci_slot_get_pirq(PCIDevice *pci_dev, int pci_intx)
{
    int slot_addend;
    slot_addend = PCI_SLOT(pci_dev->devfn) - 1;
    return (pci_intx + slot_addend) & 3;
}
```

### interrupt flow
在 [jump to C function](#jump-to-c-function) 中提到，linux kernel 利用 irq_desc::handle_irq 来处理
interrupt 的 tirgger 类型。

至于 level 和 edge 的关系，可以参考 [stackoverflow](https://stackoverflow.com/questions/7005331/difference-between-io-apic-fasteoi-and-io-apic-edge) 和 [IBM](https://www.ibm.com/docs/en/aix/7.2?topic=interrupts-interrupt-trigger) 的解释。

在 x86 中，使用 irq_desc::handle_irq 可以注册为:
1. handle_edge_irq
2. handle_fasteoi_irq
3. handle_level_irq

handle_level_irq 只有在 setup_default_timer_irq 中注册 timer_interrupt 是时候使用，使用这个 hook 主要是处理 legacy 的设备的，处理 level flow 的情况更多是
handle_fasteoi_irq
```txt
#0  timer_interrupt (irq=0, dev_id=0x0 <fixed_percpu_data>) at arch/x86/kernel/time.c:57
#1  0xffffffff81132995 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100051800, flags=flags@entry=0xffffc90000003f84) at kernel/irq/handle.c:156
#2  0xffffffff81132acc in handle_irq_event_percpu (desc=desc@entry=0xffff888100051800) at kernel/irq/handle.c:196
#3  0xffffffff81132b33 in handle_irq_event (desc=desc@entry=0xffff888100051800) at kernel/irq/handle.c:213
#4  0xffffffff8113686f in handle_level_irq (desc=0xffff888100051800) at kernel/irq/chip.c:650
#5  0xffffffff81098419 in generic_handle_irq_desc (desc=0xffff888100051800) at ./include/linux/irqdesc.h:158
#6  handle_irq (regs=<optimized out>, desc=0xffff888100051800) at arch/x86/kernel/irq.c:231
#7  __common_interrupt (regs=<optimized out>, vector=48) at arch/x86/kernel/irq.c:250
#8  0xffffffff81c366ee in common_interrupt (regs=0xffffffff82603dc8, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
但是随着系统启动，很快就切换为 lapic 来提供时钟中断，其 flow 类型为 interrupt
```txt
/*
#0  task_tick_fair (rq=0xffff8881b9c29700, curr=0xffff888100208000, queued=0) at kernel/sched/fair.c:10992
#1  0xffffffff8110d5d8 in scheduler_tick () at kernel/sched/core.c:4954
#2  0xffffffff81152cfb in update_process_times (user_tick=0) at kernel/time/timer.c:1801
#3  0xffffffff81161752 in tick_periodic (cpu=cpu@entry=0) at ./arch/x86/include/asm/ptrace.h:136
#4  0xffffffff811617bb in tick_handle_periodic (dev=0xffff8881b9c16f80) at kernel/time/tick-common.c:112
#5  0xffffffff810bc0f7 in local_apic_timer_interrupt () at arch/x86/kernel/apic/apic.c:1089
#6  __sysvec_apic_timer_interrupt (regs=<optimized out>) at arch/x86/kernel/apic/apic.c:1106
#7  0xffffffff81c3810d in sysvec_apic_timer_interrupt (regs=0xffffc90000013c98) at arch/x86/kernel/apic/apic.c:1100
```
因为 pci interrupt line 是共享的，而只有 level 类型才可以用于共享，可以看到 e1000 的就是经过 handle_fasteoi_irq 的。
```txt
/*
#0  e1000_intr (irq=11, data=0xffff888100232000) at drivers/net/ethernet/intel/e1000/e1000_main.c:3749
#1  0xffffffff81132995 in __handle_irq_event_percpu (desc=desc@entry=0xffff888100100e00, flags=flags@entry=0xffffc90000003f7c) at kernel/irq/handle.c:156
#2  0xffffffff81132acc in handle_irq_event_percpu (desc=desc@entry=0xffff888100100e00) at kernel/irq/handle.c:196
#3  0xffffffff81132b33 in handle_irq_event (desc=desc@entry=0xffff888100100e00) at kernel/irq/handle.c:213
#4  0xffffffff81136751 in handle_fasteoi_irq (desc=0xffff888100100e00) at kernel/irq/chip.c:714
#5  0xffffffff81098419 in generic_handle_irq_desc (desc=0xffff888100100e00) at ./include/linux/irqdesc.h:158
#6  handle_irq (regs=<optimized out>, desc=0xffff888100100e00) at arch/x86/kernel/irq.c:231
#7  __common_interrupt (regs=<optimized out>, vector=40) at arch/x86/kernel/irq.c:250
#8  0xffffffff81c3670e in common_interrupt (regs=0xffffc900008b3b98, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
下面分析一下，通过 elcr[^12] 是如何操控 pic 的中断类型的

首先，提供在 io 空间中注册 elcr 的两个端口

- pic_common_realize : 将 PICCommonState::elcr_io 这个地址空间注册到 isa 上去
  - `isa_register_ioport(isa, &s->elcr_io, s->elcr_addr);`
- pic_realize : 注册 handler, 作用就是修改 PICCommonState::elcr 的数值，最后的作用体现在 pic_set_irq 上的
  - `memory_region_init_io(&s->elcr_io, OBJECT(s), &pic_elcr_ioport_ops, s, "elcr", 1);`

```txt
address-space: I/O
  0000000000000000-000000000000ffff (prio 0, i/o): io
    ...
    00000000000004d0-00000000000004d0 (prio 0, i/o): elcr
    00000000000004d1-00000000000004d1 (prio 0, i/o): elcr
```
然后在 seabios 中的 piix_isa_bridge_setup 会调用 pic_elcr_ioport_ops 更新 PICCommonState::elcr
```c
struct PICCommonState {
    uint8_t elcr; /* PIIX edge/trigger selection*/
}
```

在 `pic_set_irq` 中可以看到 PICCommonState::elcr 如何影响中断的
```c
static void pic_set_irq(void *opaque, int irq, int level)
{
    // ...
    if (s->elcr & mask) {
        /* level triggered */
        if (level) {
            s->irr |= mask;
            s->last_irr |= mask;
        } else {
            s->irr &= ~mask;
            s->last_irr &= ~mask;
        }
    } else {
        /* edge triggered */
        if (level) {
            if ((s->last_irr & mask) == 0) {
                s->irr |= mask;
            }
            s->last_irr |= mask;
        } else {
            s->last_irr &= ~mask;
        }
    }
    pic_update_irq(s);
}
```

## references
[^5]: https://stackoverflow.com/questions/51490552/how-is-cr8-register-used-to-prioritize-interrupts-in-an-x86-64-cpu
[^11]: https://cloud.tencent.com/developer/article/1087271
[^12]: https://en.wikipedia.org/wiki/Intel_8259

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>
