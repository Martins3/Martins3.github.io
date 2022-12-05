# pci

<!-- vim-markdown-toc GitLab -->

* [关键 blog](#关键-blog)
* [overview](#overview)
* [hotplug](#hotplug)
* [proc and sys](#proc-and-sys)
  * [lspci](#lspci)
* [question](#question)

<!-- vim-markdown-toc -->

**When Comming back : Loyenwang has analyze PCI, I don't think we will have any question about it**

- [ ] 内核文档 : [^3]
- [ ] https://www.kernel.org/doc/Documentation/PCI/pci.txt
- [ ] https://sites.google.com/site/pinczakko/pinczakko-s-guide-to-award-bios-reverse-engineering

## 关键 blog
-  https://resources.infosecinstitute.com/topic/system-address-map-initialization-in-x86x64-architecture-part-1-pci-based-systems/


## overview

[^2]
> Generally speaking, most modern OSes (Windows and Linux) will re-scan the detected hardware as part of the boot sequence. Trusting the BIOS to detect everything and have it setup properly has proven to be unreliable.

1. os 会重新对于所有的设备进行一次扫描，bios 的扫描其实不可靠的
2. ACPI Enumeration - The OS can rely on the BIOS to describe these devices in ASL code.


## hotplug
https://github.com/intel/nemu/wiki/ACPI-PCI-discovery-hotplug


## proc and sys
存在的所有 interface :
- /sys/devices/pci0000:00
- /sys/bus/pci
  - devices : 指向 /sys/devices/pci0000:00 的软连接
  - driver : 各种 device 的驱动, 其中包含的文件是同一个模式产生的, bind, unbind 之类的, 这个是 bus.c:bus_add_driver() 产生的 *TODO*
  - slots : 没有任何东西 *TODO*
  - drivers_autoprobe : 下面的几个都是*处理 driver 和 device* 绑定的东西吧？
  - drivers_probe
  - rescan
  - resource_alignment
  - uevent
- /proc/bus/pci : /proc 下面的信息应该都是放在 linux/drivers/pci/proc.c 中间了
  - devices : https://stackoverflow.com/questions/2790637/how-to-interpret-the-contents-of-proc-bus-pci-devices : 是对应的，参考 ldd3 中文版 p303，其中
  - 00 01 02 03: 之类的, 分别对应 bus:dev:function 的 bus 为 00 的 ，在目录记录 dev:functions *TODO* 具体信息暂时看不到.

### lspci
- [ ] lspci 是如何利用 /sys 实现的
    - https://unix.stackexchange.com/questions/121357/is-there-any-substitute-for-lspci

## question

For the driver developer, the PCI family offers an attractive advantage: a system of automatic device
configuration. Unlike drivers for the older ISA generation, PCI drivers need not implement complex probing
logic. During boot, the BIOS-type boot firmware (or the kernel itself if so configured) walks the PCI bus and
assigns resources such as interrupt levels and I/O base addresses. **The device driver gleans this assignment by
peeking at a memory region called the PCI configuration space.** PCI devices possess 256 bytes of configuration
memory. The top 64 bytes of the configuration space is standardized and holds registers that contain details
such as the status, interrupt line, and I/O base addresses.
> bios 对于设备进行探测，然后将 configuration 放到 configuration region.
> 然后让设备驱动读取。

- [ ] 这个读去过程具体在哪里 ? probe 吗 ？
- [ ] pcie 如何支持一个新插入的设备 ？

To operate on a memory region such as the frame buffer on the above PCI video card, follow these steps:
1.  pci_resource_start
2.  request_mem_region


- [ ] [^1] 解释 request_mem_region 只是预留空间的作用, 但是这个预留又不能修改 pcie root hub 对于地址的解析，或者说，这完全是软件上的操作，软件申请了，向该地方写数值，最后还是靠 pcie 的解析，所以，request_mem_region 的根本作用引入检查 ？
  - [ ] 如果是引入检查，那么，从 pci configuration region 中间直接读去不香吗 ?

- [ ] host bridge 是 ?


[^1]: https://stackoverflow.com/questions/7682422/what-does-request-mem-region-actually-do-and-when-it-is-needed
[^2]: https://stackoverflow.com/questions/18854931/how-does-the-os-detect-hardware
