# 总结一下 cpuid 问题

1. guest 中对于那些 cpuid 是无影响的？
  - 或者说，guest 中根本看不到的 cpuid，那么就不要管了
    - vmx guest 看不到?
2. 分析当前机器上的那些 cpuid，和 mibook 上的 cpuid 对比一下
3. 如何知道操作系统屏蔽了那些 cpuflag ?
  - boot_cpu_flags ?
  - static_cpu_flags ?
  - ???
3. 分析一下如果操作系统会怎么屏蔽一个 cpuflags 的?
4. cpuflags 那些具有依赖关系
5. 那些 cpuflags 其实 guest 中根本不重要？

## guest kernel
guest flags : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq vmx ssse3 fma cx16 pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_vnni arat umip pku ospke waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize arch_capabilities
host flags : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb intel_pt sha_ni xsaveopt xsavec xgetbv1 xsaves split_lock_detect avx_vnni dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi umip pku ospke waitpkg gfni vaes vpclmulqdq tme rdpid movdiri movdir64b fsrm md_clear serialize pconfig arch_lbr ibt flush_l1d arch_capabilities


会消失的 flags :
dts acpi tm pbe art pebs bts nonstop_tsc aperfmperf dtes64 monitor ds_cpl smx est tm2 sdbg xtpr epb intel_pt split_lock_detect dtherm ida pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi tme pconfig arch_lbr ibt flush_l1d

- [ ] 而且还可以增加 flags

arch/x86/kernel/cpu/common.c  中通过 cpuid 指令获取:
```c
void get_cpu_cap(struct cpuinfo_x86 *c)
```
