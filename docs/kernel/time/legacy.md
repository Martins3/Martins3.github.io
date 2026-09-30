## arch/x86/kvm/i8254.c

```txt
ps -elf | grep kvm

1 I root     2488068       2  0  60 -20 -     0 -      12:10 ?        00:00:00 [kvm]
1 S root     2488069       2  0  80   0 -     0 -      12:10 ?        00:00:00 [kvm-nx-lpage-recovery-2488065]
1 S root     2488079       2  0  80   0 -     0 -      12:10 ?        00:00:00 [kvm-pit/2488065]
```

但是使用 pit_do_work ，开机一共就使用几次而已:
```txt
sudo bpftrace -e "kprobe:pit_do_work { print("hit kprobe:pit_do_work") }"
```

感觉没啥用途，只是为了模拟一个绝对 legacy 的设备而已。

但是，qemu 中也是有 pit 的啊!

## timer 中断
```c
/*
 * Default timer interrupt handler for PIT/HPET
 */
static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
    global_clock_event->event_handler(global_clock_event);
    return IRQ_HANDLED;
}

static void __init setup_default_timer_irq(void)
{
    unsigned long flags = IRQF_NOBALANCING | IRQF_IRQPOLL | IRQF_TIMER;

    /*
     * Unconditionally register the legacy timer interrupt; even
     * without legacy PIC/PIT we need this for the HPET0 in legacy
     * replacement mode.
     */
    if (request_irq(0, timer_interrupt, flags, "timer", NULL))
        pr_info("Failed to register legacy timer interrupt\n");
}
```
就对应的，这里把名称 timer 换成了 gg
```txt
🧀  cat /proc/interrupts
           CPU0       CPU1       CPU2       CPU3       CPU4       CPU5       CPU6       CPU7
  0:         34          0          0          0          0          0          0          0  IO-APIC   2-edge      gg
```
实际上就是开机的时候有用的，之后不会触发了


## 现在看，tsc 就是事实，为什么这里还写了这么多废话?
- [PSA: If your clocksource is HPET rather than TSC you may see severely crippled performance in games (DX11 especially)](https://www.reddit.com/r/linux_gaming/comments/rsvjqb/psa_if_your_clocksource_is_hpet_rather_than_tsc/)
- [Misconceptions about timers (HPET, TSC, PMT...)](https://sites.google.com/view/melodystweaks/misconceptions-about-timers-hpet-tsc-pmt)
- [the industry shift back toward favouring TSC.](https://news.ycombinator.com/item?id=16922495) - https://www.anandtech.com/show/12678/a-timely-discovery-examining-amd-2nd-gen-ryzen-results


## pit 会导致 nmi 中断注入

https://unix.stackexchange.com/questions/216925/nmi-received-for-unknown-reason-20-do-you-have-a-strange-power-saving-mode-ena

这个让现在内核无需检查:
commit c8c4076723da ("x86/timer: Skip PIT initialization on modern chipsets")

这里真的让人感觉乱七八糟的

## 看看 acpi_pm ，的确是一个时钟源的

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
