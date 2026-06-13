## x86 的 arch/x86/kernel/apic/vector.c
<!-- e843e2d9-a3dd-4d7b-9517-1e17e34506d8 -->

经常看到这个错误，这到底是如何触发的?
```txt
kernel: [6644841.818477] irq 18: Affinity broken due to vector space exhaustion.
kernel: [6644842.891529] irq 18: Affinity broken due to vector space exhaustion.
```

不管怎么说，只有那一个机器出现这个问题，也许不是 root cause ，也是需要分析的原因:
```txt
🧀  rg -l  "Affinity broken due to vector space exhaustion"
172_16_68_82_65754285-764d-4f83-b68c-8086bd0f471e/kernel_logs/dmesg/dmesg.journald.log
172_16_68_82_65754285-764d-4f83-b68c-8086bd0f471e/kernel_logs/messages/messages
```


确认:
```txt
History:        #0
Commit:         743dac494d61d991967ebcfab92e4f80dc7583b3
Author:         Neil Horman <nhorman@tuxdriver.com>
Committer:      Thomas Gleixner <tglx@linutronix.de>
Author Date:    Thu 22 Aug 2019 10:34:21 PM CST
Committer Date: Wed 28 Aug 2019 08:44:08 PM CST

x86/apic/vector: Warn when vector space exhaustion breaks affinity

On x86, CPUs are limited in the number of interrupts they can have affined
to them as they only support 256 interrupt vectors per CPU. 32 vectors are
reserved for the CPU and the kernel reserves another 22 for internal
purposes. That leaves 202 vectors for assignement to devices.

When an interrupt is set up or the affinity is changed by the kernel or the
administrator, the vector assignment code attempts to honor the requested
affinity mask. If the vector space on the CPUs in that affinity mask is
exhausted the code falls back to a wider set of CPUs and assigns a vector
on a CPU outside of the requested affinity mask silently.

While the effective affinity is reflected in the corresponding
/proc/irq/$N/effective_affinity* files the silent breakage of the requested
affinity can lead to unexpected behaviour for administrators.

Add a pr_warn() when this happens so that adminstrators get at least
informed about it in the syslog.

[ tglx: Massaged changelog and made the pr_warn() more informative ]

Reported-by: djuran@redhat.com
Signed-off-by: Neil Horman <nhorman@tuxdriver.com>
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
Tested-by: djuran@redhat.com
Link: https://lkml.kernel.org/r/20190822143421.9535-1-nhorman@tuxdriver.com
```

挺懵逼的，继续之前分析的问题，就是当修改 cpu 的 affinity 的时候，到底会切换什么?
```c
static const struct irq_domain_ops x86_vector_domain_ops = {
	.alloc		= x86_vector_alloc_irqs,
	.free		= x86_vector_free_irqs,
	.activate	= x86_vector_activate,
	.deactivate	= x86_vector_deactivate,
#ifdef CONFIG_GENERIC_IRQ_DEBUGFS
	.debug_show	= x86_vector_debug_show,
#endif
};
```
为什么绑定 cpu 中断亲和性的时候，不会有:
x86_vector_activate

这个不是这个的作用的

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
