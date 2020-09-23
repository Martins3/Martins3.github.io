## 有意思呀
https://github.com/NinjaTrappeur/ultimate-writer

# 资源
> ldd2

https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os
https://www.tldp.org/LDP/lkmpg/2.6/html/lkmpg.html
[derekmolloy](http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)　更加详细，但是缺乏解释
[字符设备 官方描述](https://www.kernel.org/doc/html/latest/core-api/kernel-api.html?highlight=cdev#char-devices)

https://atreus.technomancy.us/firmware
# LDD

# PCI 驱动编程
https://www.kernel.org/doc/Documentation/PCI/pci.txt

# usb驱动编程
测试辅助模块 `dummy_hcd` 和 `g_zero`


# [The Linux Kernel Module Programming Guide](https://www.tldp.org/LDP/lkmpg/2.6/html/lkmpg.html#AEN121)
## 2
### 2.8. Building modules for a precompiled kernel
Obviously, we strongly suggest you to recompile your kernel, so that you can enable a number of useful debugging features, such as forced module unloading (**MODULE\_FORCE\_UNLOAD**): when this option is enabled, you can force the kernel to unload a module even when it believes it is unsafe, via a `rmmod -f module` command
> 重新编译内核 ????
> 很迷， 不知道这一节说的是什么东西

## 3
One class of module is the **device driver**, which provides functionality for hardware like a TV card or a serial port. On unix, each piece of hardware is represented by a file located in /dev named a device file which **provides** the means to communicate with the hardware.

The major number tells you which driver is used to access the hardware.
Each driver is assigned a **unique** major number;
all device files with the **same** major number are controlled by the same driver.

The driver itself is the only thing that cares about the minor number. It uses the minor number to distinguish between different pieces of hardware.

The difference is that block devices have a **buffer** for requests, so they can choose the best order in which to respond to the requests.
This is important in the case of storage devices, where it's faster to read or write sectors which are close to each other, rather than those which are further apart. 
**Another** difference is that block devices can only accept input and return output in blocks (whose size can vary according to the device), 
whereas character devices are allowed to use as many or as few bytes as they like.



# 书写代码注意
1. 没有函数没有参数，那么该函数的参数列表为`void`
```
int foo(void);
``` 

# 问题
1. 阅读 /lib/modules(shell uname -a)/build 下的makefile 中间的内容
  1. 包含的头文件有点不对 asm 下头文件似乎没有被包含下来
  2. 应该不会指向glibc 的文件
2. 修改Makefile　产生的文件放置到指定的文件夹中间去
3. /dev 和 /proc/devices 两者的关系是什么
4. 为什么ebbchar是没有 手动mknod 的操作, **显然**应该含有对应的操作
5. 大量的printk 都是没有输出的，　不知道其中的原因是什么
6. 修改scull\_load 文件中间的内容
7. klogd syslogd 
8. 内核模块 和 驱动模块有什么区别


## [wowotech 阅读笔记](http://www.wowotech.net/sort/device_model)
硬件拓扑描述Linux设备模型中四个重要概念中三个：Bus，Class和Device（第四个为Device Driver，后面会说）

## 正文

Device（struct device）和Device Driver（struct device_driver ）两个数据结构，分别从“有什么用”和“怎么用”两个
> 不认同，device 的作用是 更像是管理设备的，device_driver 用于管理所有的 device deriver，然后和系统中间的 device 匹配。
> 两者配合可以实现热插拔操作，识别到硬件，然后利用 device_driver 的 probe 函数之类的创建 device

> @todo 具体的设别到底如何持有 device 和 device_driver

> 通过 bus_type 可以实现依赖关系，但是依赖关系仅限于 设备对于 bus 吗 ?


struct bus_type 

> 作者在此处提出的 bus device device_driver 和 

## lwn
1. https://lwn.net/Articles/645810/

# ldd3
实际上，ldd3 的代码又被更新了，所以，可以好好的重新看一遍
