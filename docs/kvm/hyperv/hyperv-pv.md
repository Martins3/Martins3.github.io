# Hyperv Enlightment
https://libvirt.org/formatdomain.html

![](./img/hyperv.png)


|-----------------|------------------------------------------------------------------------|-----------------------------------------------|------------------------------------------------|
| relaxed         | Relax constraints on timers                                            | on, off                                       | 1.0.0 (QEMU 2.0)                               |
| vapic           | Enable virtual APIC                                                    | on, off                                       | 1.1.0 (QEMU 2.0)
| spinlocks       | Enable spinlock support                                                | on, off; retries - at least 4095             | 1.1.0 (QEMU 2.0)
| vpindex         | Virtual processor index                                                | on, off                                       | 1.3.3 (QEMU 2.5)
| runtime         | Processor time spent on running guest code and on behalf of guest code | on, off                                       | 1.3.3 (QEMU 2.5)                               |
| synic           | Enable Synthetic Interrupt Controller (SynIC)                          | on, off                                       | 1.3.3 (QEMU 2.6)                               |
| stimer          | Enable SynIC timers, optionally with Direct Mode support               | on, off; direct - on,off                      | 1.3.3 (QEMU 2.6), direct mode 5.7.0 (QEMU 4.1) |
| reset           | Enable hypervisor reset                                                | on, off                                       | 1.3.3 (QEMU 2.5)
| vendor_id       | Set hypervisor vendor id                                               | on, off; value - string, up to 12 characters | 1.3.3 (QEMU 2.5)                               |
| frequencies     | Expose frequency MSRs                                                  | on, off                                       | 4.7.0 (QEMU 2.12)
| reenlightenment | Enable re-enlightenment notification on migration                      | on, off                                       | 4.7.0 (QEMU 3.0)
| tlbflush        | Enable PV TLB flush support                                            | on, off                                       | 4.7.0 (QEMU 3.0)
| ipi             | Enable PV IPI support                                                  | on, off                                       | 4.10.0 (QEMU 3.1)
| evmcs           | Enable Enlightened VMCS                                                | on, off                                       | 4.10.0 (QEMU 3.1)
| avic            | Enable use Hyper-V SynIC with hardware APICv/AVIC                      | on, off                                       | 8.10.0 (QEMU 6.2)

- vapic
  - [ ] 只有 pv eoi 的功能吗?
  - [ ] pv eoi 后总是 wrmsr ，这样的优化有意义吗?
- vapic 和 synic avic 是什么关系?
- 如何理解 evmcs ?
- reenlightenment ?

- https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/reference/tlfs
- [一些简单的介绍](https://archive.fosdem.org/2019/schedule/event/vai_enlightening_kvm/attachments/slides/2860/export/events/attachments/vai_enlightening_kvm/slides/2860/vkuznets_fosdem2019_enlightening_kvm.pdf)

## QEMU 的文档 : docs/system/i386/hyperv.rst

``hv-vpindex``
  Provides HV_X64_MSR_VP_INDEX (0x40000002) MSR to the guest which has Virtual
  processor index information. This enlightenment makes sense in conjunction with
  hv-synic, hv-stimer and other enlightenments which require the guest to know its
  Virtual Processor indices (e.g. when VP index needs to be passed in a
  hypercall).

实际上实现的很简单: 返回 kvm_vcpu_hv::vp_index

似乎是 kvm_hv_vcpu_init 中初始化:
```sh
	hv_vcpu->vp_index = vcpu->vcpu_idx;
```

``hv-synic``
  Enables Hyper-V Synthetic interrupt controller - an extension of a local APIC.
  When enabled, this enlightenment provides additional communication facilities
  to the guest: SynIC messages and Events. This is a pre-requisite for
  implementing VMBus devices (not yet in QEMU). Additionally, this enlightenment
  is needed to enable Hyper-V synthetic timers. SynIC is controlled through MSRs
  HV_X64_MSR_SCONTROL..HV_X64_MSR_EOM (0x40000080..0x40000084) and
  HV_X64_MSR_SINT0..HV_X64_MSR_SINT15 (0x40000090..0x4000009F)

  Requires: ``hv-vpindex``

- 似乎和 apic 性能提升没有关系，additional communication facilities
- [ ] 为什么 SynIC 是 VMBus devices 依赖的
- [ ] 为什么 Hyper-V synthetic timers 依赖

``hv-passthrough``
  In some cases (e.g. during development) it may make sense to use QEMU in
  'pass-through' mode and give Windows guests all enlightenments currently
  supported by KVM. This pass-through mode is enabled by "hv-passthrough" CPU
  flag.

  Note: ``hv-passthrough`` flag only enables enlightenments which are known to QEMU
  (have corresponding 'hv-' flag) and copies ``hv-spinlocks`` and ``hv-vendor-id``
  values from KVM to QEMU. ``hv-passthrough`` overrides all other 'hv-' settings on
  the command line. Also, enabling this flag effectively prevents migration as the
  list of enabled enlightenments may differ between target and destination hosts.

``hv-enforce-cpuid``
  By default, KVM allows the guest to use all currently supported Hyper-V
  enlightenments when Hyper-V CPUID interface was exposed, regardless of if
  some features were not announced in guest visible CPUIDs. ``hv-enforce-cpuid``
  feature alters this behavior and only allows the guest to use exposed Hyper-V
  enlightenments.

简单看，就是确定用 cpuid 来提供有哪些属性:
```c
static bool hv_check_msr_access(struct kvm_vcpu_hv *hv_vcpu, u32 msr)
{
	if (!hv_vcpu->enforce_cpuid)
		return true;
  // ...
}

static bool hv_check_hypercall_access(struct kvm_vcpu_hv *hv_vcpu, u16 code)
{
	if (!hv_vcpu->enforce_cpuid)
		return true;
  // ...
}
```

``hv-stimer``
  Enables Hyper-V synthetic timers. There are four synthetic timers per virtual
  CPU controlled through HV_X64_MSR_STIMER0_CONFIG..HV_X64_MSR_STIMER3_COUNT
  (0x400000B0..0x400000B7) MSRs. These timers can work either in single-shot or
  periodic mode. It is known that certain Windows versions revert to using HPET
  (or even RTC when HPET is unavailable) extensively when this enlightenment is
  not provided; this can lead to significant CPU consumption, even when virtual
  CPU is idle.

  Requires: ``hv-vpindex``, ``hv-synic``, ``hv-time``

- [ ] 测试下，真的会导致很多时间的消耗吗?
  - [ ] 无法理解，为什么 PV 真的可以处理 ipi 的问题吗?

## 先多看看代码吧

1. QEMU 中这里的代码如何理解: target/i386/kvm/hyperv.c
  - KVM_EXIT_HYPERV
  - 始终是无法感知到: synic 的含义

2. QEMU 中的 accel 目录下没有 hyperv ，但是 qemu 还是可以运行在 windows 中。
  - 代码在: target/i386/whpx/ ，忽然不是很懂，accel 目录是做什么的了


## 多写写
1. 看看 kvm_hv_set_msr 和  get_msr 的 trace ，看看不同 trace 下，主要都是在做什么?
2. 可以打开 hv balloon 吗?

## 问题
1. 为什么 kvm 没有提供 pv 操作来加速 nested kvm 的运行?
2. 时间，又是时间！

## [x] vapic 不是等价于 EOI ，而是一堆机制

这里是很清楚: https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/virtual-interrupts

```c
/* Define the virtual APIC registers */
#define HV_X64_MSR_EOI				0x40000070
#define HV_X64_MSR_ICR				0x40000071
#define HV_X64_MSR_TPR				0x40000072
#define HV_X64_MSR_VP_ASSIST_PAGE		0x40000073
```
这里做了两个优化，一个是:
1. The hypervisor provides accelerated MSR access to high usage memory mapped APIC registers. These are the TPR, EOI, and the ICR registers. The ICR low and ICR high registers are combined into one MSR.
2. EOI Assist

似乎还是有一点优化的。

在虚拟机中，先 exit 在 host 中，然后再报错
- https://speakerdeck.com/retrage/exploring-x86-msr-space?slide=4

### 可以知道 vector 47 是多少吗?

### 分析下，这三个 MSR 的使用频率是多少
HV_X64_MSR_ICR 的频率也非常高，不能简单的说是多少问题。

### HV_X64_MSR_VP_ASSIST_PAGE 是做啥来着的?

## 在 linux 来模拟 windows 的 msr pv eoi

```diff
diff --git a/arch/x86/include/asm/apic.h b/arch/x86/include/asm/apic.h
index f21ff1932699..5382262be4c2 100644
--- a/arch/x86/include/asm/apic.h
+++ b/arch/x86/include/asm/apic.h
@@ -407,7 +407,9 @@ static __always_inline void apic_write(u32 reg, u32 val)

 static __always_inline void apic_eoi(void)
 {
-	static_call(apic_call_eoi)();
+	#define HV_X64_MSR_EOI				0x40000070
+	wrmsr(HV_X64_MSR_EOI, APIC_EOI_ACK, 0);
+	/* static_call(apic_call_eoi)(); */
 }

 static __always_inline void apic_native_eoi(void)
```

启动参数为 -cpu host,hv_vapic

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
