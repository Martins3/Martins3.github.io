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
```plain
arch/x86/Makefile
251:PHONY += bzImage $(BOOT_TARGETS)
254:all: bzImage
257:KBUILD_IMAGE := $(boot)/bzImage
259:bzImage: vmlinux
265:	$(Q)ln -fsn ../../x86/boot/bzImage $(objtree)/arch/$(UTS_MACHINE)/boot/$@
299:  echo  '* bzImage      - Compressed kernel image (arch/x86/boot/bzImage)'
```
当执行了完成 make 之后，然后可以执行 bzImage，从观察上看，没有任何的区别:
```plain
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



- 说明如何制作 docker 镜像的方法
- generate_ml_image 说明了如何利用 iso 制作 docker image 的过程

## 16
- generate_hdd.sh : 单纯的制作 hdd


## 关于 qemu 综合的问题
1. 为什么 hdd 的内容消失了，是因为都是从头启动的吗 ?
3. 之前的几个智障 bundle 为什么最后没有办法添加到内核中间 ?
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

```plain
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
