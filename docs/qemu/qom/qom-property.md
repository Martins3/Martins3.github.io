# qom property

qom property 算是 qom 中比较复杂的部分，需要单独说明下，

关于 QEMU 中 property, 可以参考 [Paolo Bonzini 在 2014 KVM Forum 上的总结](
https://www.linux-kvm.org/images/9/90/Kvmforum14-qom.pdf)

All properties are accessed through visitors:
- Non-object
	- Example: isa-serial.iobase=0x402
	- QOM property types are QAPI types
- Object
	- `child<X>` provides the canonical path to an object
	- `link<X>` provides alternative paths
- Aliases
	- Same type as the target, except `child<X>` → `link<X>`

## 基本概念

```c
class MachineState {
public:
    bool usb;  // 每个实例一份

    bool get_usb() const { return usb; }
    void set_usb(bool value) { usb = value; }

    // 概念上的属性描述符：类级共享
    static PropertyDescriptor usb_property;
};
```

也就是说 QEMU 实际上分成两层:
```txt
MachineClass/ObjectClass
  └── properties["usb"]
        ├── type = "bool"
        ├── get = machine_get_usb
        └── set = machine_set_usb
                 │
                 ▼
MachineState 实例
  └── ms->usb                每个实例独立
```

所有的 instance 共享相同的 properties["usb"] ，但是
但是这些 instance 的 properties["usb"] 函数作用的成员是
各自的 instance 的

```py
class Machine:
    @property
    def usb(self):
        return self._usb

    @usb.setter
    def usb(self, value):
        self._usb = value
```

这里：

- Machine.usb 描述符只有一份，类似 QOM class property。
- self._usb 每个对象一份，类似 MachineState::usb。
- getter/setter 每次收到具体的 self，类似 QOM 的 Object *obj。

结合下面的 backtrace 可以分析出来，
即使 property 是 class 的，所有的 instance 共享，但是依旧可以设置到 object 的属性上。
```txt
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qmp_x_exit_preconfig
        - qemu_init_board
          - machine_run_board_init
            - pc_init1
              - x86_cpus_init
                - x86_cpu_new
                  - qdev_realize
                    - object_property_set_bool
                      - object_property_set_qobject
                        - object_property_set
                          - property_set_bool
                            - device_set_realized
                              - x86_cpu_realizefn
                                - x86_cpu_apic_create
                                  - object_new_with_type
                                    - object_initialize_with_type
                                      - object_class_property_init_all
                                        - object_property_init_defval
                                          - set_uint8
```

## 分类

 QOM API                                         property 描述符范围                     通常的值范围
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 object_class_property_add_bool(klass, ...)      当前 class 及其派生 class 的全部实例    getter/setter 决定；通常是实例成员
──────────────────────────────────────────────  ──────────────────────────────────────  ────────────────────────────────────────
 object_property_add_bool(obj, ...)              仅指定的那个 object                     通常是该实例成员
──────────────────────────────────────────────  ──────────────────────────────────────  ────────────────────────────────────────
 object_class_static_property_add_uint*_ptr()    当前 class 及其派生 class 的全部实例    明确指向实例外的全局变量，所有实例共享

### class property
这个是最常见的

### static class property
真正类似 C++ static 成员的例子在 hw/riscv/spike.c 的 spike_machine_class_init()：

object_class_static_property_add_uint8_ptr(
    oc, "signature-granularity",
    &line_size,
    OBJ_PROP_FLAG_WRITE);

line_size 定义在 hw/char/riscv_htif.c：

uint8_t line_size = 16;

这才近似：

```txt
class SpikeMachine {
    static uint8_t line_size;
};

```
所有 Spike machine 实例访问 signature-granularity，最终操作的都是同一个全局 line_size。

目前整个 QEMU 只有这一个属性是这个操作方法。

### instance property

因为有些属性并不属于该类型的所有实例。例如 hw/core/machine.c 的 machine_initfn()：

```txt
if (mc->nvdimm_supported) {
    object_property_add_bool(obj, "nvdimm", ...);
}
```

所以 QOM 实际采用的是：

- class property = 类型声明的固定接口，描述符共享，值通常在实例中
- instance property = 某个具体对象动态拥有的扩展接口

#### 案例

memory_region_initfn 中定义的属性不必要是所有的 class 都需要

object_class_property_add() 到 2016 年才由 commit 16bf7f522a2f 引入。
因此当时只能在 memory_region_initfn() 中调用 object_property_add()。

- class property：每个类型只保存一份 ObjectProperty 描述。
- object property：每个实例保存一份属性描述。
- 两者的 getter 都接收具体的 Object *obj，所以 class property 同样可以读取每个 MemoryRegion 实例自己的 mr->priority 和 mr->size。

因此现在这两个属性完全可以改成：
```c
static void memory_region_class_init(ObjectClass *oc, const void *data)
{
    object_class_property_add(oc, "priority", "uint32",
                              memory_region_get_priority,
                              NULL, NULL, NULL);
    object_class_property_add(oc, "size", "uint64",
                              memory_region_get_size,
                              NULL, NULL, NULL);
}
```

然后在 TypeInfo 中增加：

```c
  .class_init = memory_region_class_init,
```

这样语义基本不变，但所有 MemoryRegion 实例共享属性描述，可以减少每个实例的属性分配。

同一个 memory_region_initfn() 中的其他属性不能如此简单地迁移：
- container 的 ObjectProperty 返回值还被单独设置了 op->resolve，历史上也作为实例属性处理。

```c
    op = object_property_add(OBJECT(mr), "container",
                             "link<" TYPE_MEMORY_REGION ">",
                             memory_region_get_container,
                             NULL, /* memory_region_set_container */
                             NULL, NULL);
    op->resolve = memory_region_resolve_container;
```

- addr 的 opaque 实际指向每个实例的 &mr->addr，所以必须是 instance property ，也就是 Property 不仅仅
是 get 和 set 的函数指针
```c
    object_property_add_uint64_ptr(OBJECT(mr), "addr",
                                   &mr->addr, OBJ_PROP_FLAG_READ);
```


## property 内部实现
```c
struct ObjectClass
{
    GHashTable *properties;
};

struct Object
{
    GHashTable *properties;
};
```

当查询 ObjectProperty 的时候，这会同时查询两个 ObjectClass::properties 和 Object::properties 中的内容:
```c
ObjectProperty *object_property_find(Object *obj, const char *name)
{
    ObjectProperty *prop;
    ObjectClass *klass = object_get_class(obj);

    prop = object_class_property_find(klass, name);
    if (prop) {
        return prop;
    }

    return g_hash_table_lookup(obj->properties, name);
}
```


## 如何添加 Property

### device_class_set_props

例如定义到所有的 PCIDevice 上的属性
```c
static Property pci_props[] = {
		// ...
    DEFINE_PROP_BIT("multifunction", PCIDevice, cap_present,
                    QEMU_PCI_CAP_MULTIFUNCTION_BITNR, false),
		// ...
    DEFINE_PROP_END_OF_LIST()
};
```
将 macro 展开之后:

```c
static Property pci_props[] = {
    {.name = ("multifunction"),
     .info = &(qdev_prop_bit),
     .offset = offsetof(PCIDevice, cap_present) +
               type_check(uint32_t, typeof_field(PCIDevice, cap_present)),
     .bitnr = (QEMU_PCI_CAP_MULTIFUNCTION_BITNR),
     .set_default = true,
     .defval.u = (bool)false},
    {}};
```


### object_class_property_add

例如 kvm_accel_class_init 中

```txt
    object_class_property_add(oc, "kernel-irqchip", "on|off|split",
        NULL, kvm_set_kernel_irqchip,
        NULL, NULL);
```

## QOM composition tree
property 中间不仅仅可以存储 str / int 之类基本类型，还可以用于存储 Object 。
通过 link 和 child 类型的 property 可以构建出来 QOM tree

在 QEMU monitor 中使用 `info qom-tree` 可以查看 QOM tree,
全部的内容列举到了[这里](./info-qom-tree-tcg.txt)，下面只是一部分。
```txt
/machine (pc-i440fx-6.1-machine)
  /fw_cfg (fw_cfg_io)
    /\x2from@etc\x2facpi\x2frsdp[0] (memory-region)
    /\x2from@etc\x2facpi\x2ftables[0] (memory-region)
    /\x2from@etc\x2ftable-loader[0] (memory-region)
    /fwcfg.dma[0] (memory-region)
    /fwcfg[0] (memory-region)
  /i440fx (i440FX-pcihost)
    /ioapic (ioapic)
      /ioapic[0] (memory-region)
      /unnamed-gpio-in[0] (irq)
  /unattached (container)
    /device[0] (qemu64-x86_64-cpu)
      /lapic (apic)
        /apic-msi[0] (memory-region)
      /memory[0] (memory-region)
      /memory[1] (memory-region)
      /smram[0] (memory-region)
```
然后就可以通过路径直接获取到一个 object 了，例如:

```c
MemoryRegion *smram = (MemoryRegion *) object_resolve_path("/machine/smram", NULL);
```
#### child
使用上面的 qom tree 作为例子说明。

- 每一级缩进都是表示 child 和 parent 关系，例如 machine 的 child 分别为 fw_cfg / i440fx 和 unattached
- 小括号里面是 object 的 Type 类型，具体参考(print_qom_composition ->  object_get_typename)

#### link
回顾一下刚才的例子:
```c
MemoryRegion *smram = (MemoryRegion *) object_resolve_path("/machine/smram", NULL);
```

实际上，我们发现访问 smram 正确的路径应该是 "/machine/unattached/device[0]/smram[0]" 的


路径解析的一般过程为:

- object_resolve_path_type
	- object_resolve_abs_path
		- object_resolve_path_component
			- object_property_find
			- ObjectProperty::resolve 也就是 object_resolve_link_property 或者 object_resolve_child_property

在 i440fx_init 中
`object_property_add_const_link(qdev_get_machine(), "smram", OBJECT(&f->smram));`
这导致解析路径到 smram 之后，调用到 object_resolve_link_property, 最后返回的是 `OBJECT(&f->smram)`

实际上，在 QEMU 中 link 作用还可以和 object_property_add_str 类似，只是将 string 替换为	`* object`
例如在 pic 控制器中的:

- 通过 object_property_add_link 创建 property

- pic_realize
  - `qdev_init_gpio_out(dev, s->int_out, ARRAY_SIZE(s->int_out));`
		- qdev_init_gpio_out_named
			- object_property_add_link : 这里提供了一个 PICCommonState::int_out 上

- 通过 object_property_set_link 赋值这个 property

- qdev_connect_gpio_out_named
  - object_property_set_link : 实际上，这就是一个简答的赋值操作
    - object_get_canonical_path : 不是通过继承构建的，而是通过 priority 构建的
    - object_property_set_str
      - object_property_set_qobject
        - object_property_set : 对于 PICCommonState::int_out 进行赋值

### alias
alias 可以根据让两个名称找到同一个 property

比如在 x86_cpu_initfn 中间的操作:
```c
    object_property_add_alias(obj, "sse3", obj, "pni", &error_abort);
    object_property_add_alias(obj, "pclmuldq", obj, "pclmulqdq", &error_abort);
    object_property_add_alias(obj, "sse4-1", obj, "sse4.1", &error_abort);
    object_property_add_alias(obj, "sse4-2", obj, "sse4.2", &error_abort);
```

在比如 pc_machine_initfn 中:
```c
object_property_add_alias(OBJECT(pcms), "pcspk-audiodev", OBJECT(pcms->pcspk), "audiodev");
```



### 实现对于默认赋值的修改
```c
PCIDevice *pci_new_multifunction(int devfn, bool multifunction,
                                 const char *name)
{
    DeviceState *dev;

    dev = qdev_new(name);
    qdev_prop_set_int32(dev, "addr", devfn);
    qdev_prop_set_bit(dev, "multifunction", multifunction);
    return PCI_DEVICE(dev);
}
```
通过 qdev_prop_set_bit 之类的最后可以设置到 pci_props 描述的 PCIDevice 上的成员上。


这个真的很烦，让代码索引工具失效，
hw/isa/piix.c 中有这个
```c
        qdev_prop_set_bit(DEVICE(&d->pm), "smm-enabled", d->smm_enabled);
```
但是通过 struct PIIX4PMState::smm_enabled 是找不到的，应该不是 ccls 的问题

此外，真的有办法通过 cmdline 给 bit 设置上 bit 吗?

但是，这个地方，就绝对不可能发现吧:
```c
        object_property_set_bool(OBJECT(pci_dev), "smm-enabled",
                                 x86_machine_is_smm_enabled(x86ms),
                                 &error_abort);
```

```txt
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_init_board
        - machine_run_board_init
          - pc_init1
```

```txt
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_init_board
        - machine_run_board_init
          - pc_init1
            - pci_realize_and_unref
              - qdev_realize_and_unref
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - pci_qdev_realize
                              - pci_piix_realize
```

发现 pci_piix_realize 的调用来自于这里:
```c
        pci_dev = pci_new_multifunction(-1, pcms->south_bridge);
        object_property_set_bool(OBJECT(pci_dev), "has-usb",
                                 machine_usb(machine), &error_abort);
        object_property_set_bool(OBJECT(pci_dev), "has-acpi",
                                 x86_machine_is_acpi_enabled(x86ms),
                                 &error_abort);
        object_property_set_bool(OBJECT(pci_dev), "has-pic", false,
                                 &error_abort);
        object_property_set_bool(OBJECT(pci_dev), "has-pit", false,
                                 &error_abort);
        qdev_prop_set_uint32(DEVICE(pci_dev), "smb_io_base", 0xb100);
        object_property_set_bool(OBJECT(pci_dev), "smm-enabled",
                                 x86_machine_is_smm_enabled(x86ms),
                                 &error_abort);
        dev = DEVICE(pci_dev);
        for (i = 0; i < ISA_NUM_IRQS; i++) {
            qdev_connect_gpio_out_named(dev, "isa-irqs", i, x86ms->gsi[i]);
        }
        pci_realize_and_unref(pci_dev, pcms->pcibus, &error_fatal);
```
所以，我猜测，的确是没有办法实现 cmdline 来控制 piix 的

## GlobalProperty
一种通过 -global 选项来在启动的时候修改 object property 的方式，几乎没有人使用吧!

通过 Man qemu-system(1) 中找到的:
```txt
-global driver.prop=value
-global driver=driver,property=property,value=value
   Set default value of driver's property prop to value, e.g.:

           qemu-system-x86_64 -global ide-hd.physical_block_size=4096 disk-image.img

   In particular, you can use this to set driver properties for devices which are created automatically by the
   machine model. To create a device which is not created automatically and set properties on it, use -device.

   -global driver.prop=value is shorthand for -global driver=driver,property=prop,value=value.  The longhand
   syntax works even when driver contains a dot.
```
此外添加 GlobalProperty 是在 pc.c 中的:
```c
GlobalProperty pc_compat_6_0[] = {
    { "qemu64" "-" TYPE_X86_CPU, "family", "6" },
    { "qemu64" "-" TYPE_X86_CPU, "model", "6" },
    { "qemu64" "-" TYPE_X86_CPU, "stepping", "3" },
    { TYPE_X86_CPU, "x-vendor-cpuid-only", "off" },
    { "ICH9-LPC", "acpi-pci-hotplug-with-bridge-support", "off" },
};
```

构建的 GlobalProperty 主要通过 qdev_prop_register_global 添加到 global_props 上

使用 object_apply_global_props 来将 global_props 中存储的 property apply 到特定的 object 上。

object_apply_global_props 主要的两个调用位置:
- device_post_init
- do_configure_accelerator

## 经典案例


### -cpu host,tsc-frequency=1000000000

由于是在 cpu 这个 class 添加 property :
```c
    object_class_property_add(oc, "tsc-frequency", "int",
                              x86_cpuid_get_tsc_freq,
                              x86_cpuid_set_tsc_freq, NULL, NULL);
```

### machine.usb

```c
static bool machine_get_usb(Object *obj, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    return ms->usb;
}

static void machine_set_usb(Object *obj, bool value, Error **errp)
{
    MachineState *ms = MACHINE(obj);

    ms->usb = value;
    ms->usb_disabled = !value;
}
```

```c
    object_class_property_add_bool(oc, "usb",
        machine_get_usb, machine_set_usb);
```

想要修改 MachineState.usb 可以用如下四个方法:
#### 启动参数

```txt
    qemu-system-x86_64 -machine q35,usb=off
    qemu-system-x86_64 -machine q35,usb=on
```


- main
  - qemu_init
    - qemu_apply_machine_options
      - object_set_props_from_keyval
        - object_set_props_from_qdict
          - object_property_set
            - property_set_bool
              - machine_set_usb

#### hmp
```txt
    (qemu) qom-set /machine usb off
    (qemu) qom-get /machine usb
```
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - handle_hmp_command_exec
                            - hmp_qom_set
                              - object_property_parse
                                - object_property_set
                                  - property_set_bool
                                    - machine_set_usb

#### qmp
```txt

    { "execute": "qom-set",
      "arguments": { "path": "/machine", "property": "usb", "value": false } }
```
或者用 qmp shell
```txt
  (QEMU) qom-set path=/machine property=usb value=false
  {"return": {}}
```
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_bh_poll
                      - aio_bh_call
                        - do_qmp_dispatch_bh
                          - qmp_marshal_qom_set
                            - object_property_set_qobject
                              - object_property_set
                                - property_set_bool
                                  - machine_set_usb

#### 内部 C 代码

```txt
    Error *err = NULL;
    object_property_set_bool(OBJECT(machine), "usb", false, &err);
```

更底层的通用接口还有 object_property_set()（Visitor）和 object_property_set_qobject()；
命令行 -machine ...,usb=... 最终也通过 QOM 属性设置路径到达此 setter。

## TODO

1. 这里我们看到了 qmp 和 hmp 是走的不同的路径，为什么这么设计
	2. 如果这样，意味着 qmp 和 hmp 可以同时对外服务，例如 docs/qemu/migration/hotplug.md 终端
		- 我感觉可以找到更多的有趣的反例来，就是现在很多代码都是假设qmp 的执行是一条一条的

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
