# 感觉，实际上，arm 也是没问题的

## 简单分析 arm 的实现
mac 上，arm 的实现在: drivers/clocksource/arm_arch_timer.c

```c
static struct clocksource clocksource_counter = {
	.name	= "arch_sys_counter",
	.id	= CSID_ARM_ARCH_COUNTER,
	.rating	= 400,
	.read	= arch_counter_read,
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};
```



## https://docs.kernel.org/virt/kvm/arm/ptp_kvm.html

ptp_kvm
https://raw.githubusercontent.com/kata-containers/packaging/refs/heads/master/kernel/patches/4.19.x/0001-4.19-enable-ptp_kvm-for-arm64-in-kata.patch


## 看来 arm 的时钟不简单啊

http://www.wowotech.net/timer_subsystem/armgeneraltimer.html#comment-8869

## [ ] 所以，从原则上来说，arm 热迁移是有问题才对
应该是存在问题的，热迁移是虚拟化无感知的，但是频率的确是修改了的。


## arm 是如何将 frequently 传递过去的？
arch_timer_acpi_init
```c
	arch_timer_rate = arch_timer_get_cntfrq(); // 是通过访问一个寄存器获取的
	ret = validate_timer_rate();
```

kernel 中的启动代码 : arch_timer_banner
```txt
[    0.000000] arch_timer: cp15 timer(s) running at 24.00MHz (virt).
[    0.000000] clocksource: arch_sys_counter: mask: 0xffffffffffffff max_cycles: 0x588fe9dc0, max_idle_ns: 440795202592 ns
[    0.000000] sched_clock: 56 bits at 24MHz, resolution 41ns, wraps every 4398046511097ns
```

```txt
#0  arch_timer_banner (type=1) at drivers/clocksource/arm_arch_timer.c:1080
#1  0xffff800081a7d90c [PAC] in arch_timer_common_init () at drivers/clocksource/arm_arch_timer.c:1376
#2  0xffff800081a7d688 [PAC] in arch_timer_acpi_init (table=0xffffffffff476818) at drivers/clocksource/arm_arch_timer.c:1805
#3  0xffff800081a5d868 [PAC] in acpi_table_parse (id=id@entry=0xffff800081b39178 <__acpi_probe_arch_timer> "GTDT", handler=0xffff800081a7d4e4 <arch_timer_acpi_init>) at drivers/acpi/tables.c:331
#4  0xffff800081a5f38c [PAC] in __acpi_probe_device_table (ap_head=<optimized out>, nr=1) at drivers/acpi/scan.c:2767
#5  0xffff800081a7d008 [PAC] in timer_probe () at drivers/clocksource/timer-probe.c:41
#6  0xffff800081a24dc4 [PAC] in time_init () at arch/arm64/kernel/time.c:60
#7  0xffff800081a20588 [PAC] in start_kernel () at init/main.c:992
#8  0xffff800081a2a688 [PAC] in __primary_switched () at arch/arm64/kernel/head.S:243
```
居然走的是 acpi ，实在是有趣啊。

## 分析这些需要对于 arm 更加深入的理解了
https://stackoverflow.com/questions/71981223/modify-the-value-of-cntfrq-el0-in-arm-kvm-for-the-guest-to-boot

## PTP_KVM
arm 也是有时间同步的机制的:
- https://docs.kernel.org/virt/kvm/arm/ptp_kvm.html


## 看看 arm 环境中这里 pvtime 的实现原理是什么，为什么要使用
```txt
(qemu) info ramblock
              Block Name    PSize              Offset               Used              Total                HVA  RO
                    mem0   16 KiB  0x0000000000000000 0x0000000200000000 0x0000000200000000 0x0000fffce2000000  rw
             virt.flash0   16 KiB  0x0000000200000000 0x0000000004000000 0x0000000004000000 0x0000fffcd6000000  rw
             virt.flash1   16 KiB  0x0000000204000000 0x0000000004000000 0x0000000004000000 0x0000fffcd0000000  rw
    /rom@etc/acpi/tables   16 KiB  0x0000000208100000 0x0000000000020000 0x0000000000200000 0x0000ffff08c00000  ro
0000:00:02.0/virtio-net-pci.rom   16 KiB  0x0000000208040000 0x0000000000040000 0x0000000000040000 0x0000ffff14600000  ro
0000:00:03.0/virtio-net-pci.rom   16 KiB  0x0000000208080000 0x0000000000040000 0x0000000000040000 0x0000ffff14400000  ro
0000:00:04.0/virtio-net-pci.rom   16 KiB  0x00000002080c0000 0x0000000000040000 0x0000000000040000 0x0000ffff14200000  ro
   /rom@etc/table-loader   16 KiB  0x0000000208300000 0x0000000000004000 0x0000000000010000 0x0000ffff08a00000  ro
      /rom@etc/acpi/rsdp   16 KiB  0x0000000208340000 0x0000000000004000 0x0000000000010000 0x0000ffff08800000  ro
                  pvtime   16 KiB  0x0000000208000000 0x0000000000004000 0x0000000000004000 0x0000ffff18200000  rw
```
此外，对比一下 info ramblock 在 x86 中的 seabios 和 UEFI 环境中的实现。

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
