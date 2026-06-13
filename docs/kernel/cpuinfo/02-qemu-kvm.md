# 分析下 QEMU cpu model 的

## qemu 和 kvm 是如何调整 cpuid 的
<!-- 8cf013ee-f1dd-4e91-97b3-09865fdea93e -->

1. 在 kvm 启动的时候，kvm_set_cpu_caps() 会配置 cpuid 表示是 kvm 支持的 cpu flags
2. 在 QEMU 中:
kvm_arch_init_vcpu : 显然是每一个 CPU 都是会初始化一遍的
  - 如果确定是 kvm，那么需要调整 kvm 相关的字段
  - kvm_x86_build_cpuid : 枚举每一个字段 cpu_x86_cpuid ，填充 kvm_cpuid_entry2 数组，其中 eax ebx ecx 和 edx 都是靠 cpu_x86_cpuid 获取的
    - cpu_x86_cpuid : QEMU 对于每一个字段逐个特殊处理，会调用 kvm_arch_get_supported_cpuid 获取原始数据
      - kvm_arch_get_supported_cpuid : 通过 kvm `KVM_GET_SUPPORTED_CPUID` 查询完成之后
  - kvm_vcpu_ioctl(cs, `KVM_SET_CPUID2`, &cpuid_data); 调用 kvm ioctl ，最后再填充到 kvm 中

所以，简而言之，就是首先 kvm 告诉 qemu 支持哪些特性(KVM_GET_SUPPORTED_CPUID)，然后 qemu 叠加上自己执行的特性，
最后填充到 kvm 中(KVM_SET_CPUID2)

之后， cupid 指令一定会导致 vm exit ，然后通过 cpuid_entries 来查找
- kvm_emulate_cpuid
  - kvm_cpuid
    - kvm_find_cpuid_entry_index : 直接在 kvm_vcpu_arch::cpuid_entries 中找结果，这些结果是之前 QEMU 计算得到

补充:
kvm_arch_get_supported_cpuid 的使用位置:
1. target/i386/cpu.c :
  - x86_cpu_get_supported_feature_word
  - x86_cpu_get_supported_cpuid
  - cpu_x86_cpuid
2. target/i386/kvm/kvm-cpu.c : 用于查询 CPU topo 相关的
3. target/i386/kvm/kvm.c : 用于查询该如何保存 msr 之类的，例如 kvm_arch_get_supported_msr_feature

下面是一个具体的例子
### 为什么 -cpu host，有的 feature bit 观察不到?

类似的问题，为什么虚拟机中有的 leaf 观察不到

docs/kernel/cpuinfo/zhaoxin/ 中
39  40  182 是三个物理机  guest.md 是虚拟机中观察中

可以从 guest.md 中看到
```txt
   0xc0000001 0x00: eax=0x000507bb ebx=0x00000000 ecx=0x00000000 edx=0x00003dcc
   0xc0000002 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
   0xc0000003 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
   0xc0000004 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
```

其中的结果和 qemu 中的完全对应:
```c
void cpu_x86_cpuid(CPUX86State *env, uint32_t index, uint32_t count,
                   uint32_t *eax, uint32_t *ebx,
                   uint32_t *ecx, uint32_t *edx){

    case 0xC0000001:
        /* Support for VIA CPU's CPUID instruction */
        *eax = env->cpuid_version;
        *ebx = 0;
        *ecx = 0;
        *edx = env->features[FEAT_C000_0001_EDX];
        break;
}
```

观察 edx 在三个虚拟机中的结果:
0x1ec33dff
0x1ec13dff
0x1ec33dff

在虚拟机中，其结果为:
0x00003dcc

所以，就是只有 0xc0000001 中的 eax 和 edx

所以，问题是虚拟机中 edx 为 0x00003dcc :

看上去，env->features 中的结果受到的影响为:
```c
FeatureWordInfo feature_word_info[FEATURE_WORDS] = {

    [FEAT_C000_0001_EDX] = {
        .type = CPUID_FEATURE_WORD,
        .feat_names = {
            NULL, NULL, "xstore", "xstore-en",
            NULL, NULL, "xcrypt", "xcrypt-en",
            "ace2", "ace2-en", "phe", "phe-en",
            "pmm", "pmm-en", NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
        },
        .cpuid = { .eax = 0xC0000001, .reg = R_EDX, },
        .tcg_features = TCG_EXT4_FEATURES,
    },
```

在 kvm 中:
kvm_set_cpu_caps 就是最终的结果:
```c
	kvm_cpu_cap_init(CPUID_C000_0001_EDX,
		F(XSTORE),
		F(XSTORE_EN),
		F(XCRYPT),
		F(XCRYPT_EN),
		F(ACE2),
		F(ACE2_EN),
		F(PHE),
		F(PHE_EN),
		F(PMM),
		F(PMM_EN),
	);
```


使用资源为: docs/kernel/cpuinfo/intel/

也就是
```c
FeatureWordInfo feature_word_info[FEATURE_WORDS] = {
    // ...
    [FEAT_6_EAX] = {
        .type = CPUID_FEATURE_WORD,
        .feat_names = {
            NULL, NULL, "arat", NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
        },
        .cpuid = { .eax = 6, .reg = R_EAX, },
        .tcg_features = TCG_6_EAX_FEATURES,
    },
```

在物理机中结果为:
   0x00000006 0x00: eax=0x00000077 ebx=0x00000002 ecx=0x00000009 edx=0x00000000

虚拟机中结果为
   0x00000006 0x00: eax=0x00000004 ebx=0x00000000 ecx=0x00000000 edx=0x00000000

这个是在 kvm 中被 drop 掉的
### env->features 是如何被初始化的

FEAT_6_EAX

```c
typedef struct CPUArchState {
    FeatureWordArray features;

    /* Features that were explicitly enabled/disabled */
    FeatureWordArray user_features;
```

- x86_cpu_realizefn
  - x86_cpu_expand_features : 在进入这个函数的时候， CPUArchState::features 还没初始化的
其中回去调用 kvm_arch_get_supported_cpuid 来，这个会去调用 ioctl ，最后走到 __do_cpuid_func 中来
  - x86_cpu_filter_features : 用户态控制导致的


## [ ] 似乎还是没有搞清楚，从 kvm 到 qemu ，然后 qemu 到 kvm 的过程

kvm 应该就是一个预定义的数组就可以了

## \-cpu host 和 \-cpu max 的区别是什么?
<!-- 05d8b762-b746-4358-8ead-8c66424b9611 -->


从 qemu-system-x86_64 -cpu help 看，这个完全是一个废话:
```txt
  host                  processor with all supported host features
  max                   Enables all features supported by the accelerator in the current host
```

首先，搞懂他们直接的依赖关系是什么:
```c
static const TypeInfo x86_base_cpu_type_info = {
        .name = X86_CPU_TYPE_NAME("base"),
        .parent = TYPE_X86_CPU,
        .class_init = x86_cpu_base_class_init,
};

static void x86_register_cpu_model_type(const char *name, X86CPUModel *model)
{
    g_autofree char *typename = x86_cpu_type_name(name);
    TypeInfo ti = {
        .name = typename,
        .parent = TYPE_X86_CPU,
        .class_init = x86_cpu_cpudef_class_init,
        .class_data = model,
    };

    type_register_static(&ti);
}

static const TypeInfo max_x86_cpu_type_info = {
    .name = X86_CPU_TYPE_NAME("max"),
    .parent = TYPE_X86_CPU,
    .instance_init = max_x86_cpu_initfn,
    .class_init = max_x86_cpu_class_init,
};
```

也就是 host 是 max 更加细分的:
```c
static const TypeInfo host_cpu_type_info = {
    .name = X86_CPU_TYPE_NAME("host"),
    .parent = X86_CPU_TYPE_NAME("max"),
    .class_init = host_cpu_class_init,
};
```


所以，-cpu host 和 -cpu max 的区别在于 target/i386/host-cpu.c
中提供了几个函数给 kvm 使用，而不是真的添加了什么:

所以，我认为如下判断是正确的，-cpu max 的关键在于

此外，使用资源为: docs/kernel/cpuinfo/intel/ ，可以发现 kvm 模式下，-cpu max 和 -cpu host 完全相同

> [!NOTE]
> 参考神奇海螺的意见，有待验证

在 QEMU/KVM 虚拟化中，`-cpu host` 和 `-cpu max` 是两种常见的 CPU 模型配置方式。虽然它们在硬件加速模式下表现相似，但在设计逻辑和不同运行模式（KVM vs TCG）下有显著区别。

| 特性           | `-cpu host`                                                      | `-cpu max`                                                                      |
| ---            | ---                                                              | ---                                                                             |
| **逻辑定义**   | **物理 CPU 复制**：虚拟机看到的 CPU 型号和特性与物理机完全一致。 | **加速器能力上限**：开启当前加速器（如 KVM 或 TCG）能支持的所有特性。           |
| **KVM 模式**   | 与物理机一致。通常等同于 `max`。                                 | 开启 KVM 支持的所有宿主机特性，通常等同于 `host`。                              |
| **TCG 模式**   | **不可用**（或无意义），因为 TCG 是软件模拟。                    | **可用**。模拟出该版本 QEMU 能够支持的所有 CPU 指令集（即使宿主机硬件不支持）。 |
| **跨架构模拟** | 不支持。                                                         | **支持**。例如在 x86 宿主机上模拟最强的 ARM CPU。                               |


## 基本逻辑

### 数据结构
如果没有理解错，这里的 function 和 index 就是 leaf 和 subleaf 了:
```c
struct kvm_cpuid_entry2 {
	__u32 function;
	__u32 index;
	__u32 flags;
	__u32 eax;
	__u32 ebx;
	__u32 ecx;
	__u32 edx;
	__u32 padding[3];
};
```

### 基本流程

#### QEMU 内部初始化
- kvm_arch_init_vcpu 中
```c
    struct {
        struct kvm_cpuid2 cpuid;
        struct kvm_cpuid_entry2 entries[KVM_MAX_CPUID_ENTRIES];
    } cpuid_data;
```
处理各种虚拟化相关的场景

**kvm_x86_build_cpuid 是关键**，这里解释了为什么即便是 -cpu host ，在 guest 中也未必
看到的是和 host 一模一样的东西。

#### QEMU 给 kvm 设置 cpuid
KVM_SET_CPUID2 : 就是会修改 Guest 使用 cpuid 获取到能力。

- kvm_vcpu_ioctl_set_cpuid2
  - kvm_set_cpuid
    - `__kvm_update_cpuid_runtime`
    - kvm_vcpu_after_set_cpuid
      - vmx_vcpu_after_set_cpuid
        - vmx_update_exception_bitmap : 其实这个也不对啊

内容存放到这里:
```c
struct kvm_cpuid_entry2 *kvm_vcpu_arch::cpuid_entries
```

## 我说为什么总是搞不清楚 leaf 和 subleaf 是什么

cpu_x86_cpuid -> x86_cpu_get_cache_cpuid 中调用
```txt
            x86_cpu_get_cache_cpuid(index, 0, eax, ebx, ecx, edx);
```

而的参数就变成了:
```c
static void x86_cpu_get_cache_cpuid(uint32_t func, uint32_t index,
                                    uint32_t *eax, uint32_t *ebx,
                                    uint32_t *ecx, uint32_t *edx)
```

leaf 变成了 func ，subleaf 变成了 index ，真的很崩溃


## 有趣的现象
这是启动之后的结果，在虚拟机完全安静的时候，当然是不会去执行 cpuid 的，
但是如果在 ssh 中按一个字符，就是会有 cpuid 的执行的。

可以找到到底是什么程序导致的。
```txt
@[
    kvm_emulate_cpuid+5
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 46766

@[
    emulator_get_cpuid+5
    vendor_intel+77
    x86_emulate_insn+1495
    x86_emulate_instruction+824
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 3
```

- [ ] 为什么会存在这两个 backtrace ？
- [ ] guest 为什么需要如此频繁的被调用，
- [ ] 存在 `em_cpuid` ，为什么却无人调用，什么时候才会去使用

敲一个 enter ，观察tracepoint 中的结果就可以看到如下内容:
```txt
100577.392 CPU 2/KVM/1605119 kvm:kvm_cpuid(rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100577.403 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 1, index: 1981343352, rax: 722545, rbx: 35653632, rcx: 4294586915, rdx: 529267711, found: 1)
100577.409 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 7, rax: 2, rbx: 563881963, rcx: 406849340, rdx: 3154134032, found: 1)
100577.414 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 7, index: 1, rax: 2064, found: 1)
100577.419 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 1, rax: 15, rbx: 840, found: 1)
100577.424 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 20, found: 1)
100577.429 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 25)
100577.433 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2147483648, rax: 2147483656, found: 1)
100577.438 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2147483649, rax: 722545, rcx: 289, rdx: 739248128, found: 1)
100577.443 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2147483655, index: 289)
100577.447 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2147483656, rax: 3026990, rbx: 16830464, found: 1)
100577.453 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, rax: 519, rbx: 2696, rcx: 2696, found: 1)
100577.457 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 2, rax: 256, rbx: 576, found: 1)
100577.462 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 3)
100577.467 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 5)
100577.471 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 6)
100577.476 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 7)
100577.481 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 17)
100577.486 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 18)
100577.491 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 13, index: 19)
100577.498 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, index: 529236223, rax: 1, rdx: 255, found: 1)
100577.503 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.507 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.512 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.516 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.521 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100577.525 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100577.530 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.534 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.539 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.544 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.548 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.553 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.557 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.562 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.567 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.571 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.576 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.580 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.585 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.589 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100577.594 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.598 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.603 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.608 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100577.612 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.617 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.628 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.633 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100577.638 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.642 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.647 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.652 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100577.656 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100577.661 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.666 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.670 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.675 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100577.679 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100577.684 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100577.689 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.693 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.698 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100577.703 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100577.708 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 4, found: 1)
100577.713 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.717 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100577.722 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100577.726 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100577.731 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 11, rbx: 1, rcx: 256, rdx: 2, found: 1)
100577.736 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 11, index: 1, rax: 5, rbx: 32, rcx: 513, rdx: 2, found: 1)
100579.107 CPU 2/KVM/1605119 kvm:kvm_cpuid(rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100579.121 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 1, index: 1818588270, rax: 722545, rbx: 35653632, rcx: 4294586915, rdx: 529267711, found: 1)
100580.763 CPU 2/KVM/1605119 kvm:kvm_cpuid(rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100580.770 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2147483648, rax: 2147483656, found: 1)
100580.776 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 1, rax: 722545, rbx: 35653632, rcx: 4294586915, rdx: 529267711, found: 1)
100580.782 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 7, rax: 2, rbx: 563881963, rcx: 406849340, rdx: 3154134032, found: 1)
100580.789 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2147483648, rax: 2147483656, found: 1)
100580.795 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 2147483649, rax: 722545, rcx: 289, rdx: 739248128, found: 1)
100582.959 CPU 2/KVM/1605119 kvm:kvm_cpuid(rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100582.962 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 1, rax: 722545, rbx: 35653632, rcx: 4294586915, rdx: 529267711, found: 1)
100582.963 CPU 2/KVM/1605119 kvm:kvm_cpuid(function: 7, rax: 2, rbx: 563881963, rcx: 406849340, rdx: 3154134032, found: 1)
100602.574 CPU 7/KVM/1605125 kvm:kvm_cpuid(rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100602.576 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 1, index: 1657135848, rax: 722545, rbx: 119539712, rcx: 4294586915, rdx: 529267711, found: 1)
100602.577 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 7, rax: 2, rbx: 563881963, rcx: 406849340, rdx: 3154134032, found: 1)
100602.578 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 7, index: 1, rax: 2064, found: 1)
100602.579 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 1, rax: 15, rbx: 840, found: 1)
100602.580 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 20, found: 1)
100602.581 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 25)
100602.582 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2147483648, rax: 2147483656, found: 1)
100602.583 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2147483649, rax: 722545, rcx: 289, rdx: 739248128, found: 1)
100602.584 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2147483655, index: 289)
100602.585 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2147483656, rax: 3026990, rbx: 16830464, found: 1)
100602.586 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, rax: 519, rbx: 2696, rcx: 2696, found: 1)
100602.587 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 2, rax: 256, rbx: 576, found: 1)
100602.588 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 3)
100602.589 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 5)
100602.590 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 6)
100602.591 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 7)
100602.593 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 17)
100602.594 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 18)
100602.595 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 19)
100602.596 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, index: 529236223, rax: 1, rdx: 255, found: 1)
100602.597 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.600 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.601 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.602 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.603 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100602.604 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100602.605 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.606 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.606 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.607 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.608 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.609 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.610 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.611 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.612 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.613 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.614 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.615 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.616 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.617 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100602.618 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.619 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.620 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.621 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100602.622 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.623 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.624 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.625 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100602.627 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.628 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.629 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.630 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100602.630 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100602.631 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.632 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.633 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.634 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100602.635 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100602.636 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100602.637 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.638 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.639 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100602.640 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100602.641 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 4, found: 1)
100602.642 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.643 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100602.644 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100602.645 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100602.646 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 11, rbx: 1, rcx: 256, rdx: 7, found: 1)
100602.647 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 11, index: 1, rax: 5, rbx: 32, rcx: 513, rdx: 7, found: 1)
100603.119 CPU 7/KVM/1605125 kvm:kvm_cpuid(index: 3281710096, rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100603.120 CPU 7/KVM/1605125 kvm:kvm_cpuid(index: 1818588270, rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100603.121 CPU 7/KVM/1605125 kvm:kvm_cpuid(index: 1818588270, rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100603.122 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 1, index: 1818588270, rax: 722545, rbx: 119539712, rcx: 4294586915, rdx: 529267711, found: 1)
100603.123 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 7, rax: 2, rbx: 563881963, rcx: 406849340, rdx: 3154134032, found: 1)
100603.125 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 7, index: 1, rax: 2064, found: 1)
100603.131 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 13, index: 1, rax: 15, rbx: 840, found: 1)
100603.131 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 20, found: 1)
100603.132 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 25)
100603.133 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2147483648, rax: 2147483656, found: 1)
100603.135 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2147483649, rax: 722545, rcx: 289, rdx: 739248128, found: 1)
100603.136 CPU 7/KVM/1605125 kvm:kvm_cpuid(function: 2147483656, index: 33, rax: 3026990, rbx: 16830464, found: 1)
100605.563 CPU 3/KVM/1605120 kvm:kvm_cpuid(rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100605.566 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 1, index: 2472044104, rax: 722545, rbx: 52430848, rcx: 4294586915, rdx: 529267711, found: 1)
100605.567 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 7, rax: 2, rbx: 563881963, rcx: 406849340, rdx: 3154134032, found: 1)
100605.568 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 7, index: 1, rax: 2064, found: 1)
100605.569 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 1, rax: 15, rbx: 840, found: 1)
100605.570 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 20, found: 1)
100605.571 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 25)
100605.572 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2147483648, rax: 2147483656, found: 1)
100605.574 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2147483649, rax: 722545, rcx: 289, rdx: 739248128, found: 1)
100605.575 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2147483655, index: 289)
100605.576 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2147483656, rax: 3026990, rbx: 16830464, found: 1)
100605.577 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, rax: 519, rbx: 2696, rcx: 2696, found: 1)
100605.578 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 2, rax: 256, rbx: 576, found: 1)
100605.579 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 3)
100605.580 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 5)
100605.581 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 6)
100605.582 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 7)
100605.583 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 17)
100605.584 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 18)
100605.585 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 19)
100605.586 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, index: 529236223, rax: 1, rdx: 255, found: 1)
100605.587 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.588 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.589 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.590 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.591 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100605.592 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100605.593 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.594 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.595 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.596 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.597 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.598 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.599 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.600 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.601 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.601 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.602 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.603 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.604 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.605 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100605.607 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.608 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.609 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.610 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100605.611 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.612 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.613 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.614 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100605.615 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.616 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.617 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.618 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100605.619 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100605.620 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.621 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.621 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.622 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100605.623 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100605.624 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2, rax: 1, rdx: 255, found: 1)
100605.625 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.626 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.627 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100605.628 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100605.629 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 4, found: 1)
100605.629 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, rax: 2080375073, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.630 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 1, rax: 2080375074, rbx: 29360191, rcx: 63, rdx: 1, found: 1)
100605.631 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 2, rax: 2080375107, rbx: 62914623, rcx: 4095, rdx: 1, found: 1)
100605.632 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 4, index: 3, rax: 2080883043, rbx: 62914623, rcx: 16383, rdx: 6, found: 1)
100605.633 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 11, rbx: 1, rcx: 256, rdx: 3, found: 1)
100605.634 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 11, index: 1, rax: 5, rbx: 32, rcx: 513, rdx: 3, found: 1)
100606.071 CPU 3/KVM/1605120 kvm:kvm_cpuid(index: 2119494672, rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100606.072 CPU 3/KVM/1605120 kvm:kvm_cpuid(index: 1818588270, rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100606.073 CPU 3/KVM/1605120 kvm:kvm_cpuid(index: 1818588270, rax: 32, rbx: 1970169159, rcx: 1818588270, rdx: 1231384169, found: 1)
100606.074 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 1, index: 1818588270, rax: 722545, rbx: 52430848, rcx: 4294586915, rdx: 529267711, found: 1)
100606.076 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 7, rax: 2, rbx: 563881963, rcx: 406849340, rdx: 3154134032, found: 1)
100606.077 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 7, index: 1, rax: 2064, found: 1)
100606.079 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 13, index: 1, rax: 15, rbx: 840, found: 1)
100606.083 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 20, found: 1)
100606.084 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 25)
100606.085 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2147483648, rax: 2147483656, found: 1)
100606.086 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2147483649, rax: 722545, rcx: 289, rdx: 739248128, found: 1)
100606.087 CPU 3/KVM/1605120 kvm:kvm_cpuid(function: 2147483656, index: 33, rax: 3026990, rbx: 16830464, found: 1)
```

## 关键参考资料
https://wiki.qemu.org/Features/CPUModels#Querying_host_capabilities

## qmp shell 中关于 cpu 的
- query-cpu-definitions
- query-cpu-model-expansion
  - [ ] 不知道如何使用
- query-cpus-fast
  - 没啥用


### query-cpu-definitions
```c
CpuDefinitionInfoList *qmp_query_cpu_definitions(Error **errp)
{
    CpuDefinitionInfoList *cpu_list = NULL;
    GSList *list = get_sorted_cpu_model_list();
    g_slist_foreach(list, x86_cpu_definition_entry, &cpu_list);
    g_slist_free(list);
    return cpu_list;
}
```

1. 直接将当时注册全部放过来
```c
static GSList *get_sorted_cpu_model_list(void)
{
    GSList *list = object_class_get_list(TYPE_X86_CPU, false);
    list = g_slist_sort(list, x86_cpu_list_compare);
    return list;
}

static void x86_register_cpu_model_type(const char *name, X86CPUModel *model)
{
    g_autofree char *typename = x86_cpu_type_name(name);
    TypeInfo ti = {
        .name = typename,
        .parent = TYPE_X86_CPU,
        .class_init = x86_cpu_cpudef_class_init,
        .class_data = model,
    };

    type_register(&ti);
}
```

- x86_cpu_class_check_missing_features
  - x86_cpu_expand_features
    - 因为支持 `-cpu Skylake-Client-IBRS,hle=off,rtm=off`，所以可以实现
  - x86_cpu_filter_features
    - x86_cpu_filter_features
      - x86_cpu_get_supported_feature_word : CPUID_FEATURE_WORD
        - kvm_arch_get_supported_cpuid
          - get_supported_cpuid : 大多数查询 kvm 就可以了
          - host_cpuid : 但是有的是查询 cpuid， 为什么有的直接 host 上询问，有的使用 kvm 询问啊
        - kvm_arch_get_supported_msr_feature
      - kvm_arch_get_supported_msr_feature : MSR_FEATURE_WORD : 想不到啊，针对于 ARM 的确实
    - CPUID_7_0_EBX_INTEL_PT 被特殊检查了
  - x86_cpu_list_feature_names
    - 根据 bit 计算为名称

```c
struct ArchCPU {
FeatureWordArray filtered_features; // host 上不存在的
}
```

QEMU 中的 get_supported_cpuid 继续下去:
- kvm_arch_dev_ioctl
  - kvm_dev_ioctl_get_cpuid
    - get_cpuid_func
      - do_cpuid_func
        - `__do_cpuid_func`
          - do_host_cpuid

CPUArchState::features 中存储了当前 CPU 的 capability ，使用 `cpu_x86_cpuid` 来查询该


-> 启动过程中检查是如何进行的

- x86_cpu_filter_features 中
```c
    for (w = 0; w < FEATURE_WORDS; w++) {
        uint64_t host_feat =
            x86_cpu_get_supported_feature_word(w, false);
        uint64_t requested_features = env->features[w];
        uint64_t unavailable_features = requested_features & ~host_feat;
        mark_unavailable_features(cpu, w, unavailable_features, prefix);
    }
```
其实大家走的一个路径的

## QEMU 似乎在错误的使用一些全局变量
使用 `-cpu Skylake-Client-IBRS,hle=off,rtm=off` 之后，感觉
```c
/* Compatibily hack to maintain legacy +-feat semantic,
 * where +-feat overwrites any feature set by
 * feat=on|feat even if the later is parsed after +-feat
 * (i.e. "-x2apic,x2apic=on" will result in x2apic disabled)
 */
static GList *plus_features, *minus_features;
```
和 qmp query-cpu-definitions 的配合出现了问题。

本来是理解 query-cpu-definitions 是分析 host 的，但是现在感觉是分析当时提供的参数的。

```json
    {
      "name": "Skylake-Client-IBRS",
      "typename": "Skylake-Client-IBRS-x86_64-cpu",
      "unavailable-features": [
        "hle",
        "rtm"
      ],
      "alias-of": "Skylake-Client-v2",
      "static": false,
      "migration-safe": true,
      "deprecated": false
    },
```

修改后
```json
    {
      "name": "Skylake-Client-IBRS",
      "typename": "Skylake-Client-IBRS-x86_64-cpu",
      "unavailable-features": [],
      "alias-of": "Skylake-Client-v2",
      "static": false,
      "migration-safe": true,
      "deprecated": false
    },
```
### qemu -cpu ?

使用这种方式可以获取，内容是相同
```txt
🧀  qemu-x86_64 -cpu ?
```
具体内容参考 ./qemu-cpu.txt

## builtin_x86_defs 中的定义都是正确
是的，算是非常清晰了，除了 vmx feature 是过多显示的内容。

## 如果你非要使用某一个指令，可以使用 avx 来测试
后果就是程序被 kill

将 avx 指令禁用掉。

- https://github.com/kshitijl/avx2-examples

- [ ] 但是在 kvm 中没有找到证据哇。

## 在 Denverton 中存在 SPEC_CTRL 最后是怎么通过检查的
因为最后调用是通过 ioctl 询问 kvm 模块获取到的



## 有趣的问题
https://pve.proxmox.com/pve-docs/chapter-qm.html#qm_cpu

- 原来 vmware 也是有 EVC 的
  - https://knowledge.broadcom.com/external/article/313545/vmware-evc-and-cpu-compatibility-faq.html
  - https://blogs.vmware.com/vsphere/2019/06/enhanced-vmotion-compatibility-evc-explained.html

- https://docs.vmware.com/en/VMware-vSphere/8.0/vsphere-vcenter-esxi-management/GUID-03E7E5F9-06D9-463F-A64F-D4EC20DAF22E.html

> CPU compatibility masks cannot prevent virtual machines from accessing masked CPU features in all circumstances. In some circumstances, applications can detect and use masked features even though they are hidden from the guest operating system. In addition, on any host, applications that use unsupported methods of detecting CPU features rather than using the CPUID instruction can access masked features. Virtual machines running applications that use unsupported CPU detection methods might experience stability problems after migration.

这个不太科学吧

[cpuid 必然导致 vmexit 的](https://stackoverflow.com/questions/63214415/does-hypervisor-like-kvm-need-to-vm-exit-on-cpuid)

## 既然没有任何检查，是不是说明 kvm 的 filter 内容可以重新加回去?

## arch/x86/include/asm/cpufeatures.h

## QEMU 是如何调用的
关键描述:
- kvm_arch_get_supported_cpuid

其调用者为:
- x86_cpu_filter_features
- x86_cpu_get_supported_feature_word
- x86_cpu_get_supported_cpuid
- cpu_x86_cpuid

- kvm_request_xsave_components


## 为什么 Host 中没有看到，但是 Guest 中有看到
分析下，为什么 host 上不显示这个
1. arch/x86/kernel/cpu/capflags.c 直接就没有生成这个；
2. arch/x86/kernel/cpu/Makefile 中通过 arch/x86/kernel/cpu/mkcapflags.sh 来生成。

## 是不是 kvm 上也存在特殊的操作
- 应该是和那个东西没有什么关系吧

很奇怪，为什么

## cpuflags 对于性能的影响是什么？

## 如果 Host 中不提供，应该是硬件来控制的吧


### 整理思路
- 首先，存在一个 ./cpuid -d -c 1 的
```txt
CPUID 00000007:00 = 00000001 219c07ab 1840073c ac004410 | .......!<.@..D..
```
- cpu_x86_cpuid : index count
  - index 为 7 ，count 为 0 其中 ebx 为 219c07ab
- `kvm_arch_get_supported_cpuid` 为 function index


## [x] 为什么会存在 CPUID_LNX_4 ，是为了告诉用户态什么东西吗？

例如 X86_FEATURE_SPLIT_LOCK_DETECT
- 似乎很多都隐藏了，这是一个特殊点

- split_lock_setup
  - `__split_lock_setup`

有时候，其实就是像是全局变量一样，用于通知剩下的各个模块。

## [x] 简单分析一下 arch/x86/kvm/cpuid.c 到底说了什么?

1. 咨询 kernel 一共存在多少 flags :
QEMU 侧
- x86_cpu_expand_features
  - kvm_arch_get_supported_cpuid
    - get_supported_cpuid
      - try_get_cpuid
        - kvm_ioctl(s, KVM_GET_SUPPORTED_CPUID, cpuid);
    - 后面还存在一系列的 fixup

kernel 侧:
- kvm_dev_ioctl_get_cpuid : 使用命令 KVM_GET_SUPPORTED_CPUID 和 KVM_GET_EMULATED_CPUID
  - sanity_check_entries : 一些无关痛痒的检查
  - 初始化 kvm_cpuid_array
  - get_cpuid_func
    - do_cpuid_func
      - `__do_cpuid_func_emulated`
      - `__do_cpuid_func`
        - do_host_cpuid : 全部都是被 `__do_cpuid_func` 调用，
          - `get_next_cpuid`
        - kvm_cpu_cap_has : 访问数组 `kvm_cpu_caps`，检查一些比较特殊的 flags
        - cpuid_entry_override : 访问数组 `kvm_cpu_caps`，保证只有 kvm 支持的内容才会传递出去

- kvm_arch_vcpu_ioctl:
  - kvm_vcpu_ioctl_get_cpuid2 : 一个简单的拷贝，将 vcpu->arch.cpuid_entries 拷贝出来，QEMU 没有使用这个

2. QEMU 真正 cpuid

- kvm_arch_init_vcpu
  - 在 stack 中声明 cpuid_data
  - 通过调用 cpu_x86_cpuid 来获取，而 cpu_x86_cpuid 通过访问 CPUX86State::features
  - kvm_vcpu_ioctl : 设置 cpuid 了


- kvm_vcpu_ioctl_set_cpuid2
  - kvm_set_cpuid


3. kvm 内查询
- kvm_find_cpuid_entry_index
  - cpuid_entry2_find


4. kvm 初始化获取参数:
- vmx_set_cpu_caps
  - kvm_set_cpu_caps : 这里规定了 kvm 中总共可以使用那些 flags
    - `__kvm_cpu_cap_mask`
    - cpuid_count : 当然需要 host 中也提供才可以，但是只能访问两个


## hyperv cpuid 的增加流程是如何实现的
1. x86_cpu_expand_features : 根据参数来增加 features
2. kvm_arch_init_vcpu : 将数组 CPUArchState::features 中的内容同步到操作系统中


## 有的 cpuflags 是看不到的，例如 spec_ctrl
cpuid -1 可以检测到
```txt
      IBRS/IBPB: indirect branch restrictions  = true
```
但是 /proc/cpuinfo 不能
```c
#define X86_FEATURE_SPEC_CTRL		(18*32+26) /* "" Speculation Control (IBRS + IBPB) */
```
这个并不会导致，这个是一个判断失误。

## [ ] 如何理解 KVM_CPUID_FLAG_SIGNIFCANT_INDEX


## 当向 kvm 的 cpuid 的时候，kvm 是不做检查需要的 flags 的
- kvm_arch_init_vcpu 中
```c
        case 0x7:
        case 0x12:
            for (j = 0; ; j++) {
                c->function = i;
                c->flags = KVM_CPUID_FLAG_SIGNIFCANT_INDEX;
                c->index = j;
                cpu_x86_cpuid(env, i, j, &c->eax, &c->ebx, &c->ecx, &c->edx);
                // 去掉 avx2 的时候
                /* [->:kvm_arch_init_vcpu:2006] 1 219c078b 1840072c ac004410 */
                /* [->:kvm_arch_init_vcpu:2006] 810 0 0 0 */
                /* [->:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [->:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [->:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [->:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                printf("[->:%s:%d] %x %x %x %x\n", __FUNCTION__, __LINE__, c->eax, c->ebx, c->ecx, c->edx);

                // 增加 rtm 的 flag
                if (c->ebx == 0x219c07ab){
                    c->ebx = c->ebx | (1 << 11);
                }
                // 增加 avx2
                /* [->:kvm_arch_init_vcpu:2006] 1 219c07ab 1840072c ac004410 */
                /* [->:kvm_arch_init_vcpu:2006] 810 0 0 0 */
                /* [->:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [->:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [->:kvm_arch_init_vcpu:2006] 0 0 0 0 */
                /* [->:kvm_arch_init_vcpu:2006] 0 0 0 0 */

                if (j > 1 && (c->eax & 0xf) != 1) {
                    break;
                }

                if (cpuid_i == KVM_MAX_CPUID_ENTRIES) {
                    fprintf(stderr, "cpuid_data is full, no space for "
                                "cpuid(eax:0x12,ecx:0x%x)\n", j);
                    abort();
                }
                c = &cpuid_data.entries[cpuid_i++];
            }
            break;
```

```txt
[    0.743907] Run /init as init process
[    0.745631] traps: init[1] trap invalid opcode ip:7f480c569f49 sp:7ffc5c6e76b8 error:0 in libc.so.6[7f480c40a000+16f000]
[    0.746049] Kernel panic - not syncing: Attempted to kill init! exitcode=0x00000004
[    0.746323] CPU: 0 PID: 1 Comm: init Not tainted 6.3.0-rc6-dirty #214
[    0.746554] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[    0.746794] Call Trace:
[    0.746900]  <TASK>
[    0.746976]  dump_stack_lvl+0x4b/0x80
[    0.747115]  panic+0x32b/0x350
[    0.747235]  do_exit+0x988/0xb30
[    0.747355]  do_group_exit+0x31/0x80
[    0.747486]  get_signal+0xa17/0xa40
[    0.747617]  ? __schedule+0x312/0x11b0
[    0.747756]  arch_do_signal_or_restart+0x2e/0x270
[    0.747930]  exit_to_user_mode_prepare+0xea/0x130
[    0.748105]  irqentry_exit_to_user_mode+0x9/0x20
[    0.748276]  asm_exc_invalid_op+0x1a/0x20
[    0.748426] RIP: 0033:0x7f480c569f49
[    0.748558] Code: 20 c5 dd 74 d9 c5 fd d7 ca c5 fd d7 c3 09 c1 74 90 85 c0 75 2c 85 d2 0f 84 84 00 00 00 89 d0 48 89 f7 0f bd c0 48 8d 44 07 e0 <0f
> 01 d6 74 04 c5 fc 77 c3 c5 f8 77 c3 66 2e 0f 1f 84 00 00 00 00
[    0.749213] RSP: 002b:00007ffc5c6e76b8 EFLAGS: 00010206
[    0.749393] RAX: 00007ffc5c6e7fc0 RBX: 00007ffc5c6e7758 RCX: 0000000080102020
[    0.749647] RDX: 00007ffc5c6e7770 RSI: 000000000000002f RDI: 00007ffc5c6e7fe0
[    0.749904] RBP: 00007ffc5c6e7fc0 R08: 000000000000003f R09: 00007f480c3e9c50
[    0.750159] R10: 00007ffc5c6e7300 R11: 0000000000000202 R12: 00007ffc5c6e7758
[    0.750414] R13: 00007ffc5c6e7770 R14: 00007f480c5ca5a0 R15: 0000000000000000
[    0.750668]  </TASK>
[    0.750778] Kernel Offset: disabled
[    0.750907] ---[ end Kernel panic - not syncing: Attempted to kill init! exitcode=0x00000004 ]---
```
得到的结果如上。

可见，kvm 是对于 QEMU 设置何种 flags 是不做检查的。

## 那些 cpuid 是需要从 kvm 退出到 QEMU 来处理的
应该是不需要的

## 如何模拟 cache

在 `cpu_x86_cpuid` 中的:
```c
        /* cache info: needed for Pentium Pro compatibility */
        if (cpu->cache_info_passthrough) {
            x86_cpu_get_cache_cpuid(index, 0, eax, ebx, ecx, edx);
            break;
        } else if (cpu->vendor_cpuid_only && IS_AMD_CPU(env)) {
            *eax = *ebx = *ecx = *edx = 0;
            break;
        }
```

## [ ] kvm 会如何过滤 cpuid ?


## kvm 如何向 QEMU 返回系统中支持的 cpuid 的

其实，系统中，比较的应该是

```c
static __always_inline u32 kvm_cpu_cap_get(unsigned int x86_feature)
{
	unsigned int x86_leaf = __feature_leaf(x86_feature);

	reverse_cpuid_check(x86_leaf);
	return kvm_cpu_caps[x86_leaf] & __feature_bit(x86_feature);
}
```

## kvm_vcpu::arch::cpuid_entries 的组装和使用是怎么样的?
- 直接拷贝过来，然后让


## [ ] 如果是 -cpu host 的时候，那么还存在设置 KVM_SET_CPUID2 吗?
这个时候，应该是 QEMU 从 kvm 获取到什么就设置回去


## windows 的 CPUID 还是在这个体系下面的吗?

## [ ] 是如何修改 Guest 运行的状态时候的 CPUID 的，在硬件中?

## 思考一个问题
既然 model 如何难以定义，凭什么说，model 支持就可以传递过来。
是如何 check 一个 model 的?


## 为什么 13900k 虚拟机没有 pmu ，从哪一步过滤的

虚拟机中 lscpu 发现没有 arch_perfmon

虚拟机日志:
```txt
[    0.143596] Performance Events: unsupported p6 CPU model 183 no PMU driver, so ftware events only.
```

通过这个问题，理解过滤的整个过程是什么


物理机:
```txt
🧀  cpuid -l 10 -r -1
CPU:
   0x0000000a 0x00: eax=0x07300605 ebx=0x00000000 ecx=0x00000007 edx=0x00008603
```

```txt
🧀  cpuid -l 10 -r -1
CPU:
   0x0000000a 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
```

使用 -cpu max 和 -cpu host,pmu=on 都没用的


- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_init_vcpu
          - kvm_arch_init_vcpu
            - kvm_x86_build_cpuid
              - cpu_x86_cpuid  ( 这里打开了 : cpu->enable_pmu ，所以会去调用 x86_cpu_get_supported_cpuid ，但是获取的还是 0)
                - kvm_arch_get_supported_cpuid 会对于 kvm 中获取的内容再次修正

在 __do_cpuid_func 中很容易看到，kvm 把 pmu 关闭了:

cat /sys/module/kvm/parameters/enable_pmu

检查了一下，应该就是在 kvm_init_pmu_capability 中，由于 13900k 大小核，无法 enable 的。


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
