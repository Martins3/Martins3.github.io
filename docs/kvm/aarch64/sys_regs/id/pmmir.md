## aarch64 pmmir
<!-- d1a3ac18-925e-4ebe-850f-5f1e0492a81e -->

关于指令使用的测试在 ./code/src/m/arch/aarch64/

虚拟机启动 panic
```txt
[    0.000000][    T0] Linux version 5.10.0-216.0.0.115.oe2203sp4.aarch64 (root@dc-64g.compass-ci) (gcc_old (GCC) 10.3.1, GNU ld (GNU Binutils) 2.37) #1 SMP Thu Jun 27 15:22:10 CST 2024
```

```txt
[    4.553606][    T1] Call trace:
[    4.553857][    T1]  __armv8pmu_probe_pmu+0xe0/0x114
[    4.554257][    T1]  generic_exec_single+0x100/0x184
[    4.554651][    T1]  smp_call_function_single+0x154/0x1ac
[    4.555083][    T1]  smp_call_function_any+0x150/0x184
[    4.555489][    T1]  armv8_pmu_init.constprop.0+0x60/0x220
[    4.555926][    T1]  armv8_pmuv3_init+0x28/0x50
[    4.556288][    T1]  arm_pmu_acpi_probe+0x8c/0x168
[    4.556674][    T1]  armv8_pmu_driver_init+0x40/0x60
[    4.557067][    T1]  do_one_initcall+0x50/0x2a0
[    4.557431][    T1]  do_initcall_level+0xe4/0x110
[    4.557806][    T1]  do_initcalls+0x80/0xb8
[    4.558139][    T1]  kernel_init_freeable+0x1c8/0x254
[    4.558543][    T1]  kernel_init+0x1c/0x144
[    4.558881][    T1]  ret_from_fork+0x10/0x18
[    4.559223][    T1] Code: 7100127f 5400008d 926102c0 36f80056 (d5389ec0)
[    4.559815][    T1] ---[ end trace 0c7b2a7da113089c ]---
[    4.560241][    T1] Kernel panic - not syncing: Oops - Undefined instruction: Fatal exception
[    4.560940][    T1] SMP: stopping secondary CPUs
[    4.561334][    T1] Kernel Offset: disabled
[    4.561668][    T1] CPU features: 0x00e0,08040807,7a200038
[    4.562103][    T1] Memory Limit: none
[    4.562424][    T1] ---[ end Kernel panic - not syncing: Oops - Undefined instruction: Fatal exception ]---
```

物理机中观察到的结果:
```txt
[601380.271277] kvm [1391196]: Unsupported guest sys_reg access at: ffff80001003c2b4 [20400085]
[601380.284974] kvm [1391179]:  { Op0( 3), Op1( 0), CRn( 9), CRm(14), Op2( 6), func_read },
```

在 tools/arch/arm64/include/asm/sysreg.h arch/arm64/include/asm/sysreg.h 中可以找到
对应的寄存器是什么:


```txt
#define SYS_PMMIR_EL1			sys_reg(3, 0, 9, 14, 6)
```

```txt
{ SYS_DESC(SYS_PMMIR_EL1), trap_raz_wi },
```

关键问题在于: __armv8pmu_probe_pmu

```c
	/* store PMMIR register for sysfs */
	if (is_pmuv3p4(pmuver))
		cpu_pmu->reg_pmmir = read_pmmir();
	else
		cpu_pmu->reg_pmmir = 0;
```

```c
static inline bool is_pmuv3p4(int pmuver)
{
	return pmuver >= ID_AA64DFR0_EL1_PMUVer_V3P4;
}
```

```c
static inline unsigned long read_pmmir(void)
{
	return read_cpuid(PMMIR_EL1);
}
```

很容易找到这个 fix patch
```diff
From fe19b95ee12ab75765c105808c525f1aa61d0842 Mon Sep 17 00:00:00 2001
From: Andrew Murray <andrew.murray@arm.com>
Date: Thu, 30 Nov 2023 14:31:50 +0800
Subject: [PATCH 2/2] KVM: arm64: limit PMU version to PMUv3 for ARMv8.1

mainline inclusion
from mainline-v5.7-rc1
commit c854188ea01062f5a5fd7f05658feb1863774eaa
category: bugfix
bugzilla: https://gitee.com/openeuler/kernel/issues/I8K8XV
CVE: NA

Reference: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=c854188ea01062f5a5fd7f05658feb1863774eaa
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
