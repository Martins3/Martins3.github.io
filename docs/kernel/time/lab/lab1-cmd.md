# Linux time system 基本观察
<!-- 5ed3c401-e200-4113-a967-6ec9e9aadf56 -->

## clocksource

```sh
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource
```

```txt
tsc
tsc hpet acpi_pm
```

13900K intel 机器上:

虚拟机:
```txt
kvm-clock
kvm-clock tsc hpet acpi_pm
```

物理机:
```txt
tsc
tsc acpi_pm
```

- amd 机器上

虚拟机:

```txt
kvm-clock
kvm-clock tsc acpi_pm
```

物理机:
```txt
hpet
hpet acpi_pm
```

- [ ] 这里的 hpet 看来是可以被控制的，是可以不去使用这个 hpet 的

arm 机器上:
```txt
🧀  cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource

arch_sys_counter
arch_sys_counter

```

### [ ] 需要测试，让 guest os 的时钟为 tsc ，然后热迁移
到两个开机时间有差距的机器上，看看 guest 的时间如何跳变的

## clockevents
ls /sys/devices/system/clockevents/

给虚拟机配置的 maxcpu 64 ，可以观察到:
```txt
 broadcast     clockevent8    clockevent17   clockevent26   clockevent35   clockevent44   clockevent53   clockevent62
 clockevent0   clockevent9    clockevent18   clockevent27   clockevent36   clockevent45   clockevent54   clockevent63
 clockevent1   clockevent10   clockevent19   clockevent28   clockevent37   clockevent46   clockevent55   power
 clockevent2   clockevent11   clockevent20   clockevent29   clockevent38   clockevent47   clockevent56   uevent
 clockevent3   clockevent12   clockevent21   clockevent30   clockevent39   clockevent48   clockevent57
 clockevent4   clockevent13   clockevent22   clockevent31   clockevent40   clockevent49   clockevent58
 clockevent5   clockevent14   clockevent23   clockevent32   clockevent41   clockevent50   clockevent59
 clockevent6   clockevent15   clockevent24   clockevent33   clockevent42   clockevent51   clockevent60
 clockevent7   clockevent16   clockevent25   clockevent34   clockevent43   clockevent52   clockevent61
```

其中只有一个有用的文件:
```txt
🧀  cat broadcast/current_device
hpet

🧀  cat clockevent0/current_device
lapic-deadline
```

## [ ] date -s 如何工作的

```txt
 date -s "$(date -d '+2 hour' '+%F %T')"
```

```txt
➜  c ./clock_time.out
CLOCK_REALTIME : 1717740820.376 (19881 days +  6h 13m 40s)
CLOCK_TAI      : 1717740820.376 (19881 days +  6h 13m 40s)
CLOCK_MONOTONIC:         78.562 ( 0h  1m 18s)
CLOCK_BOOTTIME :         78.562 ( 0h  1m 18s)
CLOCK_MONOTONIC_RAW:         78.480 ( 0h  1m 18s)
➜  c ./clock_time.out
CLOCK_REALTIME : 1717748230.966 (19881 days +  8h 17m 10s)
CLOCK_TAI      : 1717748230.966 (19881 days +  8h 17m 10s)
CLOCK_MONOTONIC:        290.026 ( 0h  4m 50s)
CLOCK_BOOTTIME :        290.026 ( 0h  4m 50s)
CLOCK_MONOTONIC_RAW:        289.953 ( 0h  4m 49s)
```

CLOCK_MONOTONIC_RAW 不会变化。

## sleep
strace sleep 1

## time

## dmesg


## TODO

firecracker 中为什么会自动的切换的 clocksource 为 tsc 的

```txt
🧀  dmesg | grep tsc
[    0.000006] tsc: Detected 2995.200 MHz processor
[    0.134989] clocksource: tsc-early: mask: 0xffffffffffffffff max_cycles: 0x2b2c8ec87c7, max_idle_ns: 440795278598 ns
[    0.169768] clocksource: tsc: mask: 0xffffffffffffffff max_cycles: 0x2b2c8ec87c7, max_idle_ns: 440795278598 ns
[    0.170335] clocksource: Switched to clocksource tsc
```

```txt
🧀  cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource

tsc
tsc kvm-clock

```

如果 arch/x86_64/tsc.c 中 case 5 之后:
```txt
cat /sys/devices/system/clocksource/clocksource0/current_clocksource
cat /sys/devices/system/clocksource/clocksource0/available_clocksource

kvm-clock
kvm-clock
```
实在有趣

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
