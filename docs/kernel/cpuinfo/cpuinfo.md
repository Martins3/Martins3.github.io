# cpuinfo

代码 : `show_cpuinfo`

## 概括
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
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb intel_pt sha_ni xsaveopt xsavec xgetbv1 xsaves split_lock_detect avx_vnni dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi umip pku ospke waitpkg gfni vaes vpclmulqdq tme rdpid movdiri movdir64b fsrm md_clear serialize pconfig arch_lbr ibt flush_l1d arch_capabilities
vmx flags       : vnmi preemption_timer posted_intr invvpid ept_x_only ept_ad ept_1gb flexpriority apicv tsc_offset vtpr mtf vapic ept vpid unrestricted_guest vapic_reg vid ple shadow_vmcs ept_mode_based_exec tsc_scaling usr_wait_pause
bugs            : spectre_v1 spectre_v2 spec_store_bypass swapgs eibrs_pbrsb
bogomips        : 5990.40
clflush size    : 64
cache_alignment : 64
address sizes   : 46 bits physical, 48 bits virtual
power management:
```

## https://unix.stackexchange.com/questionhttps://a4lg.com/tech/x86/database/x86-families-and-models.en.htmls/43539/what-do-the-flags-in-proc-cpuinfo-mean

## https://unix.stackexchange.com/questions/146051/number-of-processors-in-proc-cpuinfo

## processor : 0 - 31
通过 cpu_detect 中调用
## vendor_id : GenuineIntel
## cpu family	: 6
intel 6
AMD 24

## model		: 183
应该是每一款 CPU 都有自己的，但是很难找到完全的数据:
- https://a4lg.com/tech/x86/database/x86-families-and-models.en.html

这个文件中存在部分的定义
/home/martins3/core/linux/arch/x86/include/asm/intel-family.h

想不到字符串也是存储到 model name 中的:
```c
static void get_model_name(struct cpuinfo_x86 *c)
{
	unsigned int *v;
	char *p, *q, *s;

	if (c->extended_cpuid_level < 0x80000004)
		return;

	v = (unsigned int *)c->x86_model_id;
	cpuid(0x80000002, &v[0], &v[1], &v[2], &v[3]);
	cpuid(0x80000003, &v[4], &v[5], &v[6], &v[7]);
	cpuid(0x80000004, &v[8], &v[9], &v[10], &v[11]);
	c->x86_model_id[48] = 0;

	/* Trim whitespace */
	p = q = s = &c->x86_model_id[0];

	while (*p == ' ')
		p++;

	while (*p) {
		/* Note the last non-whitespace index */
		if (!isspace(*p))
			s = q;

		*q++ = *p++;
	}

	*(s + 1) = '\0';
}
```

在 Guest 中观测到的:
```txt
model           : 61
model name      : Intel Core Processor (Broadwell, no TSX, IBRS)
```
而且 qemu 中也的确定义了。

## model name	: 13th Gen Intel(R) Core(TM) i9-13900K
```c
#define INTEL_FAM6_RAPTORLAKE		0xB7
#define INTEL_FAM6_RAPTORLAKE_P		0xBA
#define INTEL_FAM6_RAPTORLAKE_S		0xBF
```
- [ ] 这里的 cpu model 和 QEMU 中的 cpu model 相同吗？

openAI 的回答:
```txt
    NetBurst (Pentium 4): Launched in November 2000
    Core (Yonah): Launched in January 2006
    Penryn (Core 2): Launched in November 2007
    Nehalem: Launched in November 2008
    Westmere: Launched in January 2010
    Sandy Bridge: Launched in January 2011
    Ivy Bridge: Launched in April 2012
    Haswell: Launched in June 2013
    Broadwell: Launched in September 2014
    Skylake: Launched in August 2015
    Kaby Lake: Launched in August 2016
    Coffee Lake: Launched in October 2017
    Cannon Lake: Launched in May 2018
    Ice Lake: Launched in August 2019
    Comet Lake: Launched in August 2019
    Tiger Lake: Launched in September 2020
    Rocket Lake: Launched in March 2021
```
我发现，一个 model 可以对应多个 model name !

## stepping	: 1
- https://en.wikipedia.org/wiki/Stepping_level

## microcode	: 0x112

## cpu MHz
```txt
cpu MHz     : 5500.000
cpu MHz		: 3000.000
cpu MHz		: 5500.105
cpu MHz		: 3000.000
cpu MHz		: 5500.000
cpu MHz		: 3000.000
cpu MHz		: 5500.000
cpu MHz		: 3000.000
cpu MHz		: 5500.000
cpu MHz		: 3000.000
cpu MHz		: 5500.000
cpu MHz		: 3000.000
cpu MHz		: 5500.000
cpu MHz		: 3000.000
cpu MHz		: 5500.000
cpu MHz		: 3000.000
cpu MHz		: 4343.085
cpu MHz		: 4300.160
cpu MHz		: 4300.332
cpu MHz		: 4290.512
cpu MHz		: 4298.115
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
cpu MHz		: 3000.000
```

## cache size	: 36864 KB

## physical id	: 0

## siblings	: 32

## core id
core id		: 0
core id		: 0
core id		: 4
core id		: 4
core id		: 8
core id		: 8
core id		: 12
core id		: 12
core id		: 16
core id		: 16
core id		: 20
core id		: 20
core id		: 24
core id		: 24
core id		: 28
core id		: 28
core id		: 32
core id		: 33
core id		: 34
core id		: 35
core id		: 36
core id		: 37
core id		: 38
core id		: 39
core id		: 40
core id		: 41
core id		: 42
core id		: 43
core id		: 44
core id		: 45
core id		: 46
core id		: 47

## cpu cores	: 24

## apicid
```txt
apicid          : 0
apicid          : 1
apicid          : 8
apicid          : 9
apicid          : 16
apicid          : 17
apicid          : 24
apicid          : 25
apicid          : 32
apicid          : 33
apicid          : 40
apicid          : 41
apicid          : 48
apicid          : 49
apicid          : 56
apicid          : 57
apicid          : 64
apicid          : 66
apicid          : 68
apicid          : 70
apicid          : 72
apicid          : 74
apicid          : 76
apicid          : 78
apicid          : 80
apicid          : 82
apicid          : 84
apicid          : 86
apicid          : 88
apicid          : 90
apicid          : 92
apicid          : 94
```

## initial apicid
initial apicid	: 94
initial apicid	: 0
initial apicid	: 1
initial apicid	: 8
initial apicid	: 9
initial apicid	: 16
initial apicid	: 17
initial apicid	: 24
initial apicid	: 25
initial apicid	: 32
initial apicid	: 33
initial apicid	: 40
initial apicid	: 41
initial apicid	: 48
initial apicid	: 49
initial apicid	: 56
initial apicid	: 57
initial apicid	: 64
initial apicid	: 66
initial apicid	: 68
initial apicid	: 70
initial apicid	: 72
initial apicid	: 74
initial apicid	: 76
initial apicid	: 78
initial apicid	: 80
initial apicid	: 82
initial apicid	: 84
initial apicid	: 86
initial apicid	: 88
initial apicid	: 90
initial apicid	: 92
initial apicid	: 94

## fpu		: yes
## fpu_exception	: yes
## flags		: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single ssbd ibrs ibpb stibp ibrs_enhanced tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid rdseed adx smap clflushopt clwb intel_pt sha_ni xsaveopt xsavec xgetbv1 xsaves split_lock_detect avx_vnni dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp hwp_pkg_req hfi umip pku ospke waitpkg gfni vaes vpclmulqdq tme rdpid movdiri movdir64b fsrm md_clear serialize pconfig arch_lbr ibt flush_l1d arch_capabilities

## cpuid level	: 32

## wp		: yes

## vmx flags	: vnmi preemption_timer posted_intr invvpid ept_x_only ept_ad ept_1gb flexpriority apicv tsc_offset vtpr mtf vapic ept vpid unrestricted_guest vapic_reg vid ple shadow_vmcs ept_mode_based_exec tsc_scaling usr_wait_pause

## bugs		: spectre_v1 spectre_v2 spec_store_bypass swapgs eibrs_pbrsb

## bogomips	: 5990.40

## clflush size	: 64

## cache_alignment	: 64

## address sizes	: 46 bits physical, 48 bits virtual

## power management

https://cpuid.apps.poly.nomial.co.uk/

## 补充其他内容
- [ ] ARM 中的
  - server ？
  - ashia  linux

## [ ] 当 CPU 被 hotunplug 之后，会在 /proc/cpuinfo 中看到不同吗？
