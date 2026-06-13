# 把内容划分一下

1. 特殊 flags
2. 如何解析 cpuid
  - cpuid 工具的使用
3. qemu kvm 如何重新构造 cpuid 的?
  例如cpuid -l 6 -r 在物理机和虚拟机中不同，即便是 -cpu host
4. 热迁移的时候，如何处理的?
5. 内核的初始化过程 ，例如 cpu_has 和 boot_cpu_has 之类的是如何实现的

## clearcpuid 到底是如何实现的?
又可以禁用 avx2，同时不会影响 cpuflags 的，也许尝试 disable 其他的 flags 试试

## plans
1. 理解 cpuid -r 1 的内容
2. 整理关键的 cpu flags
3. QEMU 和 kvm 如何模拟
4. msr
5. 从 cpuid 到 cpuinfo 的实现

5. 其实就是需要解决几个基本问题，为什么有时候 feature 会增加，有时候，feature 会减少。

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


# 分析那些 CPU flags 会操作系统操作

- 那些地方修改了这些玩意儿？

- show_cpuinfo : 这里
```c
	for (i = 0; i < 32*NCAPINTS; i++)
		if (cpu_has(c, i) && x86_cap_flags[i] != NULL)
			seq_printf(m, " %s", x86_cap_flags[i]);
```

set_cpu_cap

- get_cpu_cap 总的初始化位置
  - 三个位置上调用？而是是 AMD 机器？

- [ ]

- 如何理解什么是 cpuid_leafs

## 关键文档
### /home/martins3/core/linux/Documentation/x86/cpuinfo.rst

If the expected flag does not appear in /proc/cpuinfo, things are murkier.
Users need to find out the reason why the flag is missing and find the way
how to enable it, which is not always easy. There are several factors that
can explain missing flags:
1. the expected feature failed to enable
2. the feature is missing in hardware
3. platform firmware did not enable it
5. the feature is disabled at build or run time
6. an old kernel is in use, or the kernel does not support the feature and thus has not enabled it.


## /home/martins3/core/linux/Documentation/virt/kvm/x86/cpuid.rst

## tools/arch/x86/kcpuid

来调用 cpuid 指令来获取 cpuid 的:

shows features which the kernel supports. For a full list of CPUID flags
which the CPU supports, use tools/arch/x86/kcpuid.

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

## clearcpuid
通过这个评论可以知道，
https://stackoverflow.com/questions/13965178/how-do-i-disable-avx-instructions-on-a-linux-computer

clearcpuid=156 来屏蔽 flags ，但是观察其实现，并没有什么特别的

的确是 invalid code 的错误，在内核和用户态都可以验证到:
```txt
[   22.507434] traps: wc[5360] trap invalid opcode ip:4d6b4b sp:7ffc29d5aaa0 error:0 in coreutils[408000+cf000]
[   28.486690] input: WH-1000XM3 (AVRCP) as /devices/virtual/input/input27
[   48.593359] traps: .epiphany-searc[5754] trap invalid opcode ip:7f8d0312b334 sp:7fffce48c438 error:0 in libatomic.so.1.2.0[7f8d03129000+3000]
[   48.632515] traps: .gnome-photos-w[5752] trap invalid opcode ip:7fa57966dde9 sp:7fff3c182808 error:0 in x86-64-v3-CIE.so[7fa579668000+8000]
[   48.737020] traps: .org.gnome.Char[5746] trap invalid opcode ip:7f615e4a5910 sp:7ffcaff140e8 error:0 in libgtk-4.so.1.800.2[7f615e0b4000+3f2000]
[   79.039518] traps: .epiphany-searc[7307] trap invalid opcode ip:7efd81803334 sp:7ffdfb8e6ad8 error:0 in libatomic.so.1.2.0[7efd81801000+3000]
[   79.087522] traps: .gnome-photos-w[7305] trap invalid opcode ip:7f99ce9b6de9 sp:7fffbebf8d08 error:0 in x86-64-v3-CIE.so[7f99ce9b1000+8000]
[   79.287229] traps: .org.gnome.Char[7299] trap invalid opcode ip:7f60296a5910 sp:7ffc27cecd38 error:0 in libgtk-4.so.1.800.2[7f60292b4000+3f2000]
[   79.294773] traps: .gnome-photos-w[7721] trap invalid opcode ip:7ff0618fbde9 sp:7fffb32fe118 error:0 in x86-64-v3-CIE.so[7ff0618f6000+8000]
[  116.673961] traps: wc[8899] trap invalid opcode ip:4d6b4b sp:7ffe671832c0 error:0 in coreutils[408000+cf000]
[  116.720285] traps: wc[8937] trap invalid opcode ip:4d6b4b sp:7ffd931c7820 error:0 in coreutils[408000+cf000]
[  116.769937] traps: wc[8990] trap invalid opcode ip:4d6b4b sp:7ffe8de0f480 error:0 in coreutils[408000+cf000]
[  116.796237] traps: wc[9122] trap invalid opcode ip:4d6b4b sp:7fffa70e69c0 error:0 in coreutils[408000+cf000]
[  129.641439] traps: 01-hello[9994] trap invalid opcode ip:401140 sp:7ffea850b140 error:0 in 01-hello[401000+1000]
[  140.471273] traps: .epiphany-searc[10105] trap invalid opcode ip:7f25f32d3334 sp:7fff84375ea8 error:0 in libatomic.so.1.2.0[7f25f32d1000+3000]
[  140.513184] traps: .gnome-photos-w[10103] trap invalid opcode ip:7fb1f485bde9 sp:7ffe944f3b18 error:0 in x86-64-v3-CIE.so[7fb1f4856000+8000]
[  140.729860] traps: .org.gnome.Char[10097] trap invalid opcode ip:7fa7424a5910 sp:7ffda737bcf8 error:0 in libgtk-4.so.1.800.2[7fa7420b4000+3f2000]
[  140.737712] traps: .gnome-photos-w[10476] trap invalid opcode ip:7f672d9aede9 sp:7ffd811f0818 error:0 in x86-64-v3-CIE.so[7f672d9a9000+8000]
[  341.282755] wlo1: Connection to AP 78:57:73:58:c8:30 lost
[  342.378856] wlo1: authenticate with 78:57:73:4d:5e:10
[  342.384498] wlo1: send auth to 78:57:73:4d:5e:10 (try 1/3)
[  342.410897] wlo1: authenticated
[  342.411937] wlo1: associate with 78:57:73:4d:5e:10 (try 1/3)
[  342.416201] wlo1: RX AssocResp from 78:57:73:4d:5e:10 (capab=0x1511 status=0 aid=5)
[  342.421935] wlo1: associated
[  342.470154] wlo1: Limiting TX power to 20 (23 - 3) dBm as advertised by 78:57:73:4d:5e:10
[ 1041.424378] traps: wc[20526] trap invalid opcode ip:4dee4b sp:7ffe1514cc60 error:0 in coreutils[407000+d9000]
[ 1073.208551] traps: wc[22373] trap invalid opcode ip:4d6b4b sp:7ffdb619ecc0 error:0 in coreutils[408000+cf000]
[ 1124.146359] traps: wc[23303] trap invalid opcode ip:4d6b4b sp:7ffe37ab8960 error:0 in coreutils[408000+cf000]
[ 1133.657698] traps: wc[24218] trap invalid opcode ip:4dee4b sp:7fffcdcaef40 error:0 in coreutils[407000+d9000]
[ 1139.262500] traps: 01-hello[24450] trap invalid opcode ip:401544 sp:7fff203c6a40 error:0 in 01-hello[401000+8c000]
[ 1195.931841] traps: wc[24863] trap invalid opcode ip:4d6b4b sp:7ffcca8f5060 error:0 in coreutils[408000+cf000]
[ 1375.392674] traps: 01-hello[25643] trap invalid opcode ip:40154c sp:7ffec69d1ae0 error:0 in 01-hello[401000+8c000]
[ 1404.288893] traps: 01-hello[25782] trap invalid opcode ip:40154c sp:7fff3d9592c0 error:0 in 01-hello[401000+8c000]
[ 1478.821069] traps: 01-hello[26143] trap invalid opcode ip:40154c sp:7ffed98bcda0 error:0 in 01-hello[401000+8c000]
```

## 但是通过 AVX 得到的是 2

```c
#define X86_FEATURE_AVX			( 4*32+28) /* Advanced Vector Extensions */
```

## 为什么 bios 可以控制 os 的  cpuid 输出

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
