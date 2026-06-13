# kvm feautres
```txt
CPUID 40000000:00 = 40000001 4b4d564b 564b4d56 0000004d | ...@KVMKVMKVM...
CPUID 40000001:00 = 01007afb 00000000 00000000 00000000 | .z..............
```
在 tools/testing/selftests/kvm/include/x86_64/processor.h 可以找到:

```c
/* This CPUID returns two feature bitmaps in eax, edx. Before enabling
 * a particular paravirtualization, the appropriate feature bit should
 * be checked in eax. The performance hint feature bit should be checked
 * in edx.
 */
#define KVM_CPUID_FEATURES	0x40000001
#define KVM_FEATURE_CLOCKSOURCE		0
#define KVM_FEATURE_NOP_IO_DELAY	1
#define KVM_FEATURE_MMU_OP		2
/* This indicates that the new set of kvmclock msrs
 * are available. The use of 0x11 and 0x12 is deprecated
 */
#define KVM_FEATURE_CLOCKSOURCE2        3
#define KVM_FEATURE_ASYNC_PF		4
#define KVM_FEATURE_STEAL_TIME		5
#define KVM_FEATURE_PV_EOI		6
#define KVM_FEATURE_PV_UNHALT		7
#define KVM_FEATURE_PV_TLB_FLUSH	9
#define KVM_FEATURE_ASYNC_PF_VMEXIT	10
#define KVM_FEATURE_PV_SEND_IPI	11
#define KVM_FEATURE_POLL_CONTROL	12
#define KVM_FEATURE_PV_SCHED_YIELD	13
#define KVM_FEATURE_ASYNC_PF_INT	14
#define KVM_FEATURE_MSI_EXT_DEST_ID	15
#define KVM_FEATURE_HC_MAP_GPA_RANGE	16
#define KVM_FEATURE_MIGRATION_CONTROL	17
```

在虚拟机中执行 cpuid
```txt
   hypervisor features (0x40000001/eax):
      kvmclock available at MSR 0x11           = true
      delays unnecessary for PIO ops           = true
      mmu_op                                   = false
      kvmclock available at MSR 0x4b564d00     = true
      async pf enable available by MSR         = true
      steal clock supported                    = true
      guest EOI optimization enabled           = true
      guest spinlock optimization enabled      = true
      guest TLB flush optimization enabled     = true
      async PF VM exit enable available by MSR = true
      guest send IPI optimization enabled      = true
      host HLT poll disable at MSR 0x4b564d05  = true
      guest sched yield optimization enabled   = true
      guest uses intrs for page ready APF evs  = true
      extended destination ID                  = false
      map gpa range hypercall supported        = false
      MSR_KVM_MIGRATION_CONTROL supported      = false
      stable: no guest per-cpu warps expected  = true
   hypervisor features (0x40000001/edx):
      realtime hint: no unbound preemption = false
   hypervisor_id (0x40000100) = "\0\0\0\0\0\0\0\0\0\0\0\0"
```

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
