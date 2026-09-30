# QEMU 中的面向对象 : QOM

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

## 基础

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

> [!WARNING]
>  到此你花费了 2% 的时间掌握了 80% 的 QOM 的内容，接下来是具体的代码分析部分了。

## 案例学习
例如，相同的参数可以添加多次，相当于初始化多个 object 了:
	-object memory-backend-memfd,id=mem0,size=8G,prealloc=off,share=on,hugetlb=false \

## 一些基本观察

参考:
- code/qemu/alpine-action.sh:qmp_qom
- code/qemu/alpine-action.sh:hmp_qom


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
register_module_init 会让 x86_cpu_register_types 这个函数挂载到 `init_type_list[MODULE_INIT_QOM]` 这个链表上。

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

这些初始化只会发生一次，而且 instance_init 则是创建每个新的 object 的时候都是需要
调用，例如设备热插的时候

```txt
- main
  - qemu_init
    - select_machine
      - object_class_get_list
        - object_class_foreach
          - g_hash_table_foreach
            - object_class_foreach_tramp
              - type_initialize
                - type_initialize
                  - x86_cpu_common_class_init
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
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_init_board
        - machine_run_board_init
          - pc_init_v6_1
            - pc_init1
              - x86_cpus_init
                - x86_cpu_new
                  - object_new
                    - object_new_with_type
                      - object_initialize_with_type
                        - object_init_with_type
                          - object_init_with_type
                            - object_init_with_type
                              - x86_cpu_initfn
```

## interface
-object memory-backend-memfd 这种 UserCreatableClass 就是 interface 的典型使用

```c
static const TypeInfo event_loop_base_info = {
    .name = TYPE_EVENT_LOOP_BASE,
    .parent = TYPE_OBJECT,
    .instance_size = sizeof(EventLoopBase),
    .instance_init = event_loop_base_instance_init,
    .class_size = sizeof(EventLoopBaseClass),
    .class_init = event_loop_base_class_init,
    .abstract = true,
    .interfaces = (const InterfaceInfo[]) {
        { TYPE_USER_CREATABLE },
        { }
    }
};
```

```c
/**
 * UserCreatableClass:
 * @parent_class: the base class
 * @complete: callback to be called after @obj's properties are set.
 * @can_be_deleted: callback to be called before an object is removed
 * to check if @obj can be removed safely.
 *
 * Interface is designed to work with -object/object-add/object_add
 * commands.
 * Interface is mandatory for objects that are designed to be user
 * creatable (i.e. -object/object-add/object_add, will accept only
 * objects that inherit this interface).
 *
 * Interface also provides an optional ability to do the second
 * stage * initialization of the object after its properties were
 * set.
 *
 * For objects created without using -object/object-add/object_add,
 * @user_creatable_complete() wrapper should be called manually if
 * object's type implements USER_CREATABLE interface and needs
 * complete() callback to be called.
 */
struct UserCreatableClass {
    /* <private> */
    InterfaceClass parent_class;

    /* <public> */
    void (*complete)(UserCreatable *uc, Error **errp);
    bool (*can_be_deleted)(UserCreatable *uc);
};
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


## QOM 的经典案例

QEMU 将很多内容按照 QOM 重写了之后，如果不掌握 QOM 的基本知识，有些内容是完全看不懂的，现在我举几个经典例子:

### CPU
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
```txt
#0  x86_cpu_set_bit_prop (obj=0x555555e64ac8 <object_property_find_err+43>, v=0x7fffffffd0a0, name=0x55555689ee30 "\220\356\211VUU", opaque=0x555556963e80, errp=0x555556c32070) at ../target/i386/cpu.c:4001
#1  0x0000555555e64f5a in object_property_set (obj=0x555556c32070, name=0x55555608fef1 "kvmclock", v=0x555556b55070, errp=0x5555567a1ee8 <error_abort>) at ../qom/object.c:1402
#2  0x0000555555e65b0f in object_property_parse (obj=0x555556c32070, name=0x55555608fef1 "kvmclock", string=0x55555608fefa "on", errp=0x5555567a1ee8 <error_abort>) at ../qom/object.c:1642
#3  0x0000555555ba00f3 in x86_cpu_apply_props (cpu=0x555556c32070, props=0x5555566c7e60 <kvm_default_props>) at ../target/i386/cpu.c:2638
#4  0x0000555555b3df02 in kvm_cpu_instance_init (cs=0x555556c32070) at ../target/i386/kvm/kvm-cpu.c:126
#5  0x0000555555c82967 in accel_cpu_instance_init (cpu=0x555556c32070) at ../accel/accel-common.c:110
#6  0x0000555555ba3ffa in x86_cpu_initfn (obj=0x555556c32070) at ../target/i386/cpu.c:4131
```

tcg
```txt
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


## misc
- 注意区分 QObject 和 Object，前者是放到 QList 之类 visitor 数据类型中的

## 理解下这个行为，
```c
const PropertyInfo qdev_prop_pci_host_devaddr = {
    .name = "str",
    .description = "Address (bus/device/function) of "
                   "the host device, example: 04:10.0",
    .get = get_pci_host_devaddr,
    .set = set_pci_host_devaddr,
};
```
```txt
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qmp_x_exit_preconfig
        - qemu_create_cli_devices
          - qemu_opts_foreach
            - device_init_func
              - qdev_device_add
                - qdev_device_add_from_qdict
                  - object_set_properties_from_keyval
                    - object_set_properties_from_qdict
                      - object_set_properties_from_qdict
                        - object_property_set
                          - set_pci_host_devaddr
```
如何保证这个 property 的初始化一定早于 vfio_realize

## docs/qemu/qom/info-qom-tree.txt 中的理解

PIIX3 和 i440FX-pcihost 的关系体现的合理
```txt
  /i440fx (i440FX-pcihost)
    /device[7] (PIIX3)
```

挑一个简单的:
```txt
    /virt-blk1 (virtio-blk-pci)
      /virtio-backend (virtio-blk-device)
      /virtio-bus (virtio-pci-bus)
      / 那些 memory-region 就不看了
```
1. /virt-blk1 下面为什么有 virtio-backend
  - virtio_blk_pci_instance_init
    - virtio_instance_init_common
      - object_initialize_child_with_props(proxy_obj, "virtio-backend", vdev,
                                       vdev_size, vdev_name, &error_abort,
                                       NULL);
还是由于这个原因:
```c
struct VirtIOBlkPCI {
    VirtIOPCIProxy parent_obj;
    VirtIOBlock vdev;
};
```

2.  /virt-blk1 的下面为什么有 virtio-bus

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_create_cli_devices
        - qemu_opts_foreach
          - device_init_func
            - qdev_device_add
              - qdev_device_add_from_qdict
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - pci_qdev_realize
                              - virtio_pci_realize
                                - virtio_pci_bus_new
                                  - qbus_init
                                    - qbus_init_internal
                                      - object_property_add_child


### 为什么 irq 要叫做 non-qdev-gpio

## -object
```txt
🧀  qemu-system-x86_64 -object help
List of user creatable objects:
```

也就是那些实现 USER_CREATABLE 的 object

开机的时候:
```txt
- main
  - qemu_init
    - qemu_create_early_backends
      - object_option_foreach_add
        - user_creatable_add_qapi
          - user_creatable_add_type
            - user_creatable_complete
              - event_loop_base_complete
```

如果是热添加的时候:
```txt
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
                            - handle_hmp_command_exec
                              - hmp_object_add
                                - user_creatable_add_from_str
                                  - user_creatable_add_qapi
                                    - user_creatable_add_type
                                      - user_creatable_complete
                                        - event_loop_base_complete
```

## containers

```c
static void qemu_create_machine_containers(Object *machine)
{
    static const char *const containers[] = {
        "unattached",
        "peripheral",
        "peripheral-anon",
    };

    for (unsigned i = 0; i < ARRAY_SIZE(containers); i++) {
        object_property_add_new_container(machine, containers[i]);
    }
}
```

观察 info qom-tree 发现这两个 container
```txt
  /peripheral (container)
```
和
```txt
  /peripheral-anon (container)
```

很容易可以观察到， 如果给 -device virtio-gpu-pci,id=gpu ，那么
就是 /peripheral (container) 下，如果 -device virtio-gpu-pci ，
那么就是在 /peripheral-anon (container) ，就是有无名字的差别。


各种 object 都是如何挂到上面的，以 kvmvapic 为例:
```txt
  /unattached (container)
    /device[1] (kvmvapic)
      /kvmvapic-rom[0] (memory-region)
      /kvmvapic[0] (memory-region)
```

只能找到 `/kvmvapic[0] (memory-region)` 挂到 `/device[1] (kvmvapic)` ，但是
`/device[1] (kvmvapic)` 如何挂到 /unattached (container) 有点麻烦

- vapic_realize
  - memory_region_init_io
    - memory_region_init
      - memory_region_do_init
        - object_property_add_child

但是 container 的作用是显然的，就是一个结构体，之后容易遍历:
例如 foreach_dynamic_sysbus_device


真的可以动态修改属性 hmp "qom-set /objects/mem0 seal false"

- handle_hmp_command_exec
  - handle_hmp_command_exec
    - hmp_qom_set
      - object_property_parse
        - object_property_set
          - property_set_bool
            - memfd_backend_set_seal

## TODO

sugar property 是什么东西?
```txt

        /*
         * Virtio devices can't count on directly accessing guest
         * memory, so they need iommu_platform=on to use normal DMA
         * mechanisms.  That requires also disabling legacy virtio
         * support for those virtio pci devices which allow it.
         */
        object_register_sugar_prop(TYPE_VIRTIO_PCI, "disable-legacy",
                                   "on", true);
        object_register_sugar_prop(TYPE_VIRTIO_DEVICE, "iommu_platform",
                                   "on", false);

```

总体来说，qemu 最基本的核心 qmp hmp cmdline qobj

最有趣的是 qemu 也是 bus + device ，可以和 kernel 的 sysfs 来对比

https://xz.aliyun.com/t/8320

[^2]: https://wiki.qemu.org/Features/QAPI

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
