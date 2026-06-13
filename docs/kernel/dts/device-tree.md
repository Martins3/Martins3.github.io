# 设备树
<!-- 2ce251c3-6002-4502-a46b-3ae3353d0365 -->

通过 asahi linux 做最简单的观察

1. /proc/device-tree
2. find /boot -name "*.dtb"

从 dtb 到 dts 的
```txt
dtc -I dtb -O dts -o device-tree.dts /boot/<model>.dtb
# 直接从 /proc 获取
dtc -I fs -O dts -o runtime.dts /proc/device-tree
```

设备树相关的配置:
```txt
CONFIG_OF_PARTITION=y
CONFIG_OF=y
# CONFIG_OF_UNITTEST is not set
CONFIG_OF_KUNIT_TEST=m
CONFIG_OF_FLATTREE=y
CONFIG_OF_EARLY_FLATTREE=y
CONFIG_OF_KOBJ=y
CONFIG_OF_DYNAMIC=y
CONFIG_OF_ADDRESS=y
CONFIG_OF_IRQ=y
CONFIG_OF_RESERVED_MEM=y
CONFIG_OF_RESOLVE=y
CONFIG_OF_OVERLAY=y
CONFIG_OF_OVERLAY_KUNIT_TEST=m
CONFIG_OF_NUMA=y
CONFIG_OF_MDIO=y
CONFIG_OF_GPIO=y
CONFIG_OF_IOMMU=y
CONFIG_OF_PMEM=y
CONFIG_OF_FPGA_REGION=m
```

## dts 和 acpi 的简单对比
<!-- e6341e27-e57c-4872-9ae7-b79d5a6c9945 -->

再此回到一个老问题，既然 mac 上用到的是 dts ，那么之前理解的 acpi 获取电池电量的方法，这里有吗?

参考 Documentation/devicetree/bindings/power/supply/battery.yaml 可以看到


> [!NOTE]
> 参考神奇海螺的意见，有待验证

(让 codex 分析 Documentation/devicetree/bindings/power/supply/battery.yaml ，差不多是这个意思)

Documentation/devicetree/bindings/power/supply/battery.yaml:12 说的
battery 节点，提供的是 static battery characteristics，也就是电池的静态参
数，不是实时电量。比如：

- 化学类型 device-chemistry
- 设计满充容量 charge-full-design-microamp-hours
- 最低/最高设计电压 voltage-min/max-design-microvolt
- 充电终止电流、恒压恒流上限
- OCV 容量表 ocv-capacity-table-*
- 温度和内阻表 resistance-temp-table

这也是为什么它的例子里，battery 节点要被 charger 或 fuel-gauge 通过
monitored-battery 引用，而不是自己“产出电量值”：
Documentation/devicetree/bindings/power/supply/battery.yaml:27
Documentation/devicetree/bindings/power/supply/battery.yaml:148

Linux 里对应的实现就在 drivers/power/supply/power_supply_core.c:580。
power_supply_get_battery_info() 会先沿着 monitored-battery 找到这个节点，
再把这些属性读进 include/linux/power_supply.h:475 结构体里：
drivers/power/supply/power_supply_core.c:595
drivers/power/supply/power_supply_core.c:684
drivers/power/supply/power_supply_core.c:727

所以在 DTS 场景下，“电量”通常是这样来的：

1. DTS 的 simple-battery 提供静态参数和查表数据。
2. charger / fuel-gauge 驱动从硬件读实时 voltage/current/temp。
3. 驱动再结合 battery_info 做估算，或者由 gauge 芯片直接给出 SoC。

一个很直接的例子是 drivers/power/supply/adc-battery-helper.c:139，它先算
OCV，再调用 power_supply_batinfo_ocv2cap() 把 OCV 映射成容量百分比：
drivers/power/supply/adc-battery-helper.c:121
drivers/power/supply/adc-battery-helper.c:139

一句话总结：

- battery.yaml 里的 DTS 节点描述的是“这块电池应该是什么样子”。
- 实时电量不是 DTS 里写死的，而是驱动读硬件后，结合这些静态参数算出来，或
  者直接由 fuel gauge 芯片提供。



## dts 的各种观察
在 mac 下:
```txt
🧀  pwd
/sys/firmware
/sys/firmware🔒 😚
🧀  ls
devicetree  efi  fdt
```
```txt
🧀  ls -la
lrwxrwxrwx@ - root 25 Dec 10:16 macsmc-ac -> ../../devices/platform/soc/23e400000.smc/macsmc-power/power_supply/macsmc-ac
lrwxrwxrwx@ - root 25 Dec 10:16 macsmc-battery -> ../../devices/platform/soc/23e400000.smc/macsmc-power/power_supply/macsmc-battery
lrwxrwxrwx@ - root 25 Dec 10:16 tps6598x-source-psy-0-003f -> ../../devices/platform/soc/235010000.i2c/i2c-0/0-003f/power_supply/tps6598x-source-psy-0-003f
lrwxrwxrwx@ - root 25 Dec 10:16 tps6598x-source-psy-0-0038 -> ../../devices/platform/soc/235010000.i2c/i2c-0/0-0038/power_supply/tps6598x-source-psy-0-0038
```

Hyper-V 虚拟机
```txt
/sys/class/power_supply🔒 🗔
🧀  ls -la
lrwxrwxrwx - root 13 Oct 13:08 AC1 -> ../../devices/system/container/ACPI0004:00/ACPI0003:00/power_supply/AC1
lrwxrwxrwx - root 13 Oct 13:08 BAT1 -> ../../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/PNP0C0A:00/power_supply/BAT1
```

```txt
/sys/firmware🔒 🗔
🧀  ls
acpi  dmi  efi  memmap
```

arm 虚拟机中:
```txt
 ls /sys/firmware
acpi  devicetree  dmi  efi  fdt  qemu_fw_cfg
```
但是 devicetree 目录是空的

## 和内核 drivers/of 的关系

```txt
DTS 源文件 ──dtc 编译──> DTB 二进制 ──内核启动加载──> drivers/of/ 解析 ──> 创建设备、配置资源
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
