# spectre

## [ ] 在编译上，有什么技术来防护

## [ ] kernel cmdline 上如何
- noibrs

## cpuid : spec_ctrl

QEMU 中是存在相关的文档描述 qemu/docs/system/cpu-models-x86.rst.inc

在 /home/martins3/core/linux/arch/x86/kernel/cpu/capflags.c 描述了所有的 flags ，其中有
`v_spec_ctrl`

但是 centos 对于这个问题没有显示!

- [ ] centos 是如何展示这个的?
```c
	[X86_FEATURE_SPEC_CTRL]		 = "spec_ctrl",
```

这个都是有的
```txt
#define X86_FEATURE_MSR_SPEC_CTRL	( 7*32+16) /* "" MSR SPEC_CTRL is implemented */
```

- [ ] qemu 是如何检测的 cpuflags 的？

- [ ]
```c
const char * const x86_cap_flags[NCAPINTS*32] = {
```
和 cpuid 是什么关系？

应该都是存在的。

## [ ] firmeware 为什么可以用来防护

## [LWN：利用静态调用来避免 retpoline](https://mp.weixin.qq.com/s?__biz=Mzg2MjE0NDE5OA==&mid=2247484455&idx=1&sn=3ce685da00fb31579c08ce585bfda135)

> 间接调用 indirect call 发生在编译时不知道要调用的函数的地址的情况，需要将该地址存储在一个指针变量中，在运行时使用。事实证明，这些间接调用很容易被 speculative-executation 攻击方式所利用。Retpolines 通过将间接调用转化为一个相当复杂（而且开销很大）的代码序列（code sequence）来防御这些攻击，使其无法被投机地（speculatively）执行。

## [Retpoline](https://support.google.com/faqs/answer/7625886)

## 用户态来处理
- [Speculation Control](https://docs.kernel.org/userspace-api/spec_ctrl.html)

```c
/* Called from seccomp/prctl update */
void speculation_ctrl_update_current(void)
{
	preempt_disable();
	speculation_ctrl_update(speculation_ctrl_update_tif(current));
	preempt_enable();
}
```

最后更新到这个 msr 寄存器中:
```c
#define MSR_IA32_SPEC_CTRL		0x00000048 /* Speculation Control */
#define SPEC_CTRL_IBRS			BIT(0)	   /* Indirect Branch Restricted Speculation */
#define SPEC_CTRL_STIBP_SHIFT		1	   /* Single Thread Indirect Branch Predictor (STIBP) bit */
#define SPEC_CTRL_STIBP			BIT(SPEC_CTRL_STIBP_SHIFT)	/* STIBP mask */
#define SPEC_CTRL_SSBD_SHIFT		2	   /* Speculative Store Bypass Disable bit */
#define SPEC_CTRL_SSBD			BIT(SPEC_CTRL_SSBD_SHIFT)	/* Speculative Store Bypass Disable */
```

## SSB
- https://msrc-blog.microsoft.com/2018/05/21/analysis-and-mitigation-of-speculative-store-bypass-cve-2018-3639/

## IBRS

## IPBP

## spectre 内核总结
- https://docs.kernel.org/admin-guide/hw-vuln/spectre.html
  - 最后的链接也可以看下

分别看看这些是啥作用?
```c
    "noibpb"
    "nopti"
    "nospectre_v2"
    "nospectre_v1"
    "l1tf=off"
    "nospec_store_bypass_disable"
    "no_stf_barrier"
    "mds=off"
    "tsx=on"
    "tsx_async_abort=off"
    "mitigations=off"
```

## 这个 patch 我都看不懂啊
```diff
History:        #0
Commit:         2961298efe1ea1b6fc0d7ee8b76018fa6c0bcef2
Author:         David Woodhouse <dwmw@amazon.co.uk>
Committer:      Thomas Gleixner <tglx@linutronix.de>
Author Date:    Sun 28 Jan 2018 12:24:32 AM CST
Committer Date: Sun 28 Jan 2018 02:10:44 AM CST

x86/cpufeatures: Clean up Spectre v2 related CPUID flags

We want to expose the hardware features simply in /proc/cpuinfo as "ibrs",
"ibpb" and "stibp". Since AMD has separate CPUID bits for those, use them
as the user-visible bits.

When the Intel SPEC_CTRL bit is set which indicates both IBRS and IBPB
capability, set those (AMD) bits accordingly. Likewise if the Intel STIBP
bit is set, set the AMD STIBP that's used for the generic hardware
capability.

Hide the rest from /proc/cpuinfo by putting "" in the comments. Including
RETPOLINE and RETPOLINE_AMD which shouldn't be visible there. There are
patches to make the sysfs vulnerabilities information non-readable by
non-root, and the same should apply to all information about which
mitigations are actually in use. Those *shouldn't* appear in /proc/cpuinfo.

The feature bit for whether IBPB is actually used, which is needed for
ALTERNATIVEs, is renamed to X86_FEATURE_USE_IBPB.

Originally-by: Borislav Petkov <bp@suse.de>
Signed-off-by: David Woodhouse <dwmw@amazon.co.uk>
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
Cc: ak@linux.intel.com
Cc: dave.hansen@intel.com
Cc: karahmed@amazon.de
Cc: arjan@linux.intel.com
Cc: torvalds@linux-foundation.org
Cc: peterz@infradead.org
Cc: bp@alien8.de
Cc: pbonzini@redhat.com
Cc: tim.c.chen@linux.intel.com
Cc: gregkh@linux-foundation.org
Link: https://lkml.kernel.org/r/1517070274-12128-2-git-send-email-dwmw@amazon.co.uk

diff --git a/arch/x86/include/asm/cpufeatures.h b/arch/x86/include/asm/cpufeatures.h
index 07934b2f8df2..73b5fff159a4 100644
--- a/arch/x86/include/asm/cpufeatures.h
+++ b/arch/x86/include/asm/cpufeatures.h
@@ -203,14 +203,14 @@
 #define X86_FEATURE_PROC_FEEDBACK	( 7*32+ 9) /* AMD ProcFeedbackInterface */
 #define X86_FEATURE_SME			( 7*32+10) /* AMD Secure Memory Encryption */
 #define X86_FEATURE_PTI			( 7*32+11) /* Kernel Page Table Isolation enabled */
-#define X86_FEATURE_RETPOLINE		( 7*32+12) /* Generic Retpoline mitigation for Spectre variant 2 */
-#define X86_FEATURE_RETPOLINE_AMD	( 7*32+13) /* AMD Retpoline mitigation for Spectre variant 2 */
+#define X86_FEATURE_RETPOLINE		( 7*32+12) /* "" Generic Retpoline mitigation for Spectre variant 2 */
+#define X86_FEATURE_RETPOLINE_AMD	( 7*32+13) /* "" AMD Retpoline mitigation for Spectre variant 2 */
 #define X86_FEATURE_INTEL_PPIN		( 7*32+14) /* Intel Processor Inventory Number */

 #define X86_FEATURE_MBA			( 7*32+18) /* Memory Bandwidth Allocation */
-#define X86_FEATURE_RSB_CTXSW		( 7*32+19) /* Fill RSB on context switches */
+#define X86_FEATURE_RSB_CTXSW		( 7*32+19) /* "" Fill RSB on context switches */

-#define X86_FEATURE_IBPB		( 7*32+21) /* Indirect Branch Prediction Barrier enabled*/
+#define X86_FEATURE_USE_IBPB		( 7*32+21) /* "" Indirect Branch Prediction Barrier enabled */

 /* Virtualization flags: Linux defined, word 8 */
 #define X86_FEATURE_TPR_SHADOW		( 8*32+ 0) /* Intel TPR Shadow */
@@ -271,9 +271,9 @@
 #define X86_FEATURE_CLZERO		(13*32+ 0) /* CLZERO instruction */
 #define X86_FEATURE_IRPERF		(13*32+ 1) /* Instructions Retired Count */
 #define X86_FEATURE_XSAVEERPTR		(13*32+ 2) /* Always save/restore FP error pointers */
-#define X86_FEATURE_AMD_PRED_CMD	(13*32+12) /* Prediction Command MSR (AMD) */
-#define X86_FEATURE_AMD_SPEC_CTRL	(13*32+14) /* Speculation Control MSR only (AMD) */
-#define X86_FEATURE_AMD_STIBP		(13*32+15) /* Single Thread Indirect Branch Predictors (AMD) */
+#define X86_FEATURE_IBPB		(13*32+12) /* Indirect Branch Prediction Barrier */
+#define X86_FEATURE_IBRS		(13*32+14) /* Indirect Branch Restricted Speculation */
+#define X86_FEATURE_STIBP		(13*32+15) /* Single Thread Indirect Branch Predictors */

 /* Thermal and Power Management Leaf, CPUID level 0x00000006 (EAX), word 14 */
 #define X86_FEATURE_DTHERM		(14*32+ 0) /* Digital Thermal Sensor */
@@ -325,8 +325,8 @@
 /* Intel-defined CPU features, CPUID level 0x00000007:0 (EDX), word 18 */
 #define X86_FEATURE_AVX512_4VNNIW	(18*32+ 2) /* AVX-512 Neural Network Instructions */
 #define X86_FEATURE_AVX512_4FMAPS	(18*32+ 3) /* AVX-512 Multiply Accumulation Single precision */
-#define X86_FEATURE_SPEC_CTRL		(18*32+26) /* Speculation Control (IBRS + IBPB) */
-#define X86_FEATURE_STIBP		(18*32+27) /* Single Thread Indirect Branch Predictors */
+#define X86_FEATURE_SPEC_CTRL		(18*32+26) /* "" Speculation Control (IBRS + IBPB) */
+#define X86_FEATURE_INTEL_STIBP		(18*32+27) /* "" Single Thread Indirect Branch Predictors */
 #define X86_FEATURE_ARCH_CAPABILITIES	(18*32+29) /* IA32_ARCH_CAPABILITIES MSR (Intel) */

 /*
```
## 又一个隐藏机制

```diff
History:        #0
Commit:         52817587e706686fcdb27f14c1b000c92f266c96
Author:         Thomas Gleixner <tglx@linutronix.de>
Author Date:    Fri 11 May 2018 02:21:36 AM CST
Committer Date: Thu 17 May 2018 11:09:17 PM CST

x86/cpufeatures: Disentangle SSBD enumeration

The SSBD enumeration is similarly to the other bits magically shared
between Intel and AMD though the mechanisms are different.

Make X86_FEATURE_SSBD synthetic and set it depending on the vendor specific
features or family dependent setup.

Change the Intel bit to X86_FEATURE_SPEC_CTRL_SSBD to denote that SSBD is
controlled via MSR_SPEC_CTRL and fix up the usage sites.

Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
Reviewed-by: Borislav Petkov <bp@suse.de>
Reviewed-by: Konrad Rzeszutek Wilk <konrad.wilk@oracle.com>

diff --git a/arch/x86/include/asm/cpufeatures.h b/arch/x86/include/asm/cpufeatures.h
index 7d34eb0d3715..61c34c1a525c 100644
--- a/arch/x86/include/asm/cpufeatures.h
+++ b/arch/x86/include/asm/cpufeatures.h
@@ -207,15 +207,14 @@
 #define X86_FEATURE_INTEL_PPIN		( 7*32+14) /* Intel Processor Inventory Number */
 #define X86_FEATURE_CDP_L2		( 7*32+15) /* Code and Data Prioritization L2 */
 #define X86_FEATURE_MSR_SPEC_CTRL	( 7*32+16) /* "" MSR SPEC_CTRL is implemented */
-
+#define X86_FEATURE_SSBD		( 7*32+17) /* Speculative Store Bypass Disable */
 #define X86_FEATURE_MBA			( 7*32+18) /* Memory Bandwidth Allocation */
 #define X86_FEATURE_RSB_CTXSW		( 7*32+19) /* "" Fill RSB on context switches */
 #define X86_FEATURE_SEV			( 7*32+20) /* AMD Secure Encrypted Virtualization */
-
 #define X86_FEATURE_USE_IBPB		( 7*32+21) /* "" Indirect Branch Prediction Barrier enabled */
 #define X86_FEATURE_USE_IBRS_FW		( 7*32+22) /* "" Use IBRS during runtime firmware calls */
 #define X86_FEATURE_SPEC_STORE_BYPASS_DISABLE	( 7*32+23) /* "" Disable Speculative Store Bypass. */
-#define X86_FEATURE_AMD_SSBD		( 7*32+24)  /* "" AMD SSBD implementation */
+#define X86_FEATURE_LS_CFG_SSBD		( 7*32+24)  /* "" AMD SSBD implementation via LS_CFG MSR */
 #define X86_FEATURE_IBRS		( 7*32+25) /* Indirect Branch Restricted Speculation */
 #define X86_FEATURE_IBPB		( 7*32+26) /* Indirect Branch Prediction Barrier */
 #define X86_FEATURE_STIBP		( 7*32+27) /* Single Thread Indirect Branch Predictors */
@@ -339,7 +338,7 @@
 #define X86_FEATURE_SPEC_CTRL		(18*32+26) /* "" Speculation Control (IBRS + IBPB) */
 #define X86_FEATURE_INTEL_STIBP		(18*32+27) /* "" Single Thread Indirect Branch Predictors */
 #define X86_FEATURE_ARCH_CAPABILITIES	(18*32+29) /* IA32_ARCH_CAPABILITIES MSR (Intel) */
-#define X86_FEATURE_SSBD		(18*32+31) /* Speculative Store Bypass Disable */
+#define X86_FEATURE_SPEC_CTRL_SSBD	(18*32+31) /* "" Speculative Store Bypass Disable */

 /*
  * BUG word(s)

```

## 这个 patch 也是的
```diff
History:        #0
Commit:         7e5b3c267d256822407a22fdce6afdf9cd13f9fb
Author:         Mark Gross <mgross@linux.intel.com>
Committer:      Thomas Gleixner <tglx@linutronix.de>
Author Date:    Thu 16 Apr 2020 11:54:04 PM CST
Committer Date: Mon 20 Apr 2020 06:19:22 PM CST

x86/speculation: Add Special Register Buffer Data Sampling (SRBDS) mitigation

SRBDS is an MDS-like speculative side channel that can leak bits from the
random number generator (RNG) across cores and threads. New microcode
serializes the processor access during the execution of RDRAND and
RDSEED. This ensures that the shared buffer is overwritten before it is
released for reuse.

While it is present on all affected CPU models, the microcode mitigation
is not needed on models that enumerate ARCH_CAPABILITIES[MDS_NO] in the
cases where TSX is not supported or has been disabled with TSX_CTRL.

The mitigation is activated by default on affected processors and it
increases latency for RDRAND and RDSEED instructions. Among other
effects this will reduce throughput from /dev/urandom.

* Enable administrator to configure the mitigation off when desired using
  either mitigations=off or srbds=off.

* Export vulnerability status via sysfs

* Rename file-scoped macros to apply for non-whitelist table initializations.

 [ bp: Massage,
   - s/VULNBL_INTEL_STEPPING/VULNBL_INTEL_STEPPINGS/g,
   - do not read arch cap MSR a second time in tsx_fused_off() - just pass it in,
   - flip check in cpu_set_bug_bits() to save an indentation level,
   - reflow comments.
   jpoimboe: s/Mitigated/Mitigation/ in user-visible strings
   tglx: Dropped the fused off magic for now
 ]

Signed-off-by: Mark Gross <mgross@linux.intel.com>
Signed-off-by: Borislav Petkov <bp@suse.de>
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
Reviewed-by: Tony Luck <tony.luck@intel.com>
Reviewed-by: Pawan Gupta <pawan.kumar.gupta@linux.intel.com>
Reviewed-by: Josh Poimboeuf <jpoimboe@redhat.com>
Tested-by: Neelima Krishnan <neelima.krishnan@intel.com>

diff --git a/arch/x86/include/asm/cpufeatures.h b/arch/x86/include/asm/cpufeatures.h
index db189945e9b0..02dabc9e77b0 100644
--- a/arch/x86/include/asm/cpufeatures.h
+++ b/arch/x86/include/asm/cpufeatures.h
@@ -362,6 +362,7 @@
 #define X86_FEATURE_AVX512_4FMAPS	(18*32+ 3) /* AVX-512 Multiply Accumulation Single precision */
 #define X86_FEATURE_FSRM		(18*32+ 4) /* Fast Short Rep Mov */
 #define X86_FEATURE_AVX512_VP2INTERSECT (18*32+ 8) /* AVX-512 Intersect for D/Q */
+#define X86_FEATURE_SRBDS_CTRL		(18*32+ 9) /* "" SRBDS mitigation MSR available */
 #define X86_FEATURE_MD_CLEAR		(18*32+10) /* VERW clears CPU buffers */
 #define X86_FEATURE_TSX_FORCE_ABORT	(18*32+13) /* "" TSX_FORCE_ABORT */
 #define X86_FEATURE_PCONFIG		(18*32+18) /* Intel PCONFIG */
@@ -407,5 +408,6 @@
 #define X86_BUG_SWAPGS			X86_BUG(21) /* CPU is affected by speculation through SWAPGS */
 #define X86_BUG_TAA			X86_BUG(22) /* CPU is affected by TSX Async Abort(TAA) */
 #define X86_BUG_ITLB_MULTIHIT		X86_BUG(23) /* CPU may incur MCE during certain page attribute changes */
+#define X86_BUG_SRBDS			X86_BUG(24) /* CPU may leak RNG bits if not mitigated */

 #endif /* _ASM_X86_CPUFEATURES_H */
```

## 如何在 bios 中关闭 spectre
- 为什么 spectre 的有些选项是必须在 bios 中设置的

这是应该是关闭硬件支持的，最好不要如此。

## 如何
https://unix.stackexchange.com/questions/554908/disable-spectre-and-meltdown-mitigations

## 观察下补丁的状态
3.10 内核直通:
```txt
➜  ~ grep . /sys/devices/system/cpu/vulnerabilities/*
/sys/devices/system/cpu/vulnerabilities/itlb_multihit:Not affected
/sys/devices/system/cpu/vulnerabilities/l1tf:Not affected
/sys/devices/system/cpu/vulnerabilities/mds:Not affected
/sys/devices/system/cpu/vulnerabilities/meltdown:Not affected
/sys/devices/system/cpu/vulnerabilities/spec_store_bypass:Mitigation: Speculative Store Bypass disabled via prctl and seccomp
/sys/devices/system/cpu/vulnerabilities/spectre_v1:Mitigation: Load fences, usercopy/swapgs barriers and __user pointer sanitization
/sys/devices/system/cpu/vulnerabilities/spectre_v2:Mitigation: Enhanced IBRS, IBPB
/sys/devices/system/cpu/vulnerabilities/srbds:Not affected
/sys/devices/system/cpu/vulnerabilities/tsx_async_abort:Not affected
```

6.5 内核直通:
```txt
➜  ~ grep . /sys/devices/system/cpu/vulnerabilities/*

/sys/devices/system/cpu/vulnerabilities/gather_data_sampling:Not affected
/sys/devices/system/cpu/vulnerabilities/itlb_multihit:Not affected
/sys/devices/system/cpu/vulnerabilities/l1tf:Not affected
/sys/devices/system/cpu/vulnerabilities/mds:Not affected
/sys/devices/system/cpu/vulnerabilities/meltdown:Not affected
/sys/devices/system/cpu/vulnerabilities/mmio_stale_data:Not affected
/sys/devices/system/cpu/vulnerabilities/retbleed:Not affected
/sys/devices/system/cpu/vulnerabilities/spec_rstack_overflow:Not affected
/sys/devices/system/cpu/vulnerabilities/spec_store_bypass:Mitigation: Speculative Store Bypass disabled via prctl
/sys/devices/system/cpu/vulnerabilities/spectre_v1:Mitigation: usercopy/swapgs barriers and __user pointer sanitization
/sys/devices/system/cpu/vulnerabilities/spectre_v2:Mitigation: Enhanced / Automatic IBRS, IBPB: conditional, RSB filling, PBRSB-eIBRS: SW sequence
/sys/devices/system/cpu/vulnerabilities/srbds:Not affected
/sys/devices/system/cpu/vulnerabilities/tsx_async_abort:Not affected
```

6.5 Broadwell
```txt
/sys/devices/system/cpu/vulnerabilities/gather_data_sampling:Not affected
/sys/devices/system/cpu/vulnerabilities/itlb_multihit:KVM: Mitigation: VMX unsupported
/sys/devices/system/cpu/vulnerabilities/l1tf:Mitigation: PTE Inversion
/sys/devices/system/cpu/vulnerabilities/mds:Vulnerable: Clear CPU buffers attempted, no microcode; SMT Host state unknown
/sys/devices/system/cpu/vulnerabilities/meltdown:Mitigation: PTI
/sys/devices/system/cpu/vulnerabilities/mmio_stale_data:Unknown: No mitigations
/sys/devices/system/cpu/vulnerabilities/retbleed:Not affected
/sys/devices/system/cpu/vulnerabilities/spec_rstack_overflow:Not affected
/sys/devices/system/cpu/vulnerabilities/spec_store_bypass:Vulnerable
/sys/devices/system/cpu/vulnerabilities/spectre_v1:Mitigation: usercopy/swapgs barriers and __user pointer sanitization
/sys/devices/system/cpu/vulnerabilities/spectre_v2:Mitigation: Retpolines, IBPB: conditional, IBRS_FW, STIBP: disabled, RSB filling, PBRSB-eIBRS: Not affected
/sys/devices/system/cpu/vulnerabilities/srbds:Unknown: Dependent on hypervisor status
/sys/devices/system/cpu/vulnerabilities/tsx_async_abort:Not affected
```

https://unix.stackexchange.com/questions/554908/disable-spectre-and-meltdown-mitigations
- https://news.ycombinator.com/item?id=27559916

- https://www.phoronix.com/news/Linux-Default-Mitigations-Off
  - 编译时期关闭

https://news.ycombinator.com/item?id=25663729


amd 7950hx
```txt
/sys/devices/system/cpu/vulnerabilities/gather_data_sampling:Not affected
/sys/devices/system/cpu/vulnerabilities/itlb_multihit:Not affected
/sys/devices/system/cpu/vulnerabilities/l1tf:Not affected
/sys/devices/system/cpu/vulnerabilities/mds:Not affected
/sys/devices/system/cpu/vulnerabilities/meltdown:Not affected
/sys/devices/system/cpu/vulnerabilities/mmio_stale_data:Not affected
/sys/devices/system/cpu/vulnerabilities/retbleed:Not affected
/sys/devices/system/cpu/vulnerabilities/spec_rstack_overflow:Mitigation: safe RET, no microcode
/sys/devices/system/cpu/vulnerabilities/spec_store_bypass:Mitigation: Speculative Store Bypass disabled via prctl
/sys/devices/system/cpu/vulnerabilities/spectre_v1:Mitigation: usercopy/swapgs barriers and __user pointer sanitization
/sys/devices/system/cpu/vulnerabilities/spectre_v2:Mitigation: Enhanced / Automatic IBRS, IBPB: conditional, STIBP: always-on, RSB filling, PBRSB-eIBRS: Not affected
/sys/devices/system/cpu/vulnerabilities/srbds:Not affected
/sys/devices/system/cpu/vulnerabilities/tsx_async_abort:Not affected
```

## 扩展阅读
https://news.ycombinator.com/item?id=37812556

https://grsecurity.net/amd_branch_mispredictor_part_2_where_no_cpu_has_gone_before


https://unix.stackexchange.com/questions/554908/disable-spectre-and-meltdown-mitigations

https://spectrum.ieee.org/goodbye-motherboard-hello-siliconinterconnect-fabric#toggle-gdpr


## 如何理解
```c
INDIRECT_CALLABLE_DECLARE(int udp_rcv(struct sk_buff *));
INDIRECT_CALLABLE_DECLARE(int tcp_v4_rcv(struct sk_buff *));
```

似乎网络 和 kvm 都有不同的想法。

## kimi : arch/arm64/kernel/proton-pack.c 是做什么的?
(2026-06-24 这个分析相当的清晰了)

arch/arm64/kernel/proton-pack.c 是 ARM64 Linux 内核里处理 Spectre 系列推测执行漏洞 的核心文件。名字
"proton-pack"（《捉鬼敢死队》里的质子背包）来自文件注释里那句 "If there's something strange in your neighbourhood, who you gonna
call?"——有鬼（漏洞）就call它。

它主要负责 Spectre v1 / v2 / v3a / v4 / Spectre-BHB 的检测、
缓解（mitigation）和向用户空间报告（/sys/devices/system/cpu/vulnerabilities/*）。

1. 整体结构

文件主要由 5 大块组成：

┌─────────────┬───────────────────────────────────────────────────────────────────┬──────────────────────────────────────────────────┐
│ 漏洞        │ 主要机制                                                          │ 关键函数                                         │
├─────────────┼───────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Spectre v1  │ 内核用 __user 指针消毒，用户空间自求多福                          │ cpu_show_spectre_v1()                            │
├─────────────┼───────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Spectre v2  │ 分支预测器加固（BP hardening），通过 SMCCC 调 firmware workaround │ spectre_v2_enable_mitigation()                   │
├─────────────┼───────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Spectre v3a │ 让 hypervisor 向量走间接 trampoline，防止 guest 读到 VBAR_EL2     │ spectre_v3a_enable_mitigation()                  │
├─────────────┼───────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Spectre v4  │ SSBS 硬件位 / firmware SSBD workaround，支持 prctl() 按任务控制   │ spectre_v4_enable_mitigation()、ssbd_prctl_set() │
├─────────────┼───────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────────┤
│ Spectre-BHB │ loop/clearbhb/firmware 多种缓解，动态 patch 向量                  │ spectre_bhb_enable_mitigation()                  │
└─────────────┴───────────────────────────────────────────────────────────────────┴──────────────────────────────────────────────────┘

这些函数通过 arch/arm64/kernel/cpu_errata.c 里的 arm64_cpu_capabilities 表注册为 CPU capability，启动时由 __enable_cpu_capabilities() 调用。

2. 缓解状态

```c
  enum mitigation_state {
      SPECTRE_UNAFFECTED,   // 不受影响
      SPECTRE_MITIGATED,    // 已缓解
      SPECTRE_VULNERABLE,   // 易受攻击
  };
```

update_mitigation_state() 用 cmpxchg 保证多个 CPU 上线时状态只往更严重的方向升级（比如一个 CPU 安全、另一个脆弱，系统整体算脆弱）。一旦
system_capabilities_finalized() 完成，就不能再改，避免用户空间看到状态变化。

3. Spectre v2：分支预测器加固

v2 最复杂，因为 big.LITTLE 里不同核可能处于不同状态：

1. 硬件缓解：ID_AA64PFR0_EL1.CSV2 置位，或在 safelist 里（A35/A53/A55 等）。
2. Firmware 缓解：通过 SMCCC ARM_SMCCC_ARCH_WORKAROUND_1（WA1）调用 firmware。
3. 软件缓解：某些 Qualcomm Falkor 需要额外清 link stack。
4. 纯脆弱。

如果走 firmware 路径，会安装一个 per-cpu 回调 bp_hardening_data.fn，在异常入口调用（arm64_apply_bp_hardening()）。

```c
  DEFINE_PER_CPU_READ_MOSTLY(struct bp_hardening_data, bp_hardening_data);
```

bp_hardening_data.slot 用于 KVM hyp vector 的选择（direct / spectre-direct / indirect / spectre-indirect）。

启动参数 nospectre_v2 可关闭。

4. Spectre v3a

这个最简单：只对 Cortex-A57 / A72 生效，让 EL2 的 hyp vector 使用间接 trampoline，防止 guest 通过读取 VBAR_EL2 推断 hypervisor 的虚拟地址布局。

```c
  if (this_cpu_has_cap(ARM64_SPECTRE_V3A))
      data->slot += HYP_VECTOR_INDIRECT;
```

5. Spectre v4：Speculative Store Bypass

依赖 PSTATE.SSBS 或 firmware SSBD。

- 安全列表：A35/A53/A55 等不受影响。
- 硬件支持：ARM64_SSBS 特性，内核直接置 PSTATE.SSBS。
- Firmware：通过 SMCCC ARM_SMCCC_ARCH_WORKAROUND_2（WA2）。

ssbd= 启动参数：
- force-on：强制开启缓解
- force-off：强制关闭
- kernel（默认）：动态模式，用户空间可通过 prctl(PR_SPEC_STORE_BYPASS, ...) 自己控制

arch_prctl_spec_ctrl_set/get 是 prctl() 的底层实现，任务切换时会调用 spectre_v4_enable_task_mitigation() 把线程的 SSBS 状态写进
pt_regs->pstate，这样返回用户空间时自动生效。

6. Spectre-BHB：Branch History Buffer

BHB 是 v2 的变种。缓解方式按优先级：

1. ECBHB（Exception Clears BHB）：硬件特性，无需软件缓解。
2. CSV2.3：不受影响。
3. ClearBHB 指令：ARMv8.1+ 新增指令，直接清 BHB。
4. Loop mitigation：对特定 CPU 执行固定次数的分支环冲刷 BHB（k=8/11/24/32/38/132）。
5. Firmware WA3：SMCCC ARM_SMCCC_ARCH_WORKAROUND_3。

文件里维护 system_bhb_mitigations 位图和 max_bhb_k，启动后通过 alternative patching 把向量代码里的占位指令 patch 成实际缓解代码：

- spectre_bhb_patch_loop_mitigation_enable()
- spectre_bhb_patch_clearbhb()
- spectre_bhb_patch_loop_iter()（把循环次数 patch 成 max_bhb_k）
- spectre_bhb_patch_wa3()

7. 与其他文件的连接

- arch/arm64/kernel/cpu_errata.c：把这些注册为 ARM64_CPUCAP_LOCAL_CPU_ERRATUM。
- arch/arm64/include/asm/spectre.h：声明接口、bp_hardening_data、hyp vector 枚举。
- arch/arm64/kernel/process.c：上下文切换/进程创建时调用 spectre_v4_enable_task_mitigation()。
- arch/arm64/kernel/suspend.c、hibernate.c：恢复时重新应用缓解。
- arch/arm64/net/bpf_jit_comp.c：unprivileged eBPF 启用时检查 BHB 状态并告警。

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
