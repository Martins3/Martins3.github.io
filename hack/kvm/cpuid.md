
## Guest kernel 变少了
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq vmx ssse3 fma cx16 pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_vnni arat umip pku ospke waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize arch_capabilities
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb intel_pt sha_ni xsaveopt xsavec xgetbv1 xsaves split_lock_detect avx_vnni dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi umip pku ospke waitpkg gfni vaes vpclmulqdq tme rdpid movdiri movdir64b fsrm md_clear serialize pconfig arch_lbr ibt flush_l1d arch_capabilities


会消失的 flags :
dts acpi tm pbe art pebs bts nonstop_tsc aperfmperf dtes64 monitor ds_cpl smx est tm2 sdbg xtpr epb intel_pt split_lock_detect dtherm ida pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi tme pconfig arch_lbr ibt flush_l1d

- [ ] 而且还可以增加 flags

为什么 acpi feature 消失了?

arch/x86/kernel/cpu/common.c  中通过 cpuid 指令获取:
```c
void get_cpu_cap(struct cpuinfo_x86 *c)
```

[cpuid 必然导致 vmexit 的](https://stackoverflow.com/questions/63214415/does-hypervisor-like-kvm-need-to-vm-exit-on-cpuid)

## QEMU 是如何调用的
关键描述:
- kvm_arch_get_supported_cpuid

其调用者为:
- x86_cpu_filter_features
- x86_cpu_get_supported_feature_word
- x86_cpu_get_supported_cpuid
- cpu_x86_cpuid

- kvm_request_xsave_components

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