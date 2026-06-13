## 注意到
1. drivers/pci/iov.c 如果被修改了，那么很多模块也会重新构建，但是实际上不用更新这些模块，
一样是可以自动插入的。也许可以将 module 和 kernel 自身分开，这样速度更快一点

## 原来还可以这样的啊
genisoimage -o ~/backup.iso -V BACKUP -r -J ~/Documents


解压一个 openEuler 的 iso 看看:
```txt
.
├── boot.catalog
├── docs
│   ├── OpenEuler-Software-License.docx
│   └── TRANS.TBL
├── EFI
│   ├── BOOT
│   │   ├── BOOTAA64.EFI
│   │   ├── fonts
│   │   │   ├── TRANS.TBL
│   │   │   └── unicode.pf2
│   │   ├── grubaa64.efi
│   │   ├── grub.cfg
│   │   ├── mmaa64.efi
│   │   └── TRANS.TBL
│   └── TRANS.TBL
├── images
│   ├── efiboot.img
│   ├── install.img
│   ├── pxeboot
│   │   ├── initrd.img
│   │   ├── TRANS.TBL
│   │   └── vmlinuz
│   └── TRANS.TBL
├── isolinux_ks_automatic.cfg
├── isolinux_ks_manual.cfg
├── ks_automatic.cfg
├── ks_manual.cfg
├── Packages
│   ├── adwaita-icon-theme-3.37.2-2.oe1.noarch.rpm
```


```c
static struct file_system_type rootfs_fs_type = {
	.name		= "rootfs",
	.mount		= rootfs_mount,
	.kill_sb	= kill_litter_super,
};
```
## 这里进来
https://blog.davidv.dev/posts/booting-x86-64/

## 看下 kernel source tree 下的 usr 做啥的

## 一条路有 intiramfs ，一条路是直接构建 ext4 的 rootfs 的 img 出来

## 这里的东西
- https://www.kernel.org/doc/html/latest/driver-api/early-userspace/early_userspace_support.html

- 制作 initramfs : https://michael.stapelberg.ch/posts/2020-01-21-initramfs-from-scratch-golang/

## 这个是做什么的
wget http://cdimage.ubuntu.com/ubuntu-base/releases/22.04/release/ubuntu-base-22.04-base-arm64.tar.gz

## 这个看看
https://github.com/marcov/firecracker-initrd/blob/master/guest/boot_done.c

## 这里下载的 squashfs 似乎也是一直没整理的
https://github.com/firecracker-microvm/firecracker/blob/main/docs/getting-started.md

还有，似乎 rootfs 这个东西一直没有仔细研究过

现在 firecracker 还有一个问题，为什么通过 rootfs ，他就是不需要 initramfs 了啊

当然，一个更加逆天的方法就是，mount qcow2 ，然后直接修改其中的内容。


## 当进入到 emergency mode 的时候，原来当时的文件系统是这个
:/root# cat /proc/mounts
rootfs / rootfs rw,size=1989988k,nr_inodes=497497 0 0

## 作为参考 firecracker/docs/initrd.md

理解一下这些:

> - You should not use a drive with `is_root_device: true` when using an initrd
> - Make sure your kernel configuration has `CONFIG_BLK_DEV_INITRD=y`
> - If you don't want to place your init at the root of your initrd, you can add
>   `rdinit=/path/to/init` to your `boot_args` property
> - If you intend to `pivot_root` in your init, it won't be possible because the
>   initrd is mounted as a rootfs and cannot be unmounted. You will need to use
>   `switch_root` instead.

## 工具
https://git.yoctoproject.org/yocto-kernel-tools

## 参考一下这里的技术，也许可以直接构建 iso 出来
https://github.com/archlinux/arch-boxes

或者使用这个方法来虚拟机出来。

## 这个看看，有趣的东西
https://mergeboard.com/blog/2-qemu-microvm-docker/

## 不错
C 如何编译出一个不需要操作系统的程序？ - 韦易笑的回答 - 知乎
https://www.zhihu.com/question/49580321/answer/287557834

## 既然有 go 的 busybox ，那么就一定有 rust busybox 的
https://github.com/u-root/u-root

## 这两个的区别似乎一直没搞清楚
initramfs

作用：取代 ramdisk 的新机制，基于 cpio 打包内存文件系统。
特点：
内核直接解压到内存，不需要块设备。
内容直接变成 rootfs。

内核代码位置：init/initramfs.c



initrd / initramdisk
作用：早期机制，挂载一个临时 ramdisk 作为根文件系统，然后再 pivot_root 到真实 rootfs。
特点：较老，已逐步被 initramfs 取代。
使用方式：root=/dev/ram0 initrd=...内核代码位置：init/do_mounts_initrd.c

## 进入到 initramfs 中，的确可以看到 rootfs 啊

开机立刻进入才 emergency mode ，然后 cat /proc/mounts

## 看看内核自己改如何支持 initramfs 的

有一个 initramfs 文件系统来处理解压问题?
需要用来 mount 一个问题?

## 思考一个问题，为什么 initramfs 中包含网卡

也就是 initrd 中需要继续分析网卡的

## 话说 initramfs 和 squashfs 什么区别来着
<!-- e5e11ce8-0b9d-471e-95ce-21d58cc3058a -->

在 kexec 的时候，似乎是需要 squashfs 了
```txt
mount: /squash/root: fsconfig system call failed: Filesystem uses "zstd" compression. This is not supported.
       dmesg(1) may have more information after failed mount system call.
overlayfs: upper fs does not support RENAME_WHITEOUT.
overlayfs: failed to set xattr on upper
overlayfs: ...falling back to redirect_dir=nofollow.
overlayfs: ...falling back to uuid=null.
mount: /newroot/squash: mount point does not exist.
       dmesg(1) may have more information after failed mount system call.
switch_root: cannot access /init: No such file or directory
switch_root: failed to execute /init: No such file or directory
Kernel panic - not syncing: Attempted to kill init! exitcode=0x00007f00
CPU: 0 UID: 0 PID: 1 Comm: switch_root Not tainted 6.18.3-00001-gd99e6e036338-dirty #40 PREEMPT(none)
```

## 看看这个 config 的含义是什么?
CONFIG_BLK_DEV_INITRD

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
