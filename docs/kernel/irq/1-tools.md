# 中断相关实验

## docs/kernel/sysfs-irq.md

此外 irqstat 工具来分析 /proc/interrupts

## qemu hmp

### info lapic
```txt
(qemu) info lapic
dumping local APIC state for CPU 0

LVT0     0x00010700 active-hi edge  masked                      ExtINT (vec 0)
LVT1     0x00000400 active-hi edge                              NMI
LVTPC    0x00010000 active-hi edge  masked                      Fixed  (vec 0)
LVTERR   0x000000fe active-hi edge                              Fixed  (vec 254)
LVTTHMR  0x00010000 active-hi edge  masked                      Fixed  (vec 0)
LVTT     0x000400ec active-hi edge                 tsc-deadline Fixed  (vec 236)
Timer    DCR=0x0 (divide by 2) initial_count = 0 current_count = 0
SPIV     0x000001ff APIC enabled, focus=off, spurious vec 255
ICR      0x000000fd physical edge de-assert no-shorthand
ICR2     0x01000000 cpu 16777216 (X2APIC ID)
ESR      0x00000000
ISR      (none)
IRR      (none)

APR 0x00 TPR 0x10 DFR 0x0f LDR 0x00 PPR 0x10
```
aarch64 有类似的机制吗，例如 info gic 之类的

### info pic

```txt
(qemu) info pic
ioapic0: ver=0x11 id=0x00 sel=0x22 (redir[9])
  pin 0  0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 1  0x0000000000000021 dest=0 vec=33  active-hi edge         fixed  physical
  pin 2  0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 3  0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 4  0x0100000000000021 dest=1 vec=33  active-hi edge         fixed  physical
  pin 5  0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 6  0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 7  0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 8  0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 9  0x0100000000008025 dest=1 vec=37  active-hi level        fixed  physical
  pin 10 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 11 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 12 0x0100000000000020 dest=1 vec=32  active-hi edge         fixed  physical
  pin 13 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 14 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 15 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 16 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 17 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 18 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 19 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 20 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 21 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 22 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  pin 23 0x0000000000010000 dest=0 vec=0   active-hi edge  masked fixed  physical
  IRR      (none)
  Remote IRR (none)
pic0: irr=13 imr=ff isr=00 hprio=0 irq_base=30 rr_sel=0 elcr=00 fnm=0
pic1: irr=12 imr=ff isr=00 hprio=0 irq_base=38 rr_sel=0 elcr=0c fnm=0
```

## 既然所有的中断都是在内核，那么就不该有人来调用 kvm_set_irq ，让系统是安静的时候
在 console 中执行 dmesg ，然后
```sh
sudo perf top -e kvm:kvm_set_irq
```

```txt
Samples: 672K of event 'kvm:kvm_set_irq', 1 Hz, Event count (approx.): 38414 lost: 0/0 drop: 0/0
Overhead  Trace output
  82.65%  gsi 0 level 0 source 0
   9.01%  gsi 4 level 0 source 0
   8.34%  gsi 4 level 1 source 0
```

以上内容说明:
1. Vector domain 中 hwirq 的内容和 kvm:kvm_set_irq 中观测到的 gsi 是一个东西.

这里推测 gsi 就是 ioapic 或者 pic 的引脚。

```txt
@[
    kvm_pic_set_irq+313
    kvm_pic_set_irq+313
    kvm_set_irq+192
    kvm_vm_ioctl_irq_line+39
    kvm_vm_ioctl+719
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 45408
```

- kvm_check_and_inject_events
  - kvm_cpu_has_injectable_intr
  - kvm_cpu_get_interrupt


在 kvm_cpu_get_interrupt 中，这里的判断都是需要
检查两次，是 pic 和 lpaic 都需要考虑的。


类似的:
```c
int kvm_apic_accept_pic_intr(struct kvm_vcpu *vcpu)
{
	u32 lvt0 = kvm_lapic_get_reg(vcpu->arch.apic, APIC_LVT0);

	if (!kvm_apic_hw_enabled(vcpu->arch.apic))
		return 1;
	if ((lvt0 & APIC_LVT_MASKED) == 0 &&
	    GET_APIC_DELIVERY_MODE(lvt0) == APIC_MODE_EXTINT)
		return 1;
	return 0;
}
```

物理机中设置的位置: disconnect_bsp_APIC

对应 sdm 11.1 中的:
**Locally connected I/O devices** — These interrupts originate as an edge or level asserted by an I/O device
that is connected directly to the processor’s local interrupt pins (LINT0 and LINT1). The I/O devices may also
be connected to an 8259-type interrupt controller that is in turn connected to the processor through one of the
local interrupt pins.

8259-type 现在也可以作为一个 APIC 的来源。

setup_local_APIC 中看，默认是打开的

```c
	if (!cpu && (pic_mode || !value || ioapic_is_disabled)) {
		value = APIC_DM_EXTINT;
		apic_printk(APIC_VERBOSE, "enabled ExtINT on CPU#%d\n", cpu);
	} else {
		value = APIC_DM_EXTINT | APIC_LVT_MASKED;
		apic_printk(APIC_VERBOSE, "masked ExtINT on CPU#%d\n", cpu);
	}
```

总结: PIC 在 kvm 中模拟的场景，首先模拟，然后通过 apic 的 ExtINT 注入进去。


### ioapic 还是可以工作的，为什么 irq routing 将 pic 忽视了
```txt
@[
    vmx_deliver_interrupt+320
    __apic_accept_irq+244
    kvm_irq_delivery_to_apic_fast+320
    kvm_irq_delivery_to_apic+103
    ioapic_service+311
    ioapic_set_irq+308
    kvm_ioapic_set_irq+101
    kvm_set_irq+192
    kvm_vm_ioctl_irq_line+39
    kvm_vm_ioctl+719
    __x64_sys_ioctl+148
    do_syscall_64+183
    entry_SYSCALL_64_after_hwframe+119
]: 3222
```

因为 pic 的注入需要 apicv 没有打开，但是 ioapic 是可以忽视这个。

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
