# 分析路线
1. 完全搞清楚 minimal 的是如何构建的，如果可以，将其改写为 makefile 的效果
2. lfs
3. 内核文档的完全理解 [^1] 
4. 可以尝试将内核挂载到文件系统上，然后直接拷贝的方法 ?

5. rootfs 和 initramfs 的区别是什么 ?

6. 一个不知道咋回事的项目 : https://github.com/paralin/SkiffOS
7. 这个也可以对照关注一下 : https://github.com/MichielDerhaeg/build-linux

https://github.com/coreboot/coreboot : BIOS bootloader 之类的完全搞不懂啊
https://github.com/troglobit/finit : 专门的 init
https://github.com/oasislinux/oasis : oasis is a small linux system.

- [ ] https://re-ws.pl/2020/11/busybox-based-linux-distro-from-scratch/

- [ ] https://www.qemu-advent-calendar.org/2020/ : pulished every two year, adventure for dune

- https://github.com/krallin/tini : A tiny but valid init for containers
- https://github.com/hifiberry/hifiberry-os/ : HiFiBerryOS is our version of a minimal Linux distribution optimized for audio playback. 


# 搭建一个高效的内核开发环境
1. 生成的 bzImage 包含了各种驱动吗 ?
  2. 调查一下 Manjaro 安装 kernel 的方法
  3. 看一下 arch linux 的通用安装方法

2. 如果说，ramfs 是 tmpfs 的 instance  ?
    1. 从代码上看，为什么感觉 tmpfs 应该是 shmem 一部分，可以阅读一下 tmpfs 的内容

3. rootfs 是啥 ?
  1. rootfs 是 ramfs 的特例，[^1] rootfs 会默认使用 tmpfs 而不是 rootfs
  
4. init 进程到底是什么 ?

5. 随意编译一个内核，但是驱动和内核不是兼容的，不是 GG，所以驱动是显然需要的:

6. 内核的 source tree 中间的 ./usr/

7. 那个手动搭建的例子已经很有意思了，搞清楚，内核是怎么开始执行 init 的


# 手动搭建环境
https://ibugone.com/blog/2019/04/os-lab-1/ : 中科大老哥的 blog

这里总体采用的就是 initramfs 的作用吧，但是两个内核参数，非常的质疑:


> 其实，很想复现第一部分，但是没有办法，不知道为什么
> 务必搞清楚其中所有的参数的含义是什么，比如内核参数

> cnblog 没有办法解决的 proc 问题，这里算是很容易就处理了

> **根本不知道为什么其知道创建两个设备文件的做法源自于什么地方**

> 1. 后面的内核参数根本就是骗人的，不添加也是问题不大
> 2. 实际上，连两个创建的设备的操作也是不需要的

https://ops.tips/notes/booting-linux-on-qemu/
> 这里的 ref 指出 : initramfs 和 kernel 被 bootloader 加载内存中间，
> 而 initramfs 被 mount 到 / ，并且执行其中的 init

# linuxfromscratch
- 编译出来的东西非常大，需要好几个小时，不过，可以作为一个各种软件的安装教程

http://www.linuxfromscratch.org/lfs/view/stable/prologue/audience.html


# rootfs 和 initramfs
[^1] 指出了使用 initramfs 的最经典的例子，比中科大的那个blog要简单很多。
[^2] 关于如何利用 qemu 实现内核加速，非常遗憾，文档是非常不详细的

忽然想到，为什么 mount 根节点是一个鸡生蛋的问题，因为真正的根节点 mount 到 / ，即使需要一个文件系统存在的。 [^3]

[^4] 虽然 bootloader(grub2) 可以找到内核，但是各种内核驱动都是放在文件系统中间的，
hence, the creation of a preliminary root file system that would contain just enough in the way of loadable modules to give the kernel access to the rest of the hardware.
> 岂不是，这些驱动是靠 "preliminary" fs 加载的 ?

[^1]: https://www.kernel.org/doc/html/latest/filesystems/ramfs-rootfs-initramfs.html
[^2]: https://lwn.net/Articles/660404/ 
[^3]: https://unix.stackexchange.com/questions/493644/rootfs-is-a-special-instance-of-ramfs
[^4]: https://www.linux.com/training-tutorials/kernel-newbie-corner-initrd-and-initramfs-whats/
