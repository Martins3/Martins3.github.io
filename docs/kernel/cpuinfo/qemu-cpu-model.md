# 分析下 QEMU cpu model 的

- virsh cpu-models x86_64
  - 应该展示的 qemu 支持的 x86_64 的所有的 model
- virsh domcapabilities
  - 最终调用为 qmp 的 query-cpu-definitions

- 本机不支持 rtm，如果强制使用 -cpu Skylake-Client-IBRS 启动，那么将会得到大量的警告

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
        - get_supported_cpuid : 大多数查询 kvm 就可以了
        - host_cpuid : 为什么有的从 host 上询问，有的使用 kvm 询问啊
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
