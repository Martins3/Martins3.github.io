# QEMU tcg 的中断模拟
下面的代码暂时分析基于 v6.1.0-rc2 的 x64

## 问题
- [ ] make a table explaining every field of APIC and IOAPIC struct in QEMU

- [ ] 似乎没有整明白 : gsi_handler 和 qemu_set_irq 的关系
  - [ ] qdev_connect_gpio_out 等价的效果到底是什么
  - [ ] 比如 pic_realize 中的注册

- apic timer : 总体来说，timer 是比较容易处理的
  - apic_timer 被周期性的触发
    - [ ] 思考一下如何获取 clock time, 实际上，guest 操作系统可以主动校准实践
    - QEMU_CLOCK_VIRTUAL : 当虚拟机停下来的时候，时钟中断需要停止下来
  - 考虑一个小问题，所有的 vCPU 都是需要接受 local timer 的时钟的，难道为此需要创建出来多个 timer 吗 ?
    - 是的, 而且 timer 这个线程是在 main_loop_wait => qemu_clock_run_all_timers 中使用一个新的线程来进行的

- apic_update_irq 的分析
  - apic_poll_irq : 如果中断是来自于其他的 thread，那么就采用这种方式，比如时钟中断
    - 因为时钟是在另一个线程处理的，所以需要实现
  - 如果不是来自于 pic 的中断，那就清理掉这个中断

## isa
- [ ] 实际上，ISABus 中的只有成员 irq 有用，而且是通过 isa_bus_irqs 赋值的，实际上，没有啥作用的
  - 实际上，我猜测 ISABus 主要是为了 keyboard 之类的设备容易模拟吧

将 `x86ms->gsi` 赋值给 ISABus::irqs

```c
void isa_bus_irqs(ISABus *bus, qemu_irq *irqs)
{
    bus->irqs = irqs;
}
```

#### EOI
- [ ] apic_eoi : 和 10.8.5 中描述的一致，当 apic 接受到一个 EOIUpon receiving an EOI, the APIC clears the highest priority bit in the ISR and dispatches the next highest priority
interrupt to the processor.
  - [ ] 10.8.5 : 手册中间分析的 ioapic 的 broadcast 是什么意思
  - [x] apic_sync_vapic : 这个是处理 kvm 的，暂时不分析

*If the terminated interrupt was a level-triggered interrupt, the local APIC Also sends an
end-of-interrupt message to all I/O APICs.* (**无法理解为什么 level-triggered 的就需要向 io apic 发送**)

System software may prefer to direct EOIs to specific I/O APICs rather than having the local APIC send end-of-interrupt messages to all I/O APICs.

Software can inhibit the broadcast of EOI message by setting bit 12 of the *Spurious Interrupt Vector Register* (see
Section 10.9). If this bit is set, a broadcast EOI is not generated on an EOI cycle even if the associated *TMR* bit indicates that the current interrupt was level-triggered.
The default value for the bit is 0, indicating that EOI broadcasts are performed.

Bit 12 of the Spurious Interrupt Vector Register is reserved to 0 if the processor does not support suppression of
EOI broadcasts. Support for EOI-broadcast suppression is reported in bit 24 in the Local APIC Version Register (see
Section 10.4.8); the feature is supported if that bit is set to 1. When supported, the feature is available in both
xAPIC mode and x2APIC mode.

System software desiring to perform directed EOIs for level-triggered interrupts should set bit 12 of the *Spurious Interrupt Vector Register* and follow each the EOI to the local xAPIC for a level triggered interrupt with a directed
EOI to the I/O APIC generating the interrupt (this is done by writing to the I/O APIC’s EOI register).
System software performing directed EOIs must retain a mapping associating level-triggered interrupts with the I/O APICs in the system. (**并没有看懂这个英语，是如何实现 dedicated 的 EOI 的**)

- [ ] 实际上，ioapic 也是存在 eoi 的, 而且还在两个调用位置, 放到 tcg ioapic 中间分析吧

#### lvt
lvt  中的取值总是在发生改变的, 但是
apic_timer => apic_local_deliver => apic_set_irq 的过程中，本来 apic 的中断是 APIC_LVT_TIMER 的，但是最后装换为 236 了

```txt
[0=236] [1=65536] [2=65536] [3=67328] [4=1024] [5=254]
```

- [ ] 解释一下 nvme 中地址空间的内容
```txt
      00000000febf0000-00000000febf3fff (prio 1, i/o): nvme-bar0
        00000000febf0000-00000000febf1fff (prio 0, i/o): nvme
        00000000febf2000-00000000febf240f (prio 0, i/o): msix-table
        00000000febf3000-00000000febf300f (prio 0, i/o): msix-pba
```

[^1]: https://events.static.linuxfound.org/sites/events/files/slides/VT-d%20Posted%20Interrupts-final%20.pdf
[^2]: https://luohao-brian.gitbooks.io/interrupt-virtualization/content/qemu-kvm-zhong-duan-xu-ni-hua-kuang-jia-fen-679028-4e0a29.html

> In general, one pin is used for chaining the legacy PIC and the other for NMIs (Or occasionally SMIs).
> No devices are actually connected to the LINT pins for a couple of reasons, one of which is that id just doesn't work on multiprocessor systems (They inevitably get delivered to one core).
> In practice, all devices are connected to the I/O APIC and/or PIC.[^4]

> The local APIC is enabled at boot-time and can be disabled by clearing bit 11 of the IA32_APIC_BASE Model Specific Register (MSR). [^3]

[^3]: https://wiki.osdev.org/APIC
[^4]: https://forum.osdev.org/viewtopic.php?f=1&t=22024
