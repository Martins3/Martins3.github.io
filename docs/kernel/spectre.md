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
