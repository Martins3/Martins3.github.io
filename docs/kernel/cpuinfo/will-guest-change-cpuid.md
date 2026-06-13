# 测试 guest 是否修改 cpuid

不会修改
## 反正是切换内核之后，这个是会修改的


```diff
🤒  diff host-5.15.txt docs/kernel/cpuinfo/material/3.10.txt
3c3
- CPUID 00000001:00 = 000b0671 011f0800 fffab223 1f8bfbff | q.......#.......
---
+ CPUID 00000001:00 = 000b0671 011f0800 fffa3223 1f8bfbff | q.......#2......
17c17
- CPUID 0000000a:00 = 07300602 00000000 00000000 00008603 | ..0.............
---
+ CPUID 0000000a:00 = 00000000 00000000 00000000 00000000 | ................
```


```txt
 3.10.txt
 4.18.txt
 4.19.txt
 6.3.txt : guest 内核是 6.3 的时候
 host-5.15.txt : host 内核是 5.15 的时候
 replace-6.3.txt : host 内核是 6.2 但是才用的是 replace 内核的方式
```

总体来说，符合预期，因为提供的是硬件基础。

### 第一个差别 : pdcm
CPUID 00000001:00 中存在

但是
```txt
In [3]: format(0xfffab223, '#032b') 0b11111111111110101011001000100011
In [3]: format(0xfffa3223, '#032b') 0b11111111111110100011001000100011
```

6.3 的 guest :
```txt
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc rep_good no
pl xtopology cpuid tsc_known_freq pni pclmulqdq vmx ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3
dnowprefetch cpuid_fault invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rd
seed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_vnni arat umip pku ospke waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize a
rch_capabilities
```

5.15 中的 cpuid :
```txt
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmo
n rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq vmx ssse3 fma cx16 pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hyperv
isor lahf_lm abm 3dnowprefetch cpuid_fault invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bm
i2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 xsaves avx_vnni arat umip pku ospke waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md
_clear serialize arch_capabilities
```
- pdcm 确实是存在差别的。

似乎是这个 patch 导致的:
```diff
History:        #0
Commit:         27461da31089ace6966e4f1695cba7eb87ffbe4f
Author:         Like Xu <like.xu@linux.intel.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Fri 29 May 2020 03:43:45 PM CST
Committer Date: Mon 01 Jun 2020 04:26:09 PM CST

KVM: x86/pmu: Support full width counting

Intel CPUs have a new alternative MSR range (starting from MSR_IA32_PMC0)
for GP counters that allows writing the full counter width. Enable this
range from a new capability bit (IA32_PERF_CAPABILITIES.FW_WRITE[bit 13]).

The guest would query CPUID to get the counter width, and sign extends
the counter values as needed. The traditional MSRs always limit to 32bit,
even though the counter internally is larger (48 or 57 bits).

When the new capability is set, use the alternative range which do not
have these restrictions. This lowers the overhead of perf stat slightly
because it has to do less interrupts to accumulate the counter value.

Signed-off-by: Like Xu <like.xu@linux.intel.com>
Message-Id: <20200529074347.124619-3-like.xu@linux.intel.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>

diff --git a/arch/x86/kvm/cpuid.c b/arch/x86/kvm/cpuid.c
index a9f1905896fb..253b8e875ccd 100644
--- a/arch/x86/kvm/cpuid.c
+++ b/arch/x86/kvm/cpuid.c
@@ -296,7 +296,7 @@ void kvm_set_cpu_caps(void)
 		F(XMM3) | F(PCLMULQDQ) | 0 /* DTES64, MONITOR */ |
 		0 /* DS-CPL, VMX, SMX, EST */ |
 		0 /* TM2 */ | F(SSSE3) | 0 /* CNXT-ID */ | 0 /* Reserved */ |
-		F(FMA) | F(CX16) | 0 /* xTPR Update, PDCM */ |
+		F(FMA) | F(CX16) | 0 /* xTPR Update */ | F(PDCM) |
 		F(PCID) | 0 /* Reserved, DCA */ | F(XMM4_1) |
 		F(XMM4_2) | F(X2APIC) | F(MOVBE) | F(POPCNT) |
 		0 /* Reserved*/ | F(AES) | F(XSAVE) | 0 /* OSXSAVE */ | F(AVX) |
```

#### [ ] 等待完全的验证，等待新的内核模块装上吧

## 第二个差别

似乎现在的直接去掉了
