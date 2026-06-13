# dmi

## 基本观察
13900k
```txt
Apr 23 12:23:19 nixos kernel: DMI: ASUS System Product Name/TUF GAMING B660-PLUS WIFI D4, BIOS 1620 08/12/2022
Apr 23 12:23:19 nixos kernel: DMI: Memory slots populated: 2/4
Apr 23 12:23:19 nixos kernel: tsc: Detected 3000.000 MHz processor
Apr 23 12:23:19 nixos kernel: tsc: Detected 2995.200 MHz TSC
```
```txt
bios_date:08/12/2022
bios_release:16.20
bios_vendor:American Megatrends Inc.
bios_version:1620
board_asset_tag:Default string
board_name:TUF GAMING B660-PLUS WIFI D4
```

qemu 中启动:
```txt
 sudo dmesg | grep DMI
[sudo] password for martins3:
[    0.000000] DMI: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.17.0-4-g0026c353eb4e 04/01/2014
[    0.000000] DMI: Memory slots populated: 1/1
```
```txt
martins3@linux:/sys/class/dmi/id$ grep . *
bios_date:04/01/2014
bios_release:0.0
bios_vendor:SeaBIOS
bios_version:rel-1.17.0-4-g0026c353eb4e
```

Hyper-V 虚拟机
```txt
🧀  sudo dmesg | grep DMI
[sudo] password for martins3:
[    0.000000] DMI: Microsoft Corporation Virtual Machine/Virtual Machine, BIOS Hyper-V UEFI Release v4.1 09/04/2024
[    0.000000] DMI: Memory slots populated: 2/2
```

n100 的机器:
```txt
Apr 17 00:13:01 localhost kernel: DMI: Micro Computer (HK) Tech Limited Venus Series/DNBID, BIOS 0.15 01/10/2024
Apr 17 00:13:01 localhost kernel: DMI: Memory slots populated: 4/8
Apr 17 00:13:01 localhost kernel: tsc: Detected 800.000 MHz processor
Apr 17 00:13:01 localhost kernel: tsc: Detected 806.400 MHz TSC
```

而且，这个东西可以感知到内存条有多少个在位。

## dmidecode 可以展示更加具体的 CPU 信息

920-4826
```txt
andle 0x002F, DMI type 4, 48 bytes
Processor Information
        Socket Designation: CPU02
        Type: Central Processor
        Family: ARM
        Manufacturer: HiSilicon
        ID: 10 D0 1F 48 00 00 00 00
        Signature: Implementor 0x48, Variant 0x1, Architecture 15, Part 0xd01, Revision 0
        Version: Kunpeng 920-4826
        Voltage: 0.9 V
```
```txt
processor       : 95
BogoMIPS        : 200.00
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm jscvt fcma dcpop asimddp asimdfhm
CPU implementer : 0x48
CPU architecture: 8
CPU variant     : 0x1
CPU part        : 0xd01
CPU revision    : 0
```

在另外一个机器上:
```txt
Handle 0x002F, DMI type 4, 48 bytes
Processor Information
        Socket Designation: CPU02
        Type: Central Processor
        Family: ARM
        Manufacturer: HiSilicon
        ID: 10 D0 1F 48 00 00 00 00
        Signature: Implementor 0x48, Variant 0x1, Architecture 15, Part 0xd01, Revision 0
        Version: HUAWEI Kunpeng 920 5250
        Voltage: 0.9 V
```
```txt
processor       : 95
BogoMIPS        : 200.00
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm jscvt fcma dcpop asimddp asimdfhm
CPU implementer : 0x48
CPU architecture: 8
CPU variant     : 0x1
CPU part        : 0xd01
CPU revision    : 0
```
可以发现两个机器的 /proc/cpuinfo 完全相同，但是第二个机器的 Version 是 5250


## 所以，从上面看，相当于 QEMU 还需要传递一些东西给 firmware ，最后让 firmware 来传递给操作系统
这个过程可以继续看看

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
