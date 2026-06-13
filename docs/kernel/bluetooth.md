
```txt
[    2.780523] Bluetooth: BNEP (Ethernet Emulation) ver 1.3
[    2.780527] Bluetooth: BNEP socket layer initialized
[    2.781084] Bluetooth: MGMT ver 1.22
```

```txt
🤒  cyme
  1   2  0x0b05 0x19af AURA LED Controller 9876543210 usb     12.0 Mb/s
  1   3  0x174c 0x2074 ASM107x             -          usb    480.0 Mb/s
  1   4  0x8087 0x0026 AX201 Bluetooth     -          usb     12.0 Mb/s
  2   2  0x174c 0x3074 ASM107x             -          usb      5.0 Gb/s
```

居然本来就有 bluetooth 模块

obj-$(CONFIG_BT)		+= bluetooth/

## rfkill

看来 bluetooth 和 wifi 都依赖这个东西:
```txt
🧀  ls -la /sys/class/rfkill
lrwxrwxrwx - root  8 Feb 22:39  rfkill0 -> ../../devices/pci0000:00/0000:00:14.0/usb1/1-14/1-14:1.0/bluetooth/hci0/rfkill0
lrwxrwxrwx - root  8 Feb 22:39  rfkill1 -> ../../devices/pci0000:00/0000:00:14.3/ieee80211/phy0/rfkill1
```

INSTALL /lib/modules/6.14.2/kernel/drivers/bluetooth/btusb.ko
INSTALL /lib/modules/6.14.2/kernel/drivers/bluetooth/btintel.ko
INSTALL /lib/modules/6.14.2/kernel/drivers/bluetooth/btintel_pcie.ko
INSTALL /lib/modules/6.14.2/kernel/drivers/bluetooth/btbcm.ko
INSTALL /lib/modules/6.14.2/kernel/drivers/bluetooth/btrtl.ko

INSTALL /lib/modules/6.14.2/kernel/net/bluetooth/bluetooth.ko
INSTALL /lib/modules/6.14.2/kernel/net/bluetooth/bnep/bnep.ko

- [Bluetooth Protocol?](https://stackoverflow.com/questions/1046669/bluetooth-protocol)

  - 原来，bluetooth-protocol 是连 tcp/ip 都不走的


```txt

[    5.241573] Bluetooth: hci0: Device revision is 2
[    5.241579] Bluetooth: hci0: Secure boot is enabled
[    5.241580] Bluetooth: hci0: OTP lock is enabled
[    5.241581] Bluetooth: hci0: API lock is enabled
[    5.241582] Bluetooth: hci0: Debug lock is disabled
[    5.241583] Bluetooth: hci0: Minimum firmware build 1 week 10 2014
[    5.241585] Bluetooth: hci0: Bootloader timestamp 2019.40 buildtype 1 build 38
[    5.241591] Bluetooth: hci0: No support for _PRR ACPI method
[    5.283059] Bluetooth: hci0: Found device firmware: intel/ibt-0040-2120.sfi
[    5.283089] Bluetooth: hci0: Boot Address: 0x100800
[    5.283090] Bluetooth: hci0: Firmware Version: 97-50.22
```

## 先从 https://github.com/pythops/bluetui 这个工具用起来吧

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
