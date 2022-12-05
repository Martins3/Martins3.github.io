# 我所知道 grub 的全部

## 为什么需要 bootloader
在计算上电的时候，首先会执行 ROM 中代码，这个 ROM 代码将将代码加载到内存中，然后进行各种初始化，这些程序以前是 BIOS，现在是 UEFI，如果有兴趣，可以看看 UEFI 的开源实现 edk2 .
而 grub 是一个普通的 UEFI 程序，其作用是将内核从存放在磁盘，nvme 或者网络中内核镜像加载到内存中。

所以，为什么需要 bootloader，应为内存是 volatile 的，需要从 non-volatile 的环境中搬运到内存中，然后跳转到内核的执行的入口处。

## 为什么 grub 还需要加载 initramfs
grub 的配置文件一般在 /boot/grub2/grub.cfg 中，下面为我的 NixOS 中复制出来的一部分:
```txt
menuentry "NixOS - Configuration 16 (2022-07-26 - 22.05.1322.915f5a5b3cc)" --class nixos {
search --set=drive1 --fs-uuid 597f2eef-d2cb-4e08-9fef-649a6894a159
search --set=drive2 --fs-uuid 597f2eef-d2cb-4e08-9fef-649a6894a159
  linux ($drive2)/nix/store/mql2jpaz1810pc8dchh2yp6hj8g8xwhf-linux-5.18.6/bzImage init=/nix/store/2fpn5g4nrmlydvq6rmfz2ap7b3r6d1dl-nixos-system-nixos-22.05.1322.915f5a5b3cc/init loglevel=4 c
rashkernel=128M nmi_watchdog=panic softlockup_panic=1
  initrd ($drive2)/nix/store/xbj9xgyjnrfkkmkjgms9yif5m4fpf3sm-initrd-linux-5.18.6/initrd
}
```
其中:
- linux 是 grub 加载内核 bzImage 的命令，后面还携带了传递给 kernel 的参数
- initrd 是 grub 加载 initramfs 的

为什么加载内核之后，为什么还需要加载 initramfs ?

一个很重要的原因是鸡生蛋的问题:
1. 没有必要将所有的模块都塞到 kernel 中，而使用一个个的内核模块，降低内核的耦合程度。
2. 内核的根文件系统实际上非常灵活，可能是 ext4, btrfs 等，其所在的介质可能是网络上，也可能是在磁盘上，这写通路上可能涉及的驱动太多了。

假如你现在有一个机器 A ，其 root 是在 nvme 上，文件系统是 xfs，那么就可以让 initramfs 中持有 nvme 和 xfs 驱动。
没有必要将所有的东西都塞到 initramfs 中，如何根据当前机器的情况制作出来正确的 initramfs，那就是靠 dracut 了。

### dracut
- 源码: https://github.com/dracutdevs/dracut
  - 文档: https://github.com/dracutdevs/dracut/blob/master/man/dracut.usage.asc

通过 dracut 可以检查当前系统正在使用的 initramfs 的中包含的包:
```sh
dracut --list-modules
```

还可以手动地增加
```sh
dracut --add-drivers mymod initramfs-with-mymod.img
```

- https://www.suse.com/support/kb/doc/?id=000019945 : 配置 drucut 来实现假如模块
- [How to unpack/uncompress and repack/re-compress an initial ramdisk (initrd/initramfs) boot image file on RHEL 5,6 ?](https://access.redhat.com/solutions/24029)

## grubby
如果你经常需要修改内核参数，不停使用 vim 打开 /boot/grub2/grub.cfg ，很麻烦，无法脚本化。

你可以使用 grubby 来更加方便的修改 grub.cfg 中 kernel 参数，一些使用案例参考此处:

https://www.golinuxcloud.com/grubby-command-examples/

## 其他小问题
1. grub-mkconfig || grub2-mkconfig : 扫描 /boot 然后更新 grub.conf
2. 从 grub rescue 模式下恢复: https://linuxhint.com/grub_rescue_ubuntu_1804/
  - 其实原理很简单，本来 grub 找不到配置文件，现在我们手动设置要加载的内核和 initrd，然后启动系统后，更新 grub.cfg

## 替代品
- https://github.com/limine-bootloader/limine
- https://askubuntu.com/questions/760875/any-downside-to-using-refind-instead-of-grub

## 更多

- 你可以使用 QEMU 来调试 grub :
  - https://stackoverflow.com/questions/31799336/how-to-build-grub2-bootloader-from-its-source-and-test-it-with-qemu-emulator
- 如何自己手写一个 bootloader
  - https://news.ycombinator.com/item?id=33360830


[^1]: https://askubuntu.com/questions/1288580/sudo-grub2-mkconfig-command-not-found

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
