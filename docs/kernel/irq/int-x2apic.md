## 文档

sdm :
- 13.12 Extended xAPIC (x2APIC)
	包括使能方式、MSR 地址空间、xAPIC -> x2APIC 差异、ICR、SELF IPI 等

## xAPIC 和 x2APIC 的直接区别
<!-- 702e08d4-72ce-460c-b82b-17307738a9b5 -->

1. xAPIC
	- 通过 MMIO 访问 APIC 页。
	- 典型基址是 `0xFEE00000`。
	- `ICR` 是两个 32 位寄存器。
	- `DFR` 存在。
2. x2APIC
	- 不再走 MMIO，改成 `RDMSR/WRMSR` 访问 `0x800-0x8FF`。
	- `APIC ID` 扩展到 32 位。
	- `DFR` 被取消。
	- `ICR` 合并成一个 64 位 MSR。
	- 新增 `SELF IPI` MSR。
	- 手册明确说明：`x2APIC` 下对 APIC MSR 的 `WRMSR` 序列化语义被放宽，访问效率更高。

(之前记录了一些有趣的文档，这个还是挺有意思的)
## amd 中如何打开 x2apic
amd
```txt
🧀  dmesg | grep x2apic
[    0.129273] x2apic: IRQ remapping doesn't support X2APIC mode
```

intel
```txt
🧀  dmesg | grep x2apic
[    0.181268] DMAR-IR: Queued invalidation will be enabled to support x2apic and Intr-remapping.
[    0.182790] DMAR-IR: Enabled IRQ remapping in x2apic mode
[    0.182791] x2apic enabled
[    0.182814] Switched APIC routing to cluster x2apic.
```

## x2apic 在 AMD 中如何打开

https://stackoverflow.com/questions/60219639/kernel-error-irq-remapping-doesnt-support-x2apic-mode-disabled-x2apic

## 基本介绍

https://serverfault.com/questions/873664/when-to-use-x2apic-mode

> When enabled, processor x2APIC support helps operating systems run more efficiently on high core count configurations and optimizes interrupt distribution in virtualized environments. Enabled mode only provides the support necessary to the operating system. So if you have a multiprocessor system, use virtualization and also know that the operating system that you use supports APIC, you can enable the option.

## patch
https://patchwork.ozlabs.org/project/qemu-devel/patch/1456413312-24063-1-git-send-email-tianyu.lan@intel.com/


## 到底用那个 apic chip 的，老哥!

amd
```txt
[    0.094408] APIC: Switch to symmetric I/O mode setup
[    0.095191] APIC: Switched APIC routing to: physical flat
```

amd 虚拟机中居然是:
```txt
[    0.202029] APIC: Switch to symmetric I/O mode setup
[    0.202962] APIC: Switched APIC routing to: physical x2apic
```

intel
```txt
🧀  dmesg | grep Switch
[    0.172184] APIC: Switch to symmetric I/O mode setup
[    0.173756] APIC: Switched APIC routing to: cluster x2apic
```
虚拟机中的:
```txt
[    0.142128] APIC: Switched APIC routing to: physical x2apic
```

hygon 机器上:
```txt
[    3.900415] APIC: Switch to symmetric I/O mode setup
[    4.453118] Switched APIC routing to physical flat.
```

这里我猜测是 : amd 中根本没有打开 x2apic ，所以 amd 中是 physical flat ，后续的各种优化也没法做。

## 看看
arch/x86/kernel/apic/apic_flat_64.c 中定义两个:
```c
static struct apic apic_flat __ro_after_init = {
	.name				= "flat",
```

```c
static struct apic apic_physflat __ro_after_init = {

	.name				= "physical flat",
	.probe				= physflat_probe,
	.acpi_madt_oem_check		= physflat_acpi_madt_oem_check,
```

arch/x86/kernel/apic/apic_noop.c

调用路线是:
- secondary_startup_64
  - x86_64_start_kernel
    - x86_64_start_reservations
      - start_kernel
        - x86_late_time_init
          - apic_intr_mode_init
            - x86_64_probe_apic
              - x2apic_phys_probe :

## physical x2apic 和 cluster x2apic : iommu 而已
try_to_enable_x2apic 中可以看到，这个和 iommu 有关系:

```c
		/*
		 * Without IR, all CPUs can be addressed by IOAPIC/MSI only
		 * in physical mode, and CPUs with an APIC ID that cannot
		 * be addressed must not be brought online.
		 */
		x2apic_set_max_apicid(apic_limit);
		x2apic_phys = 1;
```

但是关闭 iommu 后，这个问题也没有消失，先不管了，反正就是 x2apic 就是以上两种中的一个。

## 是不是可以说明，当 dmesg 为 physical flat ，就不是使用 x2apic mode

```txt
[    0.094913] APIC: Switched APIC routing to: physical flat
```

amd 中 但是 cat /proc/cpuinfo 的输出显示还是有 x2apic 的

## 一大堆的 apic 是做什么的
<!-- d2589932-19b3-4140-baa1-e4446dddee02 -->

而且 64 位下 probe 顺序就是编译顺序，Makefile 直接写了注释，见 arch/x86/kernel/apic/Makefile：

- apic_numachip
- x2apic_uv_x
- x2apic_savic
- x2apic_phys
- x2apic_cluster
- apic_flat_64

越靠前越“特殊”，最后的 apic_flat_64 是兜底默认。

先分清两条维度

xAPIC / x2APIC 这一维：

- xAPIC：LAPIC 寄存器经 MMIO 访问，见 arch/x86/include/asm/apic.h:97
- x2APIC：LAPIC 寄存器经 MSR 访问，见 arch/x86/include/asm/apic.h:205  physflat / cluster / UV ... 这一维：

- 决定 APIC ID 怎么解释
- IPI 发给谁、怎么发
- 次级 CPU 怎么唤醒
- 某些平台是否要走特殊硬件路径

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
