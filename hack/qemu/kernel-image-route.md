# 分析路线
1. 完全搞清楚 minimal 的是如何构建的，如果可以，将其改写为 makefile 的效果
3. 内核文档的完全理解 [^1]
4. 可以尝试将内核挂载到文件系统上，然后直接拷贝的方法 ?

5. rootfs 和 initramfs 的区别是什么 ?

https://github.com/MichielDerhaeg/build-linux
- [ ] https://re-ws.pl/2020/11/busybox-based-linux-distro-from-scratch/

# 搭建一个高效的内核开发环境
1. 生成的 bzImage 包含了各种驱动吗 ?
  2. 调查一下 Manjaro 安装 kernel 的方法
  3. 看一下 arch linux 的通用安装方法

2. 如果说，ramfs 是 tmpfs 的 instance  ?
    1. 从代码上看，为什么感觉 tmpfs 应该是 shmem 一部分，可以阅读一下 tmpfs 的内容

3. rootfs 是啥 ?
  1. rootfs 是 ramfs 的特例，[^1] rootfs 会默认使用 tmpfs 而不是 rootfs

6. 内核的 source tree 中间的 ./usr/

7. 那个手动搭建的例子已经很有意思了，搞清楚，内核是怎么开始执行 init 的

## [linuxfromscratch](http://www.linuxfromscratch.org/lfs/view/stable/prologue/audience.html)

## 各种 Linux distribution
- https://github.com/paralin/SkiffOS
- https://github.com/oasislinux/oasis : oasis is a small linux system.
- https://github.com/hifiberry/hifiberry-os/ : HiFiBerryOS is our version of a minimal Linux distribution optimized for audio playback.

## multiboot
1. https://os.phil-opp.com/multiboot-kernel/
  - https://stackoverflow.com/questions/45968876/byte-vs-long-vs-word-in-gas-assembly
2. https://www.gnu.org/software/grub/manual/grub/html_node/multiboot.html#multiboot

3. [multiboot specification](https://nongnu.askapache.com/grub/phcoder/multiboot.pdf)
4. [how to create iso from multiboot](https://wiki.osdev.org/GRUB_2)

## rootfs 和 initramfs
[^2] 关于如何利用 qemu 实现内核加速，非常遗憾，文档是非常不详细的

忽然想到，为什么 mount 根节点是一个鸡生蛋的问题，因为真正的根节点 mount 到 / ，即使需要一个文件系统存在的。 [^3]

[^4] 虽然 bootloader(grub2) 可以找到内核，但是各种内核驱动都是放在文件系统中间的，
hence, the creation of a preliminary root file system that would contain just enough in the way of loadable modules to give the kernel access to the rest of the hardware.
> 岂不是，这些驱动是靠 "preliminary" fs 加载的 ?

[^2]: https://lwn.net/Articles/660404/
[^3]: https://unix.stackexchange.com/questions/493644/rootfs-is-a-special-instance-of-ramfs
[^4]: https://www.linux.com/training-tutorials/kernel-newbie-corner-initrd-and-initramfs-whats/
