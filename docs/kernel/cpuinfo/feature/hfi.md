## 基本观察
cpuid -l 6 -r

## 其中关于 leaf 7 的 edx 的部分
Bits 31-16: Index (starting at 0) of this logical processor's row in the hardware feedback interface structure.
Note that on some parts the index may be same for multiple logical processors. On some parts the
indices may not be contiguous, i.e., there may be unused rows in the hardware feedback interface structure.



https://docs.kernel.org/6.12/arch/x86/intel-hfi.html
drivers/thermal/intel/intel_hfi.c
Documentation/arch/x86/intel-hfi.rst

```c
/**
 * struct hfi_cpu_info - Per-CPU attributes to consume HFI data
 * @index:		Row of this CPU in its HFI table
 * @hfi_instance:	Attributes of the HFI table to which this CPU belongs
 *
 * Parameters to link a logical processor to an HFI table and a row within it.
 */
struct hfi_cpu_info {
	s16			index;
	struct hfi_instance	*hfi_instance;
};
```
看来这个 index 有大用啊

最后通过这个发送出去的，看文档也是这么回事
thermal_genl_cpu_capability_event

## 关于 HFI 更多参考 : SDM V3 16.6


## 直到今天才注意到，中断是有这个的
```txt
 TRM:       1412       1412       1414       1414       1412       1412       1478       1478       1419       1419       1534       1534       1419       1419       1466       1466       1412       1412       1412       1412       1412       1412       1412       1412       1412       1412       1412       1412       1412       1412       1412       1412   Thermal event interrupts
```

intel_thermal_interrupt

如果有 hfi ，最后会调用到 intel_hfi_process_event 中去。

应该是 	rdmsr(MSR_IA32_PACKAGE_THERM_STATUS, msr); 来解决问题

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
