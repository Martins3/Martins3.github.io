# seabios 中也会校准时间

基本流程如下:
```txt
   platform_hardware_setup()                     src/post.c:139
    ├─ qemu_platform_setup()                     src/fw/paravirt.c:285
    │   ├─ kvmclock_init()  (内联)               paravirt.c:105
    │   │   └─ tsctimer_setfreq(2994*1000,"kvmclock")  paravirt.c:134  ★断点1 命中
    │   └─ pci_setup()                           pciinit.c:1240
    │       └─ pci_bios_init_devices()           pciinit.c:429
    │           └─ pci_bios_init_device()        pciinit.c:414
    │               └─ pci_init_device(表分发, 0xd7685 是 `jmp *%ecx` 尾调用)
    │                   └─ piix4_pm_setup()      pciinit.c:266   (i440fx 的 PIIX4 PM 0x7113)
    │                       └─ pmtimer_setup(acpi_pm_base+0x08) pciinit.c:272 ★断点2 命中(early return)
    ├─ timer_setup()                             post.c:153   ← TimerPort 已是 0，直接 return
    └─ clock_setup() → pit_setup()               clock.c:36   (PIT 中断初始化)
```

## 一个持续 5 年的错误

### seabios

这是 seabios 的源码:
```c
// Calibrate the CPU time-stamp-counter
static void
tsctimer_setup(void)
{
    // Setup "timer2"
    u8 orig = inb(PORT_PS2_CTRLB);
    outb((orig & ~PPCB_SPKR) | PPCB_T2GATE, PORT_PS2_CTRLB);
    /* binary, mode 0, LSB/MSB, Ch 2 */
    outb(PM_SEL_TIMER2|PM_ACCESS_WORD|PM_MODE0|PM_CNT_BINARY, PORT_PIT_MODE);
    /* LSB of ticks */
    outb(CALIBRATE_COUNT & 0xFF, PORT_PIT_COUNTER2);
    /* MSB of ticks */
    outb(CALIBRATE_COUNT >> 8, PORT_PIT_COUNTER2);
    // ...
}
```
从这里看，似乎和 PPCB_SPKR 有关，也就是 speaker 有关的

### 内核中对应的代码
```c
static unsigned long quick_pit_calibrate(void)
{
  int i;
  u64 tsc, delta;
  unsigned long d1, d2;

  /* Set the Gate high, disable speaker */
  outb((inb(0x61) & ~0x02) | 0x01, 0x61);
```

### QEMU 中观察

在 monitor 中 info mtree -f 可以知道
```txt
FlatView #1
 AS "I/O", root: io
 Root memory region: io
  0000000000000000-0000000000000007 (prio 0, i/o): dma-chan
  0000000000000008-000000000000000f (prio 0, i/o): dma-cont
  0000000000000010-000000000000001f (prio 0, container): io @0000000000000010
  0000000000000020-0000000000000021 (prio 0, i/o): kvm-pic
  0000000000000022-000000000000003f (prio 0, container): io @0000000000000022
  0000000000000040-0000000000000043 (prio 0, i/o): kvm-pit
  0000000000000044-000000000000005f (prio 0, container): io @0000000000000044
  0000000000000060-0000000000000060 (prio 0, i/o): i8042-data
  0000000000000061-0000000000000061 (prio 0, i/o): pcspk
```

分析 /hw/audio/pcspk.c isa_register_soundhw 中注册的初始化代码有效果的前提是当 QEMU 提供了 soundhw 的。

```c
static void pcspk_register(void)
{
    type_register_static(&pcspk_info);
    isa_register_soundhw("pcspk", "PC speaker", pcspk_audio_init);
}
```
### 错误的结论

由此，我得出结论，始终校准和 speaker 有关，这显然是一个很扯的结论，不过我当时的组会上提到过这个事情，
我的导师认为不对，不过这是一个细枝末节的问题，我们再也没深究了。
2026-09-21 我用 deepseek flash v4.1 分析，确认结论如下:


本质就是在用 PIT —— 具体是 PIT 的通道 2（channel 2）。但"用 speaker 来校准 TSC"这个说法要纠正一下：
这里被当作秒表的是 8253/8254 PIT 通道 2 的计数器，speaker 只是通道 2 输出信号的下游消费者之一，而我们恰恰把 PPCB_SPKR 清掉了，就是让它别出声。

数据通路

```
   14.31818 MHz 晶振 ──/12──> 1.193182 MHz
                                  │
                                  ▼
                          ┌───────────────┐
                          │ PIT ch2 计数  │  ← 装入 0x800 (2048) 个计数
                          └───────┬───────┘
                                  │ OUT (计数到 0 时翻转)
                                  ├──────────────> 0x61 bit5 (T2OUT 状态, 只读)
                                  │                    ▲
                                  │                    └── 代码在这里忙等，这就是"秒表读数"
                                  ▼
                          AND( gate(bit0) , SPKR(bit1) )
                                  │
                                  ▼
                              喇叭放大器  ← 被 ~PPCB_SPKR 静音，不参与校准
```

所以：

- 计时基准 = PIT ch2 的 2048 个计数 ≈ 1.72 ms（注释里的 “Approx 1.7ms”）。
- 读数手段 = 轮询 0x61 bit5，看它什么时候翻转。
- speaker = 只是 gate 和 SPKR 两个位做 AND 之后的分支，被拉低就哑了，对测量没有任何作用。

换句话说：不是用 speaker 校准，而是"顺手把 speaker 关掉"再借用它背后的那个定时器。

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
