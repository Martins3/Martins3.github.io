https://unix.stackexchange.com/questions/97676/how-to-find-the-driver-module-associated-with-a-device-on-linux
通过 /sys 查找一个 device 对应的 driver

https://unix.stackexchange.com/questions/248494/how-to-find-the-driver-module-associated-with-sata-device-on-linux
使用 udevadm 来找设备的 driver
```
root@n8-030-171:~# udevadm info -a -n /dev/nvme0n1 | egrep 'looking|DRIVER'
  looking at device '/devices/pci0000:80/0000:80:02.1/0000:82:00.0/nvme/nvme1/nvme0n1':
    DRIVER==""
  looking at parent device '/devices/pci0000:80/0000:80:02.1/0000:82:00.0/nvme/nvme1':
    DRIVERS==""
  looking at parent device '/devices/pci0000:80/0000:80:02.1/0000:82:00.0':
    DRIVERS=="nvme"
  looking at parent device '/devices/pci0000:80/0000:80:02.1':
    DRIVERS=="pcieport"
  looking at parent device '/devices/pci0000:80':
    DRIVERS==""
```
发现 ssd 的 driver 就是 nvme

# nvme
1. /dev/nvme 的接口的处理
  1. 是如何通过 block 的 ioctl 最后指向了 nvme 的 ioctl 的啊 ?
2. 和 nvm express controller 的关系

感觉 : 配合 multiqueue + nvme 那么内核层次的存储就清晰了:
https://hyunyoung2.github.io/2016/05/20/NVMe/
http://ari-ava.blogspot.com/2014/07/opw-linux-block-io-layer-part-4-multi.html
https://www.thomas-krenn.com/en/wiki/Linux_Multi-Queue_Block_IO_Queueing_Mechanism_(blk-mq)_Details

# PCIe 
> 1. PCIe 存在多种消息类型，message 似乎可以处理中断了，为什么还需要中断芯片
> 2. PCIe 配置了 IRQ number
> 3. 如果 PCIe 需要使用内核的代码，

代码可以看一下 :
http://www.zarb.org/~trem/kernel/pci/pci-driver.c


PCI 不只是一个传输数据的，而是指导如何和外设沟通. [^2]
As the Linux kernel initialises the PCI system it builds data structures mirroring the real PCI topology of the system
![](https://www.tldp.org/LDP/tlk/dd/pci-structures.gif)

The PCI device driver is not really a device driver at all but a function of the operating system called at system initialisation time. The PCI initialisation code must scan all of the PCI buses in the system looking for all PCI devices in the system (including PCI-PCI bridge devices).
> 非常赞同，当 PCIe 直接构建了映射关系，通信工作似乎不需要内核插手，但是，不知道，

## sys
ls -la /sys/module/sis900/parameters/


## merge 
cat /proc/modules 可以查看所有的 module 的链接位置

Most PCIe devices are DMA masters, so the driver transfers the command to the device. The device will send several write packets to transmit 4 MiB in xx max sized TLP chunks.
[^1]: https://nvmexpress.org/wp-content/uploads/NVM_Express_1_2_Gold_20141209.pdf
[^2]: https://www.tldp.org/LDP/tlk/dd/pci.html
[^3]: https://stackoverflow.com/questions/27470885/how-does-dma-work-with-pci-express-devices

