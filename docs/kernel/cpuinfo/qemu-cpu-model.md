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
Available CPUs:
x86 486                   (alias configured by machine type)
x86 486-v1
x86 Broadwell             (alias configured by machine type)
x86 Broadwell-IBRS        (alias of Broadwell-v3)
x86 Broadwell-noTSX       (alias of Broadwell-v2)
x86 Broadwell-noTSX-IBRS  (alias of Broadwell-v4)
x86 Broadwell-v1          Intel Core Processor (Broadwell)
x86 Broadwell-v2          Intel Core Processor (Broadwell, no TSX)
x86 Broadwell-v3          Intel Core Processor (Broadwell, IBRS)
x86 Broadwell-v4          Intel Core Processor (Broadwell, no TSX, IBRS)
x86 Cascadelake-Server    (alias configured by machine type)
x86 Cascadelake-Server-noTSX  (alias of Cascadelake-Server-v3)
x86 Cascadelake-Server-v1  Intel Xeon Processor (Cascadelake)
x86 Cascadelake-Server-v2  Intel Xeon Processor (Cascadelake) [ARCH_CAPABILITIES]
x86 Cascadelake-Server-v3  Intel Xeon Processor (Cascadelake) [ARCH_CAPABILITIES, no TSX]
x86 Cascadelake-Server-v4  Intel Xeon Processor (Cascadelake) [ARCH_CAPABILITIES, no TSX]
x86 Cascadelake-Server-v5  Intel Xeon Processor (Cascadelake) [ARCH_CAPABILITIES, EPT switching, XSAVES, no TSX]
x86 Conroe                (alias configured by machine type)
x86 Conroe-v1             Intel Celeron_4x0 (Conroe/Merom Class Core 2)
x86 Cooperlake            (alias configured by machine type)
x86 Cooperlake-v1         Intel Xeon Processor (Cooperlake)
x86 Cooperlake-v2         Intel Xeon Processor (Cooperlake) [XSAVES]
x86 Denverton             (alias configured by machine type)
x86 Denverton-v1          Intel Atom Processor (Denverton)
x86 Denverton-v2          Intel Atom Processor (Denverton) [no MPX, no MONITOR]
x86 Denverton-v3          Intel Atom Processor (Denverton) [XSAVES, no MPX, no MONITOR]
x86 Dhyana                (alias configured by machine type)
x86 Dhyana-v1             Hygon Dhyana Processor
x86 Dhyana-v2             Hygon Dhyana Processor [XSAVES]
x86 EPYC                  (alias configured by machine type)
x86 EPYC-IBPB             (alias of EPYC-v2)
x86 EPYC-Milan            (alias configured by machine type)
x86 EPYC-Milan-v1         AMD EPYC-Milan Processor
x86 EPYC-Rome             (alias configured by machine type)
x86 EPYC-Rome-v1          AMD EPYC-Rome Processor
x86 EPYC-Rome-v2          AMD EPYC-Rome Processor
x86 EPYC-v1               AMD EPYC Processor
x86 EPYC-v2               AMD EPYC Processor (with IBPB)
x86 EPYC-v3               AMD EPYC Processor
x86 Haswell               (alias configured by machine type)
x86 Haswell-IBRS          (alias of Haswell-v3)
x86 Haswell-noTSX         (alias of Haswell-v2)
x86 Haswell-noTSX-IBRS    (alias of Haswell-v4)
x86 Haswell-v1            Intel Core Processor (Haswell)
x86 Haswell-v2            Intel Core Processor (Haswell, no TSX)
x86 Haswell-v3            Intel Core Processor (Haswell, IBRS)
x86 Haswell-v4            Intel Core Processor (Haswell, no TSX, IBRS)
x86 Icelake-Server        (alias configured by machine type)
x86 Icelake-Server-noTSX  (alias of Icelake-Server-v2)
x86 Icelake-Server-v1     Intel Xeon Processor (Icelake)
x86 Icelake-Server-v2     Intel Xeon Processor (Icelake) [no TSX]
x86 Icelake-Server-v3     Intel Xeon Processor (Icelake)
x86 Icelake-Server-v4     Intel Xeon Processor (Icelake)
x86 Icelake-Server-v5     Intel Xeon Processor (Icelake) [XSAVES]
x86 Icelake-Server-v6     Intel Xeon Processor (Icelake) [5-level EPT]
x86 IvyBridge             (alias configured by machine type)
x86 IvyBridge-IBRS        (alias of IvyBridge-v2)
x86 IvyBridge-v1          Intel Xeon E3-12xx v2 (Ivy Bridge)
x86 IvyBridge-v2          Intel Xeon E3-12xx v2 (Ivy Bridge, IBRS)
x86 KnightsMill           (alias configured by machine type)
x86 KnightsMill-v1        Intel Xeon Phi Processor (Knights Mill)
x86 Nehalem               (alias configured by machine type)
x86 Nehalem-IBRS          (alias of Nehalem-v2)
x86 Nehalem-v1            Intel Core i7 9xx (Nehalem Class Core i7)
x86 Nehalem-v2            Intel Core i7 9xx (Nehalem Core i7, IBRS update)
x86 Opteron_G1            (alias configured by machine type)
x86 Opteron_G1-v1         AMD Opteron 240 (Gen 1 Class Opteron)
x86 Opteron_G2            (alias configured by machine type)
x86 Opteron_G2-v1         AMD Opteron 22xx (Gen 2 Class Opteron)
x86 Opteron_G3            (alias configured by machine type)
x86 Opteron_G3-v1         AMD Opteron 23xx (Gen 3 Class Opteron)
x86 Opteron_G4            (alias configured by machine type)
x86 Opteron_G4-v1         AMD Opteron 62xx class CPU
x86 Opteron_G5            (alias configured by machine type)
x86 Opteron_G5-v1         AMD Opteron 63xx class CPU
x86 Penryn                (alias configured by machine type)
x86 Penryn-v1             Intel Core 2 Duo P9xxx (Penryn Class Core 2)
x86 SandyBridge           (alias configured by machine type)
x86 SandyBridge-IBRS      (alias of SandyBridge-v2)
x86 SandyBridge-v1        Intel Xeon E312xx (Sandy Bridge)
x86 SandyBridge-v2        Intel Xeon E312xx (Sandy Bridge, IBRS update)
x86 Skylake-Client        (alias configured by machine type)
x86 Skylake-Client-IBRS   (alias of Skylake-Client-v2)
x86 Skylake-Client-noTSX-IBRS  (alias of Skylake-Client-v3)
x86 Skylake-Client-v1     Intel Core Processor (Skylake)
x86 Skylake-Client-v2     Intel Core Processor (Skylake, IBRS)
x86 Skylake-Client-v3     Intel Core Processor (Skylake, IBRS, no TSX)
x86 Skylake-Client-v4     Intel Core Processor (Skylake, IBRS, no TSX) [IBRS, XSAVES, no TSX]
x86 Skylake-Server        (alias configured by machine type)
x86 Skylake-Server-IBRS   (alias of Skylake-Server-v2)
x86 Skylake-Server-noTSX-IBRS  (alias of Skylake-Server-v3)
x86 Skylake-Server-v1     Intel Xeon Processor (Skylake)
x86 Skylake-Server-v2     Intel Xeon Processor (Skylake, IBRS)
x86 Skylake-Server-v3     Intel Xeon Processor (Skylake, IBRS, no TSX)
x86 Skylake-Server-v4     Intel Xeon Processor (Skylake, IBRS, no TSX)
x86 Skylake-Server-v5     Intel Xeon Processor (Skylake, IBRS, no TSX) [IBRS, XSAVES, EPT switching, no TSX]
x86 Snowridge             (alias configured by machine type)
x86 Snowridge-v1          Intel Atom Processor (SnowRidge)
x86 Snowridge-v2          Intel Atom Processor (Snowridge, no MPX)
x86 Snowridge-v3          Intel Atom Processor (Snowridge, no MPX) [XSAVES, no MPX]
x86 Snowridge-v4          Intel Atom Processor (Snowridge, no MPX) [no split lock detect, no core-capability]
x86 Westmere              (alias configured by machine type)
x86 Westmere-IBRS         (alias of Westmere-v2)
x86 Westmere-v1           Westmere E56xx/L56xx/X56xx (Nehalem-C)
x86 Westmere-v2           Westmere E56xx/L56xx/X56xx (IBRS update)
x86 athlon                (alias configured by machine type)
x86 athlon-v1             QEMU Virtual CPU version 2.5+
x86 core2duo              (alias configured by machine type)
x86 core2duo-v1           Intel(R) Core(TM)2 Duo CPU     T7700  @ 2.40GHz
x86 coreduo               (alias configured by machine type)
x86 coreduo-v1            Genuine Intel(R) CPU           T2600  @ 2.16GHz
x86 kvm32                 (alias configured by machine type)
x86 kvm32-v1              Common 32-bit KVM processor
x86 kvm64                 (alias configured by machine type)
x86 kvm64-v1              Common KVM processor
x86 n270                  (alias configured by machine type)
x86 n270-v1               Intel(R) Atom(TM) CPU N270   @ 1.60GHz
x86 pentium               (alias configured by machine type)
x86 pentium-v1
x86 pentium2              (alias configured by machine type)
x86 pentium2-v1
x86 pentium3              (alias configured by machine type)
x86 pentium3-v1
x86 phenom                (alias configured by machine type)
x86 phenom-v1             AMD Phenom(tm) 9550 Quad-Core Processor
x86 qemu32                (alias configured by machine type)
x86 qemu32-v1             QEMU Virtual CPU version 2.5+
x86 qemu64                (alias configured by machine type)
x86 qemu64-v1             QEMU Virtual CPU version 2.5+
x86 base                  base CPU model type with no features enabled
x86 max                   Enables all features supported by the accelerator in the current host

Recognized CPUID flags:
  3dnow 3dnowext 3dnowprefetch abm ace2 ace2-en acpi adx aes amd-no-ssb
  amd-ssbd amd-stibp amx-bf16 amx-int8 amx-tile apic arat arch-capabilities
  arch-lbr avic avx avx-vnni avx2 avx512-4fmaps avx512-4vnniw avx512-bf16
  avx512-fp16 avx512-vp2intersect avx512-vpopcntdq avx512bitalg avx512bw
  avx512cd avx512dq avx512er avx512f avx512ifma avx512pf avx512vbmi
  avx512vbmi2 avx512vl avx512vnni bmi1 bmi2 bus-lock-detect cid cldemote
  clflush clflushopt clwb clzero cmov cmp-legacy core-capability cr8legacy
  cx16 cx8 dca de decodeassists ds ds-cpl dtes64 erms est extapic f16c
  flushbyasid fma fma4 fpu fsgsbase fsrm full-width-write fxsr fxsr-opt
  gfni hle ht hypervisor ia64 ibpb ibrs ibrs-all ibs intel-pt intel-pt-lip
  invpcid invtsc kvm-asyncpf kvm-asyncpf-int kvm-hint-dedicated kvm-mmu
  kvm-msi-ext-dest-id kvm-nopiodelay kvm-poll-control kvm-pv-eoi kvm-pv-ipi
  kvm-pv-sched-yield kvm-pv-tlb-flush kvm-pv-unhalt kvm-steal-time kvmclock
  kvmclock kvmclock-stable-bit la57 lahf-lm lbrv lm lwp mca mce md-clear
  mds-no misalignsse mmx mmxext monitor movbe movdir64b movdiri mpx msr
  mtrr nodeid-msr npt nrip-save nx osvw pae pat pause-filter pbe pcid
  pclmulqdq pcommit pdcm pdpe1gb perfctr-core perfctr-nb pfthreshold pge
  phe phe-en pks pku pmm pmm-en pn pni popcnt pschange-mc-no pse pse36
  rdctl-no rdpid rdrand rdseed rdtscp rsba rtm sep serialize sgx sgx-debug
  sgx-exinfo sgx-kss sgx-mode64 sgx-provisionkey sgx-tokenkey sgx1 sgx2
  sgxlc sha-ni skinit skip-l1dfl-vmentry smap smep smx spec-ctrl
  split-lock-detect ss ssb-no ssbd sse sse2 sse4.1 sse4.2 sse4a ssse3 stibp
  svm svm-lock svme-addr-chk syscall taa-no tbm tce tm tm2 topoext tsc
  tsc-adjust tsc-deadline tsc-scale tsx-ctrl tsx-ldtrk umip v-vmsave-vmload
  vaes vgif virt-ssbd vmcb-clean vme vmx vmx-activity-hlt
  vmx-activity-shutdown vmx-activity-wait-sipi vmx-apicv-register
  vmx-apicv-vid vmx-apicv-x2apic vmx-apicv-xapic vmx-cr3-load-noexit
  vmx-cr3-store-noexit vmx-cr8-load-exit vmx-cr8-store-exit vmx-desc-exit
  vmx-encls-exit vmx-entry-ia32e-mode vmx-entry-load-bndcfgs
  vmx-entry-load-efer vmx-entry-load-pat vmx-entry-load-perf-global-ctrl
  vmx-entry-load-pkrs vmx-entry-load-rtit-ctl vmx-entry-noload-debugctl
  vmx-ept vmx-ept-1gb vmx-ept-2mb vmx-ept-advanced-exitinfo
  vmx-ept-execonly vmx-eptad vmx-eptp-switching vmx-exit-ack-intr
  vmx-exit-clear-bndcfgs vmx-exit-clear-rtit-ctl vmx-exit-load-efer
  vmx-exit-load-pat vmx-exit-load-perf-global-ctrl vmx-exit-load-pkrs
  vmx-exit-nosave-debugctl vmx-exit-save-efer vmx-exit-save-pat
  vmx-exit-save-preemption-timer vmx-flexpriority vmx-hlt-exit vmx-ins-outs
  vmx-intr-exit vmx-invept vmx-invept-all-context vmx-invept-single-context
  vmx-invept-single-context vmx-invept-single-context-noglobals
  vmx-invlpg-exit vmx-invpcid-exit vmx-invvpid vmx-invvpid-all-context
  vmx-invvpid-single-addr vmx-io-bitmap vmx-io-exit vmx-monitor-exit
  vmx-movdr-exit vmx-msr-bitmap vmx-mtf vmx-mwait-exit vmx-nmi-exit
  vmx-page-walk-4 vmx-page-walk-5 vmx-pause-exit vmx-ple vmx-pml
  vmx-posted-intr vmx-preemption-timer vmx-rdpmc-exit vmx-rdrand-exit
  vmx-rdseed-exit vmx-rdtsc-exit vmx-rdtscp-exit vmx-secondary-ctls
  vmx-shadow-vmcs vmx-store-lma vmx-true-ctls vmx-tsc-offset
  vmx-tsc-scaling vmx-unrestricted-guest vmx-vintr-pending vmx-vmfunc
  vmx-vmwrite-vmexit-fields vmx-vnmi vmx-vnmi-pending vmx-vpid
  vmx-wbinvd-exit vmx-xsaves vmx-zero-len-inject vpclmulqdq waitpkg
  wbnoinvd wdt x2apic xcrypt xcrypt-en xfd xgetbv1 xop xsave xsavec
  xsaveerptr xsaveopt xsaves xstore xstore-en xtpr
```

## builtin_x86_defs 中的定义都是正确
是的，算是非常清晰了，除了 vmx feature 是过多显示的内容。
