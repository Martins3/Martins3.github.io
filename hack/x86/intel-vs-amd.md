# Intel 和 AMD 的差异

## kvm

## mce
```txt
config X86_MCE_INTEL
	def_bool y
	prompt "Intel MCE features"
	depends on X86_MCE && X86_LOCAL_APIC
	help
	  Additional support for intel specific MCE features such as
	  the thermal monitor.

config X86_MCE_AMD
	def_bool y
	prompt "AMD MCE features"
	depends on X86_MCE && X86_LOCAL_APIC && AMD_NB
	help
	  Additional support for AMD specific MCE features such as
	  the DRAM Error Threshold.
```

## cpu flags 中的内容可以逐个查询下

## 电源管理的方式不同

## iommu

## 相同地方

intel_idle 驱动
