## x86-bare-metal-examples
项目地址 : https://github.com/cirosantilli/x86-bare-metal-examples

- [ ] 启动，从 bios 到 boot sector 到 coreboot 到 grub 到 linuz ?

### question
- [ ]  `make -C printf run`
```sh
qemu-system-i386 -drive file='$(MAIN)',format=raw
```
boot sector 存在标准格式的，bios 来执行这个代码。
那么在现代的 linux 中间，bios 会从 partition 的第一个 sector, usb , 以及 网卡中间寻找 bootloader[^1],
这个 bootloader 就是 grub。

- [x] grub 如果负责 linux 加载，他靠什么读 disk ? 应该是 bios 中间的
- [x] coreboot 是什么定位 ? BIOS / UEFI
- [x] initrd / initramdisk 是做什么 ? 猜测是，靠 grub 加载进来的，在内存的文件系统，然后内核靠他将真正的文件系统 mount 进来
    - 但是此时还是没有 驱动 ？

> The job of the boot loader is to begin the next phase, loading the kernel and an initial ram disk filesystem.

> To keep kernels to a reasonable size and permit separate modules for separate hardware, modern kernels also use a file system which is present in memory, called an 'initrd' for 'initial ram disk'.

> The kernel launches the init script inside the initrd file system, which loads hardware drivers and finds the root partition.

多个 partition 都可以作为 boot sector 是因为 disk 的第一个 sector 存储 MBR[^1] 

# boot 

https://stackoverflow.com/questions/19201579/using-qemu-to-boot-opensuse-or-any-other-os-with-custom-kernel

> bzImage 是什么 ?
> initrd ?

CPIO

https://news.ycombinator.com/item?id=24208663 : arm 启动

[^1]: https://wiki.ubuntu.com/Booting
