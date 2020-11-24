## 有意思呀
https://github.com/NinjaTrappeur/ultimate-writer

# 资源

- https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os
- [derekmolloy](http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)　更加详细，但是缺乏解释

https://atreus.technomancy.us/firmware
## 3
One class of module is the **device driver**, which provides functionality for hardware like a TV card or a serial port. On unix, each piece of hardware is represented by a file located in /dev named a device file which **provides** the means to communicate with the hardware.

The major number tells you which driver is used to access the hardware.
Each driver is assigned a **unique** major number; all device files with the **same** major number are controlled by the same driver.

The driver itself is the only thing that cares about the minor number. It uses the minor number to distinguish between different pieces of hardware.

The difference is that block devices have a **buffer** for requests, so they can choose the best order in which to respond to the requests.
This is important in the case of storage devices, where it's faster to read or write sectors which are close to each other, rather than those which are further apart. 
**Another** difference is that block devices can only accept input and return output in blocks (whose size can vary according to the device), 
whereas character devices are allowed to use as many or as few bytes as they like.


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

## lwn
1. https://lwn.net/Articles/645810/
