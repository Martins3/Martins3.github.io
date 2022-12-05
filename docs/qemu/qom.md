# QEMU 中的面向对象 : QOM

<!-- vim-markdown-toc GitLab -->

* [basic](#basic)
* [init](#init)
  * [type_init](#type_init)
  * [init static part](#init-static-part)
  * [init Non-static part](#init-non-static-part)
* [cast](#cast)
* [property](#property)
  * [Non-object](#non-object)
  * [QOM composition tree](#qom-composition-tree)
    * [child](#child)
    * [link](#link)
  * [alias](#alias)
  * [GlobalProperty](#globalproperty)
  * [struct Property](#struct-property)
* [qdev](#qdev)
  * [realize](#realize)
  * [qtree](#qtree)
* [QOM in action](#qom-in-action)
  * [cpu type](#cpu-type)
  * [qdev realize](#qdev-realize)
* [misc](#misc)

<!-- vim-markdown-toc -->
因为 QEMU 整个项目是 C 语言写的，但是 QEMU 处理的对象例如主板，CPU, 总线，外设实际上存在很多继承的关系。
所以，QEMU 为了方便整个系统的构建，实现了自己的一套的面向对象机制，也就是 QEMU Object Model（下面称为 QOM）。

首先，回忆一下面向对象的基本知识:
- 继承(inheritance)
- 静态成员(static field)
- 构造函数和析构函数(constructor and destructor)
- 多态(polymorphic)
	- 动态绑定(override)
	- 静态绑定(overload)
- 抽象类/虚基类(abstract class)
- 动态类型装换(dynamic cast)
- 接口(interface)

好的，下面我们将会分析 QEMU 是如何实现这些特性，以及 QEMU 扩展的高级特性。

## basic

* **在 QEMU 中通过 TypeInfo 来定义一个类。**

例如 `x86_base_cpu_type_info` 就是一个 class
```c
static const TypeInfo x86_base_cpu_type_info = {
        .name = X86_CPU_TYPE_NAME("base"),
        .parent = TYPE_X86_CPU,
        .class_init = x86_cpu_base_class_init,
};
```

* **利用结构体包含来实现继承**

这应该是所有的语言实现继承的方法，在 C++ 中，结构体包含的操作被语言内部实现了，而 C 语言需要手动写出来。

例如 `x86_cpu_type_info` 的 parent 是 `cpu_type_info`, 他们的结构体分别是
`X86CPU` 和 `CPUState`
```c
static const TypeInfo x86_cpu_type_info = {
    .name = TYPE_X86_CPU,
    .parent = TYPE_CPU,
		// ...
    .instance_size = sizeof(X86CPU),
};

static const TypeInfo cpu_type_info = {
    .name = TYPE_CPU,
    .parent = TYPE_DEVICE,
		// ...
    .instance_size = sizeof(CPUState),
};
```

在 `X86CPU` 中包含一个 `CPUState` 的。
```c
struct X86CPU {
    /*< private >*/
    CPUState parent_obj;
    /*< public >*/

    CPUNegativeOffsetState neg;
```

* **静态成员是所有的对象共享的，而非静态的每一个对象都有一份**

面向对象中的基本概念，qemu 也实现了静态变量和静态函数。
还是来观察 `x86_cpu_type_info` 的实现。

```c
static const TypeInfo x86_cpu_type_info = {
     // ...
    .instance_size = sizeof(X86CPU),
     // ...
    .class_size = sizeof(X86CPUClass),
};
```
其中 X86CPU 就是包含的就是非静态成员，而 X86CPUClass 描述的是静态的成员

* **QEMU 中所有的对象的 parent 是 Object 和 ObjectClass**

Object 存储 Non-static 部分，而 ObjectClass 存储 static 部分。

```c
struct X86CPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/
```


* **构造函数用于初始化对象**

```c
static const TypeInfo x86_cpu_type_info = {
    .instance_init = x86_cpu_initfn,
    .class_init = x86_cpu_common_class_init,
};
```
显然 x86_cpu_initfn 就是用于初始化 x86_cpu_type_info 的。


* **通过函数指针在子类的构造函数中重新赋值实现 override**

x86_cpu_common_class_init 和 cpu_class_init 分别是 `x86_cpu_type_info` 和 `cpu_type_info` 注册的构造函数，其中

对于相同的函数指针 parse_features，x86_cpu_common_class_init 会重新注册为 x86_cpu_parse_featurestr 的
```c

static void x86_cpu_common_class_init(ObjectClass *oc, void *data)
{
    X86CPUClass *xcc = X86_CPU_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);
    DeviceClass *dc = DEVICE_CLASS(oc);

    cc->parse_features = x86_cpu_parse_featurestr;
```

```c
static void cpu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    CPUClass *k = CPU_CLASS(klass);

    k->parse_features = cpu_common_parse_features;
```

* **QEMU 不支持多继承**

个人认为 C++ 中的多继承非常的鬼畜，谢天谢地，QEMU 没有自讨苦吃。

:warning: 到此你花费了 2% 的时间掌握了 80% 的 QOM 的内容，接下来是具体的代码分析部分了。

## init
QEMU 中一个 class 初始化可以大致划分为三个部分:
- type_init : 注册一个 TypeInfo
- TypeInfo::class_init : 初始化静态成员
- TypeInfo::instance_init : 初始化非静态成员

在 qdev 中还有 qdev_realize 来进行和 device 相关的初始化，在 [qdev](#qdev) 中再细谈。

### type_init
```c
static void x86_cpu_register_types(void)
{
		// ...
    type_register_static(&x86_cpu_type_info);
}

type_init(x86_cpu_register_types)
```

type_init 展开之后可以得到:
```c
static void __attribute__((constructor))
do_qemu_init_x86_cpu_register_types(void) {
  register_module_init(x86_cpu_register_types, MODULE_INIT_QOM);
}
```
通过 gcc 扩展属性 `__attribute__((constructor))` 可以让 `do_qemu_init_x86_cpu_register_types` 在运行 main 函数之前运行。
register_module_init 会让 x86_cpu_register_types 这个函数挂载到 init_type_list[MODULE_INIT_QOM] 这个链表上。

在启动 mian 之后，这个 hook 将会被执行:
- main
  - qemu_init
    - qemu_init_subsystems
      - module_call_init : 携带参数 MODULE_INIT_QOM, 那么将会导致曾经靠 type_init 注册上的所有函数全部都调用
				- x86_cpu_register_types : 执行在 constructor 挂载上的 hook
          - type_register_static : 参数为 x86_cpu_type_info
            - type_register
              - type_register_internal
                - type_new : 使用 TypeInfo 初始化 TypeImpl，TypeInfo 和 TypeImpl 内容很类似，基本是拷贝
                - g_hash_table_insert(type_table_get(), (void *)ti->name, ti) : 将创建的 TypeImpl 添加到 type_table 上。

> **type_new : 使用 TypeInfo 初始化 TypeImpl，TypeInfo 和 TypeImpl 内容很类似，基本是拷贝**
简单的来说，TypeInfo 是保存静态注册的数据，而 TypeImpl 保存是运行数据。

到底，所有的 TypeInfo 通过 type_init 都被放到 type_table 上了，之后通过 Typeinfo 的名称调用 type_table_lookup 获取到 TypeImpl 了。

下面分析一个 X86CPUClass 和 X86CPU 是如何初始化的。

### init static part
静态成员是所有的对象公用的，其初始化显然要发生在所有的对象初始化之前。

```c
/*
#0  x86_cpu_common_class_init (oc=0x555555e7e7b8 <device_class_base_init+32>, data=0x7fffffffd330) at ../target/i386/cpu.c:6737
#1  0x0000555555e64a46 in type_initialize (ti=0x55555686beb0) at ../qom/object.c:364
#2  0x0000555555e647b1 in type_initialize (ti=0x555556872820) at ../qom/object.c:312
#3  0x0000555555e66254 in object_class_foreach_tramp (key=0x5555568729a0, value=0x555556872820, opaque=0x7fffffffd4c0) at ../qom/object.c:1069
#4  0x00007ffff70381b8 in g_hash_table_foreach () at /lib/x86_64-linux-gnu/libglib-2.0.so.0
#5  0x0000555555e66337 in object_class_foreach (fn=0x555555e664a0 <object_class_get_list_tramp>, implements_type=0x5555560cb618 "machine", include_abstract=false, opaqu
e=0x7fffffffd510) at ../qom/object.c:1091
#6  0x0000555555e66523 in object_class_get_list (implements_type=0x5555560cb618 "machine", include_abstract=false) at ../qom/object.c:1148
#7  0x0000555555cd8bb0 in select_machine () at ../softmmu/vl.c:1629
#8  0x0000555555cdd514 in qemu_init (argc=28, argv=0x7fffffffd7c8, envp=0x7fffffffd8b0) at ../softmmu/vl.c:3570
#9  0x000055555582e575 in main (argc=28, argv=0x7fffffffd7c8, envp=0x7fffffffd8b0) at ../softmmu/main.c:49
```

select_machine 需要获取所有的 TYPE_MACHINE 的 class,
其首先会调用所有的 class list，其会遍历 type_table，遍历的过程中会顺带 type_initialize 所有的 TypeImpl
进而调用的 class_init

```plain
- object_class_get_list
  - object_class_foreach --> object_class_get_list_tramp (将元素添加到后面) <------------
    - g_hash_table_foreach (对于 type_table 循环) ---> object_class_foreach_tramp       |
                                                          - type_initialize             |
                                                          - object_class_dynamic_cast   |
                                                            - 执行 callback 函数 --------
```

- type_initialize
  - 分配 class 的空间
  - 递归的调用 parent 注册的 class_init 被调用
	- 调用自己的 class_init

### init Non-static part
通过调用 object_new 来实现初始化

- object_initialize_with_type
	- 初始化一个空的 : Object::properties
	- object_init_with_type
		- 如果 object 有 parent，那么调用 object_init_with_type 首先初始化 parent 的
		- 调用 TypeImpl::instance_init

举个例子吧:
```c
/*
#0  x86_cpu_initfn (obj=0x55555699acb0) at ../target/i386/cpu.c:6426
#1  0x0000555555e64ab0 in object_init_with_type (obj=0x555556c8bf90, ti=0x55555686beb0) at ../qom/object.c:375
#2  0x0000555555e64a92 in object_init_with_type (obj=0x555556c8bf90, ti=0x55555687da00) at ../qom/object.c:371
#3  0x0000555555e64a92 in object_init_with_type (obj=0x555556c8bf90, ti=0x55555687df20) at ../qom/object.c:371
#4  0x0000555555e6500b in object_initialize_with_type (obj=0x555556c8bf90, size=42944, type=0x55555687df20) at ../qom/object.c:517
#5  0x0000555555e65740 in object_new_with_type (type=0x55555687df20) at ../qom/object.c:732
#6  0x0000555555e6579f in object_new (typename=0x55555687e0a0 "host-x86_64-cpu") at ../qom/object.c:747
#7  0x0000555555b67369 in x86_cpu_new (x86ms=0x555556a94800, apic_id=0, errp=0x5555567a94b0 <error_fatal>) at ../hw/i386/x86.c:106
#8  0x0000555555b67485 in x86_cpus_init (x86ms=0x555556a94800, default_cpu_version=1) at ../hw/i386/x86.c:138
#9  0x0000555555b7b69b in pc_init1 (machine=0x555556a94800, host_type=0x55555609e70a "i440FX-pcihost", pci_type=0x55555609e703 "i440FX") at ../hw/i386/pc_piix.c:157
#10 0x0000555555b7c24e in pc_init_v6_1 (machine=0x555556a94800) at ../hw/i386/pc_piix.c:425
#11 0x0000555555aec313 in machine_run_board_init (machine=0x555556a94800) at ../hw/core/machine.c:1239
#12 0x0000555555cdada6 in qemu_init_board () at ../softmmu/vl.c:2526
#13 0x0000555555cdaf85 in qmp_x_exit_preconfig (errp=0x5555567a94b0 <error_fatal>) at ../softmmu/vl.c:2600
#14 0x0000555555cdd65d in qemu_init (argc=28, argv=0x7fffffffd7c8, envp=0x7fffffffd8b0) at ../softmmu/vl.c:3635
#15 0x000055555582e575 in main (argc=28, argv=0x7fffffffd7c8, envp=0x7fffffffd8b0) at ../softmmu/main.c:49
```

## cast
QEMU 定义了一些列的 macro 来封装，我将这些 macro 列举到[这里](./res/qom-macros.c)了。
最终将

```c
OBJECT_DECLARE_TYPE(X86CPU, X86CPUClass, X86_CPU)
```

装换为这个了:

```c
typedef struct X86CPU X86CPU;
typedef struct X86CPUClass X86CPUClass;
G_DEFINE_AUTOPTR_CLEANUP_FUNC(X86CPU, object_unref)
static inline G_GNUC_UNUSED X86CPU *X86_CPU(const void *obj) {
  return ((X86CPU *)object_dynamic_cast_assert(
      ((Object *)(obj)), (TYPE_X86_CPU),
      "/home/maritns3/core/vn/docs/qemu/res/qom-macros.c", 64, __func__));
}
static inline G_GNUC_UNUSED X86CPUClass *X86_CPU_GET_CLASS(const void *obj) {
  return ((X86CPUClass *)object_class_dynamic_cast_assert(
      ((ObjectClass *)(object_get_class(((Object *)(obj))))), (TYPE_X86_CPU),
      "/home/maritns3/core/vn/docs/qemu/res/qom-macros.c", 64, __func__));
}
static inline G_GNUC_UNUSED X86CPUClass *X86_CPU_CLASS(const void *klass) {
  return ((X86CPUClass *)object_class_dynamic_cast_assert(
      ((ObjectClass *)(klass)), (TYPE_X86_CPU),
      "/home/maritns3/core/vn/docs/qemu/res/qom-macros.c", 64, __func__));
}
```

- X86_CPU : 将任何一个 object 指针 转换为 X86CPU
- X86_CPU_GET_CLASS : 根据 object 指针获取到 X86CPUClass
- X86_CPU_CLASS : 根据 ObjectClass 指针获取到 X86CPUClass

在分析这些函数之前，将 ObjectClass 和 Object 中和引用计数，property 相关的内容删除之后，得到如下的简化内容:
```c
struct ObjectClass
{
    /* private: */
		struct TypeImpl * type;

    const char *object_cast_cache[OBJECT_CLASS_CAST_CACHE];
    const char *class_cast_cache[OBJECT_CLASS_CAST_CACHE];
};

struct Object
{
    /* private: */
    ObjectClass *class;
};
```

在 type_initialize 中 ObjectClass::type 将会指向 TypeImpl
```c
static void type_initialize(TypeImpl *ti){
		// ...
    ti->class->type = ti;
		// ...
}
```

现在我们就差不多可以猜到 object_dynamic_cast_assert 的实现了:
- 如果关掉动态检查，因为 Object 总是在一个结构体的最开始位置，那么这个转换无需任何操作
- 如果需要动态检查:
	- 首先在 cache 中找该 object 是否可以装换
	- 否则
		- Object 可以获取 ObjectClass
		- ObjectClass 可以获取 TypeImpl
		- TypeImpl 可以判断将要 cast 的类型是不是自己的父类型

## property
All properties are accessed through visitors:

关于 QEMU 中 property, Paolo Bonzini 在 2014 KVM Forum 上的总结[^1]
- Non-object
	- Example: isa-serial.iobase=0x402
	- QOM property types are QAPI types
- Object
	- `child<X>` provides the canonical path to an object
	- `link<X>` provides alternative paths
- Aliases
	- Same type as the target, except `child<X>` → `link<X>`

需要指出的一点是，property 也是划分为 static 和 Non-staic 的，分别挂到 ObjectClass 和 Object 上
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

当查询 ObjectProperty 的时候，这会同时查询两个位置的:
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

### Non-object
这个一般很容易的，例如下面的 string 类型的 property 的访问
唯一比较麻烦的位置是通过 visitor 机制来访问数据，visitor 是 QAPI 引入的，方便 QEMU 和 libvirt 之类的工具交互。
我水平有限，就放到以后分析了。

- object_property_add_str
	- object_property_add
		- object_property_try_add
			- 初始化 ObjectProperty
			- g_hash_table_insert(obj->properties, prop->name, prop); 然后插入到 Object::properties

### QOM composition tree
property 中间不仅仅可以存储 str / int 之类基本类型，还可以用于存储 Object 。
通过 link 和 child 类型的 property 可以构建出来 QOM tree

在 QEMU monitor 中使用 `info qom-tree` 可以查看 QOM tree,
全部的内容列举到了[这里](./res/qom-tree-tcg.txt)，下面只是一部分。
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


### GlobalProperty
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

### struct Property
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
下面分析两件事情:
1. 实现默认赋值

- pci_device_class_init
	- device_class_set_props(dc, ioapic_properties)
		- qdev_class_add_property
			- object_class_property_add
			- prop->info->set_default_value : 也即是 qdev_prop_uint8
				- object_property_set_default_uint
					- object_property_set_default
						- prop->defval = defval; // 注意，此时此刻，只是将数值保存到了 ObjectProperty 中间了
						- prop->init = object_property_init_defval; // 同时注册 hook

结合下面的 backtrace 可以分析出来，即使 property 是 class 的，但是依旧可以设置 object 的属性上。
```c
/*
#0  0x0000555555d3ae20 in set_uint8 () at ../hw/core/qdev-properties.c:269
#1  0x0000555555d23479 in object_property_init_defval (obj=0x5555569e84d0, prop=0x5555567cbc60) at ../qom/object.c:1537
#2  0x0000555555d249b5 in object_class_property_init_all (obj=0x5555569e84d0) at ../qom/object.c:499
#3  object_initialize_with_type (obj=obj@entry=0x5555569e84d0, size=size@entry=656, type=type@entry=0x5555566f2b60) at ../qom/object.c:515
#4  0x0000555555d24ab9 in object_new_with_type (type=0x5555566f2b60) at ../qom/object.c:733
#5  0x0000555555b77f45 in x86_cpu_apic_create (cpu=cpu@entry=0x555556b09d50, errp=errp@entry=0x7fffffffccf0) at ../target/i386/cpu-sysemu.c:274
#6  0x0000555555be256f in x86_cpu_realizefn (dev=0x555556b09d50, errp=0x7fffffffcd50) at ../target/i386/cpu.c:6270
#7  0x0000555555d3e017 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffcdd0) at ../hw/core/qdev.c:761
#8  0x0000555555d22cba in property_set_bool (obj=0x555556b09d50, v=<optimized out>, name=<optimized out>, opaque=0x55555670c3d0, errp=0x7fffffffcdd0) at ../qom/object.c:2258
#9  0x0000555555d251ec in object_property_set (obj=obj@entry=0x555556b09d50, name=name@entry=0x555555fe20f6 "realized", v=v@entry=0x555556a2d4e0, errp=errp@entry=0x555556618678 <error_fatal>) at ../qom/object.c:1403
#10 0x0000555555d21834 in object_property_set_qobject (obj=obj@entry=0x555556b09d50, name=name@entry=0x555555fe20f6 "realized", value=value@entry=0x555556a1ea60, errp=errp@entry=0x555556618678 <error_fatal>) at ../qom/qom-qobject.c:28
#11 0x0000555555d25459 in object_property_set_bool (obj=0x555556b09d50, name=name@entry=0x555555fe20f6 "realized", value=value@entry=true, errp=errp@entry=0x555556618678 <error_fatal>) at ../qom/object.c:1473
#12 0x0000555555d3ce42 in qdev_realize (dev=<optimized out>, bus=bus@entry=0x0, errp=errp@entry=0x555556618678 <error_fatal>) at ../hw/core/qdev.c:389
#13 0x0000555555badea5 in x86_cpu_new (x86ms=x86ms@entry=0x5555568359e0, apic_id=0, errp=errp@entry=0x555556618678 <error_fatal>) at /home/maritns3/core/kvmqemu/include/hw/qdev-core.h:17
#14 0x0000555555badf8e in x86_cpus_init (x86ms=x86ms@entry=0x5555568359e0, default_cpu_version=<optimized out>) at ../hw/i386/x86.c:138
#15 0x0000555555b8aa63 in pc_init1 (machine=0x5555568359e0, pci_type=0x555555f5d125 "i440FX", host_type=0x555555ec0aed "i440FX-pcihost") at ../hw/i386/pc_piix.c:156
#16 0x0000555555a6c094 in machine_run_board_init (machine=0x5555568359e0) at ../hw/core/machine.c:1273
#17 0x0000555555c64ed4 in qemu_init_board () at ../softmmu/vl.c:2615
#18 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2689
#19 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2682
#20 0x0000555555c68678 in qemu_init (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/vl.c:3706
#21 0x0000555555940c8d in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:49
```

2. 实现对于默认赋值的修改
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

## qdev
qdev 出现的位置比 qom 要早，当 qom 出现之后，qdev 按照 qom 的模式重写过。

### realize
device_class_init 中注册了 realized 属性
```c
object_class_property_add_bool(class, "realized", device_get_realized, device_set_realized);
```

- qdev_realize
	- qdev_set_parent_bus : 将 dev 和 bus 联系起来，构建 qtree
	- object_property_set_bool
		- device_set_realized
			- DeviceClass::realized : 调用注册的 hook 函数，将两个函数

```c
/*
#8  0x0000555555be2653 in x86_cpu_realizefn (dev=0x555556b08d50, errp=0x7fffffffcd20) at ../target/i386/cpu.c:6299
#9  0x0000555555d3e027 in device_set_realized (obj=<optimized out>, value=true, errp=0x7fffffffcda0) at ../hw/core/qdev.c:761
#10 0x0000555555d22caa in property_set_bool (obj=0x555556b08d50, v=<optimized out>, name=<optimized out>, opaque=0x55555670c430, errp=0x7fffffffcda0) at ../qom/object.c:2285
#11 0x0000555555d251dc in object_property_set (obj=obj@entry=0x555556b08d50, name=name@entry=0x555555fe20d6 "realized", v=v@entry=0x555556aeabf0, errp=errp@entry=0x555556618678 <error_fatal>) at ../qom/object.c:1410
#12 0x0000555555d21824 in object_property_set_qobject (obj=obj@entry=0x555556b08d50, name=name@entry=0x555555fe20d6 "realized", value=value@entry=0x5555569f30a0, errp=errp@entry=0x555556618678 <error_fatal>) at ../qom/qom-qobject.c:28
#13 0x0000555555d25449 in object_property_set_bool (obj=0x555556b08d50, name=name@entry=0x555555fe20d6 "realized", value=value@entry=true, errp=errp@entry=0x555556618678 <error_fatal>) at ../qom/object.c:1480
#14 0x0000555555d3ce52 in qdev_realize (dev=<optimized out>, bus=bus@entry=0x0, errp=errp@entry=0x555556618678 <error_fatal>) at ../hw/core/qdev.c:389
#15 0x0000555555badf75 in x86_cpu_new (x86ms=x86ms@entry=0x55555677cde0, apic_id=0, errp=errp@entry=0x555556618678 <error_fatal>) at /home/maritns3/core/kvmqemu/include/hw/qdev-core.h:17
```

因为 `device_type_info` 实际上也是 qdev, 其初始化的时候自然也会调用**逐级** class_init 和 instance_init 的。
然后每个设备注册的自己的 realize。

[这里](http://people.redhat.com/~thuth/blog/qemu/2018/09/10/instance-init-realize.html) 进一步分析了 realize 和 class_init/instance_init 的区别。

### qtree
和 qom-tree 非常类似，使用 info qtree 可以获取差不多下面的内容，全部的输出在 [这里](./res/qtree.txt)

```txt
bus: main-system-bus
  type System
  dev: i440FX-pcihost, id ""
    pci-hole64-size = 2147483648 (2 GiB)
    short_root_bus = 0 (0x0)
    x-pci-hole64-fix = true
    x-config-reg-migration-enabled = true
    bypass-iommu = false
    bus: pci.0
      type PCI
      dev: virtio-9p-pci, id ""
        disable-legacy = "off"
        disable-modern = false
        ioeventfd = true
        vectors = 2 (0x2)
        virtio-pci-bus-master-bug-migration = false
```

```c
struct BusState {

    QTAILQ_HEAD(, BusChild) children;
    QLIST_ENTRY(BusState) sibling;
```

```c
struct DeviceState {
    QLIST_HEAD(, BusState) child_bus;
```

- dev 和 bus 是互相交错放置的，这符合物理上设计，总线上挂载设备，总线和总线控制器交互。
  - 在 qbus_init 中间，创建的 bus 的时候，使用 BusState::sibling 将 BusState 挂到 DeviceState::child_bus 上
  - 在 bus_add_child 中，使用 DeviceState::sibling 将 DeviceState 挂到 BusState::children 上

在 qdev_realize -> qdev_set_parent_bus 将会 dev 添加到 bus 上，如果一个 dev 没有关联 bus，类似 hpet 那么就会添加到 main-system-bus 上。

## QOM in action

### cpu type
QEMU 将很多内容按照 QOM 重写了之后，如果不掌握 QOM 的基本知识，有些内容是完全看不懂的，现在我举一个比较经典的例子。
我们知道，即使是同一个指令集的 CPU 每一个版本的功能也是有差异的，使用 lscpu 可以查看当前的 CPU 支持的 feature。
QEMU 可以模拟各种版本的 x86 CPU，现在我们分析一下 QEMU 是如何做的:


1. 在 cpu.c 中会注册全部的 cpu types

- x86_cpu_register_types : 这个函数是通过 type_init 来调用的
  - type_register_static(&x86_cpu_type_info); 其他类型的 parent
  - type_register_static(&max_x86_cpu_type_info); 为什么需要这个 ？
  - type_register_static(&x86_base_cpu_type_info);
  - x86_register_cpudef_types : 对于 builtin_x86_defs 循环调用
    - 组装出来 X86CPUModel
    - x86_register_cpu_model_type : 构建 .class_data = X86CPUModel 的 TypeInfo，在 x86_cpu_cpudef_class_init 的时候，会将这个穿点到 X86CPUClass::model 上

builtin_x86_defs 定义了一组 `X86CPUDefinition`，其中的 version 信息使用
`X86CPUVersionDefinition` 描述，每一个 `X86CPUDefinition` 都会在 x86_register_cpudef_types 中生成一个或者多个，
X86CPUModel(因为 version 的原因)

如果使用 tcg 运行，默认是 qemu64 的:
```c
    {
        .name = "qemu64",
        .level = 0xd,
        .vendor = CPUID_VENDOR_AMD,
        .family = 15,
        .model = 107,
        .stepping = 1,
        .features[FEAT_1_EDX] =
            PPRO_FEATURES |
            CPUID_MTRR | CPUID_CLFLUSH | CPUID_MCA |
            CPUID_PSE36,
        .features[FEAT_1_ECX] =
            CPUID_EXT_SSE3 | CPUID_EXT_CX16,
        .features[FEAT_8000_0001_EDX] =
            CPUID_EXT2_LM | CPUID_EXT2_SYSCALL | CPUID_EXT2_NX,
        .features[FEAT_8000_0001_ECX] =
            CPUID_EXT3_LAHF_LM | CPUID_EXT3_SVM,
        .xlevel = 0x8000000A,
        .model_id = "QEMU Virtual CPU version " QEMU_HW_VERSION,
    },
```
2. 在 x86_cpu_common_class_init -> x86_cpu_register_feature_bit_props 中为每一个 feature bit 注册属性
2. 在 class init 的时候，调用 x86_cpu_cpudef_class_init 来初始化 X86CPUClass::model
此时每一个 X86CPUClass 都会指向自己的 model
3. 在 qemu_init 中进行 `current_machine->cpu_type` 的初始化,
而 pc_machine_class_init 中进行选择 MachineClass::default_cpu_type
当然还可以选择其他的 cpu，其解析工作在 parse_cpu_option，此时确定了具体的哪一个 X86CPUClass 了
4. 在 x86_cpu_initfn 中
```c
    if (xcc->model) {
        x86_cpu_load_model(cpu, xcc->model);
    }
```
- x86_cpu_load_model
  - `object_property_set_int(OBJECT(cpu), "family", def->family, &error_abort);`
    - 类似的赋值还有好几个
  - `env->features[w] = def->features[w];` 拷贝到 CPUX86State::features 中
  - x86_cpu_apply_version_props : 对于 builtin_x86_defs::versions 会在 x86_cpu_def_get_versions 中默认注册一个，其没有关联任何的 prop, 所以最后 x86_cpu_apply_version_props 在 qemu64 的请款下，是一个空操作的
    - object_property_parse
5. 在 x86_cpu_realizefn 中间注册 X86CPUDefinition::cache_info ，qemu64 注册上的就是 legacy 的数值
```c
	env->cache_info_cpuid2.l1d_cache = &legacy_l1d_cache;
```
6. 在 kvm 或者 tcg 的初始化中可以调用 x86_cpu_apply_props 来进行 accel related feature 进行设置。
kvm
```c
/*
#0  x86_cpu_set_bit_prop (obj=0x555555e64ac8 <object_property_find_err+43>, v=0x7fffffffd0a0, name=0x55555689ee30 "\220\356\211VUU", opaque=0x555556963e80, errp=0x555556c32070) at ../target/i386/cpu.c:4001
#1  0x0000555555e64f5a in object_property_set (obj=0x555556c32070, name=0x55555608fef1 "kvmclock", v=0x555556b55070, errp=0x5555567a1ee8 <error_abort>) at ../qom/object.c:1402
#2  0x0000555555e65b0f in object_property_parse (obj=0x555556c32070, name=0x55555608fef1 "kvmclock", string=0x55555608fefa "on", errp=0x5555567a1ee8 <error_abort>) at ../qom/object.c:1642
#3  0x0000555555ba00f3 in x86_cpu_apply_props (cpu=0x555556c32070, props=0x5555566c7e60 <kvm_default_props>) at ../target/i386/cpu.c:2638
#4  0x0000555555b3df02 in kvm_cpu_instance_init (cs=0x555556c32070) at ../target/i386/kvm/kvm-cpu.c:126
#5  0x0000555555c82967 in accel_cpu_instance_init (cpu=0x555556c32070) at ../accel/accel-common.c:110
#6  0x0000555555ba3ffa in x86_cpu_initfn (obj=0x555556c32070) at ../target/i386/cpu.c:4131
```

tcg
```c
/*
#0  x86_cpu_set_bit_prop (obj=0x555555e64ac8 <object_property_find_err+43>, v=0x7fffffffd090, name=0x5555568963e0 "@d\211VUU", opaque=0x555556974ba0, errp=0x555556c28050) at ../target/i386/cpu.c:4001
#1  0x0000555555e64f5a in object_property_set (obj=0x555556c28050, name=0x5555560a7af1 "vme", v=0x555556b56000, errp=0x5555567a1ee8 <error_abort>) at ../qom/object.c:14
#2  0x0000555555e65b0f in object_property_parse (obj=0x555556c28050, name=0x5555560a7af1 "vme", string=0x5555560a7af5 "off", errp=0x5555567a1ee8 <error_abort>) at ../qom/object.c:1642
#3  0x0000555555ba00f3 in x86_cpu_apply_props (cpu=0x555556c28050, props=0x5555566d9c00 <tcg_default_props>) at ../target/i386/cpu.c:2638
#4  0x0000555555bd68f2 in tcg_cpu_instance_init (cs=0x555556c28050) at ../target/i386/tcg/tcg-cpu.c:95
#5  0x0000555555c82967 in accel_cpu_instance_init (cpu=0x555556c28050) at ../accel/accel-common.c:110
#6  0x0000555555ba3ffa in x86_cpu_initfn (obj=0x555556c28050) at ../target/i386/cpu.c:4131
```

这是 tcg 的 feature
```c
/*
 * TCG-specific defaults that override cpudef models when using TCG.
 * Only for builtin_x86_defs models initialized with x86_register_cpudef_types.
 */
static PropValue tcg_default_props[] = {
    { "vme", "off" },
    { NULL, NULL },
};
```

### qdev realize
给大家再介绍一个 QEMU 处理 QOM 非常隐秘的一个点。

```c
PCIBus *i440fx_init(const char *host_type, const char *pci_type, // ...
{
    // ...
    dev = qdev_create(NULL, host_type);
    s = PCI_HOST_BRIDGE(dev);
    b = pci_root_bus_new(dev, NULL, pci_address_space,
                         address_space_io, 0, TYPE_PCI_BUS);
    s->bus = b;
    object_property_add_child(qdev_get_machine(), "i440fx", OBJECT(dev), NULL);
    qdev_init_nofail(dev);
```
review 这个代码，你会发现这里有点不顺眼的地方。

- qdev_create 创建 dev 之后，然后是创建 pci_root_bus_new 之后再去 qdev_init_nofail(dev) 的，为什么需要在 create 和 init 之间插入一个 pci_root_bus_new 的。

我尝试了一下调换顺序，但是虽然可以启动，但是 seabios 的启动会出现一个停滞。

检查 seabios 的 log 可以发现
```txt
Found 1 serial ports                  <- 首先会在这里卡住
WARNING - Timeout at nvme_wait:144!   <- 报错之后继续
```

最后在 @niugenen 和 @rrwhx 的帮助下，终于找到了下面的 backtrace
```c
/*
#0  pci_bus_realize (qbus=0x555556937d20, errp=0x7fffffffcb70) at /home/maritns3/core/xqm/hw/pci/pci.c:121
#1  0x0000555555a2852a in bus_set_realized (obj=<optimized out>, value=<optimized out>, errp=0x7fffffffcc60) at /home/maritns3/core/xqm/hw/core/bus.c:225
#2  0x0000555555bb206b in property_set_bool (obj=0x555556937d20, v=<optimized out>, name=<optimized out>, opaque=0x5555566cda50, errp=0x7fffffffcc60) at /home/maritns3
/core/xqm/qom/object.c:2083
#3  0x0000555555bb6854 in object_property_set_qobject (obj=obj@entry=0x555556937d20, value=value@entry=0x555556709610, name=name@entry=0x555555db1293 "realized", errp=
errp@entry=0x7fffffffcc60) at /home/maritns3/core/xqm/qom/qom-qobject.c:26
#4  0x0000555555bb408a in object_property_set_bool (obj=0x555556937d20, value=<optimized out>, name=0x555555db1293 "realized", errp=0x7fffffffcc60) at /home/maritns3/c
ore/xqm/qom/object.c:1341
#5  0x0000555555a2832d in qbus_realize (bus=bus@entry=0x555556937d20, parent=parent@entry=0x555556782560, name=name@entry=0x0) at /home/maritns3/core/xqm/hw/core/bus.c
:138
#6  0x0000555555a287ce in qbus_create (typename=<optimized out>, parent=0x555556782560, name=0x0) at /home/maritns3/core/xqm/hw/core/bus.c:187
#7  0x0000555555ab7856 in pci_root_bus_new (parent=parent@entry=0x555556782560, name=name@entry=0x0, address_space_mem=address_space_mem@entry=0x555556535300, address_
space_io=address_space_io@entry=0x55555653a300, devfn_min=devfn_min@entry=0 '\000', typename=typename@entry=0x555555d7af80 "PCI") at /home/maritns3/core/xqm/hw/pci/pci
.c:466
#8  0x0000555555ab4c71 in i440fx_init (host_type=host_type@entry=0x555555d74f1b "i440FX-pcihost", pci_type=pci_type@entry=0x555555d75e48 "i440FX", pi440fx_state=pi440f
x_state@entry=0x7fffffffcd90, address_space_mem=address_space_mem@entry=0x555556551700, address_space_io=address_space_io@entry=0x55555653a300, ram_size=6442450944, be
low_4g_mem_size=3221225472, above_4g_mem_size=3221225472, pci_address_space=0x555556535300, ram_memory=0x555556506700) at /home/maritns3/core/xqm/hw/pci-host/i440fx.c:
296
#9  0x0000555555913b3d in pc_init1 (machine=0x55555659a000, pci_type=0x555555d75e48 "i440FX", host_type=0x555555d74f1b "i440FX-pcihost") at /home/maritns3/core/xqm/hw/
i386/pc_piix.c:196
#10 0x0000555555a2c8d3 in machine_run_board_init (machine=0x55555659a000) at /home/maritns3/core/xqm/hw/core/machine.c:1143
#11 0x000055555582b0b8 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/xqm/vl.c:4348
```

原来注册到 DeviceClass::realize 的并不是在 property_set_bool 中直接调用的，而是在调用 device_set_realized 中调用的
device_set_realized 除了调用 DeviceClass::realize 的这个 hook 之外，还会调用
- 处理 hotplug
- 将在这个设备上的所有的 child bus 全部 realize

所以，如果将 qdev_create 和 qdev_init_nofail 放到一起初始化，那么会导致 pci_bus_realize 没有被调用
最终导致 pci 设备的 mmio 空间没有被注册。

## misc
- 注意区分 QObject 和 Object，前者是放到 QList 之类 visitor 数据类型中的
- 通过 InterfaceClass QEMU 可以模拟 interface，具体例子参考 HotplugHandlerClass

[^1]: https://www.linux-kvm.org/images/9/90/Kvmforum14-qom.pdf
[^2]: https://wiki.qemu.org/Features/QAPI
