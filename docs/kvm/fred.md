## 什么是 FRED ?

https://cdrdv2-public.intel.com/779982/346446-flexible-return-and-event-delivery.pdf

handle_external_interrupt_irqoff
```c
	if (cpu_feature_enabled(X86_FEATURE_FRED))
		fred_entry_from_kvm(EVENT_TYPE_EXTINT, vector);
	else
		vmx_do_interrupt_irqoff(gate_offset((gate_desc *)host_idt_base + vector));
```
但是 13900k 中没有 enable


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
