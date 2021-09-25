# QEMU 中的面向对象 : QOM

因为 QEMU 整个项目是 C 语言写的，但是 QEMU 处理的对象例如主板，CPU, 总线，外设实际上存在很多继承的关系。
所以，QEMU 为了方便整个系统的构建，实现了自己的一套的面向对象机制，也就是 QEMU Object Model（下面称为 QOM）。

首先，回忆一下面向对象的基本知识:
- 继承
- 静态成员
- 构造函数和析构函数
- 多态
	- 动态绑定(override)
	- 静态绑定(overload)
- 抽象类(虚基类)
- 强制类型装换

好的，下面我们将会分析 QEMU 是如何实现这些特性，以及 QEMU 扩展的高级特性。

- [ ] 画个图描述一下这个事情
- 一个 TypeImpl::class 和自己 parent TypeImpl 关联的 ObjectClass 是同一个
  - 不是的，
  - 一个 TypeImpl 和其 parent TypeImpl 初始化的对象持有的 ObjectClass 现在是部分
  - 换言之，不同的类持有静态部分必然存储在两个位置


## 基本

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
QEMU 定义了一些列的 macro 来封装，我将这些 macro 列举到[这里](./qemu/qom-macro.c)了。
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

在分析

- ObjectClass : The base for all classes
- Object : 持有一个指针 ObjectClass, 而 Object 持有一个 struct TypeImpl * ，所以可以动态的查找到一个 object 真正类型

- object_dynamic_cast_assert : 将一个 object cast 成为类型，且参数为 "machine"
  - 如果没有进行 CONFIG_QOM_CAST_DEBUG, 那么什么都不需要做，因为这些类型都是嵌套到一起的
  - 首先扫描一下 object_cast_cache 中是否以前从这个 object cast 到过参数的类型，如果之前正确，那么现在肯定正确
  - object_dynamic_cast
    - object_class_dynamic_cast : 简单来说, 通过 type_get_by_name 找到 TypeImpl, 然后通过 type_is_ancestor 就可以判断了
  - 装换成功，设置 object_cast_cache
- object_class_dynamic_cast_assert : 就会 cache 机制和调用一下 object_class_dynamic_cast

object_dynamic_cast_assert 真正恐怖的地方在于，现在所有的对象都是都是可以装换为 Object 类型，
而一个 object 类型的变量，实际上，可以在完全缺乏上下文的环境中 cast 到可以 cast 的任何类型。
而这个关键在于，Object 中通过 ObjectClass 知道自己的真正的类型。


- 总结一下几个转换函数:
	- object_dynamic_cast_assert
	- object_get_class
	- [x] object_class_dynamic_cast_assert : 什么都不需要做的
		- 但是，object_class_get_parent 为什么获取的就完全不是一个 class
		- object_class_by_name
