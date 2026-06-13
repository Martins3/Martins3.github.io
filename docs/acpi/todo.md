## 内核启动过程中是如何探测 acpi 的 是需要特殊的硬件吗?

不然如何发送中断？

## sysfs 下的如何探测的
13900k
```txt
/sys/bus/acpi
/sys/firmware/acpi
/sys/module/acpi
```

n100 下:
```txt
/sys/kernel/debug/acpi
/sys/bus/acpi
/sys/firmware/acpi
/sys/module/acpi
```

这么多设备如何探测的:
```txt
 ABCD0000:00   ACPI0007:28   device:5d   device:44     INT3420:00    PNP0C0B:01
 ACPI000C:00   ACPI0007:29   device:5e   device:45     INT3471:00    PNP0C0B:02
 ACPI000E:00   ACPI0007:30   device:5f   device:46     INT3472:0a    PNP0C0B:03
 ACPI0007:0a   ACPI0007:31   device:6a   device:47     INT3472:0b    PNP0C0B:04
 ACPI0007:0b   ACPI0007:32   device:6b   device:48     INT3472:0c    PNP0C0C:00
 ACPI0007:0c   ACPI0007:33   device:6c   device:49     INT3472:00    PNP0C0D:00
 ACPI0007:0d   ACPI0007:34   device:6d   device:50     INT3472:01    PNP0C0E:00
 ACPI0007:0e   ACPI0007:35   device:6e   device:51     INT3472:02    PNP0C0F:00
 ACPI0007:0f   ACPI0007:36   device:6f   device:52     INT3472:03    PNP0C0F:01
 ACPI0007:00   ACPI0007:37   device:7a   device:53     INT3472:04    PNP0C0F:02
 ACPI0007:01   ACPI0007:38   device:7b   device:54     INT3472:05    PNP0C0F:03
 ACPI0007:02   ACPI0007:39   device:7c   device:55     INT3472:06    PNP0C0F:04
 ACPI0007:03   device:0a     device:7d   device:56     INT3472:07    PNP0C0F:05
 ACPI0007:04   device:0b     device:7e   device:57     INT3472:08    PNP0C0F:06
 ACPI0007:05   device:0c     device:7f   device:58     INT3472:09    PNP0C0F:07
 ACPI0007:06   device:0d     device:8a   device:59     INT3474:00    PNP0C02:00
 ACPI0007:07   device:0e     device:8b   device:60     INT3515:00    PNP0C02:01
 ACPI0007:08   device:0f     device:8c   device:61     INT3515:01    PNP0C02:02
 ACPI0007:09   device:00     device:8d   device:62     INT3515:02    PNP0C02:03
 ACPI0007:1a   device:01     device:8e   device:63     INT3515:03    PNP0C02:04
 ACPI0007:1b   device:02     device:8f   device:64     INT3519:00    PNP0C02:05
 ACPI0007:1c   device:03     device:10   device:65     INT3533:00    PNP0C02:06
 ACPI0007:1d   device:04     device:11   device:66     INTC1001:00   PNP0C04:00
 ACPI0007:1e   device:05     device:12   device:67     INTC1057:00   PNP0C08:00
 ACPI0007:1f   device:06     device:13   device:68     INTC1070:00   PNP0C08:01
 ACPI0007:2a   device:07     device:14   device:69     INTC1090:00   PNP0C08:02
 ACPI0007:2b   device:08     device:15   device:70     INTC1092:00   PNP0C08:03
 ACPI0007:2c   device:09     device:16   device:71     INTC1099:00   PNP0C08:04
 ACPI0007:2d   device:1a     device:17   device:72     LNXPOWER:00   PNP0C08:05
 ACPI0007:2e   device:1b     device:18   device:73     LNXPOWER:01   PNP0C08:06
 ACPI0007:2f   device:1c     device:19   device:74     LNXPOWER:02   PNP0C08:07
 ACPI0007:3a   device:1d     device:20   device:75     LNXPOWER:03   PNP0C09:00
 ACPI0007:3b   device:1e     device:21   device:76     LNXPOWER:04   PNP0C14:00
 ACPI0007:3c   device:1f     device:22   device:77     LNXPOWER:05   PNP0C14:01
 ACPI0007:3d   device:2a     device:23   device:78     LNXPOWER:06   PNP0C50:00
 ACPI0007:3e   device:2b     device:24   device:79     LNXPWRBN:00   PNP0C50:01
 ACPI0007:3f   device:2c     device:25   device:80     LNXSYBUS:00   PNP0C50:02
 ACPI0007:10   device:2d     device:26   device:81     LNXSYBUS:01   PNP0C50:03
 ACPI0007:11   device:2e     device:27   device:82     LNXSYSTM:00   PNP0000:00
 ACPI0007:12   device:2f     device:28   device:83     LNXTHERM:00   PNP0100:00
 ACPI0007:13   device:3a     device:29   device:84     LNXVIDEO:00   PNP0103:00
 ACPI0007:14   device:3b     device:30   device:85     MCHP1930:00   PRP00001:00
 ACPI0007:15   device:3c     device:31   device:86     MCHP1930:01   PWRC0000:00
 ACPI0007:16   device:3d     device:32   device:87     MCHP1930:02   PWRC0000:01
 ACPI0007:17   device:3e     device:33   device:88     MCHP1930:03   PWRC0000:02
 ACPI0007:18   device:3f     device:34   device:89     MSFT0101:00   TXNW3643:00
 ACPI0007:19   device:4a     device:35   DUMY0000:00   OVTI01AS:00   TXNW3643:01
 ACPI0007:20   device:4b     device:36   DUMY0000:01   OVTI01AS:01   TXNW3643:02
 ACPI0007:21   device:4c     device:37   INT00000:00   OVTID858:00   USBC000:00
 ACPI0007:22   device:4d     device:38   INT33A1:00    PNP0A08:00    XXXX0000:00
 ACPI0007:23   device:4e     device:39   INT33BE:00    PNP0B00:00    XXXX0000:01
 ACPI0007:24   device:4f     device:40   INT33BE:01    PNP0C0A:00    XXXX0000:02
 ACPI0007:25   device:5a     device:41   INT33BE:02    PNP0C0A:01    XXXX0000:03
 ACPI0007:26   device:5b     device:42   INT33E1:00    PNP0C0A:02
 ACPI0007:27   device:5c     device:43   INT340E:00    PNP0C0B:00
```

猜测是，使用一个表格就可以了。
## 似乎 mac 就不用 acpi 的

```txt
/sys/bus/platform/drivers/acpi-fan
/sys/bus/platform/drivers/acpi-ged
```

## 观察
find /sys/devices/LNXSYSTM\:00/ 下的各种文件。


```txt
[root@dogfood-idc-elf-19-21-NVME-M2 15:29:42 log]$ cat /sys/devices/LNXSYSTM:00/LNXSYBUS:00/ACPI000D:00/name
power_meter
```

## 这个文档写的很好的
https://uefi.org/specs/ACPI/6.5/18_Platform_Error_Interfaces.html#ghes-assist-on-machine-check-architecture

## 可以重新看看内核的日志，理解一下问题
https://docs.kernel.org/firmware-guide/acpi/apei/einj.html

## 重点关注
https://blogsystem5.substack.com/p/hardware-autoconfiguration?utm_source=substack&publication_id=2042083&post_id=158141927&utm_medium=email&utm_content=share&utm_campaign=email-share&triggerShare=true&isFreemail=true&r=3ot3d&triedRedirect=true

这个人很强，可以持续关注一下


## 看看 kernel 的 reboot 参数
```txt
  reboot=         [KNL]
```

## 开机的启动的日志
```txt
[    0.000000] ACPI: Early table checksum verification disabled
[    0.000000] ACPI: RSDP 0x000000002F9C0000 000024 (v02 HISI  )
[    0.000000] ACPI: XSDT 0x000000002F9B0000 0000B4 (v01 HISI   HIP08    00000000      01000013)
[    0.000000] ACPI: FACP 0x000000002F3E0000 000114 (v06 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: DSDT 0x000000002F160000 00DADE (v02 HISI   HIP08    00000000 INTL 20181213)
[    0.000000] ACPI: PCCT 0x000000002F9A0000 00008A (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: SSDT 0x000000002F990000 00ABB8 (v02 HISI   HIP07    00000000 INTL 20181213)
[    0.000000] ACPI: BERT 0x000000002F870000 000030 (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: HEST 0x000000002F850000 00058C (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: ERST 0x000000002F810000 000230 (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: EINJ 0x000000002F7F0000 000170 (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: GTDT 0x000000002F3C0000 00007C (v02 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: SDEI 0x000000002F1C0000 000030 (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: MCFG 0x000000002F1B0000 00003C (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: SLIT 0x000000002F1A0000 00003C (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: SPCR 0x000000002F190000 000050 (v02 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: SRAT 0x000000002F180000 0007D0 (v03 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: APIC 0x000000002F170000 001E6C (v04 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: IORT 0x000000002F150000 001678 (v00 HISI   HIP08    00000000 INTL 20181213)
[    0.000000] ACPI: PPTT 0x0000000026580000 0031B0 (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: MPAM 0x0000000026570000 0005C4 (v01 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: SPMI 0x0000000026560000 000041 (v05 HISI   HIP08    00000000 HISI 20151124)
[    0.000000] ACPI: SPCR: console: uart,mmio,0x3f00002f8,115200
[    0.000000] ACPI: SRAT: Node 0 PXM 0 [mem 0x2080000000-0x2fffffffff]
[    0.000000] ACPI: SRAT: Node 1 PXM 1 [mem 0x3000000000-0x3fffffffff]
[    0.000000] ACPI: SRAT: Node 0 PXM 0 [mem 0x00000000-0x7fffffff]
[    0.000000] ACPI: SRAT: Node 2 PXM 2 [mem 0x202000000000-0x202fffffffff]
[    0.000000] ACPI: SRAT: Node 3 PXM 3 [mem 0x203000000000-0x203fffffffff]
[    0.000000] NUMA: NODE_DATA [mem 0x2fffff8980-0x2fffffffff]
[    0.000000] NUMA: NODE_DATA [mem 0x3fffff8980-0x3fffffffff]
[    0.000000] NUMA: NODE_DATA [mem 0x202fffff8980-0x202fffffffff]

```

## acpi 会编码机器上的 PCI 设备吗?

我理解应该不会，不然插拔掉 PCI 之后，咋办?

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
