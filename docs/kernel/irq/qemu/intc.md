## 中断是如何产生的

- main 
  - qemu_main_loop 
    - main_loop_wait 
      - os_host_main_loop_wait 
        - glib_pollfds_poll 
          - g_main_context_dispatch 
            - gtk_main_do_event 
              - g_signal_emit 
                - g_signal_emit_valist 
                  - gtk_window_propagate_key_event 
                    - g_signal_emit 
                      - g_signal_emit_valist 
                        - g_closure_invoke 
                          - gd_key_event 
                            - qkbd_state_key_event 
                              - qkbd_state_key_event 
                                - qemu_input_event_send_key_qcode 
                                  - qemu_input_event_send_key 
                                    - qemu_input_event_send_impl 
                                      - ps2_keyboard_event 
                                        - ps2_put_keycode 
                                          - kbd_update_irq_lines 

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


- clone 
  - start_thread 
    - qemu_thread_start 
      - rr_cpu_thread_fn 
        - tcg_cpus_exec 
          - cpu_exec 
            - cpu_loop_exec_tb 
              - cpu_tb_exec 
                - code_gen_buffer 
                  - address_space_stl 
                    - address_space_stl_internal 
                      - memory_region_dispatch_write 
                        - access_with_adjusted_size 
                          - memory_region_write_accessor 
                            - ide_data_writel 
                              - ide_atapi_cmd 
                                - ide_atapi_cmd_error 
                                  - ide_set_irq 
                                    - ide_set_irq 
                                      - qemu_irq_raise 
                                        - gsi_handler 
                                          - ioapic_set_irq 
                                            - ioapic_service 
                                              - stl_le_phys 
                                                - address_space_stl_le 
                                                  - address_space_stl_internal 
                                                    - memory_region_dispatch_write 
                                                      - access_with_adjusted_size 
                                                        - memory_region_write_accessor 
                                                          - apic_mem_write 
                                                            - apic_deliver_irq 
                                                              - apic_bus_deliver 
                                                                - apic_set_irq 

总而言之，是通过 interrupt 模块提供给设备模拟模块的标准函数，例如 qemu_irq_raise 和 qemu_set_irq

## 中断控制器的具体模拟

### 相关文件
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

## 使用 qemu 还是内核的中断控制器

man qemu 
```txt
              kernel-irqchip=on|off|split
                     Controls KVM in-kernel irqchip support. The default is
                     full  acceleration  of  the  interrupt controllers. On
                     x86, split irqchip reduces the kernel attack  surface,
                     at  a  performance  cost  for non-MSI interrupts. Dis‐
                     abling the in-kernel irqchip completely is not  recom‐
                     mended except for debugging purposes.
```


QEMU 一共支持三种模式，分别 off / split / on
- off : lapic 以及 pic ioapic 都是在用户态模拟的
- split : lapic 在内核中
- on : 都是在内核中模拟

hw/intc/ioapic.c 会出现 kvm_irqchip_is_split 是因为 ioapic 在用户态模拟，但是需要将消息注入到 kernel 中。

观察上面的 TypeInfo 还有一个比较有意思的地方在于，其 instance_size 总是等于 sizeof(CommonState)，这说明
QEMU 模拟和内核模拟使用结构体是相同的，实际上，对于 kvm 而言，因为 irqchip 是在内核模拟的，其作用在于临时保存一下内核中的数据(使用 kvm_get_apic_state / kvm_put_apic_state) 从而实现虚拟机的迁移



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


## 中断控制器在内核中

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
