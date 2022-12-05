# QEMU 启动代码
- [ ] ../qemu/init.md 一并整理过来

- Machine 层次相关
    - [x] hw/i386/x86.c
        - `x86_machine_info` : 注册 `x86_machine_initfn` 和 `86_machine_class_init`
        - `x86_cpu_new`
    - [x] /hw/i386/pc.c
        - 注册了 TypeInfo, 注册 `pc_machine_info` 和 `pc_machine_class_init`
    - [x] `hw/i386/pc_piix.c`
        - `pc_init1`
- CPU 层次相关
  - [x] hw/core/cpu.c
    - [x] target/i386/cpu.c
            - `x86_cpu_realizefn`

## 初始化 QEMU 大约需要处理的事情
- [ ] `rtc_set_cpus_count` 中和 seabios 的关系需要完全走通
- [ ] 如何降低 include/sysemu/numa.h 的影响
  - [ ] CPUState 中的 cpu_index, cluster_index 等
- [ ] 分析一下 include/hw/i386/topology.h 的实现
- [ ] PCMachineClass 中为什么需要将 smm
- [ ] `tcg_init_ctx` 应该是自动被初始化好了，但是 review tcg.c 中相关的代码吧

## 几个关键的结构体功能和移植差异说明

| struct      | explaination                                                                            |
|-------------|-----------------------------------------------------------------------------------------|
| CPUClass    | 在函数 x86_cpu_common_class_init 中已经知道注册的函数, 可以将其直接定义为一个静态函数集 |
| CPUState    |                                                                                         |
| CPUX86State | 和 CPUState 没有父子关系，而是靠 CPUState::env_ptr 决定的                               |
| X86CPU      |                                                                                         |
| TBContext   | 一个统计，一个 qht, 似乎只是定义了一个全局变量                                          |
| TCGContext  | TODO 每一个 thread 定义了一个，同时存在一个全局的 tcg_init_ctx                          |

## 问题
- [ ] qdev_device_add 是做什么的
```plain
huxueshi:qdev_device_add isa-debugcon
huxueshi:qdev_device_add nvme
huxueshi:qdev_device_add virtio-9p-pci
```

- 来理解一下 do_cpu_init 中的逻辑:

其实整个 CPUX86State 是被划分为三个部分的:
```c
struct {} start_init_save;
struct {} end_init_save;
struct {} end_reset_fields;
```
在调用 cpu_reset 的时候，end_reset_fields 上面的字段会被全部初始化为 0 的。
start_init_save 和 end_reset_fields 根据 intel 手册，这些需要被避免，所以特定做了一个保存。


## 整体
启动，总体划分三个部分：
1. x86_cpus_init ：进行 CPU 相关的初始化
2. pc_memory_init : 进行内存初始化
3. 设备的初始化
  - pci 相关的组织比较麻烦
4. machine 的初始化
   - 主要分布于 pc.c, 在 pc_piix.c 中只是定义了 pc_init1 而已

- CPUX86State : 这是 X86CPU 的成员，不是指针哦

文件内容的基本分析:
| file              | 行数 | 内容分析                                               |
|-------------------|------|--------------------------------------------------------|
| hw/i386/x86.c     | 1300 | cpu_hotplug / pic / x86_machine_class_init             |
| hw/i386/pc.c      | 1700 | 处理 Machine 相关的初始化，例如 hpet, vga 之类的       |
| target/i386/cpu.c | 7000 | X86CPU 相关，主要处理的都是 PC 的 feature 之类的       |
| hw/i386/pi_piix.c | 1000 | pc_init1 剩下的就是 DEFINE_I440FX_MACHINE 定义的东西了 |

定义的各种 type info
| variable          | location             | summary                                                              | instance_init                                                                     | class_init                                                                     |
|-------------------|----------------------|----------------------------------------------------------------------|-----------------------------------------------------------------------------------|--------------------------------------------------------------------------------|
| pc_machine_info   | hw/i386/pc.c         | pc_machine_initfn : 初始化一下 PCMachineState, pc_machine_class_init | 初始化了好多成员                                                                  |                                                                                |
| x86_machine_info  | hw/i386/x86.c        | 没啥东西                                                             |                                                                                   |                                                                                |
| machine_info      | hw/core/machine.c    |                                                                      | 没啥东西                                                                          | 注册 kernel initrd 之类的 property                                             |
| x86_cpu_type_info | target/i386/pc.c     |                                                                      | 调用一些 object_property_add_alias, x86_cpu_load_model 和 accel_cpu_instance_init | 注册了 x86_cpu_realizefn , 一些函数指针的初始化, vendor 之类的 property 初始化 |
| cpu_type_info     | hw/core/cpu-common.c | 并没有什么                                                           | 成员初始化，尤其是，list 之类的                                                   | 注册 cpu_common_parse_features 之类的                                          |
| device_type_info  | hw/core/qdev.c       | 处理 hotplugged 之类，这些抽象没有必要                               |                                                                                   |                                                                                |


到底初始化什么内容:
| item | necessary          | desc                                                                                     |
|------|--------------------|------------------------------------------------------------------------------------------|
| e820 | :heavy_check_mark: | - [ ] 为什么有了 acpi 还是需要 e820 啊，当使用增加了一个内存条，并没有说非要修改 acpi 啊 |
| apci | :x:                | - [ ] 在 QEMU 和 kernel 中间都存在 CONFIG_ACPI 的选项，也许暂时可以不去管                |
| pci  | :x:                | - [ ] `pcmc->pci_enabled`                                                                |
| cpu  | :x:                | - [ ] 到底初始化的是那几个结构体，和 tcg 耦合的结构体是谁                                |

非 PCI 设备枚举:
| Device       | parent              |
|--------------|---------------------|
| mc146818 rtc | TYPE_ISA_DEVICE     |
| i8254 pit    | TYPE_ISA_DEVICE     |
| i8257 dma    | TYPE_ISA_DEVICE     |
| hpet         | TYPE_SYS_BUS_DEVICE |
| i8259        | TYPE_ISA_DEVICE     |

从 type info 上可以轻易的看到一个设备是不是 TYPE_ISA_DEVICE


- qemu_register_reset(x86_cpu_machine_reset_cb, cpu);
  - pc_machine_reset
    - qemu_devices_reset
      - 调用那些 qemu_register_reset 注册的 hook，其中包括 x86_cpu_machine_reset_cb
        - cpu_reset
    - APICCommonClass::reset
- qemu_run_machine_init_done_notifiers

## e820
- 信息是如何构造出来的
  - 在  pc_memory_init 调用两次 e820_add_entry, 分别添加 below_4g_mem_size 和 above_4g_mem_size
- 如何通知 guest 内核的 ?
  - 在 fw_cfg_arch_create 中添加 `etc/e820` 实现的

- [ ] 类似 pci 映射的 MMIO 空间的分配是 e820 负责的操作的吗 ?

## choose Machine
1. `select_machine`

从注册的里面进行选择一个 default, 也可以从参数中靠 machine_parse 解析出来
```txt
// 部分省略了
pc-q35-2.11
pc-i440fx-3.0
pc-q35-2.5
pc-i440fx-2.8
pc-i440fx-5.0
pc-i440fx-4.0
pc-i440fx-2.3
microvm
xenfv-4.2
isapc
x-remote
none
xenpv
```
2. 注册 MachineClass::init，这个将会在 machine_run_board_init 中调用

举个例子，将 Machine 展开
```c
DEFINE_I440FX_MACHINE(v0_12, "pc-0.12", pc_compat_0_13, pc_i440fx_0_12_machine_options);
```

```c
static void pc_init_v4_2(MachineState *machine) {
  void (*compat)(MachineState * m) = (NULL);
  if (compat) {
    compat(machine);
  }
  pc_init1(machine, TYPE_I440FX_PCI_HOST_BRIDGE, TYPE_I440FX_PCI_DEVICE);
}
static void pc_machine_v4_2_class_init(ObjectClass *oc, void *data) {
  MachineClass *mc = MACHINE_CLASS(oc);
  pc_i440fx_4_2_machine_options(mc);
  mc->init = pc_init_v4_2;
}
static const TypeInfo pc_machine_type_v4_2 = {
    .name = "pc-i440fx-4.2"
            "-machine",
    .parent = TYPE_PC_MACHINE,
    .class_init = pc_machine_v4_2_class_init,
};
static void pc_machine_init_v4_2(void) { type_register(&pc_machine_type_v4_2); }
type_init(pc_machine_init_v4_2);
```

3. machine 的继承关系
    - pc_piix.c : 具体的主板号
    - pc.c : pc 类型
    - x86.c : x86 的机器
    - machine.c : 根类型

```c
static const TypeInfo pc_machine_info = {
    .name = TYPE_PC_MACHINE,
    .parent = TYPE_X86_MACHINE,
    .abstract = true,
    .instance_size = sizeof(PCMachineState),
    .instance_init = pc_machine_initfn,
    .class_size = sizeof(PCMachineClass),
    .class_init = pc_machine_class_init,
    .interfaces = (InterfaceInfo[]) {
         { TYPE_HOTPLUG_HANDLER },
         { }
    },
};
```

```c
static const TypeInfo x86_machine_info = {
    .name = TYPE_X86_MACHINE,
    .parent = TYPE_MACHINE,
    .abstract = true,
    .instance_size = sizeof(X86MachineState),
    .instance_init = x86_machine_initfn,
    .class_size = sizeof(X86MachineClass),
    .class_init = x86_machine_class_init,
    .interfaces = (InterfaceInfo[]) {
         { TYPE_NMI },
         { }
    },
};
```

## BUS
- pci host bridge 和 pcibus 的关系?

```plain
#0  qbus_init (bus=0x55555608760c, parent=0x7fffffffd510, name=0x555555e65068 <object_initialize+99> "\220\311\303\363\017\036\372UH\211\345H\201\354\020\001") at ../hw
/core/bus.c:103
#1  0x0000555555e78fdb in qbus_create_inplace (bus=0x555556aacb60, size=120, typename=0x55555608760c "System", parent=0x0, name=0x5555560876dc "main-system-bus") at ../
hw/core/bus.c:158
#2  0x0000555555b03423 in main_system_bus_create () at ../hw/core/sysbus.c:346
#3  0x0000555555b03451 in sysbus_get_default () at ../hw/core/sysbus.c:354
#4  0x0000555555cd9c11 in qemu_create_machine (machine_class=0x5555569a3850) at ../softmmu/vl.c:2087
#5  0x0000555555cdd500 in qemu_init (argc=28, argv=0x7fffffffd7c8, envp=0x7fffffffd8b0) at ../softmmu/vl.c:3570
#6  0x000055555582e575 in main (argc=28, argv=0x7fffffffd7c8, envp=0x7fffffffd8b0) at ../softmmu/main.c:49
```

## init machine
我们发现，整个 machine 的体系几乎完全没有被拷贝进去，对此，
原因是
1. machine 的初始化只是为了完成 cpu 的初始化，所以，当 cpu 没有初始化的时候，machine 的内容完全看不到了。
2. machine 的工作在于 acpi smbios pci 之类的, 暂时没有处理到

- [ ] 需要分析一下 MachineClass 的内容
```c
X86CPU *cpu = X86_CPU(ms->possible_cpus->cpus[0].cpu);
```

### machine_class_base_init 和 machine_class_init

## init cpu


#### CPUState
| fields         | 初始化的位置                                                                                 |
|----------------|----------------------------------------------------------------------------------------------|
| nr_cores       | cpu_common_initfn 中初始化为 1, 而在 qemu_init_vcpu 中初始化为 ms->smp.cores, 默认初始化为 1 |
| nr_threads     |                                                                                              |
| jmp_env        | 无需初始化, 使用的位置就是哪里                                                               |

```c
    /* TODO Move common fields from CPUArchState here. */
    int cpu_index;
    int cluster_index;
    uint32_t halted;
    uint32_t can_do_io;
    int32_t exception_index;
```
上面的几个变量有点类似，
在 cpu_common_reset 中进行一些简单的初始化，但是实际上这并不是正确的位置。
其实这是一个废话，都是 CPUState 在这里初始化的
这些变量之所以再次会在其他的位置初始化，是因为 X86CPU 的进一步初始化


- cpu_index : 赋值位置
  - cpu_list_add / cpu_list_remove
  - cpu_common_initfn
  - x86_cpu_pre_plug :
    - 这是实际上初始化的位置，实际上，这个 idx 获取似乎有点麻烦，但是实际上，并没有必要
- cluster_index
- halted
- can_do_io
- exception_index


#### X86CPU

| fields                        | 初始化的位置                                                   |
|-------------------------------|----------------------------------------------------------------|
| neg                           | tlb_init                                                       |
| env                           | x86_cpu_reset                                                  |
| apic_state                    | x86_cpu_realizefn => x86_cpu_apic_create                       |
| apic_id                       | x86_cpu_new 使用 CPUArchId::arch_id 初始化                     |
| expose_tcg                    | 应该是可以直接去掉, 默认是打开的，*但是不知道打开的效果是什么* |
| phys_bits                     | x86_cpu_realizefn                                              |
| enable_l3_cache / enable_lmce | 和 expose_tcg 的操作方式类似, 理解了 GlobalProperty 在说吧     |
| singlestep_enabled            | 暂时保证永远不会被启动吧                                       |
| cpu_index / cluster_index     |                                                                |

##### cpu_index / cluster_index
- [ ] cluster_index : 应该是没有被重新初始化过，具体需要使用 xqm 分析一下

为了获取 X86MachineState::apic_id_limit

- x86_cpu_apic_id_from_index
    - init_topo_info
    - x86_apicid_from_cpu_idx

#### CPUClass
几乎所有的成员都是和 vmstate 相关的，所以实际上，

x86_cpu_common_class_init 注册了大量函数
在 cpu_class_init 中也是注册了一些，处理方法很简单，首先找到
x86_cpu_common_class_init 中注册是什么，然后找 cpu_class_init 注册内容:

确定一个决定，**不要删除 CPUClass 这个东西, 将这些函数全部放到一起，这样出入更小，更容易理解**

#### X86CPUClass
- [x] model 如何处理的 : 用于 list 所有可选 cpu 的, 参考 x86_cpu_list
- [x] parent_realize 处理的

在 x86_cpu_common_class_init 中，将通过
```plain
    device_class_set_parent_realize(dc, x86_cpu_realizefn,
                                    &xcc->parent_realize);
```

```c
void device_class_set_parent_realize(DeviceClass *dc,
                                     DeviceRealize dev_realize,
                                     DeviceRealize *parent_realize)
{
    *parent_realize = dc->realize;
    dc->realize = dev_realize;
}
```
同时初始化一下 `parent_realize` 和 realize

所以，最后 `x86_cpu_realizefn` 会调用 `cpu_common_realizefn`
#### CPUX86State
- [ ] smbase : 这个地址似乎用于 smm 保存上下文的地方, 这个东西就是 SMRAM 的基地址

#### features
```c
features [Field] :1546:22                                                                                                                                                                                                                               │
max_features [Field] :1643:10                                                                                                                                                                                                                           │
force_features [Field] :1638:10                                                                                                                                                                                                                         │
hyperv_features [Field] :1627:14                                                                                                                                                                                                                        │
filtered_features [Field] :1663:22                                                                                                                                                                                                                      │

    /* Features that were explicitly enabled/disabled */
    FeatureWordArray user_features;
```
- [ ] 这里面的几个内容暂时也是没有逐个分析的

```c
(qemu) 78bfbfd 2001 0 0 0 0 20100800 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 huxueshi:x86_cpu_load_model
78bfbfd 80002001 0 0 0 0 2193fbfd 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 huxueshi:x86_cpu_realizefn
```

对于 env::features 增加赋值在两个地方:
- `x86_cpu_realizefn`
  - `IS_AMD_CPU`
  - `x86_cpu_load_model`

- [ ] `x86_cpu_filter_features` 中发现有的 feature 需要 `mark_unavailable_features` 那么又如何?

## qdev

这些属性都是在 `instance_init` 的时候初始化的:
```c
  dc->props = apic_properties_common;
```

1. `qdev_create` : `instance_init`
2. `qdev_init_nofail` : realize

# qemu log
log 的功能主要是 log.c 中实现的
- 可以导入到一个文件中间
- dfilter : 只是输出某一个方位的 log 出来
    - `QEMU_OPTION_DFILTER` / `qemu_log_in_addr_range` / `qemu_set_dfilter_ranges` 使用
- `qemu_log_items` : 用于选择到底 log 那些内容，不是划分等级的，而是通过 mask 来确定 log 哪一个部分

- 在各个文件中，还存在一些本地的 DEBUG 选项，那些都是纯粹的手动打开的，比如 `DEBUG_TLB`

[^2]: https://en.wikipedia.org/wiki/Machine-check_exception
