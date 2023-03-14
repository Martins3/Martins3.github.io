## Guest kernel 变少了
flags : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq vmx ssse3 fma cx16 pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_vnni arat umip pku ospke waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize arch_capabilities
flags : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb intel_pt sha_ni xsaveopt xsavec xgetbv1 xsaves split_lock_detect avx_vnni dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi umip pku ospke waitpkg gfni vaes vpclmulqdq tme rdpid movdiri movdir64b fsrm md_clear serialize pconfig arch_lbr ibt flush_l1d arch_capabilities


会消失的 flags :
dts acpi tm pbe art pebs bts nonstop_tsc aperfmperf dtes64 monitor ds_cpl smx est tm2 sdbg xtpr epb intel_pt split_lock_detect dtherm ida pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi tme pconfig arch_lbr ibt flush_l1d

- [ ] 而且还可以增加 flags

为什么 acpi feature 消失了?

arch/x86/kernel/cpu/common.c  中通过 cpuid 指令获取:
```c
void get_cpu_cap(struct cpuinfo_x86 *c)
```

[cpuid 必然导致 vmexit 的](https://stackoverflow.com/questions/63214415/does-hypervisor-like-kvm-need-to-vm-exit-on-cpuid)

- https://dannorth.net/2022/02/10/cupid-for-joyful-coding/
  - 哈哈，只是巧合而已

## 似乎处理 hyperv 也是在处理 cpuid 相关的

## [ ] 如果真的不提供，那么 Guest 使用对应的能力的下场是什么
KVM_SET_CPUID : 就是会修改 Guest 使用 cpuid 获取到能力。

- kvm_vcpu_ioctl_set_cpuid
  - kvm_set_cpuid
    - `__kvm_update_cpuid_runtime`
    - kvm_vcpu_after_set_cpuid
      - vmx_vcpu_after_set_cpuid
        - vmx_update_exception_bitmap : 其实这个也不对啊

## [ ] acpi 这个 cpuid 是怎么被去掉的


## QEMU 是如何调用的
关键描述:
- kvm_arch_get_supported_cpuid

其调用者为:
- x86_cpu_filter_features
- x86_cpu_get_supported_feature_word
- x86_cpu_get_supported_cpuid
- cpu_x86_cpuid

- kvm_request_xsave_components

## 分析

```c
enum cpuid_leafs
{
	CPUID_1_EDX		= 0,
	CPUID_8000_0001_EDX,
	CPUID_8086_0001_EDX,
	CPUID_LNX_1,
	CPUID_1_ECX,
	CPUID_C000_0001_EDX,
	CPUID_8000_0001_ECX,
	CPUID_LNX_2,
	CPUID_LNX_3,
	CPUID_7_0_EBX,
	CPUID_D_1_EAX,
	CPUID_LNX_4,
	CPUID_7_1_EAX,
	CPUID_8000_0008_EBX,
	CPUID_6_EAX,
	CPUID_8000_000A_EDX,
	CPUID_7_ECX,
	CPUID_8000_0007_EBX,
	CPUID_7_EDX,
	CPUID_8000_001F_EAX,
	CPUID_8000_0021_EAX,
};
```
```c
static struct kvm_x86_init_ops vmx_init_ops __initdata = {
	.hardware_setup = hardware_setup,
	.handle_intel_pt_intr = NULL,

	.runtime_ops = &vmx_x86_ops,
	.pmu_ops = &intel_pmu_ops,
};
```

- hardware_setup
  - vmx_set_cpu_caps
    - kvm_set_cpu_caps : vmx 和 svm 的公共路径


- vmx_init
  - kvm_x86_vendor_init
    - `__kvm_x86_vendor_init`

## `do_host_cpuid`

```txt
  do_host_cpuid
  __do_cpuid_func
  kvm_dev_ioctl_get_cpuid
  kvm_arch_dev_ioctl
  __x64_sys_ioctl
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    2

  do_host_cpuid
  __do_cpuid_func
  kvm_dev_ioctl_get_cpuid
  kvm_arch_dev_ioctl
  __x64_sys_ioctl
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    3

  do_host_cpuid
  __do_cpuid_func
  kvm_dev_ioctl_get_cpuid
  kvm_arch_dev_ioctl
  __x64_sys_ioctl
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    4

  do_host_cpuid
  __do_cpuid_func
  kvm_dev_ioctl_get_cpuid
  kvm_arch_dev_ioctl
  __x64_sys_ioctl
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    9

  do_host_cpuid
  __do_cpuid_func
  kvm_dev_ioctl_get_cpuid
  kvm_arch_dev_ioctl
  __x64_sys_ioctl
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
  [unknown]
    16

  do_host_cpuid
  __do_cpuid_func
  kvm_dev_ioctl_get_cpuid
  kvm_arch_dev_ioctl
  __x64_sys_ioctl
  do_syscall_64
  entry_SYSCALL_64_after_hwframe
```

## cpuid 的 spec_ctrl 的

```c
#define X86_FEATURE_SPEC_CTRL		(18*32+26) /* "" Speculation Control (IBRS + IBPB) */
```


-cpu SandyBridge-IBRS
```txt
model name      : Intel Xeon E312xx (Sandy Bridge, IBRS update)
```

-cpu SandyBridge
```txt
model name      : Intel Xeon E312xx (Sandy Bridge)
```

这是因为中的定义:
```c
static const X86CPUDefinition builtin_x86_defs[] = {
```
```c
        .versions = (X86CPUVersionDefinition[]) {
            { .version = 1 },
            {
                .version = 2,
                .alias = "SandyBridge-IBRS",
                .props = (PropValue[]) {
                    { "spec-ctrl", "on" },
                    { "model-id",
                      "Intel Xeon E312xx (Sandy Bridge, IBRS update)" },
                    { /* end of list */ }
                }
            },
```
但是在 Guest 中都是看不到这个的。

是存在这个 flag 的啊
```txt
[    0.000000] [huxueshi:init_speculation_control:996] we found this
```
- [ ] 如果内核已经作出了修改，那么也是需要对应的改动吗？

## 为什么 Host 中没有看到，但是 Guest 中有看到
分析下，为什么 host 上不显示这个
1. arch/x86/kernel/cpu/capflags.c 直接就没有生成这个；
2. arch/x86/kernel/cpu/Makefile 中通过 arch/x86/kernel/cpu/mkcapflags.sh 来生成。

## 是不是 kvm 上也存在特殊的操作
- 应该是和那个东西没有什么关系吧

很奇怪，为什么

## cpuflags 对于性能的影响是什么？

## 如果 Host 中不提供，应该是硬件来控制的吧

## 通过 cpupower 工具理解
这个的源码在什么位置。
```txt
🧀  cpupower idle-info
CPUidle driver: intel_idle
CPUidle governor: menu
analyzing CPU 0:

Number of idle states: 4
Available idle states: POLL C1_ACPI C2_ACPI C3_ACPI
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 102555
Duration: 1768149
C1_ACPI:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 593921
Duration: 89988252
C2_ACPI:
Flags/Description: ACPI FFH MWAIT 0x21
Latency: 127
Usage: 925818
Duration: 509306411
C3_ACPI:
Flags/Description: ACPI FFH MWAIT 0x60
Latency: 1048
Usage: 1196072
Duration: 5598272250
```

```txt
🧀  sudo cpupower monitor
[sudo] password for martins3:
    | Nehalem                   || Mperf              || Idle_Stats
 CPU| C3   | C6   | PC3  | PC6  || C0   | Cx   | Freq || POLL | C1_A | C2_A | C3_A
   0|  0.00|  2.65|  0.00|  0.00||  1.06| 98.94|  5483||  0.00|  0.86|  2.98| 95.15
   1|  0.00|  2.65|  0.00|  0.00||  0.07| 99.93|  5475||  0.00|  0.02|  0.66| 99.25
   2|  0.00|  1.88|  0.00|  0.00||  0.72| 99.28|  5488||  0.01|  1.06|  2.84| 95.39
   3|  0.00|  1.88|  0.00|  0.00||  0.00|100.00|  5229||  0.00|  0.00|  0.00| 99.99
   4|  0.00|  1.01|  0.00|  0.00||  0.57| 99.43|  5484||  0.00|  0.93|  1.30| 97.15
   5|  0.00|  1.01|  0.00|  0.00||  0.00|100.00|  5275||  0.00|  0.00|  0.00| 99.99
   6|  0.00|  2.49|  0.00|  0.00||  0.32| 99.68|  5490||  0.00|  0.46|  3.47| 95.77
   7|  0.00|  2.49|  0.00|  0.00||  0.02| 99.98|  5475||  0.00|  0.03|  0.00| 99.95
   8|  0.00|  0.00|  0.00|  0.00||  1.40| 98.60|  5504||  0.01|  2.59|  0.22| 95.83
   9|  0.00|  0.00|  0.00|  0.00||  0.00|100.00|  5659||  0.00|  0.00|  0.00| 99.99
  10|  0.00|  5.22|  0.00|  0.00||  0.79| 99.21|  5532||  0.00|  1.04|  5.79| 92.41
  11|  0.00|  5.22|  0.00|  0.00||  0.02| 99.98|  5493||  0.00|  0.00|  0.00| 99.99
  12|  0.00|  2.56|  0.00|  0.00||  2.02| 97.98|  5483||  0.00|  0.65|  2.97| 94.38
  13|  0.00|  2.56|  0.00|  0.00||  0.00|100.00|  5270||  0.00|  0.00|  0.00| 100.0
  14|  0.00|  0.38|  0.00|  0.00||  0.86| 99.14|  5465||  0.00|  0.89|  1.36| 96.83
  15|  0.00|  0.38|  0.00|  0.00||  0.00|100.00|  5271||  0.00|  0.00|  0.00| 100.0
  16|  0.00| 96.64|  0.00|  0.00||  0.46| 99.54|  4291||  0.00|  0.21|  1.25| 98.10
  17|  0.00| 96.26|  0.00|  0.00||  0.35| 99.65|  4293||  0.00|  0.63|  1.48| 97.57
  18|  0.00| 97.14|  0.00|  0.00||  0.58| 99.42|  4291||  0.00|  0.28|  0.59| 98.57
  19|  0.00| 95.97|  0.00|  0.00||  0.68| 99.32|  4292||  0.00|  1.02|  2.44| 95.89
  20|  0.00| 96.45|  0.00|  0.00||  1.10| 98.90|  4292||  0.00|  0.35|  0.10| 98.47
  21|  0.00| 96.63|  0.00|  0.00||  0.72| 99.28|  4292||  0.00|  0.28|  0.00| 99.01
  22|  0.00| 96.61|  0.00|  0.00||  0.34| 99.66|  4292||  0.00|  0.56|  0.18| 98.94
  23|  0.00| 96.66|  0.00|  0.00||  0.40| 99.60|  4294||  0.01|  0.74|  0.24| 98.64
  24|  0.00| 97.44|  0.00|  0.00||  0.08| 99.92|  4293||  0.00|  0.08|  0.00| 99.86
  25|  0.00| 99.30|  0.00|  0.00||  0.67| 99.33|  4292||  0.00|  0.00|  0.00| 99.34
  26|  0.00| 96.53|  0.00|  0.00||  0.10| 99.90|  4293||  0.00|  0.86|  0.06| 98.99
  27|  0.00| 99.98|  0.00|  0.00||  0.01| 99.99|  4214||  0.00|  0.00|  0.07| 99.93
  28|  0.00| 99.98|  0.00|  0.00||  0.01| 99.99|  4433||  0.00|  0.00|  0.00|100.00
  29|  0.00| 99.57|  0.00|  0.00||  0.37| 99.63|  4293||  0.00|  0.00|  0.00| 99.63
  30|  0.00| 99.98|  0.00|  0.00||  0.00|100.00|  4370||  0.00|  0.00|  0.00| 100.0
  31|  0.00| 99.74|  0.00|  0.00||  0.22| 99.78|  4295||  0.00|  0.00|  0.00| 99.78
```
