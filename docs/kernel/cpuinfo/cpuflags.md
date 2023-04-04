## [ ] 找到 qemu qmp 中是如何实现
- virsh domcapabilities ：可以查询该 qemu 支持的所有内容

- [x] qmp : query-cpu-definitions 是如何实现的

## arch/x86/include/asm/cpufeatures.h



## qemu 的 machine model
/run/current-system/sw/bin/qemu-system-x86_64 -S -no-user-config -nodefaults -nographic -machine none,accel=kvm:tcg
-qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.monitor,server=on,wait=off
-pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.pid

- [ ] -cpu SandyBridge 的时候，Guest 居然没有 vmx 的。
## eagerfpu
其中 eagerfpu 并不是硬件特性，而是操作系统用户态程序具有某项能力。

```diff
commit e63650840e8b053aa09ad934877e87e9941ed135
Author: Andy Lutomirski <luto@kernel.org>
Date:   Mon Oct 17 14:40:11 2016 -0700

    x86/fpu: Finish excising 'eagerfpu'

    Now that eagerfpu= is gone, remove it from the docs and some
    comments.  Also sync the changes to tools/.

    Signed-off-by: Andy Lutomirski <luto@kernel.org>
    Cc: Borislav Petkov <bp@alien8.de>
    Cc: Brian Gerst <brgerst@gmail.com>
    Cc: Dave Hansen <dave.hansen@linux.intel.com>
    Cc: Denys Vlasenko <dvlasenk@redhat.com>
    Cc: Fenghua Yu <fenghua.yu@intel.com>
    Cc: H. Peter Anvin <hpa@zytor.com>
    Cc: Josh Poimboeuf <jpoimboe@redhat.com>
    Cc: Linus Torvalds <torvalds@linux-foundation.org>
    Cc: Oleg Nesterov <oleg@redhat.com>
    Cc: Peter Zijlstra <peterz@infradead.org>
    Cc: Quentin Casasnovas <quentin.casasnovas@oracle.com>
    Cc: Rik van Riel <riel@redhat.com>
    Cc: Thomas Gleixner <tglx@linutronix.de>
    Link: http://lkml.kernel.org/r/cf430dd4481d41280e93ac6cf0def1007a67fc8e.1476740397.git.luto@kernel.org
    Signed-off-by: Ingo Molnar <mingo@kernel.org>

diff --git a/Documentation/kernel-parameters.txt b/Documentation/kernel-parameters.txt
index 37babf91f2cb..459b301137c2 100644
--- a/Documentation/kernel-parameters.txt
+++ b/Documentation/kernel-parameters.txt
@@ -1074,12 +1074,6 @@ bytes respectively. Such letter suffixes can also be entirely omitted.
 	nopku		[X86] Disable Memory Protection Keys CPU feature found
 			in some Intel CPUs.

-	eagerfpu=	[X86]
-			on	enable eager fpu restore
-			off	disable eager fpu restore
-			auto	selects the default scheme, which automatically
-				enables eagerfpu restore for xsaveopt.
-
 	module.async_probe [KNL]
 			Enable asynchronous probe on this module.

diff --git a/arch/x86/include/asm/cpufeatures.h b/arch/x86/include/asm/cpufeatures.h
index b212b862314a..2599222215c9 100644
--- a/arch/x86/include/asm/cpufeatures.h
+++ b/arch/x86/include/asm/cpufeatures.h
@@ -104,7 +104,6 @@
 #define X86_FEATURE_EXTD_APICID	( 3*32+26) /* has extended APICID (8 bits) */
 #define X86_FEATURE_AMD_DCM     ( 3*32+27) /* multi-node processor */
 #define X86_FEATURE_APERFMPERF	( 3*32+28) /* APERFMPERF */
-/* free, was #define X86_FEATURE_EAGER_FPU	( 3*32+29) * "eagerfpu" Non lazy FPU restore */
 #define X86_FEATURE_NONSTOP_TSC_S3 ( 3*32+30) /* TSC doesn't stop in S3 state */

 /* Intel-defined CPU features, CPUID level 0x00000001 (ecx), word 4 */
diff --git a/arch/x86/include/asm/fpu/types.h b/arch/x86/include/asm/fpu/types.h
index e31332d6f0e8..3c80f5b9c09d 100644
--- a/arch/x86/include/asm/fpu/types.h
+++ b/arch/x86/include/asm/fpu/types.h
@@ -329,29 +329,6 @@ struct fpu {
 	 * the registers in the FPU are more recent than this state
 	 * copy. If the task context-switches away then they get
 	 * saved here and represent the FPU state.
-	 *
-	 * After context switches there may be a (short) time period
-	 * during which the in-FPU hardware registers are unchanged
-	 * and still perfectly match this state, if the tasks
-	 * scheduled afterwards are not using the FPU.
-	 *
-	 * This is the 'lazy restore' window of optimization, which
-	 * we track though 'fpu_fpregs_owner_ctx' and 'fpu->last_cpu'.
-	 *
-	 * We detect whether a subsequent task uses the FPU via setting
-	 * CR0::TS to 1, which causes any FPU use to raise a #NM fault.
-	 *
-	 * During this window, if the task gets scheduled again, we
-	 * might be able to skip having to do a restore from this
-	 * memory buffer to the hardware registers - at the cost of
-	 * incurring the overhead of #NM fault traps.
-	 *
-	 * Note that on modern CPUs that support the XSAVEOPT (or other
-	 * optimized XSAVE instructions), we don't use #NM traps anymore,
-	 * as the hardware can track whether FPU registers need saving
-	 * or not. On such CPUs we activate the non-lazy ('eagerfpu')
-	 * logic, which unconditionally saves/restores all FPU state
-	 * across context switches. (if FPU state exists.)
 	 */
 	union fpregs_state		state;
 	/*
diff --git a/arch/x86/mm/pkeys.c b/arch/x86/mm/pkeys.c
index f88ce0e5efd9..2dab69a706ec 100644
--- a/arch/x86/mm/pkeys.c
+++ b/arch/x86/mm/pkeys.c
@@ -141,8 +141,7 @@ u32 init_pkru_value = PKRU_AD_KEY( 1) | PKRU_AD_KEY( 2) | PKRU_AD_KEY( 3) |
  * Called from the FPU code when creating a fresh set of FPU
  * registers.  This is called from a very specific context where
  * we know the FPU regstiers are safe for use and we can use PKRU
- * directly.  The fact that PKRU is only available when we are
- * using eagerfpu mode makes this possible.
+ * directly.
  */
 void copy_init_pkru_to_fpregs(void)
 {
diff --git a/tools/arch/x86/include/asm/cpufeatures.h b/tools/arch/x86/include/asm/cpufeatures.h
index 1188bc849ee3..2599222215c9 100644
--- a/tools/arch/x86/include/asm/cpufeatures.h
+++ b/tools/arch/x86/include/asm/cpufeatures.h
@@ -104,7 +104,6 @@
 #define X86_FEATURE_EXTD_APICID	( 3*32+26) /* has extended APICID (8 bits) */
 #define X86_FEATURE_AMD_DCM     ( 3*32+27) /* multi-node processor */
 #define X86_FEATURE_APERFMPERF	( 3*32+28) /* APERFMPERF */
-#define X86_FEATURE_EAGER_FPU	( 3*32+29) /* "eagerfpu" Non lazy FPU restore */
 #define X86_FEATURE_NONSTOP_TSC_S3 ( 3*32+30) /* TSC doesn't stop in S3 state */

 /* Intel-defined CPU features, CPUID level 0x00000001 (ecx), word 4 */
```

## [x] rtm
tsx_init
```c
	ret = cmdline_find_option(boot_command_line, "tsx", arg, sizeof(arg));
	if (ret >= 0) {
		if (!strcmp(arg, "on")) {
			tsx_ctrl_state = TSX_CTRL_ENABLE;
		} else if (!strcmp(arg, "off")) {
			tsx_ctrl_state = TSX_CTRL_DISABLE;
		} else if (!strcmp(arg, "auto")) {
			tsx_ctrl_state = x86_get_tsx_auto_mode();
		} else {
			tsx_ctrl_state = TSX_CTRL_DISABLE;
			pr_err("tsx: invalid option, defaulting to off\n");
		}
	} else {
		/* tsx= not provided */
		if (IS_ENABLED(CONFIG_X86_INTEL_TSX_MODE_AUTO))
			tsx_ctrl_state = x86_get_tsx_auto_mode();
		else if (IS_ENABLED(CONFIG_X86_INTEL_TSX_MODE_OFF))
			tsx_ctrl_state = TSX_CTRL_DISABLE;
		else
			tsx_ctrl_state = TSX_CTRL_ENABLE;
	}
```

```c
#define X86_FEATURE_RTM			( 9*32+11) /* Restricted Transactional Memory */
```
- 一直都在，但是被删除了？但是没有找到删除的代码

确认一下，这个 feature 是在这里删除的
```diff
History:        #0
Commit:         293649307ef9abcd4f83f6dac4d4400dfd97c936
Author:         Pawan Gupta <pawan.kumar.gupta@linux.intel.com>
Committer:      Borislav Petkov <bp@suse.de>
Author Date:    Tue 15 Jun 2021 05:14:25 AM CST
Committer Date: Tue 15 Jun 2021 11:46:48 PM CST

x86/tsx: Clear CPUID bits when TSX always force aborts

As a result of TSX deprecation, some processors always abort TSX
transactions by default after a microcode update.

When TSX feature cannot be used it is better to hide it. Clear CPUID.RTM
and CPUID.HLE bits when TSX transactions always abort.

 [ bp: Massage commit message and comments. ]

Signed-off-by: Pawan Gupta <pawan.kumar.gupta@linux.intel.com>
Signed-off-by: Borislav Petkov <bp@suse.de>
Reviewed-by: Thomas Gleixner <tglx@linutronix.de>
Reviewed-by: Andi Kleen <ak@linux.intel.com>
Reviewed-by: Tony Luck <tony.luck@intel.com>
Tested-by: Neelima Krishnan <neelima.krishnan@intel.com>
Link: https://lkml.kernel.org/r/5209b3d72ffe5bd3cafdcc803f5b883f785329c3.1623704845.git-series.pawan.kumar.gupta@linux.intel.com
```


```diff
commit 1348924ba8169f35cedfd0a0087872b81a632b8e
Author: Pawan Gupta <pawan.kumar.gupta@linux.intel.com>
Date:   Mon Jun 14 14:12:22 2021 -0700

    x86/msr: Define new bits in TSX_FORCE_ABORT MSR

    Intel client processors that support the IA32_TSX_FORCE_ABORT MSR
    related to perf counter interaction [1] received a microcode update that
    deprecates the Transactional Synchronization Extension (TSX) feature.
    The bit FORCE_ABORT_RTM now defaults to 1, writes to this bit are
    ignored. A new bit TSX_CPUID_CLEAR clears the TSX related CPUID bits.

    The summary of changes to the IA32_TSX_FORCE_ABORT MSR are:

      Bit 0: FORCE_ABORT_RTM (legacy bit, new default=1) Status bit that
      indicates if RTM transactions are always aborted. This bit is
      essentially !SDV_ENABLE_RTM(Bit 2). Writes to this bit are ignored.

      Bit 1: TSX_CPUID_CLEAR (new bit, default=0) When set, CPUID.HLE = 0
      and CPUID.RTM = 0.

      Bit 2: SDV_ENABLE_RTM (new bit, default=0) When clear, XBEGIN will
      always abort with EAX code 0. When set, XBEGIN will not be forced to
      abort (but will always abort in SGX enclaves). This bit is intended to
      be used on developer systems. If this bit is set, transactional
      atomicity correctness is not certain. SDV = Software Development
      Vehicle (SDV), i.e. developer systems.

    Performance monitoring counter 3 is usable in all cases, regardless of
    the value of above bits.

    Add support for a new CPUID bit - CPUID.RTM_ALWAYS_ABORT (CPUID 7.EDX[11])
     - to indicate the status of always abort behavior.

    [1] [ bp: Look for document ID 604224, "Performance Monitoring Impact
          of Intel Transactional Synchronization Extension Memory". Since
          there's no way for us to have stable links to documents... ]

     [ bp: Massage and extend commit message. ]

    Signed-off-by: Pawan Gupta <pawan.kumar.gupta@linux.intel.com>
    Signed-off-by: Borislav Petkov <bp@suse.de>
    Reviewed-by: Thomas Gleixner <tglx@linutronix.de>
    Reviewed-by: Andi Kleen <ak@linux.intel.com>
    Reviewed-by: Tony Luck <tony.luck@intel.com>
    Tested-by: Neelima Krishnan <neelima.krishnan@intel.com>
    Link: https://lkml.kernel.org/r/9add61915b4a4eedad74fbd869107863a28b428e.1623704845.git-series.pawan.kumar.gupta@linux.intel.com

diff --git a/arch/x86/include/asm/cpufeatures.h b/arch/x86/include/asm/cpufeatures.h
index 81269c73a0dc..d0ce5cfd3ac1 100644
--- a/arch/x86/include/asm/cpufeatures.h
+++ b/arch/x86/include/asm/cpufeatures.h
@@ -378,6 +378,7 @@
 #define X86_FEATURE_AVX512_VP2INTERSECT (18*32+ 8) /* AVX-512 Intersect for D/Q */
 #define X86_FEATURE_SRBDS_CTRL		(18*32+ 9) /* "" SRBDS mitigation MSR available */
 #define X86_FEATURE_MD_CLEAR		(18*32+10) /* VERW clears CPU buffers */
+#define X86_FEATURE_RTM_ALWAYS_ABORT	(18*32+11) /* "" RTM transaction always aborts */
 #define X86_FEATURE_TSX_FORCE_ABORT	(18*32+13) /* "" TSX_FORCE_ABORT */
 #define X86_FEATURE_SERIALIZE		(18*32+14) /* SERIALIZE instruction */
 #define X86_FEATURE_HYBRID_CPU		(18*32+15) /* "" This part has CPUs of more than one type */
diff --git a/arch/x86/include/asm/msr-index.h b/arch/x86/include/asm/msr-index.h
index 742d89a00721..2bc1600d3604 100644
--- a/arch/x86/include/asm/msr-index.h
+++ b/arch/x86/include/asm/msr-index.h
@@ -772,6 +772,10 @@

 #define MSR_TFA_RTM_FORCE_ABORT_BIT	0
 #define MSR_TFA_RTM_FORCE_ABORT		BIT_ULL(MSR_TFA_RTM_FORCE_ABORT_BIT)
+#define MSR_TFA_TSX_CPUID_CLEAR_BIT	1
+#define MSR_TFA_TSX_CPUID_CLEAR		BIT_ULL(MSR_TFA_TSX_CPUID_CLEAR_BIT)
+#define MSR_TFA_SDV_ENABLE_RTM_BIT	2
+#define MSR_TFA_SDV_ENABLE_RTM		BIT_ULL(MSR_TFA_SDV_ENABLE_RTM_BIT)

 /* P4/Xeon+ specific */
 #define MSR_IA32_MCG_EAX		0x00000180
```

## [x] ept_ad
当时没有加上

```diff
History:        #0
Commit:         301d328a6f8b53bb86c5ecf72db7bc178bcf1999
Author:         Peter Feiner <pfeiner@google.com>
Committer:      Thomas Gleixner <tglx@linutronix.de>
Author Date:    Thu 02 Aug 2018 02:06:57 AM CST
Committer Date: Fri 03 Aug 2018 06:36:23 PM CST

x86/cpufeatures: Add EPT_AD feature bit

Some Intel processors have an EPT feature whereby the accessed & dirty bits
in EPT entries can be updated by HW. MSR IA32_VMX_EPT_VPID_CAP exposes the
presence of this capability.

There is no point in trying to use that new feature bit in the VMX code as
VMX needs to read the MSR anyway to access other bits, but having the
feature bit for EPT_AD in place helps virtualization management as it
exposes "ept_ad" in /proc/cpuinfo/$proc/flags if the feature is present.

[ tglx: Amended changelog ]

Signed-off-by: Peter Feiner <pfeiner@google.com>
Signed-off-by: Peter Shier <pshier@google.com>
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
Reviewed-by: Jim Mattson <jmattson@google.com>
Cc: "H. Peter Anvin" <hpa@zytor.com>
Cc: Borislav Petkov <bp@suse.de>
Cc: Konrad Rzeszutek Wilk <konrad.wilk@oracle.com>
Cc: David Woodhouse <dwmw@amazon.co.uk>
Link: https://lkml.kernel.org/r/20180801180657.138051-1-pshier@google.com

diff --git a/arch/x86/include/asm/cpufeatures.h b/arch/x86/include/asm/cpufeatures.h
index 5701f5cecd31..7fff98fa5855 100644
--- a/arch/x86/include/asm/cpufeatures.h
+++ b/arch/x86/include/asm/cpufeatures.h
@@ -229,7 +229,7 @@

 #define X86_FEATURE_VMMCALL		( 8*32+15) /* Prefer VMMCALL to VMCALL */
 #define X86_FEATURE_XENPV		( 8*32+16) /* "" Xen paravirtual guest */
-
+#define X86_FEATURE_EPT_AD		( 8*32+17) /* Intel Extended Page Table access-dirty bit */

 /* Intel-defined CPU features, CPUID level 0x00000007:0 (EBX), word 9 */
 #define X86_FEATURE_FSGSBASE		( 9*32+ 0) /* RDFSBASE, WRFSBASE, RDGSBASE, WRGSBASE instructions*/

```

## [x] 浮点的问题
- [x] 互相对照的? xsaves eagerfpu

## [x] cpuid cpuid_fault
```diff
tree b06c5eadda6330f77e7ab3e47af0d3267105d3de
parent 79170fda313ed5be2394f87aa2a00d597f8ed4a1
author Kyle Huey <me@kylehuey.com> Mon Mar 20 01:16:25 2017 -0700
committer Thomas Gleixner <tglx@linutronix.de> Mon Mar 20 16:10:34 2017 +0100

x86/cpufeature: Detect CPUID faulting support

Intel supports faulting on the CPUID instruction beginning with Ivy Bridge.
When enabled, the processor will fault on attempts to execute the CPUID
instruction with CPL>0. This will allow a ptracer to emulate the CPUID
instruction.

Bit 31 of MSR_PLATFORM_INFO advertises support for this feature. It is
documented in detail in Section 2.3.2 of
https://bugzilla.kernel.org/attachment.cgi?id=243991

Detect support for this feature and expose it as X86_FEATURE_CPUID_FAULT.

Signed-off-by: Kyle Huey <khuey@kylehuey.com>
Reviewed-by: Borislav Petkov <bp@suse.de>
Cc: Grzegorz Andrejczuk <grzegorz.andrejczuk@intel.com>
Cc: kvm@vger.kernel.org
Cc: Radim Krčmář <rkrcmar@redhat.com>
Cc: Peter Zijlstra <peterz@infradead.org>
Cc: Dave Hansen <dave.hansen@linux.intel.com>
Cc: Andi Kleen <andi@firstfloor.org>
Cc: linux-kselftest@vger.kernel.org
Cc: Nadav Amit <nadav.amit@gmail.com>
Cc: Robert O'Callahan <robert@ocallahan.org>
Cc: Richard Weinberger <richard@nod.at>
Cc: "Rafael J. Wysocki" <rafael.j.wysocki@intel.com>
Cc: Andy Lutomirski <luto@kernel.org>
Cc: Len Brown <len.brown@intel.com>
Cc: Shuah Khan <shuah@kernel.org>
Cc: user-mode-linux-devel@lists.sourceforge.net
Cc: Jeff Dike <jdike@addtoit.com>
Cc: Alexander Viro <viro@zeniv.linux.org.uk>
Cc: user-mode-linux-user@lists.sourceforge.net
Cc: David Matlack <dmatlack@google.com>
Cc: Boris Ostrovsky <boris.ostrovsky@oracle.com>
Cc: Dmitry Safonov <dsafonov@virtuozzo.com>
Cc: linux-fsdevel@vger.kernel.org
Cc: Paolo Bonzini <pbonzini@redhat.com>
Link: http://lkml.kernel.org/r/20170320081628.18952-8-khuey@kylehuey.com
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>


diff --git a/arch/x86/include/asm/cpufeatures.h b/arch/x86/include/asm/cpufeatures.h
index b04bb6dfed7f..0fe00446f9ca 100644
--- a/arch/x86/include/asm/cpufeatures.h
+++ b/arch/x86/include/asm/cpufeatures.h
@@ -187,6 +187,7 @@
  * Reuse free bits when adding new feature flags!
  */
 #define X86_FEATURE_RING3MWAIT	( 7*32+ 0) /* Ring 3 MONITOR/MWAIT */
+#define X86_FEATURE_CPUID_FAULT ( 7*32+ 1) /* Intel CPUID faulting */
 #define X86_FEATURE_CPB		( 7*32+ 2) /* AMD Core Performance Boost */
 #define X86_FEATURE_EPB		( 7*32+ 3) /* IA32_ENERGY_PERF_BIAS support */
 #define X86_FEATURE_CAT_L3	( 7*32+ 4) /* Cache Allocation Technology L3 */
diff --git a/arch/x86/include/asm/msr-index.h b/arch/x86/include/asm/msr-index.h
index f429b70ebaef..b1f75daca34b 100644
--- a/arch/x86/include/asm/msr-index.h
+++ b/arch/x86/include/asm/msr-index.h
@@ -45,6 +45,8 @@
 #define MSR_IA32_PERFCTR1		0x000000c2
 #define MSR_FSB_FREQ			0x000000cd
 #define MSR_PLATFORM_INFO		0x000000ce
+#define MSR_PLATFORM_INFO_CPUID_FAULT_BIT	31
+#define MSR_PLATFORM_INFO_CPUID_FAULT		BIT_ULL(MSR_PLATFORM_INFO_CPUID_FAULT_BIT)

 #define MSR_PKG_CST_CONFIG_CONTROL	0x000000e2
 #define NHM_C3_AUTO_DEMOTE		(1UL << 25)
diff --git a/arch/x86/kernel/cpu/intel.c b/arch/x86/kernel/cpu/intel.c
index e229318d7230..a07f8295c9ed 100644
--- a/arch/x86/kernel/cpu/intel.c
+++ b/arch/x86/kernel/cpu/intel.c
@@ -488,6 +488,28 @@ static void intel_bsp_resume(struct cpuinfo_x86 *c)
 	init_intel_energy_perf(c);
 }

+static void init_cpuid_fault(struct cpuinfo_x86 *c)
+{
+	u64 msr;
+
+	if (!rdmsrl_safe(MSR_PLATFORM_INFO, &msr)) {
+		if (msr & MSR_PLATFORM_INFO_CPUID_FAULT)
+			set_cpu_cap(c, X86_FEATURE_CPUID_FAULT);
+	}
+}
+
+static void init_intel_misc_features(struct cpuinfo_x86 *c)
+{
+	u64 msr;
+
+	if (rdmsrl_safe(MSR_MISC_FEATURES_ENABLES, &msr))
+		return;
+
+	/* Check features and update capabilities */
+	init_cpuid_fault(c);
+	probe_xeon_phi_r3mwait(c);
+}
+
 static void init_intel(struct cpuinfo_x86 *c)
 {
 	unsigned int l2 = 0;
@@ -602,7 +624,7 @@ static void init_intel(struct cpuinfo_x86 *c)

 	init_intel_energy_perf(c);

-	probe_xeon_phi_r3mwait(c);
+	init_intel_misc_features(c);
 }

 #ifdef CONFIG_X86_32
```

```diff
tree 7acbeb745e004b207a8f8c168a224b501a0caa18
parent a5828ed3d03d38399159dc17a98cefde3109a66b
author Borislav Petkov <bp@suse.de> Wed Jan 18 11:15:37 2017 -0800
committer Ingo Molnar <mingo@kernel.org> Wed Jan 25 10:12:39 2017 +0100

x86/cpu: Add X86_FEATURE_CPUID

Add a synthetic CPUID flag denoting whether the CPU sports the CPUID
instruction or not. This will come useful later when accomodating
CPUID-less CPUs.

Signed-off-by: Borislav Petkov <bp@suse.de>
[ Slightly prettified. ]
Signed-off-by: Andy Lutomirski <luto@kernel.org>
Reviewed-by: Borislav Petkov <bp@suse.de>
Cc: Borislav Petkov <bp@alien8.de>
Cc: Brian Gerst <brgerst@gmail.com>
Cc: Dave Hansen <dave.hansen@linux.intel.com>
Cc: Fenghua Yu <fenghua.yu@intel.com>
Cc: H. Peter Anvin <hpa@zytor.com>
Cc: Linus Torvalds <torvalds@linux-foundation.org>
Cc: Matthew Whitehead <tedheadster@gmail.com>
Cc: Oleg Nesterov <oleg@redhat.com>
Cc: One Thousand Gnomes <gnomes@lxorguk.ukuu.org.uk>
Cc: Peter Zijlstra <peterz@infradead.org>
Cc: Rik van Riel <riel@redhat.com>
Cc: Thomas Gleixner <tglx@linutronix.de>
Cc: Yu-cheng Yu <yu-cheng.yu@intel.com>
Link: http://lkml.kernel.org/r/dcb355adae3ab812c79397056a61c212f1a0c7cc.1484705016.git.luto@kernel.org
Signed-off-by: Ingo Molnar <mingo@kernel.org>

diff --git a/arch/x86/include/asm/cpufeatures.h b/arch/x86/include/asm/cpufeatures.h
index eafee3161d1c..a0f4ff25669e 100644
--- a/arch/x86/include/asm/cpufeatures.h
+++ b/arch/x86/include/asm/cpufeatures.h
@@ -100,7 +100,7 @@
 #define X86_FEATURE_XTOPOLOGY	( 3*32+22) /* cpu topology enum extensions */
 #define X86_FEATURE_TSC_RELIABLE ( 3*32+23) /* TSC is known to be reliable */
 #define X86_FEATURE_NONSTOP_TSC	( 3*32+24) /* TSC does not stop in C states */
-/* free, was #define X86_FEATURE_CLFLUSH_MONITOR ( 3*32+25) * "" clflush reqd with monitor */
+#define X86_FEATURE_CPUID	( 3*32+25) /* CPU has CPUID instruction itself */
 #define X86_FEATURE_EXTD_APICID	( 3*32+26) /* has extended APICID (8 bits) */
 #define X86_FEATURE_AMD_DCM     ( 3*32+27) /* multi-node processor */
 #define X86_FEATURE_APERFMPERF	( 3*32+28) /* APERFMPERF */
diff --git a/arch/x86/kernel/cpu/common.c b/arch/x86/kernel/cpu/common.c
index 9bab7a8a4293..c3328d0e13f0 100644
--- a/arch/x86/kernel/cpu/common.c
+++ b/arch/x86/kernel/cpu/common.c
@@ -801,14 +801,12 @@ static void __init early_identify_cpu(struct cpuinfo_x86 *c)
 	memset(&c->x86_capability, 0, sizeof c->x86_capability);
 	c->extended_cpuid_level = 0;

-	if (!have_cpuid_p())
-		identify_cpu_without_cpuid(c);
-
 	/* cyrix could have cpuid enabled via c_identify()*/
 	if (have_cpuid_p()) {
 		cpu_detect(c);
 		get_cpu_vendor(c);
 		get_cpu_cap(c);
+		setup_force_cpu_cap(X86_FEATURE_CPUID);

 		if (this_cpu->c_early_init)
 			this_cpu->c_early_init(c);
@@ -818,6 +816,9 @@ static void __init early_identify_cpu(struct cpuinfo_x86 *c)

 		if (this_cpu->c_bsp_init)
 			this_cpu->c_bsp_init(c);
+	} else {
+		identify_cpu_without_cpuid(c);
+		setup_clear_cpu_cap(X86_FEATURE_CPUID);
 	}

 	setup_force_cpu_cap(X86_FEATURE_ALWAYS);
```
## 对比下虚拟机中到底有何区别
```txt
processor       : 30
vendor_id       : GenuineIntel
cpu family      : 6
model           : 183
model name      : 13th Gen Intel(R) Core(TM) i9-13900K
stepping        : 1
microcode       : 0x112
cpu MHz         : 2995.200
cache size      : 16384 KB
physical id     : 0
siblings        : 31
core id         : 30
cpu cores       : 31
apicid          : 30
initial apicid  : 30
fpu             : yes
fpu_exception   : yes
cpuid level     : 31
wp              : yes
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm
 constant_tsc arch_perfmon rep_good nopl xtopology cpuid tsc_known_freq pni pclmulqdq vmx ssse3 fma cx16 pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt t
sc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_sha
dow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xget
bv1 xsaves avx_vnni arat umip pku ospke waitpkg gfni vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize arch_capabilities
vmx flags       : vnmi preemption_timer posted_intr invvpid ept_x_only ept_ad ept_1gb flexpriority apicv tsc_offset vtpr mtf vapic ept vpid unrestrict
ed_guest vapic_reg vid shadow_vmcs pml tsc_scaling
bugs            : spectre_v1 spectre_v2 spec_store_bypass swapgs mmio_unknown eibrs_pbrsb
bogomips        : 5990.40
clflush size    : 64
cache_alignment : 64
address sizes   : 46 bits physical, 48 bits virtual
power management:
```

```txt
processor       : 31
vendor_id       : GenuineIntel
cpu family      : 6
model           : 183
model name      : 13th Gen Intel(R) Core(TM) i9-13900K
stepping        : 1
microcode       : 0x112
cpu MHz         : 3000.000
cache size      : 36864 KB
physical id     : 0
siblings        : 32
core id         : 47
cpu cores       : 24
apicid          : 94
initial apicid  : 94
fpu             : yes
fpu_exception   : yes
cpuid level     : 32
wp              : yes
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx p
dpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monit
or ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm ab
m 3dnowprefetch cpuid_fault epb invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi
1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb intel_pt sha_ni xsaveopt xsavec xgetbv1 xsaves split_lock_detect avx_vnni dtherm ida ara
t pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi umip pku ospke waitpkg gfni vaes vpclmulqdq tme rdpid movdiri movdir64b fsrm md_clear
serialize pconfig arch_lbr ibt flush_l1d arch_capabilities
vmx flags       : vnmi preemption_timer posted_intr invvpid ept_x_only ept_ad ept_1gb flexpriority apicv tsc_offset vtpr mtf vapic ept vpid unrestrict
ed_guest vapic_reg vid ple shadow_vmcs ept_mode_based_exec tsc_scaling usr_wait_pause
bugs            : spectre_v1 spectre_v2 spec_store_bypass swapgs eibrs_pbrsb
bogomips        : 5990.40
clflush size    : 64
cache_alignment : 64
address sizes   : 46 bits physical, 48 bits virtual
power management:
```
