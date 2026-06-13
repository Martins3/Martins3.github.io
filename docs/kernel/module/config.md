### Selects 的含义

```txt
Symbol: TASKS_TRACE_RCU [=y]
Type  : bool
Defined at kernel/rcu/Kconfig:127
Selects: IRQ_WORK [=y]
Selected by [y]:
  - TRACEPOINTS [=y]
  - BPF_SYSCALL [=y]
  - UPROBES [=y] && ARCH_SUPPORTS_UPROBES [=y]
Selected by [n]:
  - FORCE_TASKS_TRACE_RCU [=n] && RCU_EXPERT [=n]
```

当前的配置的一个现象，可以发现，kernel 中，selects 表示

```txt
Symbol: IRQ_DOMAIN [=y]                                                                                                                                                       │
Type  : bool                                                                                                                                                                  │
Defined at kernel/irq/Kconfig:60                                                                                                                                              │
Selected by [y]:                                                                                                                                                              │
  - IRQ_DOMAIN_HIERARCHY [=y]                                                                                                                                                 │
Selected by [m]:                                                                                                                                                              │
  - I2C [=m]
```
同时被 select 为多个模式，结果可以，安装要求最高的那个


### 为什么还有一堆 if else

```txt
  │ Symbol: IRQ_DOMAIN_HIERARCHY [=y]                                                                                                                                             │
  │ Type  : bool                                                                                                                                                                  │
  │ Defined at kernel/irq/Kconfig:70                                                                                                                                              │
  │ Selects: IRQ_DOMAIN [=y]                                                                                                                                                      │
  │ Selected by [y]:                                                                                                                                                              │
  │   - GENERIC_MSI_IRQ [=y]                                                                                                                                                      │
  │   - X86_LOCAL_APIC [=y] && (X86_64 [=y] || SMP [=y] || X86_32_NON_STANDARD [=n] || X86_UP_APIC [=n] || PCI_MSI [=y])                                                          │
  │ Selected by [n]:                                                                                                                                                              │
  │   - GENERIC_IRQ_IPI [=n] && SMP [=y]                                                                                                                                          │
  │   - SPMI_HISI3670 [=n] && SPMI [=n] && HAS_IOMEM [=y]                                                                                                                         │
  │   - SPMI_MSM_PMIC_ARB [=n] && SPMI [=n] && (ARCH_QCOM || COMPILE_TEST [=n]) && HAS_IOMEM [=y]                                                                                 │
  │   - PINCTRL_MSM [=n] && PINCTRL [=n] && (ARCH_QCOM || COMPILE_TEST [=n]) && GPIOLIB [=n] && OF [=n]                                                                           │
  │   - PINCTRL_QCOM_SPMI_PMIC [=n] && PINCTRL [=n] && (ARCH_QCOM || COMPILE_TEST [=n]) && OF [=n] && SPMI [=n]                                                                   │
  │   - PINCTRL_QCOM_SSBI_PMIC [=n] && PINCTRL [=n] && (ARCH_QCOM || COMPILE_TEST [=n]) && OF [=n]                                                                                │
  │   - PINCTRL_RZG2L [=n] && PINCTRL [=n] && OF [=n]                                                                                                                             │
  │   - PINCTRL_STM32 [=n] && PINCTRL [=n] && (ARCH_STM32 || COMPILE_TEST [=n]) && OF [=n]                                                                                        │
  │   - GPIO_IXP4XX [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && ARCH_IXP4XX && OF [=n]                                                                                              │
  │   - GPIO_LPC18XX [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && OF_GPIO [=n] && (ARCH_LPC18XX || COMPILE_TEST [=n])                                                                │
  │   - GPIO_SIFIVE [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && OF_GPIO [=n]                                                                                                        │
  │   - GPIO_TEGRA [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && (ARCH_TEGRA || COMPILE_TEST [=n]) && OF_GPIO [=n]                                                                    │
  │   - GPIO_TEGRA186 [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && (ARCH_TEGRA_186_SOC [=n] || ARCH_TEGRA_194_SOC [=n] || ARCH_TEGRA_234_SOC [=n] || COMPILE_TEST [=n]) && OF_GPIO [ │
  │   - GPIO_THUNDERX [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && (ARCH_THUNDER || 64BIT [=y] && COMPILE_TEST [=n]) && PCI_MSI [=y]                                                 │
  │   - GPIO_UNIPHIER [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && (ARCH_UNIPHIER || COMPILE_TEST [=n]) && OF_GPIO [=n]                                                              │
  │   - GPIO_VISCONTI [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && (ARCH_VISCONTI || COMPILE_TEST [=n]) && OF_GPIO [=n]                                                              │
  │   - GPIO_XGENE_SB [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && (ARCH_XGENE || COMPILE_TEST [=n])                                                                                 │
  │   - GPIO_MSC313 [=n] && GPIOLIB [=n] && HAS_IOMEM [=y] && ARCH_MSTARV7                                                                                                        │
  │   - MFD_PM8XXX [=n] && HAS_IOMEM [=y] && (ARM || HEXAGON || COMPILE_TEST [=n])                                                                                                │
  │   - SOC_TEGRA_PMC [=n]                                                                                                                                                        │
  │   - ARM_GIC [=n] && OF [=n]                                                                                                                                                   │
  │   - ARM_GIC_V3 [=n]                                                                                                                                                           │
  │   - ARM_NVIC [=n]                                                                                                                                                             │
  │   - DW_APB_ICTL [=n]                                                                                                                                                          │
  │   - RENESAS_RZA1_IRQC [=n]                                                                                                                                                    │
  │   - RENESAS_RZG2L_IRQC [=n]                                                                                                                                                   │
  │   - RENESAS_RZV2H_ICU [=n]                                                                                                                                                    │
  │   - SUN6I_R_INTC [=n]                                                                                                                                                         │
  │   - MIPS_GIC [=n]                                                                                                                                                             │
  │   - STM32MP_EXTI [=n] && (ARCH_STM32 && !ARM_SINGLE_ARMV7M || COMPILE_TEST [=n])                                                                                              │
  │   - QCOM_IRQ_COMBINER [=n] && ARCH_QCOM && ACPI [=y]
```

### 被 select 的，会成为其他的 config 的 depends ，感觉 select 和 depends 就是两个体系
这个感觉是错的，其实正确的思路是，当 kernel 的

## 这个自己写的其实还是问题很大
但是 make menuconfig 的都是可以精确的计算出来每一个 config 的关系，
其实真的没有必要自己搞了

## 如何展示
https://anvaka.github.io/pm/#/?_k=8ot83f : 这个太复杂了

如果可以展示 url 就可以了:
https://d3js.org/d3-force

类似 nix-tree 来显示 config 之间的依赖关系
- https://github.com/utdemir/nix-tree
  - https://github.com/craigmbooth/nix-visualize

https://github.com/nix-community/nix-melt : 似乎是参考这个最好了

也许看看 make menuconfig 的实现，也是不错的

https://notes.dsebastien.net/50+Resources/56+Obsidian+Publish/README

## 看看可不可以用 ftrace 实现函数的调用路线图 (不是 config 了)
似乎有一定的合理性，可以仿照 blktrace 的实现和 function graph 的实现

也是函数的调用路线图。如何利用 function graph 去掉重复的，对于任何一个函数，提供任何一个 backtrace ，而不是谁调用的他。


## 也许还可以用当前系统中的配置，来自动生成一下 config 出来

## 想法

1. 可以吧所有的 header 都分析下，看看是不是有什么依赖的图
从而来整理下驱动开发者常用的接口都是什么? 例如很多 driver 都 include 了 wrokqueue 的
2. 在制作 martins3_defconfig 的时候就发现，一些 config 没有满足，没有任何警告，其实内核应该需要有这个功能


## 先搞一些基本的内容吧

- https://www.kernel.org/doc/html/next/kbuild/kconfig-language.html
- https://habr.com/en/articles/515398/ : 有个小 demo 还是不错的
- https://opensource.com/article/18/10/kbuild-and-kconfig

## 为什么总是再重新编译
- 为什么将 ext4 从模块修改 ext4 built-in，这个项目重新编译

## 记录一下 CONFIG

### 打开 CONFIG_COMPILE_TEST 选项，效果差别很大

打开之后，直接默认打开的
```txt
< CONFIG_COMPILE_TEST=y
< CONFIG_GENERIC_IRQ_CHIP=y
< CONFIG_ARM_BRCMSTB_AVS_CPUFREQ=y
< CONFIG_ARM_MEDIATEK_CPUFREQ_HW=m
< CONFIG_ARM_S3C64XX_CPUFREQ=y
< CONFIG_ARM_S5PV210_CPUFREQ=y
< CONFIG_ARM_SPEAR_CPUFREQ=y
< CONFIG_ARM_TI_CPUFREQ=y
< CONFIG_NET_VENDOR_CIRRUS=y
< CONFIG_NET_VENDOR_FARADAY=y
< CONFIG_NET_VENDOR_FREESCALE=y
< CONFIG_NET_VENDOR_HISILICON=y
< CONFIG_NET_VENDOR_SUNPLUS=y
< CONFIG_SERIAL_8250_TEGRA=y
< CONFIG_HW_RANDOM_ATMEL=y
< CONFIG_HW_RANDOM_BCM2835=y
< CONFIG_HW_RANDOM_IPROC_RNG200=y
< CONFIG_HW_RANDOM_GEODE=y
< CONFIG_HW_RANDOM_IXP4XX=y
< CONFIG_HW_RANDOM_OMAP=y
< CONFIG_HW_RANDOM_OMAP3_ROM=y
< CONFIG_HW_RANDOM_MXC_RNGA=y
< CONFIG_HW_RANDOM_IMX_RNGC=y
< CONFIG_HW_RANDOM_INGENIC_RNG=y
< CONFIG_HW_RANDOM_INGENIC_TRNG=y
< CONFIG_HW_RANDOM_NOMADIK=y
< CONFIG_HW_RANDOM_HISI=y
< CONFIG_HW_RANDOM_XGENE=y
< CONFIG_HW_RANDOM_STM32=y
< CONFIG_HW_RANDOM_ROCKCHIP=y
< CONFIG_I2C_BRCMSTB=y
< CONFIG_PTP_1588_CLOCK_DTE=y
< CONFIG_PTP_1588_CLOCK_QORIQ=y
< CONFIG_WATCHDOG_CORE=y
< CONFIG_STM32_WATCHDOG=y
< CONFIG_MARVELL_GTI_WDT=y
< CONFIG_LEDS_NS2=y
< CONFIG_RTC_DRV_SPEAR=y
< CONFIG_RENESAS_DMA=y
< CONFIG_SH_DMAE_BASE=y
< CONFIG_TI_EDMA=y
< CONFIG_DMA_OMAP=y
< CONFIG_TI_DMA_CROSSBAR=y
< CONFIG_ARM64_PLATFORM_DEVICES=y
< CONFIG_STM32MP_EXTI=y
< CONFIG_THUNDERX2_PMU=m
< CONFIG_NVDIMM_TEST_BUILD=m
```

依赖这个选项的模块直接是接近 1000 项。

这几个项目也依赖，就很奇怪啊:
```txt
> # CONFIG_COMPILE_TEST is not set
> CONFIG_LOCALVERSION_AUTO=y

> # CONFIG_BPF_PRELOAD is not set
> # CONFIG_LTO_CLANG_FULL is not set
> # CONFIG_MODVERSIONS is not set
> CONFIG_WATCHDOG_CORE=m
> # CONFIG_DRM_I915_WERROR is not set
> # CONFIG_DRM_I915_DEBUG is not set
> # CONFIG_X86_DECODER_SELFTEST is not set
```

## 为什么有的显示是 {}

```txt
{*} IPv6 packet rejection
{M} IPv6 packet logging
```
对应的显示是:

```txt
config NF_REJECT_IPV6
	tristate "IPv6 packet rejection"
	default m if NETFILTER_ADVANCED=n
```

```txt
config IP6_NF_IPTABLES_LEGACY
	tristate "Legacy IP6 tables support"
	depends on INET && IPV6
	select NETFILTER_XTABLES
	default n
	help
	  ip6tables is a legacy packet classifier.
	  This is not needed if you are using iptables over nftables
	  (iptables-nft).
```

```txt
config RAID_ATTRS
	tristate "RAID Transport Class"
	default n
	depends on BLOCK
	depends on SCSI_MOD
	help
	  Provides RAID
```


## depends on 和 select 的关系是什么?

https://unix.stackexchange.com/questions/117521/what-is-the-difference-between-select-vs-depends-in-the-linux-kernel-kconfig

> Note that the kernel docs discourage the use of this for "visible" symbols (which can be selected/deselected by the user) or
> for symbols that themselves have dependencies, since those will not be checked.

或者说，select 的 config 总是叶子节点，无法被选中，不可以依赖

那么这里的例子就是有问题的
```txt
Symbol: SUNRPC [=m]
Type  : tristate
Defined at net/sunrpc/Kconfig:2
  Depends on: NETWORK_FILESYSTEMS [=y] && MULTIUSER [=y]
Selected by [m]:
  - NFS_FS [=m] && NETWORK_FILESYSTEMS [=y] && INET [=y] && FILE_LOCKING [=y] && MULTIUSER [=y]
  - NFSD [=m] && NETWORK_FILESYSTEMS [=y] && INET [=y] && FILE_LOCKING [=y] && FSNOTIFY [=y] && MULTIUSER [=y]
Selected by [n]:
  - NFS_COMMON_LOCALIO_SUPPORT [=n] && NETWORK_FILESYSTEMS [=y] && NFS_LOCALIO [=n]
```
这里的 NETWORK_FILESYSTEMS 和 MULTIUSER 为什么既是 Depends on ，又是 selected by 啊?

但是似乎正好是 commit 2813893f8b19 ("kernel: conditionally support non-root users, groups and capabilities")
引入的问题


这个例子居然是正确的，这里的 COMPAT 的使用地方为 :
block/ioctl.c ，对于 x86 而已，但是没有人去 select 它

```txt
config COMPAT
	def_bool y
	depends on IA32_EMULATION || X86_X32_ABI

config COMPAT_FOR_U64_ALIGNMENT
	def_bool y
	depends on COMPAT
```

这个结果也很奇怪:
```txt
  │ Symbol: MODULE_SIG [=n]                                                                                                          │
  │ Type  : bool                                                                                                                     │
  │ Defined at kernel/module/Kconfig:251                                                                                             │
  │   Prompt: Module signature verification                                                                                          │
  │   Depends on: MODULES [=y]                                                                                                       │
  │   Location:                                                                                                                      │
  │     -> Enable loadable module support (MODULES [=y])                                                                             │
  │ (1)   -> Module signature verification (MODULE_SIG [=n])                                                                         │
  │ Selects: MODULE_SIG_FORMAT [=n]                                                                                                  │
  │ Selected by [n]:                                                                                                                 │
  │   - SECURITY_LOCKDOWN_LSM [=n] && SECURITY [=n] && MODULES [=y]
```
这里的 Selected by 就是我一直无法理解的东西
1. 搜 config ，只有 SECURITY_LOCKDOWN_LSM 是 select 的，如果想要打开
2. 这个被 select ，但是依旧 Prompt 出来了
3. 这里的 && 是什么

甚至可以什么都不用管，然后就可以打开这个项目了
```txt
    Symbol: MODULE_SIG [=y]                                                                                                          │
  │ Type  : bool                                                                                                                     │
  │ Defined at kernel/module/Kconfig:251                                                                                             │
  │   Prompt: Module signature verification                                                                                          │
  │   Depends on: MODULES [=y]                                                                                                       │
  │   Location:                                                                                                                      │
  │     -> Enable loadable module support (MODULES [=y])                                                                             │
  │ (1)   -> Module signature verification (MODULE_SIG [=y])                                                                         │
  │ Selects: MODULE_SIG_FORMAT [=y]                                                                                                  │
  │ Selected by [n]:                                                                                                                 │
  │   - SECURITY_LOCKDOWN_LSM [=n] && SECURITY [=n] && MODULES [=y]
```

### visible 是通过什么控制的?
是看这东西是不是有 menu 吗?

## 使用 sat 求解器来分析内核的 kconfig 是否合理

我知道很复杂，但是没想到这么复杂

https://mp.weixin.qq.com/s/WgkN0L3bVUmnYPzOlKKVOw
https://github.com/paulgazz/kmax

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
