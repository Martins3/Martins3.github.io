# nvme
1. /dev/nvme 的接口的处理
  1. 是如何通过 block 的 ioctl 最后指向了 nvme 的 ioctl 的啊 ?
2. 和 nvm express controller 的关系

感觉 : 配合 multiqueue + nvme 那么内核层次的存储就清晰了:
https://hyunyoung2.github.io/2016/05/20/NVMe/
http://ari-ava.blogspot.com/2014/07/opw-linux-block-io-layer-part-4-multi.html
https://www.thomas-krenn.com/en/wiki/Linux_Multi-Queue_Block_IO_Queueing_Mechanism_(blk-mq)_Details

## PCIe
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

## module 加载的基本原理

## 准备一个经典的 module 例子
nixos 的和 centos 的，最好是可以统一的

## weak modules 基本原理

## dracut 的基本原理
- softdep 之类的
- 中 modinfo 之类的

## 解决 gdb kernel modules 调试

## 我希望既可以 -kernel 参数，也可以实现增加模块，应该可以吧

## udev 的原理
udev 是如何拉起来各种驱动的

## 如何理解 /proc/sys/kernel/modprobe
- https://docs.kernel.org/next/admin-guide/sysctl/kernel.html#modprobe

```txt
#ifdef CONFIG_MODULES
	{
		.procname	= "modprobe",
		.data		= &modprobe_path,
		.maxlen		= KMOD_PATH_LEN,
		.mode		= 0644,
		.proc_handler	= proc_dostring,
	},
	{
		.procname	= "modules_disabled",
		.data		= &modules_disabled,
		.maxlen		= sizeof(int),
		.mode		= 0644,
		/* only handle a transition from default "0" to "1" */
		.proc_handler	= proc_dointvec_minmax,
		.extra1		= SYSCTL_ONE,
		.extra2		= SYSCTL_ONE,
	},
#endif
```

不知道是谁初始化的!
```txt
🧀  cat /proc/sys/kernel/modprobe
/nix/store/1z6hk4iky1wv6gaa8s0isn35489x0fa2-kmod-30/bin/modprobe
```
其使用位置是:
- `__request_module` : 调用位置非常多，我猜测是，这个的作用是，内核想要调用 modprobe 的时候，就需要知道 modprobe 的位置。
  - call_modprobe
