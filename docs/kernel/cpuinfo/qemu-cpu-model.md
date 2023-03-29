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
  - x86_cpu_list_feature_names
    - 根据 bit 计算为名称

```c
struct ArchCPU {
FeatureWordArray filtered_features; // host 上不存在的
}
```

使用 `-cpu Skylake-Client-IBRS,hle=off,rtm=off` 之后，感觉
query-cpu-definitions.json 的语义有问题:
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

## aperfmperf 是无法透传给 vCPU 的
从 guest 中获取的 /proc/cpuinfo 完全是错的
```txt
➜  ~ cat /proc/cpuinfo | grep MHz
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
```

## 如果你非要使用某一个指令，可以使用 avx 来测试
后果就是程序被 kill

将 avx 指令禁用掉。

- https://github.com/kshitijl/avx2-examples
