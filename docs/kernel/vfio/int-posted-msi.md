## PWN : Posted MSI notification event
<!-- 78e9da73-4d18-4dfd-9c08-dc32c681fb33 -->

https://lore.kernel.org/all/20240423174114.526704-1-jacob.jun.pan@linux.intel.com/ (这里讲的更加清楚一点)

配套的 lpc (2023) : Improve Xeon IRQ throughput with posted interrupt
https://lpc.events/event/17/sessions/172/#20231115
	- https://lpc.events/event/17/contributions/1420/attachments/1198/2711/LPC2023%20CID%20Posted%20MSI%20final.pdf

> IOMMU Interrupt remapping (IR) is required and turned on by default to support X2APIC

Device MSI to CPU HW Flow (remappable mode)
1.Devices issue interrupt requests with writes to 0xFEEx_xxxx
2.The system agent accepts the IRQ and remaps/translates based on Interrupt Remapping Table Entries (IRTE)
3.Upon receiving the translation response, the system agent notifies the CPU with the translated MSI
4.CPU’s local APIC accepts the MSI into its IRR/ISR registers
5.IDT delivery to the OS IRQ handler (MSI vector)

Device MSI to CPU HW Flow (posted mode)
3. Notifies the destination CPU with a notification vector
	- IOMMU suppresses CPU notification
	- IOMMU atomic swap IRQ status to memory (PID)
4. CPU's local APIC accepts the notification interrupt into its IRR/ISR registers
5. Interrupt delivered through IDT (notification vector handler)
6. System SW allows new notifications.

Note:
- 1 & 2 are the same as the remappable mode
- APIC only sees notification vectors but not MSI vectors

这里问了一下 chatgpt ，进一步解释了一下:
- PID (Posted Interrupt Descriptor)


```txt
History:        #0
Commit:         1b03d82ba15e895776f1f7da2bb56a9a60e6dfed
Author:         Jacob Pan <jacob.jun.pan@linux.intel.com>
Committer:      Thomas Gleixner <tglx@linutronix.de>
Author Date:    Wed 24 Apr 2024 01:41:10 AM CST
Committer Date: Tue 30 Apr 2024 06:54:42 AM CST

x86/irq: Install posted MSI notification handler

All MSI vectors are multiplexed into a single notification vector when
posted MSI is enabled. It is the responsibility of the notification vector
handler to demultiplex MSI vectors. In the handler the MSI vector handlers
are dispatched without IDT delivery for each pending MSI interrupt.

For example, the interrupt flow will change as follows:
(3 MSIs of different vectors arrive in a a high frequency burst)

BEFORE:
interrupt(MSI)
    irq_enter()
    handler() /* EOI */
    irq_exit()
        process_softirq()
interrupt(MSI)
    irq_enter()
    handler() /* EOI */
    irq_exit()
        process_softirq()
interrupt(MSI)
    irq_enter()
    handler() /* EOI */
    irq_exit()
        process_softirq()

AFTER:
interrupt /* Posted MSI notification vector */
    irq_enter()
	atomic_xchg(PIR)
	handler()
	handler()
	handler()
	pi_clear_on()
    apic_eoi()
    irq_exit()
        process_softirq()

Except for the leading MSI, CPU notifications are skipped/coalesced.

For MSIs which arrive at a low frequency, the demultiplexing loop does not
wait for more interrupts to coalesce. Therefore, there's no additional
latency other than the processing time.

Signed-off-by: Jacob Pan <jacob.jun.pan@linux.intel.com>
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
Link: https://lore.kernel.org/r/20240423174114.526704-9-jacob.jun.pan@linux.intel.com
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
