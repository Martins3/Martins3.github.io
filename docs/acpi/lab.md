## 使用 acpidump 来观察
参考 : https://01.org/linux-acpi/utilities

```sh
sudo acpidump > acpidump.out
acpixtract -a acpidump.out
iasl -d apic.dat
nvim apic.dsl

iasl -d hpet.dat
nvim hpet.dsl
```

在物理机中测试，测试可以得到这些东西:
```txt
  SSDT -   13113 bytes written (0x00003339) - ssdt1.dat
  APIC -     476 bytes written (0x000001DC) - apic.dat
  SSDT -   23819 bytes written (0x00005D0B) - ssdt2.dat
  MCFG -      60 bytes written (0x0000003C) - mcfg1.dat
  TPM2 -      76 bytes written (0x0000004C) - tpm2.dat
  SSDT -    7778 bytes written (0x00001E62) - ssdt3.dat
  NHLT -      45 bytes written (0x0000002D) - nhlt.dat
  DSDT -  493489 bytes written (0x000787B1) - dsdt.dat
  SSDT -   10883 bytes written (0x00002A83) - ssdt4.dat
  WSMT -      40 bytes written (0x00000028) - wsmt.dat
  SSDT -     324 bytes written (0x00000144) - ssdt5.dat
  LPIT -     204 bytes written (0x000000CC) - lpit.dat
  SSDT -    7962 bytes written (0x00001F1A) - ssdt6.dat
  DBG2 -      84 bytes written (0x00000054) - dbg2.dat
  SSDT -   15082 bytes written (0x00003AEA) - ssdt7.dat
  SSDT -   10913 bytes written (0x00002AA1) - ssdt8.dat
  MCFG -      60 bytes written (0x0000003C) - mcfg2.dat
  DMAR -     136 bytes written (0x00000088) - dmar.dat
  FACP -     276 bytes written (0x00000114) - facp.dat
  FPDT -      68 bytes written (0x00000044) - fpdt.dat
  WPBT -      64 bytes written (0x00000040) - wpbt.dat
  PHAT -    1521 bytes written (0x000005F1) - phat.dat
  SSDT -     908 bytes written (0x0000038C) - ssdt9.dat
  SSDT -    3103 bytes written (0x00000C1F) - ssdt10.dat
  DBGP -      52 bytes written (0x00000034) - dbgp.dat
  HPET -      56 bytes written (0x00000038) - hpet.dat
  SSDT -    9047 bytes written (0x00002357) - ssdt11.dat
  FIDT -     156 bytes written (0x0000009C) - fidt.dat
  FACS -      64 bytes written (0x00000040) - facs.dat
  BGRT -      56 bytes written (0x00000038) - bgrt.dat
  SSDT -   14810 bytes written (0x000039DA) - ssdt12.dat
  SSDT -    4152 bytes written (0x00001038) - ssdt13.dat
  SSDT -    1210 bytes written (0x000004BA) - ssdt14.dat
  SSDT -    1541 bytes written (0x00000605) - ssdt15.dat
  SSDT -    4027 bytes written (0x00000FBB) - ssdt16.dat
  SSDT -    4937 bytes written (0x00001349) - ssdt17.dat
  SSDT -    7087 bytes written (0x00001BAF) - ssdt18.dat
  SSDT -     427 bytes written (0x000001AB) - ssdt19.dat
  SSDT -     916 bytes written (0x00000394) - ssdt20.dat
```

> [!NOTE]
> 参考 Deepseeek ，有待验证
>
> APIC: 高级可编程中断控制器表，描述中断控制器配置。
> BGRT: Boot Graphics Resource Table，包含引导时显示的图形信息（如 logo）。
> DBG2: Debug Port Table 2，定义调试端口（用于串口或其他调试接口）。
> DBGP: Debug Port Table，早期调试端口表，通常与 DBG2 类似。
> DMAR: DMA Remapping Table，用于支持 Intel VT-d 或 AMD-Vi 的 DMA 重映射。
> DSDT: Differentiated System Description Table，核心表，描述系统硬件和设备。
> FACP: Fixed ACPI Description Table，包含硬件寄存器和电源管理信息。
> FACS: Firmware ACPI Control Structure，存储固件状态信息。
> FIDT: 未知（可能是笔误，应为 FADT，即 FACP 的别名）。
> FPDT: Firmware Performance Data Table，记录固件性能数据。
> HPET: High Precision Event Timer Table，定义高精度计时器。
> LPIT: Low Power Idle Table，描述低功耗空闲状态。
> MCFG: Memory-Mapped Configuration Table，用于 PCIe 配置空间。
> NHLT: Non High Definition Audio Link Table，描述音频设备配置。
> PHAT: Platform Health Assessment Table，提供平台健康信息。
> SSDT: Secondary System Description Table，补充 DSDT 的扩展表。
> TPM2: Trusted Platform Module 2 Table，描述 TPM 2.0 设备。
> WPBT: Windows Platform Binary Table，包含 Windows 特定的二进制数据。
> WSMT: Windows SMM Security Mitigations Table，描述安全缓解措施。

先看看这些 table 中的内容还是不错的，例如从 apic.dsl 中可以看到 maxcpu 个数，ioapic 之类的，
一直，这个时候，我再就有了一个新的视角了:

```txt
apic_flat_64.c
27:static int physflat_acpi_madt_oem_check(char *oem_id, char *oem_table_id)
36:     .acpi_madt_oem_check            = physflat_acpi_madt_oem_check,

probe_64.c
31:int __init default_acpi_madt_oem_check(char *oem_id, char *oem_table_id)
36:             if ((*drv)->acpi_madt_oem_check(oem_id, oem_table_id)) {

apic_numachip.c
183:static int numachip1_acpi_madt_oem_check(char *oem_id, char *oem_table_id)
194:static int numachip2_acpi_madt_oem_check(char *oem_id, char *oem_table_id)
208:    .acpi_madt_oem_check            = numachip1_acpi_madt_oem_check,
242:    .acpi_madt_oem_check            = numachip2_acpi_madt_oem_check,

probe_32.c
17:#include <asm/acpi.h>

x2apic_cluster.c
24:static int x2apic_acpi_madt_oem_check(char *oem_id, char *oem_table_id)
231:    .acpi_madt_oem_check            = x2apic_acpi_madt_oem_check,

x2apic_uv_x.c
18:#include <linux/acpi.h>
450:static int __init uv_acpi_madt_oem_check(char *_oem_id, char *_oem_table_id)
757:    .acpi_madt_oem_check            = uv_acpi_madt_oem_check,

x2apic_phys.c
4:#include <linux/acpi.h>
30:     if ((acpi_gbl_FADT.header.revision >= FADT2_REVISION_ID) &&
31:             (acpi_gbl_FADT.flags & ACPI_FADT_APIC_PHYSICAL)) {
39:static int x2apic_acpi_madt_oem_check(char *oem_id, char *oem_table_id)
136:    .acpi_madt_oem_check            = x2apic_acpi_madt_oem_check,

apic.c
21:#include <linux/acpi_pmtmr.h>
54:#include <asm/acpi.h>
656:    u32 pm = acpi_pm_read_early();
1273:           if (!acpi_lapic) {
2361:    * for disabling it before entering apm/acpi suspend

io_apic.c
43:#include <linux/acpi.h>
59:#include <asm/acpi.h>
765:static int __acpi_get_override_irq(u32 gsi, bool *trigger, bool *polarity)
790:int acpi_get_override_irq(u32 gsi, int *is_level, int *active_low)
793:    return __acpi_get_override_irq(gsi, (bool *)is_level,
826:            if (__acpi_get_override_irq(gsi, &level, &pol_low) >= 0) {
1453:   if (acpi_ioapic)
2817:   } else if (__acpi_get_override_irq(gsi, &data->is_level, &data->active_low) < 0) {
```

## 这些工具都看看
acpi
acpibin
acpidump
acpiexamples
acpiexec
acpihelp
acpisrc
acpixtract

## Documentation/ABI/testing/sysfs-firmware-acpi
这个目录的东西看看

/sys/firmware/acpi/tables/DSDT

## TODO
https://www.kernel.org/doc/Documentation/acpi/namespace.txt
https://wiki.ubuntu.com/Kernel/Reference/ACPITricksAndTips

## hpet 也是 ACPI 控制的
公版 qemu 2.12
```txt
[000h 0000 004h]                   Signature : "HPET"    [High Precision Event Timer Table]
[004h 0004 004h]                Table Length : 00000038
[008h 0008 001h]                    Revision : 01
[009h 0009 001h]                    Checksum : 03
[00Ah 0010 006h]                      Oem ID : "BOCHS "
[010h 0016 008h]                Oem Table ID : "BXPCHPET"
[018h 0024 004h]                Oem Revision : 00000001
[01Ch 0028 004h]             Asl Compiler ID : "BXPC"
[020h 0032 004h]       Asl Compiler Revision : 00000001

[024h 0036 004h]           Hardware Block ID : 8086A201

[028h 0040 00Ch]        Timer Block Register : [Generic Address Structure]
[028h 0040 001h]                    Space ID : 00 [SystemMemory]
[029h 0041 001h]                   Bit Width : 00
[02Ah 0042 001h]                  Bit Offset : 00
[02Bh 0043 001h]        Encoded Access Width : 00 [Undefined/Legacy]
[02Ch 0044 008h]                     Address : 00000000FED00000

[034h 0052 001h]             Sequence Number : 00
[035h 0053 002h]         Minimum Clock Ticks : 0000
[037h 0055 001h]       Flags (decoded below) : 00
                             4K Page Protect : 0
                            64K Page Protect : 0

Raw Table Data: Length 56 (0x38)

    0000: 48 50 45 54 38 00 00 00 01 03 42 4F 43 48 53 20  // HPET8.....BOCHS
    0010: 42 58 50 43 48 50 45 54 01 00 00 00 42 58 50 43  // BXPCHPET....BXPC
    0020: 01 00 00 00 01 A2 86 80 00 00 00 00 00 00 D0 FE  // ................
    0030: 00 00 00 00 00 00 00 00                          // .
```

公版 qemu 9
```txt
[000h 0000 004h]                   Signature : "HPET"    [High Precision Event Timer Table]
[004h 0004 004h]                Table Length : 00000038
[008h 0008 001h]                    Revision : 01
[009h 0009 001h]                    Checksum : B4
[00Ah 0010 006h]                      Oem ID : "BOCHS "
[010h 0016 008h]                Oem Table ID : "BXPC    "
[018h 0024 004h]                Oem Revision : 00000001
[01Ch 0028 004h]             Asl Compiler ID : "BXPC"
[020h 0032 004h]       Asl Compiler Revision : 00000001

[024h 0036 004h]           Hardware Block ID : 8086A201

[028h 0040 00Ch]        Timer Block Register : [Generic Address Structure]
[028h 0040 001h]                    Space ID : 00 [SystemMemory]
[029h 0041 001h]                   Bit Width : 00
[02Ah 0042 001h]                  Bit Offset : 00
[02Bh 0043 001h]        Encoded Access Width : 00 [Undefined/Legacy]
[02Ch 0044 008h]                     Address : 00000000FED00000

[034h 0052 001h]             Sequence Number : 00
[035h 0053 002h]         Minimum Clock Ticks : 0000
[037h 0055 001h]       Flags (decoded below) : 00
                             4K Page Protect : 0
                            64K Page Protect : 0

Raw Table Data: Length 56 (0x38)

    0000: 48 50 45 54 38 00 00 00 01 B4 42 4F 43 48 53 20  // HPET8.....BOCHS
    0010: 42 58 50 43 20 20 20 20 01 00 00 00 42 58 50 43  // BXPC    ....BXPC
    0020: 01 00 00 00 01 A2 86 80 00 00 00 00 00 00 D0 FE  // ................
    0030: 00 00 00 00 00 00 00 00                          // ........
```

为什么在内核中模拟 pit ，不在内核中模拟 hpet ?

他们的区别是:
```diff
 [000h 0000 004h]                   Signature : "HPET"    [High Precision Event Timer Table]
 [004h 0004 004h]                Table Length : 00000038
 [008h 0008 001h]                    Revision : 01
-[009h 0009 001h]                    Checksum : B4
+[009h 0009 001h]                    Checksum : 03
 [00Ah 0010 006h]                      Oem ID : "BOCHS "
-[010h 0016 008h]                Oem Table ID : "BXPC    "
+[010h 0016 008h]                Oem Table ID : "BXPCHPET"
 [018h 0024 004h]                Oem Revision : 00000001
 [01Ch 0028 004h]             Asl Compiler ID : "BXPC"
 [020h 0032 004h]       Asl Compiler Revision : 00000001
```

## ACPI 如何控制电池的
<!-- 3f632c5d-b9da-43a3-aec5-eb5137c21937 -->

- https://www.zhihu.com/people/rabbitjump-39/posts
	- https://zhuanlan.zhihu.com/p/1942122164330402819 : 电池
		- https://gitlab.com/qemu-project/qemu/-/issues/242
	- https://zhuanlan.zhihu.com/p/1931837516459250831 : 风扇
- ACPI 中断注入:
	- https://mp.weixin.qq.com/s?__biz=MzkzMTk4OTIwNg==&mid=2247485338&idx=1&sn=bbf2a9c2b911b0f02844b1786a852df8&scene=21&poc_token=HKq_0mijET8jc3OPXR8mJ7MT4aNXac9dHRgcp_Eq

在 mac 中执行，但是在 13900k 中执行，显示没有这个设备:
```txt
 sudo acpi -b
[sudo] password for martins3:
Battery 0: Full, 97%
```

```txt
/sys/class/power_supply🔒 😚
lrwxrwxrwx@ - root 13 Dec 07:31 macsmc-ac -> ../../devices/platform/soc/23e400000.smc/macsmc-power/power_supply/macsmc-ac
lrwxrwxrwx@ - root 13 Dec 07:31 macsmc-battery -> ../../devices/platform/soc/23e400000.smc/macsmc-power/power_supply/macsmc-battery
lrwxrwxrwx@ - root 13 Dec 07:31 tps6598x-source-psy-0-003f -> ../../devices/platform/soc/235010000.i2c/i2c-0/0-003f/power_supply/tps6598x-source-psy-0-003f
lrwxrwxrwx@ - root 13 Dec 07:31 tps6598x-source-psy-0-0038 -> ../../devices/platform/soc/235010000.i2c/i2c-0/0-0038/power_supply/tps6598x-source-psy-0-0038
```

```txt
🧀  cat /sys/class/power_supply/macsmc-battery/capacity
100
```

仅仅添加 smbios_build_type_22_table 后，就可以观察到 smbios 的内容，但是 /sys/class/power_supply 下没有东西:
```txt
Handle 0x1400, DMI type 22, 26 bytes
Portable Battery
        Location: In the back
        Manufacturer: MyStackTrace
        Manufacture Date: 01/01/2025
        Serial Number: 123456789
        Name: Battery 0
        Chemistry: Lithium Polymer
        Design Capacity: 65535 mWh
        Design Voltage: 10800 mV
        SBDS Version: Not Specified
        Maximum Error: 5%
        OEM-specific Information: 0xCAFEBABE
```

补充上 acpi method 之后，然后就可以，即便是我们自己的内核中:

```txt
6.17.7-martins3-00001-gfd23f075a322

 acpi -b
Battery 0: Charging, 100%,  until charged

  ls -la /sys/class/power_supply
lrwxrwxrwx - root 13 Dec 07:54 BAT0 -> ../../devices/LNXSYSTM:00/LNXSYBUS:00/PNP0C0A:00/power_supply/BAT0
```

内核中的相关的代码为: drivers/acpi/battery.c

### 如何解释执行 _BIF 方法

```txt
sudo cat /sys/firmware/acpi/tables/DSDT > dsdt.dat
iasl -d dsdt.dat
grep -A20 "_BIF" dsdt.dsl
```



```txt
ASL Output:    dsdt.dsl - 129101 bytes
            Method (_BIF, 0, NotSerialized)  // _BIF: Battery Information
            {
                Name (PKG1, Package (0x0D)
                {
                    0x08,
                    0xFFFF,
                    0xFFFF,
                    One,
                    0x2A30,
                    Zero,
                    Zero,
                    0x0100,
                    0x40,
                    "T1000",
                    "123456789",
                    "Lithium Polymer",
                    "MyStackTrace"
                })
                Return (PKG1) /* \_SB_.BAT0._BIF.PKG1 */
            }

            Method (_BST, 0, NotSerialized)  // _BST: Battery Status
            {
                Name (PKG1, Package (0x04)
                {
                    0x02,
                    0xFFFF,
                    0xFFFF,
                    0x2A30
                })
                Return (PKG1) /* \_SB_.BAT0._BST.PKG1 */
            }
        }
    }

    Scope (_GPE)
    {
        Name (_HID, "ACPI0006" /* GPE Block Device */)  // _HID: Hardware ID
    }
```

### linux 内核具体如何执行函数的

还是这个例子，可以非常容易的看到

- acpi_battery_get_property
	- acpi_battery_get_state : 首先读取硬件，来刷新 battery 中的内容
		- status = acpi_evaluate_object(battery->device->handle, "_BST", NULL, &buffer); 执行 BST 方法


### 在 Hyper-V 虚拟机中观察

发现，在 Hyper-V 居然是模拟了电源的，所以有如下观察结果，这个就是可以模拟一个真实的电源管理了。
```txt
🧀  l
Permissions Size User Date Modified Name
lrwxrwxrwx     - root 13 Oct 13:08   AC1 -> ../../devices/system/container/ACPI0004:00/ACPI0003:00/power_supply/AC1
lrwxrwxrwx     - root 13 Oct 13:08   BAT1 -> ../../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/PNP0C0A:00/power_supply/BAT1
```

其中的 dsdt 的结果为，从这里可以很容易看到，首先定义 BSTE 接口，然后定义获取电源的方法为访问 BSTE 接口
```txt

        OperationRegion (BATM, SystemMemory, 0xFED3F000, 0x1000)
        Field (BATM, DWordAcc, NoLock, WriteAsZeros)
        {
            BSTA,   32,
            BSTE,   32,
            BRAT,   32,
            BCAP,   32,
            ACPS,   32,
            BNST,   32,
            BNCL,   32
        }

        Device (\_SB.VMOD.BAT1)
        {
            Name (BIX, Package (0x14)
            {
                Zero,
                Zero,
                0x1388,
                0x1388,
                One,
                0x1388,
                0x01F4,
                0xFA,
                0xFFFFFFFF,
                0x2710,
                0xFFFFFFFF,
                0xFFFFFFFF,
                0x03E8,
                0x64,
                0x0A,
                0x0A,
                "Microsoft Hyper-V Virtual Battery",
                "Virtual",
                "Virtual Battery",
                ""
            })
            Name (BIXE, Package (0x14)
            {
                Zero,
                Zero,
                0xFFFFFFFF,
                0xFFFFFFFF,
                One,
                0xFFFFFFFF,
                Zero,
                Zero,
                0xFFFFFFFF,
                0x2710,
                0xFFFFFFFF,
                0xFFFFFFFF,
                0x03E8,
                0x64,
                0x0A,
                0x0A,
                "",
                "",
                "",
                ""
            })
            Name (_CID, "Virtual Battery")  // _CID: Compatible ID
            Name (_HID, "PNP0C0A" /* Control Method Battery */)  // _HID: Hardware ID
            Method (_BIX, 0, NotSerialized)  // _BIX: Battery Information Extended
            {
                If ((BSTA == 0x1F))
                {
                    Return (BIX) /* \_SB_.VMOD.BAT1.BIX_ */
                }
                Else
                {
                    Return (BIXE) /* \_SB_.VMOD.BAT1.BIXE */
                }
            }

            Method (_BST, 0, NotSerialized)  // _BST: Battery Status
            {
                Name (BST, Package (0x04)
                {
                    Zero,
                    Zero,
                    Zero,
                    0x1388
                })
                BST [Zero] = BSTE /* \BSTE */
                BST [One] = BRAT /* \BRAT */
                BST [0x02] = BCAP /* \BCAP */
                If ((BSTA == 0x0F))
                {
                    BST [0x03] = 0xFFFFFFFF
                }

                Return (BST) /* \_SB_.VMOD.BAT1._BST.BST_ */
            }

            Method (_STA, 0, NotSerialized)  // _STA: Status
            {
                Return (BSTA) /* \BSTA */
            }

            Name (_PCL, Package (0x01)  // _PCL: Power Consumer List
            {
                \_SB
            })
        }
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
