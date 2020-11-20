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



# busybox
https://busybox.net/


似乎是一个比较靠谱的文章:
https://www.cnblogs.com/hellogc/p/7482066.html
busybox 是用于提供 init 程序

这个方法 : 告知 drive + root 指定，使用 linuxrc 来启动，老的做法
> 这个方法成功了，那么 Ubuntu20 的还是没有办法，是不是因为 Ubuntu20 由于 ext4，所以，存在问题
> **可以尝试的内容 : 不使用 busybox，在其中只是简单的写入一个 init 程序 : 估计是不可以的**

> 1. 这个blog中间最后的部分，mount 各种文件夹的操作暂时没有实现
> 2. 关于创建 init 的部分，感觉不科学，maybe out of dated，内核的文档中间才说到 linurc 作为启动的不科学

cnblog 的那个，其中 tty 的报错，也许可以使用如下解决

```sh
sudo mkdir -p rootfs/dev 
sudo mknod rootfs/dev/tty1 c 4 1  
sudo mknod rootfs/dev/tty2 c 4 2  
sudo mknod rootfs/dev/tty3 c 4 3  
sudo mknod rootfs/dev/tty4 c 4 4
```

# buildroot

似乎 buildroot 是一个解决方案的一样的存在:

https://github.com/buildroot/buildroot

https://medium.com/@daeseok.youn/prepare-the-environment-for-developing-linux-kernel-with-qemu-c55e37ba8ade
> 使用 buildroot 维持生活的一个例子



感觉 buildroot 和 minimal 的项目非常类似，只是其更加简化，并且限定为 x86

https://gist.github.com/chrisdone/02e165a0004be33734ac2334f215380e
1. 其中，并不知道如何加入 init.sh
2. buildroot 的部分以后用到的时候，然后慢慢分析吧


# 手动搭建环境
https://ibugone.com/blog/2019/04/os-lab-1/

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

# Minimal Linux Live
1. 想要安装 gcc，然后在上面编译程序，
2. 检查一下 linux57 中间的内容，分析其中制作 initramfs 的方法，以及 install package 的原理

## 01 02
make mrproper : https://unix.stackexchange.com/questions/387640/why-both-make-clean-and-make-mrproper-are-used 比 make clean 可以删除 config

sed 采用类似于 vim 的方法进行替换，默认全部替换，似乎是基于行的，在处理 substring 的替换。

1. 为什么需要 enable overlay fs
2. EFI 是做什么的 ? 为什么不采用 UEFI，因为内核中间只有对于 EFI 的配置!
3. 什么垃圾项目 : 居然会编译出现错误，http://lkml.iu.edu/hypermail/linux/kernel/2001.3/05638.html
4. header 的作用，应该就是简单的拷贝，只是作用需要观察一下:

https://forums.gentoo.org/viewtopic-t-1110110-start-0.html

可以确定，make 和 make bzImage 是不同的:
```
arch/x86/Makefile
251:PHONY += bzImage $(BOOT_TARGETS)
254:all: bzImage
257:KBUILD_IMAGE := $(boot)/bzImage
259:bzImage: vmlinux
265:	$(Q)ln -fsn ../../x86/boot/bzImage $(objtree)/arch/$(UTS_MACHINE)/boot/$@
299:  echo  '* bzImage      - Compressed kernel image (arch/x86/boot/bzImage)'
```
当执行了完成 make 之后，然后可以执行 bzImage，从观察上看，没有任何的区别:
```
➜  linux git:(master) ✗ make bzImage
  CALL    scripts/checksyscalls.sh
  CALL    scripts/atomic/check-atomics.sh
  DESCEND  objtool
  CHK     include/generated/compile.h
Kernel: arch/x86/boot/bzImage is ready  (#1)
➜  linux git:(master) ✗ l arch/x86_64/boot/bzImage 
lrwxrwxrwx shen shen 22 B Tue Jun 16 17:59:02 2020   bzImage ⇒ ../../x86/boot/bzImage
```

通过 make 只是可以生成一个 vmlinux 的，
但是，我的经验告诉我，make 实际上 bzImage 也是有的，暂时不清楚两者的区别 ?


## 03 04
1. 为什么需要 glibc 的内容
2. configure 的　prefix 非常神奇，因为是安装到 iso 的镜像中间，所以，应该是可以自动
3. 居然添加相同的 CFLAGS 编译 kernel 和 glibc，检查其中的每一个内容

## 05 06 07
1. 制作 sysroot 的时候，为什么要处理 usr 的目录
2. ln -s ../../ a/b 这个时候产生的有问题，目标地址只能是一个文件

3. 编译 busybox 不再是使用 static libc，而是指定刚刚制作的 sysroot ，并且安装的时候，链接刚才制作的 glibc，
同时再次使用那些 flags

## 08 overlay_build.sh
2. **additional** bundle ?
4. 在 bundle 中间存在各种 glibc 只是在进行*简单的部分的拷贝*，为什么要这么操作
3. 观察一下 overlay logo 的含义是什么 ?
4. 关键的问题 : overlay_rootfs 和 本来的 rootfs 的关系是什么 ?

- overlay_rootfs/src 下面的内容是那个位置生成的 ? 在 mll_hello 的地方，但是其中并不存在任何。
- overlay 和 overlay_rootfs 的关系是什么 ? overlay 是 download 内容
- 居然还存在 strip 这种 nb 的删除符号的方法


## 09 10
1. 其实匹配文件夹的方法，可以总结一下
2. .keep 文件是做什么的 ?
3. OVERLAY_LOCATION : iso 或者 rootfs，区别居然是是否将所有的内容进行拷贝，是不是，现在只是拷贝一部分，之后，之后在进行需要的拷贝 ?
4. 这个无穷无尽的拷贝，是不是最后就是给 buildroot 用的 ?

5. 两种构建方法，kernel header 都是没有被拷贝的，区别是，是否拷贝 overlay_rootfs，也就是用户库


- 首先，将 minimal_rootfs  全部拷贝到 overlay_rootfs 中间
- sysroot 是通过 glibc 和 kernel 安装的得到的，其实只是作为一个暂时存放的内容，此时，回头看，发现 bundle 的内容非常奇怪，首先将内容拷贝到 overlay 中间，咱那里，每个 应用的内容都是分开的，然后拷贝到 overlay_rootfs 中间，此时，合并成为一个 rootfs，最后，拷贝到 rootfs 中间。

## 11
1. overlay_type 是什么东西 ?

- 如果，制作的是 iso 的时候，那么需要将用户层的东西拷贝一下
- 9 10 两步，制作 initrd 和 iso

## 12
1. 能不能，用 grub 而不是这个小众的 systemd 来实现项目的配置

- bios 居然不需要 systemd，systemd 不是用于实现 启动的吗 ?  这就是作者随便搞的一个名字，还是分析 syslinux 吧，看到时候，能不能改成 syslinux
- 分别安装 syslinux 和 systemd
- sysliux 都是好多年不更新的东西了 : https://mirrors.edge.kernel.org/pub/linux/utils/boot/


## 15
1. 那么能不能利用 busy box 制作一个最最基本的 image
2. 


- 说明如何制作 docker 镜像的方法
- generate_ml_image 说明了如何利用 iso 制作 docker image 的过程

## 16
- generate_hdd.sh : 单纯的制作 hdd


## 关于 qemu 综合的问题
1. 为什么 hdd 的内容消失了，是因为都是从头启动的吗 ?
3. 之前的几个智障bundle 为什么最后没有办法添加到内核中间 ? 
4. 比较一下 docker 和 qemu 的内容的不同

7. 考虑一下将 sysroot 的所有东西全部拷贝到其中

8. 在默认的配置中间，似乎 rootfs 没有生成 iso，如何才可以使用那个脚本。

9. 可以安装软件 ?

10. 为什么启动的过程存在这么的等待过程 : 了解一下其自定义的启动过程吧 !
13. 理解其中的 auto run 是如何制作的， 以及是如何执行的 ?

15. 为了支持图形，内核中间打开的设置是什么 ?
    1. 这些编译的库，最后被放到 bzImage 中间了吗 ? 还是在特别的位置 ?
    2. 而且使用 kernel + initramfs 的时候，为什么没有询问 vga 的哪一步，如果想要使用更大的屏幕，如何处理

- kernel 根本没有被安装到 sysroot 的位置，而是变成了 xz
- boot 中间也是包含了 initramfs 的
- 如下是可以正确的执行的，合乎想法:
```sh
qemu-system-x86_64 -kernel $KERNEL_INSTALLED/kernel -initrd $WORK_DIR/rootfs.cpio.xz -hda ./hdd.img
```
两个问题:
  1. 可以，这种操作，我的各种 overlay 的东西都没有了啊，利用制作的 iso，我想要单独指定内核，也就是 ubuntu 的终极问题
  2. 但是，这个代码，我的 hdd.img 被挂载到在哪里

## 总结一下几个文件夹的内容

| dir              | explain                                                                                    |
|------------------|--------------------------------------------------------------------------------------------|
| minimal_rootfs   | 在 /etc/ 下面，存在其中的时候执行若干脚本                                                  |
| minimal_overlay  | bundles 提供了各种制作工具的脚本，而 rootfs 就是直接提供的拷贝                             |
| overlay_rootfs   | 各种 bundles 制作的内容                                                                    |
| rootfs           | busybox + minimal_rootfs ，根据 iso 还是 rootfs，rootfs 可能也持有 isoimage_overlay 的内容 |
| isoimage_overlay | 将 overlay_rootfs + overlay_rootfs/rootfs                                                  |
| isoimage         | isoimage_overlay + bios + (kernel + initramfs)                                             |

- rootfs 最后被制作为 initramfs
- rootfs 和 isoimage_overlay 是一个组合


## minimal_rootfs 内容分析
1. 其中的流程是什么 ?
2. inittab 居然是是规定如何创建新的线程，如果我修改成为一个不是，shell，那么岂不是可以执行一些死循环 ?
3. cttyhack 是 busybox 自己提供的，连 sbin/init 也是其提供的，但是两者的功能的作用是什么，现在并不清楚
4. minimal_rootfs/etc/04_bootscript.sh 是如何被指定执行的 ?

```
::sysinit:/etc/04_bootscript.sh
::restart:/sbin/init
::shutdown:echo -e "\nSyncing all file buffers."
::shutdown:sync
::shutdown:echo "Unmounting all filesystems."
::shutdown:umount -a -r
::shutdown:echo -e "\n  \\e[1mCome back soon. :)\\e[0m\n"
::shutdown:sleep 1
::ctrlaltdel:/sbin/reboot
::once:cat /etc/motd
::respawn:/bin/cttyhack /bin/sh
tty2::once:cat /etc/motd
tty2::respawn:/bin/sh
tty3::once:cat /etc/motd
tty3::respawn:/bin/sh
tty4::once:cat /etc/motd
tty4::respawn:/bin/sh
```

https://www.cyberciti.biz/howto/question/man/inittab-man-page.php : inittab 的说明文档，我想知道 inittab 是被谁读取的 ?

5. 搞清楚交接过程，hdd 那个磁盘到底是怎么回事 ? initramfs 中间的程序最后会消失吗，被真正的 root 取代?


## sysliux 是如何分析 iso，并将其启动的 ?


## syslinux
https://wiki.syslinux.org/wiki/index.php?title=Development/Testing : 利用 qemu 进行测试工作
https://en.wikipedia.org/wiki/Comparison_of_boot_loaders
> excuse me ?
> 和 grub 是什么关系啊

http://www.embox.rocks/ 的文档 https://github.com/embox/embox/wiki/QEMU-with-GRUB2-and-Syslinux 分别使用了 grub2 和 syslinux 作为镜像



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
