# QEMU 如何模拟 pcspker


## Seabios
commit-id: 748d619be3282fba35f99446098ac2d0579f6063
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

    u64 start = rdtscll();
    while ((inb(PORT_PS2_CTRLB) & PPCB_T2OUT) == 0)
        ;
    u64 end = rdtscll();

    // Restore PORT_PS2_CTRLB
    outb(orig, PORT_PS2_CTRLB);

    // Store calibrated cpu khz.
    u64 diff = end - start;
    dprintf(6, "tsc calibrate start=%u end=%u diff=%u\n"
            , (u32)start, (u32)end, (u32)diff);
    u64 t = DIV_ROUND_UP(diff * PMTIMER_HZ, CALIBRATE_COUNT);
    while (t >= (1<<24)) {
        ShiftTSC++;
        t = (t + 1) >> 1;
    }
    TimerKHz = DIV_ROUND_UP((u32)t, 1000 * PMTIMER_TO_PIT);
    TimerPort = 0;

    dprintf(1, "CPU Mhz=%u\n", (TimerKHz << ShiftTSC) / 1000);
}
```
## Linux kernel
4.4.142 版本中
```c
static unsigned long quick_pit_calibrate(void)
{
	int i;
	u64 tsc, delta;
	unsigned long d1, d2;

	/* Set the Gate high, disable speaker */
	outb((inb(0x61) & ~0x02) | 0x01, 0x61);
```

## QEMU
分析 /hw/audio/pcspk.c 看，isa_register_soundhw 中注册的初始化代码有效果的前提
是当 QEMU 提供了 soundhw 的。
```c
static void pcspk_register(void)
{
    type_register_static(&pcspk_info);
    isa_register_soundhw("pcspk", "PC speaker", pcspk_audio_init);
}
```

在 monitor 中 info mtree -f 可以知道
```txt
FlatView #4
 AS "I/O", root: io
 Root memory region: io
  0000000000000061-0000000000000061 (prio 0, i/o): pcspk
```

[^1]: https://techpiezo.com/linux/enable-audio-in-qemu-virtual-machine/
