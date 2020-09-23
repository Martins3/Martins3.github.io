# qemu


## 想法
1. qemu 是如何实现模拟设备的
2. qemu 和 gdb 的配合调试
1. 如何 enable kvm
2. 如何解决网络
3. 是否存在加速图形显示的操作



## 资源
Learn linux kernel with qemu:
1. https://github.com/dhruvvyas90/qemu-rpi-kernel : 在 qemu 中间运行 raspibary
2. https://github.com/geohot/qira : 不知道这调试器为什么和 qemu 有关的

这个项目的确提供了如何使用 qmeu 的内容:
https://github.com/tinyclub/linux-0.11-lab

## busy box 和 buildroot 的关系是什么 ?
https://stackoverflow.com/questions/19532564/linux-kernel-development-using-qemu


## 如何使用
https://wiki.archlinux.org/index.php/QEMU


## qemu 的操作

https://multipass.run/ 和 kvm 没有关系，而是 cloud 的问题。

alias kvm-ldd='kvm -m 8192 -smp 4 -hda ~/Downloads/VM/kvm_img/ubuntu.qcow2 -cdrom ~/Downloads/VM/kvm_img/ubuntu-5.04-install-i386.iso'

> 找到 -hda 的参数含义

还可以制定内核参数啊 !

----------------

岂不是下一步，直接操作就可以了:
https://mudongliang.github.io/2017/09/21/install-ubuntu-in-qemu.html

当进入到需要Ubuntu等待删除该内容的时候，直接去除即可。

qemu-system-x86_64 -hda Ubuntu20.img -smp 4  -m 8G -enable-kvm

----------------------

一步步的解释:
http://nickdesaulniers.github.io/blog/2018/10/24/booting-a-custom-linux-kernel-in-qemu-and-debugging-it-with-gdb/

https://xapax.github.io/blog/2017/05/09/sharing-files-kvm.html 一种mount 共享的方法

https://michael.stapelberg.ch/posts/2020-01-21-initramfs-from-scratch-golang/ : 内核中间如何处理　initramfs

https://www.collabora.com/news-and-blog/blog/2017/01/16/setting-up-qemu-kvm-for-kernel-development/ : 似乎唯一科学的想法





## 问题
1. 为什么其他人可以完全不配置 initrd
    1. 并不理解 initrd 是什么?
    2. 是不是因为 vmlinuz 和 bzImage 的关系 ?
        3. hack 使用就是 bzImage，还是没有需要 initrd 啊
    3. 和 rootfs 和 initrd 的关系是什么 ?
2. 阅读 qemu 的 man

3. 同样是 bzImage，利用 hack linux kernel 并不能启动，刺激啊!

4. 找到如何正确生成 initramfs 的方法
    1. initramfs 需要和内核配套吗 ?
    2. 自己制作一个

5. 真的存在 uuid kernel cmdline : 似乎有，还是存在关系的
6. -derive 和 -hda 存在区别
  7. 安装多个内核，如何选择

8. 手动安装一个低版本的内容，用于验证一下qemu启动是不是可以随意制定内核

9. 查一下为什么 linux57 做出来的 bzImage，切换成为直接编译的结果之后，结果还是无法 mount

10. 忽然发现 vmlinux => bzImage 是核心功能，各种驱动并没有编译到内核中间。


内核 => img 对应的
1. 接口不兼容的位置 ? 不应该啊

https://www.linux-kvm.org/page/Boot_from_virtio_block_device : 难道是这个的问题，并不是

- 当将系统的 bzImage 和 initramfs 加入之后，内核是崩溃的，但是此时还是可以获取到一个shell



## 笔记

```
   Linux/Multiboot boot specific
       When using these options, you can use a given Linux or Multiboot kernel without installing it in the disk image. It can be useful for easier testing of various kernels.

       -kernel bzImage
              Use bzImage as kernel image. The kernel can be either a Linux kernel or in multiboot format.

       -append cmdline
              Use cmdline as kernel command line

       -initrd file
              Use file as initial ram disk.

       -initrd file1 arg=foo,file2
              This syntax is only available with multiboot.

              Use file1 and file2 as modules and pass arg=foo as parameter to the first module.

       -dtb file
              Use file as a device tree binary (dtb) image and pass it to the kernel on boot.
```

## 如何使用 qemu 来调试内核

https://www.collabora.com/news-and-blog/blog/2017/03/13/kernel-debugging-with-qemu-overview-tools-available/



## todo : 找到生成 initramfs 的工具

将会是，dkms 吗 ?

```
Creating gzip-compressed initcpio image: /boot/initramfs-4.4-x86_64.img
==> Image generation successful
==> Building image from preset: /etc/mkinitcpio.d/linux44.preset: 'fallback'
  -> -k /boot/vmlinuz-4.4-x86_64 -c /etc/mkinitcpio.conf -g /boot/initramfs-4.4-x86_64-fallback.img -S autodetect
==> Starting build: 4.4.225-1-MANJARO
  -> Running build hook: [base]
  -> Running build hook: [udev]
  -> Running build hook: [modconf]
  -> Running build hook: [block]
  -> Running build hook: [keyboard]
  -> Running build hook: [keymap]
  -> Running build hook: [filesystems]
==> Generating module dependencies
==> Creating gzip-compressed initcpio image: /boot/initramfs-4.4-x86_64-fallback.img
==> Image generation successful
==> Building image from preset: /etc/mkinitcpio.d/linux56.preset: 'default'
  -> -k /boot/vmlinuz-5.6-x86_64 -c /etc/mkinitcpio.conf -g /boot/initramfs-5.6-x86_64.img
==> Starting build: 5.6.15-1-MANJARO
  -> Running build hook: [base]
  -> Running build hook: [udev]
  -> Running build hook: [autodetect]
  -> Running build hook: [modconf]
  -> Running build hook: [block]
  -> Running build hook: [keyboard]
  -> Running build hook: [keymap]
  -> Running build hook: [filesystems]
==> Generating module dependencies
==> Creating gzip-compressed initcpio image: /boot/initramfs-5.6-x86_64.img
==> Image generation successful
==> Building image from preset: /etc/mkinitcpio.d/linux56.preset: 'fallback'
  -> -k /boot/vmlinuz-5.6-x86_64 -c /etc/mkinitcpio.conf -g /boot/initramfs-5.6-x86_64-fallback.img -S autodetect
==> Starting build: 5.6.15-1-MANJARO
  -> Running build hook: [base]
  -> Running build hook: [udev]
  -> Running build hook: [modconf]
  -> Running build hook: [block]

```
